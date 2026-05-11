// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    OptionsDialog.h
// Purpose: Declares the Options and Preferences dialog.
//          Allows the user to configure:
//            - Clock format (AM/PM or 24-hour)
//            - Israel / Diaspora schedule
//            - Zmanim shita (GRA, MA 72, MA 90)
//            - Default view (Civil / Hebrew month)
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial file. Clock format, Israel/Diaspora, zmanim shita.
//          Returns updated AppSettings to caller on OK.
// =============================================================================

#pragma once
#include "Settings.h"
#include <windows.h>

// =============================================================================
// DIALOG CONTROL IDs
// =============================================================================

#define IDC_OPT_RADIO_AMPM      301   // AM/PM clock format
#define IDC_OPT_RADIO_24HR      302   // 24-hour clock format
#define IDC_OPT_RADIO_DIASPORA  303   // Diaspora holiday schedule
#define IDC_OPT_RADIO_ISRAEL    304   // Israel holiday schedule
#define IDC_OPT_RADIO_GRA       305   // Zmanim: GRA shita
#define IDC_OPT_RADIO_MA72      306   // Zmanim: MA 72 shita
#define IDC_OPT_RADIO_MA90      307   // Zmanim: MA 90 shita
#define IDC_OPT_GRP_CLOCK       308   // Group box: Clock Format
#define IDC_OPT_GRP_SCHEDULE    309   // Group box: Holiday Schedule
#define IDC_OPT_GRP_ZMANIM      310   // Group box: Zmanim Opinion

// =============================================================================
// OPTIONS DIALOG CLASS
// =============================================================================

class OptionsDialog
{
public:
    // -------------------------------------------------------------------------
    // Shows the options dialog modally.
    // parent:      owner window handle
    // current:     current settings (pre-fills the dialog)
    // updated:     filled with new settings if user clicks OK
    // Returns true if user clicked OK, false if cancelled.
    // -------------------------------------------------------------------------
    static bool Show(HWND              parent,
        const AppSettings& current,
        AppSettings& updated);

private:
    // Dialog procedure (static, registered with Win32)
    static INT_PTR CALLBACK DlgProc(HWND   hwnd,
        UINT   msg,
        WPARAM wParam,
        LPARAM lParam);

    // Called when the dialog is initialised — creates controls and sets state.
    void OnInit(HWND hwnd);

    // Called when OK is clicked — reads control states into m_result.
    bool OnOK(HWND hwnd);

    // Current settings passed in (used to pre-fill controls)
    AppSettings  m_current;

    // Result settings (filled when user clicks OK)
    AppSettings  m_result;
    bool         m_accepted = false;

    // Static instance pointer for DlgProc routing
    static OptionsDialog* s_instance;
};