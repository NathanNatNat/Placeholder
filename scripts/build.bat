@echo off
REM Build the project (Debug by default).
REM Usage: build.bat [debug|release|relwithdebinfo]

setlocal
cd /d "%~dp0.."
call scripts\setup_msvc.bat || exit /b 1

set "PRESET=debug"
if /i "%~1"=="release" set "PRESET=release"
if /i "%~1"=="relwithdebinfo" set "PRESET=relwithdebinfo"

echo Building with preset: %PRESET%
cmake --build --preset=%PRESET%
