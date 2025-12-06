#include <fcntl.h>
#include "permissions.h"

int check_permissions(int flags, int file_permissions) {
    if ((flags & O_WRONLY) && !(file_permissions & 0222)) {
        return 0; // Write permission denied
    }
    if ((flags & O_RDONLY) && !(file_permissions & 0444)) {
        return 0; // Read permission denied
    }
    if ((flags & O_RDWR) && !(file_permissions & 0666)) {
        return 0; // Read/Write permission denied
    }
    return 1; // Permission granted
}