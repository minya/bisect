#include "precise_time.h"
#include <regex.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include "win.h"
#endif

regex_t regex_datetime;
char *regex_pattern = "[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}([\\.,][0-9]{1,9})?";


char *precise_time_to_string(precise_time_t t) {
    struct tm *tm_info = localtime(&t.seconds);
    char *buffer = malloc(40);  // enough for timestamp + up to 9 digits of fractional seconds
    if (buffer) {
        size_t len = strftime(buffer, 40, "%Y-%m-%d %H:%M:%S", tm_info);
        if (t.nanoseconds > 0) {
            // Format nanoseconds, removing trailing zeros
            long ns = t.nanoseconds;
            int precision = 9;
            while (precision > 1 && ns % 10 == 0) {
                ns /= 10;
                precision--;
            }
            snprintf(buffer + len, 40 - len, ".%0*ld", precision, ns);
        }
    }
    return buffer;
}

precise_time_t string_to_precise_time(const char *str) {
    precise_time_t result = {0, 0};
    struct tm tms = {0};
    tms.tm_isdst = -1; // Let mktime determine if DST is in effect
    char *end_ptr;
    
    // Parse the base date/time
    end_ptr = strptime(str, "%Y-%m-%d %H:%M:%S", &tms);
    if (end_ptr == NULL) {
        fprintf(stderr, "Error parsing date string: %s\n", str);
        result.seconds = -1;
        return result;
    }
    
    result.seconds = mktime(&tms);

    // Check for fractional seconds
    if (*end_ptr == '.' || *end_ptr == ',') {
        end_ptr++;
        char frac_str[10] = {0};  // max 9 digits for nanoseconds
        int i = 0;
        while (*end_ptr >= '0' && *end_ptr <= '9' && i < 9) {
            frac_str[i++] = *end_ptr++;
        }
        
        if (i > 0) {
            long frac = atol(frac_str);
            // Convert to nanoseconds
            while (i < 9) {
                frac *= 10;
                i++;
            }
            result.nanoseconds = frac;
        }
    }
    
    return result;
}

time_t string_to_time_t(const char *str) {
    precise_time_t pt = string_to_precise_time(str);
    return pt.seconds;
}

int find_date_in_buffer(const char *buffer) {
    int reti;
    regmatch_t pmatch[1];

    reti = regexec(&regex_datetime, buffer, 1, pmatch, 0);
    if (reti == 0) {
        /*printf("DEBUG (find_date_in_buffer): strlen = %zu (found at %lld)\n", strlen(buffer), pmatch[0].rm_so);*/
        return pmatch[0].rm_so;
    }
    return -1;
}

// Extract date string from buffer and return its length
int extract_date_string(const char *buffer, int offset, char *date_str, size_t max_len) {
    regmatch_t pmatch[1];
    /*printf("DEBUG: strlen = %zu (offset was %d)\n", strlen(buffer + offset), offset);*/
    int reti = regexec(&regex_datetime, buffer + offset, 1, pmatch, 0);
    if (reti == 0 && pmatch[0].rm_so == 0) {
        int len = pmatch[0].rm_eo - pmatch[0].rm_so;
        if (len >= (int)max_len) len = (int)max_len - 1;
        strncpy(date_str, buffer + offset, len);
        date_str[len] = '\0';
        return len;
    }
    return -1;
}

bool less(time_t a, time_t b) {
    return a < b;
}

bool precise_less(precise_time_t a, precise_time_t b) {
    if (a.seconds < b.seconds) return true;
    if (a.seconds > b.seconds) return false;
    return a.nanoseconds < b.nanoseconds;
}

bool precise_less_equal(precise_time_t a, precise_time_t b) {
    if (a.seconds < b.seconds) return true;
    if (a.seconds > b.seconds) return false;
    return a.nanoseconds <= b.nanoseconds;
}

bool precise_greater(precise_time_t a, precise_time_t b) {
    if (a.seconds > b.seconds) return true;
    if (a.seconds < b.seconds) return false;
    return a.nanoseconds > b.nanoseconds;
}
