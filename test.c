#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#include "precise_time.h"
#include "bisect.h"
#include "search_range.h"

int test_count = 0;
int test_passed = 0;

void test_assert(int condition, const char *test_name) {
    test_count++;
    if (condition) {
        test_passed++;
        printf("PASS: %s\n", test_name);
    } else {
        printf("FAIL: %s\n", test_name);
    }
}

// Helper functions for precise_time_t comparisons
int precise_time_equal(precise_time_t a, precise_time_t b) {
    return a.seconds == b.seconds && a.nanoseconds == b.nanoseconds;
}

precise_time_t precise_time_add_seconds(precise_time_t t, time_t seconds) {
    precise_time_t result = t;
    result.seconds += seconds;
    return result;
}

precise_time_t precise_time_sub_seconds(precise_time_t t, time_t seconds) {
    precise_time_t result = t;
    result.seconds -= seconds;
    return result;
}

void test_find_date_in_buffer() {
    if (regcomp(&regex_datetime, regex_pattern, REG_EXTENDED)) {
        printf("Could not compile regex for tests\n");
        return;
    }
    
    // Test with valid date
    const char *buffer1 = "2025-06-02 11:55:34 Some log message";
    int result1 = find_date_in_buffer(buffer1);
    test_assert(result1 == 0, "find_date_in_buffer finds valid date at position 0");
    
    // Test with no date
    const char *buffer2 = "No date in this buffer";
    int result2 = find_date_in_buffer(buffer2);
    test_assert(result2 == -1, "find_date_in_buffer returns -1 for no date");
    
    // Test with multiple dates (should find first one)
    const char *buffer3 = "2025-06-02 11:55:34 First date 2025-06-02 12:00:00 Second date";
    int result3 = find_date_in_buffer(buffer3);
    test_assert(result3 == 0, "find_date_in_buffer finds first date at position 0");
    
    // Test with date not at beginning
    const char *buffer4 = "Some text 2025-06-02 11:55:34 Some log message";
    int result4 = find_date_in_buffer(buffer4);
    test_assert(result4 == 10, "find_date_in_buffer finds date at correct position");
    
    regfree(&regex_datetime);
}

void test_create_sample_file() {
    const char *filename = "test_sample.log";
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Could not create test file\n");
        return;
    }
    
    fprintf(file, "2025-06-02 10:00:00 Early log entry\n");
    fprintf(file, "2025-06-02 11:30:00 Mid morning entry\n");
    fprintf(file, "2025-06-02 11:55:34 Target time entry\n");
    fprintf(file, "2025-06-02 12:15:00 Afternoon entry\n");
    fprintf(file, "2025-06-02 14:00:00 Late entry\n");
    
    fclose(file);
    
    // Test that file was created and is readable
    test_assert(access(filename, F_OK) == 0, "test sample file created successfully");
    test_assert(access(filename, R_OK) == 0, "test sample file is readable");
    
    // Clean up
    unlink(filename);
}

void test_parse_time_range() {
    struct search_range_t range;
    
    // Test basic date parsing
    test_assert(parse_search_range("2025-06-02 11:55:34", &range) == 0, "parse_search_range parses basic date");
    test_assert(precise_time_equal(range.start, range.end), "parse_search_range sets equal start/end for basic date");
    
    // Test NULL inputs
    test_assert(parse_search_range(NULL, &range) == -1, "parse_search_range handles NULL time_str");
    test_assert(parse_search_range("2025-06-02 11:55:34", NULL) == -1, "parse_search_range handles NULL range");
    
    // Test invalid date format
    test_assert(parse_search_range("invalid-date", &range) == -1, "parse_search_range rejects invalid date format");
    test_assert(parse_search_range("2025-13-40 25:70:80", &range) == -1, "parse_search_range rejects invalid date values");
    
    // Test + operand with seconds
    test_assert(parse_search_range("2025-06-02 11:55:34+30s", &range) == 0, "parse_search_range handles +30s");
    test_assert(precise_time_equal(range.end, precise_time_add_seconds(range.start, 30)), "parse_search_range correctly adds 30 seconds");
    
    // Test - operand with minutes
    test_assert(parse_search_range("2025-06-02 11:55:34-15m", &range) == 0, "parse_search_range handles -15m");
    test_assert(precise_time_equal(range.start, precise_time_sub_seconds(range.end, 15 * 60)), "parse_search_range correctly subtracts 15 minutes");
    
    // Test ~ operand with hours
    test_assert(parse_search_range("2025-06-02 11:55:34~2h", &range) == 0, "parse_search_range handles ~2h");
    precise_time_t original_time = precise_time_add_seconds(range.start, 2 * 3600);
    test_assert(precise_time_equal(range.start, precise_time_sub_seconds(original_time, 2 * 3600)), "parse_search_range correctly sets start for ~2h");
    test_assert(precise_time_equal(range.end, precise_time_add_seconds(original_time, 2 * 3600)), "parse_search_range correctly sets end for ~2h");
    
    // Test days
    test_assert(parse_search_range("2025-06-02 11:55:34+1d", &range) == 0, "parse_search_range handles +1d");
    test_assert(precise_time_equal(range.end, precise_time_add_seconds(range.start, 86400)), "parse_search_range correctly adds 1 day");
    
    // Test invalid operands
    test_assert(parse_search_range("2025-06-02 11:55:34*30s", &range) == -1, "parse_search_range rejects invalid operand");
    test_assert(parse_search_range("2025-06-02 11:55:34&30s", &range) == -1, "parse_search_range rejects invalid operand");
    
    // Test invalid units
    test_assert(parse_search_range("2025-06-02 11:55:34+30x", &range) == -1, "parse_search_range rejects invalid unit");
    test_assert(parse_search_range("2025-06-02 11:55:34+30", &range) == -1, "parse_search_range rejects missing unit");
    
    // Test edge cases
    test_assert(parse_search_range("2025-06-02 11:55:34+0s", &range) == 0, "parse_search_range handles +0s");
    test_assert(precise_time_equal(range.end, range.start), "parse_search_range correctly handles zero offset");
}

void test_precise_time_parsing() {
    // Test parsing various fractional second formats
    const char *test_cases[] = {
        "2025-06-02 11:55:34",
        "2025-06-02 11:55:34.1",
        "2025-06-02 11:55:34.123",
        "2025-06-02 11:55:34.123456",
        "2025-06-02 11:55:34.123456789",
        "2025-06-02 11:55:34.999999999"
    };
    
    long expected_nanos[] = {
        0,
        100000000,
        123000000,
        123456000,
        123456789,
        999999999
    };
    
    for (int i = 0; i < 6; i++) {
        precise_time_t pt = string_to_precise_time(test_cases[i]);
        char msg[100];
        snprintf(msg, sizeof(msg), "string_to_precise_time parses '%s' correctly", test_cases[i]);
        test_assert(pt.seconds > 0, msg);
        
        snprintf(msg, sizeof(msg), "string_to_precise_time sets correct nanoseconds for '%s'", test_cases[i]);
        test_assert(pt.nanoseconds == expected_nanos[i], msg);
    }
    
    // Test precise_time_to_string
    precise_time_t pt = {1622641534, 123456789};
    char *str = precise_time_to_string(pt);
    test_assert(str != NULL, "precise_time_to_string returns non-NULL");
    test_assert(strstr(str, ".123456789") != NULL, "precise_time_to_string includes fractional seconds");
    free(str);
    
    // Test with zero nanoseconds
    pt.nanoseconds = 0;
    str = precise_time_to_string(pt);
    test_assert(str != NULL, "precise_time_to_string handles zero nanoseconds");
    test_assert(strstr(str, ".") == NULL, "precise_time_to_string omits fractional seconds when zero");
    free(str);
}

void test_fractional_search_range() {
    struct search_range_t range;
    
    // Test fractional seconds parsing
    test_assert(parse_search_range("2025-06-02 11:55:34.123", &range) == 0, "parse_search_range handles fractional seconds");
    test_assert(range.start.nanoseconds == 123000000, "parse_search_range sets correct start nanoseconds");
    test_assert(range.end.nanoseconds == 123000000, "parse_search_range sets correct end nanoseconds");
    
    // Test fractional seconds with offset
    test_assert(parse_search_range("2025-06-02 11:55:34.500+5s", &range) == 0, "parse_search_range handles fractional with offset");
    test_assert(range.start.nanoseconds == 500000000, "parse_search_range preserves nanoseconds with offset");
    test_assert(range.end.nanoseconds == 500000000, "parse_search_range preserves nanoseconds in end time");
    test_assert(range.end.seconds == range.start.seconds + 5, "parse_search_range adds seconds correctly with fractional");
    
    // Test fractional seconds with ~ operator
    test_assert(parse_search_range("2025-06-02 11:55:34.999~1s", &range) == 0, "parse_search_range handles fractional with ~ operator");
    test_assert(range.start.nanoseconds == 999000000, "parse_search_range preserves nanoseconds in start");
    test_assert(range.end.nanoseconds == 999000000, "parse_search_range preserves nanoseconds in end");
    test_assert(range.start.seconds == range.end.seconds - 2, "parse_search_range applies ~ operator correctly");
}

void test_date_regex_with_fractional() {
    if (regcomp(&regex_datetime, regex_pattern, REG_EXTENDED)) {
        printf("Could not compile regex for fractional tests\n");
        return;
    }
    
    // Test various fractional formats
    const char *valid_dates[] = {
        "2025-06-02 11:55:34",
        "2025-06-02 11:55:34.1",
        "2025-06-02 11:55:34.12",
        "2025-06-02 11:55:34.123456789",
        "Some text 2025-06-02 11:55:34.500 more text"
    };
    
    for (int i = 0; i < 5; i++) {
        int pos = find_date_in_buffer(valid_dates[i]);
        char msg[100];
        snprintf(msg, sizeof(msg), "find_date_in_buffer finds date in '%s'", valid_dates[i]);
        test_assert(pos >= 0, msg);
    }
    
    // Test date extraction
    char date_str[40];
    int len = extract_date_string("2025-06-02 11:55:34.123456789 log entry", 0, date_str, sizeof(date_str));
    test_assert(len > 0, "extract_date_string returns positive length");
    test_assert(strcmp(date_str, "2025-06-02 11:55:34.123456789") == 0, "extract_date_string extracts full fractional date");
}

void test_edge_cases() {
    // Test comparison functions
    precise_time_t t1 = {100, 500000000};
    precise_time_t t2 = {100, 600000000};
    precise_time_t t3 = {101, 0};
    precise_time_t t4 = {100, 500000000}; // Equal to t1
    
    test_assert(precise_less(t1, t2), "precise_less: same seconds, different nanoseconds");
    test_assert(precise_less(t1, t3), "precise_less: different seconds");
    test_assert(!precise_less(t1, t4), "precise_less: equal times returns false");
    test_assert(!precise_less(t2, t1), "precise_less: greater time returns false");
    
    test_assert(precise_less_equal(t1, t2), "precise_less_equal: less returns true");
    test_assert(precise_less_equal(t1, t4), "precise_less_equal: equal returns true");
    test_assert(!precise_less_equal(t2, t1), "precise_less_equal: greater returns false");
    
    test_assert(precise_greater(t2, t1), "precise_greater: same seconds, different nanoseconds");
    test_assert(precise_greater(t3, t1), "precise_greater: different seconds");
    test_assert(!precise_greater(t1, t4), "precise_greater: equal times returns false");
    
    // Test parsing edge cases
    precise_time_t pt;
    
    // Test with trailing zeros in fractional seconds
    pt = string_to_precise_time("2025-06-02 11:55:34.100000000");
    test_assert(pt.nanoseconds == 100000000, "string_to_precise_time handles trailing zeros");
    
    // Test maximum nanoseconds
    pt = string_to_precise_time("2025-06-02 11:55:34.999999999");
    test_assert(pt.nanoseconds == 999999999, "string_to_precise_time handles maximum nanoseconds");
    
    // Test with more than 9 digits (should truncate)
    pt = string_to_precise_time("2025-06-02 11:55:34.12345678901234");
    test_assert(pt.nanoseconds == 123456789, "string_to_precise_time truncates excess digits");
    
    // Test precise_time_to_string removes trailing zeros
    precise_time_t test_time = {1622641534, 100000000};
    char *str = precise_time_to_string(test_time);
    test_assert(strstr(str, ".1") != NULL && strstr(str, ".100000000") == NULL, 
                "precise_time_to_string removes trailing zeros");
    free(str);
    
    // Test with single nanosecond
    test_time.nanoseconds = 1;
    str = precise_time_to_string(test_time);
    test_assert(strstr(str, ".000000001") != NULL, "precise_time_to_string handles single nanosecond");
    free(str);
}

int main() {
    printf("Running unit tests...\n\n");
    
    test_find_date_in_buffer();
    test_create_sample_file();
    test_parse_time_range();
    test_precise_time_parsing();
    test_fractional_search_range();
    test_date_regex_with_fractional();
    test_edge_cases();
    
    printf("\n=== Test Results ===\n");
    printf("Tests run: %d\n", test_count);
    printf("Tests passed: %d\n", test_passed);
    printf("Tests failed: %d\n", test_count - test_passed);
    
    if (test_passed == test_count) {
        printf("All tests passed!\n");
        return 0;
    } else {
        printf("Some tests failed.\n");
        return 1;
    }
}