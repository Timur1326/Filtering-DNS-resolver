
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "response_dns.h"


void send_simple_rcode(int sock, unsigned char *query, int qlen, int rcode, struct sockaddr_in *cl, socklen_t clen)
{
    unsigned char resp[512];
    memset(resp, 0, sizeof(resp));

    resp[0] = query[0];
    resp[1] = query[1];

    unsigned char rd = (query[2] & 1);

    resp[2] = 0x80 | rd;
    resp[3] = rcode & 0x0F;

    resp[4] = query[4];
    resp[5] = query[5];

    int off = 12;
    while (off < qlen && query[off] != 0)
        off++;
    off++;
    off += 4;

    memcpy(resp + 12, query + 12, off - 12);

    sendto(sock, resp, off, 0, (struct sockaddr*)cl, clen);
}