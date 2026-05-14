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
#include "CalPrintDlg.h"

static long CalendarDateKey(const GregorianDate& g)
{
    return GregorianToJDN(g);
}

BEGIN_MESSAGE_MAP(CCalendarView, CWnd)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_RBUTTONDOWN()
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
        m_cells[i].learning = GetDailyLearning(m_cells[i].hebrew, m_cells[i].greg);
        auto [cs, ms] = m_pFrame->GetCellZmanimLabels(
            m_cells[i].greg, m_cells[i].hebrew, m_cells[i].holidays);
        m_cells[i].candleStr = std::move(cs);
        m_cells[i].motzStr   = std::move(ms);
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

    memDC.FillSolidRect(rcClient, m_pFrame ? m_pFrame->m_colorNormalCell : CLR_NORMAL);

    if (m_cells.empty()) RebuildCells();

    int gridTop = 0;

    // Draw each cell
    for (int i = 0; i < 42 && i < (int)m_cells.size(); i++)
    {
        bool isSelected = IsSelectedDate(m_cells[i].greg);

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
    auto fontHeight = [&](CFont& font) {
        CFont* oldFont = pDC->SelectObject(&font);
        TEXTMETRIC tm = {};
        pDC->GetTextMetrics(&tm);
        pDC->SelectObject(oldFont);
        return max(10, tm.tmHeight + tm.tmExternalLeading + 2);
    };
    int boldH = fontHeight(m_pFrame->m_fontBold);
    int smallH = fontHeight(m_pFrame->m_fontSmall);
    int headerH = max(boldH, smallH);
    int lineH = max(12, smallH + 1);

    // Gregorian day number (top-left, bold)
    COLORREF clrGreg = cell.isCurrentMonth ? m_pFrame->m_colorGregorianText : RGB(180, 180, 180);
    pDC->SelectObject(&m_pFrame->m_fontBold);
    pDC->SetTextColor(clrGreg);

    CString dayNum;
    dayNum.Format(L"%d", cell.greg.day);
    CRect rcGreg(rc.left + margin, rc.top + margin,
        rc.left + margin + 34, rc.top + margin + boldH);
    pDC->DrawText(dayNum, rcGreg, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Hebrew date (top-right, small blue)
    bool leap = IsHebrewLeapYear(cell.hebrew.year);
    std::wstring hebDay = std::to_wstring(cell.hebrew.day)
        + L" " + HebrewMonthName(cell.hebrew.month, leap);
    pDC->SelectObject(&m_pFrame->m_fontSmall);
    pDC->SetTextColor(cell.isCurrentMonth ? m_pFrame->m_colorHebrewText : RGB(180, 180, 200));
    CRect rcHeb(rc.left + margin, rc.top + margin,
        rc.right - margin, rc.top + margin + smallH);
    pDC->DrawText(hebDay.c_str(), -1, rcHeb,
        DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Holiday names and omer (below dates)
    if (cell.isCurrentMonth)
    {
        int yOff = rc.top + margin + headerH + 2;
        int maxLines = max(0, (rc.bottom - margin - yOff) / lineH);
        int lines = 0;

        pDC->SelectObject(&m_pFrame->m_fontSmall);
        pDC->SetTextColor(m_pFrame->m_colorHolidayText);

        if (m_pFrame->m_showMoadim)
        {
            for (const auto& hol : cell.holidays)
            {
                if (lines >= maxLines) break;
                if (hol.flags & HOLIDAY_SHABBOS_MEVAR)
                    continue;

                std::wstring txt = hol.name;
                if (!hol.subtitle.empty()) txt += L" - " + hol.subtitle;

                CRect rcHol(rc.left + margin, yOff,
                    rc.right - margin, yOff + lineH);
                pDC->DrawText(txt.c_str(), -1, rcHol,
                    DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE);
                yOff += lineH;
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
                    rc.right - margin, yOff + lineH);
                pDC->SetTextColor(m_pFrame->m_colorParshaText);
                pDC->DrawText(parTxt.c_str(), -1, rcPar,
                    DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE);
                yOff += lineH;
                lines++;
            }
        }

        auto userEvents = m_pFrame->GetCalendarEventLinesForDate(cell.greg);
        for (const auto& eventLine : userEvents)
        {
            if (lines >= maxLines) break;
            CRect rcEvent(rc.left + margin, yOff,
                rc.right - margin, yOff + lineH);
            COLORREF evColor = eventLine.isHebrew
                ? m_pFrame->m_colorHebrewEventText
                : m_pFrame->m_colorCivilEventText;
            pDC->SetTextColor(evColor);
            pDC->DrawText(eventLine.title.c_str(), -1, rcEvent,
                DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE);
            yOff += lineH;
            lines++;
        }

        if (cell.omer.isOmerDay && lines < maxLines)
        {
            CString omerTxt;
            omerTxt.Format(L"Day %d Omer", cell.omer.day);
            CRect rcOmer(rc.left + margin, yOff,
                rc.right - margin, yOff + lineH);
            pDC->SetTextColor(m_pFrame->m_colorOmerText);
            pDC->DrawText(omerTxt, rcOmer,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            yOff += lineH; lines++;
        }

        {
            pDC->SelectObject(&m_pFrame->m_fontSmall);
            auto tryLearn = [&](bool show, const std::wstring& txt, COLORREF clr) {
                if (!show || txt.empty() || lines >= maxLines) return;
                pDC->SetTextColor(clr);
                CRect rcL(rc.left + margin, yOff, rc.right - margin, yOff + lineH);
                pDC->DrawText(txt.c_str(), -1, rcL,
                    DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE);
                yOff += lineH; lines++;
            };
            tryLearn(m_pFrame->m_showDafYomi,      cell.learning.dafYomi,      m_pFrame->m_colorLearningText);
            tryLearn(m_pFrame->m_showYerushalmi,   cell.learning.yerushalmi,   m_pFrame->m_colorLearningText);
            tryLearn(m_pFrame->m_showHalachaYomit, cell.learning.halachaYomit, m_pFrame->m_colorLearningText);
            tryLearn(m_pFrame->m_showMishnaYomit,  cell.learning.mishnaYomit,  m_pFrame->m_colorLearningText);
            tryLearn(m_pFrame->m_showTanachYomi,   cell.learning.tanachYomi,   m_pFrame->m_colorLearningText);
        }

        if (!cell.candleStr.empty() && lines < maxLines)
        {
            CRect rcZ(rc.left + margin, yOff, rc.right - margin, yOff + lineH);
            pDC->SetTextColor(m_pFrame->m_colorCandleText);
            pDC->DrawText(cell.candleStr.c_str(), -1, rcZ,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            yOff += lineH; lines++;
        }
        if (!cell.motzStr.empty() && lines < maxLines)
        {
            CRect rcZ(rc.left + margin, yOff, rc.right - margin, yOff + lineH);
            pDC->SetTextColor(m_pFrame->m_colorMotzText);
            pDC->DrawText(cell.motzStr.c_str(), -1, rcZ,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE);
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
    if (!cell.isCurrentMonth) return m_pFrame->m_colorOtherMonthCell;

    const GregorianDate& g = cell.greg;
    bool isToday = (g.year == m_pFrame->m_today.year &&
        g.month == m_pFrame->m_today.month &&
        g.day == m_pFrame->m_today.day);
    if (isToday) return m_pFrame->m_colorTodayCell;

    for (const auto& h : cell.holidays)
        if (h.flags & HOLIDAY_YOM_TOV)     return m_pFrame->m_colorYomTovCell;
    for (const auto& h : cell.holidays)
        if (h.flags & HOLIDAY_ROSH_CHODESH) return m_pFrame->m_colorRoshChodeshCell;
    for (const auto& h : cell.holidays)
        if (h.flags & HOLIDAY_CHOL_HAMOED)  return m_pFrame->m_colorCholHamoedCell;
    for (const auto& h : cell.holidays)
        if (h.flags & HOLIDAY_FAST)         return m_pFrame->m_colorFastDayCell;

    if (GetDayOfWeek(g) == SHABBAT) return m_pFrame->m_colorShabbosCell;
    return m_pFrame->m_colorNormalCell;
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

bool CCalendarView::IsSelectedDate(const GregorianDate& g) const
{
    if (!m_selectedJdns.empty())
        return IsExplicitlySelected(g);
    return m_pFrame &&
        g.year == m_pFrame->m_selectedDate.year &&
        g.month == m_pFrame->m_selectedDate.month &&
        g.day == m_pFrame->m_selectedDate.day;
}

bool CCalendarView::IsExplicitlySelected(const GregorianDate& g) const
{
    long key = CalendarDateKey(g);
    return std::find(m_selectedJdns.begin(), m_selectedJdns.end(), key) != m_selectedJdns.end();
}

std::vector<GregorianDate> CCalendarView::GetSelectedDates() const
{
    std::vector<GregorianDate> dates;
    if (m_selectedJdns.empty())
    {
        if (m_pFrame)
            dates.push_back(m_pFrame->m_selectedDate);
        return dates;
    }

    std::vector<long> keys = m_selectedJdns;
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
    for (long key : keys)
        dates.push_back(JDNToGregorian(key));
    return dates;
}

void CCalendarView::ClearMultiSelection()
{
    m_selectedJdns.clear();
}

int CCalendarView::FindSelectedCellIndex() const
{
    if (!m_pFrame)
        return -1;
    for (int i = 0; i < (int)m_cells.size(); ++i)
        if (m_cells[i].greg.year == m_pFrame->m_selectedDate.year &&
            m_cells[i].greg.month == m_pFrame->m_selectedDate.month &&
            m_cells[i].greg.day == m_pFrame->m_selectedDate.day)
            return i;
    return -1;
}

void CCalendarView::ToggleSelectedCell(int idx)
{
    if (idx < 0 || idx >= (int)m_cells.size())
        return;

    if (m_selectedJdns.empty() && m_pFrame)
        m_selectedJdns.push_back(CalendarDateKey(m_pFrame->m_selectedDate));

    long key = CalendarDateKey(m_cells[idx].greg);
    auto it = std::find(m_selectedJdns.begin(), m_selectedJdns.end(), key);
    if (it != m_selectedJdns.end())
    {
        if (m_selectedJdns.size() > 1)
            m_selectedJdns.erase(it);
    }
    else
    {
        m_selectedJdns.push_back(key);
    }
    m_anchorIndex = idx;
}

void CCalendarView::SelectRangeToCell(int idx)
{
    if (idx < 0 || idx >= (int)m_cells.size())
        return;

    if (m_anchorIndex < 0 || m_anchorIndex >= (int)m_cells.size())
        m_anchorIndex = FindSelectedCellIndex();
    if (m_anchorIndex < 0)
        m_anchorIndex = idx;

    int first = min(m_anchorIndex, idx);
    int last  = max(m_anchorIndex, idx);
    m_selectedJdns.clear();
    for (int i = first; i <= last; ++i)
        m_selectedJdns.push_back(CalendarDateKey(m_cells[i].greg));
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
        bool ctrl = (nFlags & MK_CONTROL) != 0 || (::GetKeyState(VK_CONTROL) & 0x8000);
        bool shift = (nFlags & MK_SHIFT) != 0 || (::GetKeyState(VK_SHIFT) & 0x8000);

        if (shift)
            SelectRangeToCell(idx);
        else if (ctrl)
            ToggleSelectedCell(idx);
        else
        {
            ClearMultiSelection();
            m_anchorIndex = idx;
            m_dragSelecting = true;
            m_dragMoved = false;
            m_dragStartIndex = idx;
            m_lastDragIndex = idx;
            SetCapture();
        }

        m_pFrame->SelectDate(m_cells[idx].greg);
        Invalidate(FALSE);
    }
    CWnd::OnLButtonDown(nFlags, pt);
}

void CCalendarView::OnMouseMove(UINT nFlags, CPoint pt)
{
    if (m_dragSelecting && (nFlags & MK_LBUTTON))
    {
        int idx = HitTest(pt);
        if (idx >= 0 && idx < (int)m_cells.size() && idx != m_lastDragIndex)
        {
            m_lastDragIndex = idx;
            if (idx != m_dragStartIndex)
                m_dragMoved = true;
            SelectRangeToCell(idx);
            m_pFrame->SelectDate(m_cells[idx].greg);
            Invalidate(FALSE);
        }
    }
    CWnd::OnMouseMove(nFlags, pt);
}

void CCalendarView::OnLButtonUp(UINT nFlags, CPoint pt)
{
    if (m_dragSelecting)
    {
        if (GetCapture() == this)
            ReleaseCapture();
        if (!m_dragMoved)
            ClearMultiSelection();
        m_dragSelecting = false;
        m_dragStartIndex = -1;
        m_lastDragIndex = -1;
        Invalidate(FALSE);
    }
    CWnd::OnLButtonUp(nFlags, pt);
}

// Double-click — select the day and open the detailed day view popup.
void CCalendarView::OnLButtonDblClk(UINT nFlags, CPoint pt)
{
    int idx = HitTest(pt);
    if (idx >= 0 && idx < (int)m_cells.size())
    {
        if (m_dragSelecting && GetCapture() == this)
            ReleaseCapture();
        m_dragSelecting = false;
        SetFocus();
        ClearMultiSelection();
        m_anchorIndex = idx;
        m_pFrame->SelectDate(m_cells[idx].greg);
        m_pFrame->OpenDayViewForDate(m_cells[idx].greg);
    }
}

static HebrewDate AddHebrewYears(HebrewDate hd, int years)
{
    int newYear  = hd.year + years;
    bool newLeap = IsHebrewLeapYear(newYear);
    int newMonth = hd.month;
    if      (newMonth == ADAR    &&  newLeap) newMonth = ADAR_II;
    else if (newMonth == ADAR_II && !newLeap) newMonth = ADAR;
    int maxDay = DaysInHebrewMonth(newMonth, newYear);
    return HebrewDate(newYear, newMonth, hd.day < maxDay ? hd.day : maxDay);
}

// Right-click — show context menu: Add Event / Print Day Details / Bar|Bat Mitzvah jump.
void CCalendarView::OnRButtonDown(UINT nFlags, CPoint pt)
{
    int idx = HitTest(pt);
    if (idx < 0 || idx >= (int)m_cells.size())
    {
        CWnd::OnRButtonDown(nFlags, pt);
        return;
    }
    SetFocus();
    const GregorianDate& g = m_cells[idx].greg;
    std::vector<GregorianDate> selectedDates = GetSelectedDates();
    bool clickedInMultiSelection = selectedDates.size() > 1 && IsExplicitlySelected(g);
    if (!clickedInMultiSelection)
    {
        ClearMultiSelection();
        m_anchorIndex = idx;
        selectedDates.clear();
        selectedDates.push_back(g);
    }
    m_pFrame->SelectDate(g);

    wchar_t addLabel[80];
    swprintf_s(addLabel, L"Add Event on %d/%d/%d...", g.month, g.day, g.year);
    bool printSelected = selectedDates.size() > 1;

    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, 1, addLabel);
    menu.AppendMenu(MF_STRING, 2, printSelected
        ? L"Print Day Details for Selected Days"
        : L"Print Day Details");
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, 3, L"Jump to Bar Mitzvah from this date");
    menu.AppendMenu(MF_STRING, 4, L"Jump to Bat Mitzvah from this date");

    CPoint screenPt = pt;
    ClientToScreen(&screenPt);
    int cmd = (int)menu.TrackPopupMenu(
        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
        screenPt.x, screenPt.y, this);

    if (cmd == 1)
        m_pFrame->AddEventForDate(g);
    else if (cmd == 2)
    {
        if (printSelected)
            DoPrintDays(selectedDates, m_pFrame);
        else
            DoPrintDay(g, m_pFrame);
    }
    else if (cmd == 3 || cmd == 4)
    {
        ClearMultiSelection();
        m_anchorIndex = idx;
        int years = (cmd == 3) ? 13 : 12;
        HebrewDate target = AddHebrewYears(GregorianToHebrew(g), years);
        m_pFrame->SelectDate(HebrewToGregorian(target));
    }

    CWnd::OnRButtonDown(nFlags, pt);
}

// =============================================================================
// KEYBOARD HANDLING
// =============================================================================

// Arrow keys navigate days, Page Up/Down navigate months, Home = today.
void CCalendarView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (!m_pFrame) return;

    long jdn = HebrewToJDN(m_pFrame->m_selectedHebrew);
    auto clearKeyboardSelection = [&]() {
        ClearMultiSelection();
        m_anchorIndex = -1;
    };

    switch (nChar)
    {
    case VK_LEFT:  clearKeyboardSelection(); m_pFrame->SelectDate(JDNToGregorian(jdn - 1)); break;
    case VK_RIGHT: clearKeyboardSelection(); m_pFrame->SelectDate(JDNToGregorian(jdn + 1)); break;
    case VK_UP:    clearKeyboardSelection(); m_pFrame->SelectDate(JDNToGregorian(jdn - 7)); break;
    case VK_DOWN:  clearKeyboardSelection(); m_pFrame->SelectDate(JDNToGregorian(jdn + 7)); break;
    case VK_PRIOR: clearKeyboardSelection(); m_pFrame->PrevMonth(); break;
    case VK_NEXT:  clearKeyboardSelection(); m_pFrame->NextMonth(); break;
    case VK_HOME:  clearKeyboardSelection(); m_pFrame->GoToToday(); break;
    default:
        CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
        break;
    }
}
