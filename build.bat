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

echo Building test_loader...
cl /nologo /w /O2 /I"%SRC_DIR%" "%SRC_DIR%\macho.c" "%SRC_DIR%\syscall.c" "%SRC_DIR%\vfs.c" "%SRC_DIR%\dyld.c" "%SRC_DIR%\platform.c" "%SRC_DIR%\util.c" "%SRC_DIR%\test_loader.c" /Fe"%BIN_DIR%\test_loader.exe"

if %ERRORLEVEL% equ 0 (
    echo Test loader build successful!
) else (
    echo Test loader build failed!
)

echo.
echo Building app_loader (Console)...
cl /nologo /w /O2 /I"%SRC_DIR%" "%SRC_DIR%\macho.c" "%SRC_DIR%\syscall.c" "%SRC_DIR%\vfs.c" "%SRC_DIR%\dyld.c" "%SRC_DIR%\platform.c" "%SRC_DIR%\app_bundle.c" "%SRC_DIR%\dmg.c" "%SRC_DIR%\hfs.c" "%SRC_DIR%\util.c" "%SRC_DIR%\ui.c" "%SRC_DIR%\app_loader.c" /Fe"%BIN_DIR%\app_loader.exe"
if %ERRORLEVEL% equ 0 (
    echo Console app_loader build successful!
) else (
    echo Console app_loader build failed!
)

echo.
echo Building WinDarling GUI (Win32)...
rc /nologo "%SRC_DIR%\win32_gui.rc"
cl /nologo /w /O2 /source-charset:utf-8 /execution-charset:utf-8 /I"%SRC_DIR%" "%SRC_DIR%\win32_gui.c" "%SRC_DIR%\win32_gui.res" "%SRC_DIR%\macho.c" "%SRC_DIR%\syscall.c" "%SRC_DIR%\vfs.c" "%SRC_DIR%\dyld.c" "%SRC_DIR%\platform.c" "%SRC_DIR%\app_bundle.c" "%SRC_DIR%\dmg.c" "%SRC_DIR%\hfs.c" "%SRC_DIR%\util.c" /Fe"%BIN_DIR%\windarling.exe" /link comctl32.lib shell32.lib user32.lib gdi32.lib comdlg32.lib
if %ERRORLEVEL% equ 0 (
    echo.
    echo Build completed successfully!
    echo Executables created at:
    echo   - %BIN_DIR%\test_loader.exe
    echo   - %BIN_DIR%\app_loader.exe
    echo   - %BIN_DIR%\windarling.exe
) else (
    echo GUI build failed!
    echo Executables created at:
    echo   - %BIN_DIR%\test_loader.exe
    echo   - %BIN_DIR%\app_loader.exe
)