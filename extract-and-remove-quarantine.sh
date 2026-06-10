#!/bin/bash
# Script to extract split archive files and remove macOS quarantine attributes
# This helps avoid security warnings when running extracted binaries on macOS

# Extract all split archive parts and combine them using tar
cat llvm-mingw-xp-22.1.0-msvcrt-macos-universal.* | tar -xvJ

# Path to the extracted directory
TARGET_DIR="llvm-mingw-xp-22.1.0-msvcrt-pentium3-macos-universal"

# Verify the target directory exists after extraction
if [ ! -d "$TARGET_DIR" ]; then
    echo "Error: Directory $TARGET_DIR does not exist."
    exit 1
fi

# Function to remove quarantine attribute from a file
# macOS adds quarantine attributes to downloaded files which can cause security warnings
apply_xattr() {
    local file="$1"
    echo "Processing $file..."
    # Remove com.apple.quarantine extended attribute if it exists
    sudo xattr -d com.apple.quarantine "$file" 2>/dev/null

    # Check if the operation was successful
    if [ $? -eq 0 ]; then
        echo "  -> Quarantine attribute removed from $file"
    else
        echo "  -> No quarantine attribute found or error processing $file"
    fi
}

# Find all executable files in the target directory and remove quarantine attributes
# -type f: only files
# -perm +111: files with execute permission
# -print0: handle filenames with spaces correctly
echo "Scanning for executable files in $TARGET_DIR..."
find "$TARGET_DIR" -type f -perm +111 -print0 | while IFS= read -r -d '' file; do
    apply_xattr "$file"
done

echo "Processing complete. All executable files should now be free of quarantine attributes."
