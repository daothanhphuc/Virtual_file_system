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

### Check Permissions --> cái này chưa fix code nên chưa được nhé
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

