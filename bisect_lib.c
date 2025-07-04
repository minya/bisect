#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

#include "search_range.h"


static size_t _BLOCK_SIZE = 4096;

void printout(int fd, size_t position, size_t stop, precise_time_t start_time, precise_time_t end_time);

size_t lower_bound(int fd, size_t file_size, struct search_range_t range, bool (*cmp)(precise_time_t, precise_time_t)) {
    size_t begin = 0;
    size_t end = file_size;

    while (begin < end) {
        if (end - begin <= _BLOCK_SIZE) {
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
        int date_offset_in_buf = find_date_in_buffer(buffer);
        if (date_offset_in_buf < 0) {
            end = mid;
            continue;
        }

        char date_str[40];  // Enough for timestamp with fractional seconds
        int date_len = extract_date_string(buffer, date_offset_in_buf, date_str, sizeof(date_str));
        if (date_len < 0) {
            end = mid;
            continue;
        }
        
        precise_time_t found_time = string_to_precise_time(date_str);

        if (cmp(range.start, found_time)) {
            end = mid;
        } else {
            begin = mid + 1;
        }
    }
    return begin;
}

bool precise_not_greater(precise_time_t a, precise_time_t b) {
    return !precise_greater(a, b);
}

void bisect(const char *filename, struct search_range_t range) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    size_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    size_t begin = lower_bound(fd, file_size, range, precise_less);
    size_t end = lower_bound(fd, file_size, range, precise_not_greater);

    printf("Found range (hex) from %zx to %zx\n", begin, end);

    if (begin >= file_size) {
        close(fd);
        return;
    }

    printout(fd, begin, end + _BLOCK_SIZE, range.start, range.end);
    close(fd);
}

void printout(int fd, size_t from, size_t to, precise_time_t start_time, precise_time_t end_time) {
    lseek(fd, from, SEEK_SET);
    // Large enough buffer to read multiple lines.
    // Needs to be replaced by a list of buffers or become dynamic as we do not know how large the lines are.
    const size_t _BUF_SIZE = 8192;
    char buffer[_BUF_SIZE+1]; // + 1 for null terminator
    size_t remaining_bytes = 0;
    size_t pos_to_print_from = SIZE_MAX;
    while (true)
    {
        if (from >= to + _BUF_SIZE) {
            break; // Stop reading if we have reached the end position
        }

        int bytes_read = read(fd, buffer + remaining_bytes, sizeof(buffer) - 1 - remaining_bytes);
        if (bytes_read < 0) {
            perror("Error reading file");
            return;
        }
        if (bytes_read == 0) {
            break; // End of file reached
        }

        from += bytes_read;

        buffer[bytes_read + remaining_bytes] = '\0';
        size_t total_bytes_read = bytes_read + remaining_bytes;
        remaining_bytes = 0;

        char *buf_ptr = buffer;
        while (buf_ptr < buffer + total_bytes_read) {
            int found_time = find_date_in_buffer(buf_ptr);

            if (found_time < 0) {
                size_t copy_from = buf_ptr - buffer;
                if (pos_to_print_from != SIZE_MAX) {
                    // If we have a position to print from, we need to copy that part to the beginning of the buffer
                    copy_from = pos_to_print_from;
                    pos_to_print_from = 0;
                }
                memmove(buffer, buffer + copy_from, total_bytes_read - copy_from);
                remaining_bytes = total_bytes_read - copy_from;

                break;
            }

            if (pos_to_print_from != SIZE_MAX) {
                size_t pos_to_print_to = (buf_ptr - buffer) + found_time;
                int n_bytes_to_print = pos_to_print_to - pos_to_print_from;
                if (write(STDOUT_FILENO, buffer + pos_to_print_from, n_bytes_to_print) < 0) {
                    perror("(2) Error writing to stdout");
                    // printf("pos_to_print_from: %zu\n", pos_to_print_from);
                    // printf("total_bytes_read: %zu\n", total_bytes_read);
                    // printf("n_bytes_to_print: %d\n", n_bytes_to_print);
                    // printf("found_time: %d\n", found_time);
                    // printf("buf_ptr - buffer: %ld\n", (buf_ptr - buffer));
                    return;
                }
                
                pos_to_print_from = SIZE_MAX;
            }

            char date_str[40];
            int date_len = extract_date_string(buf_ptr, found_time, date_str, sizeof(date_str));
            if (date_len < 0) {
                // If we can't extract the date, skip ahead conservatively
                buf_ptr += found_time + 19;
                continue;
            }
            
            precise_time_t found_time_value = string_to_precise_time(date_str);
            if (precise_less(found_time_value, start_time)) {
                buf_ptr += found_time + date_len;
            } else if (precise_greater(found_time_value, end_time)) {
                break;
            } else {
                pos_to_print_from = (buf_ptr - buffer) + found_time;
                buf_ptr += found_time + date_len;
            }
        }
    }

    if (pos_to_print_from != SIZE_MAX) {
        if (write(STDOUT_FILENO, buffer + pos_to_print_from, remaining_bytes - pos_to_print_from) < 0) {
            perror("(3) Error writing to stdout");
        }
    }
}
