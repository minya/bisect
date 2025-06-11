#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include "bisect.h"

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
    time_t result1 = find_date_in_buffer(buffer1, strlen(buffer1));
    test_assert(result1 != 0, "find_date_in_buffer finds valid date");
    
    // Test with no date
    const char *buffer2 = "No date in this buffer";
    time_t result2 = find_date_in_buffer(buffer2, strlen(buffer2));
    test_assert(result2 == 0, "find_date_in_buffer returns 0 for no date");
    
    // Test with multiple dates (should find first one)
    const char *buffer3 = "2025-06-02 11:55:34 First date 2025-06-02 12:00:00 Second date";
    time_t result3 = find_date_in_buffer(buffer3, strlen(buffer3));
    test_assert(result3 != 0, "find_date_in_buffer finds first date in buffer with multiple dates");
    
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

int main() {
    printf("Running unit tests...\n\n");
    
    test_time_t_to_string();
    test_find_date_in_buffer();
    test_create_sample_file();
    
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