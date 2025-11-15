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


static int parse_qname(unsigned char *pkt, int off, char *out, int max) {
    int pos = off;
    int idx = 0;

    while (1) {
        unsigned char len = pkt[pos++];
        if (len == 0) {
            out[idx] = 0;
            return pos;
        }

        if (len > 63)
            return -1;

        if (idx && idx < max - 1)
            out[idx++] = '.';

        for (int i = 0; i < len && idx < max - 1; i++)
            out[idx++] = pkt[pos++];
    }
}


int blocked(const char *q) {
    for (int i = 0; i < blacklist.count; i++) {
        const char *bad = blacklist.list[i];

        int ql = strlen(q);
        int bl = strlen(bad);

        if (ql < bl) continue;

        if (strcasecmp(q + (ql - bl), bad) == 0) {
            if (ql == bl || q[ql - bl - 1] == '.')
                return 1;
        }
    }
    return 0;
}


static int validate_dns_query(unsigned char *query, int qlen) {
    if (qlen < 12)
        return 0;

    uint16_t qdcount = (query[4] << 8) | query[5];
    if (qdcount != 1)
        return 0;

    return 1;
}



static dns_question_t extract_question(unsigned char *query, int qlen) {
    dns_question_t q;
    memset(&q, 0, sizeof(q));

    int offset = 12;

    int new_offset = parse_qname(query, offset, q.qname, sizeof(q.qname));
    if (new_offset < 0)
        return q;

    offset = new_offset;

    if (offset + 4 > qlen)
        return q;

    q.qtype  = (query[offset] << 8) | query[offset+1];
    q.qclass = (query[offset+2] << 8) | query[offset+3];
    q.valid = 1;

    return q;
}

static int is_question_blocked(dns_question_t *q) {
    return blocked(q->qname);
}

static int forward_to_resolver(int resolver_sock, unsigned char *query, int qlen, struct sockaddr_in *resolver_addr, unsigned char *resp_out, int *resp_len) {
    sendto(resolver_sock, query, qlen, 0,
           (struct sockaddr*)resolver_addr, sizeof(*resolver_addr));

    struct sockaddr_in from;
    socklen_t flen = sizeof(from);

    int rn = recvfrom(resolver_sock, resp_out, 512, 0,
                      (struct sockaddr*)&from, &flen);

    if (rn <= 0)
        return 0;  

    *resp_len = rn;
    return 1;
}

void process_client_query(int listen_sock, int resolver_sock, unsigned char *query, int qlen, struct sockaddr_in *client, socklen_t clen, struct sockaddr_in *resolver_addr, int verbose)
{
    // 1) Проверка валидности DNS-запроса
    if (!validate_dns_query(query, qlen))
        return; // silently ignore

    // 2) Извлечение вопроса
    dns_question_t q = extract_question(query, qlen);
    if (!q.valid)
        return;

    if (verbose)
        fprintf(stderr, "[INFO] Q: %s (type=%u)\n", q.qname, q.qtype);

    // 3) Тип != A → NOTIMP
    if (q.qtype != A_TYPE) {
        send_simple_rcode(listen_sock, query, qlen, NOTIMP_RCODE, client, clen);
        return;
    }

    // 4) Заблокированный домен → REFUSED
    if (is_question_blocked(&q)) {
        send_simple_rcode(listen_sock, query, qlen, REFUSED_RCODE, client, clen);
        return;
    }

    // 5) Форвард в резолвер
    unsigned char resp[512];
    int rn = 0;

    if (!forward_to_resolver(resolver_sock, query, qlen,
                             resolver_addr, resp, &rn))
    {
        // SERVFAIL, если резолвер не ответил
        send_simple_rcode(listen_sock, query, qlen, SER_FAIL_RCODE, client, clen);
        return;
    }

    // 6) Проверка ID (не наш пакет → пропускаем)
    if (resp[0] != query[0] || resp[1] != query[1])
        return;

    // 7) Вернуть клиенту
    sendto(listen_sock, resp, rn, 0,
           (struct sockaddr*)client, clen);
}