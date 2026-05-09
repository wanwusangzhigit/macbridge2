#!/bin/bash

SRC_DIR="src"
BIN_DIR="bin"

if [ ! -d "$BIN_DIR" ]; then
    mkdir -p "$BIN_DIR"
fi

echo "Building test_loader..."
gcc -Wall -O2 -I"$SRC_DIR" "$SRC_DIR/macho.c" "$SRC_DIR/syscall.c" "$SRC_DIR/vfs.c" "$SRC_DIR/dyld.c" "$SRC_DIR/platform.c" "$SRC_DIR/util.c" "$SRC_DIR/test_loader.c" -o "$BIN_DIR/test_loader" -lpthread

if [ $? -eq 0 ]; then
    echo "Test loader build successful!"
else
    echo "Test loader build failed!"
fi

echo
echo "Building app_loader..."
gcc -Wall -O2 -I"$SRC_DIR" "$SRC_DIR/macho.c" "$SRC_DIR/syscall.c" "$SRC_DIR/vfs.c" "$SRC_DIR/dyld.c" "$SRC_DIR/platform.c" "$SRC_DIR/app_bundle.c" "$SRC_DIR/dmg.c" "$SRC_DIR/util.c" "$SRC_DIR/app_loader.c" -o "$BIN_DIR/app_loader" -lpthread -lz -llzma

if [ $? -eq 0 ]; then
    echo
    echo "Build completed successfully!"
    echo "Executables created at:"
    echo "  - $BIN_DIR/test_loader"
    echo "  - $BIN_DIR/app_loader"
else
    echo "App loader build failed!"
fi
