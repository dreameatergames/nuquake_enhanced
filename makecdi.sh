#!/bin/bash

# Set variables
NAME="nuquake"
BUILD_DIR="build_dc"

# Check if build_dc directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: '$BUILD_DIR' directory not found. Make sure you've built the project."
    exit 1
fi

# Check for required files and directories
if [ ! -d "cd" ]; then
    echo "Error: 'cd' directory not found in the current directory"
    exit 1
fi

if [ ! -f "IP.BIN" ]; then
    echo "Error: IP.BIN not found in the current directory"
    exit 1
fi

if [ ! -f "$BUILD_DIR/1ST_READ.BIN" ]; then
    echo "Error: 1ST_READ.BIN not found in $BUILD_DIR"
    exit 1
fi

# Create a temporary directory for ISO creation
TEMP_DIR=$(mktemp -d)

# Copy necessary files to temp directory
cp -r cd/* "$TEMP_DIR/"
cp IP.BIN "$TEMP_DIR/"
cp "$BUILD_DIR/1ST_READ.BIN" "$TEMP_DIR/"

# Change to temp directory
cd "$TEMP_DIR"

# Create ISO
mkisofs -C 0,11702 -V "${NAME^^}" -G IP.BIN -joliet -rock -l -o "$NAME.iso" .

# Create CDI
cdi4dc "$NAME.iso" "$NAME.cdi" -d

# Move CDI to original directory
mv "$NAME.cdi" "$OLDPWD/"

echo "CDI created successfully: $OLDPWD/$NAME.cdi"

# Clean up
cd "$OLDPWD"
rm -rf "$TEMP_DIR"