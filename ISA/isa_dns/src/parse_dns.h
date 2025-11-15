#ifndef PARSE_DNS_H
#define PARSE_DNS_H

#include <stdio.h>
#include <netdb.h>
#include "response_dns.h"
#include "read_filter.h"


#define A_TYPE 1

#define SER_FAIL_RCODE 2
#define NOTIMP_RCODE 4
#define REFUSED_RCODE 5

typedef struct {
    char qname[256];
    uint16_t qtype;
    uint16_t qclass;
    int valid;
} dns_question_t;


void process_client_query(int listen_sock, int resolver_sock, unsigned char *query, int qlen, struct sockaddr_in *client, socklen_t clen, struct sockaddr_in *resolver_addr, int verbose);

#endif