#define _XOPEN_SOURCE 700
#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include "logging.h"
#include "permissions.h"

#define MAX_FILE_SIZE 1024

// Virtual file content and metadata
static char virtual_file[MAX_FILE_SIZE];
static size_t virtual_file_size = 0;
static int virtual_file_permissions = 0644; // Default permissions: rw-r--r--

// Helper: create .backup dir and write current file content into timestamped file
static int save_backup(const char *path) {
    const char *backup_dir = ".backup";
    struct stat st = {0};
    if (stat(backup_dir, &st) == -1) {
        if (mkdir(backup_dir, 0755) == -1) {
            return -1;
        }
    }

    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", &tm);

    // sanitize filename (remove leading '/')
    const char *fname = path;
    if (fname[0] == '/') fname++;
    if (fname[0] == '\0') fname = "root";

    char outpath[1024];
    snprintf(outpath, sizeof(outpath), "%s/%s_%s.bak", backup_dir, fname, ts);

    FILE *f = fopen(outpath, "wb");
    if (!f) return -1;
    if (virtual_file_size > 0) {
        size_t written = fwrite(virtual_file, 1, virtual_file_size, f);
        (void)written;
    }
    fclose(f);
    return 0;
}

static int vfs_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    stbuf->st_uid = getuid(); // <--- Thêm dòng này
    stbuf->st_gid = getgid(); // <--- Thêm dòng này

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (strcmp(path, "/virtual_file") == 0) {
        stbuf->st_mode = S_IFREG | virtual_file_permissions;
        stbuf->st_nlink = 1;
        stbuf->st_size = virtual_file_size;
    } else {
        struct fuse_context *ctx = fuse_get_context();
        if (ctx) log_event("GETATTR", path, ctx->pid, ctx->uid, -ENOENT);
        return -ENOENT;
    }

    struct fuse_context *ctx = fuse_get_context();
    if (ctx) log_event("GETATTR", path, ctx->pid, ctx->uid, 0);
    return 0;
}

static int vfs_open(const char *path, struct fuse_file_info *fi) {
    struct fuse_context *ctx = fuse_get_context();
    if (strcmp(path, "/virtual_file") != 0) {
        if (ctx) log_event("OPEN", path, ctx->pid, ctx->uid, -ENOENT);
        return -ENOENT;
    }

    if (!check_permissions(fi->flags, virtual_file_permissions, get_virtual_file_uid(), get_virtual_file_gid())) {
        if (ctx) log_event("OPEN", path, ctx->pid, ctx->uid, -EACCES);
        return -EACCES;
    }

    if (ctx) log_event("OPEN", path, ctx->pid, ctx->uid, 0);
    return 0;
}

static int vfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    struct fuse_context *ctx = fuse_get_context();

    if (strcmp(path, "/virtual_file") != 0) {
        if (ctx) log_event("READ", path, ctx->pid, ctx->uid, -ENOENT);
        return -ENOENT;
    }

    if (!(virtual_file_permissions & 0444)) {
        if (ctx) log_event("READ", path, ctx->pid, ctx->uid, -EACCES);
        return -EACCES;
    }

    if (offset >= virtual_file_size) {
        if (ctx) log_event("READ", path, ctx->pid, ctx->uid, 0);
        return 0;
    }

    if (offset + size > virtual_file_size) {
        size = virtual_file_size - offset;
    }

    memcpy(buf, virtual_file + offset, size);
    if (ctx) log_event("READ", path, ctx->pid, ctx->uid, (int)size);
    return size;
}

static int vfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    struct fuse_context *ctx = fuse_get_context();

    if (strcmp(path, "/virtual_file") != 0) {
        if (ctx) log_event("WRITE", path, ctx->pid, ctx->uid, -ENOENT);
        return -ENOENT;
    }

    if (!(virtual_file_permissions & 0222)) {
        if (ctx) log_event("WRITE", path, ctx->pid, ctx->uid, -EACCES);
        return -EACCES;
    }

    if (offset + size > MAX_FILE_SIZE) {
        if (ctx) log_event("WRITE", path, ctx->pid, ctx->uid, -EFBIG);
        return -EFBIG;
    }

    // Save a backup of current content before modification
    save_backup(path);

    memcpy(virtual_file + offset, buf, size);
    if (offset + size > virtual_file_size) {
        virtual_file_size = offset + size;
    }

    if (ctx) log_event("WRITE", path, ctx->pid, ctx->uid, (int)size);
    return size;
}

static int vfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, "virtual_file", NULL, 0);

    struct fuse_context *ctx = fuse_get_context();
    if (ctx) log_event("READDIR", path, ctx->pid, ctx->uid, 0);
    return 0;
}

static int vfs_truncate(const char *path, off_t size) {
    struct fuse_context *ctx = fuse_get_context();
    if (strcmp(path, "/virtual_file") != 0) {
        if (ctx) log_event("TRUNCATE", path, ctx->pid, ctx->uid, -ENOENT);
        return -ENOENT;
    }

    if (size > MAX_FILE_SIZE) {
        if (ctx) log_event("TRUNCATE", path, ctx->pid, ctx->uid, -EFBIG);
        return -EFBIG;
    }

    // Backup before truncation
    save_backup(path);

    virtual_file_size = size;
    if (ctx) log_event("TRUNCATE", path, ctx->pid, ctx->uid, 0);
    return 0;
}

static int vfs_chmod(const char *path, mode_t mode) {
    struct fuse_context *ctx = fuse_get_context();
    if (strcmp(path, "/virtual_file") != 0) {
        if (ctx) log_event("CHMOD", path, ctx->pid, ctx->uid, -ENOENT);
        return -ENOENT;
    }
    update_virtual_file_permissions(&virtual_file_permissions, mode);
    if (ctx) log_event("CHMOD", path, ctx->pid, ctx->uid, 0);
    return 0;
}

// Instead of permanently deleting, move content to .backup and clear in-memory file
static int vfs_unlink(const char *path) {
    struct fuse_context *ctx = fuse_get_context();
    if (strcmp(path, "/virtual_file") != 0) {
        if (ctx) log_event("UNLINK", path, ctx->pid, ctx->uid, -ENOENT);
        return -ENOENT;
    }

    // Save backup and clear content
    save_backup(path);
    memset(virtual_file, 0, sizeof(virtual_file));
    virtual_file_size = 0;

    if (ctx) log_event("UNLINK", path, ctx->pid, ctx->uid, 0);
    return 0;
}

struct fuse_operations vfs_operations = {
    .getattr = vfs_getattr,
    .open = vfs_open,
    .read = vfs_read,
    .write = vfs_write,
    .readdir = vfs_readdir,
    .truncate = vfs_truncate,
    .chmod = vfs_chmod,
    .unlink = vfs_unlink,
};