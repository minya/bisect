#include "precise_time.h"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include "win.h"
#endif

static int is_timestamp_at(const char *buf, size_t len, size_t i) {
    if (i + 19 > len) return 0;

    if (buf[i+4] != '-' || buf[i+7] != '-' || buf[i+10] != ' ' ||
        buf[i+13] != ':' || buf[i+16] != ':')
        return 0;

    return isdigit((unsigned char)buf[i])    && isdigit((unsigned char)buf[i+1])  &&
           isdigit((unsigned char)buf[i+2])  && isdigit((unsigned char)buf[i+3])  &&
           isdigit((unsigned char)buf[i+5])  && isdigit((unsigned char)buf[i+6])  &&
           isdigit((unsigned char)buf[i+8])  && isdigit((unsigned char)buf[i+9])  &&
           isdigit((unsigned char)buf[i+11]) && isdigit((unsigned char)buf[i+12]) &&
           isdigit((unsigned char)buf[i+14]) && isdigit((unsigned char)buf[i+15]) &&
           isdigit((unsigned char)buf[i+17]) && isdigit((unsigned char)buf[i+18]);
}


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
    size_t len = strlen(buffer);
    for (size_t i = 0; i + 19 <= len; i++) {
        if (is_timestamp_at(buffer, len, i)) {
            return (int)i;
        }
    }
    return -1;
}

// Extract date string from buffer and return its length
int extract_date_string(const char *buffer, int offset, char *date_str, size_t max_len) {
    size_t len = strlen(buffer);
    if (!is_timestamp_at(buffer, len, (size_t)offset)) {
        return -1;
    }

    int ts_len = 19;

    size_t pos = (size_t)offset + 19;
    if (pos < len && (buffer[pos] == '.' || buffer[pos] == ',')) {
        pos++;
        while (pos < len && isdigit((unsigned char)buffer[pos]) && ts_len < 29) {
            ts_len++;
            pos++;
        }
        ts_len++; // include the . or ,
    }

    if ((size_t)ts_len >= max_len) ts_len = (int)max_len - 1;
    strncpy(date_str, buffer + offset, ts_len);
    date_str[ts_len] = '\0';
    return ts_len;
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
