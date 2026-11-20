# CannonCruise

[![build](https://github.com/mnkhprre/CannonCruise/actions/workflows/build.yml/badge.svg)](https://github.com/mnkhprre/CannonCruise/actions/workflows/build.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

A C++14 reimplementation of **Hugo: Cannon Cruise** (ITE Media, 2004, Windows),
rebuilt from disassembly of the original 32-bit executable. The game's
RenderWare 3.6 / RenderWare Studio code is rewritten against
[librw](https://github.com/aap/librw) and its Havok physics against
[Bullet](https://github.com/bulletphysics/bullet3).

> **No game data is included.** You need your own copy of the original game;
> the port reads its `Streams/`, `Video/`, `Sound/` and `Localiza/` folders.

## Status

Work in progress — one mission can be played from start to finish.

| Area | State |
|---|---|
| Boot flow | GLOBAL → intro video (Bink) → main menu → map menu → level, same order as the original |
| Levels | All 15 levels (Viking, Beaver, Iduna, Nordic) load and run |
| Player ship | Original `Hugo_Ship` model at its level start position; physics via Bullet |
| Towers | Original `Viking_firetower` models placed from level data; damaged model when destroyed |
| Mission | VIKING01: destroying the six mission towers ends the level with a summary screen |
| Object data | Level records are parsed into attribute packets; models are resolved by GUID |
| Missing | Enemy ship AI, mines and pickups, tutorial text boxes, camera modes (binoculars, cannon view), sky, returning to the map after a level |

The full gap list is in [GAP_ANALYSIS.md](GAP_ANALYSIS.md).

## Building (Windows)

Requirements: Visual Studio 2022 or 2026 with the C++ workload, CMake 3.16+,
Git (bullet3 and librw are fetched by CMake). The game is 32-bit, so build
for **Win32**.

```bat
cd code
cmake -B build -A Win32 -DCANNONCRUISE_RW_PLATFORM=D3D9
cmake --build build --config Release --parallel
```

`CANNONCRUISE_RW_PLATFORM` selects the renderer: `D3D9` (window),
`NULL` (headless, used for tests) or `GL3`. [BUILDING.md](BUILDING.md) has the
detailed recipe and dependency list.

## Running

Copy `binkw32.dll` from your game install next to `CannonCruise.exe` and start
it **from the game folder** (the one that contains `Streams/`):

```bat
cd C:\Games\CannonCruise
path\to\code\build\Release\CannonCruise.exe
path\to\code\build\Release\CannonCruise.exe Streams\VIKING01.RWS
```

Without arguments the full boot runs (intro video, menus); with a level file
it starts that level directly.

| Key | Action |
|---|---|
| W / S | Throttle / reverse |
| A / D | Rudder |
| Space | Fire cannon |
| Mouse | Orbit the camera |
| Esc | Pause menu |
| Enter / arrows | Menus |
| F5 / F6 | Debug save / load |

## Tests

```bat
cd code
cmake -B build-null -A Win32 -DCANNONCRUISE_RW_PLATFORM=NULL ^
      -DCANNONCRUISE_DATA_ROOT=C:/Games/CannonCruise
cmake --build build-null --config Release --parallel
ctest --test-dir build-null -C Release --output-on-failure
```

38 tests; 21 of them read the original game files from
`CANNONCRUISE_DATA_ROOT`. Without the data those are reported as *Not Run*
and the other 17 must pass — that is what [CI](.github/workflows/build.yml)
runs.

### Debug switches

Environment variables read by `CannonCruise.exe`, useful for headless runs:

| Variable | Effect |
|---|---|
| `CANNONCRUISE_SHIP=x,y,z,yaw` | Override the player start |
| `CANNONCRUISE_DRIVE=throttle,rudder` | Fixed steering input |
| `CANNONCRUISE_AUTOFIRE=N` | Fire every N frames |
| `CANNONCRUISE_AUTOAIM=1` | Aim at the nearest mission tower |
| `CANNONCRUISE_SCREENSHOT=file.png` | Save a frame (`CANNONCRUISE_SHOT_FRAME=n` picks which) |
| `CANNONCRUISE_FRAMES=n` | Quit after n frames |
| `CANNONCRUISE_LOADING_FRAMES=0` | Skip the loading screen |
| `CANNONCRUISE_STREAMS=dir` | Folder that contains `Streams/` |

Example — play VIKING01 to the end without input:
`CANNONCRUISE_SHIP=25,0,-150,0 CANNONCRUISE_AUTOFIRE=40 CANNONCRUISE_AUTOAIM=1`.

## Repository layout

| Path | Contents |
|---|---|
| `code/` | Game code and the CMake project |
| `code/framework/` | RenderWare Studio framework (events, attributes, class factory, streams) |
| `code/havok/` | Havok 2.2.1 interface headers, implemented on Bullet |
| `tests/` | ctest suite |
| `docs/` | Class audit, binary-vs-port gap analysis, Havok→Bullet notes |
| `tools/` | Analysis and audio extraction scripts (vgmstream) |
| `mapping/`, `validation/` | Address map and SDK interface audit outputs |

Disassembly listings (`CC.asm`, `CC_ida.asm`) and the original executable are
not in the repository.

## How it works

- **Level files** (`Streams/*.RWS`) are RenderWare Studio containers: asset
  records (textures, models, animations, collision) and object records. Each
  object record is passed to its class as an attribute packet; models are
  looked up by the GUIDs in its `CSystemCommands` block.
- **Rendering** goes through librw (D3D9 or GL3); `rw_librw.cpp` adapts the
  RenderWare 3.6 C API the game code was written against.
- **Physics**: `CPhysicsWorld` / `CPhysicsEntity` keep the original Havok-style
  interface; `PhysicsBackend.cpp` implements it with Bullet.
- Original function addresses (`FUN_*`, `sub_*`) are kept in comments next to
  the reimplemented code.

## Documents

- [GAP_ANALYSIS.md](GAP_ANALYSIS.md) — what works and what is missing
- [ROADMAP_NEXT.md](ROADMAP_NEXT.md) — current roadmap
- [ROADMAP.md](ROADMAP.md) — phase history
- [BUILDING.md](BUILDING.md) — build recipe and dependencies
- [docs/GAP_ANALYSIS.md](docs/GAP_ANALYSIS.md) — binary vs port class comparison
- [SDK_INTERFACE_AUDIT.md](SDK_INTERFACE_AUDIT.md) — RenderWare/Havok interface audit

## Legal

This is an unofficial fan project for preservation and study. Hugo and
Hugo: Cannon Cruise are trademarks of their respective owners; the original
game and its assets are copyrighted and are not distributed here. The code in
this repository is licensed under the [GPL-3.0](LICENSE).
