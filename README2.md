# Virtual File System (FUSE) — Enhanced

This repository contains a custom FUSE-based virtual file system written in C. It features enhanced logging, a "Recycle Bin" style backup mechanism, and a CLI tool for log querying.

## 🚀 Features

1.  **In-Memory Storage:** Files are stored in RAM (Linked List structure).
2.  **Auto-Backup (Versioning):**
    - Every time a file is modified (`WRITE`), the old content is saved to `.backup/`.
    - Every time a file is deleted (`UNLINK`), the content is saved to `.backup/`.
3.  **Detailed Logging:** Tracks User ID, Process ID, Timestamp, Operation, and Path.
4.  **CLI Query Tool:** A separate tool to filter and search logs.

---

## 🛠 Prerequisites

This project is built for **Linux** (or **WSL2** on Windows) using **FUSE 2.x**.

### Install Dependencies

Run the following commands to install GCC and FUSE development libraries:

```bash
sudo apt-get update
sudo apt-get install build-essential pkg-config libfuse-dev
```

⚙️ Compilation
We use specific flags (-D_FILE_OFFSET_BITS=64 and -DFUSE_USE_VERSION=26) to ensure compatibility with modern 64-bit systems while using the FUSE 2 API.

1. Build the File System (Server)

```bash
gcc -Wall -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 main.c operations.c permissions.c logging.c -o vfs $(pkg-config fuse --cflags --libs) -pthread
```

2. Build the Log Query Tool (CLI)

```bash
gcc -Wall -o cli_query cli_query.c
```

▶️ How to Run
Note: You will need two terminal windows.

Step 1: Prepare Mount Point (Terminal 1)

```bash
# Create the directory if it doesn't exist
mkdir -p /tmp/vfs_mount

# Run the VFS in foreground mode
./vfs -f /tmp/vfs_mount

```

⚠️ The terminal will hang/pause here. This is normal. The server is running.

Step 2: Interact with the File System (Terminal 2)
Open a new terminal tab and navigate to the project folder.

Create a file:

```bash
echo "Hello World" > /tmp/vfs_mount/virtual_file
```

Read the file:

```bash
cat /tmp/vfs_mount/virtual_file
```

Modify the file (Triggers Backup):

```bash
echo "New Content" > /tmp/vfs_mount/virtual_file
```

Delete the file (Triggers Recycle Bin):

```bash
rm /tmp/vfs_mount/virtual_file
```

♻️ Data Recovery (How to Restore)
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
cp .backup/virtual_file_20251231_181918.bak /tmp/vfs_mount/virtual_file
```

Scenario B: If the file exists but has wrong content Overwrite the current file with the backup:

```bash
cp .backup/virtual_file_20251231_181918.bak /tmp/vfs_mount/virtual_file
```

🔍 Using the CLI Log Tool
Instead of reading the raw text log, use the cli_query tool to filter events.

```bash
# View all WRITE operations
./cli_query --op WRITE

# View operations by specific user (e.g., root or your username)
./cli_query --user root

# View operations on a specific file
./cli_query --file virtual_file
```

🧹 Cleanup & Uninstall
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

📂 Project Structure
main.c: Entry point, initializes FUSE.

operations.c: Core logic (Create, Read, Write, Delete, Backup).

logging.c: Handles writing logs to virtual_fs.log.

cli_query.c: Source code for the log viewer tool.

.backup/: Hidden folder where backups are stored.
