@echo off
REM ============================================================
REM Blazingtron 2 - Build script (Windows 11)
REM Requires: NASM + MinGW-w64 (gcc) in PATH
REM   - Download NASM: https://nasm.us
REM   - Easiest toolchain: MSYS2 (pacman -S mingw-w64-x86_64-gcc nasm)
REM     or TDM-GCC + standalone NASM
REM ============================================================

setlocal enabledelayedexpansion

set OUT=Blazingtron2.exe
set SRC=src\main.c src\calc.asm
set RES=res\resource.rc

echo [Blazingtron 2] Building native Win32 + x64 Assembly...

where nasm >nul 2>&1
if errorlevel 1 (
    echo ERROR: nasm not found in PATH
    echo Please install NASM and add to PATH
    pause
    exit /b 1
)

where gcc >nul 2>&1
if errorlevel 1 (
    echo ERROR: gcc (MinGW) not found in PATH
    echo Install MSYS2 or TDM-GCC and add bin folder to PATH
    pause
    exit /b 1
)

echo [1/4] Assembling calc.asm (NASM win64)...
nasm -f win64 src\calc.asm -o calc.obj
if errorlevel 1 goto :fail

echo [2/4] Compiling resources (windres)...
windres -i res\resource.rc -o resource.o 2>nul
if errorlevel 1 (
    echo   (resource.o not critical - continuing without custom icon/manifest)
    set RESOBJ=
) else (
    set RESOBJ=resource.o
)

echo [3/4] Linking with gcc (GUI subsystem)...
gcc -mwindows -municode -Wall -O2 -s ^
    -o %OUT% ^
    src\main.c calc.obj %RESOBJ% ^
    -luser32 -lgdi32 -lcomctl32 -lkernel32

if errorlevel 1 goto :fail

echo [4/4] Cleaning up...
del /q calc.obj resource.o 2>nul

echo.
echo SUCCESS: %OUT% built.
echo Run it: .\%OUT%
echo.
dir %OUT%
goto :eof

:fail
echo.
echo BUILD FAILED.
echo Make sure you are using 64-bit MinGW and NASM supports win64.
pause
exit /b 1
