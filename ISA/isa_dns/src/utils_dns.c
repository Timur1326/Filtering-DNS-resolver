/* 
    Author: Nurtdinov Timur   
    Login:  xnurtd00
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>   
#include <netinet/in.h>   
#include <arpa/inet.h>    
#include <netdb.h>        

#include "utils_dns.h"
#include <string.h>
#include "utils_dns.h"

/**
 * @brief Resolve input string to IPv4 address
 * 
 * @param input 
 * @param out_ip 
 * @return int 
 */
int resolve_ipv4(const char *input, struct in_addr *out_ip) {
    // Check if input is valid IPv4 address , else resolve DNS to IPv4
    if (inet_pton(AF_INET, input, out_ip) == 1) {
        return 1;   // it is valid IPv4 address
    } else {
        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_INET;     
        hints.ai_socktype = SOCK_DGRAM;  

        // get address info from DNS
        int err = getaddrinfo(input, NULL, &hints, &res);
        if (err != 0) {
            fprintf(stderr, "DNS resolver lookup failed for '%s': %s\n", input, gai_strerror(err));
            return 0; 
        }

        struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
        *out_ip = addr->sin_addr;

        freeaddrinfo(res);
        return 1;
    }    
    return 0;
}