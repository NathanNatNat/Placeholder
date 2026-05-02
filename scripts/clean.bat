@echo off
REM Remove the build output directory.

setlocal
set "BUILD_DIR=%~dp0..\out\build"

if exist "%BUILD_DIR%" (
    echo Cleaning %BUILD_DIR% ...
    rmdir /s /q "%BUILD_DIR%"
    echo Done.
) else (
    echo Nothing to clean.
)
