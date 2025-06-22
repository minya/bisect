
#ifndef SEARCH_RANGE_H
#define SEARCH_RANGE_H

#include <time.h>
#include "time_types.h"

struct search_range_t {
    precise_time_t start;
    precise_time_t end;
};

extern size_t date_str_len; // "YYYY-MM-DD HH:MM:SS"

int parse_search_range(const char *time_str, struct search_range_t *range);

#endif // SEARCH_RANGE_H
