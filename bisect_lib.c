#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <regex.h>
#include <unistd.h>
#include <fcntl.h>
#include "bisect.h"

regex_t regex_datetime;
char *regex_pattern = "[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}";

char *time_t_to_string(time_t t) {
    struct tm *tm_info = localtime(&t);
    char *buffer = malloc(26);
    if (buffer) {
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    }
    return buffer;
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

    char *target_date_str = time_t_to_string(context.target_time);
    printf("Target date: %s\n", target_date_str);
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
      free(date_str);
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