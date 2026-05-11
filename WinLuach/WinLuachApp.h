// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    WinLuachApp.h
// Purpose: Declares the MFC application class.
//          CWinLuachApp is the top-level application object.
//          It initialises MFC, loads settings, creates the main frame,
//          and runs the message loop.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial MFC application class. Replaces the old Win32 WinMain.
// =============================================================================

#pragma once
#include <afxwin.h>
#include <afxext.h>
#include "Settings.h"

// =============================================================================
// CWinLuachApp
// The one-and-only application object. MFC creates this automatically.
// =============================================================================

class CWinLuachApp : public CWinApp
{
public:
    CWinLuachApp();

    // Called by MFC to initialise the application and create the main window.
    virtual BOOL InitInstance() override;

    // Called by MFC when the application is shutting down.
    virtual int  ExitInstance() override;

    // Application-wide settings (accessible from any window via theApp)
    AppSettings m_settings;

    DECLARE_MESSAGE_MAP()
};

// Global application object — accessible anywhere via theApp
extern CWinLuachApp theApp;