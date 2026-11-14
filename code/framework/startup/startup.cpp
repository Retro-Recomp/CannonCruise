// ============================================================================
// CannonCruise - Startup Manager (startup.cpp)
// Original file: D:\Projects\CannonCruisePC\code\framework\startup\startup.cpp
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
//
// FUN_0041a680 — RwStartup::Initialize()
// FUN_0041ab90 — RwStartup::Shutdown()
// FUN_0041a640 — RwStartup::SetLightAmbient()  (helper)
//
// This file manages the top-level startup and shutdown 
// process of the RenderWare 3.6 engine. It is called after the Win32 window is created.
//
// Binary Flow (FUN_0041a680):
//   1. Havok physics system singleton check (DAT_00635af4)
//   2. File system initialization (FUN_005ca6c0)
//   3. Texture dictionary/cache initialization (FUN_00428de0)
//   4. RwEngineInit (FUN_0050b640) — engine core
//   5. RwEngineSetSubSystem (FUN_004cab40) — graphics subsystem
//   6. RwEngineOpen (FUN_00422960) — open the engine
//      -> On failure assert: "RenderWare Audio not initialized"
//   7. Video mode & resolution settings
//   8. RwEngineStart (FUN_00418ac0) — start the engine
//   9. AttachPlugins (FUN_00419e40) — attach the plugins
//  10. RwEngineSetVideoMode (FUN_0050b440)
//  11. RwEngineSetCamera (FUN_0041af10) — create the camera
//  12. RwEngineStart completes (FUN_0050b340)
//  13. Set the ambient light (FUN_0041a640)
//  14. Start the render pipeline (FUN_0041c190)
//  15. Start the game manager (FUN_0044aaf0)
// ============================================================================

#include "attachplugins.h"
#include "win32/win.h"
#include "PhysicsWorld.h"   // CPhysicsWorld::Create() — sub_41A680 singleton step

#include <rwcore.h>
#include <rpworld.h>
#include <cassert>

// RenderWare graphics subsystem settings
// Global variables in the binary:
//   DAT_00612f0c = graphics depth (default: 16 bit)
//   DAT_00612f10 = z-buffer depth (default: 16 bit) 
//   DAT_00612f14 = fullscreen flag
//   DAT_00635acc = hwnd (window handle)
//   DAT_00635af4 = Havok singleton pointer
//   DAT_006356a0 = state flag
static int  s_colorDepth   = 16;    // DAT_00612f0c
static int  s_zBufferDepth = 16;    // DAT_00612f10
static int  s_fullScreen   = 0;     // DAT_00612f14
static HWND s_hWnd         = NULL;  // DAT_00635acc

// ============================================================================
// Forward-declared functions (from framework modules)
// ============================================================================

// FUN_00434fb0 — Havok physics system singleton creator
extern void HavokInit(void);

// FUN_005ca6c0 — File system initialization (RtFS or custom)
extern void FileSystemInit(void);

// FUN_00428de0 — Texture dictionary/cache system initialization
extern RwBool TextureCacheInit(void);

// FUN_00418ac0 — RenderWare engine startup (RwEngineStart wrapper)
extern RwBool RwStartEngine(void);

// FUN_0041af10 — Camera and viewport initialization
extern RwBool CameraInit(RwVideoMode* videoMode, int width, int height, int depth);

// FUN_0050b640 — RwEngineInit
// RwEngineInit is declared in rwcore.h; do not redeclare with a different
// parameter list (binary: memFuncs, flags(bit0), resEntrySize).

// FUN_0050b600 — RwEngineTerm (engine shutdown)
// RwEngineTerm is declared in rwcore.h; do not redeclare with a different return type.

// FUN_0050b440 — RwEngineSetVideoMode
extern RwBool RwEngineSetVideoMode(RwInt32 modeIndex);

// FUN_004cab40 — RwEngineSetSubSystem
extern RwBool RwEngineSetSubSystem(RwInt32 subSystemIndex);

// FUN_0050b340 — RwEngineStart
extern RwBool RwEngineStart(void);

// FUN_0050b3d0 — RwEngineStop
// RwEngineStop is declared in rwcore.h; do not redeclare with a different return type.

// FUN_0050b2f0 — RwEngineClose
// RwEngineClose is declared in rwcore.h; do not redeclare with a different return type.

// FUN_0050f660 — RwEngineSetConfig (near/far clip + fog)
// RwEngineSetConfig is declared in rwcore.h; do not redeclare.

// FUN_0041a640 — Create the ambient light and add it to the scene
extern void SetLightAmbient(void);

// FUN_0041c190 — Start the render pipeline (viewport, framebuffer etc.)
extern void RenderPipelineInit(RwUInt32 width, RwUInt32 height, RwUInt32 flags);

// FUN_0044aaf0 — Start the game manager (GameManager)
extern void GameManagerInit(void);

// FUN_0041ac00 — Camera/scene cleanup
extern void CameraShutdown(void);

// FUN_0041c270 — Render pipeline cleanup
extern void RenderPipelineShutdown(void);

// FUN_004cac40 — SubSystem cleanup
extern void SubSystemShutdown(void);

// FUN_004356f0 — Texture cache cleanup
extern void TextureCacheShutdown(void);

// FUN_0044aba0 — GameManager cleanup
extern void GameManagerShutdown(void);

// FUN_0040fd60 — Resource cleanup
extern void ResourceShutdown(void);

// FUN_00405670 — General cleanup (memory pool etc.)
extern void GeneralShutdown(void);

// Registry reading — resolution, color depth and fullscreen
// FUN_0059d4e0 — Registry object creator
// FUN_0059d4f0 — Open the registry ("Software\ITE\CannonCruise", READ, HKCU)
// FUN_0059d5b0 — Check whether a registry value exists
// FUN_0059d5f0 — Read a registry integer
// FUN_0059d590 — Close the registry

// ============================================================================
// RwStartup::Initialize (FUN_0041a680)
// Binary Address: 0x0041a680
// XREF: FUN_00418ce0:004190c2(c) — called from WinMain
//
// Parameters (thiscall — ECX = this):
//   param_1: void* memFuncs — memory functions (or callback table)
//   param_2: void* subSystemConfig — subsystem settings 
//   param_3: RwVideoMode* videoMode — video mode info (width, height)
//   param_4: RwUInt32 flags — engine flags (0x1000000)
//   param_5: int colorDepth — color depth
//   param_6: HWND hWnd — window handle
// ============================================================================
RwBool RwStartupInitialize(void* memFuncs, void* subSystemConfig,
                           RwVideoMode* videoMode, RwUInt32 flags,
                           int colorDepth, HWND hWnd)
{
    (void)subSystemConfig;
    // -----------------------------------------------------------------
    // Step 1: Havok Physics Singleton Check
    // ASM: 0041a6a5  CALL FUN_00434fb0
    //       0041a6aa  CMP  [DAT_00635af4], 0
    //       0041a6b0  JZ   LAB_0041a7aa (jump if successful)
    // If the singleton was already created, this raises an assert error.
    // -----------------------------------------------------------------
    HavokInit();
    // assert(DAT_00635af4 == NULL) — "ms_pSingleton == NULL Failed" (Singleton.h line 0x33)
    // Binary sub_41A680 continuation: alloc 0x440 → ctor sub_49C870 → dword_635AF4
    // (CPhysicsWorld singleton) — created here; used by the behaviour ctor.
    CPhysicsWorld::Create();

    // -----------------------------------------------------------------
    // Step 2: Read Resolution Settings from the Registry
    // ASM: from HKCU "Software\ITE\CannonCruise":
    //   - "Width"  (0x613068) → default 640 (0x280)
    //   - "Height" (0x613058) → default 480 (0x1e0)
    //   - "Depth"  (0x613048) → default 16
    // -----------------------------------------------------------------
    RwUInt32 width  = 640;   // 0x280
    RwUInt32 height = 480;   // 0x1e0
    s_colorDepth    = 16;
    s_zBufferDepth  = 16;

    // TODO: Registry read (FUN_0059d4f0, FUN_0059d5b0, FUN_0059d5f0)
    // Read Width, Height, Depth from "Software\ITE\CannonCruise"
    // On success update the values, on failure use the defaults

    s_fullScreen = 1;
    s_hWnd = hWnd;

    // -----------------------------------------------------------------
    // Step 3: Initialize File System & Texture Cache
    // ASM: 0041a7f8  CALL FUN_005ca6c0  (FileSystemInit)
    //       0041a7fd  CALL FUN_00428de0  (TextureCacheInit)
    // -----------------------------------------------------------------
    FileSystemInit();
    TextureCacheInit();

    // -----------------------------------------------------------------
    // Step 4: RwEngineInit — Initialize the Engine Core
    // ASM: 0041a816  CALL FUN_0050b640
    //       0041a81e  CMP  EAX, 0
    //       0041a820  JNZ  (continue if successful)
    // On failure: RwEngineTerm + return FALSE
    // -----------------------------------------------------------------
    if (!RwEngineInit(static_cast<RwMemoryFunctions*>(memFuncs), 0, 0))
    {
        RwEngineTerm();
        return FALSE;
    }

    // -----------------------------------------------------------------
    // Step 5: Choose the Graphics SubSystem
    // ASM: 0041a830  CALL FUN_004cab40 — RwEngineSetSubSystem(-1)
    // -1 = default subsystem (DEFAULT)
    // On failure: RwEngineTerm + return FALSE
    // -----------------------------------------------------------------
    if (!RwEngineSetSubSystem(-1))
    {
        RwEngineTerm();
        return FALSE;
    }

    // -----------------------------------------------------------------
    // Step 6: RwEngineOpen — Open the Engine
    // ASM: 0041a851  CALL FUN_00422960
    //       0041a856  CMP  EAX, 0
    //       0041a858  JNZ  LAB_0041aa59 (continue if successful)
    // On failure: assert "RenderWare Audio not initialized" (line 0x88)
    // -----------------------------------------------------------------
    RwBool openResult = RwEngineOpen();
    if (!openResult)
    {
        // Assert: "RenderWare Audio not initialized" (0x006131b0)
        assert(openResult && "RenderWare Audio not initialized");
        return FALSE;
    }

    // -----------------------------------------------------------------
    // Step 7: RwEngineStart — Start the Engine
    // ASM: 0041aa66  CALL FUN_00418ac0
    //       0041aa6b  TEST EAX, EAX
    //       0041aa6d  JNZ  (continue if successful)
    // On failure: RwEngineTerm + return FALSE
    // -----------------------------------------------------------------
    if (!RwStartEngine())
    {
        RwEngineTerm();
        return FALSE;
    }

    // -----------------------------------------------------------------
    // Step 8: Attach the Plugins
    // ASM: 0041aa84  CALL FUN_00419e40 (AttachPlugins)
    //       0041aa89  TEST AL, AL
    //       0041aa8b  JNZ  (continue if successful)
    // On failure: RwEngineTerm + return FALSE
    // -----------------------------------------------------------------
    if (!AttachPlugins())
    {
        RwEngineTerm();
        return FALSE;
    }

    // -----------------------------------------------------------------
    // Step 9: Choose the Video Mode
    // ASM: 0041aab3  CALL FUN_0050b440 (RwEngineSetVideoMode)
    //       param_5 = colorDepth
    // On failure: RwEngineTerm + return FALSE
    // -----------------------------------------------------------------
    if (!RwEngineSetVideoMode(colorDepth))
    {
        RwEngineTerm();
        return FALSE;
    }

    // -----------------------------------------------------------------
    // Step 10: Create the Camera and Set Up the Viewport
    // ASM: 0041aae5  CALL FUN_0041af10 (CameraInit)
    //       Parameters: videoMode, width, height, depth (from ESI)
    // On failure: RwEngineStop + RwEngineTerm + return FALSE
    // -----------------------------------------------------------------
    RwBool cameraOk = CameraInit(videoMode, width, height, colorDepth);
    if (!cameraOk)
    {
        RwEngineStop();
        RwEngineTerm();
        return FALSE;
    }

    // -----------------------------------------------------------------
    // Step 11: Complete the Engine Startup
    // ASM: 0041ab0b  CALL FUN_0050b340 (RwEngineStart)
    // On failure: RwEngineStop + RwEngineTerm + return FALSE
    // -----------------------------------------------------------------
    if (!RwEngineStart())
    {
        RwEngineStop();
        RwEngineTerm();
        return FALSE;
    }

    // -----------------------------------------------------------------
    // Step 12: Z-Buffer & Clip Plane Configuration
    // ASM: 0041ab26-0041ab37  load float constants:
    //   0x3727c5ac ≈ 0.00001f (near clip)
    //   0x3727c5ac ≈ 0.00001f (near clip duplicate)
    //   0x3c23d70a ≈ 0.01f    (far fog)
    // 0041ab3f  CALL FUN_0050f660
    // -----------------------------------------------------------------
    // Binary (0041ab26-0041ab3f): the {1e-5, 1e-5, 0.01f} struct is written to the stack
    // and FUN_0050f660 is called with a single pointer (Ghidra: undefined4 *param_1).
    {
        RwEngineConfig engineConfig = { 0.00001f, 0.00001f, 0.01f };
        RwEngineSetConfig(&engineConfig);
    }

    // -----------------------------------------------------------------
    // Step 13: Set the Ambient Light
    // ASM: 0041ab4a  CALL FUN_0041a640
    // -----------------------------------------------------------------
    SetLightAmbient();

    // -----------------------------------------------------------------
    // Step 14: Start the Render Pipeline
    // ASM: 0041ab5d  CALL FUN_0041c190
    //       Parameters: width, height, flags (from ESI, EDI)
    // -----------------------------------------------------------------
    RenderPipelineInit(width, height, flags);

    // -----------------------------------------------------------------
    // Step 15: Start the Game Manager
    // ASM: 0041ab65  CALL FUN_0044aaf0
    // -----------------------------------------------------------------
    GameManagerInit();

    // ASM: 0041ab6a  MOV AL, 1  — successful return
    return TRUE;
}

// ============================================================================
// RwStartup::Shutdown (FUN_0041ab90)
// Binary Address: 0x0041ab90
// XREF: FUN_00418ce0:00419832(c) — called from WinMain (on exit)
//
// Shutdown order (reverse of the startup order):
//   1. CameraShutdown        (FUN_0041ac00)
//   2. RenderPipelineShutdown (FUN_0041c270)
//   3. RwEngineClose          (FUN_0050b2f0)
//   4. RwEngineStop           (FUN_0050b3d0)
//   5. RwEngineTerm           (FUN_0050b600)
//   6. SubSystemShutdown      (FUN_004cac40)
//   7. TextureCacheShutdown   (FUN_004356f0)
//   8. GameManagerShutdown    (FUN_0044aba0)
//   9. FileSystemInit         (FUN_005ca6c0) — called again (reset/cleanup)
//  10. ResourceShutdown       (FUN_0040fd60)
//  11. GeneralShutdown        (FUN_00405670)
// ============================================================================
void RwStartupShutdown(void)
{
    CameraShutdown();           // 0041ab91
    RenderPipelineShutdown();   // 0041ab96
    RwEngineClose();            // 0041ab9b
    RwEngineStop();             // 0041aba0
    RwEngineTerm();             // 0041aba5
    SubSystemShutdown();        // 0041abaa
    TextureCacheShutdown();     // 0041abaf
    GameManagerShutdown();      // 0041abb4
    FileSystemInit();           // 0041abb9 — file system reset
    ResourceShutdown();         // 0041abbe
    GeneralShutdown();          // 0041abc3
}
