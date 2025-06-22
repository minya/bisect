#ifndef BISECT_H
#define BISECT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <regex.h>

#include "time_types.h"
#include "search_range.h"

#define PROGRAM_NAME "bisect"
#define VERSION "1.0.0"
#define MAX_PATH_LENGTH 1024
#define MAX_BUFFER_SIZE 4096

extern regex_t regex_datetime;
extern char *regex_pattern;

char *time_t_to_string(time_t t);
char *precise_time_to_string(precise_time_t t);
precise_time_t string_to_precise_time(const char *str);
time_t string_to_time_t(const char *str);
int find_date_in_buffer(const char *buffer);
int extract_date_string(const char *buffer, int offset, char *date_str, size_t max_len);
bool precise_less(precise_time_t a, precise_time_t b);
bool precise_less_equal(precise_time_t a, precise_time_t b);
bool precise_greater(precise_time_t a, precise_time_t b);
void bisect(const char *filename, struct search_range_t range);
void print_usage(const char *program_name);
void print_version(void);

#endif
