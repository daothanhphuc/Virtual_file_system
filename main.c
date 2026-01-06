#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include "operations.h"
#include "logging.h"

// Biến toàn cục lưu đường dẫn Source
char g_source_dir[PATH_MAX];

int main(int argc, char *argv[]) {
    // Initialize logging
    init_logging("virtual_fs.log");

    // 1. KIỂM TRA THAM SỐ
    if ((argc < 3) || (argv[argc-2][0] == '-')) {
        fprintf(stderr, "Usage: %s [options] <source_dir> <mount_point>\n", argv[0]);
        return 1;
    }

    // 2. LẤY ĐƯỜNG DẪN SOURCE
    if (realpath(argv[argc-2], g_source_dir) == NULL) {
        perror("Error resolving source directory");
        return 1;
    }

    printf("[INFO] Source Directory: %s\n", g_source_dir);
    printf("[INFO] Mount Point: %s\n", argv[argc-1]);

    // --- ĐOẠN CODE MỚI THÊM VÀO ---
    // Xóa sạch thư mục lưu trữ tạm (.vfs_storage) để reset trạng thái về ban đầu
    // Lệnh này đảm bảo mỗi lần chạy là VFS sẽ ánh xạ đúng theo Source gốc
    printf("[INFO] Cleaning up previous session storage...\n");
    system("rm -rf .vfs_storage");
    // -----------------------------

    // 3. ĐIỀU CHỈNH ARGV CHO FUSE
    argv[argc-2] = argv[argc-1]; 
    argv[argc-1] = NULL;
    argc--; 

    // Log startup event
    log_event("START", "/", (pid_t)getpid(), (uid_t)getuid(), 0);

    return fuse_main(argc, argv, &vfs_operations, NULL);
}