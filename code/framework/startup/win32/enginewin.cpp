// ============================================================================
// CannonCruise - Win32 engine bridge functions (enginewin.cpp)
// Original file: unknown (engine-side symbols expected by win.cpp)
// Reverse-engineered and rewritten by analyzing with CC_ida.asm.
//
// win32/win.cpp expects these five symbols as extern; previously no source
// file defined them (Phase 4.4b link blocker). Binary equivalents:
//   FUN_0041C380 — Idle               (MainLoop::Poll; called by sub_4189E0)
//   FUN_00418130 — ResizeCameraRasters (WM_SIZE; MainWndProc jumptable case 5)
//   FUN_00417930 — CleanupEngine       (WM_CLOSE; MainWndProc jumptable case 16)
//   FUN_00482BE0 — GetTime             (MainLoop frame timer)
//   InitEngine  — AppMain's engine startup call; in the binary it corresponds to
//                 sub_41A680 = RwStartup::Initialize and was restored in startup.cpp
//                 as RwStartupInitialize.
//
// Note: This file only compiles on Windows (CMakeLists excludes win32/*);
//      build/verification cannot be done in the macOS session — it will be tried with
//      the Phase 4.4b recipe (BUILDING.md §4).
// ============================================================================

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>   // timeGetTime

#include "rwcore.h"
#include "eventhandler/ceventhandler.h"
#include "rwreg.h"

#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif

// startup.cpp (FUN_0041A680 — RwStartup::Initialize; compiles on Windows only)
extern RwBool RwStartupInitialize(void *memFuncs, void *subSystemConfig,
                                  RwVideoMode *videoMode, RwUInt32 flags,
                                  int colorDepth, HWND hWnd);

// ---------------------------------------------------------------------------
// GetTime (FUN_00482BE0)
// Binary: mov eax, dword_657AA8 ; retn
//   dword_657AA8 is a cached millisecond counter written by the logic step
//   (sub_41B900 → sub_481660). Since the logic step has not been restored until
//   Phase 6, the WINMM clock is read directly — same ms unit and
//   same contract as the binary (used by the frame rate limiter in MainLoop).
// ---------------------------------------------------------------------------
DWORD GetTime()
{
    return timeGetTime();
}

// ---------------------------------------------------------------------------
// Idle (FUN_0041C380) — "MainLoop::Poll"
// Binary sub_4189E0 (MainLoop) calls this every frame. Structural skeleton:
//   1. "MainLoop::Poll" profile section — sub_49C7D0 registration + atexit;
//      Begin sub_49C940 / End sub_49C9B0 (CRealtimeProfiler modeli)
//   2. Entity counter logs (on change): bird  dword_657178,
//      fish dword_6574E8, soldier dword_6582A0, flotsam dword_65763C,
//      ship dword_6580C0, towercatapult dword_658438,
//      "MAX CannonBall" dword_6572F4
//   3. "Logic" — the delay clocked from sub_428DE0 (TextureCacheInit / WINMM)
//      is captured; sub_41B900 fixed-step logic call (on ≥33ms delay,
//      1000 units/step, max 10000 units = 10 s cap)
//   4. "Render" — sub_41CEB0 render step (double-wrapped Begin/End)
//   5. If the engine singleton (dword_635AF4) exists, sub_49CA00 (display)
// The game logic/render sub-systems required for steps 2-5 do not
// exist in the codebase: it currently returns without doing any work.
// ---------------------------------------------------------------------------
void Idle()
{
    // TODO(Phase 6): steps 1-5 — profile sections (CRealtimeProfiler),
    // entity counter logs, logic step (sub_41B900), render step
    // (sub_41CEB0) and engine display (sub_49CA00).
}

// ---------------------------------------------------------------------------
// ResizeCameraRasters (FUN_00418130) — WM_SIZE
// Binary flow:
//   1. ctx = sub_41CDF0() = dword_6361B4 — the render target created
//      by sub_41CE00 (RenderPipelineInit step 1). If NULL, returns FALSE and the message
//      is not sent. This object will be restored in Phase 4.4b; for now,
//      the temporary equivalent: the rwstubs engine state machine must be in START.
//   2. If width/height are non-zero (not minimized): the current sizes at
//      ctx+68h/+6Ch are read; sub_510660 / sub_50E6E0 raster resize and
//      the sub_429B10 camera aspect/depth setting call are made
//      — TODO because the raster/camera objects have not been restored.
//   3. On every path (including zero size): "iMsgResizeCameraRasters" is registered temporarily
//      (loc_40BF90 = RegisterMsg), sent if there are subscribers (sub_40E0F0 =
//      SendMsg), the registration is deleted (loc_40CC60 = UnRegisterMsg).
// ---------------------------------------------------------------------------
BOOL ResizeCameraRasters(int width, int height, int depth)
{
    // Step 1 — ctx check (binary: dword_6361B4 NULL → FALSE, no message)
    if (!RwEngineIsRunning())
        return FALSE;

    if (width != 0 && height != 0)
    {
        (void)depth;
        // TODO(Phase 4.4b): sub_510660 / sub_50E6E0 — raster resize
        // resize (ctx+68h/+6Ch with current sizes); sub_429B10 —
        // camera aspect/depth. Raster and camera objects do not exist.
    }

    // Step 3 — iMsgResizeCameraRasters (binary: loc_40BF90 → sub_40E0F0 →
    // loc_40CC60); SendMsg is already a no-op when there are no subscribers.
    RWS::CEventId id =
        RWS::CEventHandler::RegisterMsg("iMsgResizeCameraRasters");
    if (id)
    {
        RWS::CMsg msg(id, 0, 0);
        RWS::CEventHandler::SendMsg(msg);
        RWS::CEventHandler::UnRegisterMsg(id);
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// CleanupEngine (FUN_00417930) — WM_CLOSE
// Binary: reads the window state with GetWindowPlacement and writes it to the rwreg root
//   (Software\Criterion\RenderWare\ — rwreg.cpp 0x00612e88) under the
//   GameFrameWork/GameSettings key:
//     PosX, PosY  — rcNormalPosition.left / .top (int)
//     SizeX, SizeY — width / height (int, converted to string with sub_417740 —
//                    RwRegSetInt does the same job)
//     Minimized   — showCmd == SW_SHOWMINIMIZED (2) → "true"/"false"
//     Maximized   — showCmd == SW_MAXIMIZE (3)      → "true"/"false"
//   There is NO engine shutdown: sub_417930 only saves the window state;
//   shutdown is a separate chain (RwStartupShutdown, FUN_0041AB90).
//   the path/subKey order could not be determined 100% from the sub_416B90 context;
//   since the read side (window restore) is not written yet, it will be
//   verified when the restore code is written.
// ---------------------------------------------------------------------------
void CleanupEngine(HWND hWnd)
{
    if (!hWnd)
        return;

    WINDOWPLACEMENT wndpl;
    ZeroMemory(&wndpl, sizeof(wndpl));
    wndpl.length = sizeof(WINDOWPLACEMENT);
    if (!GetWindowPlacement(hWnd, &wndpl))
        return;

    static const char *const kPath = "GameFrameWork";
    static const char *const kSubKey = "GameSettings";

    const RECT &rc = wndpl.rcNormalPosition;
    RwRegSetInt(kPath, kSubKey, "PosX", rc.left);
    RwRegSetInt(kPath, kSubKey, "PosY", rc.top);
    RwRegSetInt(kPath, kSubKey, "SizeX", rc.right - rc.left);
    RwRegSetInt(kPath, kSubKey, "SizeY", rc.bottom - rc.top);
    RwRegSetString(kPath, kSubKey, "Minimized",
                   wndpl.showCmd == SW_SHOWMINIMIZED ? "true" : "false");
    RwRegSetString(kPath, kSubKey, "Maximized",
                   wndpl.showCmd == SW_MAXIMIZE ? "true" : "false");
}

// ---------------------------------------------------------------------------
// InitEngine — win.cpp engine startup bridge (AppMain step 5)
// Binary: AppMain (FUN_00418ce0) → sub_41A680 = RwStartup::Initialize.
// The full chain was restored in startup.cpp as RwStartupInitialize:
//   HavokInit → FileSystemInit → TextureCacheInit → RwEngineInit →
//   RwEngineSetSubSystem(-1) → RwEngineOpen → RwStartEngine →
//   AttachPlugins → RwEngineSetVideoMode → CameraInit → RwEngineStart →
//   RwEngineSetConfig → SetLightAmbient → RenderPipelineInit → GameManagerInit
// win.cpp's 6 arguments are mapped in the bridge:
//   width/height/depth → RwVideoMode (values read from the startup registry;
//   already read in win.cpp AppMain), colorDepth = depth, hWnd directly.
// hInstance/bFullscreen are unused: the window has already been created
//   (sub_418700) and startup.cpp sets s_fullScreen to a constant 1, same as the binary —
//   the fullscreen flow is a separate work item (Phase 4.4b).
// ---------------------------------------------------------------------------
BOOL InitEngine(HWND hWnd, HINSTANCE hInstance, int width, int height,
                int depth, BOOL bFullscreen)
{
    (void)hInstance;
    (void)bFullscreen;

    RwVideoMode videoMode;
    ZeroMemory(&videoMode, sizeof(videoMode));
    videoMode.width = width;
    videoMode.height = height;
    videoMode.depth = depth;

    return RwStartupInitialize(0, 0, &videoMode, 0, depth, hWnd) ? TRUE
                                                                 : FALSE;
}
