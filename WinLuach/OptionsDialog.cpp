// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    OptionsDialog.cpp
// Purpose: Full implementation of the Options and Preferences dialog.
//          Built entirely in code — no .rc resource file needed.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial implementation. Clock format radio buttons,
//          Israel/Diaspora radio buttons, Zmanim shita radio buttons.
//          All settings saved immediately on OK.
// =============================================================================

#include "OptionsDialog.h"
#include <windowsx.h>

// Static instance for DlgProc routing
OptionsDialog* OptionsDialog::s_instance = nullptr;

// =============================================================================
// SHOW (public entry point)
// =============================================================================

// Shows the options dialog modally.
// Returns true if user clicked OK.
bool OptionsDialog::Show(HWND              parent,
    const AppSettings& current,
    AppSettings& updated)
{
    OptionsDialog dlg;
    dlg.m_current = current;
    dlg.m_result = current; // start result as copy of current
    s_instance = &dlg;

    // Build a minimal DLGTEMPLATE in memory (no .rc file needed)
    struct
    {
        DLGTEMPLATE tmpl;
        WORD        menu;
        WORD        wndClass;
        wchar_t     title[24];
    } dlgData = {};

    dlgData.tmpl.style = WS_POPUP | WS_CAPTION | WS_SYSMENU
        | WS_THICKFRAME | DS_CENTER;
    dlgData.tmpl.dwExtendedStyle = 0;
    dlgData.tmpl.cdit = 0;
    dlgData.tmpl.x = 0;
    dlgData.tmpl.y = 0;
    dlgData.tmpl.cx = 260;
    dlgData.tmpl.cy = 200;
    dlgData.menu = 0;
    dlgData.wndClass = 0;
    wcscpy_s(dlgData.title, L"Options and Preferences");

    DialogBoxIndirectParamW(
        GetModuleHandleW(nullptr),
        &dlgData.tmpl,
        parent,
        DlgProc,
        (LPARAM)&dlg);

    s_instance = nullptr;

    if (dlg.m_accepted)
    {
        updated = dlg.m_result;
        return true;
    }
    return false;
}

// =============================================================================
// DLGPROC
// =============================================================================

// Routes dialog messages to the OptionsDialog instance.
INT_PTR CALLBACK OptionsDialog::DlgProc(HWND   hwnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam)
{
    OptionsDialog* self = s_instance;
    if (!self) return FALSE;

    switch (msg)
    {
    case WM_INITDIALOG:
        self->OnInit(hwnd);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if (self->OnOK(hwnd))
                EndDialog(hwnd, IDOK);
            return TRUE;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        return TRUE;
    }

    return FALSE;
}

// =============================================================================
// ON INIT
// =============================================================================

// Creates all dialog controls programmatically and sets initial state.
void OptionsDialog::OnInit(HWND hwnd)
{
    SetWindowTextW(hwnd, L"Options and Preferences");

    RECT rc;
    GetClientRect(hwnd, &rc);
    int W = rc.right;

    // Font for all controls
    HFONT hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    HFONT hFontBold = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    // Helper to create a control and set its font
    auto Make = [&](const wchar_t* cls, const wchar_t* text,
        DWORD style, int x, int y, int w, int h,
        UINT id, bool bold = false) -> HWND
        {
            HWND ctrl = CreateWindowExW(0, cls, text,
                WS_CHILD | WS_VISIBLE | style,
                x, y, w, h, hwnd,
                (HMENU)(UINT_PTR)id,
                GetModuleHandleW(nullptr), nullptr);
            SendMessage(ctrl, WM_SETFONT,
                (WPARAM)(bold ? hFontBold : hFont), TRUE);
            return ctrl;
        };

    int y = 10;

    // -----------------------------------------------------------------------
    // GROUP 1: Clock Format
    // -----------------------------------------------------------------------
    Make(L"BUTTON", L"Clock Format",
        BS_GROUPBOX, 8, y, W - 16, 54, IDC_OPT_GRP_CLOCK);
    y += 18;

    HWND hAmPm = Make(L"BUTTON", L"AM / PM",
        BS_AUTORADIOBUTTON | WS_GROUP, 18, y, 100, 20, IDC_OPT_RADIO_AMPM);
    HWND h24Hr = Make(L"BUTTON", L"24 Hour",
        BS_AUTORADIOBUTTON, 130, y, 100, 20, IDC_OPT_RADIO_24HR);
    y += 30;

    // Set clock format radio
    SendMessage(m_current.use24Hour ? h24Hr : hAmPm, BM_SETCHECK, BST_CHECKED, 0);

    y += 6;

    // -----------------------------------------------------------------------
    // GROUP 2: Holiday Schedule
    // -----------------------------------------------------------------------
    Make(L"BUTTON", L"Holiday Schedule",
        BS_GROUPBOX, 8, y, W - 16, 54, IDC_OPT_GRP_SCHEDULE);
    y += 18;

    HWND hDiaspora = Make(L"BUTTON", L"Diaspora (outside Israel)",
        BS_AUTORADIOBUTTON | WS_GROUP, 18, y, 180, 20, IDC_OPT_RADIO_DIASPORA);
    HWND hIsrael = Make(L"BUTTON", L"Israel",
        BS_AUTORADIOBUTTON, 18, y + 22, 180, 20, IDC_OPT_RADIO_ISRAEL);
    y += 50;

    // Set schedule radio
    SendMessage(m_current.isIsrael ? hIsrael : hDiaspora, BM_SETCHECK, BST_CHECKED, 0);

    y += 8;

    // -----------------------------------------------------------------------
    // GROUP 3: Zmanim Opinion (Shita)
    // -----------------------------------------------------------------------
    Make(L"BUTTON", L"Zmanim Opinion",
        BS_GROUPBOX, 8, y, W - 16, 60, IDC_OPT_GRP_ZMANIM);
    y += 18;

    HWND hGRA = Make(L"BUTTON", L"GRA / Baal HaTanya",
        BS_AUTORADIOBUTTON | WS_GROUP, 18, y, 160, 18, IDC_OPT_RADIO_GRA);
    HWND hMA72 = Make(L"BUTTON", L"Magen Avraham (72 min)",
        BS_AUTORADIOBUTTON, 18, y + 20, 160, 18, IDC_OPT_RADIO_MA72);
    HWND hMA90 = Make(L"BUTTON", L"Magen Avraham (90 min)",
        BS_AUTORADIOBUTTON, 18, y + 40, 160, 18, IDC_OPT_RADIO_MA90);
    y += 66;

    // Set shita radio
    if (m_current.zmanimShita == 1) SendMessage(hMA72, BM_SETCHECK, BST_CHECKED, 0);
    else if (m_current.zmanimShita == 2) SendMessage(hMA90, BM_SETCHECK, BST_CHECKED, 0);
    else                                 SendMessage(hGRA, BM_SETCHECK, BST_CHECKED, 0);

    y += 10;

    // -----------------------------------------------------------------------
    // OK / Cancel buttons
    // -----------------------------------------------------------------------
    Make(L"BUTTON", L"OK", BS_DEFPUSHBUTTON, W - 136, y, 60, 26, IDOK);
    Make(L"BUTTON", L"Cancel", BS_PUSHBUTTON, W - 70, y, 60, 26, IDCANCEL);
}

// =============================================================================
// ON OK
// =============================================================================

// Reads all control states into m_result.
bool OptionsDialog::OnOK(HWND hwnd)
{
    // Clock format
    m_result.use24Hour =
        (SendDlgItemMessage(hwnd, IDC_OPT_RADIO_24HR,
            BM_GETCHECK, 0, 0) == BST_CHECKED);

    // Israel / Diaspora
    m_result.isIsrael =
        (SendDlgItemMessage(hwnd, IDC_OPT_RADIO_ISRAEL,
            BM_GETCHECK, 0, 0) == BST_CHECKED);

    // Zmanim shita
    if (SendDlgItemMessage(hwnd, IDC_OPT_RADIO_MA72,
        BM_GETCHECK, 0, 0) == BST_CHECKED)
        m_result.zmanimShita = 1;
    else if (SendDlgItemMessage(hwnd, IDC_OPT_RADIO_MA90,
        BM_GETCHECK, 0, 0) == BST_CHECKED)
        m_result.zmanimShita = 2;
    else
        m_result.zmanimShita = 0;

    m_accepted = true;
    return true;
}
