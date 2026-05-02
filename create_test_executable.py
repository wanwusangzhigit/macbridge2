#!/usr/bin/env python3

# 创建一个简单的测试 Mach-O 文件
# 虽然这不是一个完整的可执行文件，但至少有正确的头部

import struct
import os

# 创建一个简化的 Mach-O 头部 (32-bit little endian)
# Mach-O 32-bit header: magic, cputype, cpusubtype, filetype, ncmds, sizeofcmds, flags
HEADER_FORMAT = "<IIIIIII"
MH_MAGIC = 0xFEEDFACE  # Mach-O 32-bit magic
MH_CPU_TYPE = 7        # CPU_TYPE_X86
MH_CPU_SUBTYPE = 3    # CPU_SUBTYPE_X86_ALL
MH_FILE_TYPE = 2      # MH_EXECUTE
MH_NCMDS = 1          # Number of commands
MH_SIZEOFCMDS = 56    # Size of commands
MH_FLAGS = 0x00800085  # Flags

# LC_SEGMENT 32-bit command
LC_SEGMENT = 0x00000001
LC_SEGMENT_LEN = 56

# 创建二进制数据
data = struct.pack(
    HEADER_FORMAT,
    MH_MAGIC,
    MH_CPU_TYPE,
    MH_CPU_SUBTYPE,
    MH_FILE_TYPE,
    MH_NCMDS,
    MH_SIZEOFCMDS,
    MH_FLAGS
)

# 添加一个简单的 LC_SEGMENT 命令
# 命令头部
data += struct.pack("<II", LC_SEGMENT, LC_SEGMENT_LEN)
# 段数据 (简化版)
data += b"__TEXT\x00\x00\x00"        # segname
data += struct.pack("<IIIIII", 
    0x1000,        # vmaddr
    0x1000,        # vmsize
    0,             # fileoff
    0,             # filesize
    7,             # maxprot (rwx)
    5              # initprot (rw)
)
data += struct.pack("<II", 0, 0)  # nsects, flags

# 填充到 4KB
while len(data) < 4096:
    data += b"\x00"

# 确保目录存在
exe_dir = "/workspace/TestApp.app/Contents/MacOS"
os.makedirs(exe_dir, exist_ok=True)

# 写入文件
exe_path = os.path.join(exe_dir, "TestApp")
with open(exe_path, "wb") as f:
    f.write(data)

print(f"Created test Mach-O executable: {exe_path}")
print(f"Size: {len(data)} bytes")

# 设置可执行权限 (在 Linux/macOS 上)
try:
    os.chmod(exe_path, 0o755)
    print("Set executable permissions")
except Exception as e:
    print(f"Warning: Could not set permissions: {e}")
