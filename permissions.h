#ifndef PERMISSIONS_H
#define PERMISSIONS_H

#include <sys/types.h>

// Check if the current process has the requested permission (4=read, 2=write, 1=exec)
int check_permission(int mode, uid_t file_uid, gid_t file_gid, int perm);

// Check open flags against permissions
int check_permissions(int flags, int mode, uid_t file_uid, gid_t file_gid);

// chmod: update file mode
void update_virtual_file_permissions(int *virtual_file_permissions, int new_mode);

// chown: update file owner/group
void update_virtual_file_owner(uid_t *file_uid, gid_t *file_gid, uid_t new_uid, gid_t new_gid);

// Get/set virtual file owner/group (for global/static vars)
uid_t get_virtual_file_uid();
gid_t get_virtual_file_gid();
void set_virtual_file_uid(uid_t uid);
void set_virtual_file_gid(gid_t gid);

#endif