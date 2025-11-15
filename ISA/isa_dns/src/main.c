#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "response_dns.h"
#include "parse_dns.h"
#include "read_filter.h"
#include "argument_parse.h"
#include "utils_dns.h"


int main(int argc, char **argv) {
    args_t args;
    parse_args(argc, argv, &args);

    load_filter(args.filter_file, &args.verbose);

    // socket klient → server
    int s_listen = socket(AF_INET, SOCK_DGRAM, 0);
    if (s_listen < 0) { 
        perror("socket"); exit(1); 
    }

    // socket server → resolver
    int s_res = socket(AF_INET, SOCK_DGRAM, 0);
    if (s_res < 0) { 
        perror("resolver socket"); 
        exit(1); 
    }

    // timeout pro resolver
    struct timeval tv = {0, 500000};
    setsockopt(s_res, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // resolver adresa
    struct sockaddr_in raddr;
    memset(&raddr, 0, sizeof(raddr));
    raddr.sin_family = AF_INET;
    raddr.sin_port   = htons(53);

    struct in_addr ip4;

    if (!resolve_ipv4(args.resolver_ip, &ip4)) {
        exit(1); 
    }

    raddr.sin_addr = ip4;

    // bind serverového portu
    struct sockaddr_in laddr;
    memset(&laddr, 0, sizeof(laddr));
    laddr.sin_family = AF_INET;
    laddr.sin_port   = htons(args.port);
    laddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s_listen, (struct sockaddr*)&laddr, sizeof(laddr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (args.verbose)
        fprintf(stderr, "[INFO] listening on port %d\n", args.port);

    unsigned char buf[512];

    while (1) {
        struct sockaddr_in client;
        socklen_t clen = sizeof(client);

        int n = recvfrom(s_listen, buf, sizeof(buf), 0,
                         (struct sockaddr*)&client, &clen);

        if (n <= 0)
            continue;

        process_client_query(
            s_listen,    // socket → klient
            s_res,       // socket → resolver
            buf,         // packet
            n,           // velikost
            &client,     // adresa klienta
            clen,
            &raddr,      // resolver adresa
            args.verbose // verbose mód
        );
    }

    return 0;
}