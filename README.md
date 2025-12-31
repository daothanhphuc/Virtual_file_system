# Virtual File System (FUSE) — Enhanced

This repository contains a minimal FUSE-based virtual file system with enhanced logging, a recycle-bin/versioning backup mechanism, and a CLI log-query tool.

**Prerequisites**

- **OS:** Linux is recommended (FUSE support). On Windows use WSL/WSL2.
- **Libraries:** Install libfuse development headers. On Debian/Ubuntu:

```bash
sudo apt update
sudo apt install build-essential pkg-config libfuse3-dev
```

If your system has FUSE 2, install `libfuse-dev` and adjust the compile flags to `-lfuse`.

**Files of interest**

- `main.c`: FUSE entrypoint that initializes logging and mounts the FS.
- `operations.c`: FUSE operation implementations (read, write, unlink, etc.) with backup behavior.
- `logging.c` / `logging.h`: Metadata-rich logging (timestamp, uid/username, pid, path, operation, result).
- `permissions.c` / `permissions.h`: Simple permission helpers.
- `cli_query.c`: Command-line tool to query and filter the log file.
- `.backup/`: Created at runtime to store timestamped backups (not tracked in repo).

## Compilation

Build the main virtual filesystem binary (example using pkg-config for FUSE3):

```bash
gcc -Wall $(pkg-config --cflags --libs fuse3) -o vfs main.c operations.c permissions.c logging.c -pthread
```

If `pkg-config` or `fuse3` is not available, try linking directly (FUSE2):

```bash
gcc -Wall -o vfs main.c operations.c permissions.c logging.c -lfuse -pthread
```

Build the CLI query tool:

```bash
gcc -Wall -o cli_query cli_query.c
```

## How to Run

1. Create a mount point:

```bash
mkdir -p /tmp/vfs_mount
```

2. Run the virtual file system in the foreground (recommended while testing):

```bash
./vfs -f /tmp/vfs_mount
```

This runs the VFS in the foreground (-f) and mounts it on `/tmp/vfs_mount`.

Notes:

- Logs are written to `virtual_fs.log` in the directory where you run `./vfs`.
- Backups are written to a hidden directory named `.backup` in the same working directory.

## Testing / Examples

Open another terminal to interact with the mounted filesystem.

- Create or write the virtual file (the FS exposes a single `virtual_file`):

```bash
echo "hello world" > /tmp/vfs_mount/virtual_file
cat /tmp/vfs_mount/virtual_file
```

- Overwrite to trigger a backup (each write operation saves a backup of the current content):

```bash
echo "new content" > /tmp/vfs_mount/virtual_file
```

- Remove the file to trigger recycle-bin behavior (content is saved to `.backup` and in-memory file cleared):

```bash
rm /tmp/vfs_mount/virtual_file
```

- Inspect backups:

```bash
ls -la .backup
```

## Using the CLI query tool

The logging format is pipe-separated: `timestamp|uid|username|pid|operation|path|result`.

Examples:

```bash
# Show all log lines referencing the virtual file
./cli_query --file virtual_file

# Filter by user (username or UID)
./cli_query --user root

# Filter by operation type (e.g., WRITE, READ, UNLINK)
./cli_query --op WRITE
```

## Cleanup / Unmount

To unmount the filesystem:

```bash
fusermount3 -u /tmp/vfs_mount   # on systems with fusermount3
# or
fusermount -u /tmp/vfs_mount    # older systems
# or
umount /tmp/vfs_mount
```

Then remove mountpoint if desired:

```bash
rmdir /tmp/vfs_mount
```

## Additional Notes

- Backups are timestamped like `virtual_file_YYYYMMDD_HHMMSS.bak` and stored under `.backup`.
- Logs are append-only; starting `vfs` will add a `START` event but will not automatically purge the log.

If you'd like, I can run quick compile commands and run a smoke test in this workspace (if you have a Linux/WSL environment). Would you like me to do that now?
