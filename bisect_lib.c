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
char *regex_pattern = "[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}([\\.,][0-9]{1,9})?";

size_t date_str_len = 19; // "YYYY-MM-DD HH:MM:SS"

char *time_t_to_string(time_t t) {
    struct tm *tm_info = localtime(&t);
    char *buffer = malloc(26);
    if (buffer) {
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    }
    return buffer;
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
    struct tm tms;
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
        return pmatch[0].rm_so;
    }
    return -1;
}

// Extract date string from buffer and return its length
int extract_date_string(const char *buffer, int offset, char *date_str, size_t max_len) {
    regmatch_t pmatch[1];
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

void printout(int fd, size_t position, size_t stop, precise_time_t start_time, precise_time_t end_time);

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

        if (precise_less(range.start, found_time)) {
            end = mid;
        } else {
            begin = mid + 1;
        }
    }

    if (begin >= file_size) {
        close(fd);
        return;
    }

    printout(fd, begin, end, range.start, range.end);
    close(fd);
}

void printout(int fd, size_t from, size_t to, precise_time_t start_time, precise_time_t end_time) {
    lseek(fd, from, SEEK_SET);
    // Large enough buffer to read multiple lines.
    // Needs to be replaced by a list of buffers or become dynamic as we do not know how large the lines are.
    const size_t BUFSIZE = 8192;
    char buffer[BUFSIZE+1]; // + 1 for null terminator
    size_t remaining_bytes = 0;
    size_t pos_to_print_from = SIZE_MAX;
    while (true)
    {
        if (from >= to + BUFSIZE) {
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
