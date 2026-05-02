# Toolchain file for Placeholder — Windows x64, MSVC
#
# Sets the C++ standard, compiler flags, and MSVC-specific options so that
# CMakeLists.txt stays declarative and tool-agnostic.
# Referenced by CMakePresets.json; can also be passed manually:
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/windows-msvc-x64.cmake

# ── C++ standard ──────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "Require C++ standard")
set(CMAKE_CXX_EXTENSIONS OFF CACHE BOOL "Disable compiler extensions")

# ── Compiler flags ────────────────────────────────────────────────────────────
# /W4           — high warning level
# /utf-8        — source and execution character set
# /permissive-  — strict conformance mode
set(CMAKE_C_FLAGS_INIT "/W4 /utf-8 /permissive-")
set(CMAKE_CXX_FLAGS_INIT "/W4 /utf-8 /permissive-")

# ── Debug information format ──────────────────────────────────────────────────
# Use /Zi (separate PDB) for all builds. Cleaner than appending raw flags.
# Options: Embedded (/Z7), ProgramDatabase (/Zi), EditAndContinue (/ZI)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "ProgramDatabase" CACHE STRING "MSVC debug info format")

# ── Linker ────────────────────────────────────────────────────────────────────
# Uncomment to use lld-link for faster link times during development.
# Requires LLVM/LLD to be installed (ships with Visual Studio 17.x+).
# set(CMAKE_LINKER_TYPE LLD CACHE STRING "Use lld-link for faster linking")
