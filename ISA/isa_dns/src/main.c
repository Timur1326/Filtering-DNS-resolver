#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>  
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "response_dns.h"
#include "parse_dns.h"
#include "read_filter.h"
#include "argument_parse.h"
#include "utils_dns.h"


volatile sig_atomic_t stop = 0;

void handle_signal(int sig) {
    (void)sig;    
    stop = 1;     
}

int main(int argc, char **argv) {

    signal(SIGINT, handle_signal);

    args_t args;

    parse_args(argc, argv, &args);

    load_filter(args.filter_file, &args.verbose);

    // socket klient → server
    int socket_client = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_client < 0) { 
        perror("socket"); exit(1); 
    }

    // socket server → resolver
    int socket_resolver = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_resolver < 0) { 
        perror("resolver socket"); 
        exit(1); 
    }

    // timeout pro resolver
    struct timeval timeout = {0, 500000};
    setsockopt(socket_resolver, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(socket_client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // resolver adresa
    struct sockaddr_in res_addr;
    memset(&res_addr, 0, sizeof(res_addr));
    res_addr.sin_family = AF_INET;
    res_addr.sin_port   = htons(53);

    struct in_addr ip4;

    if (!resolve_ipv4(args.resolver_ip, &ip4)) {
        exit(1); 
    }

    res_addr.sin_addr = ip4;

    // bind serverového portu
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port   = htons(args.port);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_client, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (args.verbose)
        fprintf(stderr, "[+] listening on port %d\n", args.port);

    unsigned char buf[512];

    while (!stop) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int size = recvfrom(socket_client, buf, sizeof(buf), 0,
                         (struct sockaddr*)&client_addr, &client_len);

        if (size <= 0)
            continue;

        process_client_query(
            socket_client,    // socket → klient
            socket_resolver,       // socket → resolver
            buf,         // packet
            size,           // velikost
            &client_addr,     // adresa klienta
            client_len,
            &res_addr,      // resolver adresa
            args.verbose // verbose mód
        );
    }

    close(socket_client);
    close(socket_resolver);
    fprintf(stderr, "\n[+] Server stopped\n");

    return 0;
}