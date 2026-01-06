#define _XOPEN_SOURCE 700
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h> 
#include "logging.h"
#include "permissions.h"

// Thư mục lưu trữ các file bị thay đổi (Lớp phủ)
#define STORAGE_DIR ".vfs_storage"
// Thư mục lưu trữ backup phiên bản cũ
#define BACKUP_DIR ".backup"

extern char g_source_dir[PATH_MAX]; // Định nghĩa bên main.c
static int backup_counter = 0;      // Biến đếm để tránh trùng tên file backup

// --- CÁC HÀM HELPER VỀ ĐƯỜNG DẪN ---

static void get_source_path(char fpath[PATH_MAX], const char *path) {
    snprintf(fpath, PATH_MAX, "%s%s", g_source_dir, path);
}

static void get_storage_path(char fpath[PATH_MAX], const char *path) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        snprintf(fpath, PATH_MAX, "%s/%s%s", cwd, STORAGE_DIR, path);
    }
}

// Hàm đệ quy tạo thư mục cha (mkdir -p)
static int mkdir_p(const char *path) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') tmp[len - 1] = 0;
    
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    return mkdir(tmp, 0755);
}

// --- HÀM BACKUP (VERSIONING) ---

static int save_backup(const char *path) {
    char source_file_path[PATH_MAX];
    char storage_file_path[PATH_MAX];
    char final_read_path[PATH_MAX];

    // 1. Xác định file đang nằm ở đâu để đọc dữ liệu backup
    get_storage_path(storage_file_path, path);
    get_source_path(source_file_path, path);

    // Ưu tiên backup phiên bản trong Storage (nếu đã từng sửa)
    if (access(storage_file_path, F_OK) == 0) {
        strcpy(final_read_path, storage_file_path);
    } else if (access(source_file_path, F_OK) == 0) {
        strcpy(final_read_path, source_file_path);
    } else {
        return 0; // File không tồn tại -> Không cần backup
    }

    // Kiểm tra file có rỗng không? (Tùy chọn: Nếu muốn backup cả file rỗng thì bỏ đoạn này)
    struct stat st_check;
    if (stat(final_read_path, &st_check) == 0) {
        if (st_check.st_size == 0) {
            // File rỗng, có thể không cần backup hoặc vẫn backup tùy nhu cầu.
            // Ở đây tôi vẫn cho backup để theo dõi lịch sử đầy đủ.
        }
    }

    // 2. Chuẩn bị thư mục .backup
    struct stat st = {0};
    if (stat(BACKUP_DIR, &st) == -1) {
        if (mkdir(BACKUP_DIR, 0755) == -1) return -1;
    }

    // 3. Tạo tên file backup
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", &tm);

    const char *fname = path;
    if (fname[0] == '/') fname++;
    if (fname[0] == '\0') fname = "root";

    char safe_fname[PATH_MAX];
    snprintf(safe_fname, sizeof(safe_fname), "%s", fname);
    for(int i=0; safe_fname[i]; i++) {
        if(safe_fname[i] == '/') safe_fname[i] = '_';
    }

    char outpath[PATH_MAX];
    snprintf(outpath, sizeof(outpath), "%s/%s_%s_%03d.bak", 
             BACKUP_DIR, safe_fname, ts, backup_counter++);

    // 4. Copy dữ liệu
    FILE *src = fopen(final_read_path, "rb");
    if (!src) return -1;
    
    FILE *dst = fopen(outpath, "wb");
    if (!dst) { fclose(src); return -1; }

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        fwrite(buf, 1, n, dst);
    }
    
    fclose(src);
    fclose(dst);

    struct fuse_context *ctx = fuse_get_context();
    if (ctx) log_event("BACKUP_CREATED", path, ctx->pid, ctx->uid, 0);

    return 0;
}

// --- CƠ CHẾ COPY-ON-WRITE (STORAGE) ---

static int copy_source_to_storage(const char *path) {
    char src_path[PATH_MAX];
    char dst_path[PATH_MAX];
    
    get_source_path(src_path, path);
    get_storage_path(dst_path, path);

    if (access(src_path, F_OK) == -1) return -errno;

    char *dst_dir = strdup(dst_path);
    mkdir_p(dirname(dst_dir));
    free(dst_dir);

    FILE *src = fopen(src_path, "rb");
    if (!src) return -errno;
    
    FILE *dst = fopen(dst_path, "wb");
    if (!dst) { fclose(src); return -errno; }

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        fwrite(buf, 1, n, dst);
    }
    
    struct stat st;
    if (fstat(fileno(src), &st) == 0) {
        fchmod(fileno(dst), st.st_mode);
    }

    fclose(src);
    fclose(dst);
    return 0;
}

// --- FUSE OPERATIONS ---

static int vfs_getattr(const char *path, struct stat *stbuf) {
    char fpath[PATH_MAX];
    int res;

    get_storage_path(fpath, path);
    res = lstat(fpath, stbuf);

    if (res == -1 && errno == ENOENT) {
        get_source_path(fpath, path);
        res = lstat(fpath, stbuf);
    }

    if (res == -1) return -errno;
    return 0;
}

static int vfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    char source_path[PATH_MAX];
    char storage_path[PATH_MAX];
    
    struct fuse_context *ctx = fuse_get_context();
    
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    get_source_path(source_path, path);
    get_storage_path(storage_path, path);

    // 1. STORAGE
    DIR *dp_storage = opendir(storage_path);
    if (dp_storage != NULL) {
        struct dirent *de;
        while ((de = readdir(dp_storage)) != NULL) {
            if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
            
            struct stat st;
            memset(&st, 0, sizeof(st));
            st.st_ino = de->d_ino;
            st.st_mode = de->d_type << 12;
            filler(buf, de->d_name, &st, 0);
        }
        closedir(dp_storage);
    }

    // 2. SOURCE (Deduplicate)
    DIR *dp_source = opendir(source_path);
    if (dp_source != NULL) {
        struct dirent *de;
        while ((de = readdir(dp_source)) != NULL) {
            if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;

            char check_duplicate_path[PATH_MAX];
            snprintf(check_duplicate_path, PATH_MAX, "%s/%s", storage_path, de->d_name);
            
            if (access(check_duplicate_path, F_OK) == 0) {
                continue; 
            }

            struct stat st;
            memset(&st, 0, sizeof(st));
            st.st_ino = de->d_ino;
            st.st_mode = de->d_type << 12;
            filler(buf, de->d_name, &st, 0);
        }
        closedir(dp_source);
    }

    if (ctx) log_event("READDIR", path, ctx->pid, ctx->uid, 0);
    return 0;
}

static int vfs_open(const char *path, struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    struct stat st;
    
    // 1. Xác định file nằm ở đâu (Storage hay Source)
    get_storage_path(fpath, path);
    if (access(fpath, F_OK) == -1) {
        get_source_path(fpath, path);
    }

    // 2. Lấy thông tin file (Owner, Mode) để kiểm tra quyền
    // Nếu file không tồn tại -> Lỗi
    if (lstat(fpath, &st) == -1) return -errno;

    // 3. --- QUAN TRỌNG: GỌI HÀM KIỂM TRA QUYỀN TẠI ĐÂY ---
    // Kiểm tra xem user hiện tại có quyền mở file với flag này không (Read/Write)
    if (!check_permissions(fi->flags, st.st_mode, st.st_uid, st.st_gid)) {
        struct fuse_context *ctx = fuse_get_context();
        if (ctx) log_event("OPEN_DENIED", path, ctx->pid, ctx->uid, -EACCES);
        return -EACCES; // Trả về lỗi Permission Denied ngay lập tức
    }
    // ----------------------------------------------------

    // 4. Logic Copy-On-Write (Nếu mở để GHI)
    if ((fi->flags & O_ACCMODE) != O_RDONLY) {
        // Phải trỏ lại fpath về Storage để chuẩn bị copy
        get_storage_path(fpath, path);
        
        // Nếu file chưa có bên Storage -> Copy sang
        if (access(fpath, F_OK) == -1) {
            int copy_res = copy_source_to_storage(path);
            if (copy_res != 0) return copy_res;
        }

        // Xử lý O_TRUNC (Backup trước khi xóa trắng nội dung)
        if (fi->flags & O_TRUNC) {
            save_backup(path); 
        }
    }

    // 5. Thực hiện mở file thật
    int fd = open(fpath, fi->flags);
    if (fd == -1) return -errno;
    close(fd);
    
    struct fuse_context *ctx = fuse_get_context();
    if (ctx) log_event("OPEN", path, ctx->pid, ctx->uid, 0);
    return 0;
}

static int vfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    struct stat st;

    // 1. Tìm đường dẫn file
    get_storage_path(fpath, path);
    if (access(fpath, F_OK) == -1) {
        get_source_path(fpath, path);
    }

    // 2. --- KIỂM TRA QUYỀN ĐỌC ---
    // Mặc dù open đã check, nhưng check lại ở đây cho chắc chắn (vì logic open/read stateless)
    if (lstat(fpath, &st) == -1) return -errno;
    if (!check_permission(st.st_mode, st.st_uid, st.st_gid, 4)) { // 4 = READ permission
        return -EACCES;
    }
    // ----------------------------

    int fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;

    int res = pread(fd, buf, size, offset);
    if (res == -1) res = -errno;
    
    close(fd);
    
    struct fuse_context *ctx = fuse_get_context();
    if (ctx) log_event("READ", path, ctx->pid, ctx->uid, res);
    return res;
}

static int vfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    
    // Lưu ý: Logic check quyền cho Write hơi phức tạp vì file có thể chưa có ở Storage
    // Ta kiểm tra quyền trên file HIỆN CÓ (dù là Source hay Storage) trước khi Copy
    
    // 1. Tìm file gốc hiện tại để check quyền
    char current_path[PATH_MAX];
    get_storage_path(current_path, path);
    if (access(current_path, F_OK) == -1) {
        get_source_path(current_path, path);
    }
    
    // 2. --- KIỂM TRA QUYỀN GHI ---
    struct stat st;
    if (lstat(current_path, &st) == -1) return -errno;
    
    // Check quyền Write (2) trên file gốc
    if (!check_permission(st.st_mode, st.st_uid, st.st_gid, 2)) {
        struct fuse_context *ctx = fuse_get_context();
        if (ctx) log_event("WRITE_DENIED", path, ctx->pid, ctx->uid, -EACCES);
        return -EACCES;
    }
    // ----------------------------

    // 3. Logic Copy-On-Write
    get_storage_path(fpath, path);
    if (access(fpath, F_OK) == -1) {
        int copy_res = copy_source_to_storage(path);
        if (copy_res != 0) return copy_res;
    }

    // 4. Backup
    save_backup(path);

    // 5. Ghi
    int fd = open(fpath, O_WRONLY);
    if (fd == -1) return -errno;

    int res = pwrite(fd, buf, size, offset);
    if (res == -1) res = -errno;

    close(fd);

    struct fuse_context *ctx = fuse_get_context();
    if (ctx) log_event("WRITE", path, ctx->pid, ctx->uid, res);
    return res;
}

static int vfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    get_storage_path(fpath, path);

    char *tmp_path = strdup(fpath);
    mkdir_p(dirname(tmp_path));
    free(tmp_path);

    int res = creat(fpath, mode);
    if (res == -1) return -errno;
    close(res);

    struct fuse_context *ctx = fuse_get_context();
    if (ctx) log_event("CREATE", path, ctx->pid, ctx->uid, 0);
    return 0;
}

static int vfs_mkdir(const char *path, mode_t mode) {
    char fpath[PATH_MAX];
    get_storage_path(fpath, path);
    int res = mkdir_p(fpath);
    
    struct fuse_context *ctx = fuse_get_context();
    if (ctx) log_event("MKDIR", path, ctx->pid, ctx->uid, res == -1 ? -errno : 0);
    return res == -1 ? -errno : 0;
}

// unlink để xóa file
static int vfs_unlink(const char *path) {
    char fpath[PATH_MAX];
    get_storage_path(fpath, path);
    struct fuse_context *ctx = fuse_get_context();

    if (access(fpath, F_OK) == 0) {
        save_backup(path); // Backup trước khi xóa
        int res = unlink(fpath);
        if (ctx) log_event("UNLINK (Storage)", path, ctx->pid, ctx->uid, res == -1 ? -errno : 0);
        return res == -1 ? -errno : 0;
    }

    if (ctx) log_event("UNLINK (Source-Protected)", path, ctx->pid, ctx->uid, -EACCES);
    return -EACCES; 
}

static int vfs_truncate(const char *path, off_t size) {
    char fpath[PATH_MAX];
    get_storage_path(fpath, path);

    if (access(fpath, F_OK) == -1) {
        int copy_res = copy_source_to_storage(path);
        if (copy_res != 0) return copy_res;
    }

    // --- FIX: Backup file trước khi TRUNCATE (Cắt file) ---
    // Đây là bước quan trọng nhất để sửa lỗi của bạn
    save_backup(path); 
    // -----------------------------------------------------

    int res = truncate(fpath, size);
    return res == -1 ? -errno : 0;
}

static int vfs_chmod(const char *path, mode_t mode) {
    char fpath[PATH_MAX];
    get_storage_path(fpath, path);

    if (access(fpath, F_OK) == -1) {
        int copy_res = copy_source_to_storage(path);
        if (copy_res != 0) return copy_res;
    }
    
    int res = chmod(fpath, mode);
    return res == -1 ? -errno : 0;
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
    .mkdir = vfs_mkdir,
    .create = vfs_create,
};