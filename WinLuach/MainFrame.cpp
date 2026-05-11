// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    MainFrame.cpp
// Purpose: MFC main frame window implementation.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial MFC main frame.
// v0.1.1 - Fixed flashing on cell click: only child panels invalidated,
//          not the full frame. Added Invalidate(FALSE) throughout.
// =============================================================================

#include "pch.h"
#include "MainFrame.h"
#include "CalendarView.h"
#include "SidebarPanel.h"
#include "ZmanimPanel.h"
#include "LocationDlg.h"
#include "OptionsDlg.h"
#include "CalPrintDlg.h"
#include "WinLuachApp.h"
#include "Resource.h"
#include <urlmon.h>
#include <fstream>
#include <thread>
#pragma comment(lib, "Urlmon.lib")

static std::wstring WebCalTrim(const std::wstring& s)
{
    size_t first = s.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return L"";
    size_t last = s.find_last_not_of(L" \t\r\n");
    return s.substr(first, last - first + 1);
}

static std::wstring DecodeUtf8(const std::string& bytes)
{
    if (bytes.empty()) return L"";
    int needed = MultiByteToWideChar(CP_UTF8, 0,
        bytes.data(), (int)bytes.size(), nullptr, 0);
    if (needed <= 0)
        return L"";

    std::wstring out(needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0,
        bytes.data(), (int)bytes.size(), out.data(), needed);
    return out;
}

static std::wstring UnescapeIcsText(const std::wstring& s)
{
    std::wstring out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == L'\\' && i + 1 < s.size())
        {
            wchar_t c = s[++i];
            if (c == L'n' || c == L'N') out += L' ';
            else if (c == L',' || c == L';' || c == L'\\') out += c;
            else { out += L'\\'; out += c; }
        }
        else
        {
            out += s[i];
        }
    }
    return out;
}

static bool ParseIcsDate(const std::wstring& line, GregorianDate& g)
{
    size_t colon = line.find(L':');
    if (colon == std::wstring::npos) return false;
    std::wstring val = line.substr(colon + 1);
    if (val.size() < 8) return false;
    try
    {
        g.year = std::stoi(val.substr(0, 4));
        g.month = std::stoi(val.substr(4, 2));
        g.day = std::stoi(val.substr(6, 2));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// =============================================================================
// CGotoDateDlg — compact mini-calendar date picker
// =============================================================================

class CGotoDateDlg : public CDialog
{
public:
    GregorianDate selectedDate;

    CGotoDateDlg(const GregorianDate& initial, CWnd* pParent = nullptr)
        : CDialog(), m_year(initial.year), m_month(initial.month), m_day(initial.day)
    {
        m_pParentWnd = pParent;
        selectedDate = initial;
    }

    INT_PTR DoModal() override
    {
        struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[20]; } b = {};
        b.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER;
        b.t.cx = 230; b.t.cy = 240;
        wcscpy_s(b.title, L"Go to Date");
        if (!InitModalIndirect((DLGTEMPLATE*)&b, m_pParentWnd)) return -1;
        return CDialog::DoModal();
    }

protected:
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override
    {
        CDialog::OnInitDialog();
        SetWindowText(L"Go to Date");
        HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CFont* pF = CFont::FromHandle(hF);
        CRect rc; GetClientRect(&rc);
        int W = rc.Width(), H = rc.Height();

        auto mkBtn = [&](const wchar_t* t, int x, int y, int w, int h, UINT id) {
            CButton* b = new CButton;
            b->Create(t, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                CRect(x, y, x+w, y+h), this, id);
            b->SetFont(pF);
        };
        mkBtn(L"<<", 4,    4, 28, 24, 601);
        mkBtn(L"<",  36,   4, 24, 24, 602);
        mkBtn(L">",  W-60, 4, 24, 24, 603);
        mkBtn(L">>", W-32, 4, 28, 24, 604);

        m_lblMonth.Create(L"", WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
            CRect(64, 4, W-64, 28), this, 605);
        m_lblMonth.SetFont(pF);

        mkBtn(L"Go",     W-128, H-32, 52, 26, IDOK);
        mkBtn(L"Cancel", W-70,  H-32, 64, 26, IDCANCEL);

        UpdateLabel();
        return TRUE;
    }

    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override
    {
        switch (LOWORD(wParam)) {
        case 601: NavYear(-1);  return TRUE;
        case 602: NavMonth(-1); return TRUE;
        case 603: NavMonth(1);  return TRUE;
        case 604: NavYear(1);   return TRUE;
        }
        return CDialog::OnCommand(wParam, lParam);
    }

    void OnOK() override
    {
        if (m_day < 1) {
            MessageBox(L"Click a day to select it.", L"WinLuach", MB_OK | MB_ICONINFORMATION);
            return;
        }
        selectedDate = GregorianDate(m_year, m_month, m_day);
        CDialog::OnOK();
    }

private:
    int     m_year, m_month, m_day;
    CStatic m_lblMonth;

    void NavMonth(int delta) {
        m_month += delta;
        while (m_month < 1)  { m_month += 12; m_year--; }
        while (m_month > 12) { m_month -= 12; m_year++; }
        m_day = 0; UpdateLabel(); Invalidate(FALSE);
    }
    void NavYear(int delta) { m_year += delta; m_day = 0; UpdateLabel(); Invalidate(FALSE); }
    void UpdateLabel() {
        if (!m_lblMonth.GetSafeHwnd()) return;
        CString s; s.Format(L"%s %d", GregorianMonthName(m_month).c_str(), m_year);
        m_lblMonth.SetWindowText(s);
    }
};

void CGotoDateDlg::OnPaint()
{
    CPaintDC dc(this);
    CRect rc; GetClientRect(&rc);
    int W = rc.Width(), H = rc.Height();

    int hdrY  = 32;
    int cellH = 26;
    int cellW = (W - 8) / 7;

    // Day header row
    dc.FillSolidRect(CRect(4, hdrY, W-4, hdrY+20), RGB(70, 100, 160));
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(RGB(255, 255, 255));
    dc.SelectObject(CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT)));
    static const wchar_t* hdrs[] = { L"Su",L"Mo",L"Tu",L"We",L"Th",L"Fr",L"Sa" };
    for (int i = 0; i < 7; i++) {
        CRect c(4 + i*cellW, hdrY, 4 + (i+1)*cellW, hdrY+20);
        dc.DrawText(hdrs[i], -1, &c, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    }

    // Calendar cells
    int gridY = hdrY + 20;
    GregorianDate first(m_year, m_month, 1);
    int startCol     = (int)GetDayOfWeek(first);
    int daysInMonth  = DaysInGregorianMonth(m_month, m_year);
    GregorianDate today = GetTodayGregorian();

    for (int day = 1; day <= daysInMonth; day++) {
        int idx = startCol + day - 1;
        int col = idx % 7, row = idx / 7;
        CRect cell(4 + col*cellW, gridY + row*cellH,
                   4 + (col+1)*cellW, gridY + (row+1)*cellH);

        COLORREF bg = RGB(255,255,255);
        if (day == m_day)
            bg = RGB(55, 90, 200);
        else if (m_year == today.year && m_month == today.month && day == today.day)
            bg = RGB(255, 255, 140);
        else if (col == 6)
            bg = RGB(228, 244, 228);

        dc.FillSolidRect(&cell, bg);
        dc.DrawEdge(&cell, EDGE_SUNKEN, BF_RECT);
        dc.SetTextColor(day == m_day ? RGB(255,255,255) : RGB(0,0,0));
        wchar_t s[4]; swprintf_s(s, L"%d", day);
        dc.DrawText(s, -1, &cell, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    }
}

void CGotoDateDlg::OnLButtonDown(UINT, CPoint pt)
{
    CRect rc; GetClientRect(&rc);
    int W = rc.Width();
    int hdrY = 32, gridY = hdrY + 20;
    int cellH = 26, cellW = (W - 8) / 7;
    if (pt.y < gridY || pt.x < 4 || pt.x > W-4) return;
    int col = (pt.x - 4) / cellW;
    int row = (pt.y - gridY) / cellH;
    if (col < 0 || col > 6) return;
    int startCol = (int)GetDayOfWeek(GregorianDate(m_year, m_month, 1));
    int day = row * 7 + col - startCol + 1;
    if (day >= 1 && day <= DaysInGregorianMonth(m_month, m_year)) {
        m_day = day; Invalidate(FALSE);
    }
}

BEGIN_MESSAGE_MAP(CGotoDateDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_PAINT()
    ON_WM_DESTROY()
    ON_WM_CLOSE()
    ON_WM_SYSCOMMAND()
    ON_WM_TIMER()
    ON_MESSAGE(WM_WINLUACH_TRAY, &CMainFrame::OnTrayNotify)
    ON_COMMAND(ID_VIEW_PREVDAY, &CMainFrame::OnViewPrevDay)
    ON_COMMAND(ID_VIEW_NEXTDAY, &CMainFrame::OnViewNextDay)
    ON_COMMAND(ID_VIEW_TODAY, &CMainFrame::OnViewToday)
    ON_COMMAND(ID_VIEW_PREVMONTH, &CMainFrame::OnViewPrevMonth)
    ON_COMMAND(ID_VIEW_NEXTMONTH, &CMainFrame::OnViewNextMonth)
    ON_COMMAND(ID_VIEW_PREVYEAR, &CMainFrame::OnViewPrevYear)
    ON_COMMAND(ID_VIEW_NEXTYEAR, &CMainFrame::OnViewNextYear)
    ON_COMMAND(ID_VIEW_PREVDECADE, &CMainFrame::OnViewPrevDecade)
    ON_COMMAND(ID_VIEW_NEXTDECADE, &CMainFrame::OnViewNextDecade)
    ON_COMMAND(ID_OPTIONS_LOCATION, &CMainFrame::OnOptionsLocation)
    ON_COMMAND(ID_OPTIONS_PREFS, &CMainFrame::OnOptionsPrefs)
    ON_COMMAND(ID_HELP_ABOUT, &CMainFrame::OnHelpAbout)
    ON_COMMAND(ID_TRAY_OPEN, &CMainFrame::RestoreFromTray)
    ON_COMMAND(ID_TRAY_ABOUT, &CMainFrame::OnHelpAbout)
    ON_COMMAND(ID_TRAY_EXIT, &CMainFrame::OnTrayExit)
    ON_CBN_SELCHANGE(IDC_MONTH_COMBO, &CMainFrame::OnMonthComboChange)
    ON_NOTIFY(UDN_DELTAPOS, IDC_YEAR_SPIN, &CMainFrame::OnYearSpinDelta)
    ON_COMMAND(ID_CAL_PRINT,    &CMainFrame::OnCalPrint)
    ON_COMMAND(ID_CAL_PREVIEW,  &CMainFrame::OnCalPreview)
    ON_MESSAGE(WM_WEBCAL_DONE,  &CMainFrame::OnWebCalDone)
    ON_WM_MOUSEWHEEL()
    ON_COMMAND(ID_FILE_BACKUP,      &CMainFrame::OnFileBackup)
    ON_COMMAND(ID_FILE_RESTORE,     &CMainFrame::OnFileRestore)
    ON_COMMAND(ID_VIEW_ZOOM_IN,     &CMainFrame::OnViewZoomIn)
    ON_COMMAND(ID_VIEW_ZOOM_OUT,    &CMainFrame::OnViewZoomOut)
    ON_COMMAND(ID_VIEW_ZOOM_RESET,  &CMainFrame::OnViewZoomReset)
    ON_COMMAND(ID_CAL_GOTO,         &CMainFrame::OnCalGoTo)
END_MESSAGE_MAP()

CMainFrame::CMainFrame() {}
CMainFrame::~CMainFrame() {}

// Creates the main frame window.
BOOL CMainFrame::Create()
{
    LPCTSTR cls = AfxRegisterWndClass(
        CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
        ::LoadCursor(nullptr, IDC_ARROW),
        (HBRUSH)(COLOR_WINDOW + 1),
        ::LoadIcon(nullptr, IDI_APPLICATION));

    if (!CFrameWnd::Create(cls,
        L"WinLuach - Hebrew Calendar",
        WS_OVERLAPPEDWINDOW,
        CRect(100, 100, 1200, 800)))
        return FALSE;

    return TRUE;
}

// =============================================================================
// ON CREATE
// =============================================================================

int CMainFrame::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CFrameWnd::OnCreate(lpcs) == -1)
        return -1;

    // Build menu
    CMenu menu;
    menu.CreateMenu();

    CMenu fileMenu;
    fileMenu.CreatePopupMenu();
    fileMenu.AppendMenu(MF_STRING, ID_CAL_PRINT,    L"&Print...\tCtrl+P");
    fileMenu.AppendMenu(MF_STRING, ID_CAL_PREVIEW,  L"Print Pre&view...");
    fileMenu.AppendMenu(MF_SEPARATOR);
    fileMenu.AppendMenu(MF_STRING, ID_FILE_BACKUP,  L"&Backup Settings...");
    fileMenu.AppendMenu(MF_STRING, ID_FILE_RESTORE, L"&Restore Settings...");
    fileMenu.AppendMenu(MF_SEPARATOR);
    fileMenu.AppendMenu(MF_STRING, ID_APP_EXIT,     L"E&xit");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)fileMenu.Detach(), L"&File");

    CMenu calMenu;
    calMenu.CreatePopupMenu();
    calMenu.AppendMenu(MF_STRING, ID_VIEW_PREVDAY,    L"Previous &Day\tLeft");
    calMenu.AppendMenu(MF_STRING, ID_VIEW_NEXTDAY,    L"Next D&ay\tRight");
    calMenu.AppendMenu(MF_SEPARATOR);
    calMenu.AppendMenu(MF_STRING, ID_VIEW_TODAY,      L"&Today\tHome");
    calMenu.AppendMenu(MF_STRING, ID_CAL_GOTO,        L"&Go to Date...\tCtrl+G");
    calMenu.AppendMenu(MF_SEPARATOR);
    calMenu.AppendMenu(MF_STRING, ID_VIEW_PREVMONTH,  L"&Previous Month\tPage Up");
    calMenu.AppendMenu(MF_STRING, ID_VIEW_NEXTMONTH,  L"&Next Month\tPage Down");
    calMenu.AppendMenu(MF_STRING, ID_VIEW_PREVYEAR,   L"Previous &Year\tCtrl+Left");
    calMenu.AppendMenu(MF_STRING, ID_VIEW_NEXTYEAR,   L"Next Y&ear\tCtrl+Right");
    calMenu.AppendMenu(MF_STRING, ID_VIEW_PREVDECADE, L"Previous De&cade\tCtrl+Alt+Left");
    calMenu.AppendMenu(MF_STRING, ID_VIEW_NEXTDECADE, L"Next Deca&de\tCtrl+Alt+Right");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)calMenu.Detach(), L"&Calendar");

    CMenu viewMenu;
    viewMenu.CreatePopupMenu();
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_ZOOM_IN,    L"Zoom &In\tCtrl++");
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_ZOOM_OUT,   L"Zoom &Out\tCtrl+-");
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_ZOOM_RESET, L"Reset Text &Size\tCtrl+0");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)viewMenu.Detach(), L"&View");

    CMenu optMenu;
    optMenu.CreatePopupMenu();
    optMenu.AppendMenu(MF_STRING, ID_OPTIONS_LOCATION, L"&Location...");
    optMenu.AppendMenu(MF_STRING, ID_OPTIONS_PREFS,    L"&Preferences...\tCtrl+,");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)optMenu.Detach(), L"&Options");

    CMenu helpMenu;
    helpMenu.CreatePopupMenu();
    helpMenu.AppendMenu(MF_STRING, ID_HELP_ABOUT, L"&About WinLuach...");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)helpMenu.Detach(), L"&Help");

    SetMenu(&menu);
    menu.Detach();

    // Load settings (calls RecreateFonts internally)
    AppSettings& s = theApp.m_settings;
    ApplySettings(s);

    // Today
    m_today = GetTodayGregorian();
    m_todayHebrew = GetTodayHebrew();
    m_viewYear = m_today.year;
    m_viewMonth = m_today.month;
    m_selectedDate = m_today;
    m_selectedHebrew = m_todayHebrew;
    SyncViewToSelected();
    m_isDST = IsDST(m_today, m_location);

    // Child panels
    m_pCalView = new CCalendarView(this);
    m_pCalView->Create(nullptr, nullptr,
        WS_CHILD | WS_VISIBLE,
        CRect(0, 0, 100, 100), this, 1001);

    m_pSidebar = new CSidebarPanel(this);
    m_pSidebar->Create(nullptr, nullptr,
        WS_CHILD | WS_VISIBLE,
        CRect(0, 0, 100, 100), this, 1002);

    m_pZmanim = new CZmanimPanel(this);
    m_pZmanim->Create(nullptr, nullptr,
        WS_CHILD | WS_VISIBLE,
        CRect(0, 0, 100, 100), this, 1003);

    // Navigation buttons in the header bar (ASCII-safe labels)
    const DWORD btnStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
    CRect r(0, 0, 64, 28);
    m_btnPrevDecade.Create(L"<< Dec",  btnStyle, r, this, ID_VIEW_PREVDECADE);
    m_btnPrevYear  .Create(L"< Year",  btnStyle, r, this, ID_VIEW_PREVYEAR);
    m_btnPrevMonth .Create(L"< Month", btnStyle, r, this, ID_VIEW_PREVMONTH);
    m_btnPrevDay   .Create(L"< Day",   btnStyle, r, this, ID_VIEW_PREVDAY);
    m_btnToday     .Create(L"Today",   btnStyle, r, this, ID_VIEW_TODAY);
    m_btnNextDay   .Create(L"Day >",   btnStyle, r, this, ID_VIEW_NEXTDAY);
    m_btnNextMonth .Create(L"Month >", btnStyle, r, this, ID_VIEW_NEXTMONTH);
    m_btnNextYear  .Create(L"Year >",  btnStyle, r, this, ID_VIEW_NEXTYEAR);
    m_btnNextDecade.Create(L"Dec >>",  btnStyle, r, this, ID_VIEW_NEXTDECADE);

    m_btnPrint.Create(L"Print", btnStyle, r, this, ID_CAL_PRINT);

    CFont* pNF = &m_fontNormal;
    m_btnPrevDecade.SetFont(pNF);  m_btnPrevYear.SetFont(pNF);
    m_btnPrevMonth .SetFont(pNF);  m_btnPrevDay .SetFont(pNF);
    m_btnToday     .SetFont(pNF);
    m_btnNextDay   .SetFont(pNF);  m_btnNextMonth.SetFont(pNF);
    m_btnNextYear  .SetFont(pNF);  m_btnNextDecade.SetFont(pNF);
    m_btnPrint     .SetFont(pNF);

    // Month combo + year edit + year spin
    m_comboMonth.Create(WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL,
        CRect(0, 0, 130, 300), this, IDC_MONTH_COMBO);
    m_comboMonth.SetFont(&m_fontNormal);

    m_editYear.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER | ES_NUMBER,
        CRect(0, 0, 55, 24), this, IDC_YEAR_EDIT);
    m_editYear.SetFont(&m_fontNormal);

    m_spinYear.Create(WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_ARROWKEYS,
        CRect(0, 0, 18, 24), this, IDC_YEAR_SPIN);
    m_spinYear.SetBuddy(&m_editYear);
    m_spinYear.SetRange(1, 9999);

    PopulateMonthCombo();
    UpdateMonthYearControls();

    RefreshZmanim();
    RefreshWebCalendarEvents();
    SetTimer(1, 60000, nullptr);
    if (m_minimizeToTray)
        AddTrayIcon();
    if (m_minimizeToTray && s.minimizeOnStartup)
    {
        ShowWindow(SW_HIDE);
    }
    return 0;
}

// =============================================================================
// ON DESTROY
// =============================================================================

void CMainFrame::OnDestroy()
{
    KillTimer(1);
    RemoveTrayIcon();
    WINDOWPLACEMENT wp = {};
    wp.length = sizeof(wp);
    GetWindowPlacement(&wp);
    AppSettings& s = theApp.m_settings;
    s.windowX = wp.rcNormalPosition.left;
    s.windowY = wp.rcNormalPosition.top;
    s.windowW = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
    s.windowH = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
    SaveSettings(s);
    CFrameWnd::OnDestroy();
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1)
    {
        GregorianDate today = GetTodayGregorian();
        if (today.year != m_today.year ||
            today.month != m_today.month ||
            today.day != m_today.day)
        {
            m_today = today;
            m_todayHebrew = GetTodayHebrew();
            if (m_isInTray) UpdateTrayIcon();
            if (m_pCalView) m_pCalView->Invalidate(FALSE);
        }
        else if (m_isInTray)
        {
            UpdateTrayIcon();
        }
        return;
    }
    CFrameWnd::OnTimer(nIDEvent);
}

void CMainFrame::OnClose()
{
    if (ShouldHideToTrayOnClose())
    {
        HideToTray();
        return;
    }
    CFrameWnd::OnClose();
}

void CMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == SC_CLOSE && ShouldHideToTrayOnClose())
    {
        HideToTray();
        return;
    }
    CFrameWnd::OnSysCommand(nID, lParam);
}

void CMainFrame::OnTrayExit()
{
    RemoveTrayIcon();
    DestroyWindow();
}

bool CMainFrame::ShouldHideToTrayOnClose() const
{
    return m_minimizeToTray &&
        (m_minimizeTrayWhen == 1 || m_minimizeTrayWhen == 2);
}

void CMainFrame::HideToTray()
{
    AddTrayIcon();
    ShowWindow(SW_HIDE);
}

// =============================================================================
// ON SIZE / LAYOUT
// =============================================================================

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
    CFrameWnd::OnSize(nType, cx, cy);
    if (nType == SIZE_MINIMIZED &&
        m_minimizeToTray &&
        (m_minimizeTrayWhen == 0 || m_minimizeTrayWhen == 2))
    {
        AddTrayIcon();
        ShowWindow(SW_HIDE);
        return;
    }
    if (cx > 0 && cy > 0)
        LayoutPanels(cx, cy);
}

void CMainFrame::LayoutPanels(int cx, int cy)
{
    if (!m_pCalView || !m_pSidebar || !m_pZmanim) return;

    int calTop = HEADER_H + DAY_HDR_H;
    int calH   = cy - ZMANIM_H - calTop;
    if (calH < 0) calH = 0;

    m_pSidebar->MoveWindow(0, 0, SIDEBAR_W, cy - ZMANIM_H);
    m_pZmanim ->MoveWindow(0, cy - ZMANIM_H, cx, ZMANIM_H);
    m_pCalView->MoveWindow(SIDEBAR_W, calTop, cx - SIDEBAR_W, calH);

    // ── Navigation buttons (top strip, y=0..38) ──────────────────────────────
    if (m_btnPrevDecade.GetSafeHwnd())
    {
        const int BTN_W = 64, BTN_H = 28, GAP = 4;
        int totalW = 9 * BTN_W + 8 * GAP;
        int availW = cx - SIDEBAR_W;
        int startX = SIDEBAR_W + max(4, (availW - totalW) / 2);
        int btnY   = (38 - BTN_H) / 2;

        auto place = [&](CButton& b) {
            b.MoveWindow(startX, btnY, BTN_W, BTN_H);
            startX += BTN_W + GAP;
        };
        place(m_btnPrevDecade);
        place(m_btnPrevYear);
        place(m_btnPrevMonth);
        place(m_btnPrevDay);
        place(m_btnToday);
        place(m_btnNextDay);
        place(m_btnNextMonth);
        place(m_btnNextYear);
        place(m_btnNextDecade);

        // Print button anchored to the right edge
        if (m_btnPrint.GetSafeHwnd())
            m_btnPrint.MoveWindow(cx - 8 - BTN_W, btnY, BTN_W, BTN_H);
    }

    // ── Month combo + year edit + spin (date display row, y=38..84) ─────────
    if (m_comboMonth.GetSafeHwnd())
    {
        const int COMBO_W = 130, YEAR_W = 55, SPIN_W = 18, CTRL_H = 24, GAP = 4;
        int totalW  = COMBO_W + GAP + YEAR_W + SPIN_W;
        int centerX = SIDEBAR_W + (cx - SIDEBAR_W) / 2;
        int comboX  = centerX - totalW / 2;
        int yearX   = comboX + COMBO_W + GAP;
        int spinX   = yearX + YEAR_W;
        int ctrlY   = 38 + (46 - CTRL_H) / 2;
        // Height 300 allows dropdown to open downward; edit part = CTRL_H
        m_comboMonth.MoveWindow(comboX, ctrlY, COMBO_W, 300);
        m_editYear  .MoveWindow(yearX,  ctrlY, YEAR_W,  CTRL_H);
        m_spinYear  .MoveWindow(spinX,  ctrlY, SPIN_W,  CTRL_H);
    }

    Invalidate(FALSE);
}

// =============================================================================
// ON PAINT
// =============================================================================

void CMainFrame::OnPaint()
{
    CPaintDC dc(this);
    CRect rcClient;
    GetClientRect(&rcClient);
    int cx = rcClient.Width();

    CRect rcHeader(SIDEBAR_W, 0, cx, HEADER_H);
    DrawHeader(&dc, rcHeader);

    CRect rcDayHdr(SIDEBAR_W, HEADER_H, cx, HEADER_H + DAY_HDR_H);
    DrawDayHeaders(&dc, rcDayHdr);
}

void CMainFrame::DrawHeader(CDC* pDC, const CRect& rc)
{
    // Top 38 px: nav-button strip (darker navy)
    CRect rcNav(rc.left, rc.top, rc.right, rc.top + 38);
    pDC->FillSolidRect(rcNav, RGB(35, 60, 115));

    // Bottom 46 px: date-label strip (blue)
    CRect rcDate(rc.left, rc.top + 38, rc.right, rc.bottom);
    pDC->FillSolidRect(rcDate, CLR_HEADER_BG);

    // Build date strings
    GregorianDate first, last;
    CString gregStr, hebFull;

    if (m_hebrewMonthView)
    {
        HebrewDate hFirst(m_viewHebrewYear, m_viewHebrewMonth, 1);
        HebrewDate hLast(m_viewHebrewYear, m_viewHebrewMonth,
            DaysInHebrewMonth(m_viewHebrewMonth, m_viewHebrewYear));
        first = HebrewToGregorian(hFirst);
        last  = HebrewToGregorian(hLast);

        hebFull.Format(L"%s  %d",
            HebrewMonthName(m_viewHebrewMonth,
                IsHebrewLeapYear(m_viewHebrewYear)).c_str(),
            m_viewHebrewYear);

        std::wstring gregRange = GregorianMonthName(first.month);
        if (first.month != last.month)
            gregRange += L" - " + GregorianMonthName(last.month);
        gregRange += L" " + std::to_wstring(first.year);
        if (first.year != last.year)
            gregRange += L" - " + std::to_wstring(last.year);
        gregStr = gregRange.c_str();
    }
    else
    {
        first = GregorianDate(m_viewYear, m_viewMonth, 1);
        last  = GregorianDate(m_viewYear, m_viewMonth,
            DaysInGregorianMonth(m_viewMonth, m_viewYear));
        gregStr.Format(L"%s %d",
            GregorianMonthName(m_viewMonth).c_str(), m_viewYear);
    }

    HebrewDate hFirst = GregorianToHebrew(first);
    HebrewDate hLast  = GregorianToHebrew(last);

    std::wstring hebStr = HebrewMonthName(hFirst.month, IsHebrewLeapYear(hFirst.year));
    if (hFirst.month != hLast.month)
        hebStr += L" - " + HebrewMonthName(hLast.month, IsHebrewLeapYear(hLast.year));

    if (!m_hebrewMonthView)
    {
        hebFull.Format(L"%s  %d", hebStr.c_str(), hFirst.year);
        if (hFirst.year != hLast.year)
            hebFull.AppendFormat(L" - %d", hLast.year);
    }

    // Draw labels in the date strip
    pDC->SelectObject(&m_fontHeader);
    pDC->SetTextColor(CLR_HEADER_TXT);
    pDC->SetBkMode(TRANSPARENT);

    int w = rcDate.Width();
    CRect rcLeft = rcDate;
    rcLeft.right = rcDate.left + w * 38 / 100;
    rcLeft.left += 12;
    pDC->DrawText(gregStr, rcLeft, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    CRect rcRight = rcDate;
    rcRight.left  = rcDate.left + w * 62 / 100;
    rcRight.right -= 12;
    pDC->DrawText(hebFull, rcRight, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void CMainFrame::DrawDayHeaders(CDC* pDC, const CRect& rc)
{
    pDC->FillSolidRect(rc, RGB(70, 100, 160));

    static const wchar_t* days[] = {
        L"Sunday", L"Monday", L"Tuesday", L"Wednesday",
        L"Thursday", L"Friday", L"Shabbos"
    };

    int colW = rc.Width() / 7;
    pDC->SelectObject(&m_fontBold);
    pDC->SetTextColor(RGB(255, 255, 255));
    pDC->SetBkMode(TRANSPARENT);

    for (int i = 0; i < 7; i++)
    {
        CRect rcCol(rc.left + i * colW, rc.top,
            rc.left + (i + 1) * colW, rc.bottom);
        pDC->DrawText(days[i], -1, rcCol,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

// =============================================================================
// NAVIGATION
// =============================================================================

void CMainFrame::PrevMonth()
{
    ChangeMonth(-1);
}

void CMainFrame::NextMonth()
{
    ChangeMonth(1);
}

void CMainFrame::ChangeDay(int deltaDays)
{
    SelectDate(JDNToGregorian(GregorianToJDN(m_selectedDate) + deltaDays));
}

void CMainFrame::ChangeMonth(int deltaMonths)
{
    if (m_hebrewMonthView)
    {
        int year = m_viewHebrewYear;
        int month = m_viewHebrewMonth + deltaMonths;
        while (month < 1)
        {
            year--;
            month += MonthsInHebrewYear(year);
        }
        while (month > MonthsInHebrewYear(year))
        {
            month -= MonthsInHebrewYear(year);
            year++;
        }
        m_viewHebrewYear = year;
        m_viewHebrewMonth = month;
    }
    else
    {
        m_viewMonth += deltaMonths;
        while (m_viewMonth < 1) { m_viewMonth += 12; m_viewYear--; }
        while (m_viewMonth > 12) { m_viewMonth -= 12; m_viewYear++; }
    }

    UpdateMonthYearControls();

    if (m_pCalView)
    {
        m_pCalView->RebuildCells();
        m_pCalView->Invalidate(FALSE);
    }
    if (m_pSidebar) m_pSidebar->Invalidate(FALSE);
    if (m_pZmanim) m_pZmanim->Invalidate(FALSE);
    Invalidate(FALSE);
}

void CMainFrame::ChangeYear(int deltaYears)
{
    if (m_hebrewMonthView)
    {
        m_viewHebrewYear += deltaYears;
        int months = MonthsInHebrewYear(m_viewHebrewYear);
        if (m_viewHebrewMonth > months) m_viewHebrewMonth = months;
    }
    else
    {
        m_viewYear += deltaYears;
    }

    UpdateMonthYearControls();

    if (m_pCalView)
    {
        m_pCalView->RebuildCells();
        m_pCalView->Invalidate(FALSE);
    }
    Invalidate(FALSE);
}

void CMainFrame::GoToToday()
{
    m_today = GetTodayGregorian();
    m_todayHebrew = GetTodayHebrew();
    SelectDate(m_today);
}

// Sets selected date — only invalidates child panels, not the full frame.
void CMainFrame::SelectDate(const GregorianDate& g)
{
    m_selectedDate = g;
    m_selectedHebrew = GregorianToHebrew(g);

    bool outsideView = false;
    if (m_hebrewMonthView)
    {
        outsideView = (m_selectedHebrew.year != m_viewHebrewYear ||
            m_selectedHebrew.month != m_viewHebrewMonth);
    }
    else
    {
        outsideView = (m_viewYear != g.year || m_viewMonth != g.month);
    }

    if (outsideView)
    {
        SyncViewToSelected();
        UpdateMonthYearControls();
        if (m_pCalView) m_pCalView->RebuildCells();
        Invalidate(FALSE);
    }

    m_isDST = IsDST(g, m_location);
    RefreshZmanim();

    if (m_pCalView)  m_pCalView->Invalidate(FALSE);
    if (m_pSidebar)  m_pSidebar->Invalidate(FALSE);
    if (m_pZmanim)   m_pZmanim->Invalidate(FALSE);
}

void CMainFrame::RefreshZmanim()
{
    m_isDST = IsDST(m_selectedDate, m_location);
    m_zmanim = CalculateZmanim(m_selectedDate, m_location, m_isDST);
    m_zmanim.candleLighting = AddMinutes(m_zmanim.shkia,
        -theApp.m_settings.candleLightingMinutes);
}

void CMainFrame::ApplySettings(const AppSettings& s)
{
    m_location = s.ToLocation();
    m_use24hr = s.use24Hour;
    m_isIsrael = s.isIsrael;
    m_showParshios = s.showParshios;
    m_showMoadim = s.showMoadim;
    m_showDafYomi = s.showDafYomi;
    m_showYerushalmi = s.showYerushalmi;
    m_showHalachaYomit = s.showHalachaYomit;
    m_showMishnaYomit = s.showMishnaYomit;
    m_showTanachYomi = s.showTanachYomi;
    m_minimizeToTray = s.minimizeToTray;
    m_minimizeTrayWhen = s.minimizeTrayWhen;
    m_hebrewMonthView = s.defaultHebrewMonth;
    RecreateFonts();
    // Reassign fonts to header controls — their HFONT handle becomes stale after RecreateFonts.
    if (m_comboMonth.GetSafeHwnd())    m_comboMonth.SetFont(&m_fontNormal);
    if (m_editYear.GetSafeHwnd())      m_editYear.SetFont(&m_fontNormal);
    auto setNavFont = [this](CButton& b) { if (b.GetSafeHwnd()) b.SetFont(&m_fontNormal); };
    setNavFont(m_btnPrevDecade); setNavFont(m_btnPrevYear);  setNavFont(m_btnPrevMonth);
    setNavFont(m_btnPrevDay);    setNavFont(m_btnToday);     setNavFont(m_btnNextDay);
    setNavFont(m_btnNextMonth);  setNavFont(m_btnNextYear);  setNavFont(m_btnNextDecade);
    setNavFont(m_btnPrint);
    if (m_pCalView) { m_pCalView->RebuildCells(); m_pCalView->Invalidate(FALSE); }
    if (m_pSidebar) m_pSidebar->Invalidate(FALSE);
    if (m_pZmanim)  m_pZmanim->Invalidate(FALSE);
}

void CMainFrame::RecreateFonts()
{
    int sz = theApp.m_settings.fontSize;   // 0-6
    int hNorm  = 13 + sz;                  // 13-19pt (default sz=1 → 14pt)
    int hBold  = hNorm + 1;
    int hSmall = hNorm;                    // buttons same size as normal text
    int hHdr   = hNorm + 6;
    auto make = [](CFont& f, int h, int weight) {
        if (f.GetSafeHandle()) f.DeleteObject();
        f.CreateFont(h, 0, 0, 0, weight, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    };
    make(m_fontNormal, hNorm,  FW_NORMAL);
    make(m_fontBold,   hBold,  FW_BOLD);
    make(m_fontSmall,  hSmall, FW_NORMAL);
    make(m_fontHeader, hHdr,   FW_BOLD);
}

// Parses an ICS file from disk and returns the event list.
static std::vector<UserEventInfo> ParseIcsFromPath(const std::wstring& icsPath)
{
    std::vector<UserEventInfo> events;

    std::ifstream f(icsPath, std::ios::binary);
    if (!f.is_open()) return events;

    std::string bytes((std::istreambuf_iterator<char>(f)),
                       std::istreambuf_iterator<char>());
    std::wstring text = DecodeUtf8(bytes);
    if (!text.empty() && text[0] == 0xFEFF)
        text.erase(text.begin());

    // Unfold continuation lines (RFC 5545 §3.1)
    std::wstringstream raw(text);
    std::vector<std::wstring> lines;
    std::wstring rawLine;
    while (std::getline(raw, rawLine))
    {
        if (!rawLine.empty() && rawLine.back() == L'\r')
            rawLine.pop_back();
        if (!rawLine.empty() &&
            (rawLine[0] == L' ' || rawLine[0] == L'\t') &&
            !lines.empty())
        {
            lines.back() += rawLine.substr(1);
        }
        else
        {
            lines.push_back(rawLine);
        }
    }

    bool inEvent = false;
    GregorianDate eventDate;
    std::wstring summary;
    for (const auto& line : lines)
    {
        if (line == L"BEGIN:VEVENT")
        {
            inEvent = true;
            eventDate = GregorianDate();
            summary.clear();
        }
        else if (line == L"END:VEVENT")
        {
            if (inEvent && eventDate.year > 0 && !summary.empty())
                events.push_back({ eventDate, summary });
            inEvent = false;
        }
        else if (inEvent)
        {
            if (line.rfind(L"DTSTART", 0) == 0)
                ParseIcsDate(line, eventDate);
            else if (line.rfind(L"SUMMARY", 0) == 0)
            {
                size_t colon = line.find(L':');
                if (colon != std::wstring::npos)
                    summary = UnescapeIcsText(WebCalTrim(line.substr(colon + 1)));
            }
        }
    }
    return events;
}

void CMainFrame::RefreshWebCalendarEvents()
{
    m_webEvents.clear();

    const auto& cals = theApp.m_settings.webCalendars;
    if (cals.empty()) return;

    std::wstring sp = GetSettingsFilePath();
    size_t sl = sp.find_last_of(L"\\/");
    std::wstring dir = (sl == std::wstring::npos) ? L"." : sp.substr(0, sl);

    // Collect enabled URLs with their cache paths
    struct CalJob { std::wstring url; std::wstring path; };
    std::vector<CalJob> jobs;
    for (int i = 0; i < (int)cals.size(); i++)
    {
        if (!cals[i].enabled || cals[i].url.empty()) continue;
        std::wstring url = cals[i].url;
        if (url.rfind(L"webcals://", 0) == 0)      url.replace(0, 10, L"https://");
        else if (url.rfind(L"webcal://", 0) == 0)  url.replace(0, 9,  L"https://");
        std::wstring path = dir + L"\\webcalendar" + std::to_wstring(i) + L".ics";
        jobs.push_back({ url, path });
    }
    if (jobs.empty()) return;

    // Keep m_icsPath pointing to first cache file (used in OnWebCalDone)
    m_icsPath = jobs[0].path;

    // Load any already-cached files immediately
    for (const auto& j : jobs)
    {
        auto ev = ParseIcsFromPath(j.path);
        m_webEvents.insert(m_webEvents.end(), ev.begin(), ev.end());
    }
    if (!m_webEvents.empty())
    {
        if (m_pCalView)  m_pCalView->Invalidate(FALSE);
        if (m_pSidebar)  m_pSidebar->Invalidate(FALSE);
    }

    // Download fresh copies in background; post WM_WEBCAL_DONE when all done
    HWND hWnd = GetSafeHwnd();
    std::thread([jobs, hWnd]() {
        for (const auto& j : jobs)
            URLDownloadToFileW(nullptr, j.url.c_str(), j.path.c_str(), 0, nullptr);
        ::PostMessage(hWnd, WM_WEBCAL_DONE, 0, 0);
    }).detach();
}

LRESULT CMainFrame::OnWebCalDone(WPARAM, LPARAM)
{
    m_webEvents.clear();
    const auto& cals = theApp.m_settings.webCalendars;
    std::wstring sp = GetSettingsFilePath();
    size_t sl = sp.find_last_of(L"\\/");
    std::wstring dir = (sl == std::wstring::npos) ? L"." : sp.substr(0, sl);
    for (int i = 0; i < (int)cals.size(); i++)
    {
        if (!cals[i].enabled) continue;
        std::wstring path = dir + L"\\webcalendar" + std::to_wstring(i) + L".ics";
        auto ev = ParseIcsFromPath(path);
        m_webEvents.insert(m_webEvents.end(), ev.begin(), ev.end());
    }
    if (m_pCalView)  m_pCalView->Invalidate(FALSE);
    if (m_pSidebar)  m_pSidebar->Invalidate(FALSE);
    return 0;
}

std::vector<std::wstring> CMainFrame::GetUserEventsForDate(const GregorianDate& g) const
{
    std::vector<std::wstring> events;
    if (!theApp.m_settings.showUserEvents)
        return events;

    for (const auto& event : m_webEvents)
    {
        if (event.date.year == g.year &&
            event.date.month == g.month &&
            event.date.day == g.day)
            events.push_back(event.title);
    }
    return events;
}

std::pair<std::wstring, std::wstring> CMainFrame::GetCellZmanimLabels(
    const GregorianDate& g, const HebrewDate& h,
    const std::vector<HolidayInfo>& hols) const
{
    std::wstring candleStr, motzStr;

    bool isDSTLocal = IsDST(g, m_location);
    ZmanimResult z  = CalculateZmanim(g, m_location, isDSTLocal);
    z.candleLighting = AddMinutes(z.shkia, -theApp.m_settings.candleLightingMinutes);

    DayOfWeek dow     = GetDayOfWeek(g);
    bool isFriday     = (dow == FRIDAY);
    bool isShabbat    = (dow == SHABBAT);

    auto hasFlag = [&](int flag) -> bool {
        for (const auto& ho : hols)
            if (ho.flags & flag) return true;
        return false;
    };
    bool hasYomTov = hasFlag(HOLIDAY_YOM_TOV);
    bool hasErev   = hasFlag(HOLIDAY_EREV);

    // Peek at next day to detect multi-day YT
    long      jdn     = GregorianToJDN(g);
    HebrewDate hNext  = JDNToHebrew(jdn + 1);
    auto holsNext     = GetHolidays(hNext, m_isIsrael);
    bool nextDayIsYT  = false;
    for (const auto& ho : holsNext)
        if (ho.flags & HOLIDAY_YOM_TOV) { nextDayIsYT = true; break; }

    // Erev Yom Kippur (9 Tishrei) has no HOLIDAY_EREV in the engine — check explicitly
    bool isErevYomKippur = (h.month == TISHREI && h.day == 9);

    // --- Candle string (orange) ---
    if (isFriday && z.candleLighting.IsValid())
    {
        candleStr = L"Cand " + FormatTime(z.candleLighting, m_use24hr);
    }
    else if (!isFriday && !isShabbat
             && (hasErev || isErevYomKippur) && z.candleLighting.IsValid())
    {
        // Erev YT / Erev Yom Kippur on a weekday
        candleStr = L"Cand " + FormatTime(z.candleLighting, m_use24hr);
    }
    else if (!isFriday && !isShabbat && hasYomTov && !m_isIsrael && nextDayIsYT
             && z.tzeit_GRA.IsValid())
    {
        // 1st day YT diaspora — light candles at tzeis for 2nd day
        candleStr = L"Cand " + FormatTime(z.tzeit_GRA, m_use24hr);
    }

    // --- Motz / Chatz string (blue) ---
    if (isShabbat && z.tzeitShabbat.IsValid())
    {
        motzStr = L"Motz " + FormatTime(z.tzeitShabbat, m_use24hr);
    }
    else if (!isShabbat && !isFriday && hasYomTov && !nextDayIsYT && z.tzeit_GRA.IsValid())
    {
        // Last day of YT
        motzStr = L"Motz " + FormatTime(z.tzeit_GRA, m_use24hr);
    }

    // Chatzot / chatzot layla — fill motzStr slot if still empty
    if (motzStr.empty())
    {
        if (h.month == IYAR && h.day == 18 && z.chatzot.IsValid())
            motzStr = L"Chatz " + FormatTime(z.chatzot, m_use24hr);       // Lag BaOmer
        else if (h.month == AV && h.day == 9 && z.chatzot.IsValid())
            motzStr = L"Chatz " + FormatTime(z.chatzot, m_use24hr);       // Tisha B'Av
        else if (h.month == NISSAN && h.day == 14 && !m_isIsrael
                 && z.chatzot.IsValid())
            motzStr = L"Chatz " + FormatTime(z.chatzot, m_use24hr);       // Erev Pesach diaspora
        else if (h.month == NISSAN && h.day == 15 && !m_isIsrael
                 && z.chatzotLayla.IsValid())
            motzStr = L"Chatz " + FormatTime(z.chatzotLayla, m_use24hr);  // Pesach night 1 diaspora
    }

    return { candleStr, motzStr };
}

void CMainFrame::SyncViewToSelected()
{
    m_viewYear = m_selectedDate.year;
    m_viewMonth = m_selectedDate.month;
    m_viewHebrewYear = m_selectedHebrew.year;
    m_viewHebrewMonth = m_selectedHebrew.month;
}

void CMainFrame::AddTrayIcon()
{
    if (m_isInTray)
    {
        UpdateTrayIcon();
        return;
    }

    ZeroMemory(&m_trayIcon, sizeof(m_trayIcon));
    m_trayIcon.cbSize = sizeof(m_trayIcon);
    m_trayIcon.hWnd = GetSafeHwnd();
    m_trayIcon.uID = 1;
    m_trayIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_trayIcon.uCallbackMessage = WM_WINLUACH_TRAY;
    HebrewDate hToday = CurrentHebrewDateForTray();
    m_hTrayDateIcon = CreateTrayDateIcon(hToday);
    m_trayIcon.hIcon = m_hTrayDateIcon ? m_hTrayDateIcon : AfxGetApp()->LoadIcon(IDI_WINLUACH);
    CString tip;
    tip.Format(L"%d %s %d",
        hToday.day,
        HebrewMonthName(hToday.month, IsHebrewLeapYear(hToday.year)).c_str(),
        hToday.year);
    wcscpy_s(m_trayIcon.szTip, tip);
    Shell_NotifyIcon(NIM_ADD, &m_trayIcon);
    m_isInTray = true;
}

void CMainFrame::UpdateTrayIcon()
{
    if (!m_isInTray) return;
    HebrewDate hToday = CurrentHebrewDateForTray();
    CString tip;
    tip.Format(L"%d %s %d",
        hToday.day,
        HebrewMonthName(hToday.month, IsHebrewLeapYear(hToday.year)).c_str(),
        hToday.year);
    wcscpy_s(m_trayIcon.szTip, tip);
    HICON hNewIcon = CreateTrayDateIcon(hToday);
    if (hNewIcon)
    {
        if (m_hTrayDateIcon)
            DestroyIcon(m_hTrayDateIcon);
        m_hTrayDateIcon = hNewIcon;
        m_trayIcon.hIcon = m_hTrayDateIcon;
        m_trayIcon.uFlags = NIF_TIP | NIF_ICON;
    }
    else
    {
        m_trayIcon.uFlags = NIF_TIP;
    }
    Shell_NotifyIcon(NIM_MODIFY, &m_trayIcon);
}

std::wstring CMainFrame::HebrewDayLetters(int day) const
{
    static const wchar_t* ones[] = {
        L"",
        L"\u05D0", L"\u05D1", L"\u05D2", L"\u05D3", L"\u05D4",
        L"\u05D5", L"\u05D6", L"\u05D7", L"\u05D8"
    };
    static const wchar_t* tens[] = {
        L"", L"\u05D9", L"\u05DB", L"\u05DC"
    };

    if (day == 15) return L"\u05D8\u05D5";
    if (day == 16) return L"\u05D8\u05D6";
    if (day >= 1 && day <= 9) return ones[day];
    if (day >= 10 && day <= 30)
        return std::wstring(tens[day / 10]) + ones[day % 10];
    return std::to_wstring(day);
}

HICON CMainFrame::CreateTrayDateIcon(const HebrewDate& h) const
{
    int size = GetSystemMetrics(SM_CXSMICON);
    if (size < 16) size = 16;
    if (size > 24) size = 24;

    HDC hdcScreen = ::GetDC(nullptr);
    if (!hdcScreen) return nullptr;

    HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, size, size);
    ::ReleaseDC(nullptr, hdcScreen);
    if (!hBmp) return nullptr;

    HDC hdc = CreateCompatibleDC(nullptr);
    HBITMAP hMask = CreateBitmap(size, size, 1, 1, nullptr);
    HDC hdcMask = CreateCompatibleDC(nullptr);
    HBITMAP hOld = (HBITMAP)SelectObject(hdc, hBmp);
    HBITMAP hOldMask = (HBITMAP)SelectObject(hdcMask, hMask);

    RECT fill = { 0, 0, size, size };
    FillRect(hdc, &fill, (HBRUSH)GetStockObject(BLACK_BRUSH));
    FillRect(hdcMask, &fill, (HBRUSH)GetStockObject(WHITE_BRUSH));

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, (COLORREF)theApp.m_settings.trayTextColor);
    SetBkMode(hdcMask, TRANSPARENT);
    SetTextColor(hdcMask, RGB(0, 0, 0));

    std::wstring txt = HebrewDayLetters(h.day);
    HFONT hFont = nullptr;
    HFONT hOldFont = nullptr;
    HFONT hOldMaskFont = nullptr;
    SIZE textSize = {};

    for (int height = size + 10; height >= size - 2; height--)
    {
        HFONT testFont = CreateFontW(
            -height, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        HFONT old = (HFONT)SelectObject(hdc, testFont);
        SIZE measured = {};
        GetTextExtentPoint32W(hdc, txt.c_str(), (int)txt.size(), &measured);
        SelectObject(hdc, old);

        if (measured.cx <= size + 2 && measured.cy <= size + 2)
        {
            hFont = testFont;
            textSize = measured;
            break;
        }
        DeleteObject(testFont);
    }

    if (!hFont)
    {
        hFont = CreateFontW(
            -size, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        hOldFont = (HFONT)SelectObject(hdc, hFont);
        GetTextExtentPoint32W(hdc, txt.c_str(), (int)txt.size(), &textSize);
    }
    else
    {
        hOldFont = (HFONT)SelectObject(hdc, hFont);
    }
    hOldMaskFont = (HFONT)SelectObject(hdcMask, hFont);

    int x = (size - textSize.cx) / 2;
    int y = (size - textSize.cy) / 2 - 1;
    if (x < -1) x = -1;
    if (y < -2) y = -2;
    TextOutW(hdc, x, y, txt.c_str(), (int)txt.size());
    TextOutW(hdcMask, x, y, txt.c_str(), (int)txt.size());

    SelectObject(hdc, hOldFont);
    SelectObject(hdcMask, hOldMaskFont);
    DeleteObject(hFont);
    SelectObject(hdc, hOld);
    SelectObject(hdcMask, hOldMask);
    DeleteDC(hdc);
    DeleteDC(hdcMask);

    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmColor = hBmp;
    ii.hbmMask = hMask;
    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hBmp);
    DeleteObject(hMask);
    return hIcon;
}

HebrewDate CMainFrame::CurrentHebrewDateForTray()
{
    GregorianDate g = GetTodayGregorian();
    bool isDST = IsDST(g, m_location);
    ZmanimResult z = CalculateZmanim(g, m_location, isDST);

    SYSTEMTIME st = {};
    GetLocalTime(&st);
    bool afterTzeit = z.tzeit_GRA.IsValid() &&
        (st.wHour > z.tzeit_GRA.hour ||
            (st.wHour == z.tzeit_GRA.hour && st.wMinute >= z.tzeit_GRA.minute));

    if (afterTzeit)
        g = JDNToGregorian(GregorianToJDN(g) + 1);

    return GregorianToHebrew(g);
}

void CMainFrame::RemoveTrayIcon()
{
    if (!m_isInTray) return;
    Shell_NotifyIcon(NIM_DELETE, &m_trayIcon);
    m_isInTray = false;
    if (m_hTrayDateIcon)
    {
        DestroyIcon(m_hTrayDateIcon);
        m_hTrayDateIcon = nullptr;
    }
}

void CMainFrame::RestoreFromTray()
{
    ShowWindow(SW_SHOWNORMAL);
    SetForegroundWindow();
}

LRESULT CMainFrame::OnTrayNotify(WPARAM wParam, LPARAM lParam)
{
    if (wParam != 1) return 0;
    if (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONUP)
        RestoreFromTray();
    else if (lParam == WM_RBUTTONUP)
    {
        CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenu(MF_STRING, ID_TRAY_OPEN, L"&Open WinLuach");
        menu.AppendMenu(MF_STRING, ID_TRAY_ABOUT, L"&About...");
        menu.AppendMenu(MF_SEPARATOR);
        menu.AppendMenu(MF_STRING, ID_TRAY_EXIT, L"E&xit");

        CPoint pt;
        GetCursorPos(&pt);
        SetForegroundWindow();
        menu.TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);
    }
    return 0;
}

// =============================================================================
// MENU HANDLERS
// =============================================================================

void CMainFrame::OnViewPrevDay() { ChangeDay(-1); }
void CMainFrame::OnViewNextDay() { ChangeDay(1); }
void CMainFrame::OnViewToday() { GoToToday(); }
void CMainFrame::OnViewPrevMonth() { PrevMonth(); }
void CMainFrame::OnViewNextMonth() { NextMonth(); }
void CMainFrame::OnViewPrevYear() { ChangeYear(-1); }
void CMainFrame::OnViewNextYear() { ChangeYear(1); }
void CMainFrame::OnViewPrevDecade() { ChangeYear(-10); }
void CMainFrame::OnViewNextDecade() { ChangeYear(10); }

void CMainFrame::OnOptionsLocation()
{
    CLocationDlg dlg(m_location, this);
    if (dlg.DoModal() == IDOK)
    {
        m_location = dlg.GetSelectedLocation();
        AppSettings& s = theApp.m_settings;
        s.locationName = m_location.name;
        s.latitude = m_location.latitude;
        s.longitude = m_location.longitude;
        s.elevation = m_location.elevation;
        s.gmtOffset = m_location.gmtOffset;
        s.usesDST = m_location.usesDST;
        if (dlg.GetSelectedIsCustom())
            s.isIsrael = dlg.GetSelectedIsIsrael();
        SaveSettings(s);
        m_isIsrael = s.isIsrael;
        RefreshZmanim();
        if (m_pCalView) { m_pCalView->RebuildCells(); m_pCalView->Invalidate(FALSE); }
        if (m_pZmanim)  m_pZmanim->Invalidate(FALSE);
        if (m_pSidebar) m_pSidebar->Invalidate(FALSE);
    }
}

void CMainFrame::OnOptionsPrefs()
{
    COptionsDlg dlg(theApp.m_settings, this);
    if (dlg.DoModal() == IDOK)
    {
        theApp.m_settings = dlg.GetSettings();
        ApplySettings(theApp.m_settings);
        SaveSettings(theApp.m_settings);
        PopulateMonthCombo();
        UpdateMonthYearControls();
        if (m_minimizeToTray)
            AddTrayIcon();
        else
            RemoveTrayIcon();
        RefreshWebCalendarEvents();
        if (m_pCalView)
        {
            m_pCalView->RebuildCells();
            m_pCalView->Invalidate(FALSE);
        }
        if (m_pZmanim)   m_pZmanim->Invalidate(FALSE);
        if (m_pSidebar)  m_pSidebar->Invalidate(FALSE);
    }
}

void CMainFrame::OnHelpAbout()
{
    MessageBoxW(
        L"2026 WinLuach - Hebrew Calendar\n\n"
        L"Version 0.6.0\n\n"
        L"A modern Hebrew/Gregorian calendar\n"
        L"with halachic times (zmanim).\n\n"
        L"Built with C++ and MFC.",
        L"About WinLuach",
        MB_OK | MB_ICONINFORMATION);
}

// =============================================================================
// MONTH / YEAR PICKER CONTROLS
// =============================================================================

void CMainFrame::PopulateMonthCombo()
{
    if (!m_comboMonth.GetSafeHwnd()) return;
    m_updatingControls = true;
    m_comboMonth.ResetContent();
    if (m_hebrewMonthView)
    {
        int months = MonthsInHebrewYear(m_viewHebrewYear);
        bool leap  = IsHebrewLeapYear(m_viewHebrewYear);
        for (int m = 1; m <= months; m++)
            m_comboMonth.AddString(HebrewMonthName(m, leap).c_str());
    }
    else
    {
        for (int m = 1; m <= 12; m++)
            m_comboMonth.AddString(GregorianMonthName(m).c_str());
    }
    m_updatingControls = false;
}

void CMainFrame::UpdateMonthYearControls()
{
    if (!m_comboMonth.GetSafeHwnd()) return;
    m_updatingControls = true;

    int month = m_hebrewMonthView ? m_viewHebrewMonth : m_viewMonth;
    int year  = m_hebrewMonthView ? m_viewHebrewYear  : m_viewYear;

    // Re-populate if the number of months changed (Hebrew leap year toggle)
    if (m_hebrewMonthView)
    {
        int expectedMonths = MonthsInHebrewYear(m_viewHebrewYear);
        if (m_comboMonth.GetCount() != expectedMonths)
        {
            m_updatingControls = false;
            PopulateMonthCombo();
            m_updatingControls = true;
        }
    }

    m_comboMonth.SetCurSel(month - 1);  // 0-based index

    CString yearStr;
    yearStr.Format(L"%d", year);
    m_editYear.SetWindowText(yearStr);

    m_updatingControls = false;
}

void CMainFrame::OnMonthComboChange()
{
    if (m_updatingControls) return;
    int sel = m_comboMonth.GetCurSel();
    if (sel == CB_ERR) return;

    int newMonth = sel + 1;  // 1-based
    if (m_hebrewMonthView)
    {
        if (newMonth == m_viewHebrewMonth) return;
        m_viewHebrewMonth = newMonth;
    }
    else
    {
        if (newMonth == m_viewMonth) return;
        m_viewMonth = newMonth;
    }

    if (m_pCalView) { m_pCalView->RebuildCells(); m_pCalView->Invalidate(FALSE); }
    if (m_pSidebar) m_pSidebar->Invalidate(FALSE);
    Invalidate(FALSE);
}

void CMainFrame::OnYearSpinDelta(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMUPDOWN* pUD = reinterpret_cast<NMUPDOWN*>(pNMHDR);
    ChangeYear(pUD->iDelta);          // +1 = up arrow = next year
    *pResult = 1;                     // prevent spin from auto-updating buddy
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
    // Ctrl+P — print
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == 'P' &&
        (GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000))
    {
        OnCalPrint();
        return TRUE;
    }

    // Ctrl+G — go to date
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == 'G' &&
        (GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000))
    {
        OnCalGoTo();
        return TRUE;
    }

    // Ctrl+, — open preferences
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_OEM_COMMA &&
        (GetKeyState(VK_CONTROL) & 0x8000))
    {
        OnOptionsPrefs();
        return TRUE;
    }

    // Ctrl+Alt+Left — previous decade
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_LEFT &&
        (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_MENU) & 0x8000))
    {
        ChangeYear(-10);
        return TRUE;
    }
    // Ctrl+Alt+Right — next decade
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RIGHT &&
        (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_MENU) & 0x8000))
    {
        ChangeYear(10);
        return TRUE;
    }

    // Ctrl + Plus/= — larger font
    if (pMsg->message == WM_KEYDOWN &&
        (pMsg->wParam == VK_OEM_PLUS || pMsg->wParam == (WPARAM)'+') &&
        (GetKeyState(VK_CONTROL) & 0x8000))
    {
        OnViewZoomIn();
        return TRUE;
    }
    // Ctrl + Minus — smaller font
    if (pMsg->message == WM_KEYDOWN &&
        (pMsg->wParam == VK_OEM_MINUS || pMsg->wParam == (WPARAM)'-') &&
        (GetKeyState(VK_CONTROL) & 0x8000))
    {
        OnViewZoomOut();
        return TRUE;
    }
    // Ctrl + 0 — reset font to default (Normal = index 3)
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == '0' &&
        (GetKeyState(VK_CONTROL) & 0x8000))
    {
        OnViewZoomReset();
        return TRUE;
    }

    // Enter in year edit: navigate to typed year
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN &&
        m_editYear.GetSafeHwnd() && pMsg->hwnd == m_editYear.GetSafeHwnd())
    {
        CString s;
        m_editYear.GetWindowText(s);
        int year = _wtoi(s);
        if (year >= 1 && year <= 9999)
        {
            int cur = m_hebrewMonthView ? m_viewHebrewYear : m_viewYear;
            if (year != cur) ChangeYear(year - cur);
        }
        UpdateMonthYearControls();
        if (m_pCalView) m_pCalView->SetFocus();
        return TRUE;
    }

    // Mouse-wheel over month combo: scroll months
    if (pMsg->message == WM_MOUSEWHEEL && m_comboMonth.GetSafeHwnd())
    {
        POINT pt;
        GetCursorPos(&pt);
        CRect rc;
        m_comboMonth.GetWindowRect(&rc);
        if (rc.PtInRect(CPoint(pt)))
        {
            int delta = GET_WHEEL_DELTA_WPARAM(pMsg->wParam);
            ChangeMonth(delta > 0 ? -1 : 1);
            return TRUE;
        }

        // Mouse-wheel over year edit or spin: scroll years
        CRect rcY, rcS;
        m_editYear.GetWindowRect(&rcY);
        m_spinYear.GetWindowRect(&rcS);
        rcY.UnionRect(rcY, rcS);
        if (rcY.PtInRect(CPoint(pt)))
        {
            int delta = GET_WHEEL_DELTA_WPARAM(pMsg->wParam);
            ChangeYear(delta > 0 ? 1 : -1);
            return TRUE;
        }
    }

    return CFrameWnd::PreTranslateMessage(pMsg);
}

// =============================================================================
// PRINT
// =============================================================================

void CMainFrame::OnCalPrint()
{
    CCalPrintDlg dlg(this, this);
    if (dlg.DoModal() == IDOK)
        DoPrint(dlg.GetOptions(), this);
}

void CMainFrame::OnCalPreview()
{
    CalPrintOptions opts;
    const auto& ps = theApp.m_settings;
    opts.landscape = ps.printLandscape;
    opts.range     = (CalPrintOptions::Range)ps.printRange;
    opts.mTop      = ps.printMarginTop;
    opts.mBot      = ps.printMarginBot;
    opts.mLeft     = ps.printMarginLeft;
    opts.mRight    = ps.printMarginRight;
    CCalPreviewDlg dlg(opts, this, this);
    dlg.DoModal();
}

BOOL CMainFrame::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    if (GetKeyState(VK_CONTROL) & 0x8000)
    {
        if (zDelta > 0) OnViewZoomIn(); else OnViewZoomOut();
        return TRUE;
    }
    return CFrameWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CMainFrame::OnFileBackup()
{
    wchar_t file[MAX_PATH] = L"WinLuach_settings_backup.json";
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = GetSafeHwnd();
    ofn.lpstrFilter = L"JSON files\0*.json\0All files\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"json";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&ofn)) return;

    std::wstring src = GetSettingsFilePath();
    if (CopyFileW(src.c_str(), file, FALSE))
        MessageBox(L"Settings backed up successfully.", L"WinLuach", MB_OK | MB_ICONINFORMATION);
    else
        MessageBox(L"Backup failed.", L"WinLuach", MB_OK | MB_ICONERROR);
}

void CMainFrame::OnViewZoomIn()
{
    if (theApp.m_settings.fontSize < 6) {
        theApp.m_settings.fontSize++;
        ApplySettings(theApp.m_settings);
        SaveSettings(theApp.m_settings);
    }
}

void CMainFrame::OnViewZoomOut()
{
    if (theApp.m_settings.fontSize > 0) {
        theApp.m_settings.fontSize--;
        ApplySettings(theApp.m_settings);
        SaveSettings(theApp.m_settings);
    }
}

void CMainFrame::OnViewZoomReset()
{
    theApp.m_settings.fontSize = 1;   // Default (index 1 → 14pt)
    ApplySettings(theApp.m_settings);
    SaveSettings(theApp.m_settings);
}

void CMainFrame::OnCalGoTo()
{
    CGotoDateDlg dlg(m_selectedDate, this);
    if (dlg.DoModal() == IDOK)
        SelectDate(dlg.selectedDate);
}

void CMainFrame::OnFileRestore()
{
    if (MessageBox(L"Restore settings from a backup file? Current settings will be overwritten.",
        L"WinLuach", MB_YESNO | MB_ICONQUESTION) != IDYES) return;

    wchar_t file[MAX_PATH] = {};
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = GetSafeHwnd();
    ofn.lpstrFilter = L"JSON files\0*.json\0All files\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;

    std::wstring dst = GetSettingsFilePath();
    if (CopyFileW(file, dst.c_str(), FALSE))
    {
        LoadSettings(theApp.m_settings);
        ApplySettings(theApp.m_settings);
        PopulateMonthCombo();
        UpdateMonthYearControls();
        MessageBox(L"Settings restored. Restart recommended.", L"WinLuach", MB_OK | MB_ICONINFORMATION);
    }
    else
        MessageBox(L"Restore failed.", L"WinLuach", MB_OK | MB_ICONERROR);
}
