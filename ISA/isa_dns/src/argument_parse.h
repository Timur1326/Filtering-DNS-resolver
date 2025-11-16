/* 
    Author: Nurtdinov Timur   
    Login:  xnurtd00
*/
#ifndef ARGUMENT_PARSE_H
#define ARGUMENT_PARSE_H

/**
 * @brief Structure which hold command line arguments
 *  resolver_ip - IP address of DNS resolver
 *  filter_file - Name of filter file
 *  port - Port number
 *  verbose - Verbose mode flag
 */
typedef struct {
    char resolver_ip[100];
    char filter_file[100];
    int port;
    int verbose;
} args_t;


void parse_args(int argc, char **argv, args_t *a);

#endif