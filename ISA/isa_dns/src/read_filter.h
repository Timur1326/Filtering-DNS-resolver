#ifndef READ_FILTER_H
#define READ_FILTER_H


#define MAX_DOMAINS 5000
#define MAX_LEN     256

typedef struct {
    char list[MAX_DOMAINS][MAX_LEN];
    int count;
} blacklist_t;

void load_filter(const char *fname, int *verbose);

blacklist_t blacklist;

#endif