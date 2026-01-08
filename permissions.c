#define _XOPEN_SOURCE 700
#define FUSE_USE_VERSION 26
#include <fuse.h>         // <--- Bắt buộc phải có để dùng fuse_get_context
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include "permissions.h"

// Biến toàn cục lưu UID/GID mặc định (khi khởi tạo)
static uid_t virtual_file_uid = 0;
static gid_t virtual_file_gid = 0;
static int initialized = 0;

// Helper: Tách bit permission (như cũ)
static int get_permission_bits(int mode, int is_user, int is_group) {
    if (is_user) return (mode >> 6) & 0x7;
    if (is_group) return (mode >> 3) & 0x7;
    return mode & 0x7;
}

// --- HÀM QUAN TRỌNG ĐÃ SỬA ---
// Helper: check if the CALLING PROCESS has the requested permission
// perm: 4=read, 2=write, 1=exec
int check_permission(int mode, uid_t file_uid, gid_t file_gid, int perm) {
    struct fuse_context *ctx = fuse_get_context();
    if (!ctx) return 0;

    uid_t caller_uid = ctx->uid;
    gid_t caller_gid = ctx->gid;
    int bits = 0;

    // --- THÊM LOG DEBUG TẠI ĐÂY ---
    // In ra dạng Octal (%o) để xem quyền (VD: 755, 644)
    printf("[DEBUG-PERM] Caller=%d, FileUID=%d, Mode=%o, Request=%d\n", 
            caller_uid, file_uid, mode, perm);
    // -----------------------------

    if (caller_uid == 0) return 1; 

    if (caller_uid == file_uid) 
        bits = get_permission_bits(mode, 1, 0);
    else if (caller_gid == file_gid) 
        bits = get_permission_bits(mode, 0, 1);
    else 
        bits = get_permission_bits(mode, 0, 0);

    int result = (bits & perm) ? 1 : 0;
    
    // In kết quả quyết định
    if (result == 0) printf(" -> [DENIED]\n");
    else printf(" -> [ALLOWED]\n");

    return result;
}

// FUSE-style: check open flags against permissions
int check_permissions(int flags, int mode, uid_t file_uid, gid_t file_gid) {
    // Nếu tạo file mới (O_CREAT), ta thường bỏ qua check permission lúc open
    if (flags & O_CREAT) return 1;

    int r = 1;
    // Kiểm tra quyền truy cập dựa trên flags
    if ((flags & O_ACCMODE) == O_RDONLY)
        r = check_permission(mode, file_uid, file_gid, 4); // Check Read
    else if ((flags & O_ACCMODE) == O_WRONLY)
        r = check_permission(mode, file_uid, file_gid, 2); // Check Write
    else if ((flags & O_ACCMODE) == O_RDWR)
        r = check_permission(mode, file_uid, file_gid, 6); // Check Read + Write
    
    return r;
}

int check_chown_permission(uid_t file_uid, uid_t new_uid, gid_t new_gid) {
    struct fuse_context *ctx = fuse_get_context();
    if (!ctx) return 0;

    uid_t caller_uid = ctx->uid;

    // 1. Nếu là Root -> Cho phép tất cả
    if (caller_uid == 0) return 1;

    // 2. Nếu người gọi KHÔNG PHẢI là chủ sở hữu file -> Cấm tiệt
    if (caller_uid != file_uid) {
        printf("[DEBUG-CHOWN] Denied: Caller %d is not owner %d\n", caller_uid, file_uid);
        return 0;
    }

    // 3. Nếu là Owner file:
    // Linux chuẩn rất khắt khe: Owner không được đổi UID của file sang người khác (để tránh đẩy file rác).
    // Tuy nhiên, để bạn dễ test trong đồ án này, ta có thể "nới lỏng" luật:
    // -> Cho phép Owner đổi quyền sở hữu (nếu bạn muốn).
    
    // Nếu muốn làm CHUẨN LINUX (Khắt khe):
    if (new_uid != -1 && new_uid != file_uid) {
        printf("[DEBUG-CHOWN] Denied: Non-root user cannot change UID\n");
        return 0; // Cấm user thường chuyển file cho người khác
    }

    // Nếu chỉ đổi Group (new_uid == -1) -> Cho phép (đơn giản hóa)
    return 1;
}

// Các hàm cập nhật metadata (như cũ)
void update_virtual_file_permissions(int *virtual_file_permissions, int new_mode) {
    *virtual_file_permissions = new_mode & 0777;
}

void update_virtual_file_owner(uid_t *file_uid, gid_t *file_gid, uid_t new_uid, gid_t new_gid) {
    if (file_uid) *file_uid = new_uid;
    if (file_gid) *file_gid = new_gid;
}

// Hàm khởi tạo UID/GID (như cũ nhưng sửa lại cho gọn)
uid_t get_virtual_file_uid() {
    if (!initialized) {
        // Mặc định lấy UID của người chạy ./vfs
        virtual_file_uid = getuid(); 
        virtual_file_gid = getgid();
        initialized = 1;
    }
    return virtual_file_uid;
}

gid_t get_virtual_file_gid() {
    if (!initialized) {
        virtual_file_uid = getuid();
        virtual_file_gid = getgid();
        initialized = 1;
    }
    return virtual_file_gid;
}

void set_virtual_file_uid(uid_t uid) { virtual_file_uid = uid; }
void set_virtual_file_gid(gid_t gid) { virtual_file_gid = gid; }