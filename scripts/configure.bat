@echo off
REM Configure the CMake project (Debug by default).
REM Usage: configure.bat [debug|release|relwithdebinfo]

setlocal
cd /d "%~dp0.."
call scripts\setup_msvc.bat || exit /b 1

set "PRESET=windows-msvc-debug"
if /i "%~1"=="release" set "PRESET=windows-msvc-release"
if /i "%~1"=="relwithdebinfo" set "PRESET=windows-msvc-relwithdebinfo"

echo Configuring with preset: %PRESET%
cmake --preset=%PRESET%
