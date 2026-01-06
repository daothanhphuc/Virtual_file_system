#define _XOPEN_SOURCE 700
#define FUSE_USE_VERSION 26  // Bắt buộc định nghĩa phiên bản FUSE
#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "logging.h"
#include "permissions.h"

#define MAX_FILE_SIZE 1024

// Virtual file content and metadata
static char virtual_file[MAX_FILE_SIZE];
static size_t virtual_file_size = 0;
static int virtual_file_permissions = 0644; // Default permissions: rw-r--r--

// --- FIX: Thêm biến đếm để tránh trùng tên file backup trong cùng 1 giây ---
static int backup_counter = 0;

// Helper: create .backup dir and write current file content into timestamped file
static int save_backup(const char *path) {
    const char *backup_dir = ".backup";
    struct stat st = {0};
    
    // Tạo thư mục .backup nếu chưa có
    if (stat(backup_dir, &st) == -1) {
        if (mkdir(backup_dir, 0755) == -1) {
            struct fuse_context *ctx = fuse_get_context();
            if (ctx) log_event("BACKUP_ERROR", path, ctx->pid, ctx->uid, -1);
            return -1;
        }
    }

    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", &tm);

    // Xử lý tên file (bỏ dấu / ở đầu)
    const char *fname = path;
    if (fname[0] == '/') fname++;
    if (fname[0] == '\0') fname = "root";

    char outpath[1024];
    
    // --- FIX: Thêm backup_counter vào tên file để không bị trùng ---
    snprintf(outpath, sizeof(outpath), "%s/%s_%s_%03d.bak", 
             backup_dir, fname, ts, backup_counter++);
    // -------------------------------------------------------------

    FILE *f = fopen(outpath, "wb");
    if (!f) {
        struct fuse_context *ctx = fuse_get_context();
        if (ctx) log_event("BACKUP_FAIL", path, ctx->pid, ctx->uid, -1);
        return -1;
    }

    // Chỉ ghi nội dung nếu file gốc có dữ liệu
    if (virtual_file_size > 0) {
        fwrite(virtual_file, 1, virtual_file_size, f);
    }
    
    fclose(f);

    // Ghi log backup thành công
    struct fuse_context *ctx = fuse_get_context();
    if (ctx) log_event("BACKUP", path, ctx->pid, ctx->uid, (int)virtual_file_size);
    
    return 0;
}

static int vfs_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    // Quan trọng: Lấy quyền của user hiện tại để không bị Permission Denied
    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (strcmp(path, "/virtual_file") == 0) {
        stbuf->st_mode = S_IFREG | virtual_file_permissions;
        stbuf->st_nlink = 1;
        stbuf->st_size = virtual_file_size;
    } else {
        return -ENOENT;
    }
    return 0;
}

static int vfs_open(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path, "/virtual_file") != 0) return -ENOENT;

    struct fuse_context *ctx = fuse_get_context();
    
    if (!check_permissions(fi->flags, virtual_file_permissions, get_virtual_file_uid(), get_virtual_file_gid())) {
        if (ctx) log_event("OPEN", path, ctx->pid, ctx->uid, -EACCES);
        return -EACCES;
    }

    if (ctx) log_event("OPEN", path, ctx->pid, ctx->uid, 0);
    return 0;
}

static int vfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    if (strcmp(path, "/virtual_file") != 0) return -ENOENT;

    struct fuse_context *ctx = fuse_get_context();

    if (!(virtual_file_permissions & 0444)) {
        if (ctx) log_event("READ", path, ctx->pid, ctx->uid, -EACCES);
        return -EACCES;
    }

    if (offset >= virtual_file_size) return 0;
    if (offset + size > virtual_file_size) size = virtual_file_size - offset;

    memcpy(buf, virtual_file + offset, size);
    
    if (ctx) log_event("READ", path, ctx->pid, ctx->uid, (int)size);
    return size;
}

static int vfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    if (strcmp(path, "/virtual_file") != 0) return -ENOENT;

    struct fuse_context *ctx = fuse_get_context();

    if (!(virtual_file_permissions & 0222)) {
        if (ctx) log_event("WRITE", path, ctx->pid, ctx->uid, -EACCES);
        return -EACCES;
    }

    if (offset + size > MAX_FILE_SIZE) {
        if (ctx) log_event("WRITE", path, ctx->pid, ctx->uid, -EFBIG);
        return -EFBIG;
    }

    // Backup TRƯỚC khi ghi đè
    // Chỉ backup khi file hiện tại đang có dữ liệu (size > 0)
    if (virtual_file_size > 0) {
        save_backup(path);
    }

    memcpy(virtual_file + offset, buf, size);
    if (offset + size > virtual_file_size) {
        virtual_file_size = offset + size;
    }

    if (ctx) log_event("WRITE", path, ctx->pid, ctx->uid, (int)size);
    return size;
}

static int vfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset; (void) fi;

    if (strcmp(path, "/") != 0) return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, "virtual_file", NULL, 0);

    return 0;
}

static int vfs_truncate(const char *path, off_t size) {
    if (strcmp(path, "/virtual_file") != 0) return -ENOENT;
    
    struct fuse_context *ctx = fuse_get_context();

    if (size > MAX_FILE_SIZE) return -EFBIG;

    // Backup TRƯỚC khi cắt file (truncate thường làm mất dữ liệu cũ)
    if (virtual_file_size > 0) {
        save_backup(path);
    }

    virtual_file_size = size;
    if (ctx) log_event("TRUNCATE", path, ctx->pid, ctx->uid, 0);
    return 0;
}

static int vfs_chmod(const char *path, mode_t mode) {
    if (strcmp(path, "/virtual_file") != 0) return -ENOENT;
    
    struct fuse_context *ctx = fuse_get_context();
    update_virtual_file_permissions(&virtual_file_permissions, mode);
    
    if (ctx) log_event("CHMOD", path, ctx->pid, ctx->uid, 0);
    return 0;
}

static int vfs_unlink(const char *path) {
    if (strcmp(path, "/virtual_file") != 0) return -ENOENT;

    struct fuse_context *ctx = fuse_get_context();

    // Backup file lần cuối trước khi xóa
    if (virtual_file_size > 0) {
        save_backup(path);
    }
    
    // Xóa nội dung trong RAM
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