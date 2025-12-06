#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "logging.h"
#include "permissions.h"

#define MAX_FILE_SIZE 1024

// Virtual file content and metadata
static char virtual_file[MAX_FILE_SIZE];
static size_t virtual_file_size = 0;
static int virtual_file_permissions = 0644; // Default permissions: rw-r--r--

// --- SỬA 1: Thêm lại tham số fuse_file_info *fi ---
static int vfs_getattr(const char *path, struct stat *stbuf) { // Lưu ý: getattr chuẩn FUSE 2 không có fi, nhưng FUSE 3 có. Với code của bạn giữ nguyên getattr như cũ là OK với FUSE 2.
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (strcmp(path, "/virtual_file") == 0) {
        stbuf->st_mode = S_IFREG | virtual_file_permissions;
        stbuf->st_nlink = 1;
        stbuf->st_size = virtual_file_size;
    } else {
        log_message("getattr: File not found\n");
        return -ENOENT;
    }

    log_message("getattr: Success\n");
    return 0;
}

// --- SỬA 2: Khôi phục tham số fi và sửa lỗi gọi hàm check_permissions ---
static int vfs_open(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path, "/virtual_file") != 0) {
        log_message("open: File not found\n");
        return -ENOENT;
    }

    // SỬA: Truyền thêm fi->flags vào tham số đầu tiên
    if (!check_permissions(fi->flags, virtual_file_permissions)) {
        log_message("open: Permission denied\n");
        return -EACCES;
    }

    log_message("open: Success\n");
    return 0;
}

// --- SỬA 3: Khôi phục tham số fi để đúng chuẩn function pointer ---
static int vfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi; // Đánh dấu là đã dùng để compiler không cảnh báo unused variable

    if (strcmp(path, "/virtual_file") != 0) {
        log_message("read: File not found\n");
        return -ENOENT;
    }

    if (!(virtual_file_permissions & 0444)) {
        log_message("read: Permission denied\n");
        return -EACCES;
    }

    if (offset >= virtual_file_size) {
        log_message("read: Offset beyond file size\n");
        return 0;
    }

    if (offset + size > virtual_file_size) {
        size = virtual_file_size - offset;
    }

    memcpy(buf, virtual_file + offset, size);
    log_message("read: Success\n");
    return size;
}

// --- SỬA 4: Khôi phục tham số fi ---
static int vfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;

    if (strcmp(path, "/virtual_file") != 0) {
        log_message("write: File not found\n");
        return -ENOENT;
    }

    if (!(virtual_file_permissions & 0222)) {
        log_message("write: Permission denied\n");
        return -EACCES;
    }

    if (offset + size > MAX_FILE_SIZE) {
        log_message("write: File too large\n");
        return -EFBIG;
    }

    memcpy(virtual_file + offset, buf, size);
    if (offset + size > virtual_file_size) {
        virtual_file_size = offset + size;
    }

    log_message("write: Success\n");
    return size;
}

// Hàm đọc thư mục (để sửa lỗi ls)
static int vfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    // Chỉ có thư mục gốc "/" là hợp lệ
    if (strcmp(path, "/") != 0)
        return -ENOENT;

    // Khai báo các thư mục mặc định "." và ".."
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    // Hiển thị file ảo của chúng ta
    filler(buf, "virtual_file", NULL, 0);

    return 0;
}

// Hàm cắt file (để sửa lỗi echo > file)
static int vfs_truncate(const char *path, off_t size) {
    if (strcmp(path, "/virtual_file") != 0) {
        return -ENOENT;
    }

    if (size > MAX_FILE_SIZE) {
        return -EFBIG;
    }

    // Cập nhật lại kích thước file
    virtual_file_size = size;
    return 0;
}

struct fuse_operations vfs_operations = {
    .getattr = vfs_getattr,
    .open = vfs_open,
    .read = vfs_read,
    .write = vfs_write,
    .readdir = vfs_readdir,  
    .truncate = vfs_truncate,
};