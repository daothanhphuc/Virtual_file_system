#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "logging.h"
#include <string.h>
#include <pwd.h>
#include <unistd.h>

static FILE *log_file = NULL;

void init_logging(const char *log_path) {
    // Open in append mode so logs survive multiple mounts unless explicitly cleared
    log_file = fopen(log_path, "a");
    if (!log_file) {
        fprintf(stderr, "Failed to open log file: %s\n", log_path);
        exit(EXIT_FAILURE);
    }
}

// Format: ISO8601|uid|username|pid|operation|path|result\n
void log_event(const char *operation, const char *path, pid_t pid, uid_t uid, int result) {
    if (!log_file) return;

    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    char ts[64];
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S%z", &tm);

    struct passwd *pw = getpwuid(uid);
    const char *username = pw ? pw->pw_name : "unknown";

    // Escape any newlines in path and operation
    char safe_path[1024];
    size_t i, j=0;
    for (i = 0; i < strlen(path) && j+1 < sizeof(safe_path); ++i) {
        if (path[i] == '\n' || path[i] == '\r') continue;
        safe_path[j++] = path[i];
    }
    safe_path[j] = '\0';

    fprintf(log_file, "%s|%u|%s|%d|%s|%s|%d\n", ts, (unsigned int)uid, username, (int)pid, operation, safe_path, result);
    fflush(log_file);
}

void close_logging(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}