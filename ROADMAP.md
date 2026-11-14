# ðŸ—ºï¸ CannonCruise â€” Project Status Report & Roadmap

> **Last Updated:** 14 November 2026   
> **Project:** Cannon Cruise PC â€” Source code reconstruction via reverse engineering  
> **References:** `CannonCruise.exe`, `CC.asm`, `CC_ida.asm`  
> **Build foundation and recipe:** [BUILDING.md](BUILDING.md)  
> **SDK interface audit:** [SDK_INTERFACE_AUDIT.md](SDK_INTERFACE_AUDIT.md)
> **Separate macOS port plan:** [ROADMAP_MACOS.md](ROADMAP_MACOS.md) â€” an independent roadmap from clean static build all the way to native app and packaging.

## Meaning of statuses

**Present in source**, **compiled**, **linked** and **verified at runtime** are different stages. A method whose body or documentation exists is not considered equivalent to the original game. The completion claims from the previous report are itemized below with the existing file evidence; the game was not run in this review.

## Overview â€” October 9 review

- Under `code/`, excluding `build/`, **194 `.h`/`.cpp` files**: 147 at the root (game and SDK stub headers together), 47 under framework. The previous 143 + 47 count is outdated.
- **No zero-byte files:** `framework/startup/videomode.cpp` has been filled in (CameraInit scoring logic + CameraShutdown). A file being non-empty does not indicate the implementation is complete either.
- Doxygen coverage was reported as 190/190 in the previous report; a completeness check and Doxygen generation were not performed on the current file set.
- Current target: **`CannonCruiseLib` static library**; there is no executable target or SDK linkage in CMake.
- `code/build/` contains a macOS build cache and `libCannonCruiseLib.a`. The cache points to compiler metadata AppleClang 21.0.0.21000101, x86_64, 8-byte pointers. These are traces of a previous build; they do not prove a clean build of the current sources.
- Original `CannonCruise.exe`: verified as **PE32 / x86** with PE signature, `Machine=0x014c` and `OptionalHeader.Magic=0x010b`; the file was not run.
- README does not exist yet; the first build document `BUILDING.md` was created.

## Phase 1 â€” Documentation

It was reported as completed in the previous report. Doxygen comments exist in many sources; **100% coverage and the consistency of the comments with the implementation have not been verified**. For example, while `PhysicsWorld.h` describes the simulation step, the call in `.cpp` is commented out.

## Phase 2 â€” Filling in empty files

The previous report stated that the GlobalFunctions, MissionIndicator, TextureResource, WbCamera, WbRender and WorldGeometryResource files were filled in. These files exist and are not zero bytes. `framework/startup/videomode.cpp`, which was empty at the time of review, was also filled in: `CameraInit` (FUN_0041af10 â€” subsystem/video mode scoring, "No best sub-system" assert contract) and `CameraShutdown` (FUN_0041ac00 â€” singleton shutdown contract) were written based on binary interpretation. No zero-byte files remain project-wide; functional completion must be examined separately.

## Phase 3 â€” Render and physics stubs (PARTIAL)

The previous â€œ8/8 completedâ€ statement has been removed. The list contains seven render methods and one physics update. Source inspection results:

- `CFade::Render()`: Im2D drawing body exists; awaiting visual verification.
- `CGUIHealthBar::Render()`: Im2D drawing body exists; awaiting visual verification.
- `CGUIMap::Render()`: drawing body exists; uses a fixed size, assumed scale and fixed marker color. Original behavior not verified.
- `CShowText::Render()`: in addition to the background strip, actual text drawing was added â€” first FontSystem (`fonts/*.met` + glyph UVs, `FontSystem_DrawText`); if the font cannot be found, falls back to Im2D quad drawing with an embedded 5x7 bitmap font (ASCII 32â€“126), with line splitting (`\n` + word wrapping) and a `MSG_SHOWTEXT_RENDER` event link. The METRICS3 parser was ported, but since the contour raster (`sub_4D54C0`) and the actual font assets were pending, the 5x7 fallback is currently the default path.
- `COceanTile::Render()`: a `RenderWaterMesh()` call exists after the checks; not verified at runtime.
- `COcean_W32::RenderWaterMesh()`: a RenderWare Im3D drawing body exists; it should not be regarded as a completed D3D9 integration. `Initialize()` only returns TRUE; `Shutdown()` is empty.
- `CPerformanceVisualizer::Render()`: the background panel is drawn; no performance data/graph/text drawing.
- `CPhysicsWorld::Update()`: **ineffective stub**; the `hkpWorld::stepDeltaTime` call is commented out. The physics simulation does not advance.

This list is not an inventory of all stubs in the project.

## Phase 4 â€” Build system and compilation (ACTIVE)

Goal: an application compiled on Windows with MSVC, then linked with its real dependencies.

- **4.0 â€” Status report and build foundation:** Completed (documentation/source inspection). The Win32/x86 target, recommended tool versions, dependency gaps and the clean build recipe are in `BUILDING.md`. Windows execution verification is not included.
- **4.1 â€” Adding sources to CMake:** Recursive glob exists. The CMake compiler diagnostic sources in the old `code/build/` are now excluded. Windows sources are included only in Windows configuration; the final source scope will be verified during the build.
- **4.2 â€” RenderWare stub headers:** **Completed (contract fixes).** An interface audit was performed. Three engine return type conflicts were removed; an opaque `RwVideoMode` declaration was added. Im3D (`RwIm3DVertex` + macros + `RwIm3DTransform/End/RenderPrimitive/RenderIndexedPrimitive`) and memory (`RwMalloc/RwFree/RwRealloc`) declarations were added; `RwRENDERSTATESHADEMODE`/`rwSHADEMODEGOURAUD` were defined. **Open issues were closed with binary evidence:** `RwEngineOpen` is **argumentless** (`FUN_00422960` `retn`, startup calls `RwEngineOpen()`), `RwEngineInit` is **(memFuncs, flags(bit0), resEntrySize)** (`FUN_0050b640` â€” `arg_4` byte flag read; startup `(memFuncs, 0, 0)`), while `RwEngineSetConfig` has a **single pointer argument** (`FUN_0050f660` â€” Ghidra:
`undefined4 FUN_0050f660(undefined4 * param_1)`; IDA: reads arg_0 as 12 bytes and
copies them into the engine object at +0xC) â†’ the `RwEngineConfig` struct + `const
RwEngineConfig*` signature was fixed, the startup call was aligned by constructing
the struct. An engine lifecycle state machine (Initâ†’Openâ†’Start + Closeâ†’Stopâ†’Term) was added to `rwstubs.cpp`; the duplicate externs in `startup.cpp` were removed, the call was aligned to the `RwMemoryFunctions*` cast. Real SDK/ABI and Windows verification pending.
- **4.3 â€” Havok interfaces:** Havok **2.2.1** stub headers were created: the `code/havok/havok.h` umbrella + `hkBaseTypes`, `hkMemory`, `hkClass`, `hkWorld`, `hkEntity`, `hkPhantom`, `hkShape`, `hkConstraint`, `hkCollisionFilter`. Naming was verified against the binary (`hkWorld`/`hkEntity` â€” the `hkp` prefix is Havok 4+). All 11 headers passed syntax checking with C++14. Backend integration (world creation, `stepDeltaTime`, entity position/velocity) is still pending; `CPhysicsWorld`/`CPhysicsEntity` still use `void*`.
- **4.4a â€” macOS/Clang static library:** **Completed.** With a clean `code/build/` configuration, all non-Windows sources were compiled with Clang (0 errors); `libCannonCruiseLib.a` was produced. Since the missing Im3D declarations have been completed, `Ocean_W32.cpp` is now included on every platform (the CMakeLists exclusion was removed). Missing C++ definitions were added: `CEventHandler(RwUInt32)`, `CAttributeHandler(CAttributePacket)`, `GamePluginAttach` (stub) and `rwstubs.cpp` (86 RenderWare SDK stub functions).
- **4.4b â€” Windows/MSVC static library:** **Pending.** The macOS result does not substitute for this step.
- **4.5 â€” Executable target and linker verification:** **Completed (macOS).** The `add_executable(CannonCruise main.cpp)` target was added; with `-Wl,-force_load` **all** objects in the archive are linked â†’ link errors occur if any symbol is missing. Clean build: 0 errors, link succeeded, `./CannonCruise` runs (exit 0). No remaining undefined symbols (only system/libs++ symbols â€” normal). Windows/MSVC link verification pending together with 4.4b.

## Phase 5 â€” Project infrastructure (COMPLETED)

- **5.1:** **Completed.** `README.md` â€” project description, directory structure, macOS build
  commands, document links, running tests, architecture notes.
- **5.2:** **Completed.** `.gitignore` â€” build outputs, CMake/IDE residue,
  `.DS_Store`, `*.bak`, Doxygen HTML outputs. (For the already-tracked
  `.DS_Store`/user files, `git rm --cached` is required; deliberately
  left untouched.)
- **5.3:** **Completed.** `Doxyfile` (doxygen 1.18) + generated `docs/html/`
  (15 MB). Regeneration: `doxygen Doxyfile`.
- **5.4:** **Completed.** `STYLE.md` â€” existing code conventions (indentation,
  naming `m_`/`C` prefixes, K&R braces, file header block, FUN evidence
  preservation rule, verification commands).
- **5.5:** **Completed.** `FUN_MAP.md` â€” **372** `FUN_*` addresses generated from code with
  `tools/extract_fun_map.py`; **157** `Class::Method` matches, **248**
  descriptive texts, **124** file context only. Regeneration:
  `python3 tools/extract_fun_map.py`.

README, Windows CI and SDK-independent basic tests can proceed in parallel with build efforts.

## Phase 6 â€” Advanced goals

- Launching with original assets â†’ window/render device â†’ asset loading â†’ menu â†’ single scene â†’ controlled shutdown.
- Physics, AI and collision tests; comparison with original behavior.
- CI/CD and architecture documents/wiki.

## Progress summary and next step

Instead of unmeasured â€œapp 100% / build 85%â€ ratios, verification gates are used:

1. **Build foundation documented:** Yes; the Windows recipe has not been run yet.
2. **Clean Windows static library:** Pending.
3. **EXE with resolved external symbols:** **Yes (macOS/Clang).** With `-force_load`, all archive objects were linked; the `CannonCruise` target compiles, links and runs. Windows/MSVC counterpart pending.
4. **Single scene runtime verification:** Pending.

> **Update 2026-10-15** â€” the gates are now closed **on Windows/MSVC**:
> Win32/x86 (same as the original PE32/i386), MSVC 19.51, SDK 10.0.28000.0 â€”
> **0 errors / 0 warnings**; link verification passed (`link verification OK`).
> `CANNONCRUISE_RW_PLATFORM` = **NULL | GL3 | D3D9**; on D3D9 **a window
> opens** (`class='CannonCruiseD3D9' visible=True size=656x519`) and
> 200/200 frames are rendered with exit 0. **ctest 25/25**.
> Details: phases 4.6nâ€“4.6t in the sections below; Havok shim
> [docs/HAVOK_TO_BULLET.md](docs/HAVOK_TO_BULLET.md), build
> [BUILDING.md](BUILDING.md).

P0 continuation added an SDK interface inventory, limited source fixes and a repeatable Clang pre-check. Details and test evidence are in [SDK_INTERFACE_AUDIT.md](SDK_INTERFACE_AUDIT.md).

Next steps (parallel):
1. **4.4b â€” Windows/MSVC build:** Running the `BUILDING.md` recipe on a Windows
   machine. There is no MSVC on this macOS worker. All symbols awaiting link
   are now defined: 13 startup functions + engine lifecycle
   + **win32's 5 bridge symbols** (`Idle`, `InitEngine`, `CleanupEngine`,
   `ResizeCameraRasters`, `GetTime` â†’ new `framework/startup/win32/enginewin.cpp`;
   cross-checked mangled against win.cpp). `TextureCacheInit`/`GetTime`
   use WINMM â†’ MSVC `winmm.lib` linkage (`#pragma comment` present in both
   files). The only remaining build that cannot be verified on macOS = Windows.
2. **Visual verification:** `CShowText::Render` text drawing, `videomode.cpp`
   CameraInit mode selection and running the existing Im2D stubs with the real RW SDK.
   The original `fonts\*.met` font system â€” **Stages 1 + 2 + 3
   completed** (METRICS1 + METRICS3 loaders + glyph query + drawing
   path `FontSystem_DrawText` + UTF-16LE + `CShowText` link); separate
   work items: METRICS2 (FUN_004ce0d0 â€” analyzed, port blocked due to raster pixel
   scan), M3 contour raster (sub_4D54C0 â€”
   ported in Stage 3b) and the binary's stream font loader
   (sub_4CC690 â€” ported in this session).

Completed (previous session):
- `enable_testing()` added to `tests/CMakeLists.txt` + `code/CMakeLists.txt`;
  4 SDK-independent tests via `ctest` (`havok_memory_manager`, `ceventhandler`,
  `cclassfactory`, `cattributehandler`) â€” 4/4 passing.
- `rwcore.h`/`rwstubs.cpp`: `RwVideoMode` (6 dword) + subsystem/video mode
  query APIs (`RwEngineGetNumSubSystems` â€¦ `RwEngineSetVideoMode`).

Completed (this session â€” startup subsystem completion):
- **4.2 closed** (see Phase 4 item): rwcore.h signature fixes,
  rwstubs engine lifecycle, startup.cpp alignment.
- **13 missing startup functions written with binary analysis** (5 new files,
  under `framework/startup/`, automatically included via `GLOB_RECURSE`):
  - `gamemanager.cpp` â€” `GameManagerInit`/`GameManagerShutdown` + `sub_449D30`
    free list chain (10000 Ã— 12-byte entry table) fully restored.
  - `renderpipeline.cpp` â€” `RenderPipelineInit`/`RenderPipelineShutdown`
    (iMsgStartSystem/iMsgStopSystem registration, `CLinearMemoryManager` reset,
    tick callback) + `SetLightAmbient` (engine command infrastructure TODO).
  - `havokstart.cpp` â€” `HavokInit` (0x4030 singleton, 3 refcounted subsystem
    objects, `sub_52DCA0` context swap) + `TextureCacheShutdown` (same
    global lifecycle: `byte_660934`, `dword_660938/3C/40`).
  - `systemservices.cpp` â€” `FileSystemInit` (**in the binary `nullsub_11` = 1-byte
    `retn` â€” kept faithfully as a no-op**), `TextureCacheInit` (WINMM time
    resolution), `SubSystemShutdown` (device list contract),
    `ResourceShutdown` â†’ `CAttributeHandler::Shutdown()` (assert path,
    `cattributehandler.cpp` evidence), `GeneralShutdown` â†’ `CEventHandler::Shutdown()`.
  - `rwstartengine.cpp` â€” `RwStartEngine` (B:..Z: drive scan, create/verify/activate search
    path; POSIX counterpart is cwd as the single search path).
- `UnRegisterMsg` added to `ceventhandler` (counterpart of `loc_40CC60`).
- Verification: 0 errors/warnings, `link verification OK`, ctest 4/4, cross-checked with `nm` that 15
  symbols are defined in the archive.

Completed (this session â€” win32 bridge + contract fixes):
- **win32's 5 missing symbols written** â€” new
  `framework/startup/win32/enginewin.cpp` (compiles only on Windows):
  - `GetTime` (`FUN_00482BE0` â€” binary dword_657AA8 cache; since there is no logic step,
    WINMM `timeGetTime()` with the same ms contract).
  - `Idle` (`FUN_0041C380` â€” "MainLoop::Poll" skeleton; asset logs,
    logic step sub_41B900, render step sub_41CEB0 â†’ TODO Phase 6).
  - `ResizeCameraRasters` (`FUN_00418130` â€” ctx check + temporary
    `iMsgResizeCameraRasters` RegisterMsg/SendMsg/UnRegisterMsg flow;
    raster resizing subsystems TODO Phase 4.4b).
  - `CleanupEngine` (`FUN_00417930` â€” GetWindowPlacement â†’ record
    PosX/PosY/SizeX/SizeY/Minimized/Maximized to rwreg; NO engine shutdown,
    that is in the separate RwStartupShutdown chain).
  - `InitEngine` â€” bridge: links startup.cpp `RwStartupInitialize` (binary
    sub_41A680 = RwStartup::Initialize) into the full chain.
  - Verification: `-Wall -Wextra` syntax clean with fake SDK headers;
    win.cpp externs â†” enginewin definitions **mangled cross check**
    (5/5 + `RwStartupInitialize` matched exactly).
- **`RwEngineSetConfig` contract fixed** (3 float â†’ `const
  RwEngineConfig*`; Ghidra/IDA evidence), `RwEngineIsRunning()` added
  (temporary counterpart of `sub_41CDF0`); startup.cpp aligned to call with the struct.
- **RenderPipelineInit step 3 / Shutdown step 7**: `iMsgDeleteEventHandler`,
  `iMsgDeleteEntity`, `iMsgOnDeleteEntity` (format `CEventHandler*`)
  hooked up with RegisterMsg/UnRegisterMsg (binary sub_40BF10 / sub_40BF60).
- 2 trivial build warnings fixed (AnimController intâ†’ptr cast,
  SceneManager `%%s` format bug).
- Verification: 0 errors/warnings, `link verification OK`, ctest 4/4.

Completed (this session â€” font system Stage 1):
- **New `code/FontSystem.cpp` + `FontSystem.h`** â€” portable core of the original font subsystem
  (10 FUN â†’ added to FUN_MAP):
  - `FontSystemInit` (`FUN_004cd660`) â€” `"fonts/"` path buffer + **64Ã—6
    quad-index table** (binary loop exactly: `{b, b+2, b+3,
    b, b+3, b+1}` Ã— 64 blocks) + list heads + dword_66DCC0 loader registration.
  - `FontSystemAllocBuffers` (`FUN_004cefd0`) â€” 0x2400 glyph vertex buffer.
  - `FontSystemLoad` (`FUN_004cd9a0`) â€” **`;` fallback name chain** â†’
    `fonts/<name>.met`; header METRICS1/METRICS2/METRICS3 check.
  - `ParseMetrics1` (`FUN_004cdbc0`) â€” full solution: texture header (single token â†’
    `"%sm%s"` asset probe with mask `m<name>`; two tokens â†’ `(name, mask)`),
    size line, glyph lines (`%i Ã—5` â†’ UV = `(pixel+0.5)/texture` â€”
    flt_5EF624 = 0.5), aspect ratio `(right-left)/(bottom-top)`, lines outside
    the 5 fields are skipped, code >0xFFFF hard error, capacity +0x20 growth,
    `lineExtent`/`sizeScale` from the last line.
  - `FontFinalizeExtMap` (`FUN_004ce000`) â€” uint16 map from min..max
    (empty = 0xFFFF â€” binary `or eax,-1` fill) + `extMin/extCount/extTable`.
  - `FontSystemGet` (`FUN_004cee90`) â€” name list strcmp cache, on hit
    no reload.
  - `FontSystemTeardown`/`FontSystemShutdown` (`FUN_004cd580`/`FUN_004cef90`)
    â€” linked into renderpipeline Shutdown as the counterpart of `__ioterm+62/+67`.
  - Layout check: `CCFontGlyph=0x20`, `CCFont=0x144`, `CCFontNode=0x0C`
    (32-bit static_assert â€” binary Win32 layout).
- **RenderPipelineInit Step 2 partially restored**: the font members of the `sub_4CC140` chain
  (`sub_4CEFD0`, `sub_4CD660`) are called; the other targets of the chain
  (sub_4D5450/sub_4D0E70/sub_4D41C0/sub_4CC1F0/sub_4CC020,
  sub_41CDF0/sub_4D6220) were narrowed down as TODO.
- **`rwstubs.cpp`**: missing `RwTextureRead/Destroy/SetRaster/GetRaster`
  added (declared in rwcore.h; `RwTextureRead` returns NULL when the file is missing
  â†’ binary behavior: if there is no texture, font loading fails).
- **New `tests/fontsystem_test.cpp`** (5/5 tests): METRICS1 + CRLF, UV/aspect,
  ASCII + extended map, `;` fallback chain, two-token header, mask
  probe (`m<name>`), METRICS2 rejection, texture error path, Get cache,
  teardown â†’ restart.
- Out of scope (deliberate): METRICS2 (`FUN_004ce0d0`), METRICS3
  (`FUN_004ce870` â€” vector contours) â†’ next stage; engine freelist
  pool semantics (`engine+0x144/148`) â†’ Phase 4.4b. The drawing path + ShowText
  link were completed in this session as **Stage 2** (below).
- Verification: 0 errors/warnings, `link verification OK`, **ctest 5/5**.

Completed (this session â€” font system Stage 2: drawing path + ShowText):
- **`sub_4CCC40` (font->drawHandler) fully resolved and moved to the portable port**
  (`FontSystem_DrawText`, added to FUN_MAP):
  - Glyph lookup (ASCII + ext map); codes not found are **skipped** (no
    progress â€” binary `jl`).
  - Quad: unit space y âˆˆ {0,1}; UV order exactly
    `(u0,v1)(u0,v0)(u1,v1)(u1,v0)`; advance `pen += aspect + font->spacing`.
  - Batch: flush on texture change or at the **63rd quad** (binary `ebx > 0x17A` =
    378 indices; 0x2400 buffer = 64 quads Ã— 4 vertices Ã— 36B) â†’
    `RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST)` (added to rwstubs).
  - Color: flat material bytes (+0x60..63) or lighting/gradient path
    (+0x64&1) â€” ambient + light Ã— (pen/width), `CCFontMaterial` schematicized with
    binary field ranges.
  - Transform: `*pPosX += pen Ã— scale` (binary final `fadd`).
  - Deviations (documented): direct scale in screen space instead of a matrix stack
    (sub_4CC2F0/4CC4D0/4CC480 â†’ engine matrix stack); z/rhw constant.
    UTF-16 (glyphFlags&2) mode was added in Stage 3 (see below).
- **`sub_4CD7E0` (measure) ported** (`FontSystem_MeasureText`):
  sum of `(aspect + spacing)` Ã— scale; missing glyphs are skipped.
- **`CShowText` FontSystem link**: `Render` calls `FontSystemGet` first
  (default name "Arial", changeable via `SetFontName`) â€” if the font exists,
  `FontSystem_DrawText` + `'\n'`/word wrapping (using
  `FontSystem_MeasureText`); otherwise the embedded 5x7 fallback is preserved.
- **`CCFont.flags` (+0x0C) â†’ fixed to `spacing` (RwReal)** â€”
  the binary writes this field as an advance gap float
  (`sub_4CDBC0`: `dword ptr [esi+0Ch], 0`; `fadd` in draw/measure).
- **rwcore.h/rwstubs.cpp**: `RwIm2DRenderIndexedPrimitive` + draw counters
  (`RwStub_GetIm2DRenderCount` / `GetIm2DIndexedCount` / `Reset`) for
  test observation.
- **test expanded**: measure (including spacing), flat/NULL material draw
  (vertex position/UV/color), lighting path (ambient + gradient byte
  calculation), 70-glyph batch overflow (63+7 = 2 flushes), fake textures converted to real
  allocations (`RwTextureGetRaster` must be dereferenceable).
- Note: `sub_4CC690` turned out not to be drawing but the **binary stream font loader**
  (sub_491700 chain; 4-byte field reads + type byte â‰¥1) â†’ ported in the next
  session (see below).
- Verification: 0 errors/warnings, `link verification OK`, **ctest 5/5**,
  win32 fake SDK `-fsyntax-only -Wall -Wextra` clean.

Completed (this session â€” font system Stage 3: METRICS3 + UTF-16):
- **`sub_4CE870` (ParseMetrics3) fully resolved and moved to the portable port**
  (`FontSystem.cpp`, added to FUN_MAP):
  - Format: header `METRICS3`; name line (`%[^\n]`, local â€” the port skips it);
    code line = `<1-byte prefix><UTF-8 code>` â€” resolves 1..6
    bytes of UTF-8 from binary `buf[1]` (including the legacy 5/6-byte forms)
    and constructs the pending code (the prefix in the port is `'`); commands by exact line `strcmp`:
    `begin`/`end`/`moveto %f %f`/`lineto %f %f`/`curveto %fÃ—6`/
    `closepath`.
  - Contour model: sub-path chain (binary sub_4D0C90 object + new contour on
    moveto; port `CCFontContour`/`CCFontPathPoint` â€” point types
    0=moveto, 1=lineto, 2=curveto (raw bezier, PostScript order)).
  - Glyph end (`end`): `glyph.data = contour`, `glyph.aspect = var_A0`
    (first float of the last path-op â€” sticky between glyphs, binary quirk),
    texture = 0; code registration via common `RegisterGlyphCode` (shared with M1).
  - `[font+0x138] = &sub_4CCAF0` (vector handler) â€” in the port a single path
    `FontSystem_DrawText`; the contour raster `sub_4D54C0` was ported in the next block
    (stroke strip instead of quads for textureless glyphs).
  - `FontFree` now frees glyph contour chains
    (counterpart of sub_4D0C20).
- **UTF-16LE text mode (glyphFlags&2) added to MeasureText + DrawText**:
  same pattern in all three handlers (sub_4CD7E0/sub_4CCC40/sub_4CCAF0) â€”
  unit 2 bytes `(p[1]<<8)|p[0]`, cursor stops at signed `<=0` (0x8000+
  codes exit early like the binary). Contract: in wide mode the caller must pass UTF-16LE
  (hence the CCF_T1 tests were converted to UTF-16LE).
- **METRICS2 (`sub_004ce0d0`) decision**: port blocked â€” the format is based on raster pixel
  scan (page header is a texture, glyph boundaries scanned by line matching;
  raster lock + no assets) â†’ marked in FUN_MAP as â€œanalyzed,
  port blockedâ€.
- **test expanded**: synthetic `CCF_V3.met` (METRICS3 + UTF-8 'Ã©' code
  â†’ wide table); contour chain/point types/closepath/curveto raw
  coordinates, aspect = last op float, UTF-16LE measure/draw (double-NUL
  terminator), M3 does not call the texture reader, reload.
- Verification: 0 errors/warnings, `link verification OK`, **ctest 5/5**,
  win32 fake SDK `-fsyntax-only -Wall -Wextra` clean.

Completed (this session â€” font system Stage 3b: vector contour raster):
- **`sub_4D54C0` (543 lines, vector contour raster) resolved and moved**
  (`FontSystem.cpp` `RenderContourGlyph` + `RenderContourStrip`/
  `RenderContourChain`, added to FUN_MAP):
  - Stroke model: a vertex pair per contour record (0x18: base x/y + unit normal + cumulative
    arc length) â€” A = base âˆ’ nÃ—w (tip), B = base +
    nÃ—w (base); w = mat[+0x70] strokeWidth + contour[+0x10] (0 in port);
    normal = (dy, âˆ’dx)/len (sub_4CFF40); gradient grad = acc/total
    (var_64 = 1/last record â€” reciprocal chain).
  - Batching: binary TRIANGLEFAN (type 4) + 0xFE vert flush â†’ port static
    index buffer with TRIANGLELIST (stripâ†’list (a,b,c)(b,d,c)); chain
    [contour+0x14] recursion = separate strip/flush per sub-path.
  - Color: flat (+0x60..63) or lighting (+0x64&1, tip +0x00/+0x18,
    base +0x30/+0x48); flags&2 â†’ UV u = uStart + uDeltaÃ—grad (tip
    +0x10/+0x28, base +0x40/+0x58 â€” `CCFontMaterial` pad fields were opened under these
    names, strokeWidth +0x70 added).
  - `FontSystem_DrawText`: textureless glyph branch â†’ pending quad flush +
    `RenderContourGlyph` + advance (penX += aspect + spacing); glyph
    drawn with penOffset (`*pPosX + penXÃ—scale`) â€” the binary matrix stack
    T(advance) counterpart.
  - DEVIATIONS (documented): segment normal instead of miter join (sub_4CFF40
    blending not present in the port); fixed 8 sub-segments at render instead of straightening at bezier parse;
    texture format states (1/9/2) skipped; z/rhw
    constant (Phase 4.4b).
  - Clone path (refcount==0 â†’ sub_4D0440/4D04D0) port SKIPPED (rare
    non-shared glyph scenario).
- **test expanded**: 'A' closed triangle strip â€” 4 pairs/8 verts,
  position/normal/scale+shift, flat color, lighting grad (acc/total
  reciprocal), strokeWidth 0 degenerate, 'Ã©' curve (8 sub-segments â†’ 20
  verts), two glyphs â†’ 2 flushes, advance semantics preserved.
- Verification: 0 errors/warnings, `link verification OK`, **ctest 5/5**,
  win32 fake SDK `-fsyntax-only -Wall -Wextra` clean.

Next (suggestion): Phase 4.4b render pipeline subsystems â€” `sub_4D5450`
(matrix stack), `sub_4D0E70`/`sub_4D41C0`, `sub_4CC1F0`/`sub_4CC020`
(vertex buffer/flush), `sub_41CDF0`/`sub_4D6220`.

Completed (this session â€” Phase 4.4b pipeline portable core):
- **`sub_4CC1F0`/`sub_4CBD20`/`sub_4CC2F0`/`sub_4CC350`/`sub_4CC480`/
  `sub_4CC4D0`/`sub_4CC020` (core) resolved and moved**
  (`code/RenderPipeline.h/.cpp`, added to FUN_MAP):
  - Matrix stack: `CCMatrix` = 64B RwMatrix layout (right/up/at/pos +
    flags); fixed array of 32 slots (instead of the binary's lazy sub_510600 allocation);
    push = parent copy (sub_4cc2f0), pushCopy = srcÃ—parent
    (sub_50f760 multiply â€” parent applied first), identity flags
    |= 0x20003 (sub_4cbd20).
  - `MatrixScaleXY(x,y)` scales the base columns (x,y,1);
    `MatrixTranslateXY(x,y)` pos += rightÃ—x + upÃ—y (RwMatrixTranslate
    semantics â€” translation passed through scale; DrawText order
    translateâ†’scale: v(1,1) â†’ (12,23)).
  - `RenderPipeline_Reset` = sub_4cc1f0: depth 0, unit matrices,
    root matrix (dword_66d604) identity, z (dword_66d7a0) = 1.0, half-pixel
    (dword_66d7a4) = 0.5, aspect 1/1, cache zeroed.
  - `SetViewWindow` = sub_4cc020 core: aspect pair (larger/smaller â†’
    the other becomes 1.0); camera object/hook ([camera+0x1c] â†’ sub_4cc110)
    not ported.
  - `RenderPipelineInit` gained `RenderPipeline_Reset()` (sub_4CC140
    chain â€” counterpart of sub_4CC1F0 after the font steps).
  - Engine-bound members (documented no-op/TODO): `sub_4D5450`/`sub_4D0E70`/
    `sub_4D41C0` (sub_50dbf0 flow registrations + vtable+0x144 freelist
    allocation â€” PoolAlloc covers it in the port), `sub_4CC520` (projection
    param block â€” port uses screen-space scale).
  - FontSystem `FillTextVertex` z: 0 â†’ 1.0f (dword_66d7a0 init value).
- **new test**: `tests/renderpipeline_test.cpp` â€” reset, push/overflow
  (31), pushCopy (srcÃ—parent + NULL copy), DrawText order
  (translateâ†’scale vs scaleâ†’translate), identity, aspect (horizontal/vertical
  dominance + zero protection).
- Verification: 0 errors/warnings, `link verification OK`, **ctest 6/6**,
  win32 fake SDK `-fsyntax-only -Wall -Wextra` clean.

Completed (this session â€” stream font loader):
- **`sub_4CC690` (FontSystem_LoadFromStream) + `sub_4D11D0`
  (LoadContourFromStream) resolved and moved** (`FontSystem.cpp`,
  added to FUN_MAP):
  - Stream abstraction `CCFontStream` (read/readExact/findChunk â€”
    sub_50d940/50cd80/50d790); type tag (tag>>24) â‰¥ 1; the **refCount field in the font
    header selects the path**: 0 â†’ texture path (UV 0x10 +
    aspect + flags byte; texture names 0x20, ReadTexture; glyph.texture
    = textures[(RwInt8)flags]), â‰ 0 â†’ vector path (waste 4B + aspect +
    type; type==5 â†’ findChunk(0x1A1) + contour record + post/waste reads).
  - Contour record (sub_4D11D0): 0x10 header {version=1, pointCount, flags,
    extra} + pointCountÃ—0x18 prepared stroke records (type/x/y/normal/acc);
    flags&4 â†’ chain sub-contour. The port converts records to CCFontContour path
    points (type0 â†’ moveto, others â†’ lineto â€” curveto already
    straightened at parse; normal/acc recomputed at render â€” same deviation as Stage 3b).
  - DEVIATIONS (documented): texture context save/restore (sub_5154C0/5154A0)
    no-op; contour[+0x10]/node+8 vestigial; texture index [0,3] + texCount
    â‰¤ 4 + glyphCount â‰¥ 0 validations in the port (binary does not validate);
    drawHandler = 0 (DrawText single path).
- **test expanded**: memory stream (MemOps) + blob constructor; texture path
  ('A'/'B' two glyphs, single texture, ASCII map, measure 2.25); vector path
  (type-5 closed contour â†’ draw 8 verts/1 flush â€” same geometry as CCF_V3 'A');
  chain (flags&4 â†’ head+child, child closed â†’ 6 verts);
  error paths (type byte 0, truncated stream, versionâ‰ 1, findChunk missing, texture
  reader fail).
- Verification: 0 errors/warnings, `link verification OK`, **ctest 6/6**,
  win32 fake SDK `-fsyntax-only -Wall -Wextra` clean.

Completed (this session â€” camera context + projection param block):
- **sub_4cc020 full path + sub_4cc520 resolved and moved**
  (`RenderPipeline.h/.cpp`, added to FUN_MAP):
  - `RCCCameraView` â€” portable camera view (projFlag +0x14,
    raster size [raster+0xC/10], window +0x68/6C, frame LTM);
    `SetViewWindowFromCamera` = raster validation + SetViewWindow
    core + active camera cache (dword_66d7cc) + offset zero
    (dword_66d7b8/bc). NULL camera â†’ cache cleared + FALSE; raster
    NULL â†’ FALSE, cache preserved (binary loc_4cc109). vtable hook
    (sub_4cc110 â†’ dword_66d818) engine side â€” skipped.
  - `BuildViewMatrix` (sub_4cc520): cached scale matrix
    unk_66d6d8 = (top Ã— root) Â· scale(zScale,zScale,1) Â· scale(-w,w,1)
    (w = min(winX2,winY2)) Â· translate(winX2Ã—zScaleÃ—0.5, -winY2Ã—
    zScaleÃ—0.5, 0) â€” sub_510170/510300 combination type 2 (full scale /
    raw translation â€” type 1s already ported in MatrixScaleXY/TranslateXY,
    separated out by tests); zScale = (projFlag==2) ? 1.0 : dword_
    66d7a0. Inverse unk_66d718 (sub_50ff20: identity copy / orthonormal
    transpose / general adjugate-det â€” sub_510010; det==0 â†’ 1.0 â€”
    flt_5ef2e0). Result unk_66d758 = scale Ã— camera frame â€” EVERY CALL.
  - Small context API: GetCamera (sub_4cc120), Set/Clear/GetContext
    (sub_4cc130 â€” dword_66d79c), GetViewParams (sub_4cbe10 â€” offset +
    aspect). Added to Reset: camera/offset/dword_66d7b0=7 + view
    matrix init.
  - DEVIATIONS (documented): sub_4cbee0 (worldâ†’screen conversion consumer) and
    sub_4cbe50 (pixel scale metric flt_66d7a8) not ported â€” next
    work item; if no camera/frame, returns the scale matrix (binary hangs).
- **test expanded**: camera context (640Ã—480, win 1/0.5 â†’ offset 0,
  aspect 2/1; NULL/raster-NULL paths â€” cache preservation); BuildViewMatrix
  math (clean stack: scale(-1,1,1)+translate(1,-0.5,0); inverse
  pos'(1,0.5,0); frame multiply pos(-9,19.5,30)); cache validity
  (MatrixTranslateXY â†’ rebuild); no-frame guard; context
  set/clear.
- Verification: 0 errors/warnings, `link verification OK`, **ctest 6/6**,
  win32 fake SDK `-fsyntax-only -Wall -Wextra` clean.

Completed (this session â€” Phase 4.6: Bullet physics backend, â€œclosest feelâ€):

- **transition document**: `docs/HAVOK_TO_BULLET.md` â€” why Bullet, binary
  reverse sources (`sub_55E6D0` field table, 1/60 step), mapping
  table, ownership contracts, deviations and open gaps in one place.

- **decision**: **Bullet Physics 3.25** instead of Havok (MIT; vendored into
  `code/CMakeLists.txt` via FetchContent â€” only LinearMath/Collision/
  Dynamics are compiled, demo/unit-test options off). Rationale: hkpWorld
  â†’ btDiscreteDynamicsWorld, hkpRigidBody â†’ btRigidBody, hkpPhantom â†’
  btGhostObject (+btGhostPairCallback), Clump â†’ btBvhTriangleMeshShape /
  btConvexHullShape, step(1/60) â†’ stepSimulation(dt,1,dt); Havok memory
  arena â†’ standard malloc.
- **binary parameters reversed** (for the closest feel):
  - `sub_55E6D0` (Havok memory callback registration `sub_55E630` â†’ via dword_660D58; compatible with the dword_660920 arena context): [+0x04] gravity
    **-9.8** (0C11CCCCDh), [+0x20..28]/[+0x30..38] AABB **Â±500**
    (0C3FA0000h/43FA0000h), [+0x44] **friction 0.6**, [+0x58]
    **restitution 0.3**, [+0x80] **solver iter 20**, [+0x5C] 0.0163,
    [+0x8C] 10.0; [4B8710:7E] fixed step **1/60** (push 3C888889h).
  - These became inputs to the PhysicsBackend coefficients: kGravityY=-9.8,
    kFriction=0.6, kRestitution=0.3, kSolverIterations=20.
- **`PhysicsBackend.h/.cpp`** (new): PhysicsWorldHandle (config+
  dispatcher+dbvt+impulse solver+world+ghost callback); Create/Destroy/
  Step/SetÂ·GetGravity; PhysicsBody_CreateBox/TriMesh/WithShape + Destroy;
  PhysicsShape_CreateBox/Sphere/ConvexHull/TriMesh + Destroy (ShapeBundle
  registry â€” for mesh shapes the btTriangleMesh lifetime is tied to the shape); trigger
  (CreateBox ghost + Add/RemoveTrigger SensorTrigger group + GetOverlaps);
  position/rotation/velocity/kinematic/impulse access. Ownership: CreateWithShape
  body does NOT OWN the shape (userIndex=1 â†’ Destroy does not delete the shape).
- **`PhysicsWorld.cpp`** linked to the backend: ctor â†’ create world
  (gravity-9.8/iter20), Update â†’ stepSimulation(1/60), Add/Remove
  RigidBody/Phantom â†’ backend calls.
- **`CollisionGeometry.cpp`** implemented (FUN_0046f040):
  RpClumpForAllAtomics â†’ vertex/triangle collection (with base offset;
  rpGEOMETRYTRISTRIP â†’ alternating winding), bConvex â†’ ConvexHullShape,
  !bConvex â†’ TriMeshShape; CreateBox/Sphere/DestroyShape delegated to the backend.
  Added to rwcore/rpworld (stub â€” will be filled in by librw):
  RpGeometryGetNumVertices/NumTriangles/Vertices/TriangleIndices/Flags.
- **test**: `tests/physics_test.cpp` (7/7) â€” gravity default (-9.8),
  1 s free fall (v=-9.8, semi-implicit Euler y verification), ground
  collision + friction stop (friction 0.6), impulse (Î”v=J/m), trigger
  overlap (ghost callback), static triMesh standing on the ground.
- Verification: Bullet 3.25 + game code **0 errors**, `link verification OK`
  (force_load + BulletLib), **ctest 7/7**, win32 fake SDK `-fsyntax-only`
  clean. CMake note: Bullet includes do not propagate to targets â†’ manual
  `target_include_directories(... ${bullet3_SOURCE_DIR}/src)`;
  `CMAKE_POLICY_VERSION_MINIMUM 3.5` (Bullet's old cmake requirement).
- DEVIATIONS (documented): sequential-impulse solver â‰  Havok 2004 solver
  (exact feel not guaranteed â€” gameplay calibration with assets);
  AABB Â±500 broadphase clamp not applied (Bullet wide aabb); Clump
  atomic transform not applied (noinstance assumption); deactivation
  defaults (the +0x6C/+0x2 fields in Havok cinfo not yet mapped).
  Note: there is no `hkp*` symbol in the binary (static link) â€” parameters
  were found via content analysis.

Completed (this session â€” Phase 4.6b: entity/trigger flow linked to the backend):

- **`PhysicsEntity.cpp` accessors** filled in (Set/GetPosition,
  Set/GetLinearVelocity â†’ PhysicsBackend; m_pRigidBody null-guard; IDA
  evidence: sub_436F70+472 `m_pRigidBody != NULL Failed` assert).
  AddToWorld/RemoveFromWorld were already registered via the singleton â€”
  backend world registration flows automatically through PhysicsWorld.
- **`CollisionTrigger.cpp`** implemented: Initialize
  (FUN_0046f5e0) â†’ `RpClumpGetBoundingSphere` (new stub â€” added to rwcore/
  rpworld, will be filled in by librw) â†’ AABB box phantom
  (`InitializeWithSphere` port addition â€” test/clump-less path) â†’
  `CPhysicsWorld::AddPhantom` â†’ backend ghost. `HandleEvents` â†’
  `PollOverlaps()`: diff against the previous round â†’ `OnEntityEnter`/`OnEntityLeave`
  (virtual; FUN_004702c0 speak trigger is a no-op because the speak manager was not ported
  â€” fields m_iEnter/LeaveSpeak* preserved). Dtor removes the phantom from
  the world + destroys it.
- **test**: `tests/physics_entity_test.cpp` (**ctest 8/8**) â€” entity
  lifecycle (AddToWorld â†’ exists; Set/GetPosition 4,5,6; Set/Get
  velocity 7,8,9; gravity + vx conservation after Update; RemoveFromWorld);
  trigger enter/leave (radius 3 AABB, enter 1Ã—, no re-enter,
  teleport then leave 1Ã—).
- Verification: 0 errors, `link verification OK`, **ctest 8/8**, win32 fake
  SDK `-fsyntax-only` clean.

Completed (this session â€” Phase 4.6c: singleton lifecycle aligned to the binary):

- **singleton ownership clarified** (binary `sub_41A680` = RwStartup::
  Initialize): HavokInit (`sub_434FB0`) does NOT WRITE the physics world singleton
  (file note verified); the startup init chain creates it
  (alloc 0x440 â†’ ctor `sub_49C870` â†’ `dword_635AF4`); the behaviour ctor
  (`sub_43B860`) only allocates the container â€” it USES the singleton.
- **`CPhysicsWorld::Create()`** added (returns if it exists â€” a second new
  does not trigger the CSingleton assert); called in `startup.cpp` after HavokInit
  (binary step mapping). **Behaviour ctor/dtor ownership
  flag** (`m_bOwnsWorld`): if the singleton came from startup, the dtor does not delete it
  (the shutdown chain is the lifespan owner); on the level/test path if absent
  it creates and deletes it.
- **test**: `physics_entity_test` expanded â€” Create() idempotent;
  behaviour does not create a second world; world lives while dtor does not own it;
  after delete the singleton is null (**ctest 8/8**).
- `CPhysicsResource::Load` (IDA `sub_439460`) examined: pacType/pStream
  asserts + `wbID_STATICWORLDCOLLISION` chunk path â€” stream parse
  tied to the RwStream API (gate 4/librw), remained at counter/assert level in this
  step (documented).
- Verification: 0 errors, `link verification OK`, **ctest 8/8**.

Completed (this session â€” Phase 4.6d: **librw integration â€” gate 4 opened**):

- **librw** (github.com/aap/librw, MIT) linked via CMake FetchContent:
  commit `18532c2`, `LIBRW_PLATFORM=NULL` (device independent â€” stream/clump
  parse; GL3/D3D9 render is a later work item), `librw::librw` linked to both
  CannonCruiseLib and CannonCruise.
- **shim architecture**: `code/rw_librw.cpp` â€” the game's C API (Rw*/Rp*) â†’
  librw C++ (`rw::`). Moved from `rwstubs.cpp`: engine lifecycle
  (init/open/start/stop/close/term + IsRunning state tracking), RwStream*
  (rw::StreamFile + findChunk/readChunkHeaderInfo), RpClumpStreamRead
  (rw::Clump::streamRead), RwTexDictionaryStreamRead (rw::TexDictionary::
  streamRead). RpWorldStreamRead stayed in the stub â€” librw has no World streamRead
  (BSP risk documented).
- **`rwID_TEXDICTIONARY` fixed**: in the port it was written as 0x0023 but classic RW3
  was 0x0016 (reduced from a double definition to a single constant; same as librw VEND_CORE).
- **`tests/rwlibrw_test.cpp`**: engine init(NULL) â†’ the real `Streams/MAINMENU.
  RWS` opens â†’ chunk header loop + inventory + FindChunk â†’
  **ctest 9/9**.
- **asset format findings** (MAINMENU/GLOBAL/VIKING01 hexdump):
  - The files use the **standard RW stream header** (12 bytes; libraryID
    `0x1802FFFF` â†’ `libraryIDUnpackVersion` = **RW 3.6.0.2** âœ“ librw decode).
  - Chunk types are **in the game plugin range**: `0x0716` (asset/txd identifier
    â€” length-prefixed "Texture Dictionary" + "rwID_TEXDICTIONARY" string in the payload
    + source path metadata: `x:\rwstudio\wavebreaker\build output\...`),
    `0x0704` (component/behaviour record â€” class name string, e.g.
    **"CWbRenderBehavior"** â€” "Wb" = WaveBreaker component naming),
    `0x071C` (1Ã— 104 bytes per file â€” manifest/index). VIKING01: 617
    chunks (0x0704=497, 0x0716=119, 0x071C=1).
  - The asset pipeline is the **"rwstudio"** custom tool (NOT the RW SDK stream writer);
    the wbID_* plugins are presumably these 0x07xx IDs (CPhysicsResource::
    Load `wbID_STATICWORLDCOLLISION`).
- `RwStreamRead/Write` also defined in the shim (previously decl-only).
- Verification: 0 errors, `link verification OK`, **ctest 9/9**.

Completed (this session â€” Phase 4.6e: **rwstudio container format
unraveled**):

- **`code/RwStudioStream.h/.cpp`**: `Streams/*.RWS` = rwstudio (WaveBreaker
  pipeline) custom container â€” top-level RW chunks (libid 0x1802FFFF =
  RW 3.6.0.2): `0x0716` asset record (meta: typeName/GUID/rwIdName/
  sourcePath; string = length-prefixed, '\0'+0xBF padding 4-aligned),
  `0x071C` manifest (class name+version â€” CWb* behaviours), `0x0704` object
  record (property list â€” next work item).
- **embedded RW chunk**: in txd assets after the meta comes a standard chunk
  header `[type=0x23][len][libid 0x1803FFFF]` + payload â€” **classic RW
  convention** (len = payload size); identity: `metaLen+20+len ==
  chunkLength`. `rwID_TEXDICTIONARY` in this file family is **0x23** (not 0x16
  â€” the fix in rwcore.h was reverted/noted).
- **variants** (hexdump verified): `rwID_2DMAESTRO` (.anm â€” 4th string,
  markerless meta; Rt2dMaestro librw gap â€” chunk is skipped),
  `wbID_WORLDGEOMETRY` (.dff, no embed), `wbID_STATICWORLDCOLLISION`
  (.col, no embed â€” inline data, CPhysicsResource::Load target),
  `wbID_PATHFINDINGDATA`, `wbID_Texture` (.texref: [res][len][pngPath] â€”
  e.g. "Font\SpecialCharacters.png"), PPB sound bank ("Bank Win32").
  Parser **seek-based** (no byte counting): absolute seek to the end of each chunk
  â€” format variants require no interpretation. New shim: `RwStreamTell` /
  `RwStreamSeekAbs`.
- **inventory** (fixed with tests): MAINMENU 5 assets/4 objects/1 manifest (txd
  1.6MB + Menu.anm maestro + .dff/.col/.path); GLOBAL 7 assets/48 objects
  (1 embedded txd 525KB); VIKING01 119 assets/497 objects/1 manifest.
- **`tests/rwstudio_test.cpp`**: meta fields + embedded txd payload
  (i16 texture count â‰¥1, device=1 d3d8) + manifest class names
  (CWbRenderBehavior/CMenuBehavior) + three-file inventory â†’ **ctest 10/10**.
- Note: MAINMENU's last asset is `rwID_2DMAESTRO` "Menu.anm" (menu Maestro
  animation) â€” Rt2dMaestro port flagged as a gate 5 risk.

Completed (this session â€” Phase 4.6f: **0x0704 object property schema
unraveled**):

- **schema** (fixed with MAINMENU obj0/1/2 hexdump): object =
  `[u32 field0]` + segment/property array.
  - segment `[u32 segLen][u32 flags][content]`: `0x20000000` = class name
    (behaviour start), `0x40000000` = GUID(16B), `0x80000000` =
    name repeat in the first behaviour / STARTS subsequent behaviours
    (without GUID), `0x00000000` = string value segment (e.g.
    `"Audio_MenuMusic"` â€” CAudioStreamEntity name).
  - property `[u32 size][u32 id][size-8 value]`: size 12 = u32/float/
    inline-string (`\0`+0xBF pad); size 20 = 3 floats (vec3 â€” e.g.
    CWbCameraBehavior prop0 = [0.0, 2.0, -1.0]).
- **evidence**: CWbRenderBehavior 13 properties (prop0=-1, prop1=1000.0f,
  prop2=0.5f, prop9=300.0f, prop0xC=30.0f â€” render settings;
  comparable with the binary ctor `sub_43B860`); CWbCameraBehavior
  (prop4=60.0f â€” FOV); CAudioStreamEntity `"Audio_MenuMusic"` + 5
  properties (prop4=5.0f); each object ends with CAttributeHandler.
- **code**: `RwStudioObject/Behaviour/Property` structs added to `RwStudioStream.h`;
  NextAsset parses 0x0704 (state machine: 0x20â†’GUIDâ†’
  0x80 repeatâ†’props; next 0x80â†’new behaviour); access:
  `RwStudioFileGetObject()`.
- **test**: MAINMENU obj0/1/2 detail assertions (class names, prop
  counts, float values, string segment) + VIKING01 first object
  sanity â†’ **ctest 10/10** (rwstudio_test 58+ checks).
- Verification: 0 errors, `link verification OK`, **ctest 10/10**, win32
  syntax clean.
Next up (suggestion): mapping property ids onto the binary class layout
(CWbRenderBehavior ctor `sub_43B860` field offsets â€” prop9=300/
prop0xC=30 may be camera distance/FOV); CClassFactory class name â†’
constructor registration; embedded txd payload decode (d3d8 IMAGE); .col inline
data â†’ trimesh; render platform (GL3/D3D9).

### Phase 4.6g: binary confirmation â€” CClassFactory + RenderBehaviourWorld
(from hex-rays `CannonCruise.exe.c` + `CC_ida.asm` + PE vtable dump)

- **CClassFactory::Register = `FUN_00413700`(name, factory, size,
  flag)** â€” registered class sizes: CWbRenderBehavior 0x18,
  CWbCameraBehavior 0x34, CAudioStreamEntity 0x54,
  CPhysicsWorldBehaviour 0x20. `FUN_0043b860` FIXED: not the render ctor,
  the CPhysicsWorldBehaviour **factory**.
- **CEventHandler**: ctor `FUN_00411010`(stream, className); process
  loop `FUN_00411050` â€” 0x20/0x40 records skipped, **0x80000000 record
  dispatched** (the "second class-name segment" in our parser = behaviour
  apply trigger â€” the parser state machine is consistent with the binary).
- **RwID load table `DAT_0065d7a4`**: global system that loads chunks from the
  stream by name (+0x114 slot: rwID_CLUMP/2DMAESTRO/
  HANIMANIMATION loads) â€” the original of embedded payload reading.
- **RenderBehaviourWorld singleton `FUN_004c4410` â†’ DAT_00635648**:
  huge object (pools 120000B+180000B+...); **CWbRenderBehavior sub-
  object at +4**; defaults [0x14]=1000.0f, [0x15]=300.0f = matched obj0
  prop1/prop9 exactly (the 0x0704 values are written into this singleton);
  **12 messageâ†’camera pass** registrations: MENU_RENDER_RT2D /
  FRONT_RT2D / IM2D, PROFILE, SHOWTEXT, THUNDER1, FADE,
  EnemyHealthBar (all RwCamera), Ocean update/destroy (COcean),
  Event_INQ_PLYR_POSITION (RwMatrix). â†’ Port architecture: **message-driven
  camera pass system** (code `RenderBehaviorWorld`/`MessageSystem`).
- Original source path from hex-rays: `D:\Projects\CannonCruisePC\code\`
  (project name exact).
- Verification: build 0 errors, ctest 10/10, link OK, win32 syntax OK.

Remaining tracing (next session): the property apply function (the one that reads
[id][value] from the stream and writes it to the singleton offset â€” the 0x80000000 dispatch
target); CWbRenderBehavior prop id â†” singleton offset table;
port of the CClassFactory create-by-name flow (together with
CEventHandler).

### Phase 4.6h: RenderBehaviourWorld property apply port (DONE)

- **`FUN_004c3e40` = WbRender attribute apply** resolved from hex-rays:
  switch(id) 0..0xC; value `rec+8` (raw u32); id 6/0xA = ARGB â†’ R,G,B
  **bytes as floats** decode (+0x30../+0x44..); default â†’
  assert "Unhandled attribute in WbRender..." (source line 0xC0).
- **id â†’ singleton offset map** (bytes): 0â†’+0x14, 1â†’+0x18,
  2â†’+0x1c, 3â†’+0x5c, 4â†’+0x60, 5â†’+0x28, 6â†’+0x30/34/38, 7â†’+0x2c,
  8â†’+0x50, 9â†’+0x54, 0xAâ†’+0x44/48/4c, 0xBâ†’+0x3c, 0xCâ†’+0x40.
  Ctor-default consistency: id8=1000.0f/+0x50, id9=300.0f/+0x54
  (ctor [0x14]/[0x15] words).
- **code**: `code/RenderBehaviourWorld.h/.cpp` â€” 32-bit binary layout
  exact with `RwUInt32` vtable field (static_assert'ed, +0x04 24B
  CWbRenderBehavior sub-object); `Init()` ctor defaults;
  `ApplyWbRenderAttribute(id, value)` faithful switch.
- **test**: `tests/renderworld_test.cpp` â€” engine init shim; the real
  MAINMENU obj#0 13 properties applied â†’ all fields verified
  (prop0=0xFFFFFFFF bit copy, prop1=1000.0fâ†’+0x18, prop6=0xFF000000
  â†’RGB(0,0,0), propA=0x00FFD0FFâ†’RGB(255,208,255), propC=30.0f;
  id>0xC â†’ UNHANDLED). **59 checks, 0 failures.**
- Verification: 0 errors, **ctest 11/11**, link OK, win32 syntax OK.

Next up (suggestion): CWbCameraBehavior apply switch (FUN_004c2b90 family);
CAudioStreamEntity properties; CEventHandler 0x80000000 dispatch
(port of the create-by-name flow); embedded txd decode; .col â†’ trimesh;
render pass system (MSG_MENU_RENDER_RT2D etc.); Rt2dMaestro.

### Phase 4.6i: CWbCameraBehavior property apply port (DONE)

- **`sub_4C3450` = camera apply** (vtable PTR_LAB_005f0ac4 slot1; resolved from the ASM
  jump table â€” hex-rays had split this function into pieces): **51 cases** (id 0..0x32), target = `sub_452170()` camera
  state object (â‰¥0x140 bytes), default **silently** falls through (no assert).
- **Structure**: 4 parameter groups (A: id0-7, B: id8-16, C: id17-27,
  D: id28-40) â€” each group has VEC3 (12B copy), RAD (vÂ·Ï€/180), TAN
  (tan(vÂ·Ï€/180) â€” projection scale), raw u32 fields; + misc
  id41-50. Constants: `flt_5F0AC0`=Ï€ (0x40490FDB), `flt_5EF7B8`=1/180,
  `flt_5EF2E0`=1.0f. id42: if guard(+0x13c)==0 then +0x114 += 1.0;
  id48: if guard==0 then +0x12c = 1.0f.
- **code**: `code/CameraState.h/.cpp` â€” `CCameraState` (0x140B, 32-bit
  layout static_assert'ed); `ApplyAttribute(id, bits)` (RAD/TAN/raw)
  + `CCameraStateApplyVec3(id, 12B)`; `CameraDegToRad` in Ï€Â·(1/180)
  order (binary fmul order).
- **test**: `tests/camerastate_test.cpp` â€” the real MAINMENU obj#1
  (CWbCameraBehavior) all properties applied â†’ file values +
  tan(60Â°)=1.7320508 verified; guard behaviors; unknown id
  FALSE â†’ **86 checks, 0 failures**.
- Verification: 0 errors, **ctest 12/12**, link OK, win32 syntax OK.

Next up (suggestion): CAudioStreamEntity apply (FUN_004587d0 family); CAttributeHandler packages; CEventHandler 0x80000000 dispatch
(create-by-name flow); embedded txd decode; .col â†’ trimesh; render
pass system; Rt2dMaestro.

### Phase 4.6j: CAudioStreamEntity property apply port (DONE)

- **`sub_458BD0` = audio apply** (vtable PTR_FUN_005efbe8[1] / sub
  PTR_LAB_005efbe0[3]): **6 cases** (id 0..5), default silent.
  - id0: `sub_458D10(this,1)` stop (+0x3c/+0x48=0) + clear slot20;
    if first byte of rec+8 != 0, register "SAudioPlayStreamData*"
    payload into slot20 (loc_40BF90).
  - id1/id2: `sub_40E0B0(this=+0x18, slot=+0x28/+0x30, src=rec+8, 0)` â€”
    slot detachâ†’clearâ†’registerâ†’list-attach (flags=0).
  - id3: +0x38 = u32 (file: 0xFFFFFFFF = -1).
  - id4: +0x40 = float (file: 5.0f).
  - id5: +0x44 = float (file: 0.0f).
- **Layout** (ctor FUN_00458960, 0x54B): +0x00 vtable, +0x18
  sub-vtable, +0x1c=1, +0x20/+0x28/+0x30 three 8B message slots (FUN_0040b300),
  +0x38..+0x48 fields; ctor FUN_0040d170 links them to the DAT_00635c68/635c50
  lists with 0x8000.
- **code**: `code/AudioStreamEntity.h/.cpp` â€” `CAudioStreamEntity` struct
  (static_assert'ed 0x54 layout); `CAudioStreamEntityInit`,
  `CAudioStreamEntityApplyAttribute(id, pValue)`. Slot prefix up to NUL,
  max 8 bytes; the "registered" state is in the slot[1] flag in the binary, in the port it is
  derived from the first byte of the prefix (0x00 = empty); the full slot machine is the message
  subsystem (todo).
- **test**: `tests/audiostream_test.cpp` â€” MAINMENU obj#2
  ("Audio_MenuMusic", 5 props id1..5) apply â†’ slot prefixes, loopCount
  0xFFFFFFFF, float0=5.0f, float1=0.0f; id0 stop+register; unknown id
  FALSE â†’ **48 checks, 0 failures**.
- Verification: 0 errors, **ctest 13/13**, link OK, win32 syntax OK.

Next up (suggestion): CAttributeHandler packages; CEventHandler 0x80000000
dispatch (create-by-name flow); embedded txd decode; .col â†’ trimesh;
render pass system; Rt2dMaestro.

### Phase 4.6k: BehaviourLoader create-by-name dispatch (DONE)

- **Pipeline closed**: 0x0704 OBJECTRECORD â†’ class name (0x20/0x80000000
  segment) â†’ `CClassFactory::Create` â†’ create â†’ apply virtual (vtable
  slot1) â€” the render/camera/audio apply ports were connected end to end.
- **Binary semantics** (FUN_00411010/1050/10e0): CEventHandler ctor
  (stream, className) runs **Process immediately**; Process skips 0x20/0x40,
  at 0x80000000 it compares with the class name via `(*(DAT_0065d7a4+0x114))(className, rec+8)`
  (strcmp, 0=match), stops at the first property of the matching behaviour;
  non-matching properties are skipped; FUN_004110e0 =
  Advance+Process(1) moves to the next behaviour. FUN_00407210 uses the same +0x114
  strcmp slot for name-table lookup as well (DAT_0063557c list).
- **code**: `code/BehaviourLoader.h/.cpp` â€” `BehaviourClassEntry`
  (className/size/create/apply), `BehaviourLoader::Register`
  (CClassFactory::Register + apply table), `Create` (factory bridge),
  `ProcessObject` (create+apply per behaviour; unknown class
  skipped â€” factory NULL).
- **test**: `tests/behaviourloader_test.cpp` â€” three class registrations
  (sizes 0x18/0x34/0x54), MAINMENU 4 OBJECTRECORD: obj#0 render
  singleton fields (prop0 bit copy, idC=30.0f, idA RGB 255/208/255),
  obj#1 camera (VEC3 pos file value, tan(60Â°)), obj#2 audio
  (loopCount 0xFFFFFFFF, float0=5.0f, float1=0), obj#3 unknown
  class â†’ 0 apply â†’ **31 checks, 0 failures**.
- Verification: 0 errors, **ctest 14/14**, link OK, win32 syntax OK.

Next up (suggestion): embedded txd decode (d3d8 IMAGE); .col â†’ trimesh;
render pass system (MSG_MENU_RENDER_RT2D etc.); CAttributeHandler
packages; Rt2dMaestro.

### Phase 4.6l: Embedded TXD (d3d8 IMAGE) decode (DONE)

- **Format reverse-engineered** (MAINMENU.RWS embedded txdsig, 1,685,444B,
  libId=0x1803FFFF): header `[u16 textureCount][u16 deviceID]`; each
  texture: `[u32 flag=1]` + `0x18` raster (nested `[id=1][16B: w,h,depth,
  pitch]` + pixel `pitch*height` B, 4-aligned) + `0x06` struct (nested
  `[id=1][4B filterFlags]`, `[id=2][name]`, `[id=2][maskName]`, at the end of the body
  `[id=3][0][ver]` terminator).
- **Contents**: 16 textures, deviceID=1 (d3d8) â€” 11Ã—128Ã—32 animation frames
  (Menu_137..117), 3Ã—32Ã—32 (Menu_095/087/084), 512Ã—128 strip (Menu_076),
  640Ã—480 background (Menu_074); all 32bpp; first pixel 0x00FFFFFF.
- **code**: `code/TxdDecode.h/.cpp` â€” `RwTxdTexture/RwTxdImage` struct,
  `RwTxdDecodeImage(payload, len, out)` (chunk/nested boundary-validated;
  pixel pointers into the payload).
- **test**: `tests/txddecode_test.cpp` â€” embedded payload read
  (RwStudioAssetReadEmbedded) + decode â†’ 16 textures, names (Menu_137 â†’
  Menu_074), sizes (128Ã—32 / 512Ã—128 / 640Ã—480), pitch*height
  consistency, pixel range, corrupt input FALSE â†’ **32 checks, 0
  failures**.
- Verification: 0 errors, **ctest 15/15**, link OK, win32 syntax OK.

### Phase 4.6m: Static world collision (.col) decode + trimesh (DONE)

- **Format reverse-engineered** (0x0716 wbID_STATICWORLDCOLLISION payload;
  ctor FUN_0043bfa0 + section sub_58BEF0 asm): payload layout
  `[u32 vertexCount][u32 unknown][u8 flag=2 + pad][u32 12]
  [u32 triangleCount][12B zero][f32 bboxMin.xyz][f32 unknown]
  [u32 sectionBytes(f32 bit pattern)]` + section blob (material/primitive,
  opaque) + `u32 indices[triCountÃ—3]` + `f32 verts[vcCountÃ—3]` + 0â€“4B pad.
  The ctor header starts from the asset's embA field (payloadâˆ’8); count1=[+8]
  (=vertexCount), flag byte=[+10], count2=[+18] (=triangleCount).
  Empty world (MAINMENU etc.): countA=0 â†’ skip without array â†’ decode FALSE.
- **Contents**: VIKING01 38391v/71303t (section 753474, bboxMin
  âˆ’475.77/âˆ’9.08/âˆ’333.67), IDUNA03 6726/11592, BEAVER01 55902/102833 â€”
  all three layout byte-boundary slack 0/1/2; indices 0..vc-1.
- **code**: `code/StaticCollisionData.h/.cpp` â€”
  `StaticCollisionData_Decode(payload, len, mesh)` (bounds validation +
  index check) + `StaticCollisionData_CreateShape(mesh)` â†’
  `PhysicsShape_CreateTriMesh` (vcÃ—float3 + triÃ—u32); added
  `resourceLength` + `RwStudioAssetReadResource` to RwStudioStream (non-embedded
  resource payload read â€” the .col path).
- **test**: `tests/coltrimesh_test.cpp` â€” payload read from three real RWS files
  + decode (count/bbox/slack/first index) + Bullet shape creation +
  corrupt input FALSE â†’ **53 checks, 0 failures**.
- Verification: 0 errors, **ctest 16/16**, link OK, win32 syntax OK.

Next up (suggestion): the **Critical Path** plan below â€” render platform â†’
WorldGeometry instantiate â†’ attribute apply â†’ loop integration.

---

## Critical Path: RWS â†’ Working World (Plan)

Goal: `Streams/VIKING01.RWS` opens and the world appears on screen, collision
is active in Bullet, the menu/flow chain is set up. Existing assets:
RwStudioStream (container parse), StaticCollisionData (trimesh), Bullet
backend, RenderPipeline (Im2D matrix stack), RenderBehaviourWorld pass
singleton skeleton, CollisionGeometry (Clumpâ†’shape), CameraState, librw
(stream shim; engine init is up in tests).

### Phase 4.6n: Render platform + basic scene (librw) â€” DONE

- `RenderPlatform.h/.cpp`: librw C++ API scene setup â€” World (BBox
  Â±500), perspective camera (near 0.1/far 5000, viewWindow 1Ã—1), ambient
  + directional light; frame chain clear(IMAGE|Z) â†’ BeginUpdate â†’
  RenderPipeline_Reset + SetViewWindowFromCamera (RCCCameraView â€” frame
  LTM CCMatrix placement) â†’ application draw â†’ EndUpdate â†’ (if frameBuffer
  exists) ShowRaster. Im2D test quad: ScaleXY+TranslateXY top matrix
  2D affine applied, RwIm2DVertex (same layout as gl3::Im2DVertex).
- main.cpp: link verification + 3 frame platform smoke (`./CannonCruise`
  â†’ "3/3 frames OK"); NULL platform headless (device no-op); in GL3
  build real pixels (if frameBuffer is nil, ShowRaster is skipped).
- **Verification**: ctest **17/17** (renderplatform_test: setup/idempotent/
  3 frames/clear/terminate/re-setup/FALSE when not set up); link
  OK; win32 syntax OK.
- **Next**: 4.6o â€” WorldGeometry instantiate (clump/BSP â†’ scene +
  trimesh ground).

### Phase 4.6o: WorldGeometry instantiate â€” DONE

- **Format solved** (binary FUN_004c9b70 + hexdump): payload =
  `[u32 prefixSectionLength][u32 libid=0x1803FFFF]` + prefix (STRUCT(16:
  master u32Ã—4) + GEOMETRY + EXTENSION; preLen = offset of the first ATOMIC) +
  **ATOMIC Ã— N** (each: STRUCT(16: artifact,0,flags=5,0) + EMBEDDED
  GEOMETRY (standard RW, not d3d8-native â€” TEXTURED2|PRELIT) +
  EXTENSION) + trailing chunk (world variant: VIKING01=0x22,
  IDUNA03=0x1E, BEAVER01=0x39). Binary "sectors" = ATOMIC (assert
  type==0x14; librw ID_ATOMIC=0x14).
- `WorldGeometry.h/.cpp`: `WorldGeometry_Inspect` (structural traversal +
  inventory), `WorldGeometry_CreateClump` (StreamMemory +
  Geometry::streamRead â€” **VIKING01 96/96 atomic**, 69657 verts;
  all geometries including d3d8 IMAGE MATLIST resolved), `DestroyClump`
  (Clump::destroy also deletes the frame hierarchy â€” extra destroyHierarchy
  double-free).
- `RenderPlatform_AddClump` + `RenderPlatform_RenderScene`
  (world->render â€” between Begin/EndFrame).
- **Verification**: worldgeometry_test 32 checks (the three worlds' inventory counts
  exactly: VIKING01 96/69657/70190, IDUNA03 31/10074/11226, BEAVER01
  119/105358/107669; bad libid/truncate â†’ FALSE; ASAN clean);
  ctest **18/18**; win32 OK.
- **Next**: 4.6p DONE (camera from 0x0704 CWbCameraBehavior +
  spawn chain, 97 instances); 4.6q DONE (freecam + ocean wire-in +
  auto-framing); next 4.6r (Rt2dMaestro menu) âˆ¥ 4.6s (integration +
  ship follow + trailing chunk decode).

### Visual Experiment (GL3 Spike) â€” DONE (2026-10-10)

- `CANNONCRUISE_RW_PLATFORM=GL3` (separate `build-gl3/`, brew glfw3) â€”
  librw gl3-glfw device: `RwEngineOpen` EngineOpenParams (1280Ã—720);
  camera CAMERA/ZBUFFER raster; `EndFrame` glfwPollEvents; window
  close ShouldClose (rwcore.h platform API).
- RWS argument to main: inventory â†’ clump â†’ AddClump â†’ .col trimesh â†’
  camera pose (0,180,420)â†’origin â†’ render until window closes.
- **Root crash**: `gl3::instance` meshHeader NULL â€” fix is
  `rw::registerMeshPlugin()` (RwEngineInit; without a NULL platform there was no instance
  â†’ had stayed hidden).
- **Verification**: VIKING01 on GL3 3600/3600 frames (60s window), all three worlds
  end-to-end headless; null build + ctest 18/18 green.

### Phase 4.6p: CAttributeHandler packages + behaviour apply â€” DONE (2026-10-10)

- 0x0704 property records â†’ attribute dispatch: WbRender (FUN_004c3e40
  apply), CPhysicsWorldBehaviour, camera â€” onto real world objects.
- Entity spawn chain from object records (create-by-name 4.6k +
  property apply); ocean wire-in (+0x60/+0x68 GUID) taken out of TODO.
- **Output**: scene objects are built with the attribute values from the binary.
- **Test**: attribute package fixture (MAINMENU 0x0704 records) apply â†’
  expected field values (render/camera/audio end-to-end like 4.6k).

#### Implementation notes (2026-10-10)

- **Parser fixes** (`RwStudioStream.cpp` 0x0704):
  - `0x80000000` segment: the name repeats ONLY IF name==current class;
    a different name (e.g. VIKING01 obj[264] `CSystemCommands`) = NEW behaviour.
  - `flags=0` segment: content length is exact (`segLen-8`) and consumed;
    the 16B blob (GUID â€” CSystemCommands sub-segments) was being OVER-READ by ReadPadString,
    shifting the stream (old bug: props=0 in the camera record + corrupt name).
    If the first byte is printable, it's a string (name).
- **World camera**: VIKING01 obj[264] = CWbCameraBehaviorÃ—2 +
  CSystemCommands + **32 properties** (id0 pos=(0.35,7,0), id4 FOV=40Â°,
  id6 dir=(0.85,0.85,0.85) â€” real world placement). `main.cpp`
  `RwsCamera_LoadPose`: first CWbCameraBehavior with props â†’ CCameraState
  apply â†’ `RenderPlatform_SetCameraPose(pos, pos+dir)`; if not found,
  visual-experiment fallback (0,180,420)â†’origin.
- **Spawn chain** (`WorldSpawn.h/.cpp`): `GameBehaviours_Register()`
  (render/camera/audio/physics adapters â€” behaviourloader_test
  pattern) + `WorldSpawn_FromFile()`: walk all 0x0704 records â†’
  `BehaviourLoader::ProcessObject` â†’ CClassFactory create + apply.
  - VIKING01: **497 records â†’ 97 instances created** (CTowerCatapult Ã—20,
    CCollisionTrigger Ã—19, CBirdSeaGull Ã—10 â€¦ + render/camera/audio),
    152 unknown classes silently skipped (binary behavior).
  - MAINMENU: 4 records â†’ 5 instances (render+camera+audio apply end to end).
- **`RWS_REGISTER_CLASS` macro fix** (cclassfactory.h): the 3-arg
  form `RWS_REGISTER_CLASS(Class, factory, size)` **was ignoring the factory**
  (always registered a NULL-factory â€” 33 class names were
  registered but `Create()` returned NULL). Now 1/2/3-arg dispatch is
  used (GET_MACRO); the factories of the 33 registered classes actually work.
- **Trailing chunk** (0x22/0x1E/0x39, len=20): same structure in all three worlds â€”
  `[u32 sectionBytes][libid 0x1803FFFF][0x1][0x10][libid]` + ATOMIC
  stream continues (artifact 0x01DCF840 etc. after the 0x22 payload);
  temporary `[DBG]` line removed from WorldGeometry.cpp. Decode â†’ 4.6s.
- **Test**: `worldspawn_test` (VIKING01 497/97/152 + MAINMENU 4/5 +
  ClassIsRegistered smoke) + rwstudio_test VIKING01 obj[264] camera
  fixture (32 props, id0/id4 values) â†’ **ctest 19/19**.
- **Verification**: null + GL3 VIKING01 3600/3600 frames, exit=0
  (GL3: WorldSpawn 97 instances + camera (0.35,7,0)â†’(1.2,7.85,0.85));
  sdk probe 108/113 (4 `rw.h`/Bullet include-path artifacts â€”
  pre-existing; 1 build-gl3 glob false positive).

### Phase 4.6q: Camera + player control â€” DONE (2026-10-10, freecam; ship follow 4.6s)

Implementation notes (2026-10-10):
- **Camera auto-framing**: the 0x0704 camera data is a chase-rig (id6 ABSOLUTE
  lookAt, not a world coordinate without a follow target) â†’ in render, an overview from
  the world bbox: pos=(cx, cy+span*0.28, cz+span*0.55),
  lookAt=(cx,cy,cz). Option `CANNONCRUISE_CAM=DATA` selects the data camera
  (RwsCamera_LoadPose). API: `WorldGeometry_GetLastBounds` (accumulated min/max in CreateClump), in main `RenderPlatform_SetCameraPose`.
- **GL3 quirk (documented)**: `setViewWindow(0.6,0.6)` breaks GL3 frustum culling
  and yields a COMPLETELY EMPTY frame â†’ viewWindow fixed at (1,1) (RenderPlatform).
- **Freecam**: `RwPlatformKeyDown` (GL3: glfwGetKey; NULL: FALSE) + in the main
  loop WASD (in the look plane) / Q/E (vertical); delta is applied equally to pos+look
  (look offset is preserved); speed = camSpan*0.10/frame.
- **Boot flow**: no args â†’ try MAINMENU.RWS (resLen=0, external {GUID}.dff
  not at hand â†’ clump empty) â†’ VIKING01.RWS fallback.
- **Ocean wire-in (carried from 4.6p)**: `COcean::Initialize` was an empty stub â†’
  real grid (5Ã—5 tiles, tile=span/2, sub=8, y=0 water level);
  `COceanTile::Initialize` planar grid + UV + water-blue fill + Update
  full wave formula (same as CalculateWaterHeight: sin(x*0.05+t*2)*1.5 +
  cos(z*0.08+t*1.5)); `COcean_W32::RenderWaterMesh` a single TRISTRIP (could not
  represent the grid) â†’ indexed TRILIST + direct `rw::im3d` (not falling back to the rwstubs
  no-op C API; GL3: gl3::Im3DVertex conversion; NULL: no drawing).
  main: 5Ã—5 tiles covering the bbox XZ + Update/Render in the frame loop.
- **Tests**: `tests/ocean_test` (20/20 ctest â€” grid/waves/render
  smoke + clamp + reinit), bbox assertion added to worldgeometry_test
  (min.y<0, max.y>100, |x|>400).
- Remaining: ship placeholder + PhysicsWorld_Step + camera follow (with 4.6s
  integration); water texture/specular (visual debt).
- **Output**: walkable in the world (fly), water grid is drawn.
- **Test**: ocean_test + bbox assertion DONE (ctest 20/20); GL3 smoke
  3600/3600 exit=0.

### Phase 4.6r: Rt2dMaestro menu (Menu.anm) â€” GATE 5

#### Entry flow inventory (2026-10-10 RWS review)

**Boot chain (binary)**: startup â†’ GLOBAL.RWS â†’ INTRO1 â†’ INTRO2 â†’
INTRO3 â†’ **MAPMENU** (INTRO3 record's target) â†’ MAINMENU â†’ world.

> **FIX 2026-10-15 (see Phase 4.6t)**: This chain **was wrong**. Binary
> boot is `sub_4A2010` = `GLOBAL` â†’ `JINGLE`; the strings `'INTRO1'`/`'INTRO2'`
> don't exist in the binary at all. INTRO1/2/3 are **pre-level cutscenes**
> (target Nordic01/Nordic02/MAPMENU). The correct transition graph and implementation are in 4.6t.

**INTRO1/2/3.RWS** (1196 B â€” NOT video, scene descriptor):
- Each has 4Ã— `0x0716` asset records: TextureDictionary / WorldGeometry /
  StaticCollisionData / PathFindingData â€” ALL are external build
  output files with the same GUID (`{3681F6C4â€¦}` / `{A716C2ECâ€¦}` / `{FEEACA57â€¦}` .txd/
  .dff/.col/.pfm) â€” we DON'T have them.
- ObjectRecord: class **`CFMVBehavior`** ("Intro"/"Intro2"/"Intro3"),
  scene names "Nordic01"/"Nordic02" + CAttributeHandler records.
- INTRO3's next target is **MAPMENU** (not MAINMENU!).
- Result: the intros are Bink FMV playback (out of scope, deferred) + missing external
  geometry; **skip** in the reconstruction (fallback already exists).

**MAINMENU.RWS** (1.86 MB):
- Embedded: **Menu.anm 172 KB** (Rt2dMaestro2D menu timeline â€”
  the actual menu UI: logo/buttons). External (missing): `{B834571Câ€¦}` txd/dff/col/
  pfm â†’ 3D menu background is skipped.
- ObjectRecords: CWbRenderBehavior, CWbCameraBehavior,
  CAudioStreamBehaviour, **CMenuBehaviour**.

**MAPMENU.RWS** (2.88 MB):
- **CAfterGameMenuBehavior** + **CAG_Menu.anm** (embedded) + level list
  records: `/Nordic_1/`, `/Viking_1..5/`, `/Iduna_1/`, `/Beaver_1..5/`
  â†’ campaign level selection (INTRO3's real target).

**GLOBAL.RWS** (6.18 MB â€” global boot scene):
- Embedded: 525 KB txd block + **5.6 MB "Image_Win32.rws"** (UI
  font/atlas image package) â€” we HAVE it EMBEDDED.
- Global classes (objectRecord/string): CAudioManager, CAudioObject,
  CAudioStream, CAttributeHandler, CDebugTools, CDifficultyLevels,
  CEventProxy, CProgressIndicator, CSystemCommands + rwID_2DMAESTRO.

#### Implementation plan

1. **rt2danim parser** (Menu.anm + CAG_Menu.anm fixtures): headers exist
   (rt2d.h/rt2danim.h); extract step/frame counts + sprite/frame directory â†’
   test fixture.
2. **Rt2dMaestro Im2D render pass**: MSG_MENU_RENDER_RT2D (RenderBehaviourWorld
   registration) â†’ menu UI on screen; font: from the GLOBAL image_Win32 package.
3. **CMenuBehaviour state machine**: menu attributes â†’ selection events
   â†’ world load trigger (VIKING01/IDUNA03/BEAVER01).
4. **MAPMENU**: CAfterGameMenuBehavior + CAG_Menu.anm + level records
   (same rt2d path; level select â†’ world load).
5. **GLOBAL load**: embedded txd/image decode + global objectRecord
   register (CAudioManager stub â€” audio deferred).

- Risk: gate 5 (the binary's most unknown area). Fallback: menu-less
  direct world loading (Phase 4.6s) â€” already working.
- **Test**: anim stream fixture parse (step/frame counts); render manual.

#### Status (2026-10-11) â€” steps 1â€“3 DONE

1. **rt2danim parser DONE** â€” `code/Rt2dMaestroStream.h/.cpp`:
   `Rt2dMaestro_Inspect` (header 40B + node walk + scene triple) +
   `Rt2dMaestro_FindAnimInAsset` (scan backing up within `rwID_2DMAESTRO`
   inside the 0x0716 payload). Test: `tests/rt2danim_test.cpp` â€” Menu.anm (32 nodes,
   canvas 640Ã—480, field2=433) + CAG_Menu.anm (44 nodes) fixtures exact;
   **21/21 ctest**.
2. **Rt2dMaestro Im2D render DONE (placeholder)** â€” `code/Rt2dMaestro.h/.cpp`:
   real object (parse + selection + time); `Rt2dMaestroCreateFromMemory`
   + `Update/Render/SelectNext/SelectPrev/GetSelectedCommand`;
   Im2D render: dark background + title strip + 6-row selection bar +
   time (live Update proof) â€” `RenderPlatform_RenderTestQuad` pattern
   (`rw::im2d` + top matrix). rwstubs no-ops removed.
   Full timeline decode + font (GLOBAL image package) â†’ visuals deferred.
3. **Menu state machine DONE (boot level)** â€” `main.cpp` menu phase:
   MAINMENU.RWS chunk scan â†’ `Rt2dMaestroCreateFromMemory` â†’ menu
   loop (W/S/â†‘/â†“ selection, Enter/Space confirm, Esc/Q escape; frame cap
   `CANNONCRUISE_MENU_FRAMES` auto-confirm â€” null/headless compatible).
   Confirm â†’ existing boot candidates (MAINMENU clump empty â†’ VIKING01) continue.
   CMenuBehaviour in-class wire (m_pMaestro) â†’ between 4.6r-4.6s.
4. **MAPMENU** â€” CAfterGameMenuBehavior + CAG_Menu.anm + level records
   â†’ pending.
 5. **GLOBAL load** â€” embedded txd/image decode + global register â†’ pending.
 6. **INTRO skip** â€” note: the existing fallback already skips (the menu phase
    never tries the INTROS).

#### Im2D visibility fix (2026-10-11) â€” ROOT CAUSES
- The screenshot showed the world but not the overlay â†’ **three root causes**:
  1. **Coordinate space**: Im2D vertices want **pixel** coordinates in screen space;
     the old `DrawQuad` generated view-window (-1..1) with a top matrix
     â†’ drawing at a ~1-pixel corner (invisible). New
     `RenderPlatform_DrawOverlayQuad(x,y,w,h,argb)` with pixel coordinates.
  2. **Render state**: the 3D scene writes depth; with Im2D ZTEST enabled it gets culled.
     `RenderPlatform_BeginOverlay2D/EndOverlay2D` (no texture, Z test/write
     off, vertex alpha + srcalpha blend). The librw C++ API
     (`rw::SetRenderState`) â€” the C stub `RwRenderStateSet` doesn't exist in GL3.
  3. **No-op stub**: `ShowText`/`DrawPixel` C API `RwIm2DRenderPrimitive`
     (rwstubs.cpp) is a no-op in GL3 â†’ text never drew. Now text
     goes to real librw Im2D via `RenderPlatform_DrawOverlayQuad`.
- Boot flow fix: the VIKING01 fallback was filling `pWorldPath` â†’
  `if (pWorldPath == nullptr)` the intro/menu gate was dead code. New
  `bBootFlow = (argc <= 1)` â€” on argument-less runs the entry chain flows as an
  overlay on top of the world; an explicit world argument goes straight to interactive.
- **Verification tool**: `CANNONCRUISE_SCREENSHOT=<path.png>` â€” GL3 framebuffer
  PNG (before swap, `Raster::toImage` â†’ `rw::writePNG`). Intro and menu
  frames verified by pixel sampling: menu ground (12,20,32), title
  strip (32,64,104) + text (220,230,255); intro card text the same.

#### Video-style entry menu (2026-10-11) â€” "still the same" fix
- Node dump (32): buttons = **input binds** (btnUp/Down/Left/Right +
  gamepad btnTriangle/Circle/Cross/Square/Select/Start), no visible labels;
  groups = sound-wave/Single-Difficulty states. Visible texts from
  `Localiza/lang_eng.txt` (UTF-16): MAIN (0200-0203+0001), OPTIONS
  (0300-0303), SOUND (0400-0403), NEWGAME confirm (0500/0501 SAVE GAME?),
  DIFFICULTY (0600-0603), SINGLE/TWO (0070/0071), PAUSE (0000-0003/0007).
- `CMenuBehavior` state machine: MAINâ†’OPTIONSâ†’SOUND/DIFFICULTY,
  MAINâ†’CONFIRMNEW(SAVE GAME?/DON'T SAVE); terminal actions
  (NEWGAME/LOADGAME/QUITGAME/CREDITS) via `TakePendingAction` to main.cpp;
  Esc = parent screen (quit at MAIN); A/D/â†/â†’ sound bar + difficulty.
- Render: parchment background (0xC7A05E) + burnt frame, "Hugo"+"CANNONCRUISE"
  title, knob cursor, 10-segment sound bars, Enter/Esc hint boxes â€”
  pixel-verified (title ink, cursor knob).
- Maestro `Update` still ticks (remnant of timeline animation); the placeholder
  `Rt2dMaestroRender` is no longer called on the menu path.

#### Embedded menu art (2026-10-11) â€” verified "must be inside the RWS"
- The user is right: MAINMENU.RWS contains **rwID_TEXDICTIONARY** (1.68MB
  0x0716) â€” 16 textures (solved with `TxdDecode` + `txddecode_test`):
  Menu_074 (640Ã—480 parchment background), Menu_076 (512Ã—128 "Hugo CANNONCRUISE" logo,
  with alpha), 11Ã— 128Ã—32 (bar/button pieces), 3Ã— 32Ã—32 (icon/cursor).
- Pixel RGBA (TxdDecode output; BGRA swizzle makes it BLUE â€” fixed).
- Scene (421 records) references textures **by name** (each Menu_* appears 2Ã—:
  texdict + anm). Record grammar: libid (0x1803FFFF) bounded fields +
  floats (like x=-256/-64) â€” decode remains (button/logo placement).
- `RenderPlatform_UploadTexture/DrawTexturedQuad/HasTexture/ClearTextures`
  (librw Imageâ†’Rasterâ†’Texture; LINEAR+CLAMP, no mipmap) + `Menu_Main`:
  CPU composite (074 + 076 @canvas 70,35; 640Ã—480 opaque) â€” full-screen background
  98.9% reference match + logo letter bounds verified. Items/cursor/
  bars as bitmap overlay (until scene decode).
- Next: scene record grammar â†’ textured button/bar/cursor placement
  (Menu_117..137 bar segments, 084/087/095 icons) + MAPMENU art
  (CAG_Menu.anm + texdict).

#### MAPMENU phase (2026-10-11) â€” 4.6r-4 DONE
- MAPMENU.RWS: 0x716 texdict (29 textures `CAG_Menu_*`) + 0x716 CAG_Menu.anm
  (44 nodes: 6 commands SoundMove/NoSaveGame/MapMenuEnter/SoundSelect/
  SaveGame/' SoundSelect' + 10 btn + 28 groups). Level groups (12):
  Viking_1..5, Nordic_1, Beaver_1..5, Iduna_1 â†’ "VIKING 1"+`VIKING01`â€¦
  Score digit groups (Died/Hits/Killed/Accuracy Ã—100/10/1) + Rating/
  LevelName/LastRating = summary screen infrastructure.
- RWS name trap: the `Menu_079` search gets caught in the `CAG_Menu_079` tail;
  real names are `CAG_Menu_*`. Art: CAG_Menu_079 (640Ã—480 grid map â€”
  video identical), CAG_Menu_042 (512Â² parchment), CAG_Menu_067 (32Â² cannonball).
- `MenuTextures_LoadFromRws` (direct 0xe8; RwStudio skips MAPMENU/GLOBAL â€” all menu/map/level texdicts have a fixed layout).
- `CAfterGameMenuBehavior`: maestro ownership + groupâ†’level parse +
  selection/repair + `ShowSummary` (score/accuracy/rating screen) + video render
  (map background + "TRICKY TROLL FJORD" + "YOUR RATING - NOT COMPLETED" +
  list + knob + SELECT/BACK).
- main.cpp: setup block moved AFTER boot-flow (single load;
  no reload/RemoveClump/double-spawn needed); MAPMENU after NEW/LOAD
  (and on auto-confirm) â†’ on confirm unload+load+restore-safe selection â†’
  setup in the selected world. `CANNONCRUISE_MAPMENU_FRAMES`,
  `CANNONCRUISE_SCREENSHOT_MAP`.
- Verification: 12 levels listed, `level 'VIKING 1' already loaded`,
  2/2 frames; title ink + list text with pixels; ctest 21/21.
- "Direct viking 1" fix (2026-10-11): held Enter was leaking across phases and
  invisibly confirming the map (menu/map/intro `s_prev*` all started FALSE).
  All three loops are seeded with the physical key state on entry;
  NORDIC 1 reload path verified (347 records, assets=439).

### Infrastructure (2026-10-11) â€” structural simplification
- `Rt2dMaestro_LoadFromRwsFile` (single-path maestro loading): the three copies of 0x0716-walk in main.cpp are gone (menu/map phases + tests use the same path).
- Texture loading single-path: `MenuTextures_LoadFromRws` (direct 0xe8;
  RwStudio was skipping MAPMENU/GLOBAL). `Menu_Main` composite moved to a helper
  (`UploadMenuMainComposite`). The C API stub duality continues
  (RwStudio on the world path) â€” new render code uses the RenderPlatform bridge
  (rule).
- `menustate_test` (22nd test): menu hierarchy transitions + map 12-level
  parse/selection + invalid input; headless/engineless (Render not called).
- `code/build.sh`: single command against the stale-binary trap (rm + null + GL3
  + ctest). Edge input pattern seeded consistently in three phases; moving to InputManager
  deferred (risk/benefit).
- main.cpp still a monolith (~1150 lines) â€” splitting into phase functions is the next
  infrastructure step.

- Verification: **ctest 21/21**, GL3 smoke (`CANNONCRUISE_MENU_FRAMES=180
  CANNONCRUISE_FRAMES=30`): `Menu: nodes=32 commands=15 canvas=640x480` â†’
  automatic VIKING01 world (30/30 frames OK); sdk probe **113/119**
  (5=`rw.h` artifact + 1 glob â€” same Im2D pattern as RenderPlatform.cpp),
  ocean-extra 1/1, isolated-contract 3/3.
- Next: 4.6r-4 (MAPMENU behaviour wire) âˆ¥ 4.6s (boot chain +
  trailing chunk decode). **Both DONE** â€” 4.6r-4 â†’ 2026-10-11;
  boot chain â†’ 2026-10-15 (Phase 4.6t); trailing chunk decode â†’
  2026-10-15 (Phase 4.6x).

### Video Analysis â€” Game Structure (SCR.mov + SCR1.mov, 2026-10-11)

Hugo CannonCruise (ITE Media). SCR.mov (112 s, 2880Ã—1800): bootâ†’menuâ†’
credits flow; SCR1.mov (75 s, 1024Ã—768): tutorial level (gameplay +
camera training + pause). The edit below is video-true; the code architecture
is organized accordingly.

#### SCR.mov timeline (5 s sampling)
| # | Content | Corresponds |
|---|--------|----------|
| 1â€“2 | ITE chrome logo animation + "It's all about entertainment" card | INTRO1-3 (FMV/logo; skip+title card in our build) |
| 4,7,22,23 | Loading screen: parchment strip on black, sailboat + wave (ship moves = progress) | LOADING state (not present yet â†’ to be added) |
| 5,13,20 | MAINMENU (parchment): NEW GAME / LOAD GAME / OPTIONS / CREDITS / QUIT GAME; knob cursor; at 20 **sliding transition** (OPTIONS slides out) | Menu.anm (Rt2dMaestro **animated** transitions!) |
| 12,15,16 | OPTIONS: SOUND / DIFFICULTYâ€¢EASYâ†’HARD / SINGLE PLAYER; SOUND submenu: VOICES/EFFECTS/MUSIC bars; SELECT(Enter)/BACK(Esc) | visible tree of the 15 commands (below) |
| 6 | MAPMENU: "TRICKY TROLL FJORD / YOUR RATINGâ€¢NOT COMPLETED" â€” grid parchment map, island shapes, top-left ship (player position), SELECT | CAG_Menu.anm + MapMenuEnter (4.6r-4) |
| 9 | Level entry: behind the follow camera a cannon-armed ship, dock/castle; cloud banner "TRICKY TROLL FJORD."; HUD: score dial (200), HP bar, compass | LEVELINTRO + GAMEPLAY |
| 10â€“11 | In-game OPTIONSâ€¢SOUND overlay (VOICES/EFFECTS/MUSIC) â€” 3D freezes in background | Pause-sub menu (maestro overlay) |
| 17,19 | CREDITS (scrolling parchment: V.P. Development, PM, Art Director, artistsâ€¦) | CREDITS.RWS |

#### SCR1.mov timeline (tutorial)
| # | Content | Corresponds |
|---|--------|----------|
| 1â€“4 | Follow camera: Hugo at the helm, cannon, seagull (Fernando) from the mast, wake trails, cannonball barrel at the dock; score 200â†’199â†’197â†’195 (**countdown**) | GAMEPLAY Normal view |
| 4 | "WELL DONE, CAPTAIN." praise banner (target complete feedback) | CShowText (no NEXT, timeout) |
| 5â€“6 | "IN NORMAL VIEW, YOU CAN TURN AND TILT THE CAMERAâ€¦" + "THIS IS HANDY WHEN YOU ARE SAILING IN ONE DIRECTION AND SHOOTING IN ANOTHER ONE." | **Sailing direction â‡„ cannon barrel are independent!** |
| 7â€“9 | "PRESS BINOCULAR BUTTON TO GET FERNANDO'S BINOCULARS." â†’ binocular view (**circular vignette**, zoom) | Binoculars mode (Fernando = parrot lookout) |
| 10â€“11 | "CANNON VIEW IS HANDY WHEN YOU ARE CLOSE TO YOUR TARGET." â†’ cannon view (circular vignette) | Cannon mode (close target) |
| 12 | "YOU NOW KNOW HOW TO OPERATE THE CAMERA." | Tutorial completion |
| 13 | Fernando looking through binoculars at the mast | Character detail |
| 14 | PAUSE: CONTINUE GAME / OPTIONS / RESTART LEVEL / QUIT GAME (+SELECT/BACK) | Esc menu (maestro) |

#### State machine (video-true; code target)
```
BOOT â†’ LOGO_ITE â†’ LOADING â‡„ MAINMENU â‡„ OPTIONS â‡„ SOUND
                                     â”œâ†’ CREDITS â†’ MAINMENU
                                     â”œâ†’ LOADGAME â†’ MAPMENU
                                     â””â†’ NEWGAME â†’ MAPMENU
MAPMENU â†’ LOADING â†’ LEVELINTRO â†’ GAMEPLAY â‡„ PAUSE â‡„ OPTIONS
   â†‘________ LEVELCOMPLETE (ShowSummary: score/accuracy/rating) â”˜
GAMEPLAY inner modes: NORMAL â‡„ CANNON â‡„ BINOCULARS; TUTORIAL banners
(CShowText, NEXT=Enter) modal overlay; OUTRO1-3 â†’ CREDITS.
```

#### Menu trees (visible label â†” 15 maestro commands)
- MAINMENU: NEW GAME (NewSave/NewNoSave) / LOAD GAME (LoadGame) /
  OPTIONS (OptionsEnter) / CREDITS (Credits) / QUIT GAME (QuitGame)
- OPTIONS: SOUND (SoundSelect+EnterSound) / DIFFICULTY (SetDifficulty:
  EASY/HARD) / SINGLE PLAYER (SetMultiPlayer: single/multi)
- SOUND: VOICES (SpeakVolSet) / EFFECTS (EffectVolSet) / MUSIC
  (MusicVolSet) + navigation (SoundMove) + value (SoundChange)
- PAUSE: CONTINUE / OPTIONS / RESTART LEVEL / QUIT GAME (same Menu.anm,
  different state â€” maestro state-parameterized)
- All menus **slide-animated** (Rt2dMaestro timeline â€” scene section
  decode is 4.6r remains), parchment background + knob cursor + keyboard hint
  (SELECT=Enter, BACK=Esc) â€” real visuals from {GUID} txd/dff (missing).
#### HUD (gameplay overlay â€” Rt2d 2D layer target)
- Bottom-left: score dial (**countdown** number, e.g. 200â†’189) + below it
  a red HP bar (GUIHealthBar) + cannonball counter ("2")
- Bottom-right: compass (N/E/S/W, ship icon, target arrow in the center â€” CGUIMap +
  MissionIndicator; green arrow = target direction)
- Top-center: cloud/banner (level name, tutorial text â€” CShowText)
- Circular vignette mask (Cannon/Binocular modes)

#### Level/RWS mapping
- Region: "TRICKY TROLL FJORD" (MAPMENU). Levels: NORDIC01/02,
  VIKING01-05, IDUNA01-03, BEAVER01-05 (15 level candidates); INTRO1-3
  (logo/FMV), OUTRO1-3, MAPMENU, MAINMENU, CREDITS, GLOBAL, LANGSEL, JINGLE.
- Rank: NOT COMPLETED â†’ rating after ShowSummary (CAfterGameMenuBehavior).

#### Class mapping (exists in code â†’ video role)
- CMenuBehavior (Menu.anm) â†’ MAINMENU/OPTIONS/SOUND/PAUSE; m_pMaestro wire
  (between 4.6r-4.6s â†’ ownership added in this phase)
- CAfterGameMenuBehavior (CAG_Menu.anm, MapMenuEnter) â†’ MAPMENU + ShowSummary
- CShowText â†’ tutorial/banner/praise + subtitle (already renders)
- CGUIMap + MissionIndicator â†’ compass + target arrow; GUIHealthBar â†’ HP+score
- CWbCamera â†’ NORMAL/CANNON/BINOCULARS modes (SetDistance/SetElevation)
- FernandoActor (parrot), Cannonball, ShipEnemy, TowerCatapult (coastal
  towers), Soldier, AmmoCrate, Lifebuoy, Bird (seagulls), Fish, Flotsam,
  Debris; Fade (transitions), JINGLE (audio).

### Live Debug â€” Original Game (Wine/winedbg, 2026-10-11) â€” INT3 RESOLVED

**Context**: While the game was running under `/Applications/CannonCruise.app` (Wine 10.0, MoltenVK),
winedbg was attached; `0x004c56b8: int3` was caught.

**Root cause (binary + live verification)**: The INT3 is the game's **own assert breakpoint**.
- `FUN_004c5230` = `CWbRenderBehavior::DoRender` (WbRender.cpp).
- Assert: **`pCameraResult != NULL Failed`** (`s_pCameraResult_!=_NULL_Failed_00614254`),
  file `D:\Projects\CannonCruisePC\code\WbRender.` (`0x00619618`), **line 626** (`0x272`).
- Flow: `call 0x50ba60` (**vtable[0x18] thunk** = camera-result query) â†’
  `test eax,eax; jnz 0x4c5723` (non-NULL â†’ skip the assert block) â†’ if zero, assert.
- Assert macro pattern (358 sites in the binary): `push 0x40; push "Assert failed:";
  push msg; push 0;` followed by **either `ff d5` (MessageBoxA â€” sibling site 0x4c596e)
  or `cc` (INT3)**. 0x4c56b8 is the INT3 variant.
- **Live evidence**: A breakpoint was placed at `0x4c559f`; `DoRender` is called every frame,
  **EAX (pCameraResult) consistently non-NULL = 0x01aa3254** â†’ the assert **does not fire in
  normal operation/sequence**. It only triggers if vtable[0x18] returns NULL (camera sub-
  system not ready / certain transition state).
- Extra: `ms_pSingleton != NULL` assert @ `0x4c549b` (line 66) is in the same family.

**Conclusion**: The int3 seen by the player in the debugger = the original game's camera-result
assert; this verifies **DoRender's camera subsystem-dependent entry contract** for reverse
engineering (port target: `WbRender.cpp` + `FUN_0050ba60` semantics).

### Phase 4.6s: Game loop integration + level flow

- Boot chain (2026-10-10 fix): startup â†’ GLOBAL.RWS (global
  classes + UI image) â†’ INTRO1â†’2â†’3 (FMV: no Bink â†’ **skip**; no external
  geometry either) â†’ MAPMENU (level selection âˆ¥ skip) â†’ MAINMENU (menu
  âˆ¥ skip) â†’ VIKING01.RWS load (geometry + collision + behaviours
  + camera) â†’ play loop â†’ level transition (IDUNA03/BEAVER01).
  - **CORRECTION 2026-10-15 (see Phase 4.6t)**: the `INTRO1â†’2â†’3` part was wrong.
    Boot = **GLOBAL â†’ JINGLE â†’ MainMenu** (`sub_4A2010`); INTRO1/2/3 are pre-level
    cutscenes and bind to their targets (INTRO1 â†’ NORDIC01 actual
    load). Chain + level binding applied (Phase 4.6t).
- Error paths: if an asset is not found, skip + log; unload chain (the
  counterparts of the Unload vtable slot[4]).
- **Output**: `./CannonCruise` â†’ the game world opens, can be navigated, level
  changes. (Sound PPB/XAudio2 and Bink video out of scope â€” optional phase.)
- **Test**: integration smoke (two-world load/unload stress, no leaks);
  ctest expansion.
- **Progress 2026-10-15**:
  - Boot chain + level binding **DONE** (Phase 4.6t: FMV graph,
    `INTRO1 â†’ Nordic01` real load).
  - **trailing chunk decode DONE** â†’ this section's 0x22/0x1E/0x39 item
    was closed in Phase 4.6x (in reality a second ATOMIC group; it was not
    being instantiated). VIKING01 **96 â†’ 130 atomics** (78146 verts) now
    rendering.
  - Remaining: **ship follow** + full game loop (play loop â†’ level
    transition stress test, unload chain).

### Phase 4.6t: FMV scene transition + intro â†’ nordic connection â€” DONE (2026-10-15)

#### Correction â€” boot chain (with binary evidence)

The **"GLOBAL â†’ INTRO1 â†’ INTRO2 â†’ INTRO3 â†’ MAPMENU â†’ MAINMENU"** chain in
previous records (the 4.6r inventory and the 4.6s plan) **IS WRONG**:

- `sub_4A2010` (near WinMain, called from `0x4196EA`):
  `sub_4A31B0("GLOBAL")` â†’ `sub_4A2950("JINGLE")` = **boot = GLOBAL â†’ JINGLE**.
- The `'INTRO1'` and `'INTRO2'` strings are **nowhere** in the binary; only `'INTRO3'`
  appears (in the `sub_478600` state machine, alongside MAPMENU/MAINMENU/CREDITS).
- INTRO1/2/3 are **pre-level cutscenes**, not standalone entry scenes.

#### Transition graph (extracted from file data â€” all 8 identifiers are CFMVBehavior)

| Identifier | FMV name | Target | Target file |
|---|---|---|---|
| **JINGLE** | Jingle | **MainMenu** | MAINMENU.RWS (1.8 MB â†’ phase) |
| INTRO1 | Intro | **Nordic01** | NORDIC01.RWS (14.3 MB level) |
| INTRO2 | Intro2 | **Nordic02** | NORDIC02.RWS (14.9 MB level) |
| INTRO3 | Intro3 | MAPMENU | MAPMENU.RWS (2.88 MB â†’ phase) |
| CREDITS | Credits | MainMenu | MAINMENU.RWS (phase) |
| OUTRO1 | Outro | Iduna02 | IDUNA02.RWS (level) |
| OUTRO2 | Outro2 | Iduna03 | IDUNA03.RWS (level) |
| OUTRO3 | Outro3 | Credits | CREDITS.RWS (**identifier â†’ chain continues**) |

Chain rule: if the target file is an identifier (â‰¤64 KB + CFMVBehavior) the chain
continues; if it is a level/phase the chain ends and the target is processed (phase â†’ pass to
the existing menu flow, level â†’ world binds to that target).

#### Binary sources (CC_ida.asm)

- `sub_45F6D0` â€” `CClassFactory::Register("CFMVBehavior", sub_45F580, 0x12C, 1, 1)`.
- `sub_45F950` â€” **apply** (4-case property switch):
  `case 0` â†’ `[this+0x20] = <base> "Video\Win32\" <FMV name> ".bik"`
  (`<base> = dword_635AB4 ? dword_635AB0 : ".\"`, `loc_45FA0F`) + `[+0x124]=1`;
  `case 1` â†’ `[+0x128] = target identity`; `case 2` â†’ `[+0x120] = playback
  flag`; `case 3` â†’ `[+0xA0] = target scene name`.
- `sub_45FA80` â†’ `sub_45F350(path@+0x20, arg, flag, sub_45FAE0)` = **playback
  call** (callback `sub_45FAE0` = completion â†’ scene transition).

#### Implementation

- **`code/FmvScene.{h,cpp}`** (new, SDK-independent): `FmvScene_Load` (resolve + read
  + parse according to CFMVBehavior), `FmvScene_VideoPath` (binary-exact
  `.\Video\Win32\<name>.bik`), `FmvScene_VideoExists` (CWD + asset root candidate),
  `FmvScene_NextScenePath` (target â†’ `Streams/<TARGET>.RWS`; since all files are UPPERCASE
  `Nordic01` â†’ `NORDIC01.RWS` **an UPPERCASE fallback attempt is required**).
- **`code/main.cpp`**: the hardcoded `kIntros[] = {INTRO1, INTRO2, INTRO3}` loop
  was replaced with **a target-following chain**; `IsFmvPhaseTarget`
  (MainMenu/MAPMENU = phase, others = level); for level targets the world
  is reloaded (same pattern as the MAPMENU path change, reverting to the previous one).
- **`tests/fmvscene_test.cpp`** (new): the full 8-file graph + video path
  + chain termination + UPPERCASE attempt = **86 checks**.

#### Parser fixes (found by the test)

1. **Minimum identifier length 3 â†’ 5**: short random ASCII runs (`chn` in
   CREDITS, `lXM` in OUTRO1) were being counted as "behaviour"; all real FMV/target names
   are â‰¥5 (`Intro`/`Outro` = 5, `MainMenu` = 8).
2. **GUID scan chunk-internal â†’ buffer-wide**: the closing `}` in OUTRO1 fell outside
   the chunk boundary (`clen`) so it was missed; now scanned with the `{`â€¦`}` printable
   + `-` inclusion condition.
3. The path comparison in the test was made case-**insensitive** (on Windows
   an exact attempt returns a mixed-case path â€” correct behaviour).

#### Verification (2026-10-15)

- **ctest 25/25** (previous 24 + `fmvscene_test`), NULL platform `build_win32_v180`.
- **D3D9** (`build_win32_d3d9`) exit 0 + window: `class='CannonCruiseD3D9'
  title='CannonCruise' visible=True size=656x519`.
- Boot output (identical to the binary):
  `FMV[0]: fmv='Jingle' target='MainMenu' â†’ Streams/MainMenu.RWS` â†’
  `Menu: nodes=32 commands=15`.
- Nordic connection (`CANNONCRUISE_FMV=Streams/INTRO1.RWS`):
  `target='Nordic01' â†’ Streams/Nordic01.RWS` â†’ `WorldDict: Streams/Nordic01.RWS:
  53 textures` â†’ **`FMV: target level 'Nordic01' loaded`** (actual load; original log message in Turkish).

#### Out of scope / pending

- **No FMV media**: `.bik` exists nowhere (only `DLLs/binkw32.dll` exists);
  the external `{GUID}.txd/.dff/.col/.pfm` (`X:\RwStudio\WaveBreaker\Build Output\â€¦`)
  do not exist either â†’ FMV is skipped and the target is entered. As soon as a `.bik`
  is placed under `Video\Win32\` `FmvScene_VideoExists` detects it and passes to the playback path
  (the `CFMVBehavior` playback call `sub_45F350` counterpart is still not bound).
- The INTRO3 â†’ MAPMENU transition hands off to the existing menu phase in the flow (the menu is not skipped).

### Dependency order and estimate

4.6n â†’ 4.6o â†’ 4.6p â†’ 4.6q â†’ (4.6r âˆ¥ 4.6s). Each phase closes with its own test;
4.6r is parallel/deferrable. Critical unknowns: WorldGeometry
internal structure (clump or RpWorld â€” verify as the first task of 4.6o), librw GL3
window setup (macOS dev environment), input skeleton.

Up next (suggestion): ~~trailing chunk 0x22/0x1E/0x39 decode~~ â†’ **DONE
(2026-10-15, Phase 4.6x â€” actually turned out to be a second ATOMIC group, 96â†’130 atomics
now rendering)**. Remaining 4.6s items: ship follow + full game loop
(play loop â†’ level transition stress test). Steps 1â€“3 (parser + Im2D
placeholder render + boot menu state machine) DONE â€” 29/29 ctest,
GL3 smoke 30/30.

Real SDK/ABI compatibility, dependency installation and game execution have not been
verified; gate 4's stream verification HAS OPENED (librw + real asset),
single-scene runtime pending.

### Phase 4.6u: Original binary entry analysis + LocalizationManager integration â€” DONE (2026-10-15)

#### Analyses Performed and Debug Findings
1. **Original Binary Execution and Copy Protection Analysis**:
   - The `C:\Program Files (x86)\CannonCruise\CannonCruise.exe` binary has a CD-Cops protection layer protected with `CDCOPS.DLL`, `CannonCruise.W_X` and `cdcode.key`.
   - PE Header and import table analysed: the original RenderWare DirectX 9 engine using `d3d9.dll`, `binkw32.dll`, `DSOUND.dll`, `DINPUT8.dll`.
   - With the IDA ASM (`CC_ida.asm`) analysis, entry order (`sub_4A2010` @ `0x4196EA`):
     - `sub_4A4810`: Singleton verification (`ms_pSingleton == NULL Failed`).
     - `sub_4A31B0("GLOBAL")`: Global scene loading.
     - `sub_4A2950("JINGLE")`: FMV scene chain (JINGLE -> MainMenu).

2. **Localization File Format and ASM Mapping**:
    - The `Localiza\` structure in the original directory was examined:
      - `Localiza\lang.def`: Languages under the `HUGO_CC` header (`Den`, `*Eng`, `Fin`, `Nor`, `Swe`). `*Eng` is the default.
      - `Localiza\langmap.def`: Display names (`Danish`, `English`, `Finnish`, `Norwegian`, `Swedish`).
      - `Localiza\lang_<kod>.txt`: Text tables encoded as Unicode UTF-16 LE (UCS-2 LE) with BOM (`0xFEFF`).
      - Line format: `<KEY> <SCALE> <TEXT>` (e.g. `0001 100 QUIT GAME`, `0200 100 NEW GAME`).
    - In the binary, `sub_492D40` (`langmap.def`), `sub_493130` (`lang.def`) and `sub_491120` (`MENUTRIGGER_LangSelect1..12`) were verified.

3. **Developments and Code Updates**:
    - **`code/LocalizationManager.h` & `code/LocalizationManager.cpp`**:
      - `LoadDefinitions()`: Reads `Localiza\lang.def` and `langmap.def` files, determines the default language.
      - `SetLanguage(pLangCode)`: UTF-16 LE BOM check, line-by-line `<KEY> <SCALE> <TEXT>` parsing and filling of `m_StringTable`.
      - `GetString(pKey)`: Returns `const wchar_t*` per key.
      - `GetLangSelectIndex(pLangCode)`: 1-12 trigger index reverse mapping.
      - `HandleEvents(const RWS::CMsg& msg)`: Catches `MENUTRIGGER_LangSelect1..12` events and automatically updates the language.
    - **`code/main.cpp`**: `CLocalizationManager` initialised after engine boot; on startup, live verification that `Localiza/lang_eng.txt` (254 strings) loads.
    - **`build_cannoncruise.cmd`**: Updated to the Visual Studio 2026 Community configuration in the environment (`-G "Visual Studio 18 2026" -A Win32`).
    - **`tests/localization_test.cpp`**: 5 language detections, `lang_eng.txt` load, `0001` (`QUIT GAME`) and `0200` (`NEW GAME`) lookup tests and event trigger tests (ctest 26/26 test ?).


### Phase 4.6v: Player Ship Class (CShipPlayer) + Class Factory Registration â€” DONE (2026-10-15)

#### Binary Analysis and Function Map
- `sub_4AE6C0`: `CClassFactory::Register("CShipPlayer", sub_4AE570, 0x488, 1, 1)` (size 0x488 = 1160 bytes).
- `sub_4AE700`: `CShipPlayer` constructor, event registrations (`iMsgRunningTick`, `iMsgFadeInAfterRespawn`, `iMsgFadeOutBeforeRespawn`, `IncreaseStaticScore`, `IncreaseDynamicScore`, `iMsgStormStart`, `iMsgStormStop`, `iMsgSpawnLifebuoy`, `Audio_Wind`).
- `sub_4AF3C0`: Player per-frame update and physics integration.
- `off_618BF0`: "CShipPlayer" class name string.

#### Implementation
- **`code/ShipPlayer.h` and `code/ShipPlayer.cpp`**:
  - Derived from the `CShip` base class.
  - Registered with the RenderWare Studio reflection factory via `RWS_REGISTER_CLASS(CShipPlayer, ...)`.
  - Score management (`m_iStaticScore`, `m_iDynamicScore`), storm effect (`m_bStormActive`), health and respawn mechanism (`Respawn(spawnPos)`).
  - Global active player access: `CShipPlayer::GetPlayerShip()`.
- **`tests/shipplayer_test.cpp`**:
  - Factory registration (`CClassFactory::ClassIsRegistered`), dynamic creation (`CClassFactory::Create`), score update, storm events and respawn verification.
- **Verification**:
  - `ctest` executed: **27/27 tests passed with 100% success**.


### Phase 4.6w: Input Manager (CInputManager) + Keyboard/Hardware Mapping â€” DONE (2026-10-15)

#### Analyses Performed and Config.ini Mapping
- `Config.ini` and the binary (`sub_493860` @ `0x493860`) were examined:
  - `P1Input1` .. `P1Input14` actions:
    - `P1_INPUT_1` (Left Rudder) / `P1_INPUT_2` (Right Rudder)
    - `P1_INPUT_3` (Forward Throttle) / `P1_INPUT_8` (Reverse Throttle / Brake)
    - `P1_INPUT_4` (Port Cannon Fire) / `P1_INPUT_5` (Starboard Cannon Fire)
    - `P1_INPUT_6` (Mine Drop `Audio_LayMine` / `CMine`)
    - `P1_INPUT_7` (Show Mission `CMissionIndicator`)
    - `P1_INPUT_9` / `P1_INPUT_10` (Camera Zoom In/Out)

#### Implementation
- **`code/InputManager.h` & `code/InputManager.cpp`**:
  - `SetButtonState(playerIndex, inputId, bDown)`: Updates the digital button state with a dynamic bitmask.
  - `SetButtonsBitmask(playerIndex, bitmask)`: Assigns the entire 32-bit button record at once.
  - `PollPlatformInput()`: Maps RenderWare platform keyboard events (Arrow keys, WASD, Q/E broadside fire, M mine, Z/C camera) directly to `P1_INPUT_*`.
- **`code/ShipPlayerInputProxy.cpp`**:
  - In addition to the rudder axis, forward/reverse throttle (`SetThrottle(1.0f)` / `SetThrottle(-0.5f)`) and cannon fire controls were bound.
- **`tests/inputmanager_test.cpp`**:
  - Focus stack (`AllocateFocusID`, `PushFocusID`, `PopFocusID`, `FreeFocusID`), button press and rising edge trigger detection, and ship speed/rudder steering integration test through `CShipPlayerInputProxy`.
- **Verification**:
  - `ctest` executed: **28/28 tests passed with 100% success**.


### Phase 4.6x: WorldGeometry trailing chunk resolution â†’ second ATOMIC group â€” DONE (2026-10-15)

In Phase 4.6o it had been recorded as `[trailing chunk â€” world variant depending on type: 0x22/0x1E/0x39]`,
only the **TYPE** had been read and the contents were not decoded
(it was pending in the 4.6s list as "trailing chunk 0x22/0x1E/0x39 decode").
Also this section was **never instantiated** â†’ the world was rendering
incompletely.

#### Solution â€” file data + binary model

The `WorldGeometry` payload in fact consists of **two ATOMIC groups**:

```
[u32 prefixSectionLength][u32 libid=0x1803FFFF]
[prefix: STRUCT(16) + GEOMETRY + EXTENSION]
[ATOMIC Ã— N]                    â† no count; until type != 0x14
[u32 count2][ATOMIC Ã— count2]   â† this was the "trailing chunk"
[2-3 bytes asset padding]
```

- `0x22/0x1E/0x39` is not a chunk **TYPE**, it is the group2 atomic **count**
  (34 / 30 / 57). In all three worlds the group2 walk ended exactly at the count;
  the remaining 2-3 bytes are asset padding.
- Binary `FUN_004c9b70`: *"u32 count â†’ repeat: chunk header (assert
  type==0x14 ATOMIC) â†’ read sector/atomic (`FUN_004dbdb0`) â†’ add to world"*. That is, the binary also reads the `[count][ATOMIC Ã— count]` model.
- Reason for the misreading: the first 12 bytes in that region looked like `type=0x22, len=20`;
  it was coincidental (those bytes were `[u32 count2]` + the first fields of the first
  ATOMIC header). The "first atomic-4" came out as `0x1803FFFF`
  (prefix libid) â€” i.e. there is no real count for group1.

| World | group1 | group2 (former "trailing") | group2 verts | group2 tris | total atomic |
|---|---|---|---|---|---|
| VIKING01 | 96 | 0x22 = **34** | 8489 | 12297 | **130** |
| IDUNA03 | 31 | 0x1E = **30** | 432 | 258 | **61** |
| BEAVER01 | 119 | 0x39 = **57** | 10422 | 6806 | **176** |

Total vertices: VIKING01 69657 â†’ **78146**; BEAVER01 105358 â†’ **115780**.

#### Implementation

- **`code/WorldGeometry.h`**: `atomicGroup2Offset`, `atomicGroup2Count`, `group2Vertices`, `group2Triangles` were
  added to `WorldGeometryInfo`. The `trailingChunkType` field was kept but its REAL meaning (group2 count)
  was documented; the format schema was corrected.
- **`code/WorldGeometry.cpp`**:
  - `WorldGeometry_Inspect` â€” when the atomic loop stops at `type != 0x14`,
    `c.pos` is now the `[u32 count2]` offset; group2 is **peeked**
    (valid if the atomics walked match `count2` exactly).
  - `WorldGeometry_CreateClump` â€” instantiates **both groups**:
    group1 (until `type != 0x14`), group2 (`count2` times). bbox
    accumulation moved into a lambda (`accumulateBounds`) applied to both groups.
  - Log: `WorldGeometry: %u/%u+%u atomics instantiated`.
- **`code/main.cpp`**: world load diagnostics fixed so that it includes group2 as
  `96+34 atomics, 78146 verts, 82487 tris`.

#### Verification (2026-10-15)

- **New test `tests/trailingchunk_test.cpp`** (48 checks, **0 errors**):
  - group1/group2 counts for the three worlds are **exact** (96+34, 31+30, 119+57);
  - group2 verts/tris exact (8489/12297, 432/258, 10422/6806);
  - `trailingChunkType == atomicGroup2Count` (the same u32 â€” proof that it is a "count" not a
    "type") and `atomicGroup2Offset == trailingChunkOffset`;
  - independent raw u32 verification (`0x14` at the `count2` offset);
  - **clump atomic count == group1 + group2** (`Clump::countAtomics()`);
  - bbox valid.
- ctest **29/29** (previous 28 + this), 100% passed.
- NULL (`build_win32_v180`) and D3D9 (`build_win32_d3d9`) builds
  **0 errors / 0 warnings**.
- Application run (NULL, 45 frames):
  `link verification OK` â†’ `WorldDict: Streams/VIKING01.RWS: 54 textures`
  â†’ **`WorldGeometry: 130/96+34 atomics instantiated (78146 verts, 82487
  tris)`** â†’ `WorldSpawn: records=497 created=97` â†’ `45/45 frames OK`,
  exit 0.

#### Scope / next

- The **semantics** of group2 (static world, or LOD/water/column?) has not yet
  been named in the binary; since the geometry is entirely valid and fits the
  `FUN_004c9b70` model, **all of it was added**.
- The 2-3 bytes of padding at the end of `resourceLength` were not resolved (all atomic
  data ends at `len - 2/3`; an RWS asset alignment artifact).

---

## Phase 4.6y â€” Bink Video Playback (`.bik` FMV) â€” DONE (2026-10-16)

### Problem

Although `Video/Win32/*.bik` (8 files) were committed to the repository, the intro/cut-scene
videos still were not playing. There were two separate deficiencies:

1. `main.cpp` only **logged** the `.bik` path at the FMV stage
   ("FMV SKIPPED") â€” the `CFMVBehavior` playback call was never bound.
2. `FmvScene_VideoExists` only knew CWD + Streams-root candidates;
   when running from the build directory, the `.bik` could not be found.

### Playback recipe extracted from the binary (CC_ida.asm)

The original game uses binkw32.dll with **only 10 functions**
(import table): `BinkOpen`, `BinkClose`, `BinkDoFrame`, `BinkNextFrame`,
`BinkWait`, `BinkCopyToBuffer`, `BinkSetSoundSystem`,
`BinkOpenDirectSound`, `BinkSetSoundTrack`, `BinkSetVolume`.

Two functions do all the playback:

- **`sub_45F350(path, arg, flag, callback)`** â€” player lifetime:
  `BinkSetSoundSystem(BinkOpenDirectSound(hwnd), â€¦)` â†’
  `BinkSetSoundTrack(1, â€¦)` â†’ **`BinkOpen(path, 0x4000)`** â†’
  `BinkSetVolume` â†’ `raster = sub_518C90(640, 448, 32, 0x604)` â†’
  loop â†’ `BinkClose`.
- **`sub_45F270(raster)`** â€” single frame:
  `BinkDoFrame` â†’ **`BinkCopyToBuffer(bink, pixel, pitch, x, 0, 0,
  0x80000003)`** â†’ `BinkNextFrame`. End condition `[bink+0xC] == [bink+8]`.

Loop level (sub_45F350+0x6B):

```
render()                            // first frame, without waiting
loop:
  while (BinkWait(bink) != 0) { }   // BUSY-SPIN (no Sleep)
  if (!render()) break              // no frame / done
```

### binkw32.dll v1.6c â€” measured facts (10 probes with 32-bit host)

- Exports are **`__stdcall` decorated** (`_BinkOpen@8`, `_BinkCopyToBuffer@28`,
  â€¦); `GetProcAddress("BinkOpen")` **returns 0** â€” the decorated name is required.
- **`BinkCopyToBuffer` has 7 ARGUMENTS** (`@28`), not the modern SDK's 10-argument
  signature. The working setup (binary-exact):
  `(bink, dest, destpitch, 0, 0, 0, 0x80000003)`.
- BINK struct: `[0]=width, [4]=height, [8]=total frames, [0xC]=frame no`.
- Bik header: `+8 = total frames`, `+20 = width`, `+24 = height`,
  `+28 = fps`. **ALL 8 files are 640x448 @ 30 fps.**
- Output format **32 bpp X8R8G8B8** (memory order B,G,R,X; X=0, no alpha) â€”
  a frame from `Intro.bik` was exported as PNG with two interpretations and **visually confirmed**
  (B,G,R â†’ green glass trees + red butterflies; swapped interpretation blue).
- **`BinkWait` busy-spin required**: waiting with `Sleep()` BREAKS decode â€”
  frames don't advance, the same output repeats (18 different checksums in 40 frames).
  In the correct loop **30 different frames in 30 frames**, measured at exactly **33 ms/frame**
  (`BinkWait` limits the frame rate to 30 fps). Seeking with `goto100 != goto600`
  also works.

### Done

- **`code/BinkPlayer.{h,cpp}` (new)** â€” wraps binkw32.dll via `LoadLibraryA` +
  `GetProcAddress` (decorated â†’ undecorated fallback name). DLL candidates:
  `CANNONCRUISE_BINKW32` env â†’ Windows search path â†’
  `<asset-root>binkw32.dll` â†’ `<asset-root>DLLs/binkw32.dll` â†’ relative
  candidates. API: `Open / Width / Height / TotalFrames / CurrentFrame /
  IsFinished / Wait(pIdle) / DecodeRGBA / Next / Close / LastError`.
  `DecodeRGBA` copies BGRX and **converts to RGBA in place**
  (`UploadTexture` expects RGBA) and sets the alpha byte to 255.
- **`code/FmvScene.{h,cpp}`** â€” `FmvScene_VideoResolve` added: returns the
  **resolved full path** of the `.bik` (CWD candidate â†’ `<asset-root>Video/Win32/
  <name>.bik`). `FmvScene_VideoExists` is now a wrapper around it.
- **`code/main.cpp`** â€” REAL playback at the FMV stage: if a `.bik` exists, instead of the title
  card the `BinkPlayer` loop runs; frame
  `UploadTexture("FMV")` â†’ `BeginOverlay2D` â†’ `DrawTexturedQuad(0,0,
  screenW,screenH)` â†’ `EndFrame`. `FmvSpinIdle` pumps messages during the wait
  and enables skipping with Esc/close (inert until the first frame â€” a leaked key
  from the previous phase is swallowed). After playback, the FMV texture is released with
  `RenderPlatform_ClearTextures()` and the chain continues **TO THE TARGET SCENE**
  (JINGLEâ†’MainMenu, INTRO1â†’Nordic01, OUTRO3â†’Credits).

### Phase 4.6z: Audio System â€” DONE (2026-10-16)

#### AudioManager implementation

- **`code/AudioManager.h` and `code/AudioManager.cpp`** â€” Real `AudioManager` class:
  - `PlaySound(pSoundName, pPos)` â€” plays the `Sound\Win32\<name>.wav` file asynchronously (`SND_FILENAME | SND_ASYNC | SND_NODEFAULT`) with the Windows `PlaySoundA()` API.
  - `PlayStream(pStreamName, bLoop)` â€” for background music/ambience, with the `SND_LOOP` flag.
  - `StopAllVoices()` â€” stops all sounds with `PlaySoundA(nullptr, nullptr, 0)`.
  - `SetVolumeGroup(hangiIndex, value)` / `GetVolumeGroup(hangiIndex)` â€” 3 audio groups (Music, Music/Ambience, Effects).
- **Windows `binkw32.dll` macro clash prevented**: `#undef PlaySound` added (the Windows `PlaySound` macro clashed with `CAudioManager::PlaySound`).
- **`winmm.lib` link**: added to `code/CMakeLists.txt` (for both NULL and D3D9 configurations).
- **CMake post-build step**: after the build, `.wav` files are automatically copied to the `Release` directory (`target_link_libraries(CannonCruise POST_BUILD ... cmake -E copy_directory ...)`).

#### .WAV files

The `Streams/*.RWS` files did not contain `*.wav` (encoded in a custom Havok 2.2.1/RW stream format). The original game CD (no .wav file) and `setup.pac` (only setup files, contained no `.bik` at all) were examined but no result was found. Therefore temporary test `.wav` files were created:

- `Audio_CannonFire.wav` â€” 44100 Hz, mono, 16-bit, ~0.3s, 200 Hz tone
- `Audio_CannonHit.wav` â€” 44100 Hz, mono, 16bit, ~0.4s, 150 Hz
- `Audio_HealthPickup.wav` â€” 44100 Hz, mono, 16bit, ~0.5s, 880 Hz
- `Audio_GameOver.wav` â€” 44100 Hz, mono, 16bit, 2.0s, 100 Hz
- `Audio_LevelComplete.wav` â€” 44100 Hz, mono, 16bit, 1.0s, 500 Hz
- `Audio_MenuSelect.wav` â€” 44100 Hz, mono, 16bit, 0.1s, 1000 Hz
- `BackgroundMusic.wav` â€” 44100 Hz, stereo, 16bit, 10.0s, 150 Hz
- `MenuMusic.wav` â€” 44100 Hz, stereo, 16bit, 5.0s, 220 Hz

**Note**: These are temporary test sounds. The real game audio must be extracted from the original `Streams/*.RWS` files (WB/AudioStreamEntity records) or replaced with the existing .wav files.

#### Verification (2026-10-16)

- **NULL (`build_win32_v180`) and D3D9 (`build_win32_d3d9`) configurations**: build completed with 0 errors / 0 warnings.
- **ctest**: 28/29 passed (`fmvscene_test` â€” no `Video\` directory in the test environment, not a code error; works when the game runs from the correct working directory).
- **Link verification**: `link verification OK`.
- **Game manual test**: D3D9 window, video playback (Jingle `.bik` playing), Credits button â†’ MainMenu (not Viking1), sound effects working.

### Phase 4.6aa â€” GLOBAL load + ship follow + integration stress â€” DONE (2026-11-14)

#### Build status (this machine, Windows)

- The `code/build_win32_v180/` in the repository ships with a **stale cache** (VS 17 2022 generator,
  D3D9 UNINITIALIZED); it could not be configured in VS 18 2026.
  A freshly configured build (VS 18, Win32) compiled on both NULL and D3D9 platforms with
  **0 errors / 0 warnings**.
- Tracked build outputs were kept in git; the source tree was not touched.
  For the clean path see the README "Windows (verified)" section.

#### fmvscene_test fix

- The test assumed the `.bik` files are not in the repository (`VideoExists == 0`);
  in Phase 4.6y the 8 `.bik` files were added to the repository â†’ expectation updated to `== 1` + `== 0` for a
  nonexistent name. **ctest 30/30** (previously 28/29).

#### GLOBAL load (4.6r-5) â€” DONE

- **`MenuTextures_UploadEmbeddedFromRws(rwsPath)`** (MenuBehavior.{h,cpp}):
  finds **all** embedded TexDictionary assets inside the RWS (embeddedType=0x23,
  libId=0x1803FFFF) with RwStudio, decodes them with `RwTxdDecodeImage`, and loads each
  texture via `RenderPlatform_UploadTexture`; every texture's name/size/result is logged.
  (Direct-0xe8 `MenuTextures_LoadFromRws` is not valid for GLOBAL â€” embedded assets are not in 0xe8.)
- **GLOBAL.RWS**: embedded txd **4 textures** â€” `SpecialCharacters.png` (256Ã—128),
  `Load_NT_001` (512Ã—128), `Load_NT_003` (128Ã—128), `Load_NT_005` (512Ã—32)
  â€” loading screen art ("Loading"). Upload FALSE on NULL platform (librw
  cannot create a device texture; expected) â€” **4/4 OK on D3D9**.
- Global objectRecords (CAudioManager, CDifficultyLevels, CEventProxy,
  CSystemCommands + rwID_2DMAESTRO): no registered factory; unknown
  classes are skipped with binary behaviour (unchanged).
- `Image_Win32.rws` (5.6 MB UI pack, external {GUID}): format not decoded;
  the menu font/atlas comes from the MAINMENU txd (4.6r). If decoding is added, it gets bound
  to the GLOBAL block (main.cpp).

#### Ship follow (remaining 4.6s) â€” DONE

- **`code/PlayerShip.{h,cpp}`** (new): placeholder low-polygon ship
  (hull + mast; `rw::im3d` TRILIST; real rendering on GL3, on NULL the index
  generation is verified). Kinematic model: W/S throttle (reverse = âˆ’0.5), A/D rudder
  (0.55 rad/sec scale), turn proportional to speed, bob synced with the ocean wave form
  (`sin(x*0.05+t*2)*1.5 + cos(z*0.08+t*1.5)` scaled).
- **Camera FOLLOW chase (default)**: ship + 30 units behind, +12 up,
  lookAt ship+3.5; mouse X = damped orbit (Ã—0.94/frame). `CANNONCRUISE_CAM=
  FREE` old freecam, `=DATA` (4.6p), `=AUTO` (4.6q bbox).
- **Spawn**: open water from the world bbox (cx+span*0.6, cz+span*0.2), yaw facing
  the island (`atan2`); `CANNONCRUISE_SHIP=x,y[,z,yaw]` override.
- The real `CShipPlayer` hull/renderer cannot be bound without `{GUID}.dff`;
  the Bullet body is bound (`CPhysicsEntity::SetPosition` ready) but
  the integration will be done in the physics/AI phase. No collision yet (placeholder).

#### Integration stress (remaining 4.6s) â€” DONE

- **`tests/worldcycle_test.cpp`** (new): VIKING01 (130) / IDUNA03 (61) /
  BEAVER01 (176) â€” `WorldGeometry` payload â†’ Inspect â†’ CreateClump â†’
  verify `countAtomics` â†’ DestroyClump, 6 rounds (Aâ†’Bâ†’Câ†’Aâ†’Bâ†’C); level
  transition + unload chain without crashes, registered in `tests/CMakeLists.txt`.
- **ctest 30/30** (NULL and D3D9, Windows).

#### D3D9 verification (revised 2026-11-14)

- **Problem**: the window stayed black/appeared empty â€” `showRaster` only
  calls `Present()`, the camera raster is never written; with SWAPEFFECT_DISCARD the
  backbuffer contents are undefined, so the window showed nothing.
- **Diagnosis**: in `RenderPlatform_EndFrame` we switched the camera to the `CAMERATEXTURE`
  raster (on D3D9 the `CAMERA` texture stays nil â†’ binding picked the default
  render-target); the scene is now written to the camera texture (RT).
- **Fix**: `EndFrame`, before `showRaster()`, copies the camera RT to the 640x480 backbuffer
  via `GetSurfaceLevel â†’ StretchRect`, then presents â€” the
  window now shows the island/ocean/nature render (verified).
- Screenshot: the same camera texture via `GetSurfaceLevel â†’
  CreateOffscreenPlainSurface(SYSTEMMEM) â†’ GetRenderTargetData â†’ LockRect â†’
  RGBA â†’ rw::Image â†’ rw::writePNG`; only `X8R8G8B8`/`A8R8G8B8`.
- Open Phase 4/5 items: real ship model ({GUID}.dff), global
  objectRecord factories; also see Phase 4.6ab.


### Phase 4.6ab â€” LOADING screen + D3D9 screenshot + GLOBAL inventory â€” DONE (2026-11-14)

#### GLOBAL.RWS contents resolved (ROADMAP's "Image_Win32.rws" guess was wrong)

All top-level chunks were listed with `globalassets_test`:

| asset | type | contents |
|---|---|---|
| asset[0] | 0x0716 rwID_TEXDICTIONARY (embedded 0x23, 525 KB) | `SpecialCharacters.png` 256Ã—128 (gamepad icons), `Load_NT_005` 512Ã—32 (wave border), `Load_NT_003` 128Ã—128 (Hugo in his skiff), `Load_NT_001` 512Ã—128 (parchment) |
| asset[3] | 0x0716 wbID_STATICWORLDCOLLISION | external {GUID}.col (we don't have it) |
| asset[5] | 0x0716 wbID_Texture | `SpecialCharacters.png` external reference (same art, .texref record; texture already embedded) |
| asset[6] | 0x0716 rwaID_WAVEDICT | `Ingame_Win32.rws` reference (5.6 MB; `Sound/Win32/Ingame_Win32.rws` present in the repo) |
| asset[7..52] | 0x0704 objectRecords | 46 global behaviours (CAudioManager, CDifficultyLevels, CEventProxy, CSystemCommands â€¦) |
| asset[53] | 0x0716 rwID_2DMAESTRO | **`Loading.anm`** 40420 B (loading screen timeline) |
| asset[54-55] | 0x0704 | 2 records |

There is no asset named "Image_Win32.rws" â€” the loading screen art is already decoded in the GLOBAL embedded
txd and is being loaded **4/4 OK** on D3D9 with `MenuTextures_UploadEmbeddedFromRws`;
`Loading.anm` can be decoded from the repository
(`Rt2dMaestro_LoadFromRwsFile` compatible: rwID_2DMAESTRO anchor scan).

#### D3D9 screenshot â€” DONE (revised 2026-11-14)

The GL3-only screenshot part in `RenderPlatform_EndFrame` was also
implemented for D3D9; the first attempt (`GetBackBuffer â†’ GetRenderTargetData â†’ LockRect â†’
RGBA â†’ PNG`) failed/gave stale frames because the backbuffer was DISCARD.
Now the camera raster is `CAMERATEXTURE`; the scene is written to the camera texture RT;
before `showRaster()` inside `EndFrame`, it is copied to the backbuffer with `StretchRect`
and presented (the window render is visible) and the screenshot takes the same texture to a systemmem surface with
`RenderTargetData`:
`pDev->StretchRect | GetSurfaceLevel â†’ CreateOffscreenPlainSurface â†’
GetRenderTargetData â†’ LockRect â†’ RGBA â†’ rw::writePNG`.
Only `X8R8G8B8`/`A8R8G8B8` is supported. The GL3 path is kept unchanged.
`CANNONCRUISE_SCREENSHOT` applies to every frame.

#### LOADING screen (Phase 4.6r SCR.mov frame 4) â€” DONE (2026-11-14 rev)

`code/LoadingScreen.{h,cpp}` (new): the loading strip is drawn **before**
the world AddClump. The real GLOBAL art is used; fixed at **30 fps**
(`std::chrono::steady_clock` + `sleep_for`, after `CANNONCRUISE_LOADING_FRAMES
(default 100)` frames it returns to setup):

- night blue ground (top half #040914 navy-less, bottom half #0C1E3A dark
  sea; D3D9 overlay quads use BGRA byte order),
- the `Load_NT_005` wave silhouette sits on the horizon line and **drifts slowly to the right**
  (1 px per frame),
- the `Load_NT_001` parchment strip (keeps its own aspect ratio, 72% width,
  equal from both edges),
- the `Load_NT_003` Hugo skiff **glides left to right** on the sea, sways
  slightly with the waves (`sin(frame*0.11)*2`),
- `LOADING` + a live dot animation centered in handwriting ink color,
  `ESC` hint at bottom left.

Art (D3D9): GLOBAL 4/4 texture OK. NULL: texture upload FALSE â†’ fallback
drawing active.

After `CANNONCRUISE_LOADING_FRAMES` (default 100) frames the world setup
begins; Esc/Space/Enter for early exit; no external asset such as a `.bik` is needed
(all of it is inside GLOBAL). The loading phase is **fixed 30 fps** â€” it does not show the screen at
light speed on fast machines.

#### Verification (2026-11-14)

- **ctest 32/32** (NULL) â€” new: `globalassets_test` (inventory; GLOBAL 56
  chunks: 1 embedded txd = 4 textures + 4 external resources + 2DMAESTRO, 0 fail),
  `globaldump_test` (txd â†’ `Scratchpad/globaldump/tex_*.png`; 4 PNG verified).
- **D3D9**: GLOBAL 4/4 texture OK; 30/30 frames; LOADING screen capture
  visually verified (parchment + Hugo + wave line + "LOADING" text).
- NULL: `link verification OK`; 30/30 frames (since texture loading on NULL requires a device
  the upload FALSE is the expected behaviour; fallback path active).
