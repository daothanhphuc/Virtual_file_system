#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include "operations.h"
#include "logging.h"

int main(int argc, char *argv[]) {
    // Initialize logging
    init_logging("virtual_fs.log");

    // Log startup message
    log_message("Starting virtual file system\n");

    // Run FUSE main loop
    return fuse_main(argc, argv, &vfs_operations, NULL);
}