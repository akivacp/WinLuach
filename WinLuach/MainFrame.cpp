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
#include "WinLuachApp.h"
#include "Resource.h"
#include <urlmon.h>
#include <fstream>
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
    ON_COMMAND(ID_JUMP_GO, &CMainFrame::OnJumpGo)
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
    fileMenu.AppendMenu(MF_STRING, ID_APP_EXIT, L"E&xit");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)fileMenu.Detach(), L"&File");

    CMenu viewMenu;
    viewMenu.CreatePopupMenu();
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_PREVDAY, L"Previous &Day");
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_NEXTDAY, L"Next D&ay");
    viewMenu.AppendMenu(MF_SEPARATOR);
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_TODAY, L"&Today\tHome");
    viewMenu.AppendMenu(MF_SEPARATOR);
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_PREVMONTH, L"&Previous Month\tPage Up");
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_NEXTMONTH, L"&Next Month\tPage Down");
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_PREVYEAR, L"Previous &Year");
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_NEXTYEAR, L"Next Y&ear");
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_PREVDECADE, L"Previous De&cade");
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_NEXTDECADE, L"Next Deca&de");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)viewMenu.Detach(), L"&View");

    CMenu optMenu;
    optMenu.CreatePopupMenu();
    optMenu.AppendMenu(MF_STRING, ID_OPTIONS_LOCATION, L"&Location...");
    optMenu.AppendMenu(MF_STRING, ID_OPTIONS_PREFS, L"&Preferences...");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)optMenu.Detach(), L"&Options");

    CMenu helpMenu;
    helpMenu.CreatePopupMenu();
    helpMenu.AppendMenu(MF_STRING, ID_HELP_ABOUT, L"&About WinLuach...");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)helpMenu.Detach(), L"&Help");

    SetMenu(&menu);
    menu.Detach();

    // Fonts
    m_fontNormal.CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    m_fontBold.CreateFont(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    m_fontSmall.CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    m_fontHeader.CreateFont(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    // Load settings
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

    // Navigation buttons in the header bar
    const DWORD btnStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
    CRect r(0, 0, 64, 28);
    m_btnPrevDecade.Create(L"«Decade", btnStyle, r, this, ID_VIEW_PREVDECADE);
    m_btnPrevYear  .Create(L"« Year",  btnStyle, r, this, ID_VIEW_PREVYEAR);
    m_btnPrevMonth .Create(L"« Month", btnStyle, r, this, ID_VIEW_PREVMONTH);
    m_btnPrevDay   .Create(L"« Day",   btnStyle, r, this, ID_VIEW_PREVDAY);
    m_btnToday     .Create(L"Today",   btnStyle, r, this, ID_VIEW_TODAY);
    m_btnNextDay   .Create(L"Day »",   btnStyle, r, this, ID_VIEW_NEXTDAY);
    m_btnNextMonth .Create(L"Month »", btnStyle, r, this, ID_VIEW_NEXTMONTH);
    m_btnNextYear  .Create(L"Year »",  btnStyle, r, this, ID_VIEW_NEXTYEAR);
    m_btnNextDecade.Create(L"Decade»", btnStyle, r, this, ID_VIEW_NEXTDECADE);

    CFont* pSF = &m_fontSmall;
    m_btnPrevDecade.SetFont(pSF);  m_btnPrevYear.SetFont(pSF);
    m_btnPrevMonth .SetFont(pSF);  m_btnPrevDay .SetFont(pSF);
    m_btnToday     .SetFont(pSF);
    m_btnNextDay   .SetFont(pSF);  m_btnNextMonth.SetFont(pSF);
    m_btnNextYear  .SetFont(pSF);  m_btnNextDecade.SetFont(pSF);

    // Month quick-jump edit and Go button
    m_editJump.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER | ES_AUTOHSCROLL,
        CRect(0, 0, 180, 24), this, IDC_JUMP_EDIT);
    m_editJump.SetFont(&m_fontNormal);
    m_btnGo.Create(L"Go", btnStyle, CRect(0, 0, 40, 24), this, ID_JUMP_GO);
    m_btnGo.SetFont(pSF);
    UpdateJumpField();

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
    }

    // ── Jump edit + Go (date display row, y=38..84) ───────────────────────────
    if (m_editJump.GetSafeHwnd())
    {
        const int EDIT_W = 180, GO_W = 44, CTRL_H = 24;
        int centerX = SIDEBAR_W + (cx - SIDEBAR_W) / 2;
        int editX   = centerX - (EDIT_W + GO_W + 4) / 2;
        int goX     = editX + EDIT_W + 4;
        int ctrlY   = 38 + (46 - CTRL_H) / 2;
        m_editJump.MoveWindow(editX, ctrlY, EDIT_W, CTRL_H);
        m_btnGo   .MoveWindow(goX,   ctrlY, GO_W,   CTRL_H);
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
            gregRange += L" – " + GregorianMonthName(last.month);
        gregRange += L" " + std::to_wstring(first.year);
        if (first.year != last.year)
            gregRange += L" – " + std::to_wstring(last.year);
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
        hebStr += L" – " + HebrewMonthName(hLast.month, IsHebrewLeapYear(hLast.year));

    if (!m_hebrewMonthView)
    {
        hebFull.Format(L"%s  %d", hebStr.c_str(), hFirst.year);
        if (hFirst.year != hLast.year)
            hebFull.AppendFormat(L" – %d", hLast.year);
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

    UpdateJumpField();

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

    UpdateJumpField();

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
        UpdateJumpField();
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
}

void CMainFrame::RefreshWebCalendarEvents()
{
    m_webEvents.clear();
    if (theApp.m_settings.webCalendarUrl.empty())
        return;

    std::wstring path = GetSettingsFilePath();
    size_t slash = path.find_last_of(L"\\/");
    std::wstring dir = (slash == std::wstring::npos) ? L"." : path.substr(0, slash);
    std::wstring icsPath = dir + L"\\webcalendar.ics";

    HRESULT hr = URLDownloadToFileW(nullptr,
        theApp.m_settings.webCalendarUrl.c_str(),
        icsPath.c_str(),
        0,
        nullptr);
    if (FAILED(hr))
        return;

    std::ifstream f(icsPath, std::ios::binary);
    if (!f.is_open())
        return;

    std::string bytes((std::istreambuf_iterator<char>(f)),
        std::istreambuf_iterator<char>());
    std::wstring text = DecodeUtf8(bytes);
    if (!text.empty() && text[0] == 0xFEFF)
        text.erase(text.begin());

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
            continue;
        }
        if (line == L"END:VEVENT")
        {
            if (inEvent && eventDate.year > 0 && !summary.empty())
                m_webEvents.push_back({ eventDate, summary });
            inEvent = false;
            continue;
        }
        if (!inEvent) continue;

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
        SaveSettings(s);
        RefreshZmanim();
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
        L"Version 0.5.3\n\n"
        L"A modern Hebrew/Gregorian calendar\n"
        L"with halachic times (zmanim).\n\n"
        L"Built with C++ and MFC.",
        L"About WinLuach",
        MB_OK | MB_ICONINFORMATION);
}

// =============================================================================
// JUMP FIELD
// =============================================================================

void CMainFrame::UpdateJumpField()
{
    if (!m_editJump.GetSafeHwnd()) return;
    CString text;
    if (m_hebrewMonthView)
    {
        text.Format(L"%s %d",
            HebrewMonthName(m_viewHebrewMonth,
                IsHebrewLeapYear(m_viewHebrewYear)).c_str(),
            m_viewHebrewYear);
    }
    else
    {
        text.Format(L"%s %d",
            GregorianMonthName(m_viewMonth).c_str(),
            m_viewYear);
    }
    m_editJump.SetWindowText(text);
}

static int ParseGregMonth(const std::wstring& s)
{
    static const wchar_t* names[12] = {
        L"january", L"february", L"march", L"april", L"may", L"june",
        L"july", L"august", L"september", L"october", L"november", L"december"
    };
    std::wstring lower = s;
    for (auto& c : lower) c = (wchar_t)towlower(c);
    for (int i = 0; i < 12; i++)
    {
        if (lower == names[i]) return i + 1;
        if (lower.size() >= 3 && std::wstring(names[i]).substr(0, 3) == lower.substr(0, 3))
            return i + 1;
    }
    try { int m = std::stoi(s); if (m >= 1 && m <= 12) return m; } catch (...) {}
    return 0;
}

static int ParseHebMonth(const std::wstring& s, bool isLeap)
{
    struct { const wchar_t* name; int m; } tbl[] = {
        { L"tishrei",     TISHREI  }, { L"tishri",      TISHREI  }, { L"tishrey",     TISHREI  },
        { L"cheshvan",    CHESHVAN }, { L"heshvan",     CHESHVAN }, { L"marcheshvan", CHESHVAN },
        { L"kislev",      KISLEV   }, { L"kislef",      KISLEV   },
        { L"tevet",       TEVET    },
        { L"shvat",       SHVAT    }, { L"shevat",      SHVAT    }, { L"shvos",       SHVAT    },
        { L"adar i",      ADAR     }, { L"adar 1",      ADAR     },
        { L"adar ii",     ADAR_II  }, { L"adar 2",      ADAR_II  },
        { L"adar",        ADAR     },
        { L"nissan",      NISSAN   }, { L"nisan",       NISSAN   },
        { L"iyar",        IYAR     }, { L"iyyar",       IYAR     },
        { L"sivan",       SIVAN    },
        { L"tammuz",      TAMMUZ   }, { L"tamuz",       TAMMUZ   },
        { L"av",          AV       },
        { L"elul",        ELUL     },
        { nullptr, 0 }
    };
    std::wstring lower = s;
    for (auto& c : lower) c = (wchar_t)towlower(c);
    for (int i = 0; tbl[i].name; i++)
        if (lower == tbl[i].name) return tbl[i].m;
    return 0;
}

void CMainFrame::OnJumpGo()
{
    CString ctext;
    m_editJump.GetWindowText(ctext);
    std::wstring s = ctext.GetString();

    auto trimFn = [](std::wstring& str) {
        size_t f = str.find_first_not_of(L" \t\r\n");
        if (f == std::wstring::npos) { str.clear(); return; }
        str = str.substr(f, str.find_last_not_of(L" \t\r\n") - f + 1);
    };
    trimFn(s);
    if (s.empty()) return;

    size_t sp = s.rfind(L' ');
    if (sp == std::wstring::npos) { MessageBeep(MB_ICONEXCLAMATION); return; }

    int year = 0;
    try { year = std::stoi(s.substr(sp + 1)); } catch (...) {}
    if (year <= 0) { MessageBeep(MB_ICONEXCLAMATION); return; }

    std::wstring monthStr = s.substr(0, sp);
    trimFn(monthStr);

    // Try Gregorian first
    int gregMonth = ParseGregMonth(monthStr);
    if (gregMonth > 0 && year >= 1 && year <= 9999)
    {
        m_viewYear  = year;
        m_viewMonth = gregMonth;
        m_hebrewMonthView = false;
        if (m_pCalView) { m_pCalView->RebuildCells(); m_pCalView->Invalidate(FALSE); }
        Invalidate(FALSE);
        UpdateJumpField();
        if (m_pCalView) m_pCalView->SetFocus();
        return;
    }

    // Try Hebrew
    bool isLeap  = IsHebrewLeapYear(year);
    int hebMonth = ParseHebMonth(monthStr, isLeap);
    if (hebMonth > 0)
    {
        if (hebMonth == ADAR_II && !isLeap) { MessageBeep(MB_ICONEXCLAMATION); return; }
        m_viewHebrewYear  = year;
        m_viewHebrewMonth = hebMonth;
        m_hebrewMonthView = true;
        if (m_pCalView) { m_pCalView->RebuildCells(); m_pCalView->Invalidate(FALSE); }
        Invalidate(FALSE);
        UpdateJumpField();
        if (m_pCalView) m_pCalView->SetFocus();
        return;
    }

    MessageBeep(MB_ICONEXCLAMATION);
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN &&
        m_editJump.GetSafeHwnd() && pMsg->hwnd == m_editJump.GetSafeHwnd())
    {
        OnJumpGo();
        return TRUE;
    }
    return CFrameWnd::PreTranslateMessage(pMsg);
}
