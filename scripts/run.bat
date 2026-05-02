@echo off
REM Run the game executable. Passes all arguments through.
REM Usage: run.bat [args...]

setlocal
cd /d "%~dp0.."

set "EXE=out\build\game\placeholder_game.exe"

if not exist "%EXE%" (
    echo ERROR: %EXE% not found. Run scripts\build.bat first.
    exit /b 1
)

"%EXE%" %*
