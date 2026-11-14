#pragma once

#ifndef _WIN_H_
#define _WIN_H_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/**
 * @file win.h
 * @brief Main entry point and window management functions for the Windows platform.
 *
 * For the PC version of CannonCruise it hosts the Win32 window creation, message loop
 * and general application lifecycle functions.
 */

/// Main window handle (Binary DAT_00635acc)
extern HWND theMainWindow;

/**
 * @brief Returns the main window handle.
 * @return HWND — The application's main window.
 */
HWND GetMainWindow();

/**
 * @brief Requests that the application quit (sends WM_QUIT).
 */
void QuitApp();

/**
 * @brief Starts the Windows application (window class registration etc.)
 * @param hInstance Handle of the current application instance.
 * @return BOOL — TRUE on success.
 */
BOOL InitApp(HINSTANCE hInstance);

/**
 * @brief Creates the main render window for the game.
 * @param hInstance    Handle of the current application instance.
 * @param x            Window's X coordinate.
 * @param y            Window's Y coordinate.
 * @param width        Window's width.
 * @param height       Window's height.
 * @param bFullscreen  Will it open in fullscreen mode?
 * @return HWND — Handle of the created window.
 */
HWND CreateMainWindow(HINSTANCE hInstance, int x, int y, int width, int height,
                      BOOL bFullscreen);

/**
 * @brief Runs the main Windows message loop (Message Loop).
 * @return int — Exit code (wParam from the WM_QUIT message).
 */
int MainLoop();

/**
 * @brief Main window procedure (Window Procedure).
 *
 * Captures and handles Windows events (WM_PAINT, WM_DESTROY etc.).
 */
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                             LPARAM lParam);

/**
 * @brief Runs the application's main entry point logic (called by WinMain).
 */
int AppMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
            int nCmdShow);

#endif // _WIN_H_

