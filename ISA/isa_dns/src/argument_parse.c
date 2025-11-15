#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argument_parse.h"


void help(void) {
    fprintf(stderr, "Usage: dns -s server [-p port] -f file [-v]\n");
    exit(1);
}

void parse_args(int argc, char **argv, args_t *a)
{
    memset(a->resolver_ip, 0, sizeof(a->resolver_ip));
    memset(a->filter_file, 0, sizeof(a->filter_file));
    a->port = 53;
    a->verbose = 0;

    for (int i = 1; i < argc; i++) {

        if (argv[i][0] != '-'){
            help();
        }
        char opt = argv[i][1];

        switch (opt) {

            case 's':
                if (i + 1 >= argc){
                    help();
                }
                strncpy(a->resolver_ip, argv[++i], sizeof(a->resolver_ip) - 1);
                break;

            case 'p':
                if (i + 1 >= argc) {
                    help();
                }
                a->port = atoi(argv[++i]);
                break;

            case 'f':
                if (i + 1 >= argc) {
                    help();
                }
                strncpy(a->filter_file, argv[++i], sizeof(a->filter_file) - 1);
                break;

            case 'v':
                a->verbose = 1;
                break;

            default:
                help();
        }
    }

    if (!a->resolver_ip[0] || !a->filter_file[0]){
        help();
    }
}