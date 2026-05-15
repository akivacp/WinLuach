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
#include <shobjidl.h>
#include <propkey.h>
#include <propvarutil.h>
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Propsys.lib")

// The one global application object
CWinLuachApp theApp;

static void RegisterJumpList()
{
    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    bool shouldUninit = SUCCEEDED(hrInit);

    ICustomDestinationList* pDL = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER,
        IID_ICustomDestinationList, (void**)&pDL);
    if (FAILED(hr) || !pDL)
    {
        if (shouldUninit) CoUninitialize();
        return;
    }

    pDL->SetAppID(L"WinLuach.WinLuach");

    UINT minSlots = 0;
    IObjectArray* pRemoved = nullptr;
    hr = pDL->BeginList(&minSlots, IID_IObjectArray, (void**)&pRemoved);
    if (pRemoved) pRemoved->Release();

    if (SUCCEEDED(hr))
    {
        IObjectCollection* pColl = nullptr;
        hr = CoCreateInstance(CLSID_EnumerableObjectCollection, nullptr, CLSCTX_INPROC_SERVER,
            IID_IObjectCollection, (void**)&pColl);
        if (SUCCEEDED(hr) && pColl)
        {
            wchar_t exePath[MAX_PATH] = {};
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);

            struct { const wchar_t* title; const wchar_t* args; const wchar_t* desc; } tasks[] =
            {
                { L"Show WinLuach",   L"/show",      L"Open the WinLuach calendar"   },
                { L"Countdown Clock", L"/countdown", L"Open the countdown clock"      },
                { L"Options",         L"/options",   L"Open WinLuach options"         },
            };

            for (auto& t : tasks)
            {
                IShellLinkW* pLink = nullptr;
                if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                    IID_IShellLinkW, (void**)&pLink)) || !pLink) continue;

                pLink->SetPath(exePath);
                pLink->SetArguments(t.args);
                pLink->SetDescription(t.desc);
                pLink->SetIconLocation(exePath, 0);

                IPropertyStore* pPS = nullptr;
                if (SUCCEEDED(pLink->QueryInterface(IID_IPropertyStore, (void**)&pPS)) && pPS)
                {
                    PROPVARIANT pv;
                    InitPropVariantFromString(L"WinLuach.WinLuach", &pv);
                    pPS->SetValue(PKEY_AppUserModel_ID, pv);
                    PropVariantClear(&pv);

                    InitPropVariantFromString(t.title, &pv);
                    pPS->SetValue(PKEY_Title, pv);
                    PropVariantClear(&pv);

                    pPS->Commit();
                    pPS->Release();
                }

                pColl->AddObject(pLink);
                pLink->Release();
            }

            IObjectArray* pArray = nullptr;
            if (SUCCEEDED(pColl->QueryInterface(IID_IObjectArray, (void**)&pArray)) && pArray)
            {
                pDL->AddUserTasks(pArray);
                pArray->Release();
            }
            pColl->Release();
        }
        pDL->CommitList();
    }

    pDL->Release();
    if (shouldUninit) CoUninitialize();
}

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

    // Must be called before any window or Jump List registration.
    SetCurrentProcessExplicitAppUserModelID(L"WinLuach.WinLuach");

    m_singleInstanceMutex = CreateMutexW(nullptr, TRUE, L"Local\\WinLuachSingleInstance");
    if (m_singleInstanceMutex && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        if (HWND hwnd = FindWindowW(nullptr, L"WinLuach - Hebrew Calendar"))
        {
            // Decode the Jump List argument and forward it to the running instance.
            int action = 0;  // 0=show, 1=countdown, 2=options
            LPCWSTR cmd = GetCommandLineW();
            if (cmd)
            {
                if (wcsstr(cmd, L"/countdown")) action = 1;
                else if (wcsstr(cmd, L"/options")) action = 2;
            }
            PostMessageW(hwnd, WM_WINLUACH_COMMAND, (WPARAM)action, 0);
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

    // Register Jump List tasks (taskbar right-click menu).
    RegisterJumpList();

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
