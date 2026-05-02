#!/usr/bin/env python3
"""
创建测试 Mach-O 可执行文件 (简化版)
这个脚本生成一个简单但结构正确的 Mach-O 可执行文件
"""

import struct
import os

def create_macho_header():
    """创建 Mach-O 头部 (32-bit little endian)"""
    # mach_header 结构
    MH_MAGIC = 0xFEEDFACE
    MH_CPU_TYPE = 7      # CPU_TYPE_X86
    MH_CPU_SUBTYPE = 3   # CPU_SUBTYPE_X86_ALL
    MH_FILE_TYPE = 2     # MH_EXECUTE
    MH_NCMDS = 1         # 1 个加载命令
    MH_SIZEOFCMDS = 56   # 命令总大小
    MH_FLAGS = 0x00800085
    
    # 7 个字段
    return struct.pack("<IIIIIII",
        MH_MAGIC,
        MH_CPU_TYPE,
        MH_CPU_SUBTYPE,
        MH_FILE_TYPE,
        MH_NCMDS,
        MH_SIZEOFCMDS,
        MH_FLAGS)

def create_lc_segment():
    """创建 LC_SEGMENT 加载命令"""
    LC_SEGMENT = 0x00000001
    cmdsize = 56
    
    # segname
    segname = b"__TEXT\x00\x00\x00"
    
    # vmaddr, vmsize, fileoff, filesize, maxprot, initprot
    header = struct.pack("<II", LC_SEGMENT, cmdsize)
    data = segname + struct.pack("<IIIIII",
        0x1000, 0x1000, 0, 0, 7, 5)  # rwx, rw
    
    # nsects, flags (unused)
    data += struct.pack("<II", 0, 0)
    
    return header + data

def create_macho_file():
    """创建完整的 Mach-O 可执行文件"""
    # 头部
    header = create_macho_header()
    
    # 加载命令
    load_cmds = create_lc_segment()
    
    # 填充到 4KB
    padding_size = 4096 - len(header) - len(load_cmds)
    padding = b"\x00" * padding_size
    
    # 组合所有内容
    macho_data = header + load_cmds + padding
    
    return macho_data

def main():
    print("=" * 50)
    print("WinDarling - Mach-O 测试文件生成器")
    print("=" * 50)
    
    # 输出目录
    output_dir = "TestApp.app/Contents/MacOS"
    os.makedirs(output_dir, exist_ok=True)
    
    # 输出文件
    output_file = os.path.join(output_dir, "TestApp")
    
    # 生成 Mach-O
    macho_data = create_macho_file()
    
    # 写入文件
    with open(output_file, "wb") as f:
        f.write(macho_data)
    
    # 设置可执行权限
    try:
        os.chmod(output_file, 0o755)
        print("✅ 已设置可执行权限")
    except Exception as e:
        print(f"⚠️  设置权限时出错: {e}")
    
    # 报告
    print(f"\n✅ 已创建 Mach-O 文件: {output_file}")
    print(f"   文件大小: {len(macho_data)} 字节")
    
    # 验证 Info.plist 存在
    plist_file = "TestApp.app/Contents/Info.plist"
    if os.path.exists(plist_file):
        print(f"✅ Info.plist 已存在: {plist_file}")
    else:
        print(f"⚠️  Info.plist 不存在: {plist_file}")
    
    print("\n" + "=" * 50)
    print("测试应用已准备就绪!")
    print("=" * 50)
    print("\n使用方法:")
    print("  ./bin/app_loader install TestApp.app")
    print("  ./bin/app_loader list")
    print("  ./bin/app_loader test TestApp.app/Contents/MacOS/TestApp")

if __name__ == "__main__":
    main()
