#ifndef LOGGING_H
#define LOGGING_H

#include <sys/types.h>

void init_logging(const char *log_path);
// Log an event with metadata: operation, path, pid, uid, result code
void log_event(const char *operation, const char *path, pid_t pid, uid_t uid, int result);
void close_logging(void);

#endif