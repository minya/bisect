#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include "bisect.h"

void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS] <filename>\n", program_name);
    printf("A command line utility.\n\n");
    printf("Arguments:\n");
    printf("  filename       Input file to process\n\n");
    printf("Options:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -v, --version  Show version information\n");
    printf("  -t, --time     Target timestamp (YYYY-MM-DD HH:MM:SS)\n");
    printf("  -V, --verbose  Enable verbose output\n");
}

void print_version() {
    printf("%s version %s\n", PROGRAM_NAME, VERSION);
}

int main(int argc, char *argv[]) {
    if (regcomp(&regex_datetime, regex_pattern, REG_EXTENDED)) {
        fprintf(stderr, "Could not compile regex\n");
        return 0;
    }

    int opt;
    int verbose = 0;
    char *timestamp = NULL;
    char *filename = NULL;
    
    static struct option long_options[] = {
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'v'},
        {"time",    required_argument, 0, 't'},
        {"verbose", no_argument,       0, 'V'},
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "hvt:V", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            case 'v':
                print_version();
                exit(EXIT_SUCCESS);
            case 't':
                timestamp = optarg;
                break;
            case 'V':
                verbose = 1;
                break;
            case '?':
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                exit(EXIT_FAILURE);
            default:
                abort();
        }
    }
    
    if (timestamp == NULL) {
        fprintf(stderr, "Error: timestamp argument required (-t or --time)\n");
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    if (optind >= argc) {
        fprintf(stderr, "Error: filename argument required\n");
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    filename = argv[optind];

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

    struct tm tm_target = {0};
    if (strptime(timestamp, "%Y-%m-%d %H:%M:%S", &tm_target) == NULL) {
        fprintf(stderr, "Error: invalid timestamp format. Use YYYY-MM-DD HH:MM:SS\n");
        exit(EXIT_FAILURE);
    }
    
    if (verbose) {
        printf("Verbose mode enabled\n");
        printf("Target timestamp: %s\n", timestamp);
        printf("Processing file: %s\n", filename);
    }

    struct search_context_t context;
    tm_target.tm_isdst = -1; // Let mktime determine DST
    context.target_time = mktime(&tm_target);
    context.seconds_around_target.tv_sec = 2;
    bisect(filename, context);
    
    return EXIT_SUCCESS;
}

time_t find_date_in_buffer(const char *buffer, size_t size);

