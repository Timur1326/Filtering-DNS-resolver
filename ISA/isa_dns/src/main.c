/* 
    Author: Nurtdinov Timur   
    Login:  xnurtd00
*/
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

// flag for stopping server
volatile sig_atomic_t stop = 0;

/**
 * @brief This function handle SIGINT signal
 * 
 * @param sig 
 */
void handle_signal(int sig) {
    (void)sig;    
    stop = 1;     
}


int main(int argc, char **argv) {

    // handle SIGINT signal
    signal(SIGINT, handle_signal);

    // argunements structure
    args_t args;

    // parse arguments
    parse_args(argc, argv, &args);

    // filter file load
    load_filter(args.filter_file, &args.verbose);

    // sket client 
    int socket_client = socket(AF_INET, SOCK_DGRAM, 0);
    // check socket
    if (socket_client < 0) { 
        perror("socket"); exit(1); 
    }

    // socket resolver
    int socket_resolver = socket(AF_INET, SOCK_DGRAM, 0);
    // check socket
    if (socket_resolver < 0) { 
        perror("resolver socket"); 
        exit(1); 
    }

    // timeout for resolver while waiting for response
    struct timeval timeout = {0, 500000};
    // set timeout
    setsockopt(socket_resolver, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(socket_client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // resolver address
    struct sockaddr_in res_addr;
    memset(&res_addr, 0, sizeof(res_addr));
    res_addr.sin_family = AF_INET;
    res_addr.sin_port   = htons(53);

    // struct for IPv4 address
    struct in_addr ip4;

    // resolve resolver IP address
    if (!resolve_ipv4(args.resolver_ip, &ip4)) {
        exit(1); 
    }

    res_addr.sin_addr = ip4;

    // bind server port
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port   = htons(args.port);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    // bind socket
    if (bind(socket_client, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (args.verbose)
        fprintf(stderr, "[+] listening on port %d\n", args.port);

    unsigned char buf[512];

    // main loop which stopps on SIGINT(ctrl+c)
    while (!stop) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int size = recvfrom(socket_client, buf, sizeof(buf), 0,
                         (struct sockaddr*)&client_addr, &client_len);

        if (size <= 0)
            continue;

        process_client_query(
            socket_client,         // socket → client
            socket_resolver,       // socket → resolver
            buf,                   // packet
            size,                  // size
            &client_addr,          // client address
            client_len,            // client address length
            &res_addr,             // resolver address
            args.verbose           // verbose mode
        );
    }

    // close sockets
    close(socket_client);
    close(socket_resolver);
    fprintf(stderr, "\n[+] Server stopped\n");

    return 0;
}