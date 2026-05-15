// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    MainWindow.cpp
// Purpose: Full implementation of the main application window.
//          Draws the calendar grid, day details sidebar, zmanim panel,
//          and toolbar. Handles all user interaction.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial implementation. Calendar grid with 42 cells (6x7),
//          day details sidebar, zmanim panel, toolbar buttons.
//          Color scheme: yellow=today, green=Shabbos,
//          orange=Yom Tov, blue=Rosh Chodesh.
//          Keyboard navigation: arrow keys, Page Up/Down, Home=today.
// v0.1.1 - Cosmetic fixes: Omer text wraps in sidebar, holiday name/subtitle
//          separator changed to " - ", Yom Yerushalayim subtitle removed.
// v0.1.2 - Wired toolbar < > Today buttons to mouse click handler.
// v0.1.3 - Added Location button and OnLocationClick() handler.
//          Clicking Location opens LocationDialog; zmanim update on change.
// v0.1.4 - Added Options button and OnOptionsClick() handler.
//          Options dialog controls 24hr clock, Israel/Diaspora, zmanim shita.
//          Settings saved to disk on every change.
// =============================================================================

#include "MainWindow.h"
#include "LocationDialog.h"
#include "OptionsDialog.h"
#include "Settings.h"
#include <windowsx.h>
#include <sstream>
#include <CommCtrl.h>
#pragma comment(lib, "Comctl32.lib")

// =============================================================================
// STATIC MEMBERS
// =============================================================================

MainWindow* MainWindow::s_instance = nullptr;
HWND        MainWindow::s_hwnd = nullptr;
const wchar_t* MainWindow::CLASS_NAME = L"WinLuachMainWindow";

// =============================================================================
// TOOLBAR BUTTON IDs
// =============================================================================

#define ID_BTN_PREV       101
#define ID_BTN_NEXT       102
#define ID_BTN_TODAY      103
#define ID_BTN_LOCATION   104
#define ID_BTN_HEBREW     105
#define ID_BTN_OPTIONS    106

// =============================================================================
// CREATE
// =============================================================================

// Creates and registers the Win32 window class, then creates the main window.
bool MainWindow::Create(HINSTANCE hInstance, int nCmdShow)
{
    s_instance = new MainWindow();
    s_instance->m_hInstance = hInstance;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = MainWindow::WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) return false;

    s_hwnd = CreateWindowExW(
        0, CLASS_NAME,
        L"WinLuach - Hebrew Calendar",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1100, 700,
        nullptr, nullptr, hInstance, nullptr);

    if (!s_hwnd) return false;

    ShowWindow(s_hwnd, nCmdShow);
    UpdateWindow(s_hwnd);
    return true;
}

// =============================================================================
// WNDPROC
// =============================================================================

// Routes Win32 messages to the MainWindow instance methods.
LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg,
    WPARAM wParam, LPARAM lParam)
{
    MainWindow* self = s_instance;

    switch (msg)
    {
    case WM_CREATE:
        if (self) self->OnCreate(hwnd);
        return 0;
    case WM_PAINT:
        if (self) self->OnPaint(hwnd);
        return 0;
    case WM_SIZE:
        if (self) self->OnSize(hwnd, LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_LBUTTONDOWN:
        if (self) self->OnLButtonDown(hwnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONDBLCLK:
        if (self) self->OnLButtonDblClk(hwnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_KEYDOWN:
        if (self) self->OnKeyDown(hwnd, wParam);
        return 0;
    case WM_COMMAND:
        if (self) self->OnCommand(hwnd, wParam);
        return 0;
    case WM_DESTROY:
        if (self) self->OnDestroy(hwnd);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// =============================================================================
// ON CREATE
// =============================================================================

// Initialises fonts, brushes, pens, and loads saved settings.
void MainWindow::OnCreate(HWND hwnd)
{
    m_hwnd = hwnd;

    // Fonts
    m_fontNormal = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    m_fontBold = CreateFontW(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    m_fontSmall = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    m_fontHeader = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    // Brushes
    m_brToday = CreateSolidBrush(CLR_TODAY);
    m_brShabbos = CreateSolidBrush(CLR_SHABBOS);
    m_brYomTov = CreateSolidBrush(CLR_YOM_TOV);
    m_brRoshChodesh = CreateSolidBrush(CLR_ROSH_CHODESH);
    m_brNormal = CreateSolidBrush(CLR_NORMAL);
    m_brSidebar = CreateSolidBrush(CLR_SIDEBAR_BG);
    m_brZmanim = CreateSolidBrush(CLR_ZMANIM_BG);

    // Pens
    m_penGrid = CreatePen(PS_SOLID, 1, CLR_GRID_LINE);
    m_penHeader = CreatePen(PS_SOLID, 1, CLR_HEADER);

    // Date state
    m_today = GetTodayGregorian();
    m_todayHebrew = GetTodayHebrew();
    m_viewYear = m_today.year;
    m_viewMonth = m_today.month;
    m_selectedDate = m_today;
    m_selectedHebrew = m_todayHebrew;

    // Load saved settings (defaults on first run)
    LoadSettings(m_settings);
    m_location = m_settings.ToLocation();
    m_use24hr = m_settings.use24Hour;
    m_isIsrael = m_settings.isIsrael;
    m_isDST = IsDST(m_today, m_location);

    RefreshMonthData();
    RefreshZmanim();
}

// =============================================================================
// ON DESTROY
// =============================================================================

// Saves settings and releases all GDI resources.
void MainWindow::OnDestroy(HWND hwnd)
{
    // Save window position before cleanup
    RECT rcWin;
    GetWindowRect(m_hwnd, &rcWin);
    m_settings.windowX = rcWin.left;
    m_settings.windowY = rcWin.top;
    m_settings.windowW = rcWin.right - rcWin.left;
    m_settings.windowH = rcWin.bottom - rcWin.top;
    SaveSettings(m_settings);

    // Release GDI resources
    if (m_fontNormal) { DeleteObject(m_fontNormal);    m_fontNormal = nullptr; }
    if (m_fontBold) { DeleteObject(m_fontBold);      m_fontBold = nullptr; }
    if (m_fontSmall) { DeleteObject(m_fontSmall);     m_fontSmall = nullptr; }
    if (m_fontHeader) { DeleteObject(m_fontHeader);    m_fontHeader = nullptr; }
    if (m_brToday) { DeleteObject(m_brToday);       m_brToday = nullptr; }
    if (m_brShabbos) { DeleteObject(m_brShabbos);     m_brShabbos = nullptr; }
    if (m_brYomTov) { DeleteObject(m_brYomTov);      m_brYomTov = nullptr; }
    if (m_brRoshChodesh) { DeleteObject(m_brRoshChodesh); m_brRoshChodesh = nullptr; }
    if (m_brNormal) { DeleteObject(m_brNormal);      m_brNormal = nullptr; }
    if (m_brSidebar) { DeleteObject(m_brSidebar);     m_brSidebar = nullptr; }
    if (m_brZmanim) { DeleteObject(m_brZmanim);      m_brZmanim = nullptr; }
    if (m_penGrid) { DeleteObject(m_penGrid);       m_penGrid = nullptr; }
    if (m_penHeader) { DeleteObject(m_penHeader);     m_penHeader = nullptr; }
}

// =============================================================================
// ON SIZE
// =============================================================================

// Recalculates all layout rectangles when the window is resized.
void MainWindow::OnSize(HWND hwnd, int w, int h)
{
    m_rcToolbar = { 0, 0, w, TOOLBAR_HEIGHT };
    m_rcSidebar = { 0, TOOLBAR_HEIGHT, SIDEBAR_WIDTH, h - ZMANIM_HEIGHT };
    m_rcZmanim = { 0, h - ZMANIM_HEIGHT, w, h };

    int calLeft = SIDEBAR_WIDTH;
    int calTop = TOOLBAR_HEIGHT;
    int calBot = h - ZMANIM_HEIGHT;
    int calW = w - calLeft;

    m_rcHeader = { calLeft, calTop, w, calTop + HEADER_HEIGHT };
    m_rcDayHeaders = { calLeft, calTop + HEADER_HEIGHT,
                       w,       calTop + HEADER_HEIGHT + DAY_HEADER_HEIGHT };
    m_rcGrid = { calLeft, calTop + HEADER_HEIGHT + DAY_HEADER_HEIGHT, w, calBot };

    int baseColW = calW / 7;
    int extra = calW - baseColW * 7;
    m_colX[0] = calLeft;
    for (int i = 1; i <= 7; i++)
        m_colX[i] = m_colX[i - 1] + baseColW + (i == 7 ? extra : 0);

    InvalidateRect(hwnd, nullptr, TRUE);
}

// =============================================================================
// ON PAINT
// =============================================================================

// Paints the entire window using double-buffering to prevent flicker.
void MainWindow::OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdcScreen = BeginPaint(hwnd, &ps);

    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    int w = rcClient.right;
    int h = rcClient.bottom;

    HDC     hdc = CreateCompatibleDC(hdcScreen);
    HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, w, h);
    HBITMAP hOld = (HBITMAP)SelectObject(hdc, hBmp);

    FillRect(hdc, &rcClient, (HBRUSH)(COLOR_WINDOW + 1));

    DrawToolbar(hdc, m_rcToolbar);
    DrawSidebar(hdc, m_rcSidebar);
    DrawMonthHeader(hdc, m_rcHeader);
    DrawDayHeaders(hdc, m_rcDayHeaders);
    DrawCalendarGrid(hdc, m_rcGrid);
    DrawZmanimPanel(hdc, m_rcZmanim);

    BitBlt(hdcScreen, 0, 0, w, h, hdc, 0, 0, SRCCOPY);

    SelectObject(hdc, hOld);
    DeleteObject(hBmp);
    DeleteDC(hdc);

    EndPaint(hwnd, &ps);
}

// =============================================================================
// DRAW TOOLBAR
// =============================================================================

// Draws the toolbar with navigation and action buttons.
void MainWindow::DrawToolbar(HDC hdc, const RECT& rc)
{
    HBRUSH brTB = CreateSolidBrush(RGB(240, 240, 245));
    FillRect(hdc, &rc, brTB);
    DeleteObject(brTB);

    HPEN penBorder = CreatePen(PS_SOLID, 1, RGB(200, 200, 210));
    HPEN hOldPen = (HPEN)SelectObject(hdc, penBorder);
    MoveToEx(hdc, rc.left, rc.bottom - 1, nullptr);
    LineTo(hdc, rc.right, rc.bottom - 1);
    SelectObject(hdc, hOldPen);
    DeleteObject(penBorder);

    auto DrawBtn = [&](int x, int y, int bw, int bh, const wchar_t* label)
        {
            RECT rcBtn = { x, y, x + bw, y + bh };
            HBRUSH brBtn = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(hdc, &rcBtn, brBtn);
            DeleteObject(brBtn);
            FrameRect(hdc, &rcBtn, (HBRUSH)GetStockObject(GRAY_BRUSH));
            SelectObject(hdc, m_fontNormal);
            SetTextColor(hdc, RGB(30, 30, 80));
            SetBkMode(hdc, TRANSPARENT);
            DrawTextW(hdc, label, -1, &rcBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        };

    int btnY = rc.top + 6;
    int btnH = 30;
    int x = 8;

    DrawBtn(x, btnY, 30, btnH, L"<");        x += 36;   // prev month
    DrawBtn(x, btnY, 30, btnH, L">");        x += 40;   // next month
    DrawBtn(x, btnY, 60, btnH, L"Today");    x += 66;   // today
    DrawBtn(x, btnY, 70, btnH, L"Location"); x += 76;   // location picker
    DrawBtn(x, btnY, 70, btnH, L"Hebrew");   x += 76;   // Hebrew view (future)
    DrawBtn(x, btnY, 70, btnH, L"Options");             // options

    RECT rcTitle = { rc.right - 220, rc.top, rc.right - 10, rc.bottom };
    SelectObject(hdc, m_fontBold);
    SetTextColor(hdc, RGB(30, 30, 80));
    SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, L"WinLuach", -1, &rcTitle, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
}

// =============================================================================
// DRAW MONTH HEADER
// =============================================================================

// Draws the month/year header bar showing both Gregorian and Hebrew month/year.
void MainWindow::DrawMonthHeader(HDC hdc, const RECT& rc)
{
    HBRUSH brHdr = CreateSolidBrush(CLR_HEADER);
    FillRect(hdc, &rc, brHdr);
    DeleteObject(brHdr);

    GregorianDate firstDay(m_viewYear, m_viewMonth, 1);
    GregorianDate lastDate(m_viewYear, m_viewMonth,
        DaysInGregorianMonth(m_viewMonth, m_viewYear));

    HebrewDate hFirst = GregorianToHebrew(firstDay);
    HebrewDate hLast = GregorianToHebrew(lastDate);

    bool leap = IsHebrewLeapYear(hFirst.year);
    std::wstring hebStr = HebrewMonthName(hFirst.month, leap);
    if (hFirst.month != hLast.month)
        hebStr += L" - " + HebrewMonthName(hLast.month, IsHebrewLeapYear(hLast.year));

    std::wstring hebYearStr = std::to_wstring(hFirst.year);
    if (hFirst.year != hLast.year)
        hebYearStr += L" - " + std::to_wstring(hLast.year);

    std::wstring gregStr = GregorianMonthName(m_viewMonth) + L" " + std::to_wstring(m_viewYear);
    std::wstring hebFull = hebStr + L"  " + hebYearStr;

    SelectObject(hdc, m_fontHeader);
    SetTextColor(hdc, CLR_HEADER_TEXT);
    SetBkMode(hdc, TRANSPARENT);

    RECT rcLeft = { rc.left + 20, rc.top, rc.left + (rc.right - rc.left) / 2, rc.bottom };
    RECT rcCenter = { rc.left, rc.top, rc.right, rc.bottom };
    DrawTextW(hdc, gregStr.c_str(), -1, &rcLeft, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawTextW(hdc, hebFull.c_str(), -1, &rcCenter, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// =============================================================================
// DRAW DAY HEADERS
// =============================================================================

// Draws the Sun / Mon / Tue ... Shabbos column header row.
void MainWindow::DrawDayHeaders(HDC hdc, const RECT& rc)
{
    static const wchar_t* days[] = {
        L"Sunday", L"Monday", L"Tuesday", L"Wednesday",
        L"Thursday", L"Friday", L"Shabbos"
    };

    HBRUSH brHdr = CreateSolidBrush(RGB(70, 100, 160));
    FillRect(hdc, &rc, brHdr);
    DeleteObject(brHdr);

    SelectObject(hdc, m_fontBold);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);

    for (int col = 0; col < 7; col++)
    {
        RECT rcCol = { m_colX[col], rc.top, m_colX[col + 1], rc.bottom };
        DrawTextW(hdc, days[col], -1, &rcCol, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

// =============================================================================
// DRAW CALENDAR GRID
// =============================================================================

// Draws all 42 calendar cells plus grid lines.
void MainWindow::DrawCalendarGrid(HDC hdc, const RECT& rc)
{
    if (m_cells.empty()) return;

    int gridH = rc.bottom - rc.top;
    int cellH = gridH / 6;

    for (int i = 0; i < 42 && i < (int)m_cells.size(); i++)
    {
        int row = i / 7;
        int col = i % 7;
        RECT rcCell = {
            m_colX[col],     rc.top + row * cellH,
            m_colX[col + 1],   rc.top + (row + 1) * cellH
        };

        bool isSelected = (m_cells[i].greg.year == m_selectedDate.year &&
            m_cells[i].greg.month == m_selectedDate.month &&
            m_cells[i].greg.day == m_selectedDate.day);

        DrawCell(hdc, rcCell, m_cells[i].greg, m_cells[i].hebrew,
            isSelected, m_cells[i].isCurrentMonth);
    }

    // Grid lines
    HPEN hOldPen = (HPEN)SelectObject(hdc, m_penGrid);
    for (int row = 0; row <= 6; row++)
    {
        int y = rc.top + row * cellH;
        MoveToEx(hdc, rc.left, y, nullptr);
        LineTo(hdc, rc.right, y);
    }
    for (int col = 0; col <= 7; col++)
    {
        MoveToEx(hdc, m_colX[col], rc.top, nullptr);
        LineTo(hdc, m_colX[col], rc.bottom);
    }
    SelectObject(hdc, hOldPen);
}

// =============================================================================
// DRAW CELL
// =============================================================================

// Draws a single calendar cell with Gregorian date, Hebrew date, and holidays.
void MainWindow::DrawCell(HDC hdc, const RECT& rc,
    const GregorianDate& g, const HebrewDate& h,
    bool isSelected, bool isCurrentMonth)
{
    // Background
    COLORREF clrBg = GetCellColor(g, h, isSelected, isCurrentMonth);
    HBRUSH brCell = CreateSolidBrush(clrBg);
    FillRect(hdc, &rc, brCell);
    DeleteObject(brCell);

    // Selection border
    if (isSelected)
    {
        HPEN penSel = CreatePen(PS_SOLID, 2, RGB(0, 80, 180));
        HPEN hOldPen = (HPEN)SelectObject(hdc, penSel);
        HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rc.left + 1, rc.top + 1, rc.right - 1, rc.bottom - 1);
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBr);
        DeleteObject(penSel);
    }

    SetBkMode(hdc, TRANSPARENT);
    int margin = 3;

    // Gregorian day number (top-left, bold)
    SelectObject(hdc, m_fontBold);
    SetTextColor(hdc, isCurrentMonth ? CLR_GREG_TEXT : RGB(180, 180, 180));
    wchar_t dayNum[8];
    swprintf_s(dayNum, L"%d", g.day);
    RECT rcGreg = { rc.left + margin, rc.top + margin,
                    rc.left + margin + 28, rc.top + margin + 18 };
    DrawTextW(hdc, dayNum, -1, &rcGreg, DT_LEFT | DT_TOP);

    // Hebrew date (top-right, small blue)
    bool leap = IsHebrewLeapYear(h.year);
    std::wstring hebDay = std::to_wstring(h.day) + L" " + HebrewMonthName(h.month, leap);
    SelectObject(hdc, m_fontSmall);
    SetTextColor(hdc, isCurrentMonth ? CLR_HEBREW_TEXT : RGB(180, 180, 200));
    RECT rcHeb = { rc.left + margin, rc.top + margin,
                   rc.right - margin, rc.top + margin + 14 };
    DrawTextW(hdc, hebDay.c_str(), -1, &rcHeb, DT_RIGHT | DT_TOP);

    // Holiday names and omer (below dates)
    if (isCurrentMonth)
    {
        const std::vector<HolidayInfo>* pHols = nullptr;
        const OmerInfo* pOmer = nullptr;

        for (const auto& cd : m_cells)
        {
            if (cd.greg.year == g.year &&
                cd.greg.month == g.month &&
                cd.greg.day == g.day)
            {
                pHols = &cd.holidays;
                pOmer = &cd.omer;
                break;
            }
        }

        int yOff = rc.top + margin + 16;
        int maxLines = (rc.bottom - rc.top - 30) / 13;
        int lines = 0;

        if (pHols)
        {
            SelectObject(hdc, m_fontSmall);
            SetTextColor(hdc, CLR_HOLIDAY_TEXT);

            for (const auto& hol : *pHols)
            {
                if (lines >= maxLines) break;
                if (hol.flags & (HOLIDAY_SHABBOS_MEVAR | HOLIDAY_SPECIAL_SHAB))
                    continue;

                std::wstring holText = hol.name;
                if (!hol.subtitle.empty()) holText += L" - " + hol.subtitle;

                RECT rcHol = { rc.left + margin, yOff,
                               rc.right - margin, yOff + 13 };
                DrawTextW(hdc, holText.c_str(), -1, &rcHol,
                    DT_LEFT | DT_TOP | DT_END_ELLIPSIS | DT_SINGLELINE);
                yOff += 13;
                lines++;
            }
        }

        if (pOmer && pOmer->isOmerDay && lines < maxLines)
        {
            wchar_t omerBuf[32];
            swprintf_s(omerBuf, L"Day %d Omer", pOmer->day);
            RECT rcOmer = { rc.left + margin, yOff, rc.right - margin, yOff + 13 };
            SetTextColor(hdc, RGB(100, 80, 150));
            DrawTextW(hdc, omerBuf, -1, &rcOmer,
                DT_LEFT | DT_TOP | DT_SINGLELINE);
        }
    }
}

// =============================================================================
// GET CELL COLOR
// =============================================================================

// Returns the background color for a cell.
// Priority: other-month > today > Yom Tov > Rosh Chodesh > Chol Hamoed > Fast > Shabbos > normal
COLORREF MainWindow::GetCellColor(const GregorianDate& g, const HebrewDate& h,
    bool isSelected, bool isCurrentMonth) const
{
    if (!isCurrentMonth) return CLR_OTHER_MONTH;

    bool isToday = (g.year == m_today.year &&
        g.month == m_today.month &&
        g.day == m_today.day);
    if (isToday) return CLR_TODAY;

    std::vector<HolidayInfo> hols = GetHolidays(h, m_isIsrael);

    for (const auto& hol : hols) if (hol.flags & HOLIDAY_YOM_TOV)      return CLR_YOM_TOV;
    for (const auto& hol : hols) if (hol.flags & HOLIDAY_ROSH_CHODESH)  return CLR_ROSH_CHODESH;
    for (const auto& hol : hols) if (hol.flags & HOLIDAY_CHOL_HAMOED)   return CLR_CHOL_HAMOED;
    for (const auto& hol : hols) if (hol.flags & HOLIDAY_FAST)          return CLR_FAST;

    if (GetDayOfWeek(g) == SHABBAT) return CLR_SHABBOS;
    return CLR_NORMAL;
}

// =============================================================================
// DRAW SIDEBAR
// =============================================================================

// Draws the day details panel on the left.
void MainWindow::DrawSidebar(HDC hdc, const RECT& rc)
{
    FillRect(hdc, &rc, m_brSidebar);

    // Right border
    HPEN penBorder = CreatePen(PS_SOLID, 1, RGB(180, 190, 210));
    HPEN hOldPen = (HPEN)SelectObject(hdc, penBorder);
    MoveToEx(hdc, rc.right - 1, rc.top, nullptr);
    LineTo(hdc, rc.right - 1, rc.bottom);
    SelectObject(hdc, hOldPen);
    DeleteObject(penBorder);

    SetBkMode(hdc, TRANSPARENT);
    int x = rc.left + 8;
    int yOff = rc.top + 10;
    int w = rc.right - rc.left - 16;

    // "Day details" header bar
    HBRUSH brHdr = CreateSolidBrush(CLR_HEADER);
    RECT rcHdr = { rc.left, yOff - 2, rc.right - 1, yOff + 20 };
    FillRect(hdc, &rcHdr, brHdr);
    DeleteObject(brHdr);
    SelectObject(hdc, m_fontBold);
    SetTextColor(hdc, RGB(255, 255, 255));
    RECT rcHdrTxt = rcHdr; rcHdrTxt.left += 6;
    DrawTextW(hdc, L"Day details", -1, &rcHdrTxt,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    yOff += 26;

    // Gregorian date
    bool leap = IsHebrewLeapYear(m_selectedHebrew.year);
    std::wstring gregFull = DayOfWeekName(GetDayOfWeek(m_selectedDate)) + L"\r\n"
        + std::to_wstring(m_selectedDate.day) + L" "
        + GregorianMonthName(m_selectedDate.month) + L" "
        + std::to_wstring(m_selectedDate.year);
    SelectObject(hdc, m_fontBold);
    SetTextColor(hdc, CLR_GREG_TEXT);
    RECT rcGreg = { x, yOff, x + w, yOff + 36 };
    DrawTextW(hdc, gregFull.c_str(), -1, &rcGreg, DT_LEFT | DT_TOP);
    yOff += 40;

    // Hebrew date
    std::wstring hebFull = std::to_wstring(m_selectedHebrew.day) + L" "
        + HebrewMonthName(m_selectedHebrew.month, leap) + L" "
        + std::to_wstring(m_selectedHebrew.year);
    SelectObject(hdc, m_fontBold);
    SetTextColor(hdc, CLR_HEBREW_TEXT);
    RECT rcHeb = { x, yOff, x + w, yOff + 18 };
    DrawTextW(hdc, hebFull.c_str(), -1, &rcHeb, DT_LEFT | DT_TOP);
    yOff += 22;

    // Separator helper
    auto DrawSep = [&]() {
        HPEN p = CreatePen(PS_SOLID, 1, RGB(180, 190, 210));
        SelectObject(hdc, p);
        MoveToEx(hdc, x, yOff, nullptr);
        LineTo(hdc, x + w, yOff);
        DeleteObject(p);
        yOff += 8;
        };
    DrawSep();

    // Holidays
    std::vector<HolidayInfo> hols = GetHolidays(m_selectedHebrew, m_isIsrael);
    if (!hols.empty())
    {
        SelectObject(hdc, m_fontNormal);
        SetTextColor(hdc, CLR_HOLIDAY_TEXT);
        for (const auto& hol : hols)
        {
            if (yOff > rc.bottom - 20) break;
            std::wstring txt = hol.name;
            if (!hol.subtitle.empty()) txt += L" - " + hol.subtitle;
            RECT rcH = { x, yOff, x + w, yOff + 16 };
            DrawTextW(hdc, txt.c_str(), -1, &rcH,
                DT_LEFT | DT_TOP | DT_END_ELLIPSIS | DT_SINGLELINE);
            yOff += 17;
        }
        yOff += 4;
    }

    // Omer
    OmerInfo omer = GetOmer(m_selectedHebrew);
    if (omer.isOmerDay)
    {
        SelectObject(hdc, m_fontNormal);
        SetTextColor(hdc, RGB(100, 60, 140));
        RECT rcOmer = { x, yOff, x + w, yOff + 28 };
        DrawTextW(hdc, omer.text.c_str(), -1, &rcOmer,
            DT_LEFT | DT_TOP | DT_WORDBREAK);
        yOff += 30;
    }

    // Parasha
    ParashaInfo parasha = GetParasha(m_selectedHebrew, m_isIsrael);
    if (!parasha.name.empty())
    {
        std::wstring parTxt = L"Parasha: " + parasha.name;
        if (parasha.isCombined && !parasha.name2.empty())
            parTxt += L" - " + parasha.name2;
        SelectObject(hdc, m_fontNormal);
        SetTextColor(hdc, RGB(40, 80, 40));
        RECT rcPar = { x, yOff, x + w, yOff + 32 };
        DrawTextW(hdc, parTxt.c_str(), -1, &rcPar,
            DT_LEFT | DT_TOP | DT_WORDBREAK);
        yOff += 36;
    }

    DrawSep();

    // Daily learning
    DailyLearning dl = GetDailyLearning(m_selectedHebrew, m_selectedDate);
    struct { const wchar_t* label; const std::wstring& value; } learnings[] = {
        { L"Daf Yomi:",   dl.dafYomi      },
        { L"Yerushalmi:", dl.yerushalmi   },
        { L"Mishna:",     dl.mishnaYomit  },
        { L"Halacha:",    dl.halachaYomit },
        { L"Tanach:",     dl.tanachYomi   }
    };

    for (const auto& lrn : learnings)
    {
        if (yOff > rc.bottom - 20 || lrn.value.empty()) break;
        SelectObject(hdc, m_fontSmall);
        SetTextColor(hdc, RGB(80, 80, 80));
        RECT rcLabel = { x,    yOff, x + 65, yOff + 14 };
        DrawTextW(hdc, lrn.label, -1, &rcLabel,
            DT_LEFT | DT_TOP | DT_SINGLELINE);
        SetTextColor(hdc, RGB(20, 20, 100));
        RECT rcVal = { x + 65, yOff, x + w, yOff + 14 };
        DrawTextW(hdc, lrn.value.c_str(), -1, &rcVal,
            DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        yOff += 16;
    }

    // Year details header bar
    yOff += 6;
    if (yOff + 20 < rc.bottom)
    {
        HBRUSH brYHdr = CreateSolidBrush(CLR_HEADER);
        RECT rcYHdr = { rc.left, yOff, rc.right - 1, yOff + 20 };
        FillRect(hdc, &rcYHdr, brYHdr);
        DeleteObject(brYHdr);
        SelectObject(hdc, m_fontBold);
        SetTextColor(hdc, RGB(255, 255, 255));
        RECT rcYTxt = rcYHdr; rcYTxt.left += 6;
        DrawTextW(hdc, L"Year details", -1, &rcYTxt,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        yOff += 24;
    }

    // Year facts
    auto facts = GetYearFacts(m_selectedHebrew.year, m_settings.yearDetailsMask);
    SelectObject(hdc, m_fontSmall);
    SetTextColor(hdc, RGB(60, 60, 60));
    for (const auto& fact : facts)
    {
        if (yOff > rc.bottom - 14) break;
        RECT rcFact = { x, yOff, x + w, yOff + 13 };
        DrawTextW(hdc, fact.c_str(), -1, &rcFact,
            DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        yOff += 14;
    }
}

// =============================================================================
// DRAW ZMANIM PANEL
// =============================================================================

// Draws the zmanim panel at the bottom of the window.
void MainWindow::DrawZmanimPanel(HDC hdc, const RECT& rc)
{
    FillRect(hdc, &rc, m_brZmanim);

    HPEN penTop = CreatePen(PS_SOLID, 1, RGB(160, 160, 140));
    HPEN hOldPen = (HPEN)SelectObject(hdc, penTop);
    MoveToEx(hdc, rc.left, rc.top, nullptr);
    LineTo(hdc, rc.right, rc.top);
    SelectObject(hdc, hOldPen);
    DeleteObject(penTop);

    SetBkMode(hdc, TRANSPARENT);

    wchar_t locBuf[256];
    swprintf_s(locBuf, L"Zmanim  |  %s  |  %s",
        m_location.name.c_str(),
        DayOfWeekName(GetDayOfWeek(m_selectedDate)).c_str());

    SelectObject(hdc, m_fontBold);
    SetTextColor(hdc, RGB(60, 60, 20));
    RECT rcHdr = { rc.left + 8, rc.top + 3, rc.right, rc.top + 20 };
    DrawTextW(hdc, locBuf, -1, &rcHdr, DT_LEFT | DT_TOP | DT_SINGLELINE);

    struct ZmanEntry { const wchar_t* label; TimeOfDay time; };
    ZmanEntry entries[] = {
        { L"Alos Hashachar",  m_zmanim.alot_GRA         },
        { L"Misheyakir",      m_zmanim.misheyakir_10    },
        { L"Hanetz",          m_zmanim.hanetz            },
        { L"Sof Shema (GRA)", m_zmanim.sofShema_GRA     },
        { L"Sof Tefilla",     m_zmanim.sofTefilla_GRA   },
        { L"Chatzos",         m_zmanim.chatzot           },
        { L"Mincha Gedola",   m_zmanim.minchaGedola_GRA },
        { L"Mincha Ketana",   m_zmanim.minchaKetana_GRA },
        { L"Plag HaMincha",   m_zmanim.plagMincha_GRA   },
        { L"Shkiah",          m_zmanim.shkia             },
        { L"Tzeis (GRA)",     m_zmanim.tzeit_GRA         },
        { L"Candle Lighting", m_zmanim.candleLighting    },
    };

    int numEntries = sizeof(entries) / sizeof(entries[0]);
    int cols = 4;
    int rows = (numEntries + cols - 1) / cols;
    int padding = 16;
    int colGap = 20;
    int colW = (rc.right - rc.left - padding * 2 - colGap * (cols - 1)) / cols;
    int startY = rc.top + 22;
    int rowH = 19;
    int labelW = 110;
    int timeW = 60;

    SelectObject(hdc, m_fontSmall);

    for (int i = 0; i < numEntries; i++)
    {
        int col = i / rows;
        int row = i % rows;
        int x = rc.left + padding + col * (colW + colGap);
        int y = startY + row * rowH;

        if (col > 0 && row == 0)
        {
            HPEN penDiv = CreatePen(PS_SOLID, 1, RGB(200, 195, 170));
            HPEN hOldP = (HPEN)SelectObject(hdc, penDiv);
            int divX = x - colGap / 2;
            MoveToEx(hdc, divX, rc.top + 4, nullptr);
            LineTo(hdc, divX, rc.bottom - 4);
            SelectObject(hdc, hOldP);
            DeleteObject(penDiv);
        }

        SetTextColor(hdc, RGB(70, 70, 50));
        RECT rcLabel = { x, y, x + labelW, y + rowH };
        DrawTextW(hdc, entries[i].label, -1, &rcLabel,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        SelectObject(hdc, m_fontNormal);
        SetTextColor(hdc, RGB(20, 20, 140));
        std::wstring timeStr = FormatTime(entries[i].time, m_use24hr);
        RECT rcTime = { x + labelW, y, x + labelW + timeW, y + rowH };
        DrawTextW(hdc, timeStr.c_str(), -1, &rcTime,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, m_fontSmall);
    }
}

// =============================================================================
// NAVIGATION
// =============================================================================

// Moves back one month.
void MainWindow::PrevMonth()
{
    m_viewMonth--;
    if (m_viewMonth < 1) { m_viewMonth = 12; m_viewYear--; }
    RefreshMonthData();
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

// Moves forward one month.
void MainWindow::NextMonth()
{
    m_viewMonth++;
    if (m_viewMonth > 12) { m_viewMonth = 1; m_viewYear++; }
    RefreshMonthData();
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

// Jumps to today's date.
void MainWindow::GoToToday()
{
    m_today = GetTodayGregorian();
    m_todayHebrew = GetTodayHebrew();
    m_viewYear = m_today.year;
    m_viewMonth = m_today.month;
    SelectDate(m_today);
}

// Sets the selected date and refreshes the display.
void MainWindow::SelectDate(const GregorianDate& g)
{
    m_selectedDate = g;
    m_selectedHebrew = GregorianToHebrew(g);

    if (m_viewYear != g.year || m_viewMonth != g.month)
    {
        m_viewYear = g.year;
        m_viewMonth = g.month;
        RefreshMonthData();
    }

    m_isDST = IsDST(g, m_location);
    RefreshZmanim();
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

// Opens the location picker and updates zmanim if a new city is chosen.
void MainWindow::OnLocationClick()
{
    Location newLoc;
    if (LocationDialog::Show(m_hwnd, m_location, newLoc))
    {
        m_location = newLoc;
        m_settings.locationName = newLoc.name;
        m_settings.latitude = newLoc.latitude;
        m_settings.longitude = newLoc.longitude;
        m_settings.elevation = newLoc.elevation;
        m_settings.gmtOffset = newLoc.gmtOffset;
        m_settings.usesDST = newLoc.usesDST;
        SaveSettings(m_settings);
        m_isDST = IsDST(m_selectedDate, m_location);
        RefreshZmanim();
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
}

// Opens the options dialog and applies any changed settings.
void MainWindow::OnOptionsClick()
{
    AppSettings updated;
    if (OptionsDialog::Show(m_hwnd, m_settings, updated))
    {
        m_settings = updated;
        m_use24hr = updated.use24Hour;
        m_isIsrael = updated.isIsrael;
        SaveSettings(m_settings);
        RefreshMonthData(); // rebuild cells with new Israel/Diaspora setting
        RefreshZmanim();
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
}

// =============================================================================
// MOUSE HANDLING
// =============================================================================

// Handles left-click on toolbar buttons and calendar cells.
void MainWindow::OnLButtonDown(HWND hwnd, int x, int y)
{
    if (y >= m_rcToolbar.top && y <= m_rcToolbar.bottom)
    {
        if (x >= 8 && x <= 38) { PrevMonth();       return; }  // <
        if (x >= 44 && x <= 74) { NextMonth();       return; }  // >
        if (x >= 84 && x <= 144) { GoToToday();       return; }  // Today
        if (x >= 150 && x <= 220) { OnLocationClick(); return; }  // Location
        if (x >= 302 && x <= 372) { OnOptionsClick();  return; }  // Options
        return;
    }

    int idx = HitTestCell(x, y);
    if (idx >= 0 && idx < (int)m_cells.size())
        SelectDate(m_cells[idx].greg);
}

// Handles double-click (reserved for future event editing).
void MainWindow::OnLButtonDblClk(HWND hwnd, int x, int y)
{
    int idx = HitTestCell(x, y);
    if (idx >= 0)
        SelectDate(m_cells[idx].greg);
}

// =============================================================================
// KEYBOARD HANDLING
// =============================================================================

// Arrow keys, Page Up/Down, Home.
void MainWindow::OnKeyDown(HWND hwnd, WPARAM key)
{
    switch (key)
    {
    case VK_LEFT:  SelectDate(JDNToGregorian(HebrewToJDN(m_selectedHebrew) - 1)); break;
    case VK_RIGHT: SelectDate(JDNToGregorian(HebrewToJDN(m_selectedHebrew) + 1)); break;
    case VK_UP:    SelectDate(JDNToGregorian(HebrewToJDN(m_selectedHebrew) - 7)); break;
    case VK_DOWN:  SelectDate(JDNToGregorian(HebrewToJDN(m_selectedHebrew) + 7)); break;
    case VK_PRIOR: PrevMonth(); break;
    case VK_NEXT:  NextMonth(); break;
    case VK_HOME:  GoToToday(); break;
    }
}

// =============================================================================
// COMMAND HANDLING
// =============================================================================

void MainWindow::OnCommand(HWND hwnd, WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
    case ID_BTN_PREV:     PrevMonth();       break;
    case ID_BTN_NEXT:     NextMonth();       break;
    case ID_BTN_TODAY:    GoToToday();       break;
    case ID_BTN_LOCATION: OnLocationClick(); break;
    case ID_BTN_OPTIONS:  OnOptionsClick();  break;
    }
}

// =============================================================================
// HIT TEST
// =============================================================================

// Returns the cell index (0-41) for the given screen coordinates, or -1.
int MainWindow::HitTestCell(int x, int y) const
{
    if (x < m_rcGrid.left || x >= m_rcGrid.right ||
        y < m_rcGrid.top || y >= m_rcGrid.bottom)
        return -1;

    int gridH = m_rcGrid.bottom - m_rcGrid.top;
    int cellH = gridH / 6;
    int row = (y - m_rcGrid.top) / cellH;

    int col = -1;
    for (int c = 0; c < 7; c++)
        if (x >= m_colX[c] && x < m_colX[c + 1]) { col = c; break; }

    if (row < 0 || row >= 6 || col < 0) return -1;
    return row * 7 + col;
}

// =============================================================================
// REFRESH MONTH DATA
// =============================================================================

// Populates m_cells with all 42 cells for the current view month.
void MainWindow::RefreshMonthData()
{
    m_cells.clear();
    m_cells.resize(42);

    GregorianDate firstOfMonth(m_viewYear, m_viewMonth, 1);
    DayOfWeek     firstDow = GetDayOfWeek(firstOfMonth);
    long          firstCellJDN = GregorianToJDN(firstOfMonth) - (int)firstDow;

    for (int i = 0; i < 42; i++)
    {
        long jdn = firstCellJDN + i;
        m_cells[i].greg = JDNToGregorian(jdn);
        m_cells[i].hebrew = JDNToHebrew(jdn);
        m_cells[i].isCurrentMonth = (m_cells[i].greg.month == m_viewMonth &&
            m_cells[i].greg.year == m_viewYear);
        m_cells[i].holidays = GetHolidays(m_cells[i].hebrew, m_isIsrael);
        m_cells[i].omer = GetOmer(m_cells[i].hebrew);
    }
}

// =============================================================================
// REFRESH ZMANIM
// =============================================================================

// Recalculates zmanim for the selected date at the current location.
void MainWindow::RefreshZmanim()
{
    m_isDST = IsDST(m_selectedDate, m_location);
    m_zmanim = CalculateZmanim(m_selectedDate, m_location, m_isDST);
}
