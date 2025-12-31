/*
 * cli_query.c
 * Simple log query tool for the virtual file system.
 * Usage: ./cli_query [--log path] [--user username|uid] [--file filename] [--op OPERATION]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int starts_with(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

int main(int argc, char **argv) {
    const char *log_path = "virtual_fs.log";
    const char *filter_user = NULL;
    const char *filter_file = NULL;
    const char *filter_op = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--log") == 0 && i+1 < argc) {
            log_path = argv[++i];
        } else if (strcmp(argv[i], "--user") == 0 && i+1 < argc) {
            filter_user = argv[++i];
        } else if (strcmp(argv[i], "--file") == 0 && i+1 < argc) {
            filter_file = argv[++i];
        } else if (strcmp(argv[i], "--op") == 0 && i+1 < argc) {
            filter_op = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [--log path] [--user username|uid] [--file filename] [--op OPERATION]\n", argv[0]);
            return 0;
        }
    }

    FILE *f = fopen(log_path, "r");
    if (!f) {
        fprintf(stderr, "Failed to open log file: %s\n", log_path);
        return 2;
    }

    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        // Trim newline
        size_t L = strlen(line);
        if (L && (line[L-1] == '\n' || line[L-1] == '\r')) line[--L] = '\0';

        // Split by '|'
        char *parts[7];
        char *p = line;
        int idx = 0;
        char *tok;
        while ((tok = strsep(&p, "|")) != NULL && idx < 7) {
            parts[idx++] = tok;
        }
        if (idx < 7) continue; // malformed

        const char *ts = parts[0];
        const char *uid = parts[1];
        const char *username = parts[2];
        const char *pid = parts[3];
        const char *op = parts[4];
        const char *path = parts[5];
        const char *result = parts[6];

        if (filter_user) {
            if (!(strcmp(filter_user, uid) == 0 || strcmp(filter_user, username) == 0)) continue;
        }
        if (filter_file) {
            // match substring of path
            if (!strstr(path, filter_file)) continue;
        }
        if (filter_op) {
            if (strcmp(filter_op, op) != 0) continue;
        }

        printf("%s | uid=%s(%s) pid=%s | %s %s | result=%s\n", ts, uid, username, pid, op, path, result);
    }

    fclose(f);
    return 0;
}
