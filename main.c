#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "operations.h"
#include "logging.h"

int main(int argc, char *argv[]) {
    // Initialize logging
    init_logging("virtual_fs.log");

    // Log startup event
    log_event("START", "/", (pid_t)getpid(), (uid_t)getuid(), 0);

    // Run FUSE main loop
    return fuse_main(argc, argv, &vfs_operations, NULL);
}