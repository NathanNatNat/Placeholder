# CLAUDE.md — Placeholder Engine

This document is the authoritative reference for Claude Code when working on the Placeholder engine. Follow every instruction in this file. If something conflicts with your defaults, this file wins.

**Companion document:** `PLAN.md` contains the phased implementation plan with dependency graph. Always read both files before starting work. When told to "start Phase X", refer to PLAN.md for the task list and this file for all conventions, rules, and architectural decisions.

**Starting state:** This project starts as a new, clean repository containing only `CLAUDE.md` and `PLAN.md`. Phase 0 in PLAN.md covers initializing git, creating the folder structure, and setting up CMake. Do not assume any files, folders, git repo, or dependencies exist until you create them.

**Migration context:** This engine is built by migrating working code from an existing C++ project (`wow.export.cpp`). See the "Migration References" section for details.

---

## Project Overview

**Placeholder** is a C++ 3D engine focused on loading, rendering, and inspecting World of Warcraft game assets, with a general-purpose engine foundation that can be extended over time. It is not a full game engine yet — the current scope is a WoW model/asset viewer built on solid engine architecture.

- **License:** MIT
- **Platform:** Windows x64 only
- **Compiler:** MSVC only (Visual Studio Build Tools / Visual Studio)
- **C++ Standard:** C++20 (C++17 is the minimum compatibility baseline — use stable C++20 features like concepts, `std::format`, designated initializers, and `std::span`, but avoid immature features like modules and coroutines)
- **IDE:** VS Code (with CMake Tools + C/C++ extensions; CMakePresets.json is auto-detected)

---

## STRICT RULES — READ FIRST

### Git Safety — NEVER Do These

- **NEVER** open a Pull Request — not on this repo, not on any repo
- **NEVER** create or interact with GitHub Issues
- **NEVER** push to, clone, fork, or interact with any repository other than this project (read-only access to reference repos for viewing code is permitted — see Migration References)
- **NEVER** add co-authors to commits (no `Co-authored-by` trailers)
- **NEVER** force push (`git push --force` or `--force-with-lease`)
- **NEVER** reset, rebase, or rewrite git history
- **NEVER** commit secrets or API keys

### System Safety — NEVER Do These

- **NEVER** run commands that install system-wide software without explicit permission from the user
- **NEVER** create, modify, or delete files outside the project directory
- **NEVER** download large binary files (>10MB) without asking first
- **NEVER** commit build artifacts, generated files, or binary output (ensure `.gitignore` covers these)

### Project Safety — NEVER Do These

- **NEVER** delete source files without explicit permission from the user
- **NEVER** run processes that cannot be interrupted (infinite loops, blocking listeners without timeout, etc.)
- **NEVER** modify this CLAUDE.md file without explicit permission from the user

---

## Migration References

This engine migrates working code from an existing C++ port of wow.export. Two reference repositories exist on the local machine:

- **Original JavaScript wow.export:** `D:\Repositories\wow.export`
  - This is the authoritative functional reference — if behaviour is unclear, this is the "correct" version
  - Use this to understand *what* a feature should do
- **Existing C++ port:** `D:\Repositories\wow.export.cpp`
  - This contains working C++ implementations of CASC, BLP, M2, WMO, DB2, 3D rendering, and audio
  - Use this to understand *how* the JS logic was converted to C++
  - Migrate working code from here into the new engine architecture, adapting it to fit

### How to Port Features

When migrating or implementing a feature:

1. **Read the existing C++ code** in `D:\Repositories\wow.export.cpp` to understand the current implementation
2. **Read the original JS code** in `D:\Repositories\wow.export` if the C++ version is incomplete or unclear
3. **Adapt the code** to fit the new engine's architecture (module structure, coding conventions, handle-based resources, etc.)
4. **Do not copy-paste blindly** — restructure the code to follow this project's conventions and patterns
5. **Test the migrated code** against the original to verify correctness

### What's Already Working in wow.export.cpp

The following systems have working C++ implementations ready for migration:

- CASC archive reading (BLTE decompression, file lookup)
- DB2/DBC table parsing and querying
- BLP texture loading (DXT compression)
- M2 model parsing (geometry, textures, bones, animations)
- WMO parsing (groups, materials, doodads)
- 3D model rendering/preview
- Audio playback (via miniaudio)
- HTTP/CDN access (via cpp-httplib + OpenSSL)

---

## Building

The project uses CMake with presets defined in `CMakePresets.json`. In VS Code, open the project folder and the **CMake Tools** extension auto-detects the presets — select `Windows MSVC — Debug` from the status bar kit picker to configure, then build with **F7** or the build button.

**From any terminal (recommended — handles MSVC environment automatically):**

```powershell
# Debug build (configure + build in one step)
pwsh scripts\build.ps1

# Other configs
pwsh scripts\build.ps1 -Config release
pwsh scripts\build.ps1 -Config relwithdebinfo

# Clean then build (use when switching configs or after broken configure)
pwsh scripts\build.ps1 -Clean

# Run
.\out\build\windows-msvc-debug\game\placeholder_game.exe
```

`scripts\build.ps1` activates the MSVC x64 environment itself via vswhere + VsDevCmd.bat, so it works from any PowerShell session — no Developer PowerShell required. It also strips conflicting GCC toolchains (Strawberry Perl, MSYS2, MinGW) from PATH before cmake runs.

**Claude Code:** Always use `pwsh scripts\build.ps1` to build. Do not call cmake directly — without the MSVC environment the configure will fail.

---

## Workflow Conventions

### Before Writing Code

Always explain what you are about to do and why before making changes. Describe the plan, the files involved, and the expected outcome. Then proceed.

### Commit Practices

- **Trunk-based development:** Commit directly to `main`. No feature branches.
- **Small, frequent commits:** One logical change per commit. Do not bundle unrelated changes.
- **Descriptive messages:** Simple, clear commit messages in plain English. No prefix convention (no `feat:`, no emojis). Examples:
  - `Add CASC archive reader with BLTE decompression`
  - `Fix M2 bone transform calculation for attached models`
  - `Wire up ImGui FreeType backend for crisp editor text`
- **Always build and verify before committing.** The project must compile successfully. Do not commit broken code.
- **TODO/FIXME/HACK markers:** Use these in code to mark incomplete or future work. Keep `TODO.md` at the project root in sync with all markers in the codebase.

### Refactoring

If you encounter existing code that could be improved, **suggest the improvement and ask before refactoring.** Do not silently restructure existing code.

---

## Architecture

The engine follows the **layered architecture** from Jason Gregory's *Game Engine Architecture* book. Each layer only depends on layers below it.

```
┌─────────────────────────────────────┐
│           Game / Application        │  ← game/, tools/
├─────────────────────────────────────┤
│          WoW Library (wowlib)       │  ← engine/wowlib/  (standalone-ready)
├─────────────────────────────────────┤
│     Skeletal Animation System       │  ← engine/animation/
├─────────────────────────────────────┤
│         Rendering Engine            │  ← engine/renderer/
├─────────────────────────────────────┤
│    Input / Audio / Debug / Editor   │  ← engine/input/, engine/audio/, etc.
├─────────────────────────────────────┤
│      Resource Manager (Handles)     │  ← engine/resources/
├─────────────────────────────────────┤
│    Core Systems (Memory, Jobs,      │
│    Logging, Math, Config, Console)  │  ← engine/core/
├─────────────────────────────────────┤
│      Platform Layer (Win32)         │  ← engine/platform/
└─────────────────────────────────────┘
```

### Core Pattern: Hybrid ECS

The engine uses a **hybrid architecture**: an Entity-Component-System at the core for data that benefits from cache-friendly iteration (transforms, meshes, animations), with traditional OOP classes for subsystems that don't need ECS (asset management, windowing, editor, audio).

### Error Handling

- **Fatal/unrecoverable errors:** Use exceptions (e.g., failed to create OpenGL context, corrupt CASC archive)
- **Recoverable errors:** Use error codes or `std::expected`/Result types (e.g., asset not found, unsupported M2 version)
- Never silently swallow errors. Always log them via spdlog.

---

## Project Structure

```
Placeholder/
├── CLAUDE.md                    # This file
├── PLAN.md                      # Implementation plan
├── TODO.md                      # Synced with TODO/FIXME/HACK markers in code
├── CMakeLists.txt               # Root CMake configuration
├── CMakePresets.json            # CMake presets for MSVC x64
├── LICENSE                      # MIT License
├── .gitignore
├── .gitmodules
│
├── cmake/                       # CMake support files
│   └── toolchains/
│       └── windows-msvc-x64.cmake  # MSVC x64 toolchain (C++ standard, compiler flags)
│
├── engine/                      # Engine source code
│   ├── core/                    # Core systems
│   │   ├── include/core/        # Public headers
│   │   ├── src/                 # Implementation
│   │   └── tests/               # Unit tests
│   │
│   ├── platform/                # Win32 platform layer (windowing, file I/O, native dialogs)
│   │   ├── include/platform/
│   │   ├── src/
│   │   └── tests/
│   │
│   ├── resources/               # Handle-based resource manager
│   │   ├── include/resources/
│   │   ├── src/
│   │   └── tests/
│   │
│   ├── renderer/                # OpenGL 4.6 forward renderer
│   │   ├── include/renderer/
│   │   ├── src/
│   │   └── tests/
│   │
│   ├── input/                   # Action mapping input system
│   │   ├── include/input/
│   │   ├── src/
│   │   └── tests/
│   │
│   ├── audio/                   # WoW audio playback (miniaudio)
│   │   ├── include/audio/
│   │   ├── src/
│   │   └── tests/
│   │
│   ├── animation/               # Skeletal animation (basic playback)
│   │   ├── include/animation/
│   │   ├── src/
│   │   └── tests/
│   │
│   ├── editor/                  # ImGui-based editor and debug tools
│   │   ├── include/editor/
│   │   └── src/
│   │
│   └── wowlib/                  # WoW file format library (STANDALONE-READY)
│       ├── include/wowlib/      # Public API — design as if this is a separate library
│       ├── src/
│       ├── tests/
│       └── CMakeLists.txt       # Self-contained build — no engine dependencies
│
├── game/                        # Application entry point and game-specific code
│   └── src/
│
├── tools/                       # Standalone tools (asset converters, etc.)
│   └── src/
│
├── third_party/                 # Git submodules and vendored dependencies
│   ├── glfw/
│   ├── glm/
│   ├── imgui/                   # docking branch
│   ├── spdlog/
│   ├── json/                    # nlohmann/json
│   ├── assimp/
│   ├── freetype/
│   ├── msdf-atlas-gen/
│   ├── stb/
│   ├── libwebp/
│   ├── cpp-httplib/
│   ├── openssl-prebuilt/        # Prebuilt Windows x64 OpenSSL DLLs
│   ├── zlib/
│   ├── googletest/
│   ├── tracy/
│   └── glad/                    # OpenGL 4.6 loader (generated)
│
├── assets/                      # Runtime assets
│   ├── shaders/                 # GLSL shader files
│   ├── textures/                # Test/default textures
│   ├── fonts/                   # UI fonts + icon font (FontAwesome/Lucide)
│   ├── models/                  # Test models
│   └── config/                  # Default configuration (JSON)
│
└── tests/                       # Integration and render tests (cross-module)
    ├── integration/
    ├── render/
    └── data/                    # Test data (sample WoW files, reference images)
```

### Header/Source Separation

Every module uses separate `include/` and `src/` directories. Public headers go in `include/<module_name>/`, private headers and implementation files go in `src/`.

### WoW Library Isolation

The `engine/wowlib/` module must have **zero dependencies on the engine**. It depends only on standard C++, its own internal utilities, zlib (for BLTE decompression), cpp-httplib + OpenSSL (for CDN access), and stb/libwebp (for BLP texture decoding). It must be extractable into its own git repository at any time without modification.

---

## Dependencies & Third-Party Libraries

All dependencies are managed as **git submodules** where possible. For libraries without good submodule/CMake support, vendor them into `third_party/` manually.

| Library | Purpose | Integration |
|---|---|---|
| GLFW | Windowing, input, OpenGL context | Git submodule |
| GLM | Math (vectors, matrices, quaternions) | Git submodule |
| GLAD | OpenGL 4.6 Core function loader | Generated, committed |
| ImGui (docking) | Editor UI, debug panels, developer console | Git submodule |
| ImGui FreeType | Crisp editor text on 4K displays | Part of ImGui extras |
| spdlog | Logging (bundles fmt) | Git submodule |
| nlohmann/json | JSON parsing and serialization | Git submodule |
| Assimp | Import glTF, OBJ, FBX models | Git submodule |
| stb | Image loading/writing (stb_image, stb_image_write) | Git submodule |
| libwebp | WebP image encoding/decoding | Git submodule |
| FreeType | Font glyph rasterization | Git submodule |
| msdf-atlas-gen | MSDF font atlas generation for engine text | Git submodule |
| cpp-httplib | HTTP client for WoW CDN access | Git submodule |
| OpenSSL | HTTPS/TLS + cryptographic hashing (MD5/SHA1/SHA256 for CASC) | Prebuilt Windows x64 DLLs |
| zlib | Compression/decompression (BLTE, etc.) | Git submodule |
| miniaudio | Audio playback (WoW sound files) | Git submodule |
| Google Test | Unit, integration, and render testing | Git submodule |
| Tracy | Real-time CPU/GPU profiler | Git submodule |

### Icon Font

Use an icon font (FontAwesome or Lucide) bundled into ImGui's font atlas for UI icons. No SVG parsing library needed.

### Native File Dialogs

Use **Win32 native APIs** (GetOpenFileName / IFileDialog) for file open/save dialogs. No third-party dialog library.

### WoW Math Utilities

GLM is the primary math library. A thin utility layer on top of GLM handles WoW-specific math (coordinate system conversion, M2 bone transforms, ADT height calculations). This lives inside `engine/wowlib/`.

### Deferred Libraries (Not Included Yet)

| Library | Purpose | When to Add |
|---|---|---|
| minizip-ng | ZIP archive I/O for export functionality | When export features are migrated |

---

## Rendering

### Current Scope: Forward Renderer (OpenGL 4.6)

The renderer uses a **forward rendering pipeline** with a **hardcoded pass order**. This is sufficient for WoW-style rendering and keeps complexity manageable.

**Hardcoded pipeline:**
1. Clear buffers
2. Geometry pass (WoW-style materials: diffuse + emissive + vertex coloring + env maps + alpha blending)
3. Skybox pass (cubemap-based, supports loading WoW skyboxes)
4. Debug drawing pass (lines, wireframe, bounding boxes, normals)
5. ImGui pass (editor, console, performance overlay)

### Renderer Abstraction

Even though only OpenGL 4.6 is implemented now, the renderer interface should be **abstracted** so that Vulkan and DirectX backends can be added later without rewriting game/engine code. Use a `RenderDevice` / `RenderBackend` interface pattern.

### Shaders

- Write shaders in **GLSL** with a **custom preprocessor** supporting `#include`, `#define`, and shader variants
- WoW materials use many permutations (blend modes, texture layers, vertex coloring on/off, env mapping on/off) — the preprocessor handles these via define flags
- Shaders live in `assets/shaders/`
- Support **hot-reloading** of shaders during development (file watcher triggers recompilation)

### Materials: WoW-Style Only (For Now)

The material system supports WoW's shading model:
- Diffuse textures (BLP format via custom loader, standard formats via stb_image)
- Emissive maps
- Environment cube maps (armor shine, reflections)
- Vertex coloring
- Multiple blend modes (opaque, alpha test, alpha blend, additive, modulate)
- Texture combiner operations (1-4 texture layers)

### HiDPI / 4K Support

The engine must handle DPI scaling correctly everywhere:
- GLFW window creation with DPI awareness
- ImGui font atlas scaled to monitor DPI
- ImGui style scaling
- Resolution-independent rendering
- Framebuffer size vs window size handled properly

---

## WoW Library (`engine/wowlib/`)

This is the most unique part of the engine. The WoW library reads World of Warcraft game files and provides the engine with structured data for rendering.

### Design Principles

- **Standalone-ready:** The wowlib has NO dependencies on the engine. It should be extractable into its own git repository at any time.
- **Engine-agnostic output:** The wowlib outputs raw structured data (vertices, indices, bone hierarchies, texture references, animation data). It does NOT create GPU resources — that's the engine's job.
- **Modern/Retail first:** Target the latest WoW client version (CASC archives, modern M2/WMO/ADT formats). Older expansion support is a future goal.
- **CDN support:** Modern WoW installations can be partial. The wowlib must support fetching missing files from Blizzard's CDN via cpp-httplib + OpenSSL.

### Implementation Priority

1. **CASC Archive Reader** — Extract files from the CASC storage system (BLTE decompression, encoding tables, root file lookup, CDN fallback). This unlocks access to all other file types.
2. **DB2/DBC Reader** — Parse game database tables (items, spells, creatures, maps, light data).
3. **M2 Model Reader** — Parse character/creature/item models (geometry, textures, bone hierarchies, animation sequences).
4. **WMO Reader** — Parse world map objects (buildings, dungeons, interiors — geometry, materials, doodad sets).
5. **ADT Terrain Reader** — Parse terrain tiles (future milestone).

### Key Reference Resources

- **wowdev.wiki** — Primary specification reference for all WoW file formats
- **https://github.com/Kruithne/wow.export** — Original JS wow.export (functional reference)
- **https://github.com/Deamon87/WebWowViewerCpp** — C++ WoW model viewer (rendering reference)
- **https://github.com/Marlamin/wow.tools.local** — Local WoW data tools (DB2 parsing reference)
- **https://github.com/JoeyDeVries/LearnOpenGL** — Excellent OpenGL learning resource with tutorials, sample code, and test models
- Other repositories by these developers should also be consulted

### BLP Texture Loader

A custom BLP texture loader handles WoW's proprietary texture format. BLP files use DXT/BC compression internally. The loader converts BLP data into a format the engine's texture system can consume.

---

## Core Systems

### Memory Management: Hybrid

- **Custom allocators** for performance-critical hot paths: pool allocators for ECS component storage, stack/linear allocators for per-frame temporary data
- **Smart pointers** (`std::unique_ptr`, `std::shared_ptr`) for everything else: subsystem lifetime, asset management, editor state
- Never use raw `new`/`delete` directly — always go through an allocator or smart pointer

### Threading: Job System + Dedicated Threads

- A **thread pool with work-stealing queues** for parallelizable tasks (asset loading, decompression, mesh processing)
- **Dedicated threads** for critical systems that need consistent timing (audio playback)
- The **main thread** owns the OpenGL context, runs the game loop, and handles input
- Asset loading is **asynchronous** — loading operations return immediately and complete in the background on the job system

### Resource Manager: Handle-Based

Resources (textures, meshes, shaders, audio clips) are managed through opaque, type-safe handles (`ResourceHandle<Texture>`, `ResourceHandle<Mesh>`, etc.).

- Handles include a generation counter to detect use-after-free
- The resource manager owns all memory and controls layout
- Async loading returns a handle immediately — access before load completes returns a placeholder
- Internally, resources are stored in per-type pools
- Hot-reloading: file watcher detects changes → resource is reloaded → existing handles automatically point to new data

### Logging: spdlog

- Use spdlog throughout the codebase
- Log levels: TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL
- Per-subsystem loggers (e.g., `spdlog::get("renderer")`, `spdlog::get("wowlib")`)
- Log to console and to file simultaneously

### Configuration

- **JSON config files** (via nlohmann/json) for persistent settings (resolution, graphics options, key bindings, WoW install path)
- **Command-line arguments** for overrides and testing (take precedence over JSON)
- Default config lives in `assets/config/`
- The WoW game data path is stored in the default config — this is intentional and not considered a secret

### Game Loop

**Fixed timestep with interpolation:**
- Logic updates at a fixed rate (e.g., 60Hz)
- Rendering runs as fast as possible
- Interpolation between the previous and current state for smooth visuals
- Delta time is accumulated and consumed in fixed steps

---

## Input System: Action Mapping

The input system uses an **action mapping abstraction**:

- **Actions** are named gameplay intents (e.g., `"MoveForward"`, `"RotateCamera"`, `"ToggleWireframe"`)
- **Bindings** map physical inputs to actions (defined in JSON config)
- **Input types:** trigger (one-shot on press), hold (continuous while held), axis (2D mouse delta)
- Game logic queries actions, never raw keys
- Supports keyboard and mouse
- Rebindable by editing the config file or through the ImGui editor

---

## Editor & Debug Tools

### ImGui Editor

ImGui (docking branch) serves as both the editor and in-game UI for now. Features:
- Scene inspection (entity list, component properties)
- Asset browser (loaded resources, WoW file browser)
- Renderer debug (draw calls, GPU stats, shader inspector)
- Material editor (tweak WoW material parameters in real time)
- Animation playback controls (play, pause, speed, sequence selection)
- Performance overlay (FPS, frame time, memory usage)

Use the **ImGui FreeType addon** for crisp text rendering on 4K displays. Use an **icon font** (FontAwesome/Lucide) for UI icons.

### Developer Console

An in-engine console (rendered via ImGui) with command registration:
- Type commands to toggle wireframe, teleport camera, reload shaders, dump resource stats, etc.
- Commands are registered by subsystems at initialization
- Console history and autocomplete

### Debug Drawing

A debug draw system for 3D overlays:
- Lines and line strips
- Wireframe rendering toggle
- Bounding boxes (AABB, OBB)
- Normal visualization
- Bone/skeleton visualization for M2 models
- All debug drawing can be toggled per-category via the editor or console

### Profiling

- **Tracy Profiler:** Instrument critical code paths with Tracy zones for CPU and GPU profiling
- **ImGui performance overlay:** Real-time FPS, frame time graph, draw call count, memory usage, loaded resource count

---

## Cameras

Two camera modes, switchable at runtime:

- **Free fly camera:** WASD + mouse look, for navigating and inspecting scenes freely
- **Third-person orbit camera:** Orbits around a target point/entity (WoW-style), zoom with scroll wheel, rotate by dragging

---

## Animation

### Current Scope: Basic Skeletal Playback

- Load bone hierarchies from glTF (via Assimp) and WoW M2 format
- Play animation sequences (play, stop, loop, set speed)
- Transform mesh vertices on the CPU based on bone matrices
- No animation blending, state machines, or IK (future milestones)

---

## Audio

### Current Scope: WoW Audio Playback Only

- Use **miniaudio** for simple audio file playback
- Play WoW sound files extracted from CASC
- No 3D spatial audio, no reverb zones, no music system (future milestones)

---

## Testing

### Three-Tier Testing with Google Test

1. **Unit tests** (per-module, in `engine/<module>/tests/`): Test individual functions and classes in isolation.
2. **Integration tests** (in `tests/integration/`): Test subsystems working together end-to-end.
3. **Render tests** (in `tests/render/`): Render to offscreen framebuffer, compare against reference images.

Test data (sample WoW files, reference images) lives in `tests/data/`.

---

## Coding Conventions

### Style

- Follow **C++ Core Guidelines**
- **Allman brace style** (opening brace on its own line):
  ```cpp
  if (condition)
  {
      doSomething();
  }
  ```
- PascalCase for types and classes: `RenderDevice`, `CascArchive`, `M2Model`
- camelCase for functions and methods: `loadTexture()`, `getBoneTransform()`
- camelCase for local variables: `vertexCount`, `boneIndex`
- UPPER_SNAKE_CASE for constants and macros: `MAX_TEXTURE_LAYERS`, `PLACEHOLDER_VERSION`
- Member variables prefixed with `m_`: `m_vertexBuffer`, `m_isLoaded`
- Use `#pragma once` for header guards

### Documentation

- **Doxygen comments** for all public headers
- Use `///` for single-line Doxygen, `/** */` for multi-line
- Document parameters, return values, and any non-obvious behavior

---

## Build System: CMake

### Philosophy: Pragmatic, Declarative CMake

CMakeLists.txt files should **describe** the project (targets, sources, dependencies), not **implement** build logic. Avoid variables, loops, conditionals, and platform-specific branching in CMakeLists — those belong in toolchain files or presets. The builder controls settings like the C++ standard and build type, not the project.

### Toolchain File

Compiler-specific settings live in `cmake/toolchains/windows-msvc-x64.cmake`, not in CMakeLists.txt:
- **C++ standard** (`CMAKE_CXX_STANDARD 20`) — set here so all targets (including dependencies) use the same standard, avoiding ABI incompatibilities
- **Compiler flags** (`/W4 /utf-8 /permissive-`) — set via `CMAKE_CXX_FLAGS_INIT` so they're applied consistently
- **Debug info format** (`CMAKE_MSVC_DEBUG_INFORMATION_FORMAT`) — use the first-class CMake variable instead of raw `/Zi` flags
- **Linker selection** (`CMAKE_LINKER_TYPE`) — ready to switch to `LLD` for faster link times (commented out by default)

The presets in `CMakePresets.json` reference this toolchain file automatically. The build script (`scripts\build.ps1`) uses the presets, so everything chains together. When adding Linux support, add a new toolchain file (e.g., `linux-gcc.cmake`) and corresponding presets — no CMakeLists changes needed.

### CMake Conventions

- **Minimum version:** `cmake_minimum_required(VERSION 3.31)` — enables `FILE_SET HEADERS`, `CODEGEN` target, preset `$comment`, and fast FetchContent
- **Alias targets:** Every engine module defines a `Placeholder::` alias (e.g., `Placeholder::Core`, `Placeholder::Renderer`). Always use alias names in `target_link_libraries` — they provide early error detection for typos vs. silent linker failures
- **Header file sets:** Modules with public headers use `target_sources(... FILE_SET HEADERS ...)` instead of manual `target_include_directories`. This declares headers in one place and enables automatic install support
- **Test target:** `include(CTest)` is in the root CMakeLists so `ctest --test-dir <build>` always works, even with zero tests registered
- **No `CMAKE_CXX_STANDARD` in CMakeLists** — this is set by the toolchain file. Do not add it back
- **No `add_compile_options` in CMakeLists** — compiler flags come from the toolchain file
- **No `CMAKE_BUILD_TYPE` in CMakeLists** — this is set by the presets or the builder. Do not set it in the project
- **`SYSTEM` on third-party subdirectories:** All `add_subdirectory()` calls for third-party dependencies use the `SYSTEM` keyword (CMake 3.25+) to treat their headers as system headers and suppress compiler warnings from dependency code
- **`CMakeUserPresets.json` is gitignored** — this file is for developer-specific local presets. The project never provides it

### Modern CMake Preferences

When adding new CMake code, prefer these modern approaches over older alternatives:

| Prefer | Over | Why |
|---|---|---|
| `target_sources(FILE_SET HEADERS)` | `target_include_directories` for public headers | Declares headers once, enables install |
| `CMAKE_MSVC_DEBUG_INFORMATION_FORMAT` | Raw `/Zi` `/Z7` flags | First-class CMake variable (3.25+) |
| `CMAKE_MSVC_RUNTIME_LIBRARY` | Raw `/MD` `/MT` flags | First-class CMake variable (3.15+) |
| `CMAKE_LINKER_TYPE` | Manual linker path flags | First-class linker selection (3.29+) |
| `CMAKE_COMPILE_WARNING_AS_ERROR` | Raw `/WX` `-Werror` flags | Portable across compilers (3.24+) |
| `add_custom_command(CODEGEN)` | Plain `add_custom_command` for code gen | Enables `codegen` build target (3.31+) |
| `cmake_path()` | `get_filename_component()` | Modern path manipulation (3.20+) |
| Toolchain files | `set(CMAKE_CXX_STANDARD)` in CMakeLists | Builder controls the standard, not the project |
| Presets | CLI flags for common configurations | Reproducible, documented, IDE-integrated |
| `add_subdirectory(dep SYSTEM)` | `add_subdirectory(dep)` for third-party code | Treats dep headers as system headers, suppresses warnings (3.25+) |
| `FetchContent` with `SYSTEM` | FetchContent without | Suppresses warnings from fetched deps (3.25+) |
| `TRANSITIVE_COMPILE_PROPERTIES` | Manual property propagation | Custom properties propagate like built-ins (3.30+) |

### Structure

- Root `CMakeLists.txt` defines the project and adds subdirectories
- Each engine module has its own `CMakeLists.txt` producing a static library
- `wowlib` has a fully self-contained `CMakeLists.txt` with no engine dependencies
- Third-party dependencies added via `add_subdirectory(... SYSTEM)` for submodules
- Assets are copied to the build output directory automatically
- CMakePresets.json (schema version 10) for MSVC x64 Debug, Release, and RelWithDebInfo
- **Windows x64 only** — enforce 64-bit architecture in CMake, reject non-x64 configurations
- OpenSSL prebuilt DLLs copied to output directory at build time

---

## Future Milestones (NOT in current scope)

These are explicitly **out of scope** for now. Do not implement these unless specifically asked.

- **Physics:** Bullet Physics integration
- **3D Spatial Audio:** Positional audio, reverb zones, music system
- **Chunked Streaming Terrain:** WoW-style ADT tile loading and rendering with LOD
- **Particle System:** Hybrid CPU/GPU particles
- **Shadow Mapping:** Directional, point, and spot light shadows
- **Deferred / Hybrid Rendering:** Deferred pass for opaque geometry, switchable pipelines
- **Frame Graph:** Declarative render pass system with automatic resource management
- **PBR Materials:** Metallic/roughness workflow alongside WoW-style materials
- **SPIR-V Pipeline:** Compile GLSL to SPIR-V for Vulkan/OpenGL binary shader loading
- **Vulkan / DirectX Backends:** Additional render backends behind the abstraction
- **Frustum & Occlusion Culling:** Frustum culling, portal-based occlusion for WMO interiors
- **LRU Asset Cache:** Evict unused assets from memory based on access patterns
- **Event Bus:** Decoupled pub/sub messaging between subsystems
- **Level of Detail:** Automatic and manual LOD for meshes
- **Water / Fog / Atmosphere:** Environmental rendering effects
- **Animation Blending / State Machines / IK**
- **Custom In-Game UI** (replacing ImGui for game-facing UI)
- **Export Functionality** (OBJ/GLTF export, minizip-ng integration)
- **Screenshot / Video Capture**
- **Networking / Multiplayer**
- **Scripting Language** (Lua, Python, C#)
- **Older WoW Expansion Support** (Classic, WotLK file formats)
- **Linux / macOS Support**
