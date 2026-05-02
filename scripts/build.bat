@echo off
REM Build the project (Debug by default).
REM Automatically configures if the build directory doesn't exist.
REM Usage: build.bat [debug|release|relwithdebinfo]

setlocal
cd /d "%~dp0.."
call scripts\setup_msvc.bat || exit /b 1

set "PRESET=debug"
set "CONFIG_PRESET=windows-msvc-debug"
if /i "%~1"=="release" (
    set "PRESET=release"
    set "CONFIG_PRESET=windows-msvc-release"
)
if /i "%~1"=="relwithdebinfo" (
    set "PRESET=relwithdebinfo"
    set "CONFIG_PRESET=windows-msvc-relwithdebinfo"
)

if not exist "out\build\build.ninja" (
    echo Build directory not found, configuring first...
    cmake --preset=%CONFIG_PRESET% || exit /b 1
)

echo Building with preset: %PRESET%
cmake --build --preset=%PRESET%
