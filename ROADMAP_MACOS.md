# CannonCruise — macOS Roadmap

> **Last updated:** 9 October 2026  
> **Status:** Plan ready; the local macOS application has not been built or run yet.  
> **Related documents:** [Main roadmap](ROADMAP.md) · [Build foundation](BUILDING.md) · [SDK interface review](SDK_INTERFACE_AUDIT.md)

## 1. Goal and scope

This document defines a **native macOS port** path, separate from the Windows/MSVC rebuild plan. The first goal is a clean macOS static library and SDK-independent tests; the ultimate goal is a macOS application that opens a window, loads assets, and runs a single scene with render/physics.

- Running the original PE32/x86 `CannonCruise.exe` directly on macOS is not the goal of this plan. Running it via Wine/CrossOver or a virtual machine is a separate compatibility effort.
- Instead of preserving the Windows x86 binary ABI, source-level portability is targeted. The fixed-width layouts of asset/file formats must also be preserved.
- **Recommended order:** refresh the existing x86_64 evidence with a clean build → build arm64 and test on a real Apple Silicon machine → package Universal 2 if all dependencies are suitable. This order is not a support guarantee.
- The minimum macOS version and distribution architectures will be fixed after the graphics/physics backend decision. Universal 2 will only be targeted if both architectures are verified.
- The backend options below are suggestions; this document is not an SDK selection, installation permission, or a claim of a finished application.

## 2. Starting state and evidence limits

The records of the 9 October SDK review are the basis; no new build/test was performed while preparing this document.

- `code/CMakeLists.txt`: C++14, only the `CannonCruiseLib` static target. No macOS executable target, `.app` bundle, or real SDK links.
- The non-Windows branch excludes `framework/startup/win32/*`, `framework/startup/startup.cpp` and `Ocean_W32.cpp`. Excluding them from the build does not mean these functions have been ported to macOS.
- The previous `code/build/` cache/archive exists; it is not a clean-build proof against current sources. Existing outputs must be preserved.
- In the recorded Clang syntax pre-check, **87/87 non-Windows sources passed**. Object generation, archiving, linking and runtime are not verified.
- The `AnimController.cpp` integer → pointer conversion and the `SceneManager.cpp` format argument warnings remain.
- The extra Ocean check failed; 2/3 of the isolated startup checks passed. The general SDK pre-check still fails; the remaining gaps are Im3D, Open/Init contracts.
- The Havok local allocator Debug/Release ASan+UBSan tests passed on x86_64. arm64, SDK integration and real physics simulation are not verified.
- In the previous worker review, Clang/Python were usable, CMake/Ninja were not found. This is a historical finding; tool availability should be re-checked when moving to implementation.

**Status language:** present in source ≠ syntax passed ≠ compiled ≠ linked ≠ verified at runtime. All of the implementation steps below are pending unless stated otherwise.

## 3. M0 — Toolchain and clean build baseline (P0)

**Goal:** obtain a repeatable macOS build independent of the old cache.

- [ ] Discover AppleClang/clang++, CMake and the build tool to be selected in the worker; verify Xcode/Command Line Tools and macOS SDK access. Get user approval before any missing installation or PATH change.
- [ ] Consider C++14 and CMake 3.25+ as the starting baseline; record exact tool versions. The project's current CMake syntax minimum is 3.16.
- [ ] Record source revision/local changes, macOS version, SDK, compiler, generator, architecture and deployment target values.
- [ ] Configure outside `code/`, in new `build/macos-x86_64-<run>/debug` and `release` directories inside the repo; do not use or delete the old `code/build/` cache.
- [ ] Explicitly select the architecture with `CMAKE_OSX_ARCHITECTURES=x86_64`; use separate directories and `CMAKE_BUILD_TYPE` for Debug/Release with the single-configuration generator.
- [ ] Compare the real CMake source list against the pre-check script; record that generated CompilerId sources do not enter the target and the platform exclusions.
- [ ] Run `CannonCruiseLib` Debug/Release builds; verify configure/build logs, exit codes and archive architecture.

**Acceptance gate:** two clean configurations and builds succeed, archives are x86_64, source scope and tool versions are recorded. This gate does not require resolving external symbols or the game running; it does not prove them.

## 4. M1 — 64-bit portability and test baseline (P0)

**Dependency:** M0; source review and independent tests can run in parallel with M0.

- [ ] Prove whether the animation ID in the `AnimController.cpp` conversion is a pointer or a handle; do not hide the warning by only adding a cast. Fix the `SceneManager.cpp` format issue with the expected file path test.
- [ ] Scan pointer/handle conversions, `int`/`long` assumptions, struct packing, alignment and file read/writes. Use pointer width for memory addresses; verified fixed widths for disk formats.
- [ ] Test case-sensitive file systems, Windows backslashes, working directory, user data paths and Unicode file names.
- [ ] Wire the allocator test into CTest; re-run Debug/Release and supported ASan+UBSan runs on x86_64. Keep SDK-independent tests separate from the game link.
- [ ] Create separate clean build directories for arm64; verify builds and tests on an Apple Silicon machine. Do not count cross-compiling or running x86_64 under Rosetta as a native arm64 test.
- [ ] Report explicitly that the 32-bit overflow test does not run on the 64-bit target; add the corresponding limit/size tests for macOS.

**Acceptance gate:** Debug/Release builds and independent tests succeed on each targeted architecture; no sanitizer errors in the handled code; remaining warnings in a justified list. If there is no arm64 evidence, mark only x86_64 as verified.

## 5. M2 — SDK/backend decision and platform boundary (P0, critical path)

**Goal:** choose the path from header stubs to a real implementation. Inventory can run in parallel with M0/M1.

- [ ] Verify whether licensed RenderWare and Havok sources/SDKs supporting the target macOS/architecture exist; do not treat old Windows `.lib`/DLL files as a macOS link solution.
- [ ] If no native SDK is suitable, scope a compatibility layer that implements the used API subset. Keep SDK headers, build stubs and backend implementations in separate targets/include paths.
- [ ] Compare a Metal-based implementation for rendering with an abstraction library that supports macOS; list Im2D/Im3D, camera, world/geometry, texture, animation, plugin and stream coverage. Take OpenGL's deprecated status on macOS into account for the long-term choice.
- [ ] For physics, compare locally available licensed Havok with alternative backend options in terms of behavior, collision, constraint, ownership and asset compatibility. Do not assume the alternative backend is equivalent to the original game.
- [ ] Evaluate Cocoa or an SDL-like layer for window/input; define the platform-independent boundary of time, file system, settings storage and audio/video interfaces.
- [ ] Clarify the Im3D data layout/constants, global memory functions, the `RwEngineOpen` parameter and the `RwEngineInit` linkage/contract. Do not carry Win32 HWND details into the common macOS interface.
- [ ] Write the chosen backend, license/distribution conditions, architecture/minimum OS support and accepted behavior differences into a short decision record; clarify the implementation scope with the user.

**Acceptance gate:** the render/physics/platform approach is decided, legal access to the required resources and target architecture support are verified, and the API list to be implemented is known. Merely adding declarations or no-op functions does not close this gate.

## 6. M3 — macOS application shell and real link (P1)

**Dependency:** M0/M1 and the M2 platform decision.

- [ ] Separate the common engine lifecycle from the Win32 startup code; add a macOS entry point and a CMake executable target. Do not break the Windows path.
- [ ] Implement window open/close, event loop, keyboard/mouse, timing, resize, Retina pixel scale and focus changes.
- [ ] Use appropriate settings storage instead of the registry, bundle resource path and writable user data directories; do not write to the application bundle at runtime.
- [ ] Link the smallest application with real platform/backend implementations; derive the unresolved symbol inventory from the link log. Clearly separate test doubles and do not use them to hide missing functions of the game target.
- [ ] Verify startup failure and cleanup paths after partial initialization; add a small window/draw smoke test.

**Acceptance gate:** a native Mach-O application links; the window and basic drawing open, input/resize are handled, and it shuts down in a controlled way. This result is not the full game or full RenderWare coverage.

## 7. M4 — Render, assets and single scene (P1)

**Dependency:** M2 render decision and M3.

- [ ] Verify render state, matrix/camera, geometry, texture and Im2D/Im3D functions with small visual tests; record coordinate, winding, color, UV and alpha differences.
- [ ] Separate the portable mesh logic in `Ocean_W32.cpp` from platform details; draw Ocean through the selected backend instead of excluding it forever.
- [ ] Verify the RWS stream/chunk/plugin and texture/animation paths with real asset samples; check asset usage rights. Do not count loading a simple mesh as full asset compatibility.
- [ ] Complete scene loading, camera, GUI layers and real text drawing; track missing performance panel/text functions explicitly.
- [ ] Review audio/Bink video dependencies; if there is no native and properly licensed solution, clearly document and exclude the features that are off in the first single-scene test.

**Acceptance gate:** a scene with proper rights loads, the defined visual tests for camera/GUI/Ocean pass, missing features are listed. Visual comparison with the original game is only possible with a reference image and the same scene conditions.

## 8. M5 — Physics, game loop and behavior verification (P1)

**Dependency:** M2 physics decision and M3; SDK-independent simulation tests can run in parallel with M4.

- [ ] Implement real physics world creation/destruction, body/phantom add/remove and ownership rules.
- [ ] Complete the `CPhysicsEntity` position/velocity getter-setters so they return real results; keep the `void*` boundary behind a typed adapter.
- [ ] Verify physics event subscription and message filtering; use a documented fixed time step and render timing instead of an unconditional update on every message.
- [ ] Add simulation tests for gravity, collision, velocity/position, object removal and different render rates.
- [ ] Test player control, physics/render sync, pause/resume and clean shutdown in a single scene; record tolerances against reference behavior.

**Acceptance gate:** the simulation actually advances; not a local vector record/no-op. The single-scene test passes, results and known backend differences are reported.

## 9. M6 — Packaging and continuous validation (P2)

**Dependency:** M4/M5; the basic build/test CI job can start after M1.

- [ ] Add clean Debug/Release builds in macOS CI, CTest, appropriate sanitizer runs and per-architecture reports. Do not put licensed SDKs and assets into unauthorized runners/artifacts.
- [ ] Arrange the `.app` bundle, `Info.plist`, resource paths, dynamic library rpath/install-name and license notices.
- [ ] Verify separate x86_64/arm64 outputs; if Universal 2 is wanted, check that all native dependencies contain both architectures and test on two hardwares.
- [ ] Do a launch test on the selected minimum macOS version and outside the development machine; verify it does not depend on external development SDK paths.
- [ ] If external distribution is required, run the code signing/notarization process separately with developer account/certificate access and user approval. Report the local unsigned build and the distribution-ready package separately.

**Acceptance gate:** a reproducible `.app`, supported architecture/OS matrix and documented known limitations; if distribution is targeted, signing/notarization verification is also complete.

## 10. Next concrete work

1. **M0:** re-check tool availability; if missing, ask for approval and perform the clean x86_64 Debug/Release static build with logs.
2. **M1:** address the two existing warnings with proven fixes and regression tests; wire the allocator test into CTest.
3. **M2:** record the RenderWare/Havok macOS access and backend options in a decision record; do not grow stub scope before choosing the real implementation path.

Waiting for the Windows build to finish is not a prerequisite for M0/M1. The scope and duration of M3–M5 depend on the SDK/backend decision; no working-game date or completion percentage is given today. The creation of this file is only **planning output**; no build, installation or code port has been performed.
