/* 
    Author: Nurtdinov Timur   
    Login:  xnurtd00
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "read_filter.h"

// Global filter_t filter variable
filter_t filter;

/**
 * @brief Check if a character is valid in a domain name 
 * 
 * @param c 
 * @return int 
 */
static int is_valid_char(char c) {
    return ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '.' || c == '-');
}

/**
 * @brief Check if a domain name is valid 
 * 
 * @param domain 
 * @return int 
 */
static int check_domain(const char *domain) {
    int has_letter = 0;

    // cycle through domain characters
    for (int i = 0; domain[i]; i++) {
        char c = domain[i];

        // check for at least one letter
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
            has_letter = 1;
        // invalid character
        if (!is_valid_char(c))
            return 0;
    }

    return has_letter;
}

/**
 * @brief Load filter file and store domains in filter list
 * 
 * @param fname 
 * @param verbose 
 */
void load_filter(const char *fname, int *verbose) {
    FILE *file = fopen(fname, "r");
    if (!file) {
        perror("filter");
        exit(1);
    }

    char line[300];
    // cycle through file lines
    while (fgets(line, sizeof(line), file)) {
        size_t length = strlen(line);
        // remove newline characters
        while (length > 0 && (line[length-1] == '\n' || line[length-1] == '\r'))
            line[--length] = 0;
        // skip empty lines and comments
        if (length == 0 || line[0] == '#')
            continue;

        // check domain validity
        if (!check_domain(line) && *verbose) {
            fprintf(stderr, "Invalid domain in file: %s\n", line);
            continue;
        }
        // check domain length
        if (strlen(line) >= MAX_LEN_OF_DOMAIN) {
            fprintf(stderr, "[-] domain too long, skipping: %s\n", line);
            continue;
        }

        // copy domain to filter list
        strcpy(filter.list[filter.count], line);

       // increment domain count
        filter.count++;

        // check max domains limit
        if (filter.count >= MAX_DOMAINS) break;
    }

    fclose(file);

    if (*verbose)
        fprintf(stderr, "[+] loaded %d domains\n", filter.count);
}