#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "logging.h"
#include <string.h>

static FILE *log_file = NULL;

void init_logging(const char *log_path) {
    log_file = fopen(log_path, "w");
    if (!log_file) {
        fprintf(stderr, "Failed to open log file: %s\n", log_path);
        exit(EXIT_FAILURE);
    }
}

void log_message(const char *message) {
    if (log_file) {
        time_t now = time(NULL);
        char *timestamp = ctime(&now);
        timestamp[strlen(timestamp) - 1] = '\0'; // Remove newline
        fprintf(log_file, "[%s] %s", timestamp, message);
        fflush(log_file);
    }
}

void close_logging() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}