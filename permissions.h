#ifndef PERMISSIONS_H
#define PERMISSIONS_H

#include <sys/types.h>

int check_permission(int mode, uid_t file_uid, gid_t file_gid, int perm);

int check_permissions(int flags, int mode, uid_t file_uid, gid_t file_gid);

int check_chown_permission(uid_t file_uid, uid_t new_uid, gid_t new_gid);

void update_virtual_file_permissions(int *virtual_file_permissions, int new_mode);

void update_virtual_file_owner(uid_t *file_uid, gid_t *file_gid, uid_t new_uid, gid_t new_gid);

uid_t get_virtual_file_uid();
gid_t get_virtual_file_gid();
void set_virtual_file_uid(uid_t uid);
void set_virtual_file_gid(gid_t gid);

#endif