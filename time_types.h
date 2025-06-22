#ifndef TIME_TYPES_H
#define TIME_TYPES_H

#include <time.h>

typedef struct {
    time_t seconds;
    long nanoseconds;  // fractional seconds in nanoseconds (0-999999999)
} precise_time_t;

#endif // TIME_TYPES_H