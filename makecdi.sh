#!/bin/bash

# Variables
NAME="nuquake"
BUILD_DIR="build_dc"
CD_DIR="cd"

# Ensure build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Build directory '$BUILD_DIR' not found!"
    exit 1
fi

# Ensure cd directory exists
if [ ! -d "$CD_DIR" ]; then
    echo "Error: CD directory '$CD_DIR' not found!"
    exit 1
fi

# Check for nuquake ELF
if [ ! -f "$BUILD_DIR/nuquake" ]; then
    echo "Error: 'nuquake' binary not found in $BUILD_DIR!"
    exit 1
fi

# Scramble the ELF binary to 1ST_READ.BIN
echo "Scrambling 'nuquake' into '1ST_READ.BIN'..."
scramble "$BUILD_DIR/nuquake" "$BUILD_DIR/1ST_READ.BIN"
if [ $? -ne 0 ]; then
    echo "Error: Failed to scramble 'nuquake'!"
    exit 1
fi

# Copy scrambled binary to the CD directory
echo "Copying '1ST_READ.BIN' to CD directory..."
cp "$BUILD_DIR/1ST_READ.BIN" "$CD_DIR/1ST_READ.BIN"
if [ $? -ne 0 ]; then
    echo "Error: Failed to copy '1ST_READ.BIN' to $CD_DIR!"
    exit 1
fi

# Generate CDI using mkdcdisc with the original ELF binary
echo "Generating CDI..."
mkdcdisc \
    -n "DCNuquake" \
    -d "$CD_DIR/id1" \
    -f "$CD_DIR/IP.BIN" \
    -e "$BUILD_DIR/nuquake" \
    -o "$NAME.cdi" \
    -N

# Check if CDI was successfully created
if [ -f "$NAME.cdi" ]; then
    echo "CDI successfully created: $(pwd)/$NAME.cdi"
else
    echo "Error: Failed to create CDI!"
    exit 1
fi

# Check if mksdiso exists before trying to create ISO
if command -v mksdiso &> /dev/null; then
    echo "Generating SD-compatible ISO..."
    mksdiso -h "$NAME.cdi" "$NAME.iso"
    if [ -f "$NAME.iso" ]; then
        echo "ISO successfully created: $(pwd)/$NAME.iso"
    else
        echo "Warning: Failed to create ISO"
    fi
else
    echo "Warning: mksdiso not found - skipping ISO creation"
fi