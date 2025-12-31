### Check File Owner and Group
To see the current owner and group of the virtual file, use:
```bash
ls -l virtual_file
```
or for more details:
```bash
stat virtual_file
```
For verifying the effect of chown and permission changes.
# Virtual File System Guide

This guide explains how to build, run, and test the virtual file system implemented using the FUSE library.

## Prerequisites

1. **Install GCC**: Ensure GCC is installed on your system.
2. **Install FUSE**: Install the FUSE library. On Linux, you can use:
   ```bash
   sudo apt-get install libfuse-dev
   ```
3. **Create a Mount Point**: Create a directory to mount the virtual file system, e.g., `/mnt/virtual_fs`.

## Build Instructions

1. Navigate to the project directory:
   ```bash
   cd /path/to/Virtual_File_simulation
   ```
2. Compile the code:
   ```bash
   gcc -o virtual_fs main.c operations.c permissions.c logging.c -lfuse -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26
   ```

## Run the Virtual File System

1. Create a directory:
   ```bash
   mkdir -p ~/my_fs
   ```
2. Run the compiled program with a mount point:
   ```bash
   ./virtual_fs ~/my_fs
   ```
3. The virtual file system will now be mounted at `~/my_fs`.

## Test the Virtual File System

### Create and Write to the Virtual File
1. Navigate to the mount point:
   ```bash
   cd ~/my_fs
   ```
2. Write data to the virtual file:
   ```bash
   echo "Hello, Virtual File System!" > virtual_file
   ```

### Read from the Virtual File
1. Read the contents of the virtual file:
   ```bash
   cat virtual_file
   ```


### Check Permissions (chmod)
1. Change the file permissions to none (no read/write/execute):
   ```bash
   chmod 000 virtual_file
   ```
   Now, reading or writing should fail:
   ```bash
   cat virtual_file           # Permission denied
   echo "test" > virtual_file # Permission denied
   ```
2. Set the file to read-only:
   ```bash
   chmod 444 virtual_file
   cat virtual_file           # Should succeed
   echo "test" > virtual_file # Permission denied
   ```
3. Set the file to write-only:
   ```bash
   chmod 222 virtual_file
   cat virtual_file           # Permission denied
   echo "test" > virtual_file # Should succeed
   ```
4. Restore to read/write:
   ```bash
   chmod 644 virtual_file
   ```
   Both read and write should work.

### Change File Owner/Group (chown)
1. Change the file owner (requires root):
   ```bash
   sudo chown <newuser>:<newgroup> virtual_file
   ```
2. Test access as the new user:
   ```bash
   sudo -u <newuser> cat virtual_file
   sudo -u <newuser> echo "test" > virtual_file
   ```
   Access will depend on the new permissions and ownership.

### Unmount the File System
1. Unmount the virtual file system:
   ```bash
   fusermount -u ~/my_fs
   ```
2. For checking if unmount is successful or not --> this command should return nothing:
   ```bash
   mount | grep my_fs
   ```

## Logs

All operations are logged in `virtual_fs.log` in the project directory. Check this file for debugging and tracking operations.

