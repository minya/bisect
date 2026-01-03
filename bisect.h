#ifndef BISECT_H
#define BISECT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "search_range.h"

#define PROGRAM_NAME "bisect"
#define VERSION "1.0.0"
#define MAX_PATH_LENGTH 1024
#define MAX_BUFFER_SIZE 4096

void bisect(const char *filename, struct search_range_t range);
void print_usage(const char *program_name);
void print_version(void);

#endif
