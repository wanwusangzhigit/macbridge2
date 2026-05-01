@echo off

set "SRC_DIR=src"
set "BIN_DIR=bin"

if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"

for /f "usebackq tokens=*" %%i in (`vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set "VS_PATH=%%i"
)

if exist "%VS_PATH%\Common7\Tools\VsDevCmd.bat" (
  call "%VS_PATH%\Common7\Tools\VsDevCmd.bat"
)

cl /nologo /w /O2 /I"%SRC_DIR%" "%SRC_DIR%\macho.c" "%SRC_DIR%\syscall.c" "%SRC_DIR%\vfs.c" "%SRC_DIR%\dyld.c" "%SRC_DIR%\platform.c" "%SRC_DIR%\test_loader.c" /Fe"%BIN_DIR%\test_loader.exe"

if %ERRORLEVEL% equ 0 (
    echo Build successful!
    echo Test loader executable created at: %BIN_DIR%\test_loader.exe
) else (
    echo Build failed!
)