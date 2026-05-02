# PLAN.md — Placeholder Engine Implementation Plan

This document defines the build order for the Placeholder engine. Every phase lists what it produces, what it depends on, and what C++ / graphics concepts you'll encounter while reviewing the code.

**Starting state:** A new, empty folder containing only `CLAUDE.md` and `PLAN.md`. No git repo, no folders, no dependencies. Phase 0 creates everything from scratch.

**Migration context:** Working code exists in `D:\Repositories\wow.export.cpp`. This plan migrates that code into the new architecture rather than rewriting from scratch. Phases marked with 🔄 involve migrating existing code. Phases marked with 🆕 involve building new systems.

Work through phases in order — each phase unlocks the next. Within a phase, tasks are listed in dependency order too.

---

## Dependency Graph Overview

```
Phase 0: Project Bootstrap 🆕
    │
    ▼
Phase 1: Core Systems 🆕
    │
    ├──────────────────────┐
    ▼                      ▼
Phase 2: Platform &      Phase 5: Testing
Window 🔄🆕               Infrastructure 🆕
(GLFW, GL 4.6)            (Google Test)
    │                      │
    ▼                      │ (tests added continuously)
Phase 3: Renderer          │
Foundation 🔄🆕            │
    │                      │
    ├──────────┐           │
    ▼          ▼           │
Phase 4:   Phase 6:       │
Input &    Asset Pipeline  │
Camera 🆕  🔄🆕            │
    │      (stb, resources │
    │       handles)       │
    │          │           │
    ▼          ▼           │
Phase 7: ImGui Editor 🔄🆕 ◄┘
Shell (panels, console,
debug drawing, profiling)
    │
    ▼
Phase 8: WoW Library Migration 🔄
(CASC, DB2, BLP, M2, WMO — migrate existing C++ code)
    │
    ▼
Phase 9: Skeletal Animation 🔄
    │
    ▼
Phase 10: Audio, Hot Reload & Polish 🔄🆕
```

---

## Phase 0: Project Bootstrap 🆕

**Depends on:** Nothing
**Starting state:** An empty folder containing only `CLAUDE.md` and `PLAN.md`
**Produces:** A fully initialized project with git, folder structure, CMake, and a "hello world" that compiles and runs

### Tasks

1. Initialize a git repository (`git init`)
2. Create `.gitignore` covering:
   - Build directories (`build/`, `cmake-build-*/`, `out/`, `x64/`)
   - IDE files (`.vs/`, `.vscode/`, `*.user`, `*.suo`, `*.sln`, `*.vcxproj*`)
   - Binaries and objects (`*.obj`, `*.exe`, `*.dll`, `*.lib`, `*.pdb`, `*.ilk`, `*.exp`)
   - CMake cache files (`CMakeCache.txt`, `CMakeFiles/`, `cmake_install.cmake`)
3. Create the complete folder structure as defined in CLAUDE.md
4. Create `LICENSE` file (MIT license)
5. Create initial `TODO.md` listing all phases from this plan
6. Write root `CMakeLists.txt`:
   - `cmake_minimum_required(VERSION 3.31)`
   - Project name, MSVC x64 enforcement, toolchain file for C++20 standard and compiler flags
   - Subdirectory includes for each engine module and game
7. Write `CMakePresets.json` with MSVC x64 presets for Debug, Release, RelWithDebInfo
8. Write placeholder `CMakeLists.txt` for each engine module (empty static libs with placeholder `.cpp`)
9. Write `engine/wowlib/CMakeLists.txt` as self-contained (no engine dependencies)
10. Create `game/src/main.cpp` that prints "Placeholder Engine v0.1" and returns 0
11. Create default `assets/config/engine.json`:
    ```json
    {
        "window": { "width": 1920, "height": 1080, "title": "Placeholder Engine", "fullscreen": false },
        "rendering": { "vsync": true },
        "wow": { "dataPath": "" }
    }
    ```
12. Verify the project builds: `cmake --preset=debug` → `cmake --build --preset=debug`
13. First commit: `Initial project structure with CMake build system`

### What You'll Learn

- **CMake basics:** How `CMakeLists.txt` files work, presets, MSVC toolchain configuration
- **Project organization:** Layered module structure from Jason Gregory's book
- **Git fundamentals:** `.gitignore` patterns, initial commit

---

## Phase 1: Core Systems 🆕

**Depends on:** Phase 0
**Produces:** Logging, memory allocators, config loading, file I/O utilities, and the job system

### Tasks

1. Add spdlog as git submodule → wire into CMake → create per-subsystem logger initialization
2. Add nlohmann/json as git submodule → wire into CMake
3. Build the configuration system:
   - Load/save `assets/config/engine.json`
   - Command-line argument parser that overrides JSON values
   - Default config includes WoW data path
4. Build file I/O utilities:
   - File reading (binary and text)
   - Path utilities (join, extension, normalize)
   - File watcher using `ReadDirectoryChangesW` (Win32 API) for hot-reload later
5. Build memory allocators:
   - Linear allocator (bump pointer, reset per frame)
   - Pool allocator (fixed-size blocks)
   - Stack allocator (LIFO allocation)
6. Build the job system:
   - Thread pool with configurable thread count
   - Work-stealing queues
   - `schedule()`, `wait()`, `waitAll()` API
   - Dedicated thread spawning utility
7. Build the developer console backend:
   - Command registration: `console.registerCommand("wireframe", callback)`
   - Command parsing, dispatch, history buffer
   - (No rendering yet — that comes with ImGui in Phase 7)

### What You'll Learn

- **RAII:** Constructors acquire, destructors release — the most important C++ pattern
- **Smart pointers:** `std::unique_ptr` vs `std::shared_ptr`, when to use each
- **Move semantics:** `std::move()`, rvalue references
- **Multithreading:** `std::thread`, `std::mutex`, atomics, work-stealing queues
- **Win32 API basics:** `ReadDirectoryChangesW` for file watching

---

## Phase 2: Platform & Window 🔄🆕

**Depends on:** Phase 1
**Produces:** A window with an OpenGL 4.6 context and DPI awareness

### Tasks

1. Add GLFW as git submodule → wire into CMake
2. Generate GLAD loader for **OpenGL 4.6 Core** (upgrade from 4.3 in wow.export.cpp) → commit to `third_party/glad/`
3. Create `Window` class: create window, handle resize, DPI scaling, fullscreen toggle
4. Initialize OpenGL 4.6 context via GLAD
5. Set up OpenGL debug message callback → spdlog
6. Main loop skeleton: poll events → clear screen → swap buffers
7. Handle DPI: query monitor DPI, store scale factor, handle framebuffer vs window size
8. Basic input polling through GLFW (raw state — action mapping comes in Phase 4)

**Migration notes:** Reference `D:\Repositories\wow.export.cpp` for GLFW initialization patterns, but restructure into the new `engine/platform/` module. Upgrade GLAD from GL 4.3 to GL 4.6.

### What You'll Learn

- **Graphics API initialization:** OpenGL context, GLAD loader, core profile
- **Event loops:** The fundamental game loop pattern
- **DPI awareness:** Why 4K monitors need special handling

---

## Phase 3: Renderer Foundation 🔄🆕

**Depends on:** Phase 2
**Produces:** A colored triangle on screen, shader loading, and the renderer abstraction

### Tasks

1. Create `RenderDevice` / `RenderBackend` abstract interface
2. `OpenGLRenderDevice` implementation: buffers (VBO/EBO/VAO), shader compilation, uniforms, render state
3. GLSL shader system with preprocessor (`#include`, `#define` injection)
4. Render a hardcoded colored triangle
5. Basic render state management (depth test, blend modes, face culling, wireframe)
6. Hardcoded render pipeline skeleton (clear → geometry → skybox → debug → ImGui)

**Migration notes:** Reference wow.export.cpp's rendering code, but rebuild behind the `RenderBackend` abstraction. The old code likely calls OpenGL directly — wrap it.

### What You'll Learn

- **The GPU pipeline:** Vertices → vertex shader → rasterizer → fragment shader → screen
- **Shaders (GLSL):** Writing GPU programs, uniforms, coordinate spaces
- **Polymorphism:** The `RenderBackend` interface uses virtual dispatch

---

## Phase 4: Input & Camera 🆕

**Depends on:** Phase 2, Phase 3
**Produces:** Action mapping input system, free fly camera, third-person orbit camera

### Tasks

1. Build the action mapping system with JSON-configurable bindings
2. Implement free fly camera (WASD + mouse look)
3. Implement third-person orbit camera (right-click drag, scroll zoom)
4. Camera switcher and view/projection matrix feed into renderer

### What You'll Learn

- **Action abstraction:** Decoupling physical keys from gameplay intents
- **Quaternions:** Camera rotation without gimbal lock
- **View/projection matrices:** How camera transforms the world

---

## Phase 5: Testing Infrastructure 🆕

**Depends on:** Phase 1
**Produces:** Google Test running, first unit tests passing

### Tasks

1. Add Google Test as git submodule → wire into CMake
2. Create per-module test targets
3. Create `tests/integration/` and `tests/render/` CMake targets
4. Write initial unit tests for Phase 1 systems (config, allocators, job system)
5. Set up render test scaffold (offscreen FBO → compare against reference PNG)
6. Add CMake `test` target

### What You'll Learn

- **Unit testing patterns:** Arrange-Act-Assert, test fixtures
- **Offscreen rendering:** Framebuffer objects, render-to-texture

---

## Phase 6: Asset Pipeline Foundation 🔄🆕

**Depends on:** Phase 3, Phase 1
**Produces:** Handle-based resource manager, texture loading, standard mesh loading, skybox

### Tasks

1. Build the handle-based resource manager (`ResourceHandle<T>` with generation counter)
2. Add stb as git submodule → texture loading (PNG, JPG, BMP, TGA)
3. Add libwebp as git submodule → WebP texture support
4. Add Assimp as git submodule → glTF, OBJ, FBX mesh loading
5. Basic material system (`Material` struct: shader, textures, blend mode)
6. Basic skybox (cubemap, rendered behind everything)
7. Render a loaded 3D model with texture — first real scene!

**Migration notes:** Migrate stb usage patterns from wow.export.cpp. The resource manager is new — the old project doesn't have handle-based management.

### What You'll Learn

- **Generational handles:** Safer than raw pointers for GPU resources
- **Async resource loading:** Futures, promises, loading without freezing
- **Texture upload:** `glTexImage2D`, compressed formats, mipmaps

---

## Phase 7: ImGui Editor Shell 🔄🆕

**Depends on:** Phase 3, Phase 4, Phase 6
**Produces:** Full ImGui integration with editor panels, console, debug drawing, profiling

### Tasks

1. Add ImGui (docking branch) as git submodule → wire into CMake with GLFW + OpenGL backends
2. Add FreeType as git submodule → enable ImGui FreeType backend for crisp 4K text
3. Add icon font (FontAwesome/Lucide) to ImGui font atlas
4. DPI-scaled ImGui initialization
5. Build editor panels: scene, properties, asset browser, renderer stats
6. Wire developer console to ImGui (text input, autocomplete, history)
7. Build debug drawing system (lines, boxes, normals, bones → batched VBO → debug pass)
8. Add Tracy profiler → instrument key code paths
9. ImGui performance overlay (FPS, frame time, memory, resource counts)
10. Register default console commands (wireframe, camera, reload, stats, debug toggles)

**Migration notes:** wow.export.cpp already has ImGui integration. Migrate useful patterns but rebuild with the FreeType addon, icon font, and docking layout.

### What You'll Learn

- **Immediate-mode GUI:** How ImGui works (no retained widget tree)
- **Font rendering:** TrueType, glyph atlases, FreeType vs stb_truetype
- **Debug visualization:** Batching debug lines into GPU buffers
- **Profiling:** Tracy zones, identifying bottlenecks

---

## Phase 8: WoW Library Migration 🔄

**Depends on:** Phase 6 (resource manager, textures), Phase 5 (testing)
**Produces:** All WoW file format code migrated into `engine/wowlib/` with proper architecture

This is the biggest migration phase. All existing WoW parsing code from `D:\Repositories\wow.export.cpp` gets restructured into the standalone `engine/wowlib/` module.

### Tasks

1. Set up `engine/wowlib/` with self-contained `CMakeLists.txt` (depends only on zlib, cpp-httplib, OpenSSL, stb, libwebp — NOT on the engine)
2. Add cpp-httplib + OpenSSL as dependencies → wire into CMake (use prebuilt OpenSSL DLLs for Windows x64)
3. **Migrate CASC reader:**
   - Copy and restructure CASC code from wow.export.cpp into wowlib
   - Adapt to new coding conventions (Allman braces, naming, Doxygen)
   - Consolidate hashing to use OpenSSL (drop mbedTLS)
   - Ensure CDN fallback for partial installations works
   - Write unit tests
4. **Migrate DB2/DBC reader:**
   - Copy and restructure DB2 parsing code
   - Adapt to new conventions
   - Write unit tests
5. **Migrate BLP texture loader:**
   - Copy and restructure BLP code
   - Integrate with engine texture system via resource manager
   - Write unit tests
6. **Migrate M2 model parser:**
   - Copy and restructure M2 parsing code
   - Output structured data (no GPU resources — engine converts to meshes)
   - M2 → engine mesh + material conversion layer (lives in engine, not wowlib)
7. **Build WoW-style material shaders:**
   - Implement WoW shader combiners in GLSL (texture layers, vertex coloring, env maps, blend modes)
   - Shader preprocessor variants for different material permutations
8. **Migrate WMO parser:**
   - Copy and restructure WMO parsing code
   - WMO → engine mesh conversion with doodad placement
9. **Render a WoW M2 model with correct textures and materials!**
10. **Render a WMO with doodads!**
11. Add WoW model/WMO browser to ImGui asset browser
12. Write integration tests: load known models, verify correctness
13. Write render tests: compare against reference screenshots

### What You'll Learn

- **Large-scale code migration:** Restructuring existing code into clean architecture
- **Library API design:** Keeping wowlib engine-agnostic with clean boundaries
- **Binary file parsing:** Structs from byte buffers, endianness, packed fields
- **Shader permutations:** One uber-shader with `#define` flags for WoW materials
- **The full pipeline:** CASC → raw bytes → parsed model → engine mesh → rendered on screen

---

## Phase 9: Skeletal Animation 🔄

**Depends on:** Phase 8 (M2 models with bone hierarchy)
**Produces:** WoW models playing their animations

### Tasks

1. Migrate animation data parsing from wow.export.cpp (keyframes, interpolation types)
2. `AnimationPlayer`: plays sequence, evaluates keyframes, computes bone matrices
3. CPU skinning (vertices transformed by up to 4 bone weights)
4. ImGui animation controls (sequence list, play/pause, speed, timeline)
5. Debug bone visualization (draw skeleton as lines)
6. Tests: verify bone positions at known timestamps

### What You'll Learn

- **Skeletal animation:** Bones, hierarchies, bind pose vs animated pose
- **Keyframe interpolation:** Lerp, slerp for quaternions, bezier curves
- **Vertex skinning:** Blending influence from multiple bones per vertex

---

## Phase 10: Audio, Hot Reload & Polish 🔄🆕

**Depends on:** Phase 8 (CASC for WoW audio), Phase 6 (resource manager)
**Produces:** WoW audio playback, live asset hot-reloading, stability and polish

### Tasks

1. **Migrate audio system:**
   - Add miniaudio as git submodule
   - Migrate playback code from wow.export.cpp
   - Simple API: `play()`, `stop()`, `setVolume()`
   - Dedicated audio thread
   - ImGui audio browser (play WoW sounds)
2. **Hot-reloading system:**
   - Use Win32 `ReadDirectoryChangesW` file watcher from Phase 1
   - Watch shaders → recompile on change
   - Watch textures → reload on change
   - Watch models → reload on change
   - Resource handles remain valid — data swapped transparently
3. **Polish:**
   - Review error handling across all modules
   - Add missing tests for edge cases
   - Profile with Tracy — fix obvious bottlenecks
   - Clean up TODO/FIXME items
   - Update TODO.md
   - Write README.md with build instructions and usage guide
4. **Verify parity:** Compare rendered output against wow.export.cpp for known models

### What You'll Learn

- **Audio programming:** PCM data, sample rates, the audio thread
- **File watching:** Win32 `ReadDirectoryChangesW` API
- **Hot-reload architecture:** Swapping data behind stable handles
- **Profiling-driven optimization:** Using Tracy to find real bottlenecks

---

## Phase 11: WoW Library — ADT Terrain (Future)

**Depends on:** Phase 8, future chunked terrain system
**Produces:** WoW world terrain rendering

> **NOTE:** This phase requires the chunked streaming terrain system listed in CLAUDE.md's future milestones. Do not begin until that system is designed.

### Tasks (Outline Only)

1. ADT parser: heightmap, texture layers, object placements
2. Terrain mesh generation from heightmaps
3. Texture splatting shader
4. WDT parser for continent tile maps

---

## Summary: Build Order At a Glance

| Phase | Name | Type | Key Deliverable |
|-------|------|------|-----------------|
| 0 | Project Bootstrap | 🆕 | Empty project compiles |
| 1 | Core Systems | 🆕 | Logging, memory, config, jobs, console |
| 2 | Platform & Window | 🔄🆕 | Window with OpenGL 4.6 context |
| 3 | Renderer Foundation | 🔄🆕 | Colored triangle on screen |
| 4 | Input & Camera | 🆕 | Fly around an empty scene |
| 5 | Testing Infrastructure | 🆕 | Google Test running |
| 6 | Asset Pipeline | 🔄🆕 | Textured 3D model rendered |
| 7 | ImGui Editor | 🔄🆕 | Full editor shell with panels and tools |
| 8 | WoW Library Migration | 🔄 | **WoW models and WMOs on screen!** |
| 9 | Skeletal Animation | 🔄 | WoW models playing animations |
| 10 | Audio & Polish | 🔄🆕 | Sound playback, hot-reload, stability |
| 11 | ADT Terrain | 🔄 | World terrain (future) |

---

## How to Use This Plan

1. **You have an empty folder** with just `CLAUDE.md` and `PLAN.md` in it
2. **Open Claude Code** in that folder
3. **Tell Claude Code:** "Read CLAUDE.md and PLAN.md, then start Phase 0" — it will initialize git, create the project structure, set up CMake, and get a hello world compiling
4. **For each subsequent phase:** Tell Claude Code "Start Phase X" — it will reference CLAUDE.md for conventions and this file for what to build
5. **For migration phases (🔄):** Claude Code will read the existing code from `D:\Repositories\wow.export.cpp`, understand it, restructure it to fit the new architecture, and adapt it to the project's coding conventions
6. **Review each commit** — read the code, ask Claude Code to explain anything you don't understand. This is how you learn.
7. **Run tests frequently** — after Phase 5, every new feature should come with tests
8. **Update TODO.md** as phases complete
9. **Don't skip ahead** — each phase builds on the last. The dependency graph exists for a reason.
10. **If something breaks:** Tell Claude Code what went wrong. It will diagnose, fix, build, and verify before committing.
