#!/bin/bash

# WinDarling 项目演示脚本

echo "========================================="
echo "  WinDarling - macOS 应用加载器"
echo "========================================="
echo ""

# 检查是否已构建
if [ ! -f "bin/app_loader" ]; then
    echo "项目未构建，正在构建..."
    ./build.sh
fi

echo ""
echo "1. 测试应用安装..."
echo "-----------------------------------------"
./bin/app_loader install TestApp.app

echo ""
echo "2. 列出已安装的应用..."
echo "-----------------------------------------"
./bin/app_loader list

echo ""
echo "3. 测试 Mach-O 文件解析..."
echo "-----------------------------------------"
./bin/app_loader test TestApp.app/Contents/MacOS/TestApp

echo ""
echo "========================================="
echo "  演示完成！"
echo ""
echo "  可用命令:"
echo "    ./bin/app_loader help"
echo "    ./bin/app_loader install <app>"
echo "    ./bin/app_loader list"
echo "    ./bin/app_loader test <mach-o>"
echo "    ./bin/app_loader uninstall <id>"
echo "========================================="
