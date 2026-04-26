#!/bin/bash

SRC_DIR="src"
BIN_DIR="bin"

if [ ! -d "$BIN_DIR" ]; then
    mkdir -p "$BIN_DIR"
fi

gcc -Wall -O2 -I"$SRC_DIR" "$SRC_DIR/macho.c" "$SRC_DIR/syscall.c" "$SRC_DIR/vfs.c" "$SRC_DIR/dyld.c" "$SRC_DIR/platform.c" "$SRC_DIR/test_loader.c" -o "$BIN_DIR/test_loader" -lpthread

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Test loader executable created at: $BIN_DIR/test_loader"
else
    echo "Build failed!"
fi
