#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex.h>
#include <time.h>

#define PROGRAM_NAME "bisect"
#define VERSION "1.0.0"

struct search_context_t {
    time_t target_time;
    struct timespec seconds_around_target;
};

regex_t regex_datetime;
char *regex_pattern = "[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}"; // Example regex pattern for datetime

char *time_t_to_string(time_t t) {
    struct tm *tm_info = localtime(&t);
    char *buffer = malloc(26);
    if (buffer) {
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    }
    return buffer;
}

void bisect(const char *filename, struct search_context_t context);

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
    realpath(filename, absolute_path);
    if (absolute_path == NULL) {
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
    context.target_time = mktime(&tm_target);
    context.seconds_around_target.tv_sec = 2;
    bisect(filename, context);
    
    return EXIT_SUCCESS;
}

time_t find_date_in_buffer(const char *buffer, size_t size);

void bisect(const char *filename, struct search_context_t context) {
    printf("Bisecting file: %s\n", filename);
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
      size_t mid = (begin + end) / 2;

      lseek(fd, mid, SEEK_SET);
      char buffer[256];
      ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
      if (bytes_read < 0) {
          perror("Error reading file");
          close(fd);
          exit(EXIT_FAILURE);
      }

      buffer[bytes_read] = '\0';
      time_t first_buf_time = find_date_in_buffer(buffer, bytes_read);
      char *date_str = time_t_to_string(first_buf_time);
      printf("Checking position %zu: found date %s\n", mid, date_str);
      if (context.target_time < first_buf_time) {
          end = mid;
      } else {
          begin = mid + 1;
      }
    }

    if (begin >= file_size) {
        printf("No suitable date found in the file.\n");
        close(fd);
        return;
    }
    lseek(fd, begin, SEEK_SET);
    char buffer[256];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("Error reading file");
        close(fd);
        exit(EXIT_FAILURE);
    }
    buffer[bytes_read] = '\0';
    time_t found_time = find_date_in_buffer(buffer, bytes_read);
    if (found_time == 0) {
        printf("No date found in the buffer.\n");
    } else {
        printf("Found date: %s", ctime(&found_time));
    }
    close(fd);

}

time_t find_date_in_buffer(const char *buffer, size_t size) {
    int reti;

    time_t found_time = 0;
    regmatch_t pmatch[1];
    const char *p = buffer;

    while (p < buffer + size) {
        reti = regexec(&regex_datetime, p, 1, pmatch, 0);
        if (reti == 0) {
            char date_str[20];
            strncpy(date_str, p + pmatch[0].rm_so, pmatch[0].rm_eo - pmatch[0].rm_so);
            date_str[pmatch[0].rm_eo - pmatch[0].rm_so] = '\0';
            struct tm tms;
            strptime(date_str, "%Y-%m-%d %H:%M:%S", &tms);
            found_time = mktime(&tms);
            break;
        }
        p += pmatch[0].rm_eo;
    }
    return found_time;
}