#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "parse_dns.h"
#include "response_dns.h"
#include "read_filter.h"



static int parse_qname(unsigned char *packet, int offset, char *output, int output_size)
{
    int pos = offset;
    int out_pos = 0;

    while (1) {
        unsigned char label_len = packet[pos++];

        // end of QNAME
        if (label_len == 0) {
            output[out_pos] = '\0';
            return pos;
        }

        // invalid: label too long
        if (label_len > 63)
            return -1;

        // add dot before next label
        if (out_pos > 0 && out_pos < output_size - 1) {
            output[out_pos++] = '.';
        }

        // copy label
        for (int i = 0; i < label_len && out_pos < output_size - 1; i++) {
            output[out_pos++] = packet[pos++];
        }
    }
}
 
int domain_blocked(const char *domain)
{
    for (int i = 0; i < blacklist.count; i++) {
        const char *blocked_domain = blacklist.list[i];

        int domain_len = strlen(domain);
        int blocked_len = strlen(blocked_domain);

        if (domain_len < blocked_len)
            continue;

        if (strcasecmp(domain + (domain_len - blocked_len), blocked_domain) == 0) {
            if (domain_len == blocked_len ||
                domain[domain_len - blocked_len - 1] == '.')
            {
                return 1;
            }
        }
    }

    return 0;
}

static int validate_dns_query(unsigned char *query, int size) {
    if (size < 12)
        return 0;

    uint16_t qdcount = (query[4] << 8) | query[5];
    if (qdcount != 1)
        return 0;

    return 1;
}



static dns_question_t extract_question(unsigned char *query, int size) {
    dns_question_t question;
    memset(&question, 0, sizeof(question));

    int offset = 12;

    int new_offset = parse_qname(query, offset, question.qname, sizeof(question.qname));
    if (new_offset < 0) {
        return question;
    }

    offset = new_offset;

    if (offset + 4 > size) {
        return question;
    }

    question.qtype  = (query[offset] << 8) | query[offset+1];
    question.qclass = (query[offset+2] << 8) | query[offset+3];
    question.valid = 1;

    return question;
}

static int is_question_blocked(dns_question_t *q) {
    return domain_blocked(q->qname);
}

static int forward_to_resolver(int socket_resolver, unsigned char *query, int size, struct sockaddr_in *resolver_addr, unsigned char *resp_out, int *resp_len) {
    sendto(socket_resolver, query, size, 0,
           (struct sockaddr*)resolver_addr, sizeof(*resolver_addr));

    struct sockaddr_in from;
    socklen_t flen = sizeof(from);

    int byte = recvfrom(socket_resolver, resp_out, 512, 0,
                      (struct sockaddr*)&from, &flen);

    if (byte <= 0)
        return 0;  

    *resp_len = byte;
    return 1;
}

void process_client_query(int socket_client, int socket_resolver, unsigned char *query, int size, struct sockaddr_in *client_addr, socklen_t client_len, struct sockaddr_in *resolver_addr, int verbose) {
    // 1) Проверка валидности DNS-запроса
    if (!validate_dns_query(query, size)) {
        return; // silently ignore
    }

    // 2) Извлечение вопроса
    dns_question_t q = extract_question(query, size);
    if (!q.valid) {
        return;
    }

    if (verbose) {
        fprintf(stderr, "[+] %s (type=%u)\n", q.qname, q.qtype);
    }
    
    // 3) Тип != A → NOTIMP
    if (q.qtype != A_TYPE) {
        send_rcode(socket_client, query, size, NOTIMP_RCODE, client_addr, client_len);
        return;
    }

    // 4) Заблокированный домен → REFUSED
    if (is_question_blocked(&q)) {
        send_rcode(socket_client, query, size, REFUSED_RCODE, client_addr, client_len);
        return;
    }

    // 5) Форвард в резолвер
    unsigned char resp[512];
    int rn = 0;

    if (!forward_to_resolver(socket_resolver, query, size, resolver_addr, resp, &rn)) {
        // SERVFAIL, если резолвер не ответил
        send_rcode(socket_client, query, size, SER_FAIL_RCODE, client_addr, client_len);
        return;
    }

    // 6) Проверка ID (не наш пакет → пропускаем)
    if (resp[0] != query[0] || resp[1] != query[1]) {
        return;
    }

    sendto(socket_client, resp, rn, 0, (struct sockaddr*)client_addr, client_len);
}