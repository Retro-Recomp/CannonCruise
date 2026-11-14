#include "win.h"
#include <windows.h>
#include <cassert>

// ============================================================================
// CannonCruise - Framework Startup Win32 (win.cpp)
// Original file: D:\Projects\CannonCruisePC\code\framework\startup\win32\win.cpp
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// ============================================================================

// Global Variables (memory addresses: DAT_00635acc, DAT_00635ad0, DAT_00635ad1)
HWND theMainWindow    = NULL;   // DAT_00635acc
static HINSTANCE ghInstance  = NULL;   // DAT_00635ac8
static BOOL bSuspended        = FALSE;  // DAT_00635ad0 (suspend when window loses focus)
static BOOL bQuit             = FALSE;  // DAT_00635ad1 (exit flag for the game loop)
static DWORD gLastFrameTime   = 0;      // DAT_00635690 (frame timer)

// Constants (from the binary string pool)
static const char* const APP_CLASS_NAME  = "CannonCruise";             // 0x005ef420
static const char* const APP_WINDOW_NAME = "CannonCruise";             // 0x005ef430
static const char* const APP_MUTEX_NAME  = "CANNONCRUISE";             // 0x00613094
static const char* const APP_ATOM_NAME   = "cannoncruise";             // 0x00612fe4
static const char* const REG_KEY_APP     = "Software\\ITE\\CannonCruise"; // 0x00613078
static const char* const REG_VAL_WIDTH   = "VideoResWidth";            // 0x00613068
static const char* const REG_VAL_HEIGHT  = "VideoResHeight";           // 0x00613058
static const char* const REG_VAL_DEPTH   = "VideoResDepth";            // 0x00613048

// External Engine Callbacks (startup.cpp / RenderWare engine linkage)
// NOTE: ResizeCameraRasters' REAL definition in enginewin.cpp returns BOOL
// (FUN_00418130). If the signature is written as 'void', MSVC produces a different mangled symbol
// → LNK2019 (?ResizeCameraRasters@@YAXHHH@Z vs @@YAHHH@Z).
extern void Idle();
extern BOOL ResizeCameraRasters(int width, int height, int depth);
extern void CleanupEngine(HWND hWnd);
extern BOOL InitEngine(HWND hWnd, HINSTANCE hInstance, int width, int height, int depth, BOOL bFullscreen);
extern DWORD GetTime();

// ----------------------------------------------------------------------------
// MainWndProc (0x00418230): Main Window Message Procedure
// ----------------------------------------------------------------------------
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ACTIVATEAPP:
        // When the app loses focus, stop rendering and enter wait mode
        bSuspended = (wParam == 0);
        return 0;

    case WM_SIZE:
        // When the window size changes, update the RenderWare camera rasters
        if (wParam != SIZE_MINIMIZED) {
            int width  = LOWORD(lParam);
            int height = HIWORD(lParam);
            ResizeCameraRasters(width, height, 32);
        }
        return 0;

    case WM_KEYDOWN:
        // Cheat / console key combination logging (A-Z keys)
        if (wParam >= 'A' && wParam <= 'Z') {
            // Internal buffer logging
        }
        return 0;

    case WM_CLOSE:
        // Clean up engine resources and terminate the window
        CleanupEngine(hWnd);
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        // End the message loop
        theMainWindow = NULL;
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }
    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

// ----------------------------------------------------------------------------
// InitApp (FUN_00418420): Registers the Window Class (RegisterClass)
// ----------------------------------------------------------------------------
BOOL InitApp(HINSTANCE hInstance) {
    // Assert check (0x00612fb8)
    assert(hInstance != NULL && "Failed PRE-condition: processInstance");

    ghInstance = hInstance;

    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.style         = CS_BYTEALIGNCLIENT;
    wc.lpfnWndProc   = MainWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIconA(hInstance, MAKEINTRESOURCE(101)); // IDI_MAIN_ICON (0x65)
    wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = APP_CLASS_NAME;

    return (RegisterClassA(&wc) != 0);
}

// ----------------------------------------------------------------------------
// CreateMainWindow (FUN_00418700): Creates the Main Game Window
// ----------------------------------------------------------------------------
HWND CreateMainWindow(HINSTANCE hInstance, int x, int y, int width, int height, BOOL bFullscreen) {
    assert(hInstance != NULL && "Failed PRE-condition: processInstance");

    DWORD dwStyle = 0;
    if (bFullscreen) {
        dwStyle = WS_POPUP;
    } else {
        dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        RECT rect = { 0, 0, width, height };
        AdjustWindowRect(&rect, dwStyle, FALSE);
        width  = rect.right - rect.left;
        height = rect.bottom - rect.top;
    }

    HWND hWnd = CreateWindowExA(
        0,
        APP_CLASS_NAME,
        APP_WINDOW_NAME,
        dwStyle,
        x, y,
        width, height,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    return hWnd;
}

// ----------------------------------------------------------------------------
// MainLoop (FUN_004189e0): Main Game Message and Frame Loop
// ----------------------------------------------------------------------------
int MainLoop() {
    MSG msg;
    while (!bQuit) {
        Sleep(0); // Yield the CPU time slice

        if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                bQuit = TRUE;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        } else {
            if (bSuspended) {
                // Wait to avoid burning CPU while the window is minimized or unfocused
                WaitMessage();
            } else {
                // Game draw and update step
                Idle();

                // Frame rate limiter (if frame time < 10ms, sleep -> Max ~100 FPS)
                DWORD currentTime = GetTime();
                DWORD elapsed = currentTime - gLastFrameTime;
                if (elapsed < 10) {
                    Sleep(10 - elapsed);
                }
                gLastFrameTime = GetTime();
            }
        }
    }
    return (int)msg.wParam;
}

// ----------------------------------------------------------------------------
// GetMainWindow & QuitApp (FUN_00418c70 & FUN_00418c80)
// ----------------------------------------------------------------------------
HWND GetMainWindow() {
    return theMainWindow;
}

void QuitApp() {
    if (theMainWindow) {
        PostMessageA(theMainWindow, WM_CLOSE, 0, 0);
    }
}

// ----------------------------------------------------------------------------
// AppMain (FUN_00418ce0): Application Startup and Configuration Logic
// ----------------------------------------------------------------------------
int AppMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 1. Check whether another copy is already running
    if (FindWindowA(APP_CLASS_NAME, APP_WINDOW_NAME) != NULL) {
        return -1;
    }

    // 2. Register the window class
    if (!InitApp(hInstance)) {
        return -1;
    }

    // 3. Read the video settings from the Registry
    int width  = 640;
    int height = 480;
    int depth  = 16;
    BOOL bFullscreen = TRUE;

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_KEY_APP, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwType, dwVal, dwSize = sizeof(dwVal);
        if (RegQueryValueExA(hKey, REG_VAL_WIDTH, NULL, &dwType, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
            width = (int)dwVal;
        }
        dwSize = sizeof(dwVal);
        if (RegQueryValueExA(hKey, REG_VAL_HEIGHT, NULL, &dwType, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
            height = (int)dwVal;
        }
        dwSize = sizeof(dwVal);
        if (RegQueryValueExA(hKey, REG_VAL_DEPTH, NULL, &dwType, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
            depth = (int)dwVal;
        }
        RegCloseKey(hKey);
    }

    // Hide the cursor in fullscreen
    if (bFullscreen) {
        ShowCursor(FALSE);
    }

    // 4. Create the main window
    theMainWindow = CreateMainWindow(hInstance, 0, 0, width, height, bFullscreen);
    if (!theMainWindow) {
        return -1;
    }

    // In windowed mode, center on the screen
    if (!bFullscreen) {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        int posX = (screenW - width) / 2;
        int posY = (screenH - height) / 2;
        SetWindowPos(theMainWindow, NULL, posX, posY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    ShowWindow(theMainWindow, nCmdShow);
    UpdateWindow(theMainWindow);

    // 5. Initialize RenderWare and the engine systems
    if (!InitEngine(theMainWindow, hInstance, width, height, depth, bFullscreen)) {
        DestroyWindow(theMainWindow);
        return -1;
    }

    // 6. Enter the main game loop
    int exitCode = MainLoop();

    // 7. Post-exit check after the loop
    assert(theMainWindow == NULL && "theMainWindow hasn't been destroyed");

    return exitCode;
}

// ----------------------------------------------------------------------------
// WinMain (FUN_00418ca0): Win32 Entry Point
// ----------------------------------------------------------------------------
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    ATOM atom = GlobalAddAtomA(APP_ATOM_NAME);
    int result = AppMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    GlobalDeleteAtom(atom);
    return result;
}
