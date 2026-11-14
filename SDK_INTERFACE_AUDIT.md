# Havok / RenderWare Interface Audit

> 9 October 2026 · P0 continuation · **SDK integration and the Windows build are not complete.**

## Scope and method

- The sources and the nine local RenderWare stub headers under `code/` were reviewed; the old `code/build/` outputs were not used.
- AppleClang 21.0.0, x86_64 was available in the worker. `cmake`, `cl` and `ninja` were not found. No installation, PATH change, external SDK access or Windows run was performed.
- `tools/check_sdk_interfaces.py` checks **87 source files** with `-std=c++14 -fsyntax-only`, modeling the current non-Windows CMake exclusion rules. It does not run CMake and does not produce objects/static archives.
- In addition, `Ocean_W32.cpp` (excluded by the macOS CMake target) is checked directly; this file does not require the Windows header, so checking is possible. The other three Windows sources (`startup.cpp`, `win.cpp`, `rwreg.cpp`) are skipped.
- Startup declarations/calls taken from the source text are compared against `rwcore.h` in small isolated checks. No fake `windows.h` is used; **these are not startup.cpp or a Windows/MSVC build.**
- First measurement: 87/87 non-Windows sources passed; Ocean and the three isolated checks failed. Full commands, diagnostics and source SHA-256 values are preserved in `validation/sdk-before.json`.

## 1. RenderWare: build blockers

### RW-01 — Im3D and global memory API missing — OPEN

Evidence: `code/Ocean_W32.cpp:41–63`; local `code/rwcore.h` does not declare this usage set:

- Type: `RwIm3DVertex`.
- Vertex helpers: `RwIm3DVertexSetPos`, `RwIm3DVertexSetRGBA`, `RwIm3DVertexSetU`, `RwIm3DVertexSetV`.
- Drawing: `RwIm3DTransform`, `RwIm3DRenderPrimitive`, `RwIm3DEnd`.
- Global memory calls: `RwMalloc`, `RwFree`.
- Constants: `rwRENDERSTATESHADEMODE`, `rwSHADEMODEGOURAUD`, `rwIM3D_VERTEXUV`, `rwIM3D_VERTEXRGBA`.

The first Ocean check produced 14 Clang errors; some are the cascading result of the first missing type. The error count must not be read as the number of independent gaps.

Although `RWS::CMemoryHandler::RwFree(RwFreeList*, void*)` exists, it is not the global `RwFree(void*)`; it cannot substitute because of the different scope and signature. `RwAlloc` is also not a proven SDK implementation for `RwMalloc`.

**Required decision:** will verified SDK headers be used, or only a build-only, explicitly separated stub layer? Estimated definitions were not added before the vertex layout, color packing, flag/constant values and memory callback ABI were verified with SDK/binary evidence. In particular, it must be checked separately whether the name `rwIM3D_VERTEXRGBA` used in the source actually exists in the target SDK.

### RW-02 — Engine shutdown declarations clash on return type — FIXED LOCALLY

The `extern void RwEngineTerm/Stop/Close(void)` declarations in `startup.cpp` clashed with the `RwBool` returns in `rwcore.h`. The first isolated check produced three real Clang errors.

The three duplicate declarations were removed; the common header became the single declaration source. Calls and the shutdown order were not changed. This fix ensures local consistency; it does not verify the real SDK signature.

### RW-03 — RwVideoMode unknown — POINTER USAGE FIXED

`CameraInit` and `RwStartupInitialize` in `startup.cpp` only take `RwVideoMode*`. An opaque forward declaration was added to `rwcore.h`. No struct field/size was made up; creating values or accessing fields will require the real SDK definition later.

### RW-04 — Missing parameter in RwEngineOpen call — OPEN

`startup.cpp` calls `RwEngineOpen()`; the common stub declares `RwEngineOpen(void* param)`. The isolated call check fails.

Instead of hiding the error by adding `nullptr`, the real open-param structure, HWND/device binding and the meaning of the `subSystemConfig` data should be investigated. The address comments in the startup code are not sufficient by themselves to distinguish a wrapper from the SDK function.

### RW-05 — RwEngineInit and link declarations diverge — OPEN

- `rwcore.h`: `RwEngineInit(void*, RwInt32, RwMemoryFunctions*)`, inside `extern "C"`.
- `startup.cpp`: `RwEngineInit(void*, RwUInt32, RwUInt32)`, a separate local declaration.
- The `memoryhandler.h` example passes the callback table as the third argument while startup assumes a different contract.

The isolated `RwEngineInit(memFuncs, 0, 0)` check **passed** in Clang (`validation/startup-init.json`). This does not mean the two signatures are equivalent: a separately compilable overload does not eliminate the wrong/missing external symbol risk. Also, `RwEngineSetSubSystem` and `RwEngineSetVideoMode` are declared only in startup; the SDK contract and C/C++ linkage should be gathered in a single place.

## 2. RenderWare: link and ABI gates

Local headers provide declarations; they are not real SDK function implementations. Static source checking does not verify the real signatures/layouts/constants of `RwRenderStateGet`, matrix/camera accessors, plugin callbacks and stream functions.

In particular, the `RwMemoryFunctions`, `RwGlobals`, `RwFreeList`, `RwFrame`, Im2D/Im3D layouts; enum/chunk constants; calling convention, struct packing, x86 pointer size and CRT match must be compared before SDK integration. Local headers must not shadow the real SDK via include order.

Wrapper candidates declared and called in startup, but with no definition visible in the source scan:

- `HavokInit`, `FileSystemInit`, `TextureCacheInit`, `RwStartEngine`, `CameraInit`.
- `RwEngineSetConfig`, `SetLightAmbient`, `RenderPipelineInit`, `GameManagerInit`.
- `CameraShutdown`, `RenderPipelineShutdown`, `SubSystemShutdown`, `TextureCacheShutdown`, `GameManagerShutdown`, `ResourceShutdown`, `GeneralShutdown`.
- In the Win32 loop: `InitEngine`, `CleanupEngine`, `Idle`, `ResizeCameraRasters`, global `GetTime`.

`CMenuGetTimeDelegate::GetTime()` is not the same symbol as the global Win32 `GetTime()`. This list is the result of a source scan; **it is not the complete unresolved-symbol list produced by the linker**. The EXE target and a real link run are still pending.

## 3. Havok: local interface in use and behavior gaps

### HK-01 — No world lifecycle or SDK link — OPEN

`m_pHavokWorld` inside `CPhysicsWorld` is private `void*`; the constructor zeroes it, and there is no path in the current class that creates the world or assigns one to this member. The `stepDeltaTime` call in `Update()` is commented out. No real Havok header include or active `hkp*` type call is seen. The names in comments are not proof of the Havok version.

### HK-02 — Body/phantom operations are only vector records — OPEN

- `AddRigidBody` / `RemoveRigidBody` / `RigidBodyExistsInWorld`: membership in a `std::vector<void*>`.
- `AddPhantom` / `RemovePhantom`: membership in another vector.
- Entity add/remove into the SDK world, refcount, ownership, lock/thread rules and collision filters are not applied.
- `CPhysicsEntity::SetRigidBody` only assigns a pointer; `AddToWorld` / `RemoveFromWorld` go to the local record layer.
- `SetPosition`, `GetPosition`, `SetLinearVelocity`, `GetLinearVelocity` only assert. In particular, the getters do not fill the output vector; valid physics results cannot be accepted.

### HK-03 — Physics event wiring missing — OPEN

`CPhysicsWorldBehaviour::HandleEvents` calls `Update(1/60)` on every delivered message without testing the message ID; there is no physics message subscription in the constructor. Because the physics step is already a no-op, this does not currently advance the simulation. No new ID was made up until the event ID/subscription contract is proven.

### HK-04 — Local allocator pointer mask — FIXED / TESTED

The old `~(alignment - 1)` operation stayed at `RwUInt32` width. On x64 it could zero the high pointer bits. Example: when aligning the `0x100000018` address to 16, the old expression gives `0x20`, the wide mask gives `0x100000020`; verified with an isolated compile-time assert.

- The mask was moved to `uintptr_t` width before the complement operation.
- An allocation size add overflow check was added; the addition is done in `size_t`.
- The existing 16-byte contract was preserved; unsupported alignment returns `nullptr` in Release as well.
- `Initialize/Shutdown` are still no-ops. This change is not a Havok callback registration or physics integration.
- Misleading allocator and world descriptions were fixed to match the actual behavior.

`tests/havok_memory_manager_test.cpp`: allocation of 0, 1, 15, 16, 17, 4096 and 1048576 bytes; alignment; writing/freeing the allocated area; null-free; invalid alignment checks in Release. **Debug and Release passed with ASan+UBSan.** The 32-bit overflow test exists in the test file, but did not run on the x64 run; it must be verified on a Windows/x86 run.

The first test run caught a comment-editing error (`validation/allocator-tests.json`); the error was fixed and the successful re-run was written to `validation/allocator-tests-fixed.json`. The interim source scan was stopped so it would not produce mixed results with changed sources; `sdk-after.json` was not created. Failure evidence was not deleted.

## 4. Re-running

Clang pre-check (at the repo root, the tool must be available beforehand):

```sh
python3 tools/check_sdk_interfaces.py --output validation/sdk-next.json
```

Does not overwrite the existing report. Because of the open Ocean/Open blockers, **a nonzero exit is expected**; this must not be presented as success. The script models the CMake source scope; if CMake changes, its mapping must be updated separately.

Example for the SDK-independent allocator test (repo root; use new output names):

```sh
mkdir -p build/sdk-audit-tests
clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -Icode \
  tests/havok_memory_manager_test.cpp code/HavokMemoryManager.cpp \
  -o build/sdk-audit-tests/havok-memory-next-debug
./build/sdk-audit-tests/havok-memory-next-debug
# For Release, add -DNDEBUG -O2 and a different output name to the same command.
```

## 5. Final validation result

`validation/sdk-final.json` was re-run with the latest sources; recorded source hashes match the current files:

- **87/87** non-Windows sources passed the syntax check. No warning-free build is claimed: the warnings at `AnimController.cpp:43` (integer to pointer conversion) and `SceneManager.cpp:81` (format argument) remain and were not addressed in these changes.
- **Ocean: failed.** RW-01 open.
- **Isolated startup checks: 2/3 passed.** The return type clashes and the opaque video type were fixed; the Open call continues to fail.
- **Pre-check overall result nonzero**; the Windows gate is not complete. Three Windows sources are outside the check.
- `validation/allocator-tests-fixed.json`: Debug and Release compile/run outputs are zero; no ASan+UBSan diagnostics. The test and source hashes in this log also match the current files.
- Document links and code blocks were also checked. No CMake configuration, static game library, Windows/MSVC, SDK link or game runtime tests were performed.

## 6. Next acceptance gates

1. Clarify the RenderWare version and the licensed SDK/build-stub strategy to be used; close RW-01, RW-04 and RW-05 with a verified contract.
2. Run the [BUILDING.md](BUILDING.md) recipe on a Windows machine; record `configure.log`, Debug/Release logs, tool versions and the Win32/SDK selection. Do not report as if MSVC had been run on macOS.
3. Determine the Havok backend choice, world/body ownership and event contract; do not count silent no-op physics as running simulation.
4. Later: the EXE/link gate, real SDK ABI comparison and single-scene runtime validation.
