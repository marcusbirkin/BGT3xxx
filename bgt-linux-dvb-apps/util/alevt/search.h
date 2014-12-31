#ifndef SEARCH_H
#define SEARCH_H

#include <regex.h>

struct search
{
    struct cache *cache;
    regex_t pattern[1];
    int x, y, len; // the position of the match
};

struct search *search_start(struct cache *ca, u8 *pattern);
void search_end(struct search *s);
int search_next(struct search *s, int *pgno, int *subno, int dir);
#endif
