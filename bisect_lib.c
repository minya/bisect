#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

#include "precise_time.h"
#include "search_range.h"


static size_t _BLOCK_SIZE = 8192;

void printout(int fd, size_t from, struct search_range_t range);


ssize_t lower_bound_block(int fd, size_t file_size, struct search_range_t range, bool (*cmp)(precise_time_t, precise_time_t)) {
    size_t n_blocks = file_size / _BLOCK_SIZE;
    size_t begin = 0;
    size_t end = n_blocks;

    char buffer[_BLOCK_SIZE];
    char date_str[64];
    while (begin < end) {
        size_t mid = (begin + end) / 2;

        lseek(fd, mid * _BLOCK_SIZE, SEEK_SET);
        int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read < 0) {
            return -1;
        }

        buffer[bytes_read] = '\0';
        int date_offset_in_buf = find_date_in_buffer(buffer);
        if (date_offset_in_buf < 0) {
            return -1;
        }

        int date_len = extract_date_string(buffer, date_offset_in_buf, date_str, sizeof(date_str));
        if (date_len < 0) {
            end = mid;
            continue;
        }
        
        precise_time_t found_time = string_to_precise_time(date_str);

        if (cmp(found_time, range.start)) {
            begin = mid + 1;
        } else {
            end = mid;
        }
    }
    if (begin > 0)
        --begin;
    return begin;
}


int bisect(const char *filename, struct search_range_t range) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    size_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    size_t first_block_with_date = lower_bound_block(fd, file_size, range, precise_less);
    if (first_block_with_date < 0) {
        close(fd);
        return -1;
    }

    if (first_block_with_date * _BLOCK_SIZE >= file_size) {
        close(fd);
        return 0;
    }

    printout(fd, first_block_with_date * _BLOCK_SIZE, range);
    close(fd);
    return 0;
}

void printout(int fd, size_t from, struct search_range_t range) {
    char buf[_BLOCK_SIZE+1];
    lseek(fd, from, SEEK_SET);
    size_t rd = read(fd, buf, _BLOCK_SIZE);
    buf[rd] = '\0';

    int dt_offset = 0;
    char date_str[64];
    int date_len = 0;
    precise_time_t date;

    // Do not read more than 2 blocks to find the first date
    unsigned int blocks_read = 1;

    do {
        int new_dt_offset_rel = find_date_in_buffer(buf + dt_offset + date_len);
        if (new_dt_offset_rel < 0) {
            if (blocks_read++ > 1) {
                return;
            }
            size_t remainder_size = sizeof(buf) - 1 - dt_offset;
            memcpy(buf, buf + dt_offset, remainder_size);
            int rd = read(fd, buf + remainder_size, sizeof(buf) - remainder_size - 1);
            if (rd == 0) {
                return;
            }
            buf[remainder_size + rd] = '\0';
            dt_offset = 0;
            new_dt_offset_rel = 0;
            continue;
        }
        dt_offset += date_len + new_dt_offset_rel;
        date_len = extract_date_string(buf, dt_offset, date_str, sizeof(date_str));
        date = string_to_precise_time(date_str);
    } while (precise_less(date, range.start));

    if (dt_offset < 0)
        return;

    while (precise_less_equal(date, range.end)) {
        int new_dt_offset_rel = find_date_in_buffer(buf + dt_offset + date_len);
        if (new_dt_offset_rel >= 0) {
            int new_dt_offset = new_dt_offset_rel + dt_offset + date_len;
            date_len = extract_date_string(buf, new_dt_offset, date_str, sizeof(date_str));
            date = string_to_precise_time(date_str);
            write(STDOUT_FILENO, buf + dt_offset, new_dt_offset - dt_offset);
            dt_offset = new_dt_offset;
        } else {
            size_t remainder_size = sizeof(buf) - 1 - dt_offset;
            memcpy(buf, buf + dt_offset, remainder_size);
            int rd = read(fd, buf + remainder_size, sizeof(buf) - remainder_size - 1);
            buf[remainder_size + rd] = '\0';
            dt_offset = find_date_in_buffer(buf + date_len);
            if (dt_offset < 0 ) {
                break;
            }
            date_len = extract_date_string(buf, dt_offset, date_str, sizeof(date_str));
            date = string_to_precise_time(date_str);
            write(STDOUT_FILENO, buf, dt_offset);
        }
    }
}
