#!/bin/bash

# Test script for the virtual file system
MOUNT_POINT="/mnt/virtual_fs"
LOG_FILE="../virtual_fs.log"

# Ensure the mount point exists
if [ ! -d "$MOUNT_POINT" ]; then
    echo "Creating mount point at $MOUNT_POINT"
    mkdir -p $MOUNT_POINT
fi

# Mount the virtual file system
./virtual_fs $MOUNT_POINT &
VFS_PID=$!

# Wait for the file system to mount
sleep 2

echo "Running tests..."

# Test 1: Write to the virtual file
echo "Testing write operation" > $MOUNT_POINT/virtual_file
if [ $? -eq 0 ]; then
    echo "Write operation: SUCCESS"
else
    echo "Write operation: FAILED"
fi

# Test 2: Read from the virtual file
CONTENT=$(cat $MOUNT_POINT/virtual_file)
if [ "$CONTENT" == "Testing write operation" ]; then
    echo "Read operation: SUCCESS"
else
    echo "Read operation: FAILED"
fi

# Test 3: Check permissions
chmod 000 $MOUNT_POINT/virtual_file
cat $MOUNT_POINT/virtual_file 2>/dev/null
if [ $? -ne 0 ]; then
    echo "Permission check (read denied): SUCCESS"
else
    echo "Permission check (read denied): FAILED"
fi
chmod 644 $MOUNT_POINT/virtual_file

# Test 4: Logging
if [ -f "$LOG_FILE" ]; then
    echo "Log file exists: SUCCESS"
    echo "Log contents:" && cat $LOG_FILE
else
    echo "Log file exists: FAILED"
fi

# Unmount the file system
fusermount -u $MOUNT_POINT
if [ $? -eq 0 ]; then
    echo "Unmount operation: SUCCESS"
else
    echo "Unmount operation: FAILED"
fi

# Kill the virtual file system process
kill $VFS_PID

# End of tests
echo "Tests completed."