#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <regex.h>
#include "bisect.h"

#if defined(_WIN32) || defined(_WIN64)
#include "win.h"
#endif

void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS] <filename>\n", program_name);
    printf("A command line utility.\n\n");
    printf("Arguments:\n");
    printf("  filename       Input file to process\n\n");
    printf("Options:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -v, --version  Show version information\n");
    printf("  -t, --time     Target time range (YYYY-MM-DD HH:MM:SS[+|-|~]<number><unit>)\n");
    printf("  -V, --verbose  Enable verbose output\n");
}

void print_version(void) {
    printf("%s version %s\n", PROGRAM_NAME, VERSION);
}

int main(int argc, char *argv[]) {
    if (regcomp(&regex_datetime, regex_pattern, REG_EXTENDED)) {
        fprintf(stderr, "Could not compile regex\n");
        return EXIT_FAILURE;
    }

    int verbose = 0;
    char *time_range_str = NULL;
    char *filename = NULL;

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        } else if (strcmp(arg, "-v") == 0 || strcmp(arg, "--version") == 0) {
            print_version();
            exit(EXIT_SUCCESS);
        } else if (strcmp(arg, "-V") == 0 || strcmp(arg, "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(arg, "-t") == 0 || strcmp(arg, "--time") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires an argument\n", arg);
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                exit(EXIT_FAILURE);
            }
            time_range_str = argv[++i];
        } else if (strncmp(arg, "--time=", 7) == 0) {
            time_range_str = arg + 7;
        } else if (arg[0] == '-') {
            fprintf(stderr, "Error: unknown option '%s'\n", arg);
            fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
            exit(EXIT_FAILURE);
        } else {
            if (filename != NULL) {
                fprintf(stderr, "Error: unexpected argument '%s'\n", arg);
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                exit(EXIT_FAILURE);
            }
            filename = arg;
        }
    }

    if (time_range_str == NULL) {
        fprintf(stderr, "Error: time argument required (-t or --time)\n");
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (filename == NULL) {
        fprintf(stderr, "Error: filename argument required\n");
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char absolute_path[PATH_MAX];
    if (realpath(filename, absolute_path) == NULL) {
        fprintf(stderr, "Error: could not resolve absolute path for '%s'\n", filename);
        exit(EXIT_FAILURE);
    }
    filename = absolute_path;
    if (strlen(filename) == 0) {
        fprintf(stderr, "Error: filename cannot be empty\n");
        exit(EXIT_FAILURE);
    }
    if (access(filename, F_OK | R_OK) == -1) {
        fprintf(stderr, "Error: file '%s' does not exist\n", filename);
        exit(EXIT_FAILURE);
    }

    if (verbose) {
        printf("Verbose mode enabled\n");
        printf("Target time: %s\n", time_range_str);
        printf("Processing file: %s\n", filename);
    }

    struct search_range_t range;
    if (parse_search_range(time_range_str, &range) != 0) {
        fprintf(stderr, "Error: invalid time format '%s'. Expected format: YYYY-MM-DD HH:MM:SS[+|-|~]<number><unit>\n", time_range_str);
        exit(EXIT_FAILURE);
    }
    bisect(filename, range);
    
    return EXIT_SUCCESS;
}

