// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    SidebarPanel.cpp
// Purpose: Draws the day details sidebar: selected date, holidays,
//          omer, parasha, daily learning, year facts.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial implementation.
// =============================================================================

#include "pch.h"
#include "SidebarPanel.h"
#include "HolidayEngine.h"

BEGIN_MESSAGE_MAP(CSidebarPanel, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

CSidebarPanel::CSidebarPanel(CMainFrame* pFrame)
    : m_pFrame(pFrame)
{
}

CSidebarPanel::~CSidebarPanel()
{
}

BOOL CSidebarPanel::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

void CSidebarPanel::OnLButtonDown(UINT /*nFlags*/, CPoint pt)
{
    if (m_yearHdrY >= 0 && abs(pt.y - m_yearHdrY) <= 8)
    {
        SetCapture();
        m_dragging   = true;
        m_dragStartY = pt.y;
        SetCursor(LoadCursor(nullptr, IDC_SIZENS));
    }
}

void CSidebarPanel::OnMouseMove(UINT /*nFlags*/, CPoint pt)
{
    if (!m_dragging) return;
    CRect rc; GetClientRect(&rc);
    int cy = rc.Height();
    if (cy > 40)
    {
        m_splitFrac = (float)max(80, min(pt.y, cy - 40)) / (float)cy;
        Invalidate(FALSE);
    }
}

void CSidebarPanel::OnLButtonUp(UINT /*nFlags*/, CPoint /*pt*/)
{
    if (m_dragging)
    {
        ReleaseCapture();
        m_dragging = false;
    }
}

// Draws a thin separator line.
void CSidebarPanel::DrawSep(CDC* pDC, int x, int& yOff, int w)
{
    CPen pen(PS_SOLID, 1, RGB(180, 190, 210));
    CPen* pOld = pDC->SelectObject(&pen);
    pDC->MoveTo(x, yOff);
    pDC->LineTo(x + w, yOff);
    pDC->SelectObject(pOld);
    yOff += 8;
}

// Draws the entire sidebar using double-buffering.
void CSidebarPanel::OnPaint()
{
    CPaintDC dcScreen(this);
    CRect rcClient;
    GetClientRect(&rcClient);
    int cx = rcClient.Width();
    int cy = rcClient.Height();

    CDC memDC;
    CBitmap bmp;
    memDC.CreateCompatibleDC(&dcScreen);
    bmp.CreateCompatibleBitmap(&dcScreen, cx, cy);
    CBitmap* pOldBmp = memDC.SelectObject(&bmp);

    // Background
    memDC.FillSolidRect(rcClient, CLR_SIDEBAR_BG);

    // Right border
    CPen penBorder(PS_SOLID, 1, RGB(180, 190, 210));
    CPen* pOldPen = memDC.SelectObject(&penBorder);
    memDC.MoveTo(cx - 1, 0);
    memDC.LineTo(cx - 1, cy);
    memDC.SelectObject(pOldPen);

    memDC.SetBkMode(TRANSPARENT);
    int x    = 8;
    int indent = 14;  // left indent for text items
    int yOff = 10;
    int w = cx - 16;

    // Helper: draw word-wrapped text at (x+indent, yOff), returns height used
    auto drawWrapped = [&](const wchar_t* txt, int maxH) -> int {
        CRect rc(x + indent, yOff, x + w, yOff + maxH);
        memDC.DrawText(txt, -1, &rc, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_CALCRECT);
        int h = rc.Height();
        rc.top = yOff; rc.bottom = yOff + h;
        memDC.DrawText(txt, -1, &rc, DT_LEFT | DT_TOP | DT_WORDBREAK);
        return h + 3;
    };

    // "Day details" header bar
    CRect rcHdr(0, yOff - 2, cx - 1, yOff + 20);
    memDC.FillSolidRect(rcHdr, CLR_HEADER_BG);
    memDC.SelectObject(&m_pFrame->m_fontBold);
    memDC.SetTextColor(RGB(255, 255, 255));
    CRect rcHdrTxt = rcHdr; rcHdrTxt.left += 6;
    memDC.DrawText(L"Day details", rcHdrTxt,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    yOff += 26;

    // Gregorian date
    bool leap = IsHebrewLeapYear(m_pFrame->m_selectedHebrew.year);
    CString gregLine1 = DayOfWeekName(
        GetDayOfWeek(m_pFrame->m_selectedDate)).c_str();
    CString gregLine2;
    gregLine2.Format(L"%d %s %d",
        m_pFrame->m_selectedDate.day,
        GregorianMonthName(m_pFrame->m_selectedDate.month).c_str(),
        m_pFrame->m_selectedDate.year);
    CString gregFull = gregLine1 + L"\r\n" + gregLine2;

    memDC.SelectObject(&m_pFrame->m_fontBold);
    memDC.SetTextColor(CLR_GREG_TXT);
    CRect rcGreg(x, yOff, x + w, yOff + 36);
    memDC.DrawText(gregFull, rcGreg, DT_LEFT | DT_TOP);
    yOff += 40;

    // Hebrew date
    CString hebDate;
    hebDate.Format(L"%d %s %d",
        m_pFrame->m_selectedHebrew.day,
        HebrewMonthName(m_pFrame->m_selectedHebrew.month, leap).c_str(),
        m_pFrame->m_selectedHebrew.year);
    memDC.SelectObject(&m_pFrame->m_fontBold);
    memDC.SetTextColor(CLR_HEBREW_TXT);
    CRect rcHeb(x, yOff, x + w, yOff + 18);
    memDC.DrawText(hebDate, rcHeb, DT_LEFT | DT_TOP);
    yOff += 22;

    DrawSep(&memDC, x, yOff, w);

    // Holidays
    auto hols = GetHolidays(m_pFrame->m_selectedHebrew, m_pFrame->m_isIsrael);
    if (m_pFrame->m_showMoadim && !hols.empty())
    {
        memDC.SelectObject(&m_pFrame->m_fontNormal);
        memDC.SetTextColor(CLR_HOLIDAY_TXT);
        for (const auto& hol : hols)
        {
            if (yOff > cy - 20) break;
            std::wstring txt = hol.name;
            if (!hol.subtitle.empty()) txt += L" - " + hol.subtitle;
            yOff += drawWrapped(txt.c_str(), 48);
        }
        yOff += 2;
    }

    auto userEvents = m_pFrame->GetUserEventsForDate(m_pFrame->m_selectedDate);
    if (!userEvents.empty())
    {
        memDC.SelectObject(&m_pFrame->m_fontNormal);
        memDC.SetTextColor(RGB(120, 30, 120));
        for (const auto& eventTitle : userEvents)
        {
            if (yOff > cy - 20) break;
            yOff += drawWrapped(eventTitle.c_str(), 48);
        }
        yOff += 2;
    }

    // Omer
    OmerInfo omer = GetOmer(m_pFrame->m_selectedHebrew);
    if (omer.isOmerDay)
    {
        memDC.SelectObject(&m_pFrame->m_fontNormal);
        memDC.SetTextColor(RGB(100, 60, 140));
        yOff += drawWrapped(omer.text.c_str(), 60);
    }

    // Parasha
    ParashaInfo par = GetParasha(m_pFrame->m_selectedHebrew,
        m_pFrame->m_isIsrael);
    if (m_pFrame->m_showParshios && !par.name.empty())
    {
        std::wstring parTxt = L"Parasha: " + par.name;
        if (par.isCombined && !par.name2.empty())
            parTxt += L" - " + par.name2;
        memDC.SelectObject(&m_pFrame->m_fontNormal);
        memDC.SetTextColor(RGB(40, 80, 40));
        yOff += drawWrapped(parTxt.c_str(), 48);
    }

    DrawSep(&memDC, x, yOff, w);

    // Daily learning — label on left, value wraps on right
    DailyLearning dl = GetDailyLearning(m_pFrame->m_selectedHebrew,
        m_pFrame->m_selectedDate);
    struct { const wchar_t* label; const std::wstring& val; }
    learnings[] = {
        { L"Daf Yomi:",   dl.dafYomi      },
        { L"Yerushalmi:", dl.yerushalmi   },
        { L"Mishna:",     dl.mishnaYomit  },
        { L"Halacha:",    dl.halachaYomit },
        { L"Tanach:",     dl.tanachYomi   }
    };
    for (int i = 0; i < (int)(sizeof(learnings) / sizeof(learnings[0])); i++)
    {
        const auto& lrn = learnings[i];
        if (yOff > cy - 20 || lrn.val.empty()) continue;
        memDC.SelectObject(&m_pFrame->m_fontSmall);
        memDC.SetTextColor(RGB(80, 80, 80));
        CRect rcLbl(x + indent, yOff, x + indent + 65, yOff + 14);
        memDC.DrawText(lrn.label, -1, rcLbl, DT_LEFT | DT_TOP | DT_SINGLELINE);
        // Value: measure wrap height from after label
        int valX = x + indent + 65;
        CRect rcVal(valX, yOff, x + w, yOff + 60);
        memDC.SetTextColor(RGB(20, 20, 100));
        memDC.DrawText(lrn.val.c_str(), -1, &rcVal,
            DT_LEFT | DT_TOP | DT_WORDBREAK | DT_CALCRECT);
        int rowH = max(14, rcVal.Height());
        rcVal.top = yOff; rcVal.bottom = yOff + rowH;
        memDC.DrawText(lrn.val.c_str(), -1, &rcVal,
            DT_LEFT | DT_TOP | DT_WORDBREAK);
        yOff += rowH + 2;
    }

    // Molad box
    {
        const HebrewDate& hm = m_pFrame->m_selectedHebrew;
        int Y = hm.year;
        bool isLeap = IsHebrewLeapYear(Y);
        // Offset of current month from Tishrei (0-based)
        long monthOffset;
        if (!isLeap)
            monthOffset = (hm.month <= ADAR) ? (hm.month - 1) : (hm.month - 2);
        else
            monthOffset = hm.month - 1;
        long long completedYears = (long long)(Y - 1);
        long long leapsBeforeY   = (7LL * completedYears + 1LL) / 19LL;
        long long M              = 12LL * completedYears + leapsBeforeY + monthOffset;
        long long totalParts     = M * 765433LL + 57444LL;
        int dayIdx = (int)((totalParts / 25920LL) % 7LL);
        int hour   = (int)((totalParts % 25920LL) / 1080LL);
        int parts  = (int)(totalParts % 1080LL);
        static const wchar_t* kDayNames[7] = {
            L"Shabbos", L"Sunday", L"Monday", L"Tuesday",
            L"Wednesday", L"Thursday", L"Friday"
        };
        DrawSep(&memDC, x, yOff, w);
        CRect rcMHdr(0, yOff - 2, cx - 1, yOff + 20);
        memDC.FillSolidRect(rcMHdr, CLR_HEADER_BG);
        memDC.SelectObject(&m_pFrame->m_fontBold);
        memDC.SetTextColor(RGB(255, 255, 255));
        CRect rcMHdrTxt = rcMHdr; rcMHdrTxt.left += 6;
        memDC.DrawText(L"Molad", rcMHdrTxt, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        yOff += 26;
        CString moladStr;
        moladStr.Format(L"%s, %dh %dp", kDayNames[dayIdx], hour, parts);
        // Also show civil time approximation: epoch = Friday, 6 Oct 3761 BCE
        // Skip civil time for now; just show traditional
        memDC.SelectObject(&m_pFrame->m_fontNormal);
        memDC.SetTextColor(RGB(60, 40, 110));
        CRect rcMolad(x, yOff, x + w, yOff + 16);
        memDC.DrawText(moladStr, rcMolad, DT_LEFT | DT_TOP | DT_SINGLELINE);
        yOff += 20;
    }

    // Year details header — positioned at m_splitFrac or after day content
    {
        int splitY = (int)(m_splitFrac * cy);
        yOff = max(yOff + 6, min(splitY, cy - 40));
        m_yearHdrY = yOff;
        if (yOff + 20 < cy)
        {
            CRect rcYHdr(0, yOff, cx - 1, yOff + 20);
            memDC.FillSolidRect(rcYHdr, CLR_HEADER_BG);
            memDC.SelectObject(&m_pFrame->m_fontBold);
            memDC.SetTextColor(RGB(255, 255, 255));
            CRect rcYTxt = rcYHdr; rcYTxt.left += 6;
            memDC.DrawText(L"Year details", rcYTxt,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            yOff += 24;
        }
    }

    // Year facts
    auto facts = GetYearFacts(m_pFrame->m_selectedHebrew.year);
    memDC.SelectObject(&m_pFrame->m_fontSmall);
    memDC.SetTextColor(RGB(60, 60, 60));
    for (const auto& fact : facts)
    {
        if (yOff > cy - 14) break;
        yOff += drawWrapped(fact.c_str(), 48);
    }

    dcScreen.BitBlt(0, 0, cx, cy, &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}
