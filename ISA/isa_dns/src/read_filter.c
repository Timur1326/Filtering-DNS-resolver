#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "read_filter.h"

blacklist_t blacklist;


int domain_ok(const char *d) {
    int letters = 0;
    for (int i = 0; d[i]; i++) {
        char c = d[i];

        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z'))
            letters = 1;

        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '.' || c == '-'))
            return 0;
    }
    return letters;
}

// -----------------------------
void load_filter(const char *fname, int *verbose) {
    FILE *f = fopen(fname, "r");
    if (!f) {
        perror("filter");
        exit(1);
    }

    char line[300];
    while (fgets(line, sizeof(line), f)) {
        size_t l = strlen(line);
        while (l > 0 && (line[l-1] == '\n' || line[l-1] == '\r'))
            line[--l] = 0;

        if (l == 0 || line[0] == '#')
            continue;

        if (!domain_ok(line)) {
            fprintf(stderr, "Invalid domain in file: %s\n", line);
            continue;
        }
        if (strlen(line) >= MAX_LEN) {
            fprintf(stderr, "[WARN] domain too long, skipping: %s\n", line);
            continue;
        }

        strcpy(blacklist.list[blacklist.count], line);

       
        blacklist.count++;

        if (blacklist.count >= MAX_DOMAINS) break;
    }

    fclose(f);

    if (verbose)
        fprintf(stderr, "[INFO] loaded %d domains\n", blacklist.count);
}