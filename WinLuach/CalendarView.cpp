// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    CalendarView.cpp
// Purpose: Owner-draw calendar grid implementation.
//          Draws 42 cells covering the current month, handles
//          mouse clicks and keyboard navigation.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial implementation. 42-cell grid, color coding,
//          Hebrew dates, holiday names, omer count.
//          Mouse click selects a date. Arrow keys navigate days.
//          Page Up/Down navigate months. Home jumps to today.
// =============================================================================

#include "pch.h"
#include "CalendarView.h"

BEGIN_MESSAGE_MAP(CCalendarView, CWnd)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_KEYDOWN()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
END_MESSAGE_MAP()

// =============================================================================
// CONSTRUCTION
// =============================================================================

CCalendarView::CCalendarView(CMainFrame* pFrame)
    : m_pFrame(pFrame)
{
}

CCalendarView::~CCalendarView()
{
}

// =============================================================================
// REBUILD CELLS
// =============================================================================

// Populates m_cells with all 42 days for the current view month.
// Called whenever the month or Israel/Diaspora setting changes.
void CCalendarView::RebuildCells()
{
    if (!m_pFrame) return;

    m_cells.clear();
    m_cells.resize(42);

    GregorianDate firstOfMonth;
    if (m_pFrame->m_hebrewMonthView)
    {
        firstOfMonth = HebrewToGregorian(HebrewDate(
            m_pFrame->m_viewHebrewYear,
            m_pFrame->m_viewHebrewMonth,
            1));
    }
    else
    {
        firstOfMonth = GregorianDate(m_pFrame->m_viewYear,
            m_pFrame->m_viewMonth, 1);
    }

    DayOfWeek firstDow = GetDayOfWeek(firstOfMonth);
    long      firstCellJDN = GregorianToJDN(firstOfMonth) - (int)firstDow;

    for (int i = 0; i < 42; i++)
    {
        long jdn = firstCellJDN + i;
        m_cells[i].greg = JDNToGregorian(jdn);
        m_cells[i].hebrew = JDNToHebrew(jdn);
        if (m_pFrame->m_hebrewMonthView)
        {
            m_cells[i].isCurrentMonth =
                (m_cells[i].hebrew.month == m_pFrame->m_viewHebrewMonth &&
                    m_cells[i].hebrew.year == m_pFrame->m_viewHebrewYear);
        }
        else
        {
            m_cells[i].isCurrentMonth =
                (m_cells[i].greg.month == m_pFrame->m_viewMonth &&
                    m_cells[i].greg.year == m_pFrame->m_viewYear);
        }
        m_cells[i].holidays = GetHolidays(m_cells[i].hebrew,
            m_pFrame->m_isIsrael);
        m_cells[i].omer = GetOmer(m_cells[i].hebrew);
    }
}

// =============================================================================
// ON SIZE
// =============================================================================

// Recalculates column positions and row height when the window is resized.
void CCalendarView::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);

    if (cx <= 0 || cy <= 0) return;

    // Skip the header rows — they are drawn by MainFrame
    int gridTop = 0;
    int gridH = cy - gridTop;
    m_cellH = (gridH > 0) ? gridH / 6 : 80;

    int baseColW = cx / 7;
    int extra = cx - baseColW * 7;
    m_colX[0] = 0;
    for (int i = 1; i <= 7; i++)
        m_colX[i] = m_colX[i - 1] + baseColW + (i == 7 ? extra : 0);

    RebuildCells();
    Invalidate();
}

// =============================================================================
// ON PAINT
// =============================================================================

// Draws all 42 cells and the grid lines using double-buffering.
void CCalendarView::OnPaint()
{
    CPaintDC dcScreen(this);

    CRect rcClient;
    GetClientRect(&rcClient);
    int cx = rcClient.Width();
    int cy = rcClient.Height();

    // Double buffer
    CDC     memDC;
    CBitmap bmp;
    memDC.CreateCompatibleDC(&dcScreen);
    bmp.CreateCompatibleBitmap(&dcScreen, cx, cy);
    CBitmap* pOldBmp = memDC.SelectObject(&bmp);

    memDC.FillSolidRect(rcClient, CLR_NORMAL);

    if (m_cells.empty()) RebuildCells();

    int gridTop = 0;

    // Draw each cell
    for (int i = 0; i < 42 && i < (int)m_cells.size(); i++)
    {
        bool isSelected =
            (m_cells[i].greg.year == m_pFrame->m_selectedDate.year &&
                m_cells[i].greg.month == m_pFrame->m_selectedDate.month &&
                m_cells[i].greg.day == m_pFrame->m_selectedDate.day);

        CRect rcCell = GetCellRect(i);
        DrawCell(&memDC, rcCell, m_cells[i], isSelected);
    }

    // Grid lines
    CPen penGrid(PS_SOLID, 1, CLR_GRID);
    CPen* pOldPen = memDC.SelectObject(&penGrid);

    int rows = 6;
    for (int row = 0; row <= rows; row++)
    {
        int y = gridTop + row * m_cellH;
        memDC.MoveTo(0, y);
        memDC.LineTo(cx, y);
    }
    for (int col = 0; col <= 7; col++)
    {
        memDC.MoveTo(m_colX[col], gridTop);
        memDC.LineTo(m_colX[col], gridTop + rows * m_cellH);
    }
    memDC.SelectObject(pOldPen);

    // Blit to screen
    dcScreen.BitBlt(0, 0, cx, cy, &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}

// Suppress default background erase to avoid flicker.
BOOL CCalendarView::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

// =============================================================================
// DRAW CELL
// =============================================================================

// Draws a single calendar cell.
void CCalendarView::DrawCell(CDC* pDC, const CRect& rc,
    const CalCellData& cell, bool isSelected)
{
    // Background
    COLORREF clrBg = GetCellColor(cell, isSelected);
    pDC->FillSolidRect(rc, clrBg);

    // Selection border
    if (isSelected)
    {
        CPen penSel(PS_SOLID, 2, RGB(0, 80, 180));
        CPen* pOld = pDC->SelectObject(&penSel);
        CBrush* pOldBr = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
        pDC->Rectangle(rc.left + 1, rc.top + 1, rc.right - 1, rc.bottom - 1);
        pDC->SelectObject(pOld);
        pDC->SelectObject(pOldBr);
    }

    pDC->SetBkMode(TRANSPARENT);
    int margin = 3;

    // Gregorian day number (top-left, bold)
    COLORREF clrGreg = cell.isCurrentMonth ? CLR_GREG_TXT : RGB(180, 180, 180);
    pDC->SelectObject(&m_pFrame->m_fontBold);
    pDC->SetTextColor(clrGreg);

    CString dayNum;
    dayNum.Format(L"%d", cell.greg.day);
    CRect rcGreg(rc.left + margin, rc.top + margin,
        rc.left + margin + 28, rc.top + margin + 18);
    pDC->DrawText(dayNum, rcGreg, DT_LEFT | DT_TOP);

    // Hebrew date (top-right, small blue)
    bool leap = IsHebrewLeapYear(cell.hebrew.year);
    std::wstring hebDay = std::to_wstring(cell.hebrew.day)
        + L" " + HebrewMonthName(cell.hebrew.month, leap);
    pDC->SelectObject(&m_pFrame->m_fontSmall);
    pDC->SetTextColor(cell.isCurrentMonth ? CLR_HEBREW_TXT : RGB(180, 180, 200));
    CRect rcHeb(rc.left + margin, rc.top + margin,
        rc.right - margin, rc.top + margin + 14);
    pDC->DrawText(hebDay.c_str(), -1, rcHeb, DT_RIGHT | DT_TOP);

    // Holiday names and omer (below dates)
    if (cell.isCurrentMonth)
    {
        int yOff = rc.top + margin + 16;
        int maxLines = (rc.Height() - 30) / 13;
        int lines = 0;

        pDC->SelectObject(&m_pFrame->m_fontSmall);
        pDC->SetTextColor(CLR_HOLIDAY_TXT);

        if (m_pFrame->m_showMoadim)
        {
            for (const auto& hol : cell.holidays)
            {
                if (lines >= maxLines) break;
                if (hol.flags & (HOLIDAY_SHABBOS_MEVAR | HOLIDAY_SPECIAL_SHAB))
                    continue;

                std::wstring txt = hol.name;
                if (!hol.subtitle.empty()) txt += L" - " + hol.subtitle;

                CRect rcHol(rc.left + margin, yOff,
                    rc.right - margin, yOff + 13);
                pDC->DrawText(txt.c_str(), -1, rcHol,
                    DT_LEFT | DT_TOP | DT_END_ELLIPSIS | DT_SINGLELINE);
                yOff += 13;
                lines++;
            }
        }

        if (m_pFrame->m_showParshios &&
            GetDayOfWeek(cell.greg) == SHABBAT &&
            lines < maxLines)
        {
            ParashaInfo par = GetParasha(cell.hebrew, m_pFrame->m_isIsrael);
            if (!par.name.empty())
            {
                std::wstring parTxt = par.name;
                if (par.isCombined && !par.name2.empty())
                    parTxt += L" - " + par.name2;

                CRect rcPar(rc.left + margin, yOff,
                    rc.right - margin, yOff + 13);
                pDC->SetTextColor(RGB(30, 80, 160));
                pDC->DrawText(parTxt.c_str(), -1, rcPar,
                    DT_LEFT | DT_TOP | DT_END_ELLIPSIS | DT_SINGLELINE);
                yOff += 13;
                lines++;
            }
        }

        auto userEvents = m_pFrame->GetUserEventsForDate(cell.greg);
        for (const auto& eventTitle : userEvents)
        {
            if (lines >= maxLines) break;
            CRect rcEvent(rc.left + margin, yOff,
                rc.right - margin, yOff + 13);
            pDC->SetTextColor(RGB(120, 30, 120));
            pDC->DrawText(eventTitle.c_str(), -1, rcEvent,
                DT_LEFT | DT_TOP | DT_END_ELLIPSIS | DT_SINGLELINE);
            yOff += 13;
            lines++;
        }

        if (cell.omer.isOmerDay && lines < maxLines)
        {
            CString omerTxt;
            omerTxt.Format(L"Day %d Omer", cell.omer.day);
            CRect rcOmer(rc.left + margin, yOff,
                rc.right - margin, yOff + 13);
            pDC->SetTextColor(RGB(100, 80, 150));
            pDC->DrawText(omerTxt, rcOmer,
                DT_LEFT | DT_TOP | DT_SINGLELINE);
        }
    }
}

// =============================================================================
// GET CELL COLOR
// =============================================================================

// Returns the background color for a cell based on its date and holidays.
COLORREF CCalendarView::GetCellColor(const CalCellData& cell,
    bool isSelected) const
{
    if (!cell.isCurrentMonth) return CLR_OTHER_MONTH;

    const GregorianDate& g = cell.greg;
    bool isToday = (g.year == m_pFrame->m_today.year &&
        g.month == m_pFrame->m_today.month &&
        g.day == m_pFrame->m_today.day);
    if (isToday) return CLR_TODAY;

    for (const auto& h : cell.holidays)
        if (h.flags & HOLIDAY_YOM_TOV)     return CLR_YOM_TOV;
    for (const auto& h : cell.holidays)
        if (h.flags & HOLIDAY_ROSH_CHODESH) return CLR_ROSH_CHODESH;
    for (const auto& h : cell.holidays)
        if (h.flags & HOLIDAY_CHOL_HAMOED)  return CLR_CHOL_HAMOED;
    for (const auto& h : cell.holidays)
        if (h.flags & HOLIDAY_FAST)         return CLR_FAST_DAY;

    if (GetDayOfWeek(g) == SHABBAT) return CLR_SHABBOS;
    return CLR_NORMAL;
}

// =============================================================================
// HIT TEST
// =============================================================================

// Returns the cell index (0-41) for the given point, or -1.
int CCalendarView::HitTest(CPoint pt) const
{
    int gridTop = 0;
    if (pt.y < gridTop) return -1;

    int row = (pt.y - gridTop) / m_cellH;
    if (row < 0 || row >= 6) return -1;

    int col = -1;
    for (int c = 0; c < 7; c++)
        if (pt.x >= m_colX[c] && pt.x < m_colX[c + 1]) { col = c; break; }

    if (col < 0) return -1;
    return row * 7 + col;
}

// Returns the CRect for cell index (0-41).
CRect CCalendarView::GetCellRect(int idx) const
{
    int row = idx / 7;
    int col = idx % 7;
    int gridTop = 0;

    return CRect(m_colX[col], gridTop + row * m_cellH,
        m_colX[col + 1], gridTop + (row + 1) * m_cellH);
}

// =============================================================================
// MOUSE HANDLING
// =============================================================================

// Selects the clicked cell and notifies the main frame.
void CCalendarView::OnLButtonDown(UINT nFlags, CPoint pt)
{
    int idx = HitTest(pt);
    if (idx >= 0 && idx < (int)m_cells.size())
    {
        SetFocus();
        m_pFrame->SelectDate(m_cells[idx].greg);
    }
    CWnd::OnLButtonDown(nFlags, pt);
}

// Double-click — same as single click for now (future: open event editor)
void CCalendarView::OnLButtonDblClk(UINT nFlags, CPoint pt)
{
    OnLButtonDown(nFlags, pt);
}

// =============================================================================
// KEYBOARD HANDLING
// =============================================================================

// Arrow keys navigate days, Page Up/Down navigate months, Home = today.
void CCalendarView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (!m_pFrame) return;

    long jdn = HebrewToJDN(m_pFrame->m_selectedHebrew);

    switch (nChar)
    {
    case VK_LEFT:  m_pFrame->SelectDate(JDNToGregorian(jdn - 1)); break;
    case VK_RIGHT: m_pFrame->SelectDate(JDNToGregorian(jdn + 1)); break;
    case VK_UP:    m_pFrame->SelectDate(JDNToGregorian(jdn - 7)); break;
    case VK_DOWN:  m_pFrame->SelectDate(JDNToGregorian(jdn + 7)); break;
    case VK_PRIOR: m_pFrame->PrevMonth(); break;
    case VK_NEXT:  m_pFrame->NextMonth(); break;
    case VK_HOME:  m_pFrame->GoToToday(); break;
    default:
        CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
        break;
    }
}
