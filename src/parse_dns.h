/* 
    Author: Nurtdinov Timur   
    Login:  xnurtd00
*/
#ifndef PARSE_DNS_H
#define PARSE_DNS_H

#include <stdio.h>
#include <netdb.h>
#include "response_dns.h"
#include "read_filter.h"

// DNS type A
#define A_TYPE 1
// RCODE values
#define FORM_ERR_RCODE 1
#define SER_FAIL_RCODE 2
#define NOTIMP_RCODE 4
#define REFUSED_RCODE 5

/**
 * @brief DNS question structure
 *  qname - Queried domain name
 *  qtype - Query type
 *  qclass - Query class
 *  valid - Validity flag
 */
typedef struct {
    char qname[256];
    uint16_t qtype;
    uint16_t qclass;
    int valid;
} dns_question_t;


void process_client_query(int socket_client, int socket_resolver, unsigned char *query, int size, struct sockaddr_in *client_addr, socklen_t client_len, struct sockaddr_in *resolver_addr, int verbose);

#endif