// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    CalPrintDlg.cpp
// Purpose: Calendar printing and print-preview implementation.
// =============================================================================

#include "pch.h"
#include "CalPrintDlg.h"
#include "MainFrame.h"
#include "WinLuachApp.h"
#include "Settings.h"
#include "Zmanim.h"
#include "HolidayEngine.h"
#include <afxdlgs.h>
#include <functional>

// =============================================================================
// DRAW CAL MONTH PAGE
// Renders a complete calendar month into rcPage (device units of pDC).
// Works for any DC: screen (preview), printer, metafile.
// =============================================================================

void DrawCalMonthPage(CDC* pDC, const CRect& rcPage,
                      int year, int month, CMainFrame* pFrame,
                      const CalPrintOptions& opts)
{
    const int W  = rcPage.Width();
    const int H  = rcPage.Height();
    const int x0 = rcPage.left;
    const int y0 = rcPage.top;

    pDC->FillSolidRect(rcPage, RGB(255, 255, 255));

    // --- Font sizes (negative = character height in device units) ---
    int fTitle = -(H * 5  / 100);  if (fTitle > -6)  fTitle = -6;
    int fHdr   = -(H * 28 / 1000); if (fHdr   > -5)  fHdr   = -5;
    int fDay   = -(H * 26 / 1000); if (fDay   > -5)  fDay   = -5;
    int fSmall = -(H * 20 / 1000); if (fSmall > -4)  fSmall = -4;

    CFont fontTitle, fontHdr, fontDay, fontSmall;
    fontTitle.CreateFont(fTitle, 0,0,0, FW_BOLD,   0,0,0, DEFAULT_CHARSET, 0,0,
        CLEARTYPE_QUALITY, DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
    fontHdr  .CreateFont(fHdr,   0,0,0, FW_BOLD,   0,0,0, DEFAULT_CHARSET, 0,0,
        CLEARTYPE_QUALITY, DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
    fontDay  .CreateFont(fDay,   0,0,0, FW_BOLD,   0,0,0, DEFAULT_CHARSET, 0,0,
        CLEARTYPE_QUALITY, DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
    fontSmall.CreateFont(fSmall, 0,0,0, FW_NORMAL, 0,0,0, DEFAULT_CHARSET, 0,0,
        CLEARTYPE_QUALITY, DEFAULT_PITCH|FF_SWISS, L"Segoe UI");

    // === Title bar (10% of height) ==========================================
    int titleH = H * 10 / 100;
    CRect rcTitle(x0, y0, x0 + W, y0 + titleH);
    pDC->FillSolidRect(rcTitle, RGB(50, 80, 140));

    GregorianDate gFirst(year, month, 1);
    GregorianDate gLast(year, month, DaysInGregorianMonth(month, year));
    HebrewDate hFirst = GregorianToHebrew(gFirst);
    HebrewDate hLast  = GregorianToHebrew(gLast);

    CString gregStr;
    gregStr.Format(L"%s  %d", GregorianMonthName(month).c_str(), year);

    std::wstring hs = HebrewMonthName(hFirst.month, IsHebrewLeapYear(hFirst.year));
    if (hFirst.month != hLast.month)
        hs += L" - " + HebrewMonthName(hLast.month, IsHebrewLeapYear(hLast.year));
    CString hebStr;
    hebStr.Format(L"%s  %d", hs.c_str(), hFirst.year);
    if (hFirst.year != hLast.year)
        hebStr.AppendFormat(L" - %d", hLast.year);

    pDC->SetBkMode(TRANSPARENT);
    pDC->SetTextColor(RGB(255, 255, 255));
    pDC->SelectObject(&fontTitle);

    CRect rcG(x0 + W/20, y0, x0 + W * 45/100, y0 + titleH);
    CRect rcH(x0 + W * 55/100, y0, x0 + W - W/20, y0 + titleH);
    pDC->DrawText(gregStr, rcG, DT_LEFT  | DT_VCENTER | DT_SINGLELINE);
    pDC->DrawText(hebStr,  rcH, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    // === Day-of-week header row (5% of height) ==============================
    int dayHdrH = H * 5 / 100;
    int dayHdrY = y0 + titleH;
    CRect rcDayHdr(x0, dayHdrY, x0 + W, dayHdrY + dayHdrH);
    pDC->FillSolidRect(rcDayHdr, RGB(70, 100, 160));

    // Column layout
    int colX[8];
    int baseColW = W / 7;
    int extra    = W - baseColW * 7;
    colX[0] = x0;
    for (int i = 1; i <= 7; i++)
        colX[i] = colX[i - 1] + baseColW + (i == 7 ? extra : 0);

    static const wchar_t* kDays[7] = {
        L"Sunday", L"Monday", L"Tuesday", L"Wednesday",
        L"Thursday", L"Friday", L"Shabbos"
    };
    pDC->SelectObject(&fontHdr);
    pDC->SetTextColor(RGB(255, 255, 255));
    for (int c = 0; c < 7; c++)
    {
        CRect rDN(colX[c], dayHdrY, colX[c + 1], dayHdrY + dayHdrH);
        pDC->DrawText(kDays[c], -1, rDN, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // === Calendar cells =====================================================
    int gridTop  = dayHdrY + dayHdrH;
    int availH   = H - (gridTop - y0);
    int gridH    = opts.includeZmanim ? (availH * 56 / 100) : availH;
    int cellH    = gridH / 6;

    DayOfWeek dow0     = GetDayOfWeek(gFirst);
    long      jdn0     = GregorianToJDN(gFirst) - (int)dow0;
    int       lineH    = -fSmall + 2;
    int       mg       = max(2, W / 250);

    bool isIsrael   = pFrame ? pFrame->m_isIsrael    : false;
    bool showMoadim = pFrame ? pFrame->m_showMoadim  : true;
    bool showPar    = pFrame ? pFrame->m_showParshios: true;
    GregorianDate today = pFrame ? pFrame->m_today : GregorianDate{};

    for (int cell = 0; cell < 42; cell++)
    {
        long         jdn = jdn0 + cell;
        GregorianDate g  = JDNToGregorian(jdn);
        HebrewDate    h  = JDNToHebrew(jdn);
        bool cur = (g.month == month && g.year == year);

        int row = cell / 7, col = cell % 7;
        CRect rCell(colX[col],     gridTop + row * cellH,
                    colX[col + 1], gridTop + (row + 1) * cellH);

        // --- Background ---
        COLORREF bg = cur ? RGB(255, 255, 255) : RGB(245, 245, 245);
        if (cur)
        {
            if (g.year  == today.year &&
                g.month == today.month &&
                g.day   == today.day)
            {
                bg = CLR_TODAY;
            }
            else
            {
                auto hols = GetHolidays(h, isIsrael);
                bool set = false;
                for (auto& ho : hols) if (!set && (ho.flags & HOLIDAY_YOM_TOV))      { bg = CLR_YOM_TOV;     set = true; }
                for (auto& ho : hols) if (!set && (ho.flags & HOLIDAY_ROSH_CHODESH)) { bg = CLR_ROSH_CHODESH; set = true; }
                for (auto& ho : hols) if (!set && (ho.flags & HOLIDAY_CHOL_HAMOED))  { bg = CLR_CHOL_HAMOED;  set = true; }
                for (auto& ho : hols) if (!set && (ho.flags & HOLIDAY_FAST))         { bg = CLR_FAST_DAY;     set = true; }
                if (!set && GetDayOfWeek(g) == SHABBAT) bg = CLR_SHABBOS;
            }
        }
        pDC->FillSolidRect(rCell, bg);

        pDC->SetBkMode(TRANSPARENT);

        // --- Gregorian day number (top-left, bold) ---
        pDC->SelectObject(&fontDay);
        pDC->SetTextColor(cur ? CLR_GREG_TXT : RGB(180, 180, 180));
        CString dn; dn.Format(L"%d", g.day);
        CRect rGD(rCell.left + mg, rCell.top + mg,
                  rCell.left + mg + cellH / 3, rCell.top + mg - fDay + 2);
        pDC->DrawText(dn, rGD, DT_LEFT | DT_TOP);

        // --- Hebrew date (top-right, small blue) ---
        bool leap = IsHebrewLeapYear(h.year);
        std::wstring hDay = std::to_wstring(h.day) + L" "
                          + HebrewMonthName(h.month, leap);
        pDC->SelectObject(&fontSmall);
        pDC->SetTextColor(cur ? CLR_HEBREW_TXT : RGB(180, 180, 200));
        CRect rHD(rCell.left + mg, rCell.top + mg,
                  rCell.right - mg, rCell.top + mg + lineH);
        pDC->DrawText(hDay.c_str(), -1, rHD, DT_RIGHT | DT_TOP);

        // --- Holidays, parasha, omer ---
        if (cur)
        {
            auto hols = GetHolidays(h, isIsrael);
            OmerInfo omer = GetOmer(h);
            int yOff   = rCell.top + mg - fDay + 4;
            int maxLn  = max(1, (rCell.Height() - mg * 2 + fDay - 4) / lineH);
            int lines  = 0;

            if (showMoadim)
            {
                for (auto& ho : hols)
                {
                    if (lines >= maxLn) break;
                    if (ho.flags & (HOLIDAY_SHABBOS_MEVAR | HOLIDAY_SPECIAL_SHAB)) continue;
                    std::wstring txt = ho.name;
                    if (!ho.subtitle.empty()) txt += L" - " + ho.subtitle;
                    pDC->SetTextColor(CLR_HOLIDAY_TXT);
                    CRect rHol(rCell.left + mg, yOff, rCell.right - mg, yOff + lineH);
                    pDC->DrawText(txt.c_str(), -1, rHol,
                        DT_LEFT | DT_TOP | DT_END_ELLIPSIS | DT_SINGLELINE);
                    yOff += lineH; lines++;
                }
            }
            if (showPar && GetDayOfWeek(g) == SHABBAT && lines < maxLn)
            {
                ParashaInfo par = GetParasha(h, isIsrael);
                if (!par.name.empty())
                {
                    std::wstring pt = par.name;
                    if (par.isCombined && !par.name2.empty())
                        pt += L" - " + par.name2;
                    pDC->SetTextColor(RGB(30, 80, 160));
                    CRect rP(rCell.left + mg, yOff, rCell.right - mg, yOff + lineH);
                    pDC->DrawText(pt.c_str(), -1, rP,
                        DT_LEFT | DT_TOP | DT_END_ELLIPSIS | DT_SINGLELINE);
                    yOff += lineH; lines++;
                }
            }
            if (omer.isOmerDay && lines < maxLn)
            {
                CString ot; ot.Format(L"Day %d Omer", omer.day);
                pDC->SetTextColor(RGB(100, 80, 150));
                CRect rO(rCell.left + mg, yOff, rCell.right - mg, yOff + lineH);
                pDC->DrawText(ot, rO, DT_LEFT | DT_TOP | DT_SINGLELINE);
                yOff += lineH; lines++;
            }

            // User (web calendar) events
            if (pFrame && lines < maxLn)
            {
                auto userEvts = pFrame->GetUserEventsForDate(g);
                for (auto& ev : userEvts)
                {
                    if (lines >= maxLn) break;
                    pDC->SetTextColor(RGB(120, 30, 120));
                    CRect rEv(rCell.left + mg, yOff, rCell.right - mg, yOff + lineH);
                    pDC->DrawText(ev.c_str(), -1, rEv,
                        DT_LEFT | DT_TOP | DT_END_ELLIPSIS | DT_SINGLELINE);
                    yOff += lineH; lines++;
                }
            }

            // Zmanim labels (candle / motz / chatzos)
            if (pFrame)
            {
                auto [candleStr, motzStr] = pFrame->GetCellZmanimLabels(g, h, hols);
                if (!candleStr.empty() && lines < maxLn)
                {
                    pDC->SetTextColor(RGB(180, 80, 0));
                    CRect rZ(rCell.left + mg, yOff, rCell.right - mg, yOff + lineH);
                    pDC->DrawText(candleStr.c_str(), -1, rZ,
                        DT_LEFT | DT_TOP | DT_SINGLELINE);
                    yOff += lineH; lines++;
                }
                if (!motzStr.empty() && lines < maxLn)
                {
                    pDC->SetTextColor(RGB(0, 80, 160));
                    CRect rZ(rCell.left + mg, yOff, rCell.right - mg, yOff + lineH);
                    pDC->DrawText(motzStr.c_str(), -1, rZ,
                        DT_LEFT | DT_TOP | DT_SINGLELINE);
                }
            }
        }
    }

    // === Grid lines =========================================================
    int penW = max(1, W / 500);
    CPen penGrid(PS_SOLID, penW, CLR_GRID);
    CPen* pOld = pDC->SelectObject(&penGrid);
    for (int r = 0; r <= 6; r++)
    {
        int gy = gridTop + r * cellH;
        pDC->MoveTo(x0, gy);
        pDC->LineTo(x0 + W, gy);
    }
    for (int c = 0; c <= 7; c++)
    {
        pDC->MoveTo(colX[c], gridTop);
        pDC->LineTo(colX[c], gridTop + 6 * cellH);
    }
    pDC->SelectObject(pOld);

    // === Weekly Zmanim Table ================================================
    if (opts.includeZmanim && pFrame)
    {
        int tblTop = gridTop + 6 * cellH;
        int tblH   = y0 + H - tblTop;

        if (tblH >= 40)
        {
            // Fonts for table
            int fTbl = -(tblH * 8 / 100);
            if (fTbl > -4) fTbl = -4;
            CFont fontTbl;
            fontTbl.CreateFont(fTbl, 0,0,0, FW_NORMAL,0,0,0, DEFAULT_CHARSET,0,0,
                CLEARTYPE_QUALITY, DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
            CFont fontTblB;
            fontTblB.CreateFont(fTbl, 0,0,0, FW_BOLD,0,0,0, DEFAULT_CHARSET,0,0,
                CLEARTYPE_QUALITY, DEFAULT_PITCH|FF_SWISS, L"Segoe UI");

            // Column layout: 1 label column + 15 data columns
            static const wchar_t* kColHdr[16] = {
                L"Week / Parasha",
                L"Alos", L"Mishey.", L"Netz",
                L"Shma MA", L"Shma GRA",
                L"Tfla MA", L"Tfla GRA",
                L"Chatzos", L"Mn.Gd.", L"Mn.Kt.",
                L"Plag", L"Candle", L"Shkia", L"Tzais", L"Sha’a"
            };
            int lblW  = W * 21 / 100;
            int timeW = (W - lblW) / 15;
            int tblColX[17];
            tblColX[0] = x0;
            tblColX[1] = x0 + lblW;
            for (int c = 2; c <= 15; c++) tblColX[c] = tblColX[c-1] + timeW;
            tblColX[16] = x0 + W;

            // Title row (10%)
            int titleRowH = max(6, tblH * 10 / 100);
            pDC->FillSolidRect(CRect(x0, tblTop, x0+W, tblTop+titleRowH), RGB(50,80,140));
            pDC->SetBkMode(TRANSPARENT);
            pDC->SetTextColor(RGB(255,255,255));
            pDC->SelectObject(&fontTblB);
            pDC->DrawText(L"Weekly Zmanim",
                CRect(x0+4, tblTop, x0+W, tblTop+titleRowH),
                DT_LEFT|DT_VCENTER|DT_SINGLELINE);

            // Header row (14%)
            int hdrRowH = max(6, tblH * 14 / 100);
            int hdrRowY = tblTop + titleRowH;
            pDC->FillSolidRect(CRect(x0, hdrRowY, x0+W, hdrRowY+hdrRowH), RGB(70,100,160));
            pDC->SetTextColor(RGB(255,255,255));
            pDC->SelectObject(&fontTbl);
            for (int c = 0; c < 16; c++)
            {
                int cx = tblColX[c], cw = tblColX[c+1] - tblColX[c];
                pDC->DrawText(kColHdr[c], -1, CRect(cx, hdrRowY, cx+cw, hdrRowY+hdrRowH),
                    DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            }

            // Data rows
            int dataY    = hdrRowY + hdrRowH;
            int dataRowH = max(4, (y0 + H - dataY) / 6);

            for (int w = 0; w < 6; w++)
            {
                int ry = dataY + w * dataRowH;
                COLORREF rowBg = (w % 2 == 0) ? RGB(248,250,255) : RGB(235,242,255);
                pDC->FillSolidRect(CRect(x0, ry, x0+W, ry+dataRowH), rowBg);
                pDC->SetBkMode(TRANSPARENT);

                // Shabbos for this row (col 6); candle lighting from Friday (col 5)
                long         shabJDN = jdn0 + w * 7 + 6;
                long         friJDN  = jdn0 + w * 7 + 5;
                GregorianDate shab   = JDNToGregorian(shabJDN);
                GregorianDate fri    = JDNToGregorian(friJDN);
                HebrewDate    hShab  = JDNToHebrew(shabJDN);

                ParashaInfo par = GetParasha(hShab, isIsrael);
                CString lbl;
                lbl.Format(L"Shab %d/%d", shab.month, shab.day);
                if (!par.name.empty())
                {
                    lbl += L"  ";
                    lbl += par.name.c_str();
                    if (par.isCombined && !par.name2.empty())
                    { lbl += L" - "; lbl += par.name2.c_str(); }
                }
                pDC->SelectObject(&fontTbl);
                pDC->SetTextColor(RGB(30,30,80));
                pDC->DrawText(lbl,
                    CRect(x0+2, ry, x0+lblW-2, ry+dataRowH),
                    DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);

                // Shabbos zmanim (most times from Saturday)
                bool         shabDst = IsDST(shab, pFrame->m_location);
                ZmanimResult z       = CalculateZmanim(shab, pFrame->m_location, shabDst);

                // Candle lighting: Friday shkia minus offset
                bool         friDst  = IsDST(fri, pFrame->m_location);
                ZmanimResult zFri    = CalculateZmanim(fri, pFrame->m_location, friDst);
                zFri.candleLighting  = AddMinutes(zFri.shkia,
                                           -theApp.m_settings.candleLightingMinutes);

                const TimeOfDay* kT[14] = {
                    &z.alot_GRA,         &z.misheyakir_11,    &z.hanetz,
                    &z.sofShema_MA72,    &z.sofShema_GRA,
                    &z.sofTefilla_MA72,  &z.sofTefilla_GRA,
                    &z.chatzot,          &z.minchaGedola_GRA, &z.minchaKetana_GRA,
                    &z.plagMincha_GRA,   &zFri.candleLighting,  // Friday candle time
                    &z.shkia,            &z.tzeit_GRA
                };
                pDC->SetTextColor(RGB(20,40,80));
                for (int c = 0; c < 14; c++)
                {
                    if (!kT[c]->IsValid()) continue;
                    std::wstring ts = FormatTime(*kT[c], true);
                    int cx = tblColX[c+1], cw = tblColX[c+2] - tblColX[c+1];
                    pDC->DrawText(ts.c_str(), -1, CRect(cx, ry, cx+cw, ry+dataRowH),
                        DT_CENTER|DT_VCENTER|DT_SINGLELINE);
                }
                if (z.shaahZmanit_GRA > 0.0)
                {
                    int szMin = (int)round(z.shaahZmanit_GRA);
                    CString szs; szs.Format(L"%d:%02d", szMin/60, szMin%60);
                    pDC->DrawText(szs,
                        CRect(tblColX[15], ry, tblColX[16], ry+dataRowH),
                        DT_CENTER|DT_VCENTER|DT_SINGLELINE);
                }
            }

            // Table grid lines
            CPen penTbl(PS_SOLID, max(1,W/800), RGB(170,170,180));
            CPen* pOldT = pDC->SelectObject(&penTbl);
            pDC->MoveTo(x0, tblTop);  pDC->LineTo(x0+W, tblTop);
            pDC->MoveTo(x0, hdrRowY); pDC->LineTo(x0+W, hdrRowY);
            for (int r = 0; r <= 6; r++)
            {
                int gy = dataY + r * dataRowH;
                pDC->MoveTo(x0, gy); pDC->LineTo(x0+W, gy);
            }
            for (int c = 0; c <= 16; c++)
            {
                pDC->MoveTo(tblColX[c], tblTop);
                pDC->LineTo(tblColX[c], dataY + 6*dataRowH);
            }
            pDC->SelectObject(pOldT);
        }
    }

    // Footer
    int fFoot = -(H * 16 / 1000); if (fFoot > -4) fFoot = -4;
    CFont fontFoot;
    fontFoot.CreateFont(fFoot,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
    pDC->SelectObject(&fontFoot);
    pDC->SetTextColor(RGB(140,140,140));
    pDC->SetBkMode(TRANSPARENT);
    int footH = -fFoot + 2;
    pDC->FillSolidRect(CRect(x0, y0+H-footH, x0+W, y0+H), RGB(245, 245, 245));
    pDC->DrawText(L"© 2026 WinLuach  https://github.com/akivacp/WinLuach/  MIT License",
        CRect(x0, y0+H-footH, x0+W, y0+H), DT_CENTER|DT_BOTTOM|DT_SINGLELINE);
}

// =============================================================================
// BUILD PAGE LIST
// =============================================================================

std::vector<std::pair<int, int>> BuildPageList(
    const CalPrintOptions& opts, int viewYear, int viewMonth)
{
    std::vector<std::pair<int, int>> pages;
    switch (opts.range)
    {
    case CalPrintOptions::RANGE_MONTH:
        pages.push_back({ viewYear, viewMonth });
        break;
    case CalPrintOptions::RANGE_YEAR:
        for (int m = 1; m <= 12; m++)
            pages.push_back({ viewYear, m });
        break;
    case CalPrintOptions::RANGE_12:
        for (int i = 0; i < 12; i++)
        {
            int m = viewMonth + i, y = viewYear;
            while (m > 12) { m -= 12; y++; }
            pages.push_back({ y, m });
        }
        break;
    }
    return pages;
}

// =============================================================================
// DO PRINT
// Shows CPrintDialog (standard Windows print dialog) and prints.
// =============================================================================

bool DoPrint(const CalPrintOptions& opts, CMainFrame* pFrame)
{
    CPrintDialog pd(FALSE);
    pd.m_pd.Flags |= PD_RETURNDC | PD_NOPAGENUMS | PD_NOCURRENTPAGE;

    // Pre-apply orientation into a DEVMODE so the dialog inherits it
    HGLOBAL hDM = GlobalAlloc(GHND, sizeof(DEVMODE));
    if (hDM)
    {
        DEVMODE* dm = (DEVMODE*)GlobalLock(hDM);
        if (dm)
        {
            ZeroMemory(dm, sizeof(DEVMODE));
            dm->dmSize       = sizeof(DEVMODE);
            dm->dmFields     = DM_ORIENTATION;
            dm->dmOrientation = opts.landscape ? DMORIENT_LANDSCAPE : DMORIENT_PORTRAIT;
            GlobalUnlock(hDM);
        }
        pd.m_pd.hDevMode = hDM;
    }

    if (pd.DoModal() != IDOK)
    {
        if (hDM) GlobalFree(hDM);
        return false;
    }

    HDC hDC = pd.GetPrinterDC();
    if (!hDC)
    {
        if (hDM) GlobalFree(hDM);
        return false;
    }

    CDC dc;
    dc.Attach(hDC);

    int dpiX  = dc.GetDeviceCaps(LOGPIXELSX);
    int dpiY  = dc.GetDeviceCaps(LOGPIXELSY);
    int pageW = dc.GetDeviceCaps(HORZRES);
    int pageH = dc.GetDeviceCaps(VERTRES);

    int mL = (int)(opts.mLeft  * dpiX);
    int mR = (int)(opts.mRight * dpiX);
    int mT = (int)(opts.mTop   * dpiY);
    int mB = (int)(opts.mBot   * dpiY);

    CRect rcContent(mL, mT, pageW - mR, pageH - mB);
    if (rcContent.IsRectEmpty()) rcContent = CRect(0, 0, pageW, pageH);

    DOCINFO di  = { sizeof(DOCINFO) };
    di.lpszDocName = L"WinLuach Calendar";

    auto pages = BuildPageList(opts,
        pFrame->m_viewYear, pFrame->m_viewMonth);

    dc.StartDoc(&di);
    for (auto& [yr, mo] : pages)
    {
        dc.StartPage();
        DrawCalMonthPage(&dc, rcContent, yr, mo, pFrame, opts);
        dc.EndPage();
    }
    dc.EndDoc();

    dc.Detach();
    ::DeleteDC(hDC);
    if (hDM) GlobalFree(hDM);
    return true;
}

// =============================================================================
// DRAW ZMANIM MONTH PAGE
// Portrait-oriented daily zmanim table for one calendar month.
// colMask: bitmask selecting which of the 15 standard zmanim columns to show.
// =============================================================================

void DrawZmanimMonthPage(CDC* pDC, const CRect& rcPage,
                         int year, int month, CMainFrame* pFrame,
                         uint32_t colMask, bool use24hr)
{
    const int W  = rcPage.Width();
    const int H  = rcPage.Height();
    const int x0 = rcPage.left;
    const int y0 = rcPage.top;

    pDC->FillSolidRect(rcPage, RGB(255, 255, 255));

    int fTitle = -(H * 4  / 100); if (fTitle > -6) fTitle = -6;
    int fHdr   = -(H * 22 / 1000); if (fHdr  > -4) fHdr   = -4;
    int fData  = -(H * 20 / 1000); if (fData > -4) fData  = -4;

    CFont fontTitle, fontHdr, fontData;
    fontTitle.CreateFont(fTitle,0,0,0,FW_BOLD,  0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
    fontHdr  .CreateFont(fHdr,  0,0,0,FW_BOLD,  0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
    fontData .CreateFont(fData, 0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");

    // ── Title row ─────────────────────────────────────────────────────────────
    int titleH = H * 7 / 100;
    pDC->FillSolidRect(CRect(x0, y0, x0+W, y0+titleH), RGB(50,80,140));
    pDC->SetBkMode(TRANSPARENT);
    pDC->SetTextColor(RGB(255,255,255));
    pDC->SelectObject(&fontTitle);

    GregorianDate gFirst(year, month, 1);
    GregorianDate gLast (year, month, DaysInGregorianMonth(month, year));
    HebrewDate    hFirst = GregorianToHebrew(gFirst);
    HebrewDate    hLast  = GregorianToHebrew(gLast);

    CString gregStr;
    gregStr.Format(L"Zmanim - %s  %d", GregorianMonthName(month).c_str(), year);

    std::wstring hs = HebrewMonthName(hFirst.month, IsHebrewLeapYear(hFirst.year));
    if (hFirst.month != hLast.month)
        hs += L" - " + HebrewMonthName(hLast.month, IsHebrewLeapYear(hLast.year));
    CString hebStr; hebStr.Format(L"%s  %d", hs.c_str(), hFirst.year);
    if (hFirst.year != hLast.year) hebStr.AppendFormat(L" - %d", hLast.year);

    std::wstring locName = pFrame ? pFrame->m_location.name : L"";

    pDC->DrawText(gregStr, CRect(x0+8, y0, x0+W*40/100, y0+titleH), DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    pDC->DrawText(hebStr,  CRect(x0+W*40/100, y0, x0+W*72/100, y0+titleH), DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    pDC->DrawText(locName.c_str(), -1, CRect(x0+W*72/100, y0, x0+W-8, y0+titleH), DT_RIGHT|DT_VCENTER|DT_SINGLELINE);

    // ── Build active column list from bitmask ─────────────────────────────────
    static const wchar_t* kColHdr[15] = {
        L"Alos", L"Mishey.", L"Netz",
        L"Shma MA", L"Shma GRA", L"Tfla MA", L"Tfla GRA",
        L"Chatzos", L"Mn.Gd.", L"Mn.Kt.",
        L"Plag", L"Candle", L"Shkia", L"Tzais", L"Sha'a"
    };
    std::vector<int> activeCols;
    for (int i = 0; i < 15; i++)
        if (colMask & (1u << i)) activeCols.push_back(i);
    int nc = (int)activeCols.size();

    // ── Column geometry: Date | Day | Hebrew | [time cols...] ─────────────────
    int dateW = W * 7 / 100;
    int dayW  = W * 5 / 100;
    int hebW  = W * 9 / 100;
    int timeW = nc > 0 ? (W - dateW - dayW - hebW) / nc : 1;

    // colX[0..3+nc]: colX[c] = left edge of column c; colX[c+1] = right edge
    // cols 0,1,2 = Date, Day, Hebrew; cols 3..3+nc-1 = time data
    std::vector<int> colX(3 + nc + 1);
    colX[0] = x0;
    colX[1] = x0 + dateW;
    colX[2] = x0 + dateW + dayW;
    colX[3] = x0 + dateW + dayW + hebW;
    for (int c = 1; c <= nc; c++)  colX[3+c] = colX[3] + c * timeW;
    colX[3+nc] = x0 + W;  // stretch last column to right edge

    // ── Header row ────────────────────────────────────────────────────────────
    int hdrH = H * 5 / 100;
    int hdrY = y0 + titleH;
    pDC->FillSolidRect(CRect(x0, hdrY, x0+W, hdrY+hdrH), RGB(70,100,160));
    pDC->SetTextColor(RGB(255,255,255));
    pDC->SelectObject(&fontHdr);

    auto drawHdr = [&](int ci, const wchar_t* txt) {
        pDC->DrawText(txt, -1, CRect(colX[ci], hdrY, colX[ci+1], hdrY+hdrH),
            DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
    };
    drawHdr(0, L"Date");
    drawHdr(1, L"Day");
    drawHdr(2, L"Hebrew");
    for (int c = 0; c < nc; c++) drawHdr(3+c, kColHdr[activeCols[c]]);

    // ── Data rows ─────────────────────────────────────────────────────────────
    int dataY    = hdrY + hdrH;
    int daysInMo = DaysInGregorianMonth(month, year);
    int rowH     = max(4, (y0 + H - dataY) / daysInMo);

    static const wchar_t* kDayAbbr[7] = {
        L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };
    bool isIsrael = pFrame ? pFrame->m_isIsrael : false;

    for (int d = 1; d <= daysInMo; d++)
    {
        GregorianDate g(year, month, d);
        HebrewDate    h  = GregorianToHebrew(g);
        DayOfWeek     dow = GetDayOfWeek(g);
        int           ry  = dataY + (d-1) * rowH;

        // Holiday shading (same priority as main calendar)
        auto hols2 = pFrame ? GetHolidays(h, isIsrael) : std::vector<HolidayInfo>{};
        auto hasFlag2 = [&](int flag) {
            for (const auto& ho : hols2) if (ho.flags & flag) return true;
            return false;
        };
        COLORREF bg = (d % 2 == 0) ? RGB(248,250,255) : RGB(255,255,255);
        if (dow == SHABBAT)                        bg = CLR_SHABBOS;
        if (dow == FRIDAY)                         bg = RGB(255,248,220);
        if (hasFlag2(HOLIDAY_ROSH_CHODESH))        bg = CLR_ROSH_CHODESH;
        if (hasFlag2(HOLIDAY_FAST))                bg = CLR_FAST_DAY;
        if (hasFlag2(HOLIDAY_CHOL_HAMOED))         bg = CLR_CHOL_HAMOED;
        if (hasFlag2(HOLIDAY_YOM_TOV))             bg = CLR_YOM_TOV;
        pDC->FillSolidRect(CRect(x0, ry, x0+W, ry+rowH), bg);
        pDC->SetBkMode(TRANSPARENT);

        bool         dst = pFrame ? IsDST(g, pFrame->m_location) : false;
        ZmanimResult z   = pFrame ? CalculateZmanim(g, pFrame->m_location, dst) : ZmanimResult{};
        // Candle lighting: Friday, erev YT, erev Yom Kippur, and diaspora 1st-day YT
        if (pFrame)
        {
            z.candleLighting = AddMinutes(z.shkia, -theApp.m_settings.candleLightingMinutes);
            bool erevYK      = (h.month == TISHREI && h.day == 9);
            bool hasErev2    = hasFlag2(HOLIDAY_EREV);
            // Diaspora 1st-day YT: next day is also YT → light at tzeis
            HebrewDate hNext2 = JDNToHebrew(GregorianToJDN(g) + 1);
            auto holsN        = GetHolidays(hNext2, isIsrael);
            bool nextIsYT2    = false;
            for (const auto& ho : holsN) if (ho.flags & HOLIDAY_YOM_TOV) { nextIsYT2 = true; break; }

            bool showCandle = (dow == FRIDAY)
                || (!isIsrael && hasFlag2(HOLIDAY_YOM_TOV) && nextIsYT2)
                || (dow != FRIDAY && dow != SHABBAT && (hasErev2 || erevYK));
            if (!showCandle)
                z.candleLighting = TimeOfDay{};
            else if (!isIsrael && hasFlag2(HOLIDAY_YOM_TOV) && nextIsYT2 && dow != FRIDAY)
                z.candleLighting = z.tzeit_GRA;  // light at tzeis for 2nd day
        }
        else
            z.candleLighting = TimeOfDay{};

        pDC->SelectObject(&fontData);

        // Date
        CString ds; ds.Format(L"%d/%d", month, d);
        pDC->SetTextColor(RGB(30,30,30));
        pDC->DrawText(ds, CRect(colX[0], ry, colX[1], ry+rowH), DT_CENTER|DT_VCENTER|DT_SINGLELINE);

        // Day of week
        pDC->SetTextColor(dow == SHABBAT ? RGB(0,120,0) : RGB(50,50,100));
        pDC->DrawText(kDayAbbr[(int)dow], -1, CRect(colX[1], ry, colX[2], ry+rowH),
            DT_CENTER|DT_VCENTER|DT_SINGLELINE);

        // Hebrew date
        bool         leap     = IsHebrewLeapYear(h.year);
        std::wstring hebDayStr = std::to_wstring(h.day) + L" " +
            HebrewMonthName(h.month, leap).substr(0, 4);
        pDC->SetTextColor(RGB(20,20,160));
        pDC->DrawText(hebDayStr.c_str(), -1, CRect(colX[2], ry, colX[3], ry+rowH),
            DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);

        // Time columns
        const TimeOfDay* kTimes[14] = {
            &z.alot_GRA,        &z.misheyakir_11,   &z.hanetz,
            &z.sofShema_MA72,   &z.sofShema_GRA,
            &z.sofTefilla_MA72, &z.sofTefilla_GRA,
            &z.chatzot,         &z.minchaGedola_GRA,&z.minchaKetana_GRA,
            &z.plagMincha_GRA,  &z.candleLighting,
            &z.shkia,           &z.tzeit_GRA
        };
        pDC->SetTextColor(RGB(20,40,100));
        for (int c = 0; c < nc; c++)
        {
            int ci = activeCols[c];
            std::wstring ts;
            if (ci == 14)
            {
                if (z.shaahZmanit_GRA > 0.0) {
                    int szMin = (int)round(z.shaahZmanit_GRA);
                    CString ss; ss.Format(L"%d:%02d", szMin/60, szMin%60);
                    ts = (LPCWSTR)ss;
                }
            }
            else if (ci < 14 && kTimes[ci]->IsValid())
            {
                ts = FormatTime(*kTimes[ci], use24hr);
            }
            if (!ts.empty())
                pDC->DrawText(ts.c_str(), -1, CRect(colX[3+c], ry, colX[3+c+1], ry+rowH),
                    DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        }
    }

    // ── Grid lines ────────────────────────────────────────────────────────────
    CPen penGrid(PS_SOLID, max(1,W/600), RGB(180,180,200));
    CPen* pOld = pDC->SelectObject(&penGrid);
    pDC->MoveTo(x0, hdrY); pDC->LineTo(x0+W, hdrY);
    for (int d = 0; d <= daysInMo; d++) {
        int gy = dataY + d * rowH;
        pDC->MoveTo(x0, gy); pDC->LineTo(x0+W, gy);
    }
    for (int c = 0; c <= 3+nc; c++) {
        pDC->MoveTo(colX[c], hdrY);
        pDC->LineTo(colX[c], dataY + daysInMo * rowH);
    }
    pDC->SelectObject(pOld);

    // Footer
    int fFoot2 = -(H * 16 / 1000); if (fFoot2 > -4) fFoot2 = -4;
    CFont fontFoot2;
    fontFoot2.CreateFont(fFoot2,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
    pDC->SelectObject(&fontFoot2);
    pDC->SetTextColor(RGB(140,140,140));
    pDC->SetBkMode(TRANSPARENT);
    int footH2 = -fFoot2 + 2;
    pDC->FillSolidRect(CRect(x0, y0+H-footH2, x0+W, y0+H), RGB(245, 245, 245));
    pDC->DrawText(L"© 2026 WinLuach  https://github.com/akivacp/WinLuach/  MIT License",
        CRect(x0, y0+H-footH2, x0+W, y0+H), DT_CENTER|DT_BOTTOM|DT_SINGLELINE);
}

// =============================================================================
// DRAW DAY PAGE
// Portrait-oriented single-day details page (holidays, learning, zmanim).
// =============================================================================

void DrawDayPage(CDC* pDC, const CRect& rcPage,
                 const GregorianDate& g, CMainFrame* pFrame)
{
    const int W  = rcPage.Width();
    const int H  = rcPage.Height();
    const int x0 = rcPage.left;
    const int y0 = rcPage.top;

    pDC->FillSolidRect(rcPage, RGB(255, 255, 255));

    HebrewDate   h   = GregorianToHebrew(g);
    bool         dst = pFrame ? IsDST(g, pFrame->m_location) : false;
    ZmanimResult z   = pFrame ? CalculateZmanim(g, pFrame->m_location, dst) : ZmanimResult{};
    bool         u24 = pFrame ? pFrame->m_use24hr : false;

    std::vector<HolidayInfo>  hols;
    OmerInfo                  omer  = {};
    ParashaInfo               par   = {};
    DailyLearning             learn = {};
    std::vector<std::wstring> userEvts;

    if (pFrame) {
        hols     = GetHolidays(h, pFrame->m_isIsrael);
        omer     = GetOmer(h);
        learn    = GetDailyLearning(h, g);
        if (GetDayOfWeek(g) == SHABBAT) par = GetParasha(h, pFrame->m_isIsrael);
        userEvts = pFrame->GetUserEventsForDate(g);
    }

    int fTitle = -(H * 4 / 100); if (fTitle > -8) fTitle = -8;
    int fSec   = -(H * 26/1000); if (fSec   > -6) fSec   = -6;
    int fRow   = -(H * 22/1000); if (fRow   > -5) fRow   = -5;

    CFont fontTitle, fontSec, fontRow;
    fontTitle.CreateFont(fTitle,0,0,0,FW_BOLD,  0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
    fontSec  .CreateFont(fSec,  0,0,0,FW_BOLD,  0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
    fontRow  .CreateFont(fRow,  0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");

    // Header band
    int hdrH = H * 8 / 100;
    pDC->FillSolidRect(CRect(x0, y0, x0+W, y0+hdrH), RGB(50,80,140));
    pDC->SetBkMode(TRANSPARENT);
    pDC->SetTextColor(RGB(255,255,255));
    pDC->SelectObject(&fontTitle);

    static const wchar_t* kDow[] = {
        L"Sunday",L"Monday",L"Tuesday",L"Wednesday",L"Thursday",L"Friday",L"Shabbos"};
    CString gregStr;
    gregStr.Format(L"%s,  %s %d,  %d",
        kDow[(int)GetDayOfWeek(g)],
        GregorianMonthName(g.month).c_str(), g.day, g.year);
    pDC->DrawText(gregStr, CRect(x0+8, y0, x0+W-8, y0+hdrH/2+4), DT_LEFT|DT_VCENTER|DT_SINGLELINE);

    bool   leap   = IsHebrewLeapYear(h.year);
    CString hebStr;
    hebStr.Format(L"%d %s %d", h.day, HebrewMonthName(h.month, leap).c_str(), h.year);
    pDC->SetTextColor(RGB(200,220,255));
    pDC->DrawText(hebStr, CRect(x0+8, y0+hdrH/2, x0+W-8, y0+hdrH-4), DT_RIGHT|DT_VCENTER|DT_SINGLELINE);

    int y    = y0 + hdrH + 4;
    int rowH = -fRow + 4;
    int secH = -fSec + 4;
    int lblW = W * 28 / 100;

    auto drawSection = [&](const wchar_t* lbl) {
        pDC->FillSolidRect(CRect(x0, y, x0+W, y+secH), RGB(200,212,240));
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(RGB(30,50,110));
        pDC->SelectObject(&fontSec);
        pDC->DrawText(lbl, CRect(x0+6, y, x0+W-4, y+secH), DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        y += secH + 1;
    };
    auto drawRow = [&](const wchar_t* lbl, const std::wstring& val, COLORREF clr) {
        if (y + rowH > y0 + H) return;
        pDC->SetBkMode(TRANSPARENT);
        pDC->SelectObject(&fontRow);
        pDC->SetTextColor(RGB(110,110,110));
        pDC->DrawText(lbl, CRect(x0+8, y, x0+8+lblW, y+rowH), DT_LEFT|DT_TOP|DT_SINGLELINE);
        pDC->SetTextColor(clr);
        pDC->DrawText(val.c_str(), -1, CRect(x0+8+lblW, y, x0+W-8, y+rowH),
            DT_LEFT|DT_TOP|DT_SINGLELINE|DT_END_ELLIPSIS);
        y += rowH;
    };

    if (!hols.empty() || !par.name.empty() || omer.isOmerDay) {
        drawSection(L"Holidays & Special Days");
        for (auto& ho : hols) {
            if (ho.flags & (HOLIDAY_SHABBOS_MEVAR | HOLIDAY_SPECIAL_SHAB)) continue;
            std::wstring txt = ho.name;
            if (!ho.subtitle.empty()) txt += L" - " + ho.subtitle;
            drawRow(L"", txt, RGB(170,25,25));
        }
        if (!par.name.empty()) {
            std::wstring pt = L"Parshas " + par.name;
            if (par.isCombined && !par.name2.empty()) pt += L" - " + par.name2;
            drawRow(L"Parasha:", pt, RGB(25,70,160));
        }
        if (omer.isOmerDay) {
            CString ot; ot.Format(L"Day %d of the Omer", omer.day);
            drawRow(L"Omer:", (LPCWSTR)ot, RGB(90,50,140));
        }
    }

    if (!userEvts.empty()) {
        drawSection(L"Calendar Events");
        for (auto& ev : userEvts) drawRow(L"", ev, RGB(90,20,110));
    }

    bool anyLearn = !learn.dafYomi.empty() || !learn.yerushalmi.empty() ||
        !learn.halachaYomit.empty() || !learn.mishnaYomit.empty() || !learn.tanachYomi.empty();
    if (anyLearn) {
        drawSection(L"Daily Learning");
        if (!learn.dafYomi.empty())      drawRow(L"Daf Yomi:",      learn.dafYomi,      RGB(0,70,160));
        if (!learn.yerushalmi.empty())   drawRow(L"Yerushalmi:",    learn.yerushalmi,   RGB(0,100,100));
        if (!learn.halachaYomit.empty()) drawRow(L"Halacha Yomit:", learn.halachaYomit, RGB(90,50,0));
        if (!learn.mishnaYomit.empty())  drawRow(L"Mishna Yomit:",  learn.mishnaYomit,  RGB(0,90,0));
        if (!learn.tanachYomi.empty())   drawRow(L"Tanach Yomi:",   learn.tanachYomi,   RGB(70,0,70));
    }

    drawSection(L"Zmanim");
    auto zRow = [&](const wchar_t* lbl, const TimeOfDay& t) {
        if (t.IsValid()) drawRow(lbl, FormatTime(t, u24), RGB(20,60,20));
    };
    zRow(L"Alot HaShachar:",    z.alot_GRA);
    zRow(L"Misheyakir:",         z.misheyakir_11);
    zRow(L"Hanetz (Netz):",      z.hanetz);
    zRow(L"Sof Shema (MA):",     z.sofShema_MA72);
    zRow(L"Sof Shema (GRA):",    z.sofShema_GRA);
    zRow(L"Sof Tefilla (MA):",   z.sofTefilla_MA72);
    zRow(L"Sof Tefilla (GRA):",  z.sofTefilla_GRA);
    zRow(L"Chatzot:",            z.chatzot);
    zRow(L"Mincha Gedola:",      z.minchaGedola_GRA);
    zRow(L"Mincha Ketana:",      z.minchaKetana_GRA);
    zRow(L"Plag HaMincha:",      z.plagMincha_GRA);
    if (z.candleLighting.IsValid()) zRow(L"Candle Lighting:", z.candleLighting);
    zRow(L"Shkia (Sunset):",     z.shkia);
    zRow(L"Tzais HaKochavim:",   z.tzeit_GRA);
    if (y + rowH <= y0+H && z.shaahZmanit_GRA > 0.0) {
        int szMin = (int)round(z.shaahZmanit_GRA);
        CString ss; ss.Format(L"%d:%02d  (%d min)", szMin/60, szMin%60, szMin);
        drawRow(L"Sha'a Zmanit:", (LPCWSTR)ss, RGB(20,60,20));
    }

    // Footer
    int fFoot3 = -(H * 16 / 1000); if (fFoot3 > -4) fFoot3 = -4;
    CFont fontFoot3;
    fontFoot3.CreateFont(fFoot3,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
    pDC->SelectObject(&fontFoot3);
    pDC->SetTextColor(RGB(140,140,140));
    pDC->SetBkMode(TRANSPARENT);
    int footH3 = -fFoot3 + 2;
    pDC->FillSolidRect(CRect(x0, y0+H-footH3, x0+W, y0+H), RGB(245, 245, 245));
    pDC->DrawText(L"© 2026 WinLuach  https://github.com/akivacp/WinLuach/  MIT License",
        CRect(x0, y0+H-footH3, x0+W, y0+H), DT_CENTER|DT_BOTTOM|DT_SINGLELINE);
}

// =============================================================================
// DO PRINT ZMANIM MONTH / DO PRINT DAY
// =============================================================================

bool SimplePrint(const wchar_t* docName, const SimplePageOpts& opts,
    std::function<void(CDC*, const CRect&)> render)
{
    CPrintDialog pd(FALSE);
    pd.m_pd.Flags |= PD_RETURNDC | PD_NOPAGENUMS | PD_NOCURRENTPAGE;

    HGLOBAL hDM = GlobalAlloc(GHND, sizeof(DEVMODE));
    if (hDM) {
        DEVMODE* dm = (DEVMODE*)GlobalLock(hDM);
        if (dm) {
            ZeroMemory(dm, sizeof(DEVMODE));
            dm->dmSize        = sizeof(DEVMODE);
            dm->dmFields      = DM_ORIENTATION;
            dm->dmOrientation = opts.landscape ? DMORIENT_LANDSCAPE : DMORIENT_PORTRAIT;
            GlobalUnlock(hDM);
        }
        pd.m_pd.hDevMode = hDM;
    }
    if (pd.DoModal() != IDOK) { if (hDM) GlobalFree(hDM); return false; }

    HDC hDC = pd.GetPrinterDC();
    if (!hDC) { if (hDM) GlobalFree(hDM); return false; }

    CDC dc; dc.Attach(hDC);
    int dpiX = dc.GetDeviceCaps(LOGPIXELSX);
    int dpiY = dc.GetDeviceCaps(LOGPIXELSY);
    int pageW= dc.GetDeviceCaps(HORZRES);
    int pageH= dc.GetDeviceCaps(VERTRES);
    int mL   = (int)(opts.mLeft  * dpiX), mR = (int)(opts.mRight * dpiX);
    int mT   = (int)(opts.mTop   * dpiY), mB = (int)(opts.mBot   * dpiY);
    CRect rcContent(mL, mT, pageW-mR, pageH-mB);
    if (rcContent.IsRectEmpty()) rcContent = CRect(0, 0, pageW, pageH);

    DOCINFO di = { sizeof(DOCINFO) };
    di.lpszDocName = docName;
    dc.StartDoc(&di);
    dc.StartPage();
    render(&dc, rcContent);
    dc.EndPage();
    dc.EndDoc();
    dc.Detach(); ::DeleteDC(hDC);
    if (hDM) GlobalFree(hDM);
    return true;
}

bool DoPrintZmanimMonth(int year, int month, CMainFrame* pFrame, uint32_t colMask)
{
    bool u24 = pFrame ? pFrame->m_use24hr : true;
    CSimplePageSetupDlg dlg(
        [=](CDC* pDC, const CRect& rc){ DrawZmanimMonthPage(pDC, rc, year, month, pFrame, colMask, u24); },
        L"WinLuach Zmanim", true /* defaultLandscape */, pFrame);
    dlg.DoModal();
    return true;
}

bool DoPrintDay(const GregorianDate& g, CMainFrame* pFrame)
{
    CSimplePageSetupDlg dlg(
        [=](CDC* pDC, const CRect& rc){ DrawDayPage(pDC, rc, g, pFrame); },
        L"WinLuach Day Details", false /* defaultLandscape */, pFrame);
    dlg.DoModal();
    return true;
}

// =============================================================================
// CSimplePageSetupDlg — orientation + margins + preview/print for day/zmanim
// =============================================================================

BEGIN_MESSAGE_MAP(CSimplePageSetupDlg, CDialog)
    ON_BN_CLICKED(CSimplePageSetupDlg::IDC_SPS_BTN_PREVIEW, &CSimplePageSetupDlg::OnPreview)
    ON_BN_CLICKED(IDOK, &CSimplePageSetupDlg::OnPrint)
END_MESSAGE_MAP()

CSimplePageSetupDlg::CSimplePageSetupDlg(
    std::function<void(CDC*, const CRect&)> render,
    const wchar_t* docName, bool defaultLandscape, CWnd* pParent)
    : CDialog(), m_render(std::move(render)), m_docName(docName)
{
    m_opts.landscape = defaultLandscape;
    m_pParentWnd = pParent;
}

INT_PTR CSimplePageSetupDlg::DoModal()
{
    struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[32]; } b = {};
    b.t.style = WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_CENTER;
    b.t.cx = 310; b.t.cy = 220;
    wcscpy_s(b.title, L"Page Setup");
    if (!InitModalIndirect((DLGTEMPLATE*)&b, m_pParentWnd)) return -1;
    return CDialog::DoModal();
}

BOOL CSimplePageSetupDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    SetWindowText(L"Page Setup");
    HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    CFont* pF = CFont::FromHandle(hF);
    CRect rc; GetClientRect(&rc);
    const int W = rc.Width(), H = rc.Height();

    auto mkStatic = [&](const wchar_t* txt, int x, int y, int w, int h) {
        CStatic* s = new CStatic;
        s->Create(txt, WS_CHILD|WS_VISIBLE|SS_LEFT, CRect(x, y, x+w, y+h), this);
        s->SetFont(pF);
    };
    auto mkRadio = [&](CButton& b, const wchar_t* txt, UINT id,
                       int x, int y, int w, bool first, bool checked) {
        DWORD style = WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTORADIOBUTTON;
        if (first) style |= WS_GROUP;
        b.Create(txt, style, CRect(x, y, x+w, y+18), this, id);
        b.SetFont(pF);
        if (checked) b.SetCheck(BST_CHECKED);
    };
    auto mkEdit = [&](CEdit& e, UINT id, const wchar_t* val, int x, int y, int w) {
        e.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_CENTER,
            CRect(x, y, x+w, y+20), this, id);
        e.SetFont(pF);
        e.SetWindowText(val);
    };

    int y = 10;
    mkStatic(L"Orientation", 8, y, 200, 16); y += 20;
    mkRadio(m_radPortrait,  L"Portrait",  IDC_SPS_RAD_PORT, 20,  y, 100, true,  !m_opts.landscape);
    mkRadio(m_radLandscape, L"Landscape", IDC_SPS_RAD_LAND, 140, y, 120, false,  m_opts.landscape);
    y += 28;

    auto fmtM = [](float v) -> CString { CString s; s.Format(L"%.2f", v); return s; };
    mkStatic(L"Margins (inches)", 8, y, 200, 16); y += 20;
    mkStatic(L"Top:",    8,  y+2, 34, 16);
    mkEdit(m_editTop,    IDC_SPS_EDT_TOP,   fmtM(m_opts.mTop),   46,  y, 50);
    mkStatic(L"Bottom:", 115, y+2, 48, 16);
    mkEdit(m_editBot,    IDC_SPS_EDT_BOT,   fmtM(m_opts.mBot),  168, y, 50);
    y += 26;
    mkStatic(L"Left:",   8,  y+2, 34, 16);
    mkEdit(m_editLeft,   IDC_SPS_EDT_LEFT,  fmtM(m_opts.mLeft),  46,  y, 50);
    mkStatic(L"Right:",  115, y+2, 48, 16);
    mkEdit(m_editRight,  IDC_SPS_EDT_RIGHT, fmtM(m_opts.mRight), 168, y, 50);
    y += 30;

    int btnY = H - 36;
    m_btnPreview.Create(L"Preview",
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON|WS_GROUP,
        CRect(10, btnY, 90, btnY+26), this, IDC_SPS_BTN_PREVIEW);
    m_btnPreview.SetFont(pF);

    CButton* pPrint = new CButton;
    pPrint->Create(L"Print...",
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
        CRect(W-170, btnY, W-90, btnY+26), this, IDOK);
    pPrint->SetFont(pF);

    CButton* pCancel = new CButton;
    pCancel->Create(L"Cancel",
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
        CRect(W-82, btnY, W-8, btnY+26), this, IDCANCEL);
    pCancel->SetFont(pF);

    return TRUE;
}

void CSimplePageSetupDlg::ReadControls()
{
    m_opts.landscape = (m_radLandscape.GetCheck() == BST_CHECKED);
    auto getF = [](CEdit& e, float def) {
        CString s; e.GetWindowText(s);
        float v = (float)_wtof(s);
        return (v > 0.0f && v < 5.0f) ? v : def;
    };
    m_opts.mTop   = getF(m_editTop,   0.75f);
    m_opts.mBot   = getF(m_editBot,   0.75f);
    m_opts.mLeft  = getF(m_editLeft,  0.50f);
    m_opts.mRight = getF(m_editRight, 0.50f);
}

void CSimplePageSetupDlg::OnPreview()
{
    ReadControls();
    CSimplePreviewDlg prev(m_render, m_docName.c_str(), !m_opts.landscape, this);
    prev.DoModal();
}

void CSimplePageSetupDlg::OnPrint()
{
    ReadControls();
    SimplePrint(m_docName.c_str(), m_opts, m_render);
    CDialog::OnOK();
}

// =============================================================================
// CSimplePreviewDlg — single-page print preview with Print / Close buttons
// =============================================================================

BEGIN_MESSAGE_MAP(CSimplePreviewDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_BN_CLICKED(CSimplePreviewDlg::IDC_SP_PRINT, &CSimplePreviewDlg::OnPrint)
END_MESSAGE_MAP()

CSimplePreviewDlg::CSimplePreviewDlg(
    std::function<void(CDC*, const CRect&)> render,
    const wchar_t* docName, bool portrait, CWnd* pParent)
    : CDialog(), m_render(std::move(render)), m_docName(docName), m_portrait(portrait)
{
    m_pParentWnd = pParent;
}

INT_PTR CSimplePreviewDlg::DoModal()
{
    struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[40]; } b = {};
    b.t.style = WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|DS_CENTER;
    b.t.cx = 520; b.t.cy = 420;
    wcscpy_s(b.title, L"Print Preview");
    if (!InitModalIndirect((DLGTEMPLATE*)&b, m_pParentWnd)) return -1;
    return CDialog::DoModal();
}

BOOL CSimplePreviewDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    SetWindowText(L"Print Preview");
    HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    CFont* pF = CFont::FromHandle(hF);
    CRect rc; GetClientRect(&rc);
    int W = rc.Width(), H = rc.Height();
    int bh = 32, bw = 80, margin = 8;
    m_btnPrint.Create(L"Print...", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
        CRect(W/2 - bw - margin/2, H - bh - margin, W/2 - margin/2, H - margin),
        this, IDC_SP_PRINT);
    m_btnPrint.SetFont(pF);
    m_btnClose.Create(L"Close", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
        CRect(W/2 + margin/2, H - bh - margin, W/2 + bw + margin/2, H - margin),
        this, IDCANCEL);
    m_btnClose.SetFont(pF);
    return TRUE;
}

void CSimplePreviewDlg::OnPaint()
{
    CPaintDC dc(this);
    CRect rc; GetClientRect(&rc);

    const int BTN_STRIP = 44;
    CRect previewRc(0, 0, rc.right, rc.bottom - BTN_STRIP);

    // Grey background
    dc.FillSolidRect(previewRc, RGB(100, 100, 100));

    // Compute page rectangle (preserve aspect ratio)
    float aspect = m_portrait ? (11.0f / 8.5f) : (8.5f / 11.0f);
    int avW = previewRc.Width()  - 20;
    int avH = previewRc.Height() - 20;
    int pageW, pageH;
    if ((float)avH / avW > aspect) { pageW = avW; pageH = (int)(avW * aspect); }
    else                           { pageH = avH; pageW = (int)(avH / aspect); }
    int pageX = previewRc.left + (previewRc.Width()  - pageW) / 2;
    int pageY = previewRc.top  + (previewRc.Height() - pageH) / 2;
    CRect rcPage(pageX, pageY, pageX + pageW, pageY + pageH);

    // Drop shadow
    CRect shadow = rcPage; shadow.OffsetRect(5, 5);
    dc.FillSolidRect(shadow, RGB(50, 50, 50));

    // White page
    dc.FillSolidRect(rcPage, RGB(255, 255, 255));

    // Render content with proportional margin
    int marg = max(6, pageW / 25);
    CRect rcContent = rcPage;
    rcContent.DeflateRect(marg, marg);
    m_render(&dc, rcContent);

    // Button strip background
    CRect btnRc(0, rc.bottom - BTN_STRIP, rc.right, rc.bottom);
    dc.FillSolidRect(btnRc, ::GetSysColor(COLOR_BTNFACE));
}

BOOL CSimplePreviewDlg::OnEraseBkgnd(CDC*) { return TRUE; }

void CSimplePreviewDlg::OnPrint()
{
    SimplePageOpts opts;
    opts.landscape = !m_portrait;
    SimplePrint(m_docName.c_str(), opts, m_render);
}

void CSimplePreviewDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);
    if (!m_btnPrint.GetSafeHwnd()) return;
    const int BTN_STRIP = 44, bh = 32, bw = 80, margin = 8;
    m_btnPrint.SetWindowPos(nullptr, cx/2 - bw - margin/2, cy - bh - margin, bw, bh, SWP_NOZORDER);
    m_btnClose.SetWindowPos(nullptr, cx/2 + margin/2,       cy - bh - margin, bw, bh, SWP_NOZORDER);
    Invalidate(FALSE);
}

// =============================================================================
// CCalPrintDlg — Message map
// =============================================================================

BEGIN_MESSAGE_MAP(CCalPrintDlg, CDialog)
    ON_BN_CLICKED(CCalPrintDlg::IDC_PD_BTN_PREVIEW, &CCalPrintDlg::OnBnPreview)
END_MESSAGE_MAP()

// =============================================================================
// CCalPrintDlg — Construction
// =============================================================================

CCalPrintDlg::CCalPrintDlg(CMainFrame* pFrame, CWnd* pParent)
    : CDialog(), m_pFrame(pFrame)
{
    m_pParentWnd = pParent;
}

// =============================================================================
// CCalPrintDlg — DoModal (in-memory template, no .rc needed)
// =============================================================================

INT_PTR CCalPrintDlg::DoModal()
{
    struct DlgTmpl {
        DLGTEMPLATE t;
        WORD menu; WORD cls;
        wchar_t title[32];
    } buf = {};
    buf.t.style  = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER;
    buf.t.cdit   = 0;
    buf.t.cx     = 310;
    buf.t.cy     = 256;
    wcscpy_s(buf.title, L"Print Calendar");
    if (!InitModalIndirect((DLGTEMPLATE*)&buf, m_pParentWnd)) return -1;
    return CDialog::DoModal();
}

// =============================================================================
// CCalPrintDlg — OnInitDialog
// =============================================================================

BOOL CCalPrintDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    SetWindowText(L"Print Calendar");

    // Restore saved print settings
    {
        const auto& ps = theApp.m_settings;
        m_opts.landscape     = ps.printLandscape;
        m_opts.range         = (CalPrintOptions::Range)ps.printRange;
        m_opts.mTop          = ps.printMarginTop;
        m_opts.mBot          = ps.printMarginBot;
        m_opts.mLeft         = ps.printMarginLeft;
        m_opts.mRight        = ps.printMarginRight;
        m_opts.includeZmanim = ps.printWeeklyZmanim;
    }

    CRect rcClient;
    GetClientRect(&rcClient);
    const int W = rcClient.Width();
    const int H = rcClient.Height();

    HFONT hGUI = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    CFont* pF  = CFont::FromHandle(hGUI);

    // Helper lambdas
    auto mkStatic = [&](const wchar_t* txt, int x, int y, int w, int h) {
        CStatic* s = new CStatic;
        s->Create(txt, WS_CHILD | WS_VISIBLE | SS_LEFT,
                  CRect(x, y, x + w, y + h), this);
        s->SetFont(pF);
    };
    auto mkRadio = [&](CButton& b, const wchar_t* txt, UINT id,
                       int x, int y, int w, bool first, bool checked) {
        DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON;
        if (first) style |= WS_GROUP;
        b.Create(txt, style, CRect(x, y, x + w, y + 18), this, id);
        b.SetFont(pF);
        if (checked) b.SetCheck(BST_CHECKED);
    };
    auto mkEdit = [&](CEdit& e, UINT id, const wchar_t* val,
                      int x, int y, int w) {
        e.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_CENTER,
                 CRect(x, y, x + w, y + 20), this, id);
        e.SetFont(pF);
        e.SetWindowText(val);
    };

    int y = 10;

    // ── Print Range ──────────────────────────────────────────────────────────
    mkStatic(L"Print Range", 8, y, 200, 16); y += 20;

    CString sMonth, sYear;
    if (m_pFrame)
    {
        sMonth.Format(L"Current month  (%s %d)",
            GregorianMonthName(m_pFrame->m_viewMonth).c_str(),
            m_pFrame->m_viewYear);
        sYear.Format(L"Full year  (%d)", m_pFrame->m_viewYear);
    }
    else
    {
        sMonth = L"Current month";
        sYear  = L"Full year";
    }

    mkRadio(m_radMonth, sMonth, IDC_PD_RAD_MONTH, 20, y, W - 30, true,
            m_opts.range == CalPrintOptions::RANGE_MONTH);  y += 22;
    mkRadio(m_radYear,  sYear,  IDC_PD_RAD_YEAR,  20, y, W - 30, false,
            m_opts.range == CalPrintOptions::RANGE_YEAR);   y += 22;
    mkRadio(m_rad12, L"Next 12 months", IDC_PD_RAD_12, 20, y, W - 30, false,
            m_opts.range == CalPrintOptions::RANGE_12);     y += 28;

    // ── Orientation ──────────────────────────────────────────────────────────
    mkStatic(L"Orientation", 8, y, 200, 16); y += 20;
    mkRadio(m_radPortrait,  L"Portrait",  IDC_PD_RAD_PORT, 20,  y, 100, true,  !m_opts.landscape);
    mkRadio(m_radLandscape, L"Landscape", IDC_PD_RAD_LAND, 140, y, 120, false,  m_opts.landscape);
    y += 28;

    // ── Margins ──────────────────────────────────────────────────────────────
    auto fmtMargin = [](float v) -> CString {
        CString s; s.Format(L"%.2f", v); return s;
    };
    mkStatic(L"Margins (inches)", 8, y, 200, 16); y += 20;
    mkStatic(L"Top:",    8,  y + 2, 34, 16);
    mkEdit(m_editTop,    IDC_PD_EDT_TOP,   fmtMargin(m_opts.mTop),   46,  y, 50);
    mkStatic(L"Bottom:", 115, y + 2, 48, 16);
    mkEdit(m_editBot,    IDC_PD_EDT_BOT,   fmtMargin(m_opts.mBot),  168, y, 50);
    y += 26;
    mkStatic(L"Left:",   8,  y + 2, 34, 16);
    mkEdit(m_editLeft,   IDC_PD_EDT_LEFT,  fmtMargin(m_opts.mLeft),  46,  y, 50);
    mkStatic(L"Right:",  115, y + 2, 48, 16);
    mkEdit(m_editRight,  IDC_PD_EDT_RIGHT, fmtMargin(m_opts.mRight), 168, y, 50);
    y += 30;

    // ── Include weekly zmanim ────────────────────────────────────────────────
    m_chkZmanim.Create(
        L"Include weekly zmanim table  (shows Friday zmanim for each week of the month)",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | WS_GROUP,
        CRect(20, y, W - 10, y + 18), this, IDC_PD_CHK_ZMANIM);
    m_chkZmanim.SetFont(pF);
    m_chkZmanim.SetCheck(m_opts.includeZmanim ? BST_CHECKED : BST_UNCHECKED);
    y += 26;

    // ── Buttons ──────────────────────────────────────────────────────────────
    int btnY = H - 36;
    m_btnPreview.Create(L"Preview",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | WS_GROUP,
        CRect(10, btnY, 90, btnY + 26), this, IDC_PD_BTN_PREVIEW);
    m_btnPreview.SetFont(pF);

    CButton* pPrint = new CButton;
    pPrint->Create(L"Print...",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        CRect(W - 170, btnY, W - 90, btnY + 26), this, IDOK);
    pPrint->SetFont(pF);

    CButton* pCancel = new CButton;
    pCancel->Create(L"Cancel",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(W - 82, btnY, W - 8, btnY + 26), this, IDCANCEL);
    pCancel->SetFont(pF);

    return TRUE;
}

// =============================================================================
// CCalPrintDlg — ReadControls
// =============================================================================

void CCalPrintDlg::ReadControls()
{
    if (m_radMonth.GetCheck()) m_opts.range = CalPrintOptions::RANGE_MONTH;
    else if (m_radYear.GetCheck()) m_opts.range = CalPrintOptions::RANGE_YEAR;
    else m_opts.range = CalPrintOptions::RANGE_12;

    m_opts.landscape     = (m_radLandscape.GetCheck() == BST_CHECKED);
    m_opts.includeZmanim = (m_chkZmanim.GetCheck()    == BST_CHECKED);

    auto getFloat = [](CEdit& e, float def) {
        CString s; e.GetWindowText(s);
        float v = (float)_wtof(s);
        return (v > 0.0f && v < 5.0f) ? v : def;
    };
    m_opts.mTop   = getFloat(m_editTop,   0.75f);
    m_opts.mBot   = getFloat(m_editBot,   0.75f);
    m_opts.mLeft  = getFloat(m_editLeft,  0.50f);
    m_opts.mRight = getFloat(m_editRight, 0.50f);
}

// =============================================================================
// CCalPrintDlg — OnOK (Print button)
// =============================================================================

static void SavePrintOptsToSettings(const CalPrintOptions& opts)
{
    auto& ps = theApp.m_settings;
    ps.printLandscape    = opts.landscape;
    ps.printRange        = (int)opts.range;
    ps.printMarginTop    = opts.mTop;
    ps.printMarginBot    = opts.mBot;
    ps.printMarginLeft   = opts.mLeft;
    ps.printMarginRight  = opts.mRight;
    ps.printWeeklyZmanim = opts.includeZmanim;
    SaveSettings(ps);
}

void CCalPrintDlg::OnOK()
{
    ReadControls();
    SavePrintOptsToSettings(m_opts);
    CDialog::OnOK();
}

// =============================================================================
// CCalPrintDlg — OnBnPreview
// =============================================================================

void CCalPrintDlg::OnBnPreview()
{
    ReadControls();
    SavePrintOptsToSettings(m_opts);
    CCalPreviewDlg prev(m_opts, m_pFrame, this);
    prev.DoModal();
}

// =============================================================================
// CCalPreviewDlg — Message map
// =============================================================================

BEGIN_MESSAGE_MAP(CCalPreviewDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_BN_CLICKED(CCalPreviewDlg::IDC_PRV_PREV,  &CCalPreviewDlg::OnPrevPage)
    ON_BN_CLICKED(CCalPreviewDlg::IDC_PRV_NEXT,  &CCalPreviewDlg::OnNextPage)
    ON_BN_CLICKED(CCalPreviewDlg::IDC_PRV_PRINT, &CCalPreviewDlg::OnPrint)
END_MESSAGE_MAP()

// =============================================================================
// CCalPreviewDlg — Construction
// =============================================================================

CCalPreviewDlg::CCalPreviewDlg(const CalPrintOptions& opts,
                                 CMainFrame* pFrame, CWnd* pParent)
    : CDialog(), m_opts(opts), m_pFrame(pFrame)
{
    m_pParentWnd = pParent;
    if (pFrame)
        m_pages = BuildPageList(opts, pFrame->m_viewYear, pFrame->m_viewMonth);
}

// =============================================================================
// CCalPreviewDlg — DoModal
// =============================================================================

INT_PTR CCalPreviewDlg::DoModal()
{
    struct DlgTmpl {
        DLGTEMPLATE t;
        WORD menu; WORD cls;
        wchar_t title[32];
    } buf = {};
    buf.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
                  DS_MODALFRAME | DS_CENTER;
    buf.t.cdit  = 0;
    buf.t.cx    = 560;
    buf.t.cy    = 460;
    wcscpy_s(buf.title, L"Print Preview");
    if (!InitModalIndirect((DLGTEMPLATE*)&buf, m_pParentWnd)) return -1;
    return CDialog::DoModal();
}

// =============================================================================
// CCalPreviewDlg — OnInitDialog
// =============================================================================

BOOL CCalPreviewDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    SetWindowText(L"Print Preview");

    HFONT hGUI = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    CFont* pF  = CFont::FromHandle(hGUI);

    // Buttons at bottom (positioned in LayoutButtons)
    m_btnPrev.Create(L"< Prev",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | WS_GROUP,
        CRect(0, 0, 80, 28), this, IDC_PRV_PREV);
    m_btnPrev.SetFont(pF);

    m_btnNext.Create(L"Next >",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(0, 0, 80, 28), this, IDC_PRV_NEXT);
    m_btnNext.SetFont(pF);

    m_lblPage.Create(L"",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        CRect(0, 0, 120, 28), this, IDC_PRV_LBL);
    m_lblPage.SetFont(pF);

    m_btnPrint.Create(L"Print...",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        CRect(0, 0, 80, 28), this, IDC_PRV_PRINT);
    m_btnPrint.SetFont(pF);

    CButton* pClose = new CButton;
    pClose->Create(L"Close",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(0, 0, 80, 28), this, IDCANCEL);
    pClose->SetFont(pF);

    UpdateNavButtons();
    UpdatePageLabel();

    CRect rc;
    GetClientRect(&rc);
    LayoutButtons();

    return TRUE;
}

// =============================================================================
// CCalPreviewDlg — LayoutButtons
// =============================================================================

void CCalPreviewDlg::LayoutButtons()
{
    CRect rc;
    GetClientRect(&rc);
    int W = rc.Width(), H = rc.Height();
    int bY = H - 38, bH = 28;

    // Centre the navigation group
    m_btnPrev .MoveWindow(10,         bY, 80, bH);
    m_lblPage .MoveWindow(100,        bY, 120, bH);
    m_btnNext .MoveWindow(230,        bY, 80, bH);
    m_btnPrint.MoveWindow(W - 180,    bY, 80, bH);

    // Close button — find by IDCANCEL
    CWnd* pClose = GetDlgItem(IDCANCEL);
    if (pClose) pClose->MoveWindow(W - 90, bY, 80, bH);
}

// =============================================================================
// CCalPreviewDlg — OnSize
// =============================================================================

void CCalPreviewDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);
    if (m_btnPrev.GetSafeHwnd()) LayoutButtons();
    Invalidate(FALSE);
}

// =============================================================================
// CCalPreviewDlg — Navigation
// =============================================================================

void CCalPreviewDlg::UpdateNavButtons()
{
    bool hasPrev = (m_curPage > 0);
    bool hasNext = (m_curPage < (int)m_pages.size() - 1);
    if (m_btnPrev.GetSafeHwnd())
    {
        m_btnPrev.EnableWindow(hasPrev);
        m_btnNext.EnableWindow(hasNext);
    }
}

void CCalPreviewDlg::UpdatePageLabel()
{
    if (!m_lblPage.GetSafeHwnd()) return;
    CString s;
    s.Format(L"Page %d of %d", m_curPage + 1, (int)m_pages.size());
    m_lblPage.SetWindowText(s);
}

void CCalPreviewDlg::OnPrevPage()
{
    if (m_curPage > 0)
    {
        m_curPage--;
        UpdateNavButtons();
        UpdatePageLabel();
        Invalidate(FALSE);
    }
}

void CCalPreviewDlg::OnNextPage()
{
    if (m_curPage < (int)m_pages.size() - 1)
    {
        m_curPage++;
        UpdateNavButtons();
        UpdatePageLabel();
        Invalidate(FALSE);
    }
}

void CCalPreviewDlg::OnPrint()
{
    DoPrint(m_opts, m_pFrame);
}

// =============================================================================
// CCalPreviewDlg — Painting
// =============================================================================

BOOL CCalPreviewDlg::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

void CCalPreviewDlg::OnPaint()
{
    CPaintDC dc(this);

    CRect rcClient;
    GetClientRect(&rcClient);
    int W = rcClient.Width();
    int H = rcClient.Height();

    // Double-buffer to avoid flicker
    CDC     memDC;
    CBitmap bmp;
    memDC.CreateCompatibleDC(&dc);
    bmp.CreateCompatibleBitmap(&dc, W, H);
    CBitmap* pOldBmp = memDC.SelectObject(&bmp);

    // Gray background
    memDC.FillSolidRect(rcClient, RGB(100, 100, 100));

    const int toolH = 46;  // toolbar height at bottom

    // Preview area (above toolbar)
    CRect rcPv(10, 10, W - 10, H - toolH - 5);
    if (rcPv.IsRectEmpty() || m_pages.empty())
    {
        dc.BitBlt(0, 0, W, H, &memDC, 0, 0, SRCCOPY);
        memDC.SelectObject(pOldBmp);
        return;
    }

    // Paper aspect ratio
    float paperW = m_opts.landscape ? 11.0f : 8.5f;
    float paperH = m_opts.landscape ?  8.5f : 11.0f;
    float aspect  = paperW / paperH;

    int pvW = rcPv.Width(), pvH = rcPv.Height();
    int pageW, pageH;
    if ((float)pvW / pvH > aspect)
    {
        pageH = pvH;
        pageW = (int)(pageH * aspect);
    }
    else
    {
        pageW = pvW;
        pageH = (int)(pageW / aspect);
    }

    int pageX = rcPv.left  + (pvW - pageW) / 2;
    int pageY = rcPv.top   + (pvH - pageH) / 2;

    // Page shadow
    memDC.FillSolidRect(
        CRect(pageX + 4, pageY + 4, pageX + pageW + 4, pageY + pageH + 4),
        RGB(60, 60, 60));

    // White page
    memDC.FillSolidRect(
        CRect(pageX, pageY, pageX + pageW, pageY + pageH),
        RGB(255, 255, 255));

    // Content rect (apply margins scaled to page pixels)
    float scaleX = (float)pageW / paperW;
    float scaleY = (float)pageH / paperH;
    int mL = (int)(m_opts.mLeft  * scaleX);
    int mR = (int)(m_opts.mRight * scaleX);
    int mT = (int)(m_opts.mTop   * scaleY);
    int mB = (int)(m_opts.mBot   * scaleY);

    CRect rcContent(pageX + mL, pageY + mT,
                    pageX + pageW - mR, pageY + pageH - mB);

    // Draw calendar
    auto [yr, mo] = m_pages[m_curPage];
    DrawCalMonthPage(&memDC, rcContent, yr, mo, m_pFrame, m_opts);

    // Toolbar background
    CRect rcTool(0, H - toolH, W, H);
    memDC.FillSolidRect(rcTool, GetSysColor(COLOR_3DFACE));

    dc.BitBlt(0, 0, W, H, &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}
