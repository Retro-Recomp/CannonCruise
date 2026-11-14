# CannonCruise — Build Foundation

> **Update 2026-11-14:** this document is the historical P0 recipe. The Windows/MSVC
> build was verified with VS 18 (2026, Win32); see [README.md](README.md)
> and [ROADMAP.md](ROADMAP.md) for the current status, commands and the
> tracked stale `build_win32_v180/` cache warning. The "not run yet" texts
> date from 9 October 2026.
> 9 October 2026 · P0 / status and build recipe  
> Status: **Documented validation target; not yet run on Windows.**
> **For macOS:** [Separate macOS roadmap](ROADMAP_MACOS.md). The Win32/x86 target and PowerShell recipe below do not apply to the macOS port.

## 1. Goal and scope

The first goal is to build the `CannonCruiseLib` static library cleanly with Windows/MSVC. The existing `code/CMakeLists.txt` uses C++14 and only defines `add_library(... STATIC ...)`. It does not yet produce an EXE, does not link the real SDK libraries, and performs no runtime testing.

**Target architecture: Win32 / x86 (32-bit).** The PE header of the original `CannonCruise.exe` reads `Machine=0x014c` (i386) and optional-header magic `0x010b` (PE32). This is the rationale for choosing x86 as the first reconstruction target; it does not prove binary compatibility of the rewritten classes. The x64 port is outside the first acceptance scope.

### Toolchain selected for the first validation

These are the baseline for the first Windows attempt, not a claim of an existing setup or a successful build:

- Host: Windows 10/11 x64; target processor: x86.
- Visual Studio 2022 or Build Tools 2022, **Desktop development with C++** components.
- MSVC **v143**, x86/x64 build tools. The exact 19.xx version must be recorded in the first run's log; real SDK compatibility is not yet known.
- Windows SDK **10.0.22621.0**; explicitly selected in the command. If another version is required, a deliberate change and a new validation record are needed.
- CMake **3.25 or newer**; the `Visual Studio 17 2022` generator (requires CMake 3.21+). The project file's 3.16 minimum is the lower bound of the project syntax with old generators, not the minimum of this recipe.
- Language standard: C++14. First configuration: **Debug**, then **Release** with the same sources.
- Ninja/MinGW are not part of this first baseline. CRT selection is left to the existing CMake/MSVC default; before SDK linking, Debug/Release, `/MD`–`/MT`, calling conventions, packing and structure sizes must be decided separately.

Without pinning exact tool patch versions, no bit-level reproducibility is claimed. This recipe defines a repeatable clean-build process. Old proprietary SDKs may require a different compiler/ABI; v143 is not yet a choice verified for SDK compatibility.

## 2. Dependency inventory

### RenderWare / RenderWare Studio

- Local stub headers: `code/rwcore.h`, `rpworld.h`, `rpcollision.h`, `rphanim.h`, `rpskin.h`, `rtanim.h`, `rtcmpkey.h`, `rt2d.h`, `rt2danim.h`.
- `rwcore.h` declares itself a minimal 3.x type/signature stub. The 3.6 version mentioned in the project descriptions is not verified against the real SDK archive.
- Framework sources are under `code/framework/`. The Windows startup code needs external engine entry points.
- No `.lib` archive for RenderWare/Havok was found in the workspace; the `.a` found is a previous macOS game library. The existence of the real SDK elsewhere was not investigated.
- Stub headers give declarations to the compiler; they do not provide function implementations, correct layout, or binary compatibility. When the real SDK is added, the local stubs must not shadow the SDK headers via include order.

### Havok

- `PhysicsWorld.h` keeps world/body objects as `void*`; the `stepDeltaTime` call inside `PhysicsWorld.cpp` is commented out.
- `HavokMemoryManager.cpp` has an allocation helper; this is not physics SDK integration.
- Havok version, headers, x86 libraries and a compatible ABI are **not determined**. The exact SDK version must not be inferred from the `hkpWorld` comment in the source.
- Next step: inventory of the types/functions used and build stubs; the decision about a real SDK or a compatibility layer for actual simulation should be kept separate.

### Windows / graphics / other runtime components

- `windows.h`, Win32 window and registry sources require the Windows SDK.
- `Ocean_W32.cpp` currently uses the RenderWare Im3D API; there is no direct `d3d9.h` include. That the legacy DirectX SDK is required has not been shown yet; automatic installation must not be performed.
- The presence of D3D9, BINK32 and Windows DLLs under `DLLs/` does not prove the existence of development headers/import `.lib` files or a redistribution license. Do not copy them into system folders.
- `startup.cpp` contains `HavokInit`, `FileSystemInit`, `TextureCacheInit`, `RwStartEngine` and similar extern declarations. These are not the complete missing-symbol list; the inventory should be derived through an EXE target and a real link attempt.
- SDK license/access and the use of original assets must be verified separately. No download, installation or SDK selection was done in this P0 scope.

## 3. Existing local evidence and its limits

In the 9 October 2026 review:

- `code/build/CMakeCache.txt`: Unix Makefiles, `/usr/bin/c++`, empty `CMAKE_BUILD_TYPE`.
- `code/build/CMakeFiles/4.4.3/CMakeCXXCompiler.cmake`: AppleClang 21.0.0.21000101, x86_64 and 8-byte pointers.
- `code/build/libCannonCruiseLib.a`: present; its contents/match with current sources was not verified. The previous roadmap had reported the macOS build as successful; it was not re-run in this session.
- The non-Windows CMake branch excludes `framework/startup/win32/*`, `framework/startup/startup.cpp` and `Ocean_W32.cpp`. Therefore the macOS result does not test the Windows scope.
- Four generated `.c`/`.cpp` diagnostic files were found under `code/build/CMakeFiles/`. To avoid them accidentally entering the recursive source glob, `build/` subdirectories were excluded in CMake; previous outputs were not deleted or moved.
- This session's worker PATH had `python3` and `clang`; `cmake`, `cl` and `ninja` were not found. No installation or PATH changes were made. The stale CMake path in the cache is not evidence of availability for the running worker.

## 4. Clean Windows build recipe

**Prerequisite:** the tools above must be installed on the Windows machine. If they are missing, they must be installed first with user approval; the commands below do not perform installation. These commands are prepared for Windows and have not been validated in the current macOS session.

In the **Developer PowerShell for VS 2022** terminal of Visual Studio, switch to the repository root (not `code/`). Use a new build directory; do not reuse the existing `code/build` cache on Windows. All new outputs live under `build/` at the repository root, and the CMake source tree stays outside `code/`.

```powershell
# Working directory: the repository root containing ROADMAP.md and code/.
$ErrorActionPreference = 'Stop'
if (-not (Test-Path '.\code\CMakeLists.txt')) {
    throw 'Run the commands from the repository root.'
}
Get-Command cmake, cl, msbuild -ErrorAction Stop | Out-Null

# A unique, clean directory without deleting previous outputs.
$build = Join-Path $PWD ('build\win32-v143-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $build | Out-Null
cmake --version | Tee-Object -FilePath "$build\tools.log"
if ($LASTEXITCODE -ne 0) { throw 'CMake version query failed.' }
# Save the cl /Bv output separately along with the configure/build log.
# cl /Bv may return nonzero because it does not compile a source without arguments.
cmd /c 'cl /Bv 2>&1' | Out-File -Append "$build\tools.log"

cmake -S code -B "$build" -G 'Visual Studio 17 2022' -A Win32 -T v143 `
    -DCMAKE_SYSTEM_VERSION=10.0.22621.0 2>&1 |
    Tee-Object -FilePath "$build\configure.log"
if ($LASTEXITCODE -ne 0) { throw "Configure failed; inspect $build\configure.log." }

cmake --build "$build" --config Debug --target CannonCruiseLib --parallel 2>&1 |
    Tee-Object -FilePath "$build\build-debug.log"
if ($LASTEXITCODE -ne 0) { throw "Debug build failed; inspect $build\build-debug.log." }

cmake --build "$build" --config Release --target CannonCruiseLib --parallel 2>&1 |
    Tee-Object -FilePath "$build\build-release.log"
if ($LASTEXITCODE -ne 0) { throw "Release build failed; inspect $build\build-release.log." }
```

In Visual Studio's multi-configuration generator, `--config` is used instead of `CMAKE_BUILD_TYPE`. When the SDK or v143 cannot be found, do not silently change the version; record the missing component and the decision. For a repeat clean attempt, re-running the command block generates a new directory and preserves the old one.

**Expected outputs on success:** `$build\Debug\CannonCruiseLib.lib` and `$build\Release\CannonCruiseLib.lib`. These are static archives; they are not the runnable game. Because of the existing type/signature gaps, the first attempt may error out; the recipe guarantees no success.

## 5. Acceptance/recording criteria for the first Windows run

- [ ] Source revision and any local changes, Windows version, full CMake/MSVC versions, v143 and the selected SDK are recorded.
- [ ] Cache/project files confirm the Win32/x86 target and the `10.0.22621.0` SDK selection.
- [ ] The source list contains the Windows startup files and `Ocean_W32.cpp`; there are no CMake CompilerId sources.
- [ ] Configure and Debug/Release builds completed with zero exit codes; both `.lib` files are present.
- [ ] Logs are preserved, warnings and failures classified (header/dependency, type/signature, platform/ABI).
- [ ] `ROADMAP.md` 4.4b was updated only with this evidence; 4.5 link and runtime statuses were left separate.

When an executable target is added in the next stage, the link log, import libraries, unresolved symbols, SDK/CRT compatibility and application launch result must also be recorded. Static archive success does not mean working physics/render or binary equivalence with the original.

## 6. P0 continuation — SDK interface pre-check

[SDK_INTERFACE_AUDIT.md](SDK_INTERFACE_AUDIT.md) records the gaps, limited fixes and validation limits of the 9 October continuation review. A CMake-free Clang syntax check was added via `tools/check_sdk_interfaces.py`; it does not replace the Windows recipe.

- Non-Windows sources are evaluated separately, `Ocean_W32.cpp` as an extra check, and startup contracts in isolated fragments.
- The Im3D declarations and the parameterless `RwEngineOpen()` call are still open build blockers.
- The local Havok allocator test passed Debug/Release ASan+UBSan; it is not a Havok SDK or Windows/x86 test.
- When open blockers are found, the pre-check returns nonzero; source SHA-256, commands and diagnostics are preserved in the reports. It performs no installation.
