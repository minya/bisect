
#ifndef SEARCH_RANGE_H
#define SEARCH_RANGE_H

#include <time.h>

struct search_range_t {
    time_t start;
    time_t end;
};

extern size_t date_str_len; // "YYYY-MM-DD HH:MM:SS"

int parse_search_range(const char *time_str, struct search_range_t *range);

#endif // SEARCH_RANGE_H
