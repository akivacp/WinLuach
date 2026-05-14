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
    ON_WM_VSCROLL()
    ON_WM_MOUSEWHEEL()
    ON_WM_SIZE()
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

void CSidebarPanel::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    UpdateScrollBar(cy);
}

BOOL CSidebarPanel::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    int old = m_scrollPos;
    m_scrollPos = max(0, min(max(0, m_contentHeight - 1), m_scrollPos - zDelta / WHEEL_DELTA * 45));
    CRect rc; GetClientRect(&rc);
    if (m_contentHeight > rc.Height())
        m_scrollPos = min(m_scrollPos, m_contentHeight - rc.Height());
    else
        m_scrollPos = 0;
    if (m_scrollPos != old)
    {
        UpdateScrollBar(rc.Height());
        Invalidate(FALSE);
    }
    return TRUE;
}

void CSidebarPanel::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CRect rc; GetClientRect(&rc);
    int page = max(20, rc.Height());
    int maxPos = max(0, m_contentHeight - rc.Height());
    int pos = m_scrollPos;
    switch (nSBCode)
    {
    case SB_LINEUP: pos -= 24; break;
    case SB_LINEDOWN: pos += 24; break;
    case SB_PAGEUP: pos -= page; break;
    case SB_PAGEDOWN: pos += page; break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK: pos = (int)nPos; break;
    case SB_TOP: pos = 0; break;
    case SB_BOTTOM: pos = maxPos; break;
    default: break;
    }
    m_scrollPos = max(0, min(maxPos, pos));
    UpdateScrollBar(rc.Height());
    Invalidate(FALSE);
}

void CSidebarPanel::UpdateScrollBar(int clientHeight)
{
    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = max(0, m_contentHeight);
    si.nPage = max(1, clientHeight);
    int maxPos = max(0, m_contentHeight - clientHeight);
    m_scrollPos = max(0, min(m_scrollPos, maxPos));
    si.nPos = m_scrollPos;
    SetScrollInfo(SB_VERT, &si, TRUE);
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
    int contentLimit = m_scrollPos + cy + 10000;

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

    memDC.SetViewportOrg(0, -m_scrollPos);
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
            if (yOff > contentLimit - 20) break;
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
            if (yOff > contentLimit - 20) break;
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
        if (yOff > contentLimit - 20 || lrn.val.empty()) continue;
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

    // Zmanim section — sof shema and sof tefila (GRA and MA)
    if (yOff < contentLimit - 20)
    {
        const ZmanimResult& zz = m_pFrame->m_zmanim;
        DrawSep(&memDC, x, yOff, w);
        CRect rcZHdr(0, yOff - 2, cx - 1, yOff + 20);
        memDC.FillSolidRect(rcZHdr, CLR_HEADER_BG);
        memDC.SelectObject(&m_pFrame->m_fontBold);
        memDC.SetTextColor(RGB(255, 255, 255));
        CRect rcZTxt = rcZHdr; rcZTxt.left += 6;
        memDC.DrawText(L"Zmanim", rcZTxt, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        yOff += 26;

        struct ZRow { const wchar_t* label; TimeOfDay time; };
        ZRow zrows[] = {
            { L"Sof Shema (GRA):",  zz.sofShema_GRA   },
            { L"Sof Shema (MA):",   zz.sofShema_MA72  },
            { L"Sof Tefila (GRA):", zz.sofTefilla_GRA },
            { L"Sof Tefila (MA):",  zz.sofTefilla_MA72},
        };
        int lblW2 = 110;
        for (const auto& zr : zrows)
        {
            if (yOff > contentLimit - 16) break;
            memDC.SelectObject(&m_pFrame->m_fontSmall);
            memDC.SetTextColor(RGB(70, 70, 50));
            CRect rcL(x + indent, yOff, x + indent + lblW2, yOff + 16);
            memDC.DrawText(zr.label, -1, rcL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
            memDC.SelectObject(&m_pFrame->m_fontNormal);
            memDC.SetTextColor(RGB(20, 20, 140));
            CRect rcT(x + indent + lblW2, yOff, x + w, yOff + 16);
            std::wstring ts = FormatTime(zr.time, m_pFrame->m_use24hr);
            memDC.DrawText(ts.c_str(), -1, rcT, DT_LEFT | DT_TOP | DT_SINGLELINE);
            yOff += 18;
        }
    }

    // Special times for notable days
    {
        const ZmanimResult& zs = m_pFrame->m_zmanim;
        const HebrewDate& hs   = m_pFrame->m_selectedHebrew;
        const GregorianDate& gs = m_pFrame->m_selectedDate;
        DayOfWeek sdow  = GetDayOfWeek(gs);
        bool isShab2    = (sdow == SHABBAT);
        bool isFri2     = (sdow == FRIDAY);

        auto hols3 = GetHolidays(hs, m_pFrame->m_isIsrael);
        auto hasF3 = [&](int flag) {
            for (const auto& ho : hols3) if (ho.flags & flag) return true; return false;
        };
        bool hasFast2     = hasF3(HOLIDAY_FAST);
        bool isErevYK2    = (hs.month == TISHREI && hs.day == 9);
        bool isTishaBav2  = (hs.month == AV && hs.day == 9);
        bool isErevTB2    = (hs.month == AV && hs.day == 8);
        bool isErevPesach2= (hs.month == NISSAN && hs.day == 14);
        bool isLagBaOmer2 = (hs.month == IYAR   && hs.day == 18);
        bool is10Av2      = (hs.month == AV      && hs.day == 10);

        struct SpecTime { const wchar_t* label; TimeOfDay time; };
        SpecTime stimes[12]; int nst = 0;

        if (isShab2 && zs.tzeitShabbat.IsValid())
            stimes[nst++] = { L"Tzeis Shabbos:", zs.tzeitShabbat };
        if (hasFast2 && !isShab2 && zs.tzeit_GRA.IsValid())
            stimes[nst++] = { L"Fast ends:", zs.tzeit_GRA };
        if (isErevTB2 && !isShab2 && !isFri2 && zs.shkia.IsValid())
            stimes[nst++] = { L"Fast begins:", zs.shkia };
        if (isTishaBav2 && zs.chatzot.IsValid())
            stimes[nst++] = { L"Chatzos:", zs.chatzot };
        if (is10Av2 && zs.chatzot.IsValid())
            stimes[nst++] = { L"Chatzos:", zs.chatzot };
        if (isErevYK2 && zs.shkia.IsValid())
        {
            if (zs.chatzot.IsValid())
                stimes[nst++] = { L"Chatzos:", zs.chatzot };
            stimes[nst++] = { L"Fast begins:", zs.shkia };
            if (zs.candleLighting.IsValid())
                stimes[nst++] = { L"Candle lighting:", zs.candleLighting };
        }
        if (isErevPesach2 && zs.hanetz.IsValid() && zs.shaahZmanit_GRA > 0.0)
        {
            TimeOfDay sofAchila = AddMinutes(zs.hanetz, (int)(4.0 * zs.shaahZmanit_GRA));
            TimeOfDay sofBiur   = AddMinutes(zs.hanetz, (int)(5.0 * zs.shaahZmanit_GRA));
            if (sofAchila.IsValid()) stimes[nst++] = { L"Eat chametz by:", sofAchila };
            if (sofBiur.IsValid())   stimes[nst++] = { L"Burn chametz by:", sofBiur };
            if (zs.chatzot.IsValid()) stimes[nst++] = { L"Chatzos:", zs.chatzot };
        }
        if (isLagBaOmer2 && zs.chatzot.IsValid())
            stimes[nst++] = { L"Chatzos:", zs.chatzot };

        if (nst > 0 && yOff < contentLimit - 20)
        {
            DrawSep(&memDC, x, yOff, w);
            CRect rcSHdr(0, yOff - 2, cx - 1, yOff + 20);
            memDC.FillSolidRect(rcSHdr, CLR_HEADER_BG);
            memDC.SelectObject(&m_pFrame->m_fontBold);
            memDC.SetTextColor(RGB(255, 255, 255));
            CRect rcSTxt = rcSHdr; rcSTxt.left += 6;
            memDC.DrawText(L"Special Times", rcSTxt, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            yOff += 26;

            for (int si = 0; si < nst; si++)
            {
                if (yOff > contentLimit - 16) break;
                int lblW2 = 100;
                memDC.SelectObject(&m_pFrame->m_fontSmall);
                memDC.SetTextColor(RGB(70, 70, 50));
                CRect rcL(x + indent, yOff, x + indent + lblW2, yOff + 16);
                memDC.DrawText(stimes[si].label, -1, rcL,
                    DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
                memDC.SelectObject(&m_pFrame->m_fontNormal);
                memDC.SetTextColor(RGB(20, 20, 140));
                std::wstring ts = FormatTime(stimes[si].time, m_pFrame->m_use24hr);
                CRect rcT(x + indent + lblW2, yOff, x + w, yOff + 16);
                memDC.DrawText(ts.c_str(), -1, rcT, DT_LEFT | DT_TOP | DT_SINGLELINE);
                yOff += 18;
            }
        }
    }

    // Year details header — positioned at m_splitFrac or after day content
    {
        int splitY = (int)(m_splitFrac * cy);
        yOff = max(yOff + 6, min(splitY, contentLimit - 40));
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

    // Year facts with alternating row shading
    auto facts = GetYearFacts(m_pFrame->m_selectedHebrew.year);
    memDC.SelectObject(&m_pFrame->m_fontSmall);
    memDC.SetTextColor(RGB(60, 60, 60));
    int rowIdx = 0;
    for (const auto& fact : facts)
    {
        if (yOff > contentLimit - 14) break;
        COLORREF rowBg = (rowIdx % 2 == 0) ? RGB(235, 240, 250) : RGB(220, 228, 245);
        memDC.FillSolidRect(CRect(0, yOff, cx, yOff + 16), rowBg);
        rowIdx++;
        yOff += drawWrapped(fact.c_str(), 48);
    }

    // Molad box
    {
        const HebrewDate& hm = m_pFrame->m_selectedHebrew;
        static const wchar_t* kDayNames[7] = {
            L"Shabbos", L"Sunday", L"Monday", L"Tuesday",
            L"Wednesday", L"Thursday", L"Friday"
        };

        auto moladText = [&](int year, int month) -> CString {
            bool isLeap = IsHebrewLeapYear(year);
            long monthOffset = isLeap
                ? (month - 1)
                : ((month <= ADAR) ? (month - 1) : (month - 2));
            long long completedYears = (long long)(year - 1);
            long long leapsBeforeY   = (7LL * completedYears + 1LL) / 19LL;
            long long M              = 12LL * completedYears + leapsBeforeY + monthOffset;
            long long totalParts     = M * 765433LL + 57444LL;
            long long dayCount = totalParts / 25920LL;
            long long dayParts = totalParts % 25920LL;
            dayParts -= 6LL * 1080LL; // molad hours are counted from 6pm; convert to civil clock
            if (dayParts < 0)
            {
                dayParts += 25920LL;
                dayCount--;
            }
            int dayIdx = (int)(dayCount % 7LL);
            if (dayIdx < 0) dayIdx += 7;
            int hour   = (int)(dayParts / 1080LL);
            int parts  = (int)(dayParts % 1080LL);
            int mins   = parts / 18;
            int chalak = parts % 18;
            int hr12   = hour % 12; if (hr12 == 0) hr12 = 12;
            const wchar_t* ampm = (hour < 12) ? L"am" : L"pm";
            CString text;
            text.Format(L"Molad %s %d: %s %d:%02d %s and %d chalakim",
                HebrewMonthName(month, isLeap).c_str(), year,
                kDayNames[dayIdx], hr12, mins, ampm, chalak);
            return text;
        };

        int nextYear = hm.year;
        int nextMonth = hm.month + 1;
        if (nextMonth > MonthsInHebrewYear(hm.year))
        {
            nextMonth = TISHREI;
            nextYear++;
        }

        DrawSep(&memDC, x, yOff, w);
        CRect rcMHdr(0, yOff - 2, cx - 1, yOff + 20);
        memDC.FillSolidRect(rcMHdr, CLR_HEADER_BG);
        memDC.SelectObject(&m_pFrame->m_fontBold);
        memDC.SetTextColor(RGB(255, 255, 255));
        CRect rcMHdrTxt = rcMHdr; rcMHdrTxt.left += 6;
        memDC.DrawText(L"Molad", rcMHdrTxt, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        yOff += 26;
        memDC.SelectObject(&m_pFrame->m_fontNormal);
        memDC.SetTextColor(RGB(60, 40, 110));
        CString currentMolad = moladText(hm.year, hm.month);
        CString nextMolad = moladText(nextYear, nextMonth);
        yOff += drawWrapped(currentMolad.GetString(), 70);
        yOff += drawWrapped(nextMolad.GetString(), 70);
    }

    m_contentHeight = max(yOff + 12, cy);
    memDC.SetViewportOrg(0, 0);
    UpdateScrollBar(cy);
    dcScreen.BitBlt(0, 0, cx, cy, &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}
