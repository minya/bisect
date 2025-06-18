#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <regex.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>

#include "bisect.h"
#include "search_range.h"

#if defined(_WIN32) || defined(_WIN64)
#include "win.h"
#endif

regex_t regex_datetime;
char *regex_pattern = "[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}";

size_t date_str_len = 19; // "YYYY-MM-DD HH:MM:SS"

char *time_t_to_string(time_t t) {
    struct tm *tm_info = localtime(&t);
    char *buffer = malloc(26);
    if (buffer) {
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    }
    return buffer;
}

time_t string_to_time_t(const char *str) {
    struct tm tms;
    if (strptime(str, "%Y-%m-%d %H:%M:%S", &tms) == NULL) {
        fprintf(stderr, "Error parsing date string: %s\n", str);
        return -1;
    }
    return mktime(&tms);
}

int find_date_in_buffer(const char *buffer, size_t size) {
    int reti;
    regmatch_t pmatch[1];
    const char *p = buffer;

    while (p < buffer + size) {
        reti = regexec(&regex_datetime, p, 1, pmatch, 0);
        if (reti == 0) {
            return pmatch[0].rm_so + (p - buffer);
        }
        p += pmatch[0].rm_eo;
    }
    return -1;
}

bool less(time_t a, time_t b) {
    return a < b;
}

void printout(int fd, size_t position, time_t start_time, time_t end_time);

void bisect(const char *filename, struct search_range_t range) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    size_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    size_t begin = 0;
    size_t end = file_size;

    while (begin < end) {
        if (end - begin <= 4096) {
            break; // If the range is small enough, stop bisecting
        }
        size_t mid = (begin + end) / 2;

        lseek(fd, mid, SEEK_SET);
        size_t remaining_size = end - mid;
        size_t read_size = remaining_size < 1023 ? remaining_size : 1023;
        char buffer[read_size + 1];
        int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read < 0) {
            perror("Error reading file");
            close(fd);
            exit(EXIT_FAILURE);
        }

        buffer[bytes_read] = '\0';
        int date_offset_in_buf = find_date_in_buffer(buffer, bytes_read);
        if (date_offset_in_buf < 0) {
            end = mid;
            continue;
        }

        char date_str[20];
        strncpy(date_str, buffer + date_offset_in_buf, 20);
        date_str[19] = '\0';
        struct tm tms;
        strptime(date_str, "%Y-%m-%d %H:%M:%S", &tms);
        time_t found_time = mktime(&tms);

        if (less(range.start, found_time)) {
            end = mid;
        } else {
            begin = mid + 1;
        }
    }

    if (begin >= file_size) {
        close(fd);
        return;
    }

    printout(fd, begin, range.start, range.end);
    close(fd);
}

void printout(int fd, size_t position, time_t start_time, time_t end_time) {
    lseek(fd, position, SEEK_SET);
    char buffer[1025];

    size_t pos_to_print_from = SIZE_MAX;
    while (true)
    {
        int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read < 0) {
            perror("Error reading file");
            return;
        }
        if (bytes_read == 0) {
            break; // End of file reached
        }
        buffer[bytes_read] = '\0';


        char *buf_ptr = buffer;
        while (buf_ptr < buffer + bytes_read) {
            int found_time = find_date_in_buffer(buf_ptr, bytes_read);
            if (found_time < 0) {
                if (pos_to_print_from != SIZE_MAX) {
                    if (write(STDOUT_FILENO, buffer + pos_to_print_from, bytes_read - pos_to_print_from) < 0) {
                        perror("Error writing to stdout");
                        return;
                    }
                    pos_to_print_from = 0;
                }
                buf_ptr += bytes_read; // Move to the end of the buffer
                continue;
            }

            if (pos_to_print_from != SIZE_MAX) {
                if (write(STDOUT_FILENO, buffer + pos_to_print_from, buf_ptr - buffer + found_time - pos_to_print_from) < 0) {
                    perror("Error writing to stdout");
                    return;
                }
                
                pos_to_print_from = SIZE_MAX;
            }

            time_t found_time_value = string_to_time_t(buf_ptr + found_time);
            if (found_time_value < start_time) {
                buf_ptr += found_time + 20;
                continue;
            } else if (found_time_value > end_time) {
                return;
            } else {
                pos_to_print_from = (buf_ptr - buffer) + found_time;
                buf_ptr += found_time + 20;
            }
        }
    }
    
}
