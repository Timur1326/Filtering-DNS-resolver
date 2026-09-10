/* 
    Author: Nurtdinov Timur   
    Login:  xnurtd00
*/
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


/**
 * @brief This function parse QNAME from DNS packet and store it in output
 * 
 * @param packet 
 * @param offset 
 * @param output 
 * @param output_size 
 * @return int 
 */
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

        // label length too big
        if (label_len > 63)
            return -1;

        // check output size
        if (out_pos > 0 && out_pos < output_size - 1) {
            output[out_pos++] = '.';
        }

        // copy label to output
        for (int i = 0; i < label_len && out_pos < output_size - 1; i++) {
            output[out_pos++] = packet[pos++];
        }
    }
}
 

/**
 * @brief Check if a domain is blocked based on the filter list
 * 
 * @param domain 
 * @return int 
 */
int domain_blocked(const char *domain)
{
    // Cycle through each blocked domain in the filter list
    for (int i = 0; i < filter.count; i++) {
        const char *blocked_domain = filter.list[i];

        int domain_len = strlen(domain);
        int blocked_len = strlen(blocked_domain);
        // If the domain is shorter than the blocked domain, skip
        if (domain_len < blocked_len)
            continue;

        // Check if the end of the domain matches the blocked domain
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

/**
 * @brief Validate the DNS query for basic correctness
 * 
 * @param query 
 * @param size 
 * @return int 
 */
static int validate_dns_query(unsigned char *query, int size) {
    // Minimum DNS header size is 12 bytes
    if (size < 12)
        return 0;

    // Check that it's a standard query 
    uint16_t qdcount = (query[4] << 8) | query[5];
    if (qdcount != 1)
        return 0;

    return 1;
}


/**
 * @brief Extract the DNS question section from the query
 * 
 * @param query 
 * @param size 
 * @return dns_question_t 
 */
static dns_question_t extract_question(unsigned char *query, int size) {
    dns_question_t question;
    memset(&question, 0, sizeof(question));

    // DNS header is 12 bytes
    int offset = 12;

    // new offset after parsing QNAME
    int new_offset = parse_qname(query, offset, question.qname, sizeof(question.qname));
    if (new_offset < 0) {
        return question;
    }

    offset = new_offset;

    if (offset + 4 > size) {
        return question;
    }

    // extract QTYPE and QCLASS from the question section
    question.qtype  = (query[offset] << 8) | query[offset+1];
    question.qclass = (query[offset+2] << 8) | query[offset+3];
    question.valid = 1;

    //return the question
    return question;
}

/**
 * @brief Check if the DNS question is blocked based on the filter list
 * 
 * @param q 
 * @return int 
 */
static int is_question_blocked(dns_question_t *q) {
    return domain_blocked(q->qname);
}


/**
 * @brief Forward the DNS query to the resolver and receive the response
 * 
 * @param socket_resolver 
 * @param query 
 * @param size 
 * @param resolver_addr 
 * @param resp_out 
 * @param resp_len 
 * @return int 
 */
static int forward_to_resolver(int socket_resolver, unsigned char *query, int size, struct sockaddr_in *resolver_addr, unsigned char *resp_out, int *resp_len) {
    // send query to resolver
    sendto(socket_resolver, query, size, 0,(struct sockaddr*)resolver_addr, sizeof(*resolver_addr));

    struct sockaddr_in from;
    socklen_t flen = sizeof(from);

    // receive response from resolver
    int byte = recvfrom(socket_resolver, resp_out, 512, 0,(struct sockaddr*)&from, &flen);

     // byte <= 0 means timeout or error
    if (byte <= 0)
        return 0;  

    // set response length
    *resp_len = byte;
    return 1;
}


/**
 * @brief This function process client DNS query and send response back to client
 * 
 * @param socket_client 
 * @param socket_resolver 
 * @param query 
 * @param size 
 * @param client_addr 
 * @param client_len 
 * @param resolver_addr 
 * @param verbose 
 */
void process_client_query(int socket_client, int socket_resolver, unsigned char *query, int size, struct sockaddr_in *client_addr, socklen_t client_len, struct sockaddr_in *resolver_addr, int verbose) {
    // validate DNS query
    if (!validate_dns_query(query, size)) {
        return; 
    }

    // extract DNS question
    dns_question_t q = extract_question(query, size);
    // check if question is valid 
    if (!q.valid) {
        return;
    }

    if (verbose) {
        fprintf(stderr, "[+] %s (type=%u)\n", q.qname, q.qtype);
    }
    
    // only A type
    if (q.qtype != A_TYPE) {
        send_rcode(socket_client, query, size, NOTIMP_RCODE, client_addr, client_len);
        return;
    }

    // check if question is blocked
    if (is_question_blocked(&q)) {
        send_rcode(socket_client, query, size, REFUSED_RCODE, client_addr, client_len);
        return;
    }

    
    unsigned char resp[512];
    int rn = 0;
    // forward query to resolver
    if (!forward_to_resolver(socket_resolver, query, size, resolver_addr, resp, &rn)) {
        send_rcode(socket_client, query, size, SER_FAIL_RCODE, client_addr, client_len);
        return;
    }

    // response ID must match query ID
    if (resp[0] != query[0] || resp[1] != query[1]) {
        return;
    }

    // send response back to client
    sendto(socket_client, resp, rn, 0, (struct sockaddr*)client_addr, client_len);
}