#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>   // socket(), SOCK_DGRAM, SOL_SOCKET, etc.
#include <netinet/in.h>   // struct sockaddr_in, AF_INET
#include <arpa/inet.h>    // inet_pton
#include <netdb.h>        // getaddrinfo, struct addrinfo

#include "utils_dns.h"
#include <string.h>
#include "utils_dns.h"

int resolve_ipv4(const char *input, struct in_addr *out_ip) {
    // 1) Zkusit IPv4 literal (např. 8.8.8.8)
    if (inet_pton(AF_INET, input, out_ip) == 1) {
        return 1;   // OK
    } else {
        // 2) Zkusit DNS name (např. dns.google)
        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_INET;      // jen IPv4
        hints.ai_socktype = SOCK_DGRAM;   // UDP

        int err = getaddrinfo(input, NULL, &hints, &res);
        if (err != 0) {
            fprintf(stderr, "DNS resolver lookup failed for '%s': %s\n",
                    input, gai_strerror(err));
            return 0; 
        }

        struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
        *out_ip = addr->sin_addr;

        freeaddrinfo(res);
        return 1;
    }    
    return 0;
}