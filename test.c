#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
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

void test_time_t_to_string() {
    struct tm test_tm = {0};
    test_tm.tm_year = 125;  // 2025
    test_tm.tm_mon = 5;     // June (0-based)
    test_tm.tm_mday = 2;
    test_tm.tm_hour = 11;
    test_tm.tm_min = 55;
    test_tm.tm_sec = 34;
    test_tm.tm_isdst = -1;  // Let mktime determine DST
    
    time_t test_time = mktime(&test_tm);
    char *result = time_t_to_string(test_time);
    
    test_assert(result != NULL, "time_t_to_string returns non-NULL");
    test_assert(result && strlen(result) == 19, "time_t_to_string returns correct length");
    test_assert(result && strstr(result, "2025-06-02") != NULL, "time_t_to_string contains correct date");
    
    if (result) free(result);
}

void test_find_date_in_buffer() {
    if (regcomp(&regex_datetime, regex_pattern, REG_EXTENDED)) {
        printf("Could not compile regex for tests\n");
        return;
    }
    
    // Test with valid date
    const char *buffer1 = "2025-06-02 11:55:34 Some log message";
    int result1 = find_date_in_buffer(buffer1, strlen(buffer1));
    test_assert(result1 == 0, "find_date_in_buffer finds valid date at position 0");
    
    // Test with no date
    const char *buffer2 = "No date in this buffer";
    int result2 = find_date_in_buffer(buffer2, strlen(buffer2));
    test_assert(result2 == -1, "find_date_in_buffer returns -1 for no date");
    
    // Test with multiple dates (should find first one)
    const char *buffer3 = "2025-06-02 11:55:34 First date 2025-06-02 12:00:00 Second date";
    int result3 = find_date_in_buffer(buffer3, strlen(buffer3));
    test_assert(result3 == 0, "find_date_in_buffer finds first date at position 0");
    
    // Test with date not at beginning
    const char *buffer4 = "Some text 2025-06-02 11:55:34 Some log message";
    int result4 = find_date_in_buffer(buffer4, strlen(buffer4));
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
    test_assert(parse_time_range("2025-06-02 11:55:34", &range) == 0, "parse_time_range parses basic date");
    test_assert(range.start == range.end, "parse_time_range sets equal start/end for basic date");
    
    // Test NULL inputs
    test_assert(parse_time_range(NULL, &range) == -1, "parse_time_range handles NULL time_str");
    test_assert(parse_time_range("2025-06-02 11:55:34", NULL) == -1, "parse_time_range handles NULL range");
    
    // Test invalid date format
    test_assert(parse_time_range("invalid-date", &range) == -1, "parse_time_range rejects invalid date format");
    test_assert(parse_time_range("2025-13-40 25:70:80", &range) == -1, "parse_time_range rejects invalid date values");
    
    // Test + operand with seconds
    test_assert(parse_time_range("2025-06-02 11:55:34+30s", &range) == 0, "parse_time_range handles +30s");
    test_assert(range.end == range.start + 30, "parse_time_range correctly adds 30 seconds");
    
    // Test - operand with minutes
    test_assert(parse_time_range("2025-06-02 11:55:34-15m", &range) == 0, "parse_time_range handles -15m");
    test_assert(range.start == range.end - (15 * 60), "parse_time_range correctly subtracts 15 minutes");
    
    // Test ~ operand with hours
    test_assert(parse_time_range("2025-06-02 11:55:34~2h", &range) == 0, "parse_time_range handles ~2h");
    time_t original_time = range.start + (2 * 3600);
    test_assert(range.start == original_time - (2 * 3600), "parse_time_range correctly sets start for ~2h");
    test_assert(range.end == original_time + (2 * 3600), "parse_time_range correctly sets end for ~2h");
    
    // Test days
    test_assert(parse_time_range("2025-06-02 11:55:34+1d", &range) == 0, "parse_time_range handles +1d");
    test_assert(range.end == range.start + 86400, "parse_time_range correctly adds 1 day");
    
    // Test invalid operands
    test_assert(parse_time_range("2025-06-02 11:55:34*30s", &range) == -1, "parse_time_range rejects invalid operand");
    test_assert(parse_time_range("2025-06-02 11:55:34&30s", &range) == -1, "parse_time_range rejects invalid operand");
    
    // Test invalid units
    test_assert(parse_time_range("2025-06-02 11:55:34+30x", &range) == -1, "parse_time_range rejects invalid unit");
    test_assert(parse_time_range("2025-06-02 11:55:34+30", &range) == -1, "parse_time_range rejects missing unit");
    
    // Test edge cases
    test_assert(parse_time_range("2025-06-02 11:55:34+0s", &range) == 0, "parse_time_range handles +0s");
    test_assert(range.end == range.start, "parse_time_range correctly handles zero offset");
}

int main() {
    printf("Running unit tests...\n\n");
    
    test_time_t_to_string();
    test_find_date_in_buffer();
    test_create_sample_file();
    test_parse_time_range();
    
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