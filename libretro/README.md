# SpaceCadetPinball Libretro Core

A libretro core port of Space Cadet Pinball with OpenGL hardware rendering.

## Features

- **No SDL dependency** - Pure libretro implementation
- **No ImGui dependency** - UI stubs for clean compilation  
- **OpenGL Hardware Rendering** - Uses libretro HW render callback
- **Software Rendering Fallback** - Works if OpenGL is unavailable

## Building

### Requirements
- MinGW-w64 (GCC 11+ recommended)
- GNU Make

### Build Commands

```bash
cd libretro
make          # Release build
make DEBUG=1  # Debug build
make clean    # Clean build artifacts
```

### Output
- Windows: `spacecadetpinball_libretro.dll`
- Linux: `spacecadetpinball_libretro.so`
- macOS: `spacecadetpinball_libretro.dylib`

## Installation

1. Copy `spacecadetpinball_libretro.dll` to your RetroArch `cores` directory
2. Copy game data files (PINBALL.DAT, etc.) to RetroArch `system` directory

## Usage

### Loading the Core
1. Open RetroArch
2. Load Core -> Select "Space Cadet Pinball"
3. Load Content -> Select your PINBALL.DAT file (or use "Start Core" if data is in system dir)

### Controls

| RetroArch Input | Game Action |
|-----------------|-------------|
| L / Left        | Left Flipper |
| R / Right       | Right Flipper |
| A / Down        | Plunger |
| L2              | Nudge Left |
| R2              | Nudge Right |
| Up / B          | Nudge Up |
| Start           | New Game |
| Select          | Pause |

## Game Data

The core requires the original Space Cadet Pinball data files:
- `PINBALL.DAT` - Main game data
- Or `CADET.DAT` for Full Tilt mode

These can be found in:
- Windows XP/Vista installations
- Full Tilt! Pinball game

## Technical Details

### Architecture
- `libretro_core.cpp` - Main libretro API implementation
- `libretro_game.cpp` - Game wrapper implementing pinball_game interface
- `libretro_sdl_compat.h` - SDL/ImGui compatibility stubs
- `options_libretro.cpp` - Simplified options (no ImGui)
- `high_score_libretro.cpp` - Simplified high scores (no ImGui)
- `winmain_libretro.cpp` - Window management stubs

### Rendering
The core uses OpenGL 2.1 for hardware-accelerated rendering:
1. Game renders to internal framebuffer
2. Framebuffer uploaded to OpenGL texture
3. Texture rendered to libretro FBO

### Limitations
- Save states not implemented
- Audio uses silence (SDL_mixer replaced with stubs)
- No ImGui menus (options dialog, high scores dialog)

## License

Same license as the original SpaceCadetPinball project.
