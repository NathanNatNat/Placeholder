# TODO — Placeholder Engine

Tracks all TODO/FIXME/HACK markers in the codebase, plus phase progress.

---

## Phase Progress

- [x] Phase 0: Project Bootstrap
- [x] Phase 1: Core Systems
- [x] Phase 2: Platform & Window
- [x] Phase 3: Renderer Foundation
- [x] Phase 4: Input & Camera
- [x] Phase 5: Testing Infrastructure
- [x] Phase 6: Asset Pipeline Foundation
- [x] Phase 7: ImGui Editor Shell
- [ ] Phase 8: WoW Library Migration
- [ ] Phase 9: Skeletal Animation
- [ ] Phase 10: Audio, Hot Reload & Polish
- [ ] Phase 11: WoW Library — ADT Terrain (Future)

---

## Code Markers

### Phase 6 Notes
- Model loading via `--model=path/to/file` command-line argument
- WebP texture loading available but untested with real WebP files
- No async resource loading yet — all loading is synchronous on the main thread

### Phase 7 Notes
- ImGui docking branch with FreeType font rasterization for crisp 4K text
- Font Awesome 6 icon font merged into default font atlas
- DPI-aware font scaling with runtime rebuild on monitor change
- Editor panels: Scene, Properties, Asset Browser, Renderer Stats, Performance Overlay
- Developer console with command history, autocomplete, and colored output
- Debug drawing system: grid, axes, lines, AABBs, spheres (batched VBO)
- Tracy profiler submodule added (disabled by default, enable with -DTRACY_ENABLE=ON)
- Console commands: wireframe, camera, reload, grid, axes, debug, demo, clear
- ImGui input capture prevents game input when interacting with editor UI

### Phase 8 Notes (In Progress)
- Dependencies added: zlib (submodule), cpp-httplib (submodule), OpenSSL 3.3.2 (prebuilt DLLs)
- DataBuffer: binary I/O class with endianness, zlib inflate/deflate, OpenSSL hashing
- CASC system migrated: BLTE reader (lazy decompression, Salsa20 encryption), root/encoding parsing, local archive reading, CDN resolver
- BLP texture decoder: palette, DXT1/3/5, BGRA encoding types
- M2 loader: full MD21 chunk parsing (geometry, textures, materials, bones, collision), skin file parsing
- WMO loader: root file parsing (materials, groups, doodads), group geometry extraction
- WowScene: CASC-to-engine bridge (M2 loading, BLP textures, mesh conversion, material mapping)
- WoW model shaders: hemisphere ambient + directional lighting, alpha test/blend variants
- TODO: DB2/DBC full implementation, WMO rendering integration, ImGui WoW browser panel, render tests
