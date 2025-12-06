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
   gcc -o virtual_fs virtual_fs.c -lfuse
   ```

## Run the Virtual File System

1. Run the compiled program with a mount point:
   ```bash
   ./virtual_fs /mnt/virtual_fs
   ```
2. The virtual file system will now be mounted at `/mnt/virtual_fs`.

## Test the Virtual File System

### Create and Write to the Virtual File
1. Navigate to the mount point:
   ```bash
   cd /mnt/virtual_fs
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

### Check Permissions
1. Change the file permissions:
   ```bash
   chmod 000 virtual_file
   ```
2. Try reading or writing to the file to verify permission handling.

### Unmount the File System
1. Unmount the virtual file system:
   ```bash
   fusermount -u /mnt/virtual_fs
   ```

## Logs

All operations are logged in `virtual_fs.log` in the project directory. Check this file for debugging and tracking operations.

---

For further assistance, feel free to reach out!