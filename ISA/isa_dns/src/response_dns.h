#ifndef RESPONSE_DNS_H
#define RESPONSE_DNS_H

#include <netinet/in.h>

void send_simple_rcode(int sock, unsigned char *query, int qlen, int rcode, struct sockaddr_in *cl, socklen_t clen);

#endif