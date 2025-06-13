#include "search_range.h"


int parse_time_range(const char *time_str, struct search_range_t *range) {
    if (time_str == NULL || range == NULL) {
        return -1;
    }

    struct tm tm_target = {0};
    if (strptime(time_str, "%Y-%m-%d %H:%M:%S", &tm_target) == NULL) {
        return -1;
    }

    tm_target.tm_isdst = -1; // Let mktime determine DST
    range->start = mktime(&tm_target);
    range->end = mktime(&tm_target);

    return 0;
}