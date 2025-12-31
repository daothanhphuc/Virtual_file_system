#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include "permissions.h"
#include <stdio.h>

// Virtual file metadata (should be moved to a struct in real code)
static uid_t virtual_file_uid = 0; // Owner UID
static gid_t virtual_file_gid = 0; // Owner GID

// Helper: get permission bits for user/group/other
static int get_permission_bits(int mode, int is_user, int is_group) {
    if (is_user) return (mode >> 6) & 0x7;
    if (is_group) return (mode >> 3) & 0x7;
    return mode & 0x7;
}

// Helper: check if the current process has the requested permission
// perm: 4=read, 2=write, 1=exec
int check_permission(int mode, uid_t file_uid, gid_t file_gid, int perm) {
    uid_t uid = getuid();
    gid_t gid = getgid();
    int bits = 0;
    if (uid == 0) return 1; // root can do anything
    if (uid == file_uid) bits = get_permission_bits(mode, 1, 0);
    else if (gid == file_gid) bits = get_permission_bits(mode, 0, 1);
    else bits = get_permission_bits(mode, 0, 0);
    return (bits & perm) ? 1 : 0;
}

// FUSE-style: check open flags against permissions
int check_permissions(int flags, int mode, uid_t file_uid, gid_t file_gid) {
    int r = 1;
    if ((flags & O_ACCMODE) == O_RDONLY)
        r = check_permission(mode, file_uid, file_gid, 4);
    else if ((flags & O_ACCMODE) == O_WRONLY)
        r = check_permission(mode, file_uid, file_gid, 2);
    else if ((flags & O_ACCMODE) == O_RDWR)
        r = check_permission(mode, file_uid, file_gid, 6);
    return r;
}

// chmod: change file mode
void update_virtual_file_permissions(int *virtual_file_permissions, int new_mode) {
    *virtual_file_permissions = new_mode & 0777;
}

// chown: change file owner/group
void update_virtual_file_owner(uid_t *file_uid, gid_t *file_gid, uid_t new_uid, gid_t new_gid) {
    if (file_uid) *file_uid = new_uid;
    if (file_gid) *file_gid = new_gid;
}

// Expose for use in other files
uid_t get_virtual_file_uid() { return virtual_file_uid; }
gid_t get_virtual_file_gid() { return virtual_file_gid; }
void set_virtual_file_uid(uid_t uid) { virtual_file_uid = uid; }
void set_virtual_file_gid(gid_t gid) { virtual_file_gid = gid; }