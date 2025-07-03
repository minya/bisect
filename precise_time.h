#ifndef TIME_TYPES_H
#define TIME_TYPES_H

#include <time.h>
#include <stdbool.h>

typedef struct {
    time_t seconds;
    long nanoseconds;  // fractional seconds in nanoseconds (0-999999999)
} precise_time_t;

char *precise_time_to_string(precise_time_t t);
precise_time_t string_to_precise_time(const char *str);
time_t string_to_time_t(const char *str);
int find_date_in_buffer(const char *buffer);
int extract_date_string(const char *buffer, int offset, char *date_str, size_t max_len);
bool precise_less(precise_time_t a, precise_time_t b);
bool precise_less_equal(precise_time_t a, precise_time_t b);
bool precise_greater(precise_time_t a, precise_time_t b);

#endif // TIME_TYPES_H