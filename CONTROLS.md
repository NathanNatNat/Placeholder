# Controls & Commands Reference

Quick reference for all key bindings, mouse controls, console commands, and command-line options.

---

## Key Bindings

### Camera Movement (Fly Camera)

| Key | Action |
|-----|--------|
| W | Move forward |
| S | Move backward |
| A | Strafe left |
| D | Strafe right |
| Space | Move up |
| Left Shift | Move down |

### Camera Control

| Input | Action |
|-------|--------|
| Middle Mouse (drag) | Look / orbit |
| Shift + Middle Mouse (drag) | Pan |
| Scroll Wheel | Zoom (orbit camera) |

### Toggles

| Key | Action |
|-----|--------|
| F1 | Toggle wireframe rendering |
| F2 | Toggle camera mode (fly / orbit) |
| F11 | Toggle fullscreen |
| Escape | Quit |

All key bindings above are suppressed when an ImGui text field has focus (e.g. while typing in the console).

### Editor Menu

The menu bar at the top of the screen has two menus:

**View** — Toggle editor panels on/off:
- Viewport, Scene, Properties, Asset Browser, Renderer Stats, Performance, Console (~), ImGui Demo

**Debug** — Toggle debug features:
- Wireframe (F1)

---

## Console Commands

Open the console from the **View > Console** menu (shortcut shown as `~`). Type `help` for a list of all commands.

| Command | Description |
|---------|-------------|
| `wireframe [on\|off]` | Toggle wireframe rendering. No argument toggles. |
| `camera [fly\|orbit]` | Switch camera mode. No argument toggles. |
| `reload` | Reload all shaders from disk (hot-reload). |
| `grid [on\|off]` | Toggle the ground grid. |
| `axes [on\|off]` | Toggle the origin axes (RGB = XYZ). |
| `debug [on\|off]` | Master toggle for all debug drawing. |
| `demo` | Toggle the ImGui demo window. |
| `clear` | Clear console output. |
| `help` | List all registered commands. |
| `help <command>` | Show help for a specific command. |

Console supports **Tab** autocomplete and **Up/Down** arrow history navigation.

---

## Command-Line Options

All JSON config keys can be overridden on the command line with `--key value` syntax:

```
placeholder_game.exe --window.width 1280 --window.height 720
placeholder_game.exe --rendering.vsync false
placeholder_game.exe --window.fullscreen true
placeholder_game.exe --model "path/to/model.obj"
```

| Option | Default | Description |
|--------|---------|-------------|
| `--window.width` | 1920 | Window width in pixels |
| `--window.height` | 1080 | Window height in pixels |
| `--window.title` | Placeholder Engine | Window title |
| `--window.fullscreen` | false | Start in fullscreen |
| `--rendering.vsync` | true | Enable vertical sync |
| `--model` | assets/models/planet/planet.obj | Model file to load on startup |

---

## Input Configuration

Key bindings are defined in `assets/config/engine.json` under the `input.bindings` array. Each binding has:

- **action** — Named intent (e.g. `"MoveForward"`)
- **type** — `trigger` (one-shot on press), `hold` (continuous), or `axis` (mouse delta)
- **key** — Key name (e.g. `"W"`, `"F1"`, `"Space"`, `"LeftShift"`)
- **mouseButton** — Mouse button name (e.g. `"Middle"`, `"Left"`, `"Right"`)
- **modifierKey** — Optional modifier requirement (e.g. `"LeftShift"`)
