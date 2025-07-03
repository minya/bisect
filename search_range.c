#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "precise_time.h"
#include "search_range.h"
#ifdef _WIN32
#include "win.h"
#endif


bool is_valid_operand(char operand) {
    return operand == '+' || operand == '-' || operand == '~';
}

bool is_valid_offset_unit(char unit) {
    return unit == 's' || unit == 'm' || unit == 'h' || unit == 'd';
}

int parse_search_range(const char *time_str, struct search_range_t *range) {
    if (time_str == NULL || range == NULL) {
        return -1;
    }

    size_t time_str_len = strlen(time_str);

    struct tm tm_target = {0};
    char *end_ptr = strptime(time_str, "%Y-%m-%d %H:%M:%S", &tm_target);
    if (end_ptr == NULL) {
        return -1;
    }

    tm_target.tm_isdst = -1; // Let mktime determine DST
    time_t base_time = mktime(&tm_target);
    
    // Initialize range with base time
    range->start.seconds = base_time;
    range->start.nanoseconds = 0;
    range->end.seconds = base_time;
    range->end.nanoseconds = 0;
    
    // Check for fractional seconds
    if (*end_ptr == '.') {
        end_ptr++;
        char frac_str[10] = {0};
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
            range->start.nanoseconds = frac;
            range->end.nanoseconds = frac;
        }
    }

    // If no modifier, return
    if (*end_ptr == '\0') {
        return 0;
    }

    char operand_char = *end_ptr;
    if (!is_valid_operand(operand_char)) {
        return -1;
    }
    
    // Find the unit character at the end
    char offset_unit_char = time_str[time_str_len - 1];
    if (!is_valid_offset_unit(offset_unit_char)) {
        return -1;
    }
    
    // Parse offset value
    int offset_value = atoi(end_ptr + 1);

    time_t offset = 0;
    switch (offset_unit_char) {
        case 's':
            offset = offset_value;
            break;
        case 'm':
            offset = offset_value * 60;
            break;
        case 'h':
            offset = offset_value * 3600;
            break;
        case 'd':
            offset = offset_value * 86400;
            break;
        default:
            return -1; // Should not reach here due to earlier validation
    }

    if (operand_char == '+') {
        range->end.seconds += offset;
    } else if (operand_char == '-') {
        range->start.seconds -= offset;
    } else if (operand_char == '~') {
        range->start.seconds -= offset;
        range->end.seconds += offset;
    } else {
        return -1; // Should not reach here due to earlier validation
    }

    return 0;
}