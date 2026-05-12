// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    OptionsDlg.cpp
// Purpose: MFC preferences dialog implementation.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial implementation.
// v0.1.1 - Fixed: use InitModalIndirect + in-memory template.
// v0.1.2 - Fixed CreateEx call to use 10 arguments (removed 2 extra nullptrs).
//          Moved DoModal() to public in header.
// =============================================================================

#include "pch.h"
#include "OptionsDlg.h"
#include "MainFrame.h"

#define IDC_OPT_RAD_AMPM       301
#define IDC_OPT_RAD_24HR       302
#define IDC_OPT_RAD_DIASPORA   303
#define IDC_OPT_RAD_ISRAEL     304
#define IDC_OPT_RAD_GRA        305
#define IDC_OPT_RAD_MA72       306
#define IDC_OPT_RAD_MA90       307
#define IDC_OPT_RAD_CIVIL      311
#define IDC_OPT_RAD_HEBREW     312
#define IDC_OPT_TRACKING       313
#define IDC_OPT_PARSHIOS       314
#define IDC_OPT_MOADIM         315
#define IDC_OPT_USER_EVENTS    316
#define IDC_OPT_DAF            317
#define IDC_OPT_YERUSHALMI     318
#define IDC_OPT_HALACHA        319
#define IDC_OPT_MISHNA         320
#define IDC_OPT_TANACH         321
#define IDC_OPT_MIN_TRAY       322
#define IDC_OPT_MIN_STARTUP    323
#define IDC_OPT_START_WINDOWS  324
#define IDC_OPT_DESKTOP        325
#define IDC_OPT_HAFTARAH       327
#define IDC_OPT_FONT_SIZE      328
#define IDC_OPT_LANGUAGE       329
#define IDC_OPT_TRAY_WHEN      330
#define IDC_OPT_CANDLE         331
#define IDC_OPT_TRAY_COLOR     332
#define IDC_OPT_MANAGE_CALS    333
#define IDC_OPT_NOTIFY_PERSONAL 334
#define IDC_OPT_NOTIFY_WEBCAL   335
#define IDC_OPT_PREVIEW_NOTIFY  336
#define IDC_OPT_SHOW_TRAY       337
#define IDC_OPT_ALOT_SHITA      338
#define IDC_OPT_TZEIT_SHITA     339

// =============================================================================
// CWebCalDlg — inline multi-calendar manager
// =============================================================================

class CWebCalDlg : public CDialog
{
public:
    std::vector<WebCalEntry> calendars;

    explicit CWebCalDlg(const std::vector<WebCalEntry>& initial, CWnd* pParent = nullptr)
        : CDialog(), calendars(initial)
    {
        m_pParentWnd = pParent;
    }

    virtual INT_PTR DoModal() override
    {
        struct DlgTmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[32]; } buf = {};
        buf.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER;
        buf.t.cx = 380; buf.t.cy = 260;
        wcscpy_s(buf.title, L"Manage Calendars");
        if (!InitModalIndirect((DLGTEMPLATE*)&buf, m_pParentWnd)) return -1;
        return CDialog::DoModal();
    }

protected:
    BOOL OnInitDialog() override
    {
        CDialog::OnInitDialog();
        SetWindowText(L"Manage Calendars");

        HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CFont* pF = CFont::FromHandle(hF);
        CRect rc; GetClientRect(&rc);
        int W = rc.Width(), H = rc.Height();

        m_listCals.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            CRect(8, 8, W - 8, H - 80), this, 401);
        m_listCals.SetFont(pF);

        m_editUrl.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            CRect(8, H - 68, W - 8, H - 48), this, 402);
        m_editUrl.SetFont(pF);
        ::SendMessage(m_editUrl.GetSafeHwnd(), EM_SETCUEBANNER, TRUE,
            (LPARAM)L"Paste calendar URL here (https://...)  then click Add");

        auto mkBtn = [&](const wchar_t* t, int x, int w, UINT id) {
            CButton* b = new CButton;
            b->Create(t, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                CRect(x, H - 38, x + w, H - 8), this, id);
            b->SetFont(pF);
        };
        mkBtn(L"Add",    8,   54, 411);
        mkBtn(L"Remove", 66,  64, 412);
        mkBtn(L"Toggle", 134, 58, 415);
        mkBtn(L"Up",     196, 40, 413);
        mkBtn(L"Down",   240, 48, 414);
        mkBtn(L"OK",    W - 138, 60, IDOK);
        mkBtn(L"Cancel",W - 74,  66, IDCANCEL);

        RefreshList();
        return TRUE;
    }

    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override
    {
        UINT id = LOWORD(wParam);
        if (id == 411) { // Add
            CString s; m_editUrl.GetWindowText(s); s.Trim();
            if (!s.IsEmpty()) {
                calendars.push_back({ (LPCWSTR)s, true });
                RefreshList();
                m_listCals.SetCurSel((int)calendars.size() - 1);
                m_editUrl.SetWindowText(L"");
            }
            return TRUE;
        }
        if (id == 412) { // Remove
            int sel = m_listCals.GetCurSel();
            if (sel != LB_ERR && sel < (int)calendars.size()) {
                calendars.erase(calendars.begin() + sel);
                RefreshList();
                if (sel < (int)calendars.size()) m_listCals.SetCurSel(sel);
                else if (!calendars.empty()) m_listCals.SetCurSel((int)calendars.size() - 1);
            }
            return TRUE;
        }
        if (id == 413) { // Up
            int sel = m_listCals.GetCurSel();
            if (sel > 0 && sel < (int)calendars.size()) {
                std::swap(calendars[sel], calendars[sel - 1]);
                RefreshList(); m_listCals.SetCurSel(sel - 1);
            }
            return TRUE;
        }
        if (id == 414) { // Down
            int sel = m_listCals.GetCurSel();
            if (sel >= 0 && sel + 1 < (int)calendars.size()) {
                std::swap(calendars[sel], calendars[sel + 1]);
                RefreshList(); m_listCals.SetCurSel(sel + 1);
            }
            return TRUE;
        }
        if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == 401) {
            int sel = m_listCals.GetCurSel();
            if (sel != LB_ERR && sel < (int)calendars.size())
                m_editUrl.SetWindowText(calendars[sel].url.c_str());
            return TRUE;
        }
        if (HIWORD(wParam) == LBN_DBLCLK && LOWORD(wParam) == 401) {
            DoToggle();
            return TRUE;
        }
        if (id == 415) { // Toggle on/off
            DoToggle();
            return TRUE;
        }
        return CDialog::OnCommand(wParam, lParam);
    }

    void OnOK() override { CDialog::OnOK(); }
    DECLARE_MESSAGE_MAP()

private:
    CListBox m_listCals;
    CEdit    m_editUrl;

    void DoToggle()
    {
        int sel = m_listCals.GetCurSel();
        if (sel != LB_ERR && sel < (int)calendars.size()) {
            calendars[sel].enabled = !calendars[sel].enabled;
            RefreshList();
            m_listCals.SetCurSel(sel);
        }
    }

    void RefreshList()
    {
        m_listCals.ResetContent();
        for (const auto& c : calendars)
        {
            std::wstring disp = (c.enabled ? L"[ON]  " : L"[OFF] ") + c.url;
            m_listCals.AddString(disp.c_str());
        }
    }
};

BEGIN_MESSAGE_MAP(CWebCalDlg, CDialog)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
    ON_BN_CLICKED(IDC_OPT_TRAY_COLOR,      &COptionsDlg::OnTrayTextColor)
    ON_BN_CLICKED(IDC_OPT_MANAGE_CALS,     &COptionsDlg::OnManageCals)
    ON_BN_CLICKED(IDC_OPT_PREVIEW_NOTIFY,  &COptionsDlg::OnPreviewNotification)
END_MESSAGE_MAP()

// =============================================================================
// CONSTRUCTION
// =============================================================================

COptionsDlg::COptionsDlg(const AppSettings& current, CWnd* pParent)
    : CDialog(), m_current(current), m_result(current)
{
    m_pParentWnd = pParent;
}

// =============================================================================
// DOMODAL — in-memory dialog template
// =============================================================================

INT_PTR COptionsDlg::DoModal()
{
    struct DlgTmpl {
        DLGTEMPLATE t;
        WORD menu;
        WORD cls;
        wchar_t title[32];
    } buf = {};

    buf.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU |
        DS_MODALFRAME | DS_CENTER;
    buf.t.dwExtendedStyle = 0;
    buf.t.cdit = 0;
    buf.t.x = 0;
    buf.t.y = 0;
    buf.t.cx = 430;
    buf.t.cy = 550;
    wcscpy_s(buf.title, L"Options and Preferences");

    if (!InitModalIndirect((DLGTEMPLATE*)&buf, m_pParentWnd))
        return -1;

    return CDialog::DoModal();
}



// =============================================================================
// ON INIT DIALOG — creates all controls
// =============================================================================

BOOL COptionsDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    SetWindowText(L"Options and Preferences");

    CRect rcClient;
    GetClientRect(&rcClient);
    int W = rcClient.Width();

    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    CFont* pFont = CFont::FromHandle(hFont);

    // Helper: create a plain child control (10-arg CreateEx)
    auto MakeCtrl = [&](const wchar_t* cls, const wchar_t* text,
        DWORD style, int x, int y, int w, int h, UINT id)
        {
            CWnd* p = new CWnd();
            p->CreateEx(0, cls, text, WS_CHILD | WS_VISIBLE | style,
                x, y, w, h, GetSafeHwnd(),
                (HMENU)(UINT_PTR)id);
            p->SetFont(pFont);
        };

    auto InitCombo = [&](CComboBox& combo, int x, int y, int w, UINT id,
        std::initializer_list<const wchar_t*> items, int sel)
        {
            combo.Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                CRect(x, y, x + w, y + 120), this, id);
            combo.SetFont(pFont);
            for (const wchar_t* item : items)
                combo.AddString(item);
            combo.SetCurSel(sel);
        };

    int y = 10;

    // Display preferences
    MakeCtrl(L"BUTTON", L"Display Preferences", BS_GROUPBOX,
        8, y, W - 16, 118, 300);
    y += 18;

    m_radAmPm.Create(L"AM / PM",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        CRect(18, y, 115, y + 20), this, IDC_OPT_RAD_AMPM);
    m_radAmPm.SetFont(pFont);

    m_rad24hr.Create(L"24 Hour",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(120, y, 210, y + 20), this, IDC_OPT_RAD_24HR);
    m_rad24hr.SetFont(pFont);

    m_radCivilMonth.Create(L"Civil month",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        CRect(225, y, 320, y + 20), this, IDC_OPT_RAD_CIVIL);
    m_radCivilMonth.SetFont(pFont);

    m_radHebrewMonth.Create(L"Hebrew month",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(325, y, W - 18, y + 20), this, IDC_OPT_RAD_HEBREW);
    m_radHebrewMonth.SetFont(pFont);
    y += 24;

    m_chkDateTracking.Create(L"Date tracking (on mouse hover)",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(18, y, 240, y + 20), this, IDC_OPT_TRACKING);
    m_chkDateTracking.SetFont(pFont);
    y += 25;

    MakeCtrl(L"STATIC", L"Font size", 0, 18, y + 3, 65, 18, 341);
    InitCombo(m_cmbFontSize, 88, y, 90, IDC_OPT_FONT_SIZE,
        { L"Tiny (13)", L"Normal (14)", L"Medium (15)", L"Large (16)",
          L"Larger (17)", L"Big (18)", L"Biggest (19)" },
        max(0, min(6, m_current.fontSize)));
    y = 136;

    // Month view
    MakeCtrl(L"BUTTON", L"Month View", BS_GROUPBOX,
        8, y, W - 16, 104, 309);
    y += 18;

    m_chkParshios.Create(L"Parshios", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(18, y, 115, y + 20), this, IDC_OPT_PARSHIOS);
    m_chkParshios.SetFont(pFont);
    m_chkDafYomi.Create(L"Daf yomi", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(225, y, 330, y + 20), this, IDC_OPT_DAF);
    m_chkDafYomi.SetFont(pFont);
    y += 22;

    m_chkMoadim.Create(L"Moadim", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(18, y, 115, y + 20), this, IDC_OPT_MOADIM);
    m_chkMoadim.SetFont(pFont);
    m_chkYerushalmi.Create(L"Yerushalmi yomi", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(225, y, 360, y + 20), this, IDC_OPT_YERUSHALMI);
    m_chkYerushalmi.SetFont(pFont);
    y += 22;

    m_chkUserEvents.Create(L"User events", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(18, y, 125, y + 20), this, IDC_OPT_USER_EVENTS);
    m_chkUserEvents.SetFont(pFont);
    m_chkHalacha.Create(L"Halacha yomit", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(225, y, 350, y + 20), this, IDC_OPT_HALACHA);
    m_chkHalacha.SetFont(pFont);
    y += 22;

    m_chkMishna.Create(L"Mishna yomit", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(225, y, 350, y + 20), this, IDC_OPT_MISHNA);
    m_chkMishna.SetFont(pFont);
    y += 22;

    m_chkTanach.Create(L"Tanach yomi", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(225, y, 350, y + 20), this, IDC_OPT_TANACH);
    m_chkTanach.SetFont(pFont);
    y = 248;

    // Holiday schedule / zmanim
    MakeCtrl(L"BUTTON", L"Holiday Schedule and Zmanim", BS_GROUPBOX,
        8, y, W - 16, 130, 350);
    y += 18;

    m_radDiaspora.Create(L"Diaspora (outside Israel)",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        CRect(18, y, 180, y + 20), this, IDC_OPT_RAD_DIASPORA);
    m_radDiaspora.SetFont(pFont);
    m_radIsrael.Create(L"Israel",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(190, y, 260, y + 20), this, IDC_OPT_RAD_ISRAEL);
    m_radIsrael.SetFont(pFont);
    MakeCtrl(L"STATIC", L"Candle lighting", 0, 285, y + 3, 80, 18, 351);
    InitCombo(m_cmbCandleMinutes, 365, y, 46, IDC_OPT_CANDLE,
        { L"15", L"18", L"21", L"22", L"30", L"40" }, 1);
    y += 24;

    m_radGRA.Create(L"GRA / Baal HaTanya",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        CRect(18, y, 150, y + 18), this, IDC_OPT_RAD_GRA);
    m_radGRA.SetFont(pFont);

    m_radMA72.Create(L"Magen Avraham (72 min)",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(160, y, 305, y + 18), this, IDC_OPT_RAD_MA72);
    m_radMA72.SetFont(pFont);

    m_radMA90.Create(L"Magen Avraham (90 min)",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(18, y + 22, 170, y + 40), this, IDC_OPT_RAD_MA90);
    m_radMA90.SetFont(pFont);
    y += 46;

    MakeCtrl(L"STATIC", L"Alot hashachar:", 0, 18, y + 3, 100, 18, 352);
    InitCombo(m_cmbAlotShita, 122, y, 190, IDC_OPT_ALOT_SHITA,
        { L"GRA (16.1°)", L"Magen Avraham (72 min)", L"Magen Avraham (90 min)" },
        max(0, min(2, m_current.alotShita)));
    y += 24;

    MakeCtrl(L"STATIC", L"Tzeis hakochavim:", 0, 18, y + 3, 110, 18, 353);
    InitCombo(m_cmbTzeitShita, 132, y, 240, IDC_OPT_TZEIT_SHITA,
        { L"GRA (8.5°)", L"Magen Avraham (72 min)", L"Magen Avraham (90 min)",
          L"Magen Avraham (72 min prop.)", L"Magen Avraham (90 min prop.)" },
        max(0, min(4, m_current.tzeitShita)));
    y = 386;

    // Interface / tray
    MakeCtrl(L"BUTTON", L"Interface Preferences", BS_GROUPBOX,
        8, y, W - 16, 68, 360);
    y += 18;

    m_chkShowTrayIcon.Create(L"Always show tray icon",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(18, y, 200, y + 20), this, IDC_OPT_SHOW_TRAY);
    m_chkShowTrayIcon.SetFont(pFont);
    y += 22;

    m_chkMinimizeToTray.Create(L"Minimize to system tray",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(18, y, 170, y + 20), this, IDC_OPT_MIN_TRAY);
    m_chkMinimizeToTray.SetFont(pFont);
    InitCombo(m_cmbTrayWhen, 175, y, 130, IDC_OPT_TRAY_WHEN,
        { L"when minimized", L"when closed", L"when minimized or closed" },
        max(0, min(2, m_current.minimizeTrayWhen)));
    m_chkMinimizeOnStartup.Create(L"minimize on startup",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(315, y, W - 18, y + 20), this, IDC_OPT_MIN_STARTUP);
    m_chkMinimizeOnStartup.SetFont(pFont);
    y += 22;

    m_chkStartWithWindows.Create(L"start with Windows",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(18, y, 150, y + 20), this, IDC_OPT_START_WINDOWS);
    m_chkStartWithWindows.SetFont(pFont);
    m_chkDesktopShortcut.Create(L"desktop shortcut",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(175, y, 300, y + 20), this, IDC_OPT_DESKTOP);
    m_chkDesktopShortcut.SetFont(pFont);
    y += 28;

    MakeCtrl(L"STATIC", L"Tray text color", 0, 18, y + 4, 100, 18, 372);
    m_btnTrayTextColor.Create(L"Choose...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(125, y, 205, y + 24), this, IDC_OPT_TRAY_COLOR);
    m_btnTrayTextColor.SetFont(pFont);
    y += 30;

    m_btnManageCals.Create(L"Manage Calendars...",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(18, y, 180, y + 24), this, IDC_OPT_MANAGE_CALS);
    m_btnManageCals.SetFont(pFont);
    y += 32;

    // Notifications
    MakeCtrl(L"BUTTON", L"Notifications", BS_GROUPBOX, 8, y, W - 16, 58, 373);
    y += 18;
    MakeCtrl(L"STATIC", L"Events (birthdays, yahrzeits):", 0, 18, y + 3, 170, 18, 374);
    InitCombo(m_cmbNotifyPersonal, 192, y, 110, IDC_OPT_NOTIFY_PERSONAL,
        { L"Off", L"Toast", L"Popup", L"Toast + Popup" },
        max(0, min(3, m_current.notifyPersonalEvents)));
    y += 24;
    MakeCtrl(L"STATIC", L"Web calendar events:", 0, 18, y + 3, 170, 18, 375);
    InitCombo(m_cmbNotifyWebCal, 192, y, 110, IDC_OPT_NOTIFY_WEBCAL,
        { L"Off", L"Toast", L"Popup", L"Toast + Popup" },
        max(0, min(3, m_current.notifyWebCalEvents)));

    // "Test" button so user can preview what the selected style looks like
    m_btnPreviewNotify.Create(L"Test Notification",
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
        CRect(316, y, 416, y+22), this, IDC_OPT_PREVIEW_NOTIFY);
    m_btnPreviewNotify.SetFont(pFont);
    y += 30;

    // OK / Cancel buttons
    CButton* btnOK = new CButton();
    btnOK->Create(L"OK",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        CRect(W - 148, y, W - 80, y + 26), this, IDOK);
    btnOK->SetFont(pFont);

    CButton* btnCancel = new CButton();
    btnCancel->Create(L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(W - 72, y, W - 8, y + 26), this, IDCANCEL);
    btnCancel->SetFont(pFont);

    // Set initial radio button states from current settings
    m_radAmPm.SetCheck(!m_current.use24Hour ? BST_CHECKED : BST_UNCHECKED);
    m_rad24hr.SetCheck(m_current.use24Hour ? BST_CHECKED : BST_UNCHECKED);
    m_radCivilMonth.SetCheck(!m_current.defaultHebrewMonth ? BST_CHECKED : BST_UNCHECKED);
    m_radHebrewMonth.SetCheck(m_current.defaultHebrewMonth ? BST_CHECKED : BST_UNCHECKED);
    m_chkDateTracking.SetCheck(m_current.dateTracking ? BST_CHECKED : BST_UNCHECKED);
    m_radDiaspora.SetCheck(!m_current.isIsrael ? BST_CHECKED : BST_UNCHECKED);
    m_radIsrael.SetCheck(m_current.isIsrael ? BST_CHECKED : BST_UNCHECKED);
    m_chkParshios.SetCheck(m_current.showParshios ? BST_CHECKED : BST_UNCHECKED);
    m_chkMoadim.SetCheck(m_current.showMoadim ? BST_CHECKED : BST_UNCHECKED);
    m_chkUserEvents.SetCheck(m_current.showUserEvents ? BST_CHECKED : BST_UNCHECKED);
    m_chkDafYomi.SetCheck(m_current.showDafYomi ? BST_CHECKED : BST_UNCHECKED);
    m_chkYerushalmi.SetCheck(m_current.showYerushalmi ? BST_CHECKED : BST_UNCHECKED);
    m_chkHalacha.SetCheck(m_current.showHalachaYomit ? BST_CHECKED : BST_UNCHECKED);
    m_chkMishna.SetCheck(m_current.showMishnaYomit ? BST_CHECKED : BST_UNCHECKED);
    m_chkTanach.SetCheck(m_current.showTanachYomi ? BST_CHECKED : BST_UNCHECKED);
    m_chkShowTrayIcon.SetCheck(m_current.showTrayIcon ? BST_CHECKED : BST_UNCHECKED);
    m_chkMinimizeToTray.SetCheck(m_current.minimizeToTray ? BST_CHECKED : BST_UNCHECKED);
    m_chkMinimizeOnStartup.SetCheck(m_current.minimizeOnStartup ? BST_CHECKED : BST_UNCHECKED);
    m_chkStartWithWindows.SetCheck(m_current.startWithWindows ? BST_CHECKED : BST_UNCHECKED);
    m_chkDesktopShortcut.SetCheck(m_current.desktopShortcut ? BST_CHECKED : BST_UNCHECKED);

    if (m_current.zmanimShita == 1) m_radMA72.SetCheck(BST_CHECKED);
    else if (m_current.zmanimShita == 2) m_radMA90.SetCheck(BST_CHECKED);
    else                                  m_radGRA.SetCheck(BST_CHECKED);

    int candleSel = 1;
    int candleValues[] = { 15, 18, 21, 22, 30, 40 };
    for (int i = 0; i < 6; i++)
        if (m_current.candleLightingMinutes == candleValues[i]) candleSel = i;
    m_cmbCandleMinutes.SetCurSel(candleSel);

    return TRUE;
}

// =============================================================================
// ON OK — reads radio button states into m_result
// =============================================================================

void COptionsDlg::OnOK()
{
    m_result.use24Hour = (m_rad24hr.GetCheck() == BST_CHECKED);
    m_result.defaultHebrewMonth = (m_radHebrewMonth.GetCheck() == BST_CHECKED);
    m_result.dateTracking = (m_chkDateTracking.GetCheck() == BST_CHECKED);
    m_result.isIsrael = (m_radIsrael.GetCheck() == BST_CHECKED);
    m_result.showParshios = (m_chkParshios.GetCheck() == BST_CHECKED);
    m_result.showMoadim = (m_chkMoadim.GetCheck() == BST_CHECKED);
    m_result.showUserEvents = (m_chkUserEvents.GetCheck() == BST_CHECKED);
    m_result.showDafYomi = (m_chkDafYomi.GetCheck() == BST_CHECKED);
    m_result.showYerushalmi = (m_chkYerushalmi.GetCheck() == BST_CHECKED);
    m_result.showHalachaYomit = (m_chkHalacha.GetCheck() == BST_CHECKED);
    m_result.showMishnaYomit = (m_chkMishna.GetCheck() == BST_CHECKED);
    m_result.showTanachYomi = (m_chkTanach.GetCheck() == BST_CHECKED);
    m_result.haftarahShita = m_current.haftarahShita;
    m_result.fontSize = max(0, min(6, m_cmbFontSize.GetCurSel()));
    m_result.language = 0;
    m_result.showTrayIcon   = (m_chkShowTrayIcon.GetCheck()   == BST_CHECKED);
    m_result.minimizeToTray = (m_chkMinimizeToTray.GetCheck() == BST_CHECKED);
    m_result.minimizeTrayWhen = m_cmbTrayWhen.GetCurSel();
    m_result.minimizeOnStartup = (m_chkMinimizeOnStartup.GetCheck() == BST_CHECKED);
    m_result.startWithWindows = (m_chkStartWithWindows.GetCheck() == BST_CHECKED);
    m_result.desktopShortcut = (m_chkDesktopShortcut.GetCheck() == BST_CHECKED);
    if (m_radMA72.GetCheck() == BST_CHECKED) m_result.zmanimShita = 1;
    else if (m_radMA90.GetCheck() == BST_CHECKED) m_result.zmanimShita = 2;
    else                                           m_result.zmanimShita = 0;

    m_result.alotShita  = max(0, min(2, m_cmbAlotShita.GetCurSel()));
    m_result.tzeitShita = max(0, min(4, m_cmbTzeitShita.GetCurSel()));

    CString candle;
    m_cmbCandleMinutes.GetWindowText(candle);
    m_result.candleLightingMinutes = _wtoi(candle);

    m_result.notifyPersonalEvents = max(0, m_cmbNotifyPersonal.GetCurSel());
    m_result.notifyWebCalEvents   = max(0, m_cmbNotifyWebCal.GetCurSel());

    CDialog::OnOK();
}

void COptionsDlg::OnTrayTextColor()
{
    CColorDialog dlg((COLORREF)m_result.trayTextColor, CC_FULLOPEN, this);
    if (dlg.DoModal() == IDOK)
        m_result.trayTextColor = (int)dlg.GetColor();
}

void COptionsDlg::OnManageCals()
{
    CWebCalDlg dlg(m_result.webCalendars, this);
    if (dlg.DoModal() == IDOK)
        m_result.webCalendars = dlg.calendars;
}

void COptionsDlg::OnPreviewNotification()
{
    // Pick the highest-priority style the user has enabled across both combos
    int styleP = max(0, m_cmbNotifyPersonal.GetCurSel());
    int styleW = max(0, m_cmbNotifyWebCal.GetCurSel());
    int style  = max(styleP, styleW);

    if (style == 0) {
        MessageBox(
            L"Both notification settings are set to Off.\n\n"
            L"Change one to Toast, Popup, or Toast + Popup to see a sample.",
            L"WinLuach – Notifications", MB_OK | MB_ICONINFORMATION);
        return;
    }

    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    if (!pFrame) return;

    std::wstring body =
        L"Birthday: Moshe Levy\n"
        L"Yahrzeit: Rivka Cohen (eve)\n"
        L"Anniversary: Yitzchak & Sara";

    pFrame->ShowEventNotification(L"Today's Events  —  Sample", body, style);
}
