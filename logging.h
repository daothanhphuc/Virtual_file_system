#ifndef LOGGING_H
#define LOGGING_H

void init_logging(const char *log_path);
void log_message(const char *message);
void close_logging();

#endif