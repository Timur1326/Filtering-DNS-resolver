#ifndef ARGUMENT_PARSE_H
#define ARGUMENT_PARSE_H


typedef struct {
    char resolver_ip[100];
    char filter_file[100];
    int port;
    int verbose;
} args_t;


void parse_args(int argc, char **argv, args_t *a);

#endif