# Virtual File System (FUSE) — Enhanced

### Prerequisites

This project is built for **Linux** (or **WSL2** on Windows) using **FUSE 2.x**.

### Install Dependencies

Run the following commands to install GCC and FUSE development libraries:

```bash
sudo apt-get update
sudo apt-get install build-essential pkg-config libfuse-dev
```

### Compilation
We use specific flags (-D_FILE_OFFSET_BITS=64 and -DFUSE_USE_VERSION=26) to ensure compatibility with modern 64-bit systems while using the FUSE 2 API.
1. Build the File System (Server)

```bash
gcc -Wall -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 main.c operations.c permissions.c logging.c -o vfs $(pkg-config fuse --cflags --libs) -pthread
```

2. Build the Log Query Tool (CLI)

```bash
gcc -Wall -o cli_query cli_query.c
```

### How to Run
Note: You will need two terminal windows.

Create a sample source folder

```bash
mkdir -p ~/my_source_data

#write sample data into folder
echo "Day la file that 1" > ~/my_source_data/test.txt
echo "Day la file that version 2" > ~/my_source_data/test_2.txt

#visualize to check
ls
cat ~/my_source_data/test.txt
```

Step 1: Prepare Mount Point (Terminal 1)

```bash
# Create the directory if it doesn't exist
mkdir -p /tmp/vfs_mount

# Run the VFS in foreground mode
./vfs -f ~/my_source_data /tmp/vfs_mount

```

The terminal will hang/pause here. This is normal. The server is running.

Step 2: Interact with the File System (Terminal 2)
Open a new terminal tab and navigate to the project folder.

Create a file:

```bash
echo "Hello World" > /tmp/vfs_mount/test.txt
```

Read the file:

```bash
cat /tmp/vfs_mount/test.txt
```

Modify the file (Triggers Backup):

```bash
echo "New Content" > /tmp/vfs_mount/test.txt
```

Delete the file (Triggers Recycle Bin):

```bash
rm /tmp/vfs_mount/test.txt
```

### Data Recovery (How to Restore)
When a file is modified or deleted, a backup is automatically saved in the hidden .backup folder.

1. List available backups
   Check the timestamped backup files:

```bash
ls -la .backup
# Example output:
# virtual_file_20251231_181832.bak
# virtual_file_20251231_181918.bak
```

2. View backup content

```bash
cat .backup/virtual_file_20251231_181918.bak
```

3. Restore a file
   To restore, simply copy the backup file back to the mount point.

Scenario A: If the file was deleted 

```bash
cp .backup/virtual_file_20251231_181918.bak /tmp/vfs_mount/test.txt
```

Scenario B: If the file exists but has wrong content Overwrite the current file with the backup:

```bash
cp .backup/virtual_file_20251231_181918.bak /tmp/vfs_mount/test.txt
```

### Check Permissions (chmod)
CD to the /tmp/vfs_mount/
1. Change the file permissions to none (no read/write/execute):
   ```bash
   chmod 000 test.txt
   ```
   Now, reading or writing should fail:
   ```bash
   cat test.txt           # Permission denied
   echo "test" > test.txt # Permission denied
   ```
2. Set the file to read-only:
   ```bash
   chmod 444 test.txt
   cat test.txt           # Should succeed
   echo "test" > test.txt # Permission denied
   ```
3. Set the file to write-only:
   ```bash
   chmod 222 test.txt
   cat test.txt           # Permission denied
   echo "test" > test.txt # Should succeed
   ```
4. Restore to read/write:
   ```bash
   chmod 644 test.txt
   ```
   Both read and write should work.
--> To check permission, run the command: ls -n /tmp/vfs_mount/test.txt 

### Using the CLI Log Tool
Instead of reading the raw text log, use the cli_query tool to filter events.

```bash
# View all WRITE operations
./cli_query --op WRITE

# View operations on a specific file
./cli_query --file test.txt
```

### On development: 

#### Change File Owner/Group (chown)

1. Change the file owner (using user's role):
   ```bash
   chown 1001 test.txt
   ```
   Result should be "chown: changing ownership of 'test_chown.txt': Operation not permitted"
2. Change the group:
   ```bash
   chown :1000 test_chown.txt
   ```
   "Success"
3. Change the file owner (requires root):
   ```bash
   sudo chown 1005 /tmp/vfs_mount/test.txt
   sudo useradd -u 1005 -M -s /bin/bash tester1005 # create dummy user
   ls -n /tmp/vfs_mount/test.txt  # check permission
   sudo chmod 400 /tmp/vfs_mount/test.txt # add a role to test
   sudo -u tester1005 cat /tmp/vfs_mount/test.txt
   ```
   Command "cat /tmp/vfs_mount/test.txt" should print "Permission denied"
4. Test Copy-On-Write Function:
   ```bash
   ls -l ~/my_project/.vfs_storage/test.txt
   ```
#### Move/Rename a File (mv)
```bash
mv /tmp/vfs_mount/test.txt /tmp/vfs_mount/test_renamed.txt
ls /tmp/vfs_mount
cat /tmp/vfs_mount/test_renamed.txt
```

#### Copy a File (cp)
```bash
cp /tmp/vfs_mount/test_renamed.txt /tmp/vfs_mount/test_copy.txt
ls /tmp/vfs_mount
cat /tmp/vfs_mount/test_copy.txt
```

#### Create/Update a File (touch)
```bash
# Create a new empty file
touch /tmp/vfs_mount/newfile.txt
ls -l /tmp/vfs_mount/newfile.txt

# Update the timestamp of an existing file
touch /tmp/vfs_mount/test_copy.txt
ls -l /tmp/vfs_mount/test_copy.txt
```

### Cleanup & Uninstall
When you are done, you must unmount the file system properly.

1. Stop the Server: Press Ctrl + C in Terminal 1.

2.Unmount (if it's stuck): If the folder is locked or you get "Transport endpoint is not connected", run:

```bash
fusermount -u -z /tmp/vfs_mount
```

3.Clean up generated files (Optional):

```bash
rm vfs cli_query *.o
rm -rf .backup virtual_fs.log
rmdir /tmp/vfs_mount
```
### Project's progress:

#### Completed Features
- Core FUSE-based virtual file system implemented with read, write, open, and directory operations.
- Copy-on-write and backup/versioning mechanism (.backup folder) for file modifications and deletions.
- Permission and ownership management (chmod, chown) with enforcement and logging.
- Logging system with detailed metadata (timestamp, uid, username, pid, operation, path, result).
- CLI log query tool (`cli_query`) for filtering and viewing log events by user, file, or operation.
- Data recovery workflow for restoring files from backups.
- Automated test script for basic file system operations and permission checks.

#### In Progress / To Do

- Implement support for additional essential file system commands:
   - `truncate` (shrink or extend file size)
   - `stat` (display file or filesystem status)
   - `find` (search for files)
   - `df` (report file system disk space usage)
   - `du` (estimate file space usage)
   - `ln` (create hard and symbolic links)
   - `sync` (flush file system buffers)
- Further testing for edge cases (e.g., concurrent access, large files, unusual permission scenarios).
- Additional CLI log query features (e.g., export, advanced filters).
- Documentation enhancements and usage examples.
- Optional: Cross-platform support improvements (native Windows, MacOS).

#### Status
- The core functionality is complete and stable for typical use cases on Linux/WSL2.
- The project is ready for further testing, documentation, and optional feature expansion.

### Member roles:

#### Khai

#### Quan

#### Phuc

#### Manh