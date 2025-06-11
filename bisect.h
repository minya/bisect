#ifndef BISECT_H
#define BISECT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <regex.h>

#define PROGRAM_NAME "bisect"
#define VERSION "1.0.0"
#define MAX_PATH_LENGTH 1024
#define MAX_BUFFER_SIZE 4096

struct search_context_t {
    time_t target_time;
    struct timespec seconds_around_target;
};

extern regex_t regex_datetime;
extern char *regex_pattern;

char *time_t_to_string(time_t t);
time_t find_date_in_buffer(const char *buffer, size_t size);
void bisect(const char *filename, struct search_context_t context);
void print_usage(const char *program_name);
void print_version(void);

#endif