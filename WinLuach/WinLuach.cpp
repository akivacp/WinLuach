// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    WinLuach.cpp
// Purpose: Application entry point. Creates and runs the main window.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial Win32 scaffold.
// v0.1.1 - Added Hebrew date engine test.
// v0.1.2 - Added Zmanim engine test.
// v0.2.0 - All engines verified. Switched to full UI launch.
// v0.3.0 - MainWindow created. Real calendar UI running.
// =============================================================================

#include "framework.h"
#include "WinLuach.h"
#include "MainWindow.h"

// ---------------------------------------------------------------------------
// wWinMain - application entry point.
// Creates the main window and runs the message loop.
// ---------------------------------------------------------------------------
int APIENTRY wWinMain(_In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Create the main window
    if (!MainWindow::Create(hInstance, nCmdShow))
        return 1;

    // Standard Win32 message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}