#ifndef BISECT_H
#define BISECT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROGRAM_NAME "bisect"
#define VERSION "1.0.0"
#define MAX_PATH_LENGTH 1024
#define MAX_BUFFER_SIZE 4096

typedef struct {
    char *input_file;
    char *output_file;
    int verbose;
} config_t;

void print_usage(const char *program_name);
void print_version(void);
int parse_arguments(int argc, char *argv[], config_t *config);
void cleanup_config(config_t *config);

#endif