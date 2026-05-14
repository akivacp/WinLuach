// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    WinLuachApp.cpp
// Purpose: MFC application class implementation.
//          Initialises the app, loads settings, creates the main frame.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial implementation. MFC app init, settings load,
//          main frame creation.
// =============================================================================

#include "pch.h"
#include "WinLuachApp.h"
#include "MainFrame.h"

// The one global application object
CWinLuachApp theApp;

BEGIN_MESSAGE_MAP(CWinLuachApp, CWinApp)
END_MESSAGE_MAP()

// Constructor — nothing to do, MFC handles initialisation
CWinLuachApp::CWinLuachApp()
{
}

// InitInstance — called once at startup.
// Loads settings, creates the main window, and shows it.
BOOL CWinLuachApp::InitInstance()
{
    // Standard MFC initialisation
    CWinApp::InitInstance();

    m_singleInstanceMutex = CreateMutexW(nullptr, TRUE, L"Local\\WinLuachSingleInstance");
    if (m_singleInstanceMutex && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        if (HWND hwnd = FindWindowW(nullptr, L"WinLuach - Hebrew Calendar"))
        {
            ShowWindow(hwnd, SW_SHOWNORMAL);
            SetForegroundWindow(hwnd);
        }
        CloseHandle(m_singleInstanceMutex);
        m_singleInstanceMutex = nullptr;
        return FALSE;
    }

    // Enable visual styles (Windows XP+ themed controls)
    InitCommonControls();

    // Load saved settings (fills with defaults on first run)
    LoadSettings(m_settings);

    // Create and show the main frame
    CMainFrame* pFrame = new CMainFrame();
    if (!pFrame->Create())
        return FALSE;

    m_pMainWnd = pFrame;
    if (m_settings.minimizeToTray && m_settings.minimizeOnStartup)
    {
        pFrame->ShowWindow(SW_HIDE);
    }
    else
    {
        pFrame->ShowWindow(SW_SHOW);
        pFrame->UpdateWindow();
    }

    return TRUE;
}

// ExitInstance — called once at shutdown.
// Saves settings before the app closes.
int CWinLuachApp::ExitInstance()
{
    SaveSettings(m_settings);
    if (m_singleInstanceMutex)
    {
        ReleaseMutex(m_singleInstanceMutex);
        CloseHandle(m_singleInstanceMutex);
        m_singleInstanceMutex = nullptr;
    }
    return CWinApp::ExitInstance();
}
