@echo off

set "SRC_DIR=src"
set "BIN_DIR=bin"

if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"

cl /nologo /W3 /O2 /I"%SRC_DIR%" "%SRC_DIR%\macho.c" "%SRC_DIR%\syscall.c" "%SRC_DIR%\vfs.c" "%SRC_DIR%\dyld.c" "%SRC_DIR%\test_loader.c" /Fe"%BIN_DIR%\test_loader.exe"

if %ERRORLEVEL% equ 0 (
    echo Build successful!
    echo Test loader executable created at: %BIN_DIR%\test_loader.exe
) else (
    echo Build failed!
)
