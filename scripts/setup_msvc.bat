@echo off
REM Locate Visual Studio and set up the MSVC x64 environment.
REM This script is called by the other build scripts — not used directly.

if defined VCINSTALLDIR (
    exit /b 0
)

for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath 2^>nul`) do (
    set "VS_PATH=%%i"
)

if not defined VS_PATH (
    echo ERROR: Visual Studio installation not found. Install Visual Studio or Build Tools.
    exit /b 1
)

call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo ERROR: Failed to initialize MSVC x64 environment.
    exit /b 1
)
