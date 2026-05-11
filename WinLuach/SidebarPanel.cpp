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
    int x = 8;
    int yOff = 10;
    int w = cx - 16;

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
            CRect rcH(x, yOff, x + w, yOff + 16);
            memDC.DrawText(txt.c_str(), -1, rcH,
                DT_LEFT | DT_TOP | DT_END_ELLIPSIS | DT_SINGLELINE);
            yOff += 17;
        }
        yOff += 4;
    }

    auto userEvents = m_pFrame->GetUserEventsForDate(m_pFrame->m_selectedDate);
    if (!userEvents.empty())
    {
        memDC.SelectObject(&m_pFrame->m_fontNormal);
        memDC.SetTextColor(RGB(120, 30, 120));
        for (const auto& eventTitle : userEvents)
        {
            if (yOff > cy - 20) break;
            CRect rcEvent(x, yOff, x + w, yOff + 16);
            memDC.DrawText(eventTitle.c_str(), -1, rcEvent,
                DT_LEFT | DT_TOP | DT_END_ELLIPSIS | DT_SINGLELINE);
            yOff += 17;
        }
        yOff += 4;
    }

    // Omer
    OmerInfo omer = GetOmer(m_pFrame->m_selectedHebrew);
    if (omer.isOmerDay)
    {
        memDC.SelectObject(&m_pFrame->m_fontNormal);
        memDC.SetTextColor(RGB(100, 60, 140));
        CRect rcOmer(x, yOff, x + w, yOff + 28);
        memDC.DrawText(omer.text.c_str(), -1, rcOmer,
            DT_LEFT | DT_TOP | DT_WORDBREAK);
        yOff += 30;
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
        CRect rcPar(x, yOff, x + w, yOff + 32);
        memDC.DrawText(parTxt.c_str(), -1, rcPar,
            DT_LEFT | DT_TOP | DT_WORDBREAK);
        yOff += 36;
    }

    DrawSep(&memDC, x, yOff, w);

    // Daily learning
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
    bool showLearning[] = {
        m_pFrame->m_showDafYomi,
        m_pFrame->m_showYerushalmi,
        m_pFrame->m_showMishnaYomit,
        m_pFrame->m_showHalachaYomit,
        m_pFrame->m_showTanachYomi
    };

    for (int i = 0; i < (int)(sizeof(learnings) / sizeof(learnings[0])); i++)
    {
        const auto& lrn = learnings[i];
        if (!showLearning[i]) continue;
        if (yOff > cy - 20 || lrn.val.empty()) break;
        memDC.SelectObject(&m_pFrame->m_fontSmall);
        memDC.SetTextColor(RGB(80, 80, 80));
        CRect rcLbl(x, yOff, x + 65, yOff + 14);
        memDC.DrawText(lrn.label, -1, rcLbl,
            DT_LEFT | DT_TOP | DT_SINGLELINE);
        memDC.SetTextColor(RGB(20, 20, 100));
        CRect rcVal(x + 65, yOff, x + w, yOff + 14);
        memDC.DrawText(lrn.val.c_str(), -1, rcVal,
            DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        yOff += 16;
    }

    // Year details header
    yOff += 6;
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

    // Year facts
    auto facts = GetYearFacts(m_pFrame->m_selectedHebrew.year);
    memDC.SelectObject(&m_pFrame->m_fontSmall);
    memDC.SetTextColor(RGB(60, 60, 60));
    for (const auto& fact : facts)
    {
        if (yOff > cy - 14) break;
        CRect rcFact(x, yOff, x + w, yOff + 13);
        memDC.DrawText(fact.c_str(), -1, rcFact,
            DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        yOff += 14;
    }

    dcScreen.BitBlt(0, 0, cx, cy, &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}
