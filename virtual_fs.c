#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

// Define the maximum size of the virtual file
#define MAX_FILE_SIZE 1024

// Virtual file content and metadata
static char virtual_file[MAX_FILE_SIZE];
static size_t virtual_file_size = 0;
static int virtual_file_permissions = 0644; // Default permissions: rw-r--r--

// Log file path
static const char *log_path = "virtual_fs.log";

// Helper function to log operations
void log_operation(const char *operation, const char *path, int result) {
    FILE *log_file = fopen(log_path, "a");
    if (log_file) {
        fprintf(log_file, "%s: %s (result: %d)\n", operation, path, result);
        fclose(log_file);
    }
}

// Get file attributes
static int vfs_getattr(const char *path, struct stat *stbuf) { //struct fuse_file_info *fi for version 3 of FUSE
    // (void) fi; // Unused parameter
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (strcmp(path, "/virtual_file") == 0) {
        stbuf->st_mode = S_IFREG | virtual_file_permissions;
        stbuf->st_nlink = 1;
        stbuf->st_size = virtual_file_size;
    } else {
        log_operation("getattr", path, -ENOENT);
        return -ENOENT;
    }

    log_operation("getattr", path, 0);
    return 0;
}

// Open a file
static int vfs_open(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path, "/virtual_file") != 0) {
        log_operation("open", path, -ENOENT);
        return -ENOENT;
    }

    // Check permissions
    if ((fi->flags & O_WRONLY) && !(virtual_file_permissions & 0222)) {
        log_operation("open", path, -EACCES);
        return -EACCES;
    }
    if ((fi->flags & O_RDONLY) && !(virtual_file_permissions & 0444)) {
        log_operation("open", path, -EACCES);
        return -EACCES;
    }

    log_operation("open", path, 0);
    return 0;
}

// Read from a file
static int vfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi; // Unused parameter

    if (strcmp(path, "/virtual_file") != 0) {
        log_operation("read", path, -ENOENT);
        return -ENOENT;
    }

    if (!(virtual_file_permissions & 0444)) {
        log_operation("read", path, -EACCES);
        return -EACCES;
    }

    if (offset >= virtual_file_size) {
        log_operation("read", path, 0);
        return 0;
    }

    if (offset + size > virtual_file_size) {
        size = virtual_file_size - offset;
    }

    memcpy(buf, virtual_file + offset, size);
    log_operation("read", path, size);
    return size;
}

// Write to a file
static int vfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi; // Unused parameter

    if (strcmp(path, "/virtual_file") != 0) {
        log_operation("write", path, -ENOENT);
        return -ENOENT;
    }

    if (!(virtual_file_permissions & 0222)) {
        log_operation("write", path, -EACCES);
        return -EACCES;
    }

    if (offset + size > MAX_FILE_SIZE) {
        log_operation("write", path, -EFBIG);
        return -EFBIG;
    }

    memcpy(virtual_file + offset, buf, size);
    if (offset + size > virtual_file_size) {
        virtual_file_size = offset + size;
    }

    log_operation("write", path, size);
    return size;
}

// File system operations structure
static struct fuse_operations vfs_operations = {
    .getattr = vfs_getattr,
    .open = vfs_open,
    .read = vfs_read,
    .write = vfs_write,
};

int main(int argc, char *argv[]) {
    // Clear the log file at startup
    FILE *log_file = fopen(log_path, "w");
    if (log_file) {
        fprintf(log_file, "Starting virtual file system\n");
        fclose(log_file);
    }

    return fuse_main(argc, argv, &vfs_operations, NULL);
}