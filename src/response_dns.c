/* 
    Author: Nurtdinov Timur   
    Login:  xnurtd00
*/
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "response_dns.h"


/**
 * @brief Send DNS response with specified RCODE to client 
 * 
 * @param sock 
 * @param query 
 * @param qlen 
 * @param rcode 
 * @param cl 
 * @param clen 
 */
void send_rcode(int sock, unsigned char *query, int qlen, int rcode, struct sockaddr_in *cl, socklen_t clen)
{
    unsigned char resp[512];
    memset(resp, 0, sizeof(resp));

    // copy transaction ID
    resp[0] = query[0];
    resp[1] = query[1];

    // copy RD flag
    unsigned char rd = (query[2] & 1);

    // form response header
    resp[2] = 0x80 | rd;
    resp[3] = rcode & 0x0F;

    // copy QDCOUNT
    resp[4] = query[4];
    resp[5] = query[5];

    // QNAME 
    int head = 12;
    while (head < qlen && query[head] != 0)
        head++;
    head++;
    head += 4;
    // copy question section
    memcpy(resp + 12, query + 12, head - 12);

    // send response to client
    sendto(sock, resp, head, 0, (struct sockaddr*)cl, clen);
}