/* 
    Author: Nurtdinov Timur   
    Login:  xnurtd00
*/
#ifndef READ_FILTER_H
#define READ_FILTER_H

// Maximum number of domains in filter
#define MAX_DOMAINS 5000
// Maximum length of a domain
#define MAX_LEN_OF_DOMAIN     256


/**
 * @brief Structure to hold the list of filtered domains
 *  list - Array of domain strings
 *  count - Number of domains in the list
 */
typedef struct {
    char list[MAX_DOMAINS][MAX_LEN_OF_DOMAIN];
    int count;
} filter_t;

void load_filter(const char *fname, int *verbose);

extern filter_t filter;
#endif