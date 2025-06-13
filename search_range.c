#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "search_range.h"


bool is_valid_operand(char operand) {
    return operand == '+' || operand == '-' || operand == '~';
}

bool is_valid_offset_unit(char unit) {
    return unit == 's' || unit == 'm' || unit == 'h' || unit == 'd';
}

int parse_time_range(const char *time_str, struct search_range_t *range) {
    if (time_str == NULL || range == NULL) {
        return -1;
    }

    size_t time_str_len = strlen(time_str);

    struct tm tm_target = {0};
    if (strptime(time_str, "%Y-%m-%d %H:%M:%S", &tm_target) == NULL) {
        return -1;
    }

    tm_target.tm_isdst = -1; // Let mktime determine DST
    range->start = mktime(&tm_target);
    range->end = mktime(&tm_target);

    if (time_str_len == 19) {
        return 0;
    }


    char operand_char = time_str[19];
    if (!is_valid_operand(operand_char)) {
        return -1;
    }
    char offset_unit_char = time_str[time_str_len - 1];
    if (!is_valid_offset_unit(offset_unit_char)) {
        return -1;
    }
    int offset_value = atoi(&time_str[20]);

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
        range->end += offset;
    } else if (operand_char == '-') {
        range->start -= offset;
    } else if (operand_char == '~') {
        range->start -= offset;
        range->end += offset;
    } else {
        return -1; // Should not reach here due to earlier validation
    }

    return 0;
}