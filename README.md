# CannonCruise

Source code of the CannonCruisePC game rebuilt by reverse engineering.
The original 2004 32-bit Windows game (`CannonCruise.exe`) is being rewritten
in C++14 based on Ghidra and IDA analyses (`CC.asm`, `CC_ida.asm`).

**Status:** Active development — Phase 4 (build/link) verified: macOS/Clang
`CannonCruiseLib` + link verification target; Windows Win32 build with VS 18
(2026) and ctest 33/33 passed (NULL and D3D9 platforms). In D3D9, actual
rendering of the game with a real window was visually verified (island + ocean +
VIEW camera afterward); GLOBAL.RWS contents were resolved and the LOADING
screen works with the original art (parchment, wave border, Hugo).

## Directory layout

| Path | Contents |
|---|---|
| `code/` | Rewritten source code (game + framework) |
| `code/framework/` | RenderWare Studio framework core (event/attribute/stream/memory) |
| `code/havok/` | Havok Physics 2.2.1 stub headers (Phase 4.3) |
| `code/rwstubs.cpp` | RenderWare SDK link stubs (Phase 4.5) |
| `code/build/` | CMake outputs (not tracked) |
| `CC.asm` / `CC_ida.asm` | Ghidra / IDA reverse engineering dumps (reference) |
| `tests/` | SDK-independent unit tests (e.g. Havok allocator) |
| `tools/`, `validation/` | Interface audit scripts and validation JSON outputs |
| `DLLs/`, `Streams/`, `Sound/`, `Localiza/`, `Setup/` | Original game files (reference/assets) |

## Build

### macOS (verified)

```bash
cd code
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
./build/CannonCruise        # link verification entry — should print "link verification OK"
```

- The `CannonCruiseLib` static library + `CannonCruise` link verification target are produced.
- The target links all archive objects with `-Wl,-force_load`; a missing symbol
  produces a link error. Also, the `CannonCruise` target links only
  dynamically against system libraries (game SDKs are not linked yet).

### Windows (validated — VS 18 2026)

```bat
cd code
cmake -B build -G "Visual Studio 18 2026" -A Win32 -DCANNONCRUISE_RW_PLATFORM=NULL
cmake --build build --config Release --parallel 8
ctest --test-dir build -C Release --output-on-failure
```

- `CANNONCRUISE_RW_PLATFORM`: `NULL` (headless) | `D3D9` (windowed) | `GL3`.
- D3D9: real-time windowed rendering — Pull-and-write: world assets +
  GLOBAL + LOADING visible from the first frames after loading
  (`CANNONCRUISE_CAM=FOLLOW|FREE|AUTO|DATA`). Screenshot support:
  `CANNONCRUISE_SCREENSHOT=<file.png>` (CAMERATEXTURE RT → systemmem
  surface → PNG; `EndFrame` before swap). For a loading-screen frame use
  `CANNONCRUISE_SCREENSHOT_LOADING=<file.png>`, for a game world frame use
  `CANNONCRUISE_SCREENSHOT`.
- The loading screen is fixed at **30 fps** (`std::chrono::steady_clock` +
  `sleep_for`); frame count is `CANNONCRUISE_LOADING_FRAMES` (default 100;
  `0` = skip). Esc/Space/Enter early exit.
- **Caution — tracked build outputs:** `code/build_win32_v180/` ships with the
  repo and contains a stale CMakeCache (generated with the VS 17 2022 generator,
  D3D9); it cannot be used in place with VS 18. Clean path: use a new build
  directory (`cmake -B build_win32 ...`) and delete it when done; or the
  `CMakeCache.txt`/`CMakeFiles/`/`*.vcxproj*` inside `build_win32_v180/` can be
  deleted and reconfigured (the tracked outputs remain in git).
- D3D9 note: the D3D9 screenshot path that reads the backbuffer via
  `GetRenderTargetData` was moved to the `CAMERATEXTURE` raster (scene → camera
  texture RT → systemmem surface → PNG); `EndFrame` copies the camera RT to the
  backbuffer with `StretchRect` before `showRaster` — otherwise the window stays
  black (backbuffer SWAPEFFECT_DISCARD).
- `build_cannoncruise.cmd` contains the VS 18 recipe and works with the D3D9 platform;
  the `build_win32/` and `build_d3d9/` directories are in gitignore (safe to delete).

## Documents

- [ROADMAP_NEXT.md](ROADMAP_NEXT.md) — current roadmap (where we are + this week)
- [ROADMAP.md](ROADMAP.md) — historical phase log (Phases 1–4.6ab)
- [ROADMAP_MACOS.md](ROADMAP_MACOS.md) — macOS port roadmap
- [BUILDING.md](BUILDING.md) — Windows/MSVC build recipe and dependency inventory
- [SDK_INTERFACE_AUDIT.md](SDK_INTERFACE_AUDIT.md) — RenderWare/Havok interface audit
- [LICENSE](LICENSE) — GPL-3.0

## Tests

SDK-independent tests live under `tests/`. All run with CMake/ctest:

```bash
cd code
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
ctest --test-dir build --output-on-failure
```

Coverage: `havok_memory_manager` (allocator), `ceventhandler` (RegisterMsg/
LinkMsg/SendMsg/lifecycle), `cclassfactory` (Register/Create/Shutdown),
`cattributehandler` (instance registry + packet predictors),
`worldcycle_test` (VIKING01/IDUNA03/BEAVER01 load→clump→unload stress,
Phase 4.6s), `fmvscene_test` (FMV transition graph, 8 identifiers),
`globalassets_test` (GLOBAL.RWS inventory: embedded txd + external resources),
`globaldump_test` (GLOBAL embedded txd → PNG dump). The whole package
runs with **ctest 33/33**. You can also compile a single test with a
sanitizer manually:

```bash
c++ -std=c++14 -fsanitize=address,undefined \
    tests/havok_memory_manager_test.cpp code/HavokMemoryManager.cpp \
    -Icode -o /tmp/havok_test && /tmp/havok_test
```

## Architecture notes

- **RenderWare 3.6 / RenderWare Studio:** local stub headers (`code/rwcore.h`
  etc.) provide declarations; the real SDK libraries are not linked. Game code
  is written against RenderWare types (`RpClump*`, `RwFrame*` etc.).
- **Havok Physics 2.2.1:** the `code/havok/` stub headers match the binary naming
  (`hkWorld`, `hkRigidBody` — no `hkp` prefix). `CPhysicsWorld` /
  `CPhysicsEntity` currently use `void*`; awaiting SDK integration.
- **Framework:** `RWS::CEventHandler`, `CAttributeHandler`, `CClassFactory`
  mimic RenderWare Studio behavior; `FUN_*` binary addresses are preserved in
  code comments.

## Warning

This project involves reverse engineering; the original game is protected by copyright.
The original game files in the repository are for analysis/reference purposes only.
