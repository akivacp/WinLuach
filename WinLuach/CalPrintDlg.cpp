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

static void DrawTextFit(CDC* pDC, const wchar_t* text, const CRect& rc, UINT format)
{
    if (!text || !*text || rc.Width() <= 1 || rc.Height() <= 1)
        return;

    CFont* pCurrent = pDC->GetCurrentFont();
    LOGFONT lf = {};
    if (!pCurrent || pCurrent->GetLogFont(&lf) == 0)
    {
        CRect drawRc(rc);
        pDC->DrawText(text, -1, drawRc, format);
        return;
    }

    int fitWidth = max(1, rc.Width() - 2);
    int fitHeight = max(1, rc.Height());

    CSize sz = pDC->GetTextExtent(text);
    TEXTMETRIC tm = {};
    pDC->GetTextMetrics(&tm);
    if (sz.cx <= fitWidth && tm.tmHeight <= fitHeight)
    {
        CRect drawRc(rc);
        pDC->DrawText(text, -1, drawRc, format);
        return;
    }

    int originalHeight = abs(lf.lfHeight);
    if (originalHeight <= 0)
    {
        CRect drawRc(rc);
        pDC->DrawText(text, -1, drawRc, format);
        return;
    }

    int maxHeight = min(originalHeight, fitHeight);
    int minHeight = min(maxHeight, 3);
    int low = minHeight, high = maxHeight, best = 0;
    CFont* pOld = pCurrent;

    while (low <= high)
    {
        int mid = (low + high) / 2;
        LOGFONT trial = lf;
        trial.lfHeight = (lf.lfHeight < 0) ? -mid : mid;

        CFont fontTrial;
        fontTrial.CreateFontIndirect(&trial);
        CFont* pPrev = pDC->SelectObject(&fontTrial);
        CSize trialSize = pDC->GetTextExtent(text);
        TEXTMETRIC trialTm = {};
        pDC->GetTextMetrics(&trialTm);
        bool fits = trialSize.cx <= fitWidth && trialTm.tmHeight <= fitHeight;
        pDC->SelectObject(pPrev);

        if (fits)
        {
            best = mid;
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }

    LOGFONT finalLf = lf;
    finalLf.lfHeight = (lf.lfHeight < 0)
        ? -(best > 0 ? best : minHeight)
        :  (best > 0 ? best : minHeight);

    CFont fontFit;
    fontFit.CreateFontIndirect(&finalLf);
    pDC->SelectObject(&fontFit);
    UINT drawFormat = (best > 0) ? format : (format | DT_END_ELLIPSIS);
    CRect drawRc(rc);
    pDC->DrawText(text, -1, drawRc, drawFormat);
    pDC->SelectObject(pOld);
}

static void DrawTextFit(CDC* pDC, const CString& text, const CRect& rc, UINT format)
{
    DrawTextFit(pDC, (LPCWSTR)text, rc, format);
}

static void DrawTextFit(CDC* pDC, const std::wstring& text, const CRect& rc, UINT format)
{
    DrawTextFit(pDC, text.c_str(), rc, format);
}

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
    DrawTextFit(pDC, gregStr, rcG, DT_LEFT  | DT_VCENTER | DT_SINGLELINE);
    DrawTextFit(pDC, hebStr,  rcH, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

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
        DrawTextFit(pDC, kDays[c], rDN, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // === Footer height (computed early so cells don't overlap it) ===========
    int fFoot = -(H * 16 / 1000); if (fFoot > -4) fFoot = -4;
    int footH = opts.showFooter ? (-fFoot + 4) : 0;

    // === Calendar cells =====================================================
    int gridTop  = dayHdrY + dayHdrH;
    int availH   = H - (gridTop - y0) - footH;
    int gridH    = opts.includeZmanim ? (availH * 56 / 100) : availH;
    int contentBottom = y0 + H - footH;

    DayOfWeek dow0     = GetDayOfWeek(gFirst);
    long      jdn0     = GregorianToJDN(gFirst) - (int)dow0;
    int       daysInMo = DaysInGregorianMonth(month, year);
    int       numRows  = ((int)dow0 + daysInMo - 1) / 7 + 1;  // 4, 5, or 6
    int       cellH    = gridH / numRows;
    int       lineH    = -fSmall + 2;
    int       mg       = max(3, W / 120);

    bool isIsrael   = pFrame ? pFrame->m_isIsrael    : false;
    bool showMoadim = pFrame ? pFrame->m_showMoadim  : true;
    bool showPar    = pFrame ? pFrame->m_showParshios: true;
    GregorianDate today = pFrame ? pFrame->m_today : GregorianDate{};

    for (int cell = 0; cell < numRows * 7; cell++)
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
        DrawTextFit(pDC, dn, rGD, DT_LEFT | DT_TOP | DT_SINGLELINE);

        // --- Hebrew date (top-right, small blue) ---
        bool leap = IsHebrewLeapYear(h.year);
        std::wstring hDay = std::to_wstring(h.day) + L" "
                          + HebrewMonthName(h.month, leap);
        pDC->SelectObject(&fontSmall);
        pDC->SetTextColor(cur ? CLR_HEBREW_TXT : RGB(180, 180, 200));
        CRect rHD(rCell.left + mg, rCell.top + mg,
                  rCell.right - mg, rCell.top + mg + lineH);
        DrawTextFit(pDC, hDay, rHD, DT_RIGHT | DT_TOP | DT_SINGLELINE);

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
                    DrawTextFit(pDC, txt, rHol,
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
                    DrawTextFit(pDC, pt, rP,
                        DT_LEFT | DT_TOP | DT_END_ELLIPSIS | DT_SINGLELINE);
                    yOff += lineH; lines++;
                }
            }
            if (omer.isOmerDay && lines < maxLn)
            {
                CString ot; ot.Format(L"Day %d Omer", omer.day);
                pDC->SetTextColor(RGB(100, 80, 150));
                CRect rO(rCell.left + mg, yOff, rCell.right - mg, yOff + lineH);
                DrawTextFit(pDC, ot, rO, DT_LEFT | DT_TOP | DT_SINGLELINE);
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
                    DrawTextFit(pDC, ev, rEv,
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
                    DrawTextFit(pDC, candleStr, rZ,
                        DT_LEFT | DT_TOP | DT_SINGLELINE);
                    yOff += lineH; lines++;
                }
                if (!motzStr.empty() && lines < maxLn)
                {
                    pDC->SetTextColor(RGB(0, 80, 160));
                    CRect rZ(rCell.left + mg, yOff, rCell.right - mg, yOff + lineH);
                    DrawTextFit(pDC, motzStr, rZ,
                        DT_LEFT | DT_TOP | DT_SINGLELINE);
                }
            }
        }
    }

    // === Grid lines =========================================================
    int penW = max(1, W / 500);
    CPen penGrid(PS_SOLID, penW, CLR_GRID);
    CPen* pOld = pDC->SelectObject(&penGrid);
    for (int r = 0; r <= numRows; r++)
    {
        int gy = gridTop + r * cellH;
        pDC->MoveTo(x0, gy);
        pDC->LineTo(x0 + W, gy);
    }
    for (int c = 0; c <= 7; c++)
    {
        pDC->MoveTo(colX[c], gridTop);
        pDC->LineTo(colX[c], gridTop + numRows * cellH);
    }
    pDC->SelectObject(pOld);

    // === Weekly Zmanim Table ================================================
    if (opts.includeZmanim && pFrame)
    {
        int tblTop = gridTop + numRows * cellH;
        int tblH   = contentBottom - tblTop;

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

            // Column layout: 1 label column + up to 15 data columns (filtered by bitmask)
            static const wchar_t* kAllColHdr[15] = {
                L"Alos", L"Mishey.", L"Netz",
                L"Shma MA", L"Shma GRA",
                L"Tfla MA", L"Tfla GRA",
                L"Chatzos", L"Mn.Gd.", L"Mn.Kt.",
                L"Plag", L"Candle", L"Shkia", L"Tzais", L"Sha’a"
            };
            uint32_t colMask = opts.zmanimColumns ? opts.zmanimColumns : 0x7FFF;
            int numDataCols = 0;
            int visIdx[15];   // maps visible column → original index 0-14
            for (int i = 0; i < 15; i++)
                if ((colMask >> i) & 1) visIdx[numDataCols++] = i;
            if (numDataCols == 0) { colMask = 0x7FFF; numDataCols = 15; for (int i=0;i<15;i++) visIdx[i]=i; }

            int lblW  = W * 21 / 100;
            int timeW = (numDataCols > 0) ? (W - lblW) / numDataCols : 1;
            int tblColX[17];   // tblColX[0]=label start, [1..numDataCols+1]=data cols
            tblColX[0] = x0;
            tblColX[1] = x0 + lblW;
            for (int c = 1; c <= numDataCols; c++) tblColX[c+1] = tblColX[c] + timeW;
            // ensure last column reaches right edge
            tblColX[numDataCols + 1] = x0 + W;

            // Title row (10%)
            int titleRowH = max(6, tblH * 10 / 100);
            pDC->FillSolidRect(CRect(x0, tblTop, x0+W, tblTop+titleRowH), RGB(50,80,140));
            pDC->SetBkMode(TRANSPARENT);
            pDC->SetTextColor(RGB(255,255,255));
            pDC->SelectObject(&fontTblB);
            DrawTextFit(pDC, L"Weekly Zmanim",
                CRect(x0+4, tblTop, x0+W, tblTop+titleRowH),
                DT_LEFT|DT_VCENTER|DT_SINGLELINE);

            // Header row (14%)
            int hdrRowH = max(6, tblH * 14 / 100);
            int hdrRowY = tblTop + titleRowH;
            pDC->FillSolidRect(CRect(x0, hdrRowY, x0+W, hdrRowY+hdrRowH), RGB(70,100,160));
            pDC->SetTextColor(RGB(255,255,255));
            pDC->SelectObject(&fontTbl);
            // Label column header
            DrawTextFit(pDC, L"Week / Parasha",
                CRect(tblColX[0]+2, hdrRowY, tblColX[1], hdrRowY+hdrRowH),
                DT_LEFT|DT_VCENTER|DT_SINGLELINE);
            // Data column headers (only visible columns)
            for (int v = 0; v < numDataCols; v++)
            {
                int cx = tblColX[v+1], cw = tblColX[v+2] - tblColX[v+1];
                DrawTextFit(pDC, kAllColHdr[visIdx[v]], CRect(cx, hdrRowY, cx+cw, hdrRowY+hdrRowH),
                    DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            }

            // Data rows
            int dataY = hdrRowY + hdrRowH;
            // Count shabboses in this month to size rows
            int numShabRows = 0;
            for (int w = 0; w < numRows; w++) {
                GregorianDate ts = JDNToGregorian(jdn0 + w * 7 + 6);
                if (ts.year == year && ts.month == month) numShabRows++;
            }
            int dataRowH = max(4, (contentBottom - dataY) / max(1, numShabRows));

            int drawnRows = 0;
            for (int w = 0; w < numRows; w++)
            {
                long         shabJDN = jdn0 + w * 7 + 6;
                GregorianDate shab   = JDNToGregorian(shabJDN);
                if (shab.year != year || shab.month != month) continue;  // skip out-of-month shabbos
                int ry = dataY + drawnRows * dataRowH;
                drawnRows++;
                COLORREF rowBg = (drawnRows % 2 == 0) ? RGB(248,250,255) : RGB(235,242,255);
                pDC->FillSolidRect(CRect(x0, ry, x0+W, ry+dataRowH), rowBg);
                pDC->SetBkMode(TRANSPARENT);

                // Candle lighting from Friday (col 5)
                long         friJDN  = jdn0 + w * 7 + 5;
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
                DrawTextFit(pDC, lbl,
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

                const TimeOfDay* kAllT[15] = {
                    &z.alot_GRA,         &z.misheyakir_11,    &z.hanetz,
                    &z.sofShema_MA72,    &z.sofShema_GRA,
                    &z.sofTefilla_MA72,  &z.sofTefilla_GRA,
                    &z.chatzot,          &z.minchaGedola_GRA, &z.minchaKetana_GRA,
                    &z.plagMincha_GRA,   &zFri.candleLighting,
                    &z.shkia,            &z.tzeit_GRA,
                    nullptr              // placeholder for sha'a (handled separately)
                };
                pDC->SetTextColor(RGB(20,40,80));
                for (int v = 0; v < numDataCols; v++)
                {
                    int orig = visIdx[v];
                    int cx = tblColX[v+1], cw = tblColX[v+2] - tblColX[v+1];
                    if (orig == 14) // Sha'a Zmanit
                    {
                        if (z.shaahZmanit_GRA > 0.0) {
                            int szMin = (int)round(z.shaahZmanit_GRA);
                            CString szs; szs.Format(L"%d:%02d", szMin/60, szMin%60);
                            DrawTextFit(pDC, szs, CRect(cx, ry, cx+cw, ry+dataRowH),
                                DT_CENTER|DT_VCENTER|DT_SINGLELINE);
                        }
                    }
                    else if (kAllT[orig] && kAllT[orig]->IsValid())
                    {
                        std::wstring ts = FormatTime(*kAllT[orig], opts.use24hr);
                        DrawTextFit(pDC, ts, CRect(cx, ry, cx+cw, ry+dataRowH),
                            DT_CENTER|DT_VCENTER|DT_SINGLELINE);
                    }
                }
            }

            // Table grid lines
            CPen penTbl(PS_SOLID, max(1,W/800), RGB(170,170,180));
            CPen* pOldT = pDC->SelectObject(&penTbl);
            pDC->MoveTo(x0, tblTop);  pDC->LineTo(x0+W, tblTop);
            pDC->MoveTo(x0, hdrRowY); pDC->LineTo(x0+W, hdrRowY);
            for (int r = 0; r <= numShabRows; r++)
            {
                int gy = dataY + r * dataRowH;
                pDC->MoveTo(x0, gy); pDC->LineTo(x0+W, gy);
            }
            for (int c = 0; c <= numDataCols + 1; c++)
            {
                pDC->MoveTo(tblColX[c], tblTop);
                pDC->LineTo(tblColX[c], dataY + numShabRows*dataRowH);
            }
            pDC->SelectObject(pOldT);
        }
    }

    // Footer (footH and fFoot already computed above)
    if (opts.showFooter)
    {
        CFont fontFoot;
        fontFoot.CreateFont(fFoot,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
        pDC->SelectObject(&fontFoot);
        pDC->SetTextColor(RGB(140,140,140));
        pDC->SetBkMode(TRANSPARENT);
        pDC->FillSolidRect(CRect(x0, y0+H-footH, x0+W, y0+H), RGB(245, 245, 245));
        DrawTextFit(pDC, L"© 2026 WinLuach  https://github.com/akivacp/WinLuach/  MIT License",
            CRect(x0, y0+H-footH, x0+W, y0+H), DT_CENTER|DT_BOTTOM|DT_SINGLELINE);
    }
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

static bool UseTwoColumnCalendarPrint(const CalPrintOptions& opts,
                                      const std::vector<std::pair<int, int>>& months)
{
    return months.size() > 1 && (opts.twoColumns || months.size() > 1);
}

static int CalendarPrintSheetCount(const CalPrintOptions& opts,
                                   const std::vector<std::pair<int, int>>& months)
{
    int perSheet = UseTwoColumnCalendarPrint(opts, months) ? 2 : 1;
    return max(1, ((int)months.size() + perSheet - 1) / perSheet);
}

static void DrawCalendarPrintSheet(CDC* pDC, const CRect& rcContent,
                                   const std::vector<std::pair<int, int>>& months,
                                   int sheetIndex, CMainFrame* pFrame,
                                   const CalPrintOptions& opts)
{
    if (months.empty())
        return;

    if (!UseTwoColumnCalendarPrint(opts, months))
    {
        auto [yr, mo] = months[min(sheetIndex, (int)months.size() - 1)];
        DrawCalMonthPage(pDC, rcContent, yr, mo, pFrame, opts);
        return;
    }

    pDC->FillSolidRect(rcContent, RGB(255, 255, 255));
    int gap = max(10, rcContent.Width() / 45);
    int colW = (rcContent.Width() - gap) / 2;
    int start = sheetIndex * 2;
    int fFoot = -(rcContent.Height() * 16 / 1000);
    if (fFoot > -4) fFoot = -4;
    int footH = opts.showFooter ? (-fFoot + 4) : 0;

    CalPrintOptions colOpts = opts;
    colOpts.showFooter = false;

    for (int i = 0; i < 2; ++i)
    {
        int idx = start + i;
        if (idx >= (int)months.size())
            break;

        int left = rcContent.left + i * (colW + gap);
        CRect rcCol(left, rcContent.top, left + colW, rcContent.bottom - footH);
        auto [yr, mo] = months[idx];
        DrawCalMonthPage(pDC, rcCol, yr, mo, pFrame, colOpts);
    }

    CPen pen(PS_SOLID, max(1, rcContent.Width() / 900), RGB(190, 195, 205));
    CPen* oldPen = pDC->SelectObject(&pen);
    int x = rcContent.left + colW + gap / 2;
    pDC->MoveTo(x, rcContent.top);
    pDC->LineTo(x, rcContent.bottom);
    pDC->SelectObject(oldPen);

    if (opts.showFooter)
    {
        CFont fontFoot;
        fontFoot.CreateFont(fFoot, 0, 0, 0, FW_NORMAL, 0, 0, 0,
            DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        pDC->SelectObject(&fontFoot);
        pDC->SetTextColor(RGB(140, 140, 140));
        pDC->SetBkMode(TRANSPARENT);
        CRect rcFoot(rcContent.left, rcContent.bottom - footH, rcContent.right, rcContent.bottom);
        pDC->FillSolidRect(rcFoot, RGB(245, 245, 245));
        DrawTextFit(pDC, L"© 2026 WinLuach  https://github.com/akivacp/WinLuach/  MIT License",
            rcFoot, DT_CENTER | DT_BOTTOM | DT_SINGLELINE);
    }
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
    int sheetCount = CalendarPrintSheetCount(opts, pages);

    dc.StartDoc(&di);
    for (int pageIndex = 0; pageIndex < sheetCount; ++pageIndex)
    {
        dc.StartPage();
        DrawCalendarPrintSheet(&dc, rcContent, pages, pageIndex, pFrame, opts);
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
                         uint32_t colMask, bool use24hr, bool showFooter, bool showSedra,
                         uint32_t sedraHolMask)
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

    DrawTextFit(pDC, gregStr, CRect(x0+8, y0, x0+W*40/100, y0+titleH), DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    DrawTextFit(pDC, hebStr,  CRect(x0+W*40/100, y0, x0+W*72/100, y0+titleH), DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    DrawTextFit(pDC, locName, CRect(x0+W*72/100, y0, x0+W-8, y0+titleH), DT_RIGHT|DT_VCENTER|DT_SINGLELINE);

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
    int dateW  = W * 7 / 100;
    int dayW   = W * 5 / 100;
    int hebW   = W * 9 / 100;
    int sedraW = showSedra ? W * 11 / 100 : 0;
    int ftc    = showSedra ? 4 : 3;  // first time column index
    int timeW  = nc > 0 ? (W - dateW - dayW - hebW - sedraW) / nc : 1;

    // colX[0..ftc+nc]: cols 0,1,2 = Date,Day,Hebrew; [3=Sedra if showSedra]; ftc..ftc+nc = time data
    std::vector<int> colX(ftc + nc + 1);
    colX[0] = x0;
    colX[1] = x0 + dateW;
    colX[2] = x0 + dateW + dayW;
    colX[3] = x0 + dateW + dayW + hebW;
    if (showSedra) colX[4] = colX[3] + sedraW;
    for (int c = 1; c <= nc; c++) colX[ftc+c] = colX[ftc] + c * timeW;
    colX[ftc+nc] = x0 + W;  // stretch last column to right edge

    // ── Header row ────────────────────────────────────────────────────────────
    int hdrH = H * 5 / 100;
    int hdrY = y0 + titleH;
    pDC->FillSolidRect(CRect(x0, hdrY, x0+W, hdrY+hdrH), RGB(70,100,160));
    pDC->SetTextColor(RGB(255,255,255));
    pDC->SelectObject(&fontHdr);

    auto drawHdr = [&](int ci, const wchar_t* txt) {
        DrawTextFit(pDC, txt, CRect(colX[ci], hdrY, colX[ci+1], hdrY+hdrH),
            DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
    };
    drawHdr(0, L"Date");
    drawHdr(1, L"Day");
    drawHdr(2, L"Hebrew");
    if (showSedra) drawHdr(3, L"Sedra");
    for (int c = 0; c < nc; c++) drawHdr(ftc+c, kColHdr[activeCols[c]]);

    // ── Data rows ─────────────────────────────────────────────────────────────
    int dataY    = hdrY + hdrH;
    int daysInMo = DaysInGregorianMonth(month, year);
    // Compute footer height first to avoid overlap
    int fFootZ = -(H * 16 / 1000); if (fFootZ > -4) fFootZ = -4;
    int footHZ = showFooter ? (-fFootZ + 2) : 0;
    int rowH     = max(4, (y0 + H - dataY - footHZ) / daysInMo);

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
        DrawTextFit(pDC, ds, CRect(colX[0], ry, colX[1], ry+rowH), DT_CENTER|DT_VCENTER|DT_SINGLELINE);

        // Day of week
        pDC->SetTextColor(dow == SHABBAT ? RGB(0,120,0) : RGB(50,50,100));
        DrawTextFit(pDC, kDayAbbr[(int)dow], CRect(colX[1], ry, colX[2], ry+rowH),
            DT_CENTER|DT_VCENTER|DT_SINGLELINE);

        // Hebrew date
        bool         leap     = IsHebrewLeapYear(h.year);
        std::wstring hebDayStr = std::to_wstring(h.day) + L" " +
            HebrewMonthName(h.month, leap).substr(0, 4);
        pDC->SetTextColor(RGB(20,20,160));
        DrawTextFit(pDC, hebDayStr, CRect(colX[2], ry, colX[3], ry+rowH),
            DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);

        // Sedra column: parasha on Shabbos; holiday names on other days per sedraHolMask
        if (showSedra) {
            std::wstring sedraText;
            COLORREF sedraColor = RGB(30, 80, 160);
            if (dow == SHABBAT) {
                ParashaInfo parZ = GetParasha(h, isIsrael);
                if (!parZ.name.empty()) {
                    sedraText = parZ.name;
                    if (parZ.isCombined && !parZ.name2.empty()) sedraText += L"-" + parZ.name2;
                }
            } else if (sedraHolMask) {
                // Priority: Yom Tov > Minor > Fast > Modern
                static const uint32_t kPrio[] = {
                    HOLIDAY_YOM_TOV, HOLIDAY_MINOR, HOLIDAY_FAST, HOLIDAY_MODERN
                };
                static const COLORREF kClr[] = {
                    RGB(0, 110, 0), RGB(60, 100, 0), RGB(140, 0, 0), RGB(0, 100, 80)
                };
                for (int pi = 0; pi < 4 && sedraText.empty(); ++pi) {
                    if (!(sedraHolMask & kPrio[pi])) continue;
                    for (const auto& ho : hols2) {
                        if (ho.flags & kPrio[pi]) {
                            sedraText  = ho.name;
                            sedraColor = kClr[pi];
                            break;
                        }
                    }
                }
            }
            if (!sedraText.empty()) {
                pDC->SetTextColor(sedraColor);
                DrawTextFit(pDC, sedraText, CRect(colX[3], ry, colX[4], ry+rowH),
                    DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
            }
        }

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
                DrawTextFit(pDC, ts, CRect(colX[ftc+c], ry, colX[ftc+c+1], ry+rowH),
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
    for (int c = 0; c <= ftc+nc; c++) {
        pDC->MoveTo(colX[c], hdrY);
        pDC->LineTo(colX[c], dataY + daysInMo * rowH);
    }
    pDC->SelectObject(pOld);

    // Footer (uses fFootZ and footHZ computed above)
    CFont fontFoot2;
    fontFoot2.CreateFont(fFootZ,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
    if (showFooter) {
        pDC->SelectObject(&fontFoot2);
        pDC->SetTextColor(RGB(140,140,140));
        pDC->SetBkMode(TRANSPARENT);
        pDC->FillSolidRect(CRect(x0, y0+H-footHZ, x0+W, y0+H), RGB(245, 245, 245));
        DrawTextFit(pDC, L"© 2026 WinLuach  https://github.com/akivacp/WinLuach/  MIT License",
            CRect(x0, y0+H-footHZ, x0+W, y0+H), DT_CENTER|DT_BOTTOM|DT_SINGLELINE);
    }
}

// =============================================================================
// DRAW DAY PAGE
// Portrait-oriented single-day details page (holidays, learning, zmanim).
// =============================================================================

static const wchar_t* const kDayPrintZmanLabels[] = {
    L"Alot (custom)",
    L"Alot GRA 16.1",
    L"Alot MA72",
    L"Alot MA90",
    L"Misheyakir (custom)",
    L"Misheyakir 10.2",
    L"Misheyakir 11.5",
    L"Hanetz (Netz)",
    L"Sof Shema (custom)",
    L"Sof Shema GRA",
    L"Sof Shema MA72",
    L"Sof Shema MA90",
    L"Sof Tefilla (custom)",
    L"Sof Tefilla GRA",
    L"Sof Tefilla MA72",
    L"Sof Tefilla MA90",
    L"Chatzos",
    L"Mincha Gedola (custom)",
    L"Mincha Gedola GRA",
    L"Mincha Gedola MA72",
    L"Mincha Gedola MA90",
    L"Mincha Ketana (custom)",
    L"Mincha Ketana GRA",
    L"Mincha Ketana MA72",
    L"Mincha Ketana MA90",
    L"Plag (custom)",
    L"Plag GRA",
    L"Plag MA72",
    L"Plag MA90",
    L"Candle Lighting",
    L"Shkia (Sunset)",
    L"Tzais (custom)",
    L"Tzais GRA 8.5",
    L"Tzais MA72",
    L"Tzais MA90",
    L"Sha'a Zmanit",
};

static constexpr int kDayPrintZmanCount =
    (int)(sizeof(kDayPrintZmanLabels) / sizeof(kDayPrintZmanLabels[0]));

static uint64_t NormalizeDayPrintZmanMask(uint64_t mask)
{
    uint64_t all = (kDayPrintZmanCount >= 64)
        ? UINT64_MAX
        : ((1ull << kDayPrintZmanCount) - 1ull);
    return mask & all;
}

void DrawDayPage(CDC* pDC, const CRect& rcPage,
                 const GregorianDate& g, CMainFrame* pFrame, bool showFooter,
                 uint64_t zmanimMask)
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
    int fFoot3 = -(H * 16 / 1000); if (fFoot3 > -4) fFoot3 = -4;
    int footH3 = -fFoot3 + 2;
    int bottomLimit = y0 + H - (showFooter ? footH3 : 0);
    zmanimMask = NormalizeDayPrintZmanMask(zmanimMask);

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
    if (pFrame && !pFrame->m_location.name.empty())
        gregStr.Format(L"%s,  %s %d,  %d    |    %s",
            kDow[(int)GetDayOfWeek(g)],
            GregorianMonthName(g.month).c_str(), g.day, g.year,
            pFrame->m_location.name.c_str());
    else
        gregStr.Format(L"%s,  %s %d,  %d",
            kDow[(int)GetDayOfWeek(g)],
            GregorianMonthName(g.month).c_str(), g.day, g.year);
    DrawTextFit(pDC, gregStr, CRect(x0+8, y0, x0+W-8, y0+hdrH/2+4), DT_LEFT|DT_VCENTER|DT_SINGLELINE);

    bool   leap   = IsHebrewLeapYear(h.year);
    CString hebStr;
    hebStr.Format(L"%d %s %d", h.day, HebrewMonthName(h.month, leap).c_str(), h.year);
    pDC->SetTextColor(RGB(200,220,255));
    DrawTextFit(pDC, hebStr, CRect(x0+8, y0+hdrH/2, x0+W-8, y0+hdrH-4), DT_RIGHT|DT_VCENTER|DT_SINGLELINE);

    int y    = y0 + hdrH + 4;
    int rowH = -fRow + 4;
    int secH = -fSec + 4;
    int lblW = W * 28 / 100;

    auto drawSection = [&](const wchar_t* lbl) {
        pDC->FillSolidRect(CRect(x0, y, x0+W, y+secH), RGB(200,212,240));
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(RGB(30,50,110));
        pDC->SelectObject(&fontSec);
        DrawTextFit(pDC, lbl, CRect(x0+6, y, x0+W-4, y+secH), DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        y += secH + 1;
    };
    auto drawRow = [&](const wchar_t* lbl, const std::wstring& val, COLORREF clr) {
        if (y + rowH > bottomLimit) return;
        pDC->SetBkMode(TRANSPARENT);
        pDC->SelectObject(&fontRow);
        pDC->SetTextColor(RGB(110,110,110));
        DrawTextFit(pDC, lbl, CRect(x0+8, y, x0+8+lblW, y+rowH), DT_LEFT|DT_TOP|DT_SINGLELINE);
        pDC->SetTextColor(clr);
        DrawTextFit(pDC, val, CRect(x0+8+lblW, y, x0+W-8, y+rowH),
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

    // Special Times section (shown before zmanim, with bold times like CDayViewDlg)
    {
        DayOfWeek sdow = GetDayOfWeek(g);
        bool isShabDP  = (sdow == SHABBAT);
        bool isFriDP   = (sdow == FRIDAY);
        bool hasFastDP = false, hasYTDP = false;
        for (const auto& ho : hols) {
            if (ho.flags & HOLIDAY_FAST)    hasFastDP = true;
            if (ho.flags & HOLIDAY_YOM_TOV) hasYTDP   = true;
        }
        bool isErevYKDP     = (h.month == TISHREI && h.day == 9);
        bool isTishaBavDP   = (h.month == AV      && h.day == 9) && !isShabDP;
        bool is10AvDP       = (h.month == AV      && h.day == 10);
        bool isErevTBDP     = (h.month == AV      && h.day == 8) && !isShabDP;
        bool isErevPesachDP = (h.month == NISSAN  && h.day == 14);
        bool isLagBaOmerDP  = (h.month == IYAR    && h.day == 18);

        // Fri/Shab after Pesach and after Rosh Hashana
        bool isFriAfterPes = false, isShabAfterPes = false;
        bool isFriAfterRH = false, isShabAfterRH = false;
        {
            long jn = GregorianToJDN(g);
            for (int i = 1; i <= 7; i++) {
                HebrewDate hi = JDNToHebrew(jn - i);
                if (hi.month == NISSAN && hi.day == 22) { isFriAfterPes = isFriDP; isShabAfterPes = isShabDP; }
                if (hi.month == TISHREI && hi.day == 2) { isFriAfterRH = isFriDP; isShabAfterRH = isShabDP; }
            }
        }

        bool anySpecial = isShabDP || (hasFastDP && !isShabDP) || isLagBaOmerDP
            || isErevTBDP || isTishaBavDP || is10AvDP || isErevYKDP || isErevPesachDP
            || isFriAfterPes || isShabAfterPes || isFriAfterRH || isShabAfterRH;

        if (anySpecial) {
            drawSection(L"Special Times");
            // Bold row helper for key times
            auto boldRow = [&](const wchar_t* lbl, const TimeOfDay& t, COLORREF clr) {
                if (!t.IsValid() || y + rowH > bottomLimit) return;
                CFont fBold;
                fBold.CreateFont(fRow, 0,0,0, FW_BOLD, 0,0,0, DEFAULT_CHARSET,0,0,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
                pDC->SetBkMode(TRANSPARENT);
                pDC->SelectObject(&fontRow);
                pDC->SetTextColor(RGB(110,110,110));
                DrawTextFit(pDC, lbl, CRect(x0+8, y, x0+8+lblW, y+rowH), DT_LEFT|DT_TOP|DT_SINGLELINE);
                pDC->SelectObject(&fBold);
                pDC->SetTextColor(clr);
                DrawTextFit(pDC, FormatTime(t, u24), CRect(x0+8+lblW, y, x0+W-8, y+rowH),
                    DT_LEFT|DT_TOP|DT_SINGLELINE);
                y += rowH;
            };
            auto noteRow = [&](const wchar_t* txt) {
                if (y + rowH > bottomLimit) return;
                pDC->SetBkMode(TRANSPARENT);
                pDC->SelectObject(&fontRow);
                pDC->SetTextColor(RGB(130, 60, 0));
                DrawTextFit(pDC, txt, CRect(x0+8, y, x0+W-8, y+rowH*2), DT_LEFT|DT_TOP|DT_WORDBREAK);
                y += rowH * 2;
            };

            if (isShabDP && z.tzeitShabbat.IsValid())
                boldRow(L"Tzeis Shabbos:", z.tzeitShabbat, RGB(0, 120, 40));
            if (hasFastDP && !isShabDP && z.tzeit_GRA.IsValid())
                boldRow(L"Fast Ends:", z.tzeit_GRA, RGB(180, 0, 0));
            if (isLagBaOmerDP && z.chatzot.IsValid())
                boldRow(L"Chatzos:", z.chatzot, RGB(100, 50, 0));
            if (isErevTBDP && z.shkia.IsValid())
                boldRow(L"Shkia (Fast Begins):", z.shkia, RGB(180, 0, 0));
            if (isTishaBavDP && z.chatzot.IsValid())
                boldRow(L"Chatzos:", z.chatzot, RGB(100, 50, 0));
            if (is10AvDP && z.chatzot.IsValid())
                boldRow(L"Chatzos (Midday):", z.chatzot, RGB(100, 50, 0));
            if (isErevYKDP) {
                if (z.chatzot.IsValid())    boldRow(L"Chatzos:",            z.chatzot,          RGB(100, 50, 0));
                if (z.shkia.IsValid())      boldRow(L"Shkia (Fast Beg.):",  z.shkia,            RGB(180, 0, 0));
                if (z.candleLighting.IsValid()) boldRow(L"Candle Lighting:", z.candleLighting,   RGB(200, 80, 0));
            }
            if (isErevPesachDP && z.hanetz.IsValid() && z.shaahZmanit_GRA > 0.0) {
                TimeOfDay sofAchila = AddMinutes(z.hanetz, (int)(4.0 * z.shaahZmanit_GRA));
                TimeOfDay sofBiur   = AddMinutes(z.hanetz, (int)(5.0 * z.shaahZmanit_GRA));
                if (sofAchila.IsValid()) boldRow(L"Eat Chametz by:",   sofAchila, RGB(180, 60, 0));
                if (sofBiur.IsValid())   boldRow(L"Burn Chametz by:",  sofBiur,   RGB(180, 30, 0));
                if (z.chatzot.IsValid()) boldRow(L"Chatzos:",          z.chatzot, RGB(100, 50, 0));
            }
            if (isFriAfterPes || isShabAfterPes)
                noteRow(L"Key Challah / Shlissel Challah — Fri/Shab after Pesach");
            if (isFriAfterRH || isShabAfterRH)
                noteRow(L"Round Challah — Fri/Shab after Rosh Hashana");
        }
    }

    // Full zmanim section: selectable rows, rendered in two columns so the
    // full default list fits on one page.
    {
        DisplayZmanimTimes dz = pFrame
            ? pFrame->BuildDisplayZmanim(g, z, dst) : DisplayZmanimTimes{};

        struct PrintZmanRow
        {
            int bit;
            const wchar_t* label;
            std::wstring value;
        };

        std::vector<PrintZmanRow> zRows;
        auto addTime = [&](int bit, const TimeOfDay& t) {
            if (bit < 0 || bit >= kDayPrintZmanCount) return;
            if (!(zmanimMask & (1ull << bit)) || !t.IsValid()) return;
            zRows.push_back({ bit, kDayPrintZmanLabels[bit], FormatTime(t, u24) });
        };
        addTime(0,  dz.alot);
        addTime(1,  z.alot_GRA);
        addTime(2,  z.alot_MA72);
        addTime(3,  z.alot_MA90);
        addTime(4,  dz.misheyakir);
        addTime(5,  z.misheyakir_10);
        addTime(6,  z.misheyakir_11);
        addTime(7,  z.hanetz);
        addTime(8,  dz.sofShema);
        addTime(9,  z.sofShema_GRA);
        addTime(10, z.sofShema_MA72);
        addTime(11, z.sofShema_MA90);
        addTime(12, dz.sofTefilla);
        addTime(13, z.sofTefilla_GRA);
        addTime(14, z.sofTefilla_MA72);
        addTime(15, z.sofTefilla_MA90);
        addTime(16, z.chatzot);
        addTime(17, dz.minchaGedola);
        addTime(18, z.minchaGedola_GRA);
        addTime(19, z.minchaGedola_MA72);
        addTime(20, z.minchaGedola_MA90);
        addTime(21, dz.minchaKetana);
        addTime(22, z.minchaKetana_GRA);
        addTime(23, z.minchaKetana_MA72);
        addTime(24, z.minchaKetana_MA90);
        addTime(25, dz.plagMincha);
        addTime(26, z.plagMincha_GRA);
        addTime(27, z.plagMincha_MA72);
        addTime(28, z.plagMincha_MA90);
        addTime(29, z.candleLighting);
        addTime(30, z.shkia);
        addTime(31, dz.tzeit);
        addTime(32, z.tzeit_GRA);
        addTime(33, z.tzeit_MA72);
        addTime(34, z.tzeit_MA90);
        if ((zmanimMask & (1ull << 35)) && dz.shaahZmanit > 0.0) {
            int szMin = (int)round(dz.shaahZmanit);
            CString ss; ss.Format(L"%d:%02d  (%d min)", szMin/60, szMin%60, szMin);
            zRows.push_back({ 35, kDayPrintZmanLabels[35], (LPCWSTR)ss });
        }

        if (!zRows.empty() && y + secH < bottomLimit)
        {
            drawSection(L"Zmanim");

            int top = y;
            int avail = max(1, bottomLimit - top);
            int rowsPerCol = max(1, ((int)zRows.size() + 1) / 2);
            int zRowH = min(rowH, max(8, avail / rowsPerCol));
            int zFontH = -max(5, min(-fRow, zRowH - 2));
            CFont fontZman;
            fontZman.CreateFont(zFontH, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY,
                DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            int padX = 8;
            int gap = max(10, W / 45);
            int colW = (W - padX * 2 - gap) / 2;
            int labelW2 = colW * 66 / 100;

            pDC->SelectObject(&fontZman);
            pDC->SetBkMode(TRANSPARENT);
            for (int i = 0; i < (int)zRows.size(); ++i)
            {
                int col = i / rowsPerCol;
                int row = i % rowsPerCol;
                int cx0 = x0 + padX + col * (colW + gap);
                int cy0 = top + row * zRowH;
                if (cy0 + zRowH > bottomLimit) continue;

                pDC->SetTextColor(RGB(110, 110, 110));
                DrawTextFit(pDC, zRows[i].label,
                    CRect(cx0, cy0, cx0 + labelW2, cy0 + zRowH),
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                pDC->SetTextColor(RGB(20, 60, 20));
                DrawTextFit(pDC, zRows[i].value,
                    CRect(cx0 + labelW2 + 4, cy0, cx0 + colW, cy0 + zRowH),
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            }
            y = top + rowsPerCol * zRowH;
        }
    }

    if (showFooter) {
        CFont fontFoot3;
        fontFoot3.CreateFont(fFoot3,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
        pDC->SelectObject(&fontFoot3);
        pDC->SetTextColor(RGB(140,140,140));
        pDC->SetBkMode(TRANSPARENT);
        pDC->FillSolidRect(CRect(x0, y0+H-footH3, x0+W, y0+H), RGB(245, 245, 245));
        DrawTextFit(pDC, L"© 2026 WinLuach  https://github.com/akivacp/WinLuach/  MIT License",
            CRect(x0, y0+H-footH3, x0+W, y0+H), DT_CENTER|DT_BOTTOM|DT_SINGLELINE);
    }
}

// =============================================================================
// DO PRINT ZMANIM MONTH / DO PRINT DAY
// =============================================================================

bool SimplePrint(const wchar_t* docName, const SimplePageOpts& opts,
    PageRenderFn render, bool showFooter)
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
    render(&dc, rcContent, showFooter);
    dc.EndPage();
    dc.EndDoc();
    dc.Detach(); ::DeleteDC(hDC);
    if (hDM) GlobalFree(hDM);
    return true;
}

// =============================================================================
// DRAW EVENTS LIST PAGE
// Renders a year's recurring events as a sorted table.
// =============================================================================

struct EventListRow
{
    GregorianDate greg;
    HebrewDate    heb;
    std::wstring  typeStr;
    std::wstring  name;
    int           typeOrd = 3;  // for category sorting (0=Birthday,1=Anniversary,2=Yahrzeit,3=Custom)
};

// categoryMask: bits 0-3 = Birthday/Anniversary/Yahrzeit/Custom (type 0/1/2/3)
// separateCategories: if true, groups Birthday→Anniversary→Yahrzeit→Custom instead of chrono
static std::vector<EventListRow> BuildEventRows(
    const std::vector<UserEventEntry>& events, int gregYear,
    uint8_t categoryMask = 0x0F, bool separateCategories = false)
{
    std::vector<EventListRow> rows;

    for (const auto& ev : events)
    {
        // Filter by category
        int typeBit = (ev.type >= 0 && ev.type <= 3) ? ev.type : 3;
        if (!(categoryMask & (1u << typeBit))) continue;

        // Skip one-time events that don't fall in this year (#18 fix: observeAnnually)
        if (!ev.observeAnnually && ev.gregYear != 0 && ev.gregYear != gregYear) continue;
        if (!ev.observeAnnually && ev.hebYear  != 0 && ev.gregYear  == 0)
        {
            // Hebrew-anchored one-time event: check approximate year match
            bool yearOk = false;
            for (int tryHY = gregYear + 3760; tryHY <= gregYear + 3762; ++tryHY)
                if (ev.hebYear == tryHY) { yearOk = true; break; }
            if (!yearOk) continue;
        }

        EventListRow row;
        static const wchar_t* kTypes[] = { L"Birthday", L"Anniversary", L"Yahrzeit", L"Custom" };
        row.typeStr = (ev.type >= 0 && ev.type <= 3) ? kTypes[ev.type] : L"Event";
        row.typeOrd = (ev.type >= 0 && ev.type <= 3) ? ev.type : 3;
        row.name    = ev.name;

        if (ev.hebMonth != 0)
        {
            // Hebrew-based: find the Gregorian date for this month/day in gregYear
            for (int tryHY = gregYear + 3760; tryHY <= gregYear + 3762; ++tryHY)
            {
                if (ev.hebYear != 0 && !ev.observeAnnually && ev.hebYear != tryHY) continue;
                bool ly = IsHebrewLeapYear(tryHY);
                int mo = ev.hebMonth;
                if (mo == ADAR && ly) mo = ADAR_II;
                HebrewDate h; h.year = tryHY; h.month = mo; h.day = ev.hebDay;
                int daysInMo = DaysInHebrewMonth(mo, tryHY);
                if (ev.hebDay > daysInMo) h.day = daysInMo;
                GregorianDate g = HebrewToGregorian(h);
                if (g.year == gregYear)
                {
                    if (ev.afterSunset)
                    {
                        long jd = GregorianToJDN(g) - 1;
                        g = JDNToGregorian(jd);
                    }
                    row.greg = g;
                    row.heb  = GregorianToHebrew(g);
                    rows.push_back(row);
                    break;
                }
            }
        }
        else if (ev.gregMonth != 0)
        {
            // Gregorian-based
            if (!ev.observeAnnually && ev.gregYear != 0 && ev.gregYear != gregYear) continue;
            GregorianDate g; g.year = gregYear; g.month = ev.gregMonth; g.day = ev.gregDay;
            row.greg = g;
            row.heb  = GregorianToHebrew(g);
            rows.push_back(row);
        }
    }

    if (separateCategories)
    {
        // Sort by category order (Birthday=0, Anniversary=1, Yahrzeit=2, Custom=3),
        // then chronologically within each category
        static const int kOrder[] = { 0, 1, 2, 3 };
        std::sort(rows.begin(), rows.end(), [](const EventListRow& a, const EventListRow& b) {
            if (a.typeOrd != b.typeOrd) return a.typeOrd < b.typeOrd;
            if (a.greg.month != b.greg.month) return a.greg.month < b.greg.month;
            return a.greg.day < b.greg.day;
        });
    }
    else
    {
        std::sort(rows.begin(), rows.end(), [](const EventListRow& a, const EventListRow& b) {
            if (a.greg.month != b.greg.month) return a.greg.month < b.greg.month;
            return a.greg.day < b.greg.day;
        });
    }
    return rows;
}

void DrawEventsListPage(CDC* pDC, const CRect& rcPage,
                        const std::vector<UserEventEntry>& events,
                        int gregYear, int pageIndex, int totalPages,
                        bool showFooter,
                        uint8_t categoryMask, bool separateCategories)
{
    const int W  = rcPage.Width();
    const int H  = rcPage.Height();
    const int x0 = rcPage.left;
    const int y0 = rcPage.top;

    pDC->FillSolidRect(rcPage, RGB(255, 255, 255));

    int fTitle = -(H * 4 / 100);   if (fTitle > -10) fTitle = -10;
    int fHdr   = -(H * 28/1000);   if (fHdr   > -7)  fHdr   = -7;
    int fRow   = -(H * 24/1000);   if (fRow   > -6)  fRow   = -6;

    CFont fontTitle, fontHdr, fontRow;
    fontTitle.CreateFont(fTitle,0,0,0,FW_BOLD,  0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
    fontHdr  .CreateFont(fHdr,  0,0,0,FW_BOLD,  0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
    fontRow  .CreateFont(fRow,  0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");

    // Header bar
    int hdrH = H * 8 / 100;
    pDC->FillSolidRect(CRect(x0, y0, x0+W, y0+hdrH), RGB(50, 80, 140));
    pDC->SetBkMode(TRANSPARENT);
    pDC->SetTextColor(RGB(255, 255, 255));
    pDC->SelectObject(&fontTitle);
    CString title; title.Format(L"Events for %d", gregYear);
    DrawTextFit(pDC, title, CRect(x0+8, y0, x0+W-8, y0+hdrH), DT_LEFT|DT_VCENTER|DT_SINGLELINE);

    // Column layout: Greg date | Heb date | Type | Name
    int colGreg  = x0 + 4;
    int wGreg    = W * 18 / 100;
    int colHeb   = colGreg + wGreg + 4;
    int wHeb     = W * 22 / 100;
    int colType  = colHeb + wHeb + 4;
    int wType    = W * 16 / 100;
    int colName  = colType + wType + 4;
    int wName    = x0 + W - colName - 4;

    // Column header row
    int y = y0 + hdrH + 2;
    int rowH = -fRow + 4;
    pDC->FillSolidRect(CRect(x0, y, x0+W, y+rowH+2), RGB(200, 212, 240));
    pDC->SetTextColor(RGB(30, 50, 110));
    pDC->SelectObject(&fontHdr);
    DrawTextFit(pDC, L"Date",   CRect(colGreg, y, colGreg+wGreg, y+rowH+2), DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    DrawTextFit(pDC, L"Hebrew", CRect(colHeb,  y, colHeb +wHeb,  y+rowH+2), DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    DrawTextFit(pDC, L"Type",   CRect(colType, y, colType+wType, y+rowH+2), DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    DrawTextFit(pDC, L"Event",  CRect(colName, y, colName+wName, y+rowH+2), DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    y += rowH + 4;

    // Build rows for this year, calculate which ones fall on this page
    auto rows = BuildEventRows(events, gregYear, categoryMask, separateCategories);
    int footerH = showFooter ? (H * 4 / 100 + 4) : 0;
    int contentBottom = y0 + H - footerH;
    int rowsPerPage = (contentBottom - y) / (rowH + 2);
    if (rowsPerPage < 1) rowsPerPage = 1;
    int startIdx = pageIndex * rowsPerPage;
    int endIdx   = min((int)rows.size(), startIdx + rowsPerPage);

    pDC->SelectObject(&fontRow);
    bool altRow = false;
    int lastTypeOrd = -1;
    static const wchar_t* kCatHeaders[] = { L"Birthdays", L"Anniversaries", L"Yahrzeits", L"Custom Events" };
    for (int i = startIdx; i < endIdx; ++i)
    {
        const auto& r = rows[i];
        if (y + rowH > contentBottom) break;

        // Category header when separating by type
        if (separateCategories && r.typeOrd != lastTypeOrd)
        {
            lastTypeOrd = r.typeOrd;
            if (y + rowH + 2 > contentBottom) break;
            pDC->FillSolidRect(CRect(x0, y, x0+W, y+rowH), RGB(80, 110, 160));
            pDC->SelectObject(&fontHdr);
            pDC->SetBkMode(TRANSPARENT);
            pDC->SetTextColor(RGB(255, 255, 255));
            const wchar_t* hdr = (r.typeOrd >= 0 && r.typeOrd <= 3) ? kCatHeaders[r.typeOrd] : L"Events";
            DrawTextFit(pDC, hdr, CRect(x0+8, y, x0+W-8, y+rowH), DT_LEFT|DT_VCENTER|DT_SINGLELINE);
            y += rowH + 2;
            altRow = false;
            pDC->SelectObject(&fontRow);
            if (y + rowH > contentBottom) break;
        }

        if (altRow)
            pDC->FillSolidRect(CRect(x0, y, x0+W, y+rowH), RGB(245, 247, 255));

        // Gregorian date
        CString gs; gs.Format(L"%s %d, %d",
            GregorianMonthName(r.greg.month).c_str(), r.greg.day, r.greg.year);
        pDC->SetTextColor(RGB(50, 50, 50));
        DrawTextFit(pDC, gs, CRect(colGreg, y, colGreg+wGreg, y+rowH), DT_LEFT|DT_TOP|DT_SINGLELINE);

        // Hebrew date
        bool leap = IsHebrewLeapYear(r.heb.year);
        CString hs; hs.Format(L"%d %s %d",
            r.heb.day, HebrewMonthName(r.heb.month, leap).c_str(), r.heb.year);
        pDC->SetTextColor(RGB(30, 30, 180));
        DrawTextFit(pDC, hs, CRect(colHeb, y, colHeb+wHeb, y+rowH), DT_LEFT|DT_TOP|DT_SINGLELINE);

        // Type
        pDC->SetTextColor(RGB(100, 60, 140));
        DrawTextFit(pDC, r.typeStr, CRect(colType, y, colType+wType, y+rowH), DT_LEFT|DT_TOP|DT_SINGLELINE);

        // Name
        pDC->SetTextColor(RGB(20, 20, 20));
        DrawTextFit(pDC, r.name, CRect(colName, y, colName+wName, y+rowH), DT_LEFT|DT_TOP|DT_SINGLELINE|DT_END_ELLIPSIS);

        y += rowH + 2;
        altRow = !altRow;
    }

    if (rows.empty())
    {
        pDC->SetTextColor(RGB(120, 120, 120));
        pDC->SelectObject(&fontRow);
        pDC->DrawText(L"No events found for this year.", CRect(x0+8, y, x0+W-8, y+rowH*3), DT_LEFT|DT_TOP|DT_WORDBREAK);
    }

    // Footer
    if (showFooter)
    {
        int fy = y0 + H - H * 4 / 100;
        CPen penLine(PS_SOLID, 1, RGB(180, 180, 180));
        CPen* pOld = pDC->SelectObject(&penLine);
        pDC->MoveTo(x0, fy); pDC->LineTo(x0+W, fy);
        pDC->SelectObject(pOld);
        fy += 3;
        CFont fontFoot;
        fontFoot.CreateFont(-(H*22/1000), 0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
        pDC->SelectObject(&fontFoot);
        pDC->SetTextColor(RGB(120,120,120));
        CString pg; pg.Format(L"Page %d of %d", pageIndex+1, totalPages);
        DrawTextFit(pDC, L"WinLuach - Hebrew Calendar", CRect(x0, fy, x0+W, y0+H), DT_LEFT|DT_TOP|DT_SINGLELINE);
        DrawTextFit(pDC, pg, CRect(x0, fy, x0+W-4, y0+H), DT_RIGHT|DT_TOP|DT_SINGLELINE);
    }
}

// Year-picker + print dialog for events list
bool DoPrintEventsList(CMainFrame* pFrame)
{
    if (!pFrame) return false;
    const auto& events = theApp.m_settings.userEvents;

    // Simple year-picker dialog
    int chosenYear = pFrame->m_today.year;
    {
        struct YearDlg : public CDialog
        {
            int year;
            CEdit m_edit;
            CSpinButtonCtrl m_spin;
            YearDlg(int y, CWnd* p) : CDialog(), year(y) { m_pParentWnd = p; }
            INT_PTR DoModal() override
            {
                struct T { DLGTEMPLATE t; WORD menu,cls; wchar_t title[24]; } b = {};
                b.t.style = WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|DS_CENTER;
                b.t.dwExtendedStyle = WS_EX_APPWINDOW;
                b.t.cx = 140; b.t.cy = 60;
                wcscpy_s(b.title, L"Choose Year");
                if (!InitModalIndirect((LPCDLGTEMPLATE)&b, m_pParentWnd)) return -1;
                return CDialog::DoModal();
            }
            BOOL OnInitDialog() override
            {
                CDialog::OnInitDialog();
                SetWindowText(L"Choose Year");
                CRect rc; GetClientRect(&rc);
                int bh = rc.Height()*40/100;
                int by = (rc.Height() - bh*2 - 4) / 2;
                int ew = 60, sw = 16, ex = (rc.Width()-ew-sw)/2;
                m_edit.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|ES_NUMBER, CRect(ex,by,ex+ew,by+bh), this, 100);
                m_spin.Create(WS_CHILD|WS_VISIBLE|UDS_SETBUDDYINT|UDS_ALIGNRIGHT|UDS_ARROWKEYS|UDS_NOTHOUSANDS,
                    CRect(ex+ew,by,ex+ew+sw,by+bh), this, 101);
                m_spin.SetBuddy(&m_edit);
                m_spin.SetRange32(1, 9999);
                m_spin.SetPos32(year);
                CString s; s.Format(L"%d", year);
                m_edit.SetWindowText(s);
                CButton* ok = new CButton();
                ok->Create(L"Print...", WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
                    CRect(rc.Width()/2-28, by+bh+4, rc.Width()/2+28, by+bh*2+4), this, IDOK);
                return TRUE;
            }
            void OnOK() override
            {
                CString s; m_edit.GetWindowText(s);
                year = _wtoi(s);
                if (year < 1) year = 1;
                CDialog::OnOK();
            }
        } dlg(chosenYear, pFrame);
        if (dlg.DoModal() != IDOK) return false;
        chosenYear = dlg.year;
    }

    // Estimate total pages using approximate portrait page dimensions (8.5"x11" at 100dpi)
    auto rows = BuildEventRows(events, chosenYear);
    int totalPages = 1;
    {
        int approxH = 1100;
        float mTop = 0.75f, mBot = 0.75f;
        int contentH = approxH - (int)(approxH * mTop / 11.0f) - (int)(approxH * mBot / 11.0f);
        int fRow = -(contentH * 24/1000); if (fRow > -6) fRow = -6;
        int hdrH = contentH * 8 / 100;
        int rowH  = -fRow + 4;
        int firstRowY = hdrH + 2 + rowH + 4 + 2;
        int footerH = contentH * 4 / 100 + 4;
        int rpp = (contentH - firstRowY - footerH) / (rowH + 2); if (rpp < 1) rpp = 1;
        totalPages = rows.empty() ? 1 : ((int)rows.size() + rpp - 1) / rpp;
    }

    struct CEventsSetupDlg : public CDialog
    {
        std::vector<UserEventEntry> evts;
        int                         year;
        int                         totalPgs;
        SimplePageOpts              opts;
        uint8_t                     categoryMask       = 0x0F;
        bool                        separateCategories = false;
        CButton m_radPortrait, m_radLandscape;
        CEdit   m_editTop, m_editBot, m_editLeft, m_editRight;
        CButton m_btnPreview, m_chkFooter;
        CButton m_chkBirthday, m_chkAnniversary, m_chkYahrzeit, m_chkCustom;
        CButton m_chkSeparate;
        CWnd*   m_pOwner;

        enum {
            IDC_EP_PORT      = 260,
            IDC_EP_LAND      = 261,
            IDC_EP_TOP       = 262,
            IDC_EP_BOT       = 263,
            IDC_EP_LEFT      = 264,
            IDC_EP_RIGHT     = 265,
            IDC_EP_PREVIEW   = 266,
            IDC_EP_FOOTER    = 267,
            IDC_EP_BIRTHDAY  = 268,
            IDC_EP_ANNIV     = 269,
            IDC_EP_YAHRZEIT  = 270,
            IDC_EP_CUSTOM    = 271,
            IDC_EP_SEPARATE  = 272,
        };

        CEventsSetupDlg(const std::vector<UserEventEntry>& e, int y, int tp,
                        uint8_t catMask, bool sepCats, CWnd* p)
            : CDialog(), evts(e), year(y), totalPgs(tp),
              categoryMask(catMask), separateCategories(sepCats), m_pOwner(p)
        { m_pParentWnd = p; }

        INT_PTR DoModal() override
        {
            struct T { DLGTEMPLATE t; WORD menu,cls; wchar_t title[32]; } b = {};
            b.t.style = WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|DS_CENTER;
            b.t.dwExtendedStyle = WS_EX_APPWINDOW;
            b.t.cx = 230; b.t.cy = 260;
            wcscpy_s(b.title, L"Print Events List");
            if (!InitModalIndirect((LPCDLGTEMPLATE)&b, m_pParentWnd)) return -1;
            return CDialog::DoModal();
        }
        BOOL OnInitDialog() override
        {
            CDialog::OnInitDialog();
            SetWindowText(L"Print Events List");
            int lh = 16, pad = 8, ex = pad, ey = pad;
            HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

            auto mkLabel = [&](const wchar_t* text, int x, int y, int w, int h) {
                CStatic* s = new CStatic();
                s->Create(text, WS_CHILD|WS_VISIBLE, CRect(x,y,x+w,y+h), this, 0);
                s->SetFont(CFont::FromHandle(hF));
            };

            // Orientation
            m_radPortrait .Create(L"Portrait",  WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON|WS_GROUP,
                CRect(ex,ey,ex+70,ey+lh), this, IDC_EP_PORT);
            m_radPortrait.SetFont(CFont::FromHandle(hF));
            m_radLandscape.Create(L"Landscape", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
                CRect(ex+76,ey,ex+170,ey+lh), this, IDC_EP_LAND);
            m_radLandscape.SetFont(CFont::FromHandle(hF));
            m_radPortrait.SetCheck(BST_CHECKED);
            ey += lh+8;

            // Margins
            auto makeRow = [&](const wchar_t* lbl, CEdit& ed, float v) {
                mkLabel(lbl, ex, ey, 65, lh);
                CString vs; vs.Format(L"%.2f", v);
                ed.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,
                    CRect(ex+68,ey,ex+110,ey+lh), this, 0);
                ed.SetFont(CFont::FromHandle(hF));
                ed.SetWindowText(vs);
                ey += lh+4;
            };
            makeRow(L"Top margin:",    m_editTop,   opts.mTop);
            makeRow(L"Bottom margin:", m_editBot,   opts.mBot);
            makeRow(L"Left margin:",   m_editLeft,  opts.mLeft);
            makeRow(L"Right margin:",  m_editRight, opts.mRight);
            ey += 4;

            // Footer
            m_chkFooter.Create(L"Show footer", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,
                CRect(ex,ey,ex+130,ey+lh), this, IDC_EP_FOOTER);
            m_chkFooter.SetFont(CFont::FromHandle(hF));
            m_chkFooter.SetCheck(BST_CHECKED);
            ey += lh+10;

            // Category group
            mkLabel(L"Include categories:", ex, ey, 130, lh); ey += lh+2;
            m_chkBirthday.Create(L"Birthdays", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,
                CRect(ex+8,ey,ex+100,ey+lh), this, IDC_EP_BIRTHDAY);
            m_chkBirthday.SetFont(CFont::FromHandle(hF));
            m_chkBirthday.SetCheck((categoryMask & 1) ? BST_CHECKED : BST_UNCHECKED);
            m_chkAnniversary.Create(L"Anniversaries", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,
                CRect(ex+104,ey,ex+210,ey+lh), this, IDC_EP_ANNIV);
            m_chkAnniversary.SetFont(CFont::FromHandle(hF));
            m_chkAnniversary.SetCheck((categoryMask & 2) ? BST_CHECKED : BST_UNCHECKED);
            ey += lh+4;
            m_chkYahrzeit.Create(L"Yahrzeits", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,
                CRect(ex+8,ey,ex+100,ey+lh), this, IDC_EP_YAHRZEIT);
            m_chkYahrzeit.SetFont(CFont::FromHandle(hF));
            m_chkYahrzeit.SetCheck((categoryMask & 4) ? BST_CHECKED : BST_UNCHECKED);
            m_chkCustom.Create(L"Custom events", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,
                CRect(ex+104,ey,ex+210,ey+lh), this, IDC_EP_CUSTOM);
            m_chkCustom.SetFont(CFont::FromHandle(hF));
            m_chkCustom.SetCheck((categoryMask & 8) ? BST_CHECKED : BST_UNCHECKED);
            ey += lh+8;

            // Separate categories
            m_chkSeparate.Create(L"Group by category", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,
                CRect(ex,ey,ex+160,ey+lh), this, IDC_EP_SEPARATE);
            m_chkSeparate.SetFont(CFont::FromHandle(hF));
            m_chkSeparate.SetCheck(separateCategories ? BST_CHECKED : BST_UNCHECKED);
            ey += lh+10;

            // Buttons
            m_btnPreview.Create(L"Preview", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                CRect(ex,ey,ex+60,ey+22), this, IDC_EP_PREVIEW);
            m_btnPreview.SetFont(CFont::FromHandle(hF));
            CButton* ok = new CButton();
            ok->Create(L"Print", WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
                CRect(ex+64,ey,ex+124,ey+22), this, IDOK);
            ok->SetFont(CFont::FromHandle(hF));
            CButton* cancel = new CButton();
            cancel->Create(L"Cancel", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                CRect(ex+128,ey,ex+195,ey+22), this, IDCANCEL);
            cancel->SetFont(CFont::FromHandle(hF));
            return TRUE;
        }

        void ReadOpts()
        {
            opts.landscape = (m_radLandscape.GetSafeHwnd() && m_radLandscape.GetCheck() == BST_CHECKED);
            auto getF = [](CEdit& e) -> float {
                CString s; e.GetWindowText(s); float v = (float)_wtof(s);
                return (v >= 0.0f && v < 3.0f) ? v : 0.0f;
            };
            opts.mTop   = getF(m_editTop);
            opts.mBot   = getF(m_editBot);
            opts.mLeft  = getF(m_editLeft);
            opts.mRight = getF(m_editRight);
            categoryMask = 0;
            if (m_chkBirthday.GetSafeHwnd()   && m_chkBirthday.GetCheck()   == BST_CHECKED) categoryMask |= 1;
            if (m_chkAnniversary.GetSafeHwnd() && m_chkAnniversary.GetCheck()== BST_CHECKED) categoryMask |= 2;
            if (m_chkYahrzeit.GetSafeHwnd()    && m_chkYahrzeit.GetCheck()   == BST_CHECKED) categoryMask |= 4;
            if (m_chkCustom.GetSafeHwnd()      && m_chkCustom.GetCheck()     == BST_CHECKED) categoryMask |= 8;
            separateCategories = (m_chkSeparate.GetSafeHwnd() && m_chkSeparate.GetCheck() == BST_CHECKED);
        }

        void OnPreview()
        {
            ReadOpts();
            bool sf = (m_chkFooter.GetSafeHwnd() && m_chkFooter.GetCheck() == BST_CHECKED);
            auto& ev2 = evts; int yr = year; int tp = totalPgs;
            uint8_t cm = categoryMask; bool sc = separateCategories;
            PageRenderFn fn = [ev2,yr,tp,sf,cm,sc](CDC* dc, const CRect& rc, bool /*ignored*/) {
                DrawEventsListPage(dc, rc, ev2, yr, 0, tp, sf, cm, sc);
            };
            CSimplePreviewDlg prev(fn, L"WinLuach Events", !opts.landscape, sf, m_pParentWnd);
            prev.DoModal();
        }

        LRESULT WindowProc(UINT msg, WPARAM wp, LPARAM lp) override
        {
            if (msg == WM_COMMAND && LOWORD(wp) == IDC_EP_PREVIEW)
            {
                OnPreview();
                return 0;
            }
            return CDialog::WindowProc(msg, wp, lp);
        }

        void OnOK() override
        {
            ReadOpts();
            bool sf = (m_chkFooter.GetSafeHwnd() && m_chkFooter.GetCheck() == BST_CHECKED);

            CPrintDialog pd(FALSE);
            pd.m_pd.Flags |= PD_RETURNDC | PD_NOSELECTION | PD_NOPAGENUMS;
            if (pd.DoModal() != IDOK) { CDialog::OnCancel(); return; }
            HDC hDC = pd.GetPrinterDC();
            if (!hDC) { CDialog::OnCancel(); return; }

            CDC dc; dc.Attach(hDC);
            int pw = dc.GetDeviceCaps(HORZRES);
            int ph = dc.GetDeviceCaps(VERTRES);
            int dpiX = dc.GetDeviceCaps(LOGPIXELSX);
            int dpiY = dc.GetDeviceCaps(LOGPIXELSY);

            int mL = (int)(opts.mLeft  * dpiX);
            int mR = (int)(opts.mRight * dpiX);
            int mT = (int)(opts.mTop   * dpiY);
            int mB = (int)(opts.mBot   * dpiY);

            DOCINFOW di = {}; di.cbSize = sizeof(di);
            CString docName; docName.Format(L"WinLuach Events %d", year);
            di.lpszDocName = docName;
            dc.StartDoc(&di);

            auto& ev2 = evts; int yr = year;

            // Recompute total pages on the actual printer DC
            {
                int fRow = -(ph * 24/1000); if (fRow > -6) fRow = -6;
                int hdrH = ph * 8 / 100;
                int rowH  = -fRow + 4;
                int firstRowY = hdrH + 2 + rowH + 4 + 2;
                int footerH = sf ? (ph * 4/100 + 4) : 0;
                int usable  = ph - mT - mB - firstRowY - footerH;
                int rpp = usable / (rowH + 2); if (rpp < 1) rpp = 1;
                auto rows2 = BuildEventRows(ev2, yr, categoryMask, separateCategories);
                totalPgs = rows2.empty() ? 1 : ((int)rows2.size() + rpp - 1) / rpp;
            }

            int tp2 = totalPgs;
            for (int pg = 0; pg < tp2; ++pg)
            {
                dc.StartPage();
                CRect rcPage(mL, mT, pw-mR, ph-mB);
                DrawEventsListPage(&dc, rcPage, ev2, yr, pg, tp2, sf, categoryMask, separateCategories);
                dc.EndPage();
            }
            dc.EndDoc();
            dc.Detach(); ::DeleteDC(hDC);
            CDialog::OnOK();
        }

    };

    // Load persisted category settings
    uint8_t catMask = 0x0F; bool sepCats = false;
    if (pFrame)
    {
        catMask  = theApp.m_settings.printEventCategoryMask;
        sepCats  = theApp.m_settings.printEventSeparateCategories;
    }
    CEventsSetupDlg dlg(events, chosenYear, totalPages, catMask, sepCats, pFrame);
    if (dlg.DoModal() == IDOK)
    {
        // Persist chosen options
        if (pFrame)
        {
            theApp.m_settings.printEventCategoryMask       = dlg.categoryMask;
            theApp.m_settings.printEventSeparateCategories = dlg.separateCategories;
            SaveSettings(theApp.m_settings);
        }
    }
    return true;
}

bool DoPrintZmanimMonth(int year, int month, CMainFrame* pFrame, uint32_t colMask)
{
    auto pU24 = std::make_shared<bool>(pFrame ? pFrame->m_use24hr : true);
    CSimplePageSetupDlg dlg(
        [=](CDC* pDC, const CRect& rc, bool sf){
            DrawZmanimMonthPage(pDC, rc, year, month, pFrame, colMask, *pU24, sf);
        },
        L"WinLuach Zmanim", true, pFrame);
    dlg.SetUse24HrPtr(pU24);
    dlg.DoModal();
    return true;
}

bool DoPrintDay(const GregorianDate& g, CMainFrame* pFrame)
{
    auto pMask = std::make_shared<uint64_t>(
        NormalizeDayPrintZmanMask(theApp.m_settings.printDayZmanimMask));
    CSimplePageSetupDlg dlg(
        [=](CDC* pDC, const CRect& rc, bool sf){
            DrawDayPage(pDC, rc, g, pFrame, sf, *pMask);
        },
        L"WinLuach Day Details", theApp.m_settings.dayDetailLandscape, pFrame);
    SimplePageOpts dayOpts;
    dayOpts.landscape = theApp.m_settings.dayDetailLandscape;
    dayOpts.mTop      = theApp.m_settings.dayDetailMarginTop;
    dayOpts.mBot      = theApp.m_settings.dayDetailMarginBot;
    dayOpts.mLeft     = theApp.m_settings.dayDetailMarginLeft;
    dayOpts.mRight    = theApp.m_settings.dayDetailMarginRight;
    dlg.SetInitialOpts(dayOpts);
    dlg.SetInitialShowFooter(theApp.m_settings.dayDetailShowFooter);
    dlg.SetDayZmanimMaskPtr(pMask);
    INT_PTR result = dlg.DoModal();
    // Always persist UI selections (even on cancel) so the user doesn't lose their choices
    {
        const SimplePageOpts& fo = dlg.GetFinalOpts();
        theApp.m_settings.dayDetailLandscape   = fo.landscape;
        theApp.m_settings.dayDetailMarginTop   = fo.mTop;
        theApp.m_settings.dayDetailMarginBot   = fo.mBot;
        theApp.m_settings.dayDetailMarginLeft  = fo.mLeft;
        theApp.m_settings.dayDetailMarginRight = fo.mRight;
        theApp.m_settings.dayDetailShowFooter  = dlg.GetFinalShowFooter();
        theApp.m_settings.printDayZmanimMask   = NormalizeDayPrintZmanMask(*pMask);
        SaveSettings(theApp.m_settings);
    }
    if (result == IDOK)
    {
    }
    return result == IDOK;
}

static bool PrintDayPages(const std::vector<GregorianDate>& pages,
                          CMainFrame* pFrame,
                          const SimplePageOpts& opts,
                          bool showFooter,
                          uint64_t zmanimMask)
{
    if (pages.empty())
        return false;

    CPrintDialog pd(FALSE);
    pd.m_pd.Flags |= PD_RETURNDC | PD_NOPAGENUMS | PD_NOCURRENTPAGE | PD_ALLPAGES;

    HGLOBAL hDM = GlobalAlloc(GHND, sizeof(DEVMODE));
    if (hDM)
    {
        DEVMODE* dm = (DEVMODE*)GlobalLock(hDM);
        if (dm)
        {
            ZeroMemory(dm, sizeof(DEVMODE));
            dm->dmSize = sizeof(DEVMODE);
            dm->dmFields = DM_ORIENTATION;
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
    int pageW = dc.GetDeviceCaps(HORZRES);
    int pageH = dc.GetDeviceCaps(VERTRES);
    int dpiX = dc.GetDeviceCaps(LOGPIXELSX);
    int dpiY = dc.GetDeviceCaps(LOGPIXELSY);
    CRect rcPage((int)(opts.mLeft * dpiX), (int)(opts.mTop * dpiY),
        pageW - (int)(opts.mRight * dpiX), pageH - (int)(opts.mBot * dpiY));
    if (rcPage.IsRectEmpty()) rcPage = CRect(0, 0, pageW, pageH);

    DOCINFO di = { sizeof(DOCINFO) };
    di.lpszDocName = L"WinLuach Selected Day Details";
    dc.StartDoc(&di);
    for (const auto& g : pages)
    {
        dc.StartPage();
        DrawDayPage(&dc, rcPage, g, pFrame, showFooter, zmanimMask);
        dc.EndPage();
    }
    dc.EndDoc();
    dc.Detach();
    ::DeleteDC(hDC);
    if (hDM) GlobalFree(hDM);
    return true;
}

class CMultiDayPreviewDlg : public CDialog
{
public:
    CMultiDayPreviewDlg(const std::vector<GregorianDate>& pages,
                        CMainFrame* frame,
                        const SimplePageOpts& opts,
                        bool showFooter,
                        uint64_t zmanimMask,
                        CWnd* parent = nullptr)
        : CDialog(), m_pages(pages), m_pFrame(frame), m_opts(opts),
          m_showFooter(showFooter),
          m_zmanimMask(NormalizeDayPrintZmanMask(zmanimMask))
    {
        m_pParentWnd = parent;
    }

    INT_PTR DoModal() override
    {
        struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[40]; } b = {};
        b.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_CENTER;
        b.t.dwExtendedStyle = WS_EX_APPWINDOW;
        b.t.cx = 520; b.t.cy = 420;
        wcscpy_s(b.title, L"Print Preview");
        if (!InitModalIndirect((DLGTEMPLATE*)&b, m_pParentWnd)) return -1;
        return CDialog::DoModal();
    }

protected:
    BOOL OnInitDialog() override
    {
        CDialog::OnInitDialog();
        SetWindowText(L"Selected Days Print Preview");
        HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CFont* pF = CFont::FromHandle(hF);
        CRect rc; GetClientRect(&rc);
        int W = rc.Width(), H = rc.Height();
        const int bh = 32, bw = 80, margin = 8;
        int y = H - bh - margin;

        m_btnPrev.Create(L"Previous", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            CRect(10, y, 90, y + bh), this, IDC_MDP_PREV);
        m_btnPrev.SetFont(pF);
        m_btnNext.Create(L"Next", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            CRect(96, y, 176, y + bh), this, IDC_MDP_NEXT);
        m_btnNext.SetFont(pF);
        m_lblPage.Create(L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
            CRect(190, y + 8, W - 190, y + bh), this, IDC_MDP_LABEL);
        m_lblPage.SetFont(pF);
        m_btnPrint.Create(L"Print...", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            CRect(W - 180, y, W - 100, y + bh), this, IDC_MDP_PRINT);
        m_btnPrint.SetFont(pF);
        m_btnClose.Create(L"Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            CRect(W - 92, y, W - 12, y + bh), this, IDCANCEL);
        m_btnClose.SetFont(pF);
        UpdateButtons();
        return TRUE;
    }

    afx_msg void OnPaint()
    {
        CPaintDC dc(this);
        CRect rc; GetClientRect(&rc);
        const int BTN_STRIP = 44;
        CRect previewRc(0, 0, rc.right, rc.bottom - BTN_STRIP);
        dc.FillSolidRect(previewRc, RGB(100, 100, 100));

        float aspect = m_opts.landscape ? (8.5f / 11.0f) : (11.0f / 8.5f);
        int avW = previewRc.Width() - 20;
        int avH = previewRc.Height() - 20;
        int pageW, pageH;
        if ((float)avH / avW > aspect) { pageW = avW; pageH = (int)(avW * aspect); }
        else                           { pageH = avH; pageW = (int)(avH / aspect); }
        int pageX = previewRc.left + (previewRc.Width() - pageW) / 2;
        int pageY = previewRc.top + (previewRc.Height() - pageH) / 2;
        CRect rcPage(pageX, pageY, pageX + pageW, pageY + pageH);

        CRect shadow = rcPage; shadow.OffsetRect(5, 5);
        dc.FillSolidRect(shadow, RGB(50, 50, 50));
        dc.FillSolidRect(rcPage, RGB(255, 255, 255));

        int marg = max(6, pageW / 25);
        CRect rcContent = rcPage;
        rcContent.DeflateRect(marg, marg);
        if (!m_pages.empty())
            DrawDayPage(&dc, rcContent, m_pages[m_pageIndex], m_pFrame,
                m_showFooter, m_zmanimMask);

        CRect btnRc(0, rc.bottom - BTN_STRIP, rc.right, rc.bottom);
        dc.FillSolidRect(btnRc, ::GetSysColor(COLOR_BTNFACE));
    }

    afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }

    afx_msg void OnSize(UINT nType, int cx, int cy)
    {
        CDialog::OnSize(nType, cx, cy);
        if (!m_btnPrev.GetSafeHwnd()) return;
        const int bh = 32, margin = 8;
        int y = cy - bh - margin;
        m_btnPrev.SetWindowPos(nullptr, 10, y, 80, bh, SWP_NOZORDER);
        m_btnNext.SetWindowPos(nullptr, 96, y, 80, bh, SWP_NOZORDER);
        m_lblPage.SetWindowPos(nullptr, 190, y + 8, max(20, cx - 380), 20, SWP_NOZORDER);
        m_btnPrint.SetWindowPos(nullptr, cx - 180, y, 80, bh, SWP_NOZORDER);
        m_btnClose.SetWindowPos(nullptr, cx - 92, y, 80, bh, SWP_NOZORDER);
        Invalidate(FALSE);
    }

    afx_msg void OnPrev()
    {
        if (m_pageIndex > 0)
        {
            --m_pageIndex;
            UpdateButtons();
            Invalidate(FALSE);
        }
    }

    afx_msg void OnNext()
    {
        if (m_pageIndex + 1 < (int)m_pages.size())
        {
            ++m_pageIndex;
            UpdateButtons();
            Invalidate(FALSE);
        }
    }

    afx_msg void OnPrint()
    {
        PrintDayPages(m_pages, m_pFrame, m_opts, m_showFooter, m_zmanimMask);
    }

    DECLARE_MESSAGE_MAP()

private:
    void UpdateButtons()
    {
        if (m_btnPrev.GetSafeHwnd())
            m_btnPrev.EnableWindow(m_pageIndex > 0 ? TRUE : FALSE);
        if (m_btnNext.GetSafeHwnd())
            m_btnNext.EnableWindow(m_pageIndex + 1 < (int)m_pages.size() ? TRUE : FALSE);
        if (m_lblPage.GetSafeHwnd())
        {
            CString s;
            s.Format(L"Page %d of %d", m_pageIndex + 1, (int)m_pages.size());
            m_lblPage.SetWindowText(s);
        }
    }

    enum {
        IDC_MDP_PREV = 1101,
        IDC_MDP_NEXT = 1102,
        IDC_MDP_PRINT = 1103,
        IDC_MDP_LABEL = 1104,
    };

    std::vector<GregorianDate> m_pages;
    CMainFrame* m_pFrame = nullptr;
    SimplePageOpts m_opts;
    bool m_showFooter = true;
    uint64_t m_zmanimMask = UINT64_MAX;
    int m_pageIndex = 0;
    CButton m_btnPrev, m_btnNext, m_btnPrint, m_btnClose;
    CStatic m_lblPage;
};

BEGIN_MESSAGE_MAP(CMultiDayPreviewDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_BN_CLICKED(CMultiDayPreviewDlg::IDC_MDP_PREV, &CMultiDayPreviewDlg::OnPrev)
    ON_BN_CLICKED(CMultiDayPreviewDlg::IDC_MDP_NEXT, &CMultiDayPreviewDlg::OnNext)
    ON_BN_CLICKED(CMultiDayPreviewDlg::IDC_MDP_PRINT, &CMultiDayPreviewDlg::OnPrint)
END_MESSAGE_MAP()

class CMultiDayPageSetupDlg : public CDialog
{
public:
    SimplePageOpts opts;
    bool showFooter = true;
    uint64_t dayZmanimMask = UINT64_MAX;

    CMultiDayPageSetupDlg(const std::vector<GregorianDate>& pages,
                          CMainFrame* frame,
                          CWnd* parent = nullptr)
        : CDialog(), m_pages(pages), m_pFrame(frame)
    {
        m_pParentWnd = parent;
        opts.landscape = theApp.m_settings.dayDetailLandscape;
        opts.mTop      = theApp.m_settings.dayDetailMarginTop;
        opts.mBot      = theApp.m_settings.dayDetailMarginBot;
        opts.mLeft     = theApp.m_settings.dayDetailMarginLeft;
        opts.mRight    = theApp.m_settings.dayDetailMarginRight;
        showFooter     = theApp.m_settings.dayDetailShowFooter;
        dayZmanimMask  = NormalizeDayPrintZmanMask(theApp.m_settings.printDayZmanimMask);
    }

    INT_PTR DoModal() override
    {
    struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[32]; } b = {};
    b.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_CENTER;
    b.t.dwExtendedStyle = WS_EX_APPWINDOW;
    b.t.cx = 470; b.t.cy = 390;
        wcscpy_s(b.title, L"Page Setup");
        if (!InitModalIndirect((DLGTEMPLATE*)&b, m_pParentWnd)) return -1;
        return CDialog::DoModal();
    }

protected:
    BOOL OnInitDialog() override
    {
        CDialog::OnInitDialog();
        SetWindowText(L"Selected Days Page Setup");
        HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CFont* pF = CFont::FromHandle(hF);
        CRect rc; GetClientRect(&rc);
        int W = rc.Width(), H = rc.Height();

        auto mkStatic = [&](const wchar_t* txt, int x, int y, int w, int h) {
            CStatic* s = new CStatic;
            s->Create(txt, WS_CHILD|WS_VISIBLE|SS_LEFT, CRect(x, y, x+w, y+h), this);
            s->SetFont(pF);
        };
        auto mkRadio = [&](CButton& b, const wchar_t* txt, UINT id, int x, int y, int w, bool first, bool checked) {
            DWORD style = WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTORADIOBUTTON;
            if (first) style |= WS_GROUP;
            b.Create(txt, style, CRect(x, y, x+w, y+18), this, id);
            b.SetFont(pF);
            b.SetCheck(checked ? BST_CHECKED : BST_UNCHECKED);
        };
        auto mkEdit = [&](CEdit& e, UINT id, float v, int x, int y, int w) {
            CString s; s.Format(L"%.2f", v);
            e.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_CENTER,
                CRect(x, y, x+w, y+20), this, id);
            e.SetFont(pF);
            e.SetWindowText(s);
        };
        int y = 10;
        mkStatic(L"Orientation", 8, y, 120, 16); y += 20;
        mkRadio(m_radPortrait,  L"Portrait",   1001, 20,  y, 100, true,  !opts.landscape);
        mkRadio(m_radLandscape, L"Landscape",  1002, 140, y, 120, false,  opts.landscape);
        y += 28;
        mkStatic(L"Margins (inches)", 8, y, 160, 16); y += 20;
        mkStatic(L"Top:", 8, y + 2, 34, 16); mkEdit(m_top, 1003, opts.mTop, 46, y, 50);
        mkStatic(L"Bottom:", 115, y + 2, 52, 16); mkEdit(m_bot, 1004, opts.mBot, 168, y, 50);
        y += 26;
        mkStatic(L"Left:", 8, y + 2, 34, 16); mkEdit(m_left, 1005, opts.mLeft, 46, y, 50);
        mkStatic(L"Right:", 115, y + 2, 52, 16); mkEdit(m_right, 1006, opts.mRight, 168, y, 50);
        y += 30;
        m_footer.Create(L"Show footer", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
            CRect(8, y, 160, y+20), this, 1007);
        m_footer.SetFont(pF);
        m_footer.SetCheck(showFooter ? BST_CHECKED : BST_UNCHECKED);
        y += 28;

        CButton* group = new CButton;
        group->Create(L"Zmanim to include",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            CRect(8, y, W - 8, y + 222), this, 1010);
        group->SetFont(pF);
        y += 22;

        uint64_t mask = NormalizeDayPrintZmanMask(dayZmanimMask);
        m_zmanChecks.clear();
        m_zmanChecks.reserve(kDayPrintZmanCount);
        for (int i = 0; i < kDayPrintZmanCount; ++i)
        {
            int col = i / 12;
            int row = i % 12;
            int x0 = 18 + col * 148;
            int yy = y + row * 16;
            CButton* chk = new CButton;
            chk->Create(kDayPrintZmanLabels[i],
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                CRect(x0, yy, x0 + 144, yy + 16), this, 1100 + i);
            chk->SetFont(pF);
            chk->SetCheck((mask & (1ull << i)) ? BST_CHECKED : BST_UNCHECKED);
            m_zmanChecks.push_back(chk);
        }

        CButton* preview = new CButton;
        preview->Create(L"Preview", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
            CRect(8, H - 34, 86, H - 8), this, 1008);
        preview->SetFont(pF);
        CButton* ok = new CButton;
        ok->Create(L"Print...", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
            CRect(W - 166, H - 34, W - 88, H - 8), this, IDOK);
        ok->SetFont(pF);
        CButton* cancel = new CButton;
        cancel->Create(L"Cancel", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
            CRect(W - 82, H - 34, W - 8, H - 8), this, IDCANCEL);
        cancel->SetFont(pF);
        return TRUE;
    }

    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override
    {
        if (LOWORD(wParam) == 1008)
        {
            ReadControls();
            if (!m_pages.empty())
            {
                CMultiDayPreviewDlg prev(m_pages, m_pFrame, opts,
                    showFooter, dayZmanimMask, this);
                prev.DoModal();
            }
            return TRUE;
        }
        return CDialog::OnCommand(wParam, lParam);
    }

    void OnOK() override
    {
        ReadControls();
        CDialog::OnOK();
    }

    void OnCancel() override
    {
        ReadControls();
        CDialog::OnCancel();
    }

private:
    void ReadControls()
    {
        opts.landscape = (m_radLandscape.GetCheck() == BST_CHECKED);
        auto getF = [](CEdit& e, float def) {
            CString s; e.GetWindowText(s);
            float v = (float)_wtof(s);
            return (v >= 0.0f && v < 5.0f) ? v : def;
        };
        opts.mTop = getF(m_top, 0.5f);
        opts.mBot = getF(m_bot, 0.5f);
        opts.mLeft = getF(m_left, 0.5f);
        opts.mRight = getF(m_right, 0.5f);
        showFooter = (m_footer.GetCheck() == BST_CHECKED);
        uint64_t mask = 0;
        for (int i = 0; i < (int)m_zmanChecks.size() && i < kDayPrintZmanCount; ++i)
            if (m_zmanChecks[i] && m_zmanChecks[i]->GetSafeHwnd() &&
                m_zmanChecks[i]->GetCheck() == BST_CHECKED)
                mask |= (1ull << i);
        dayZmanimMask = NormalizeDayPrintZmanMask(mask);
    }

    std::vector<GregorianDate> m_pages;
    CMainFrame* m_pFrame = nullptr;
    CButton m_radPortrait, m_radLandscape, m_footer;
    CEdit m_top, m_bot, m_left, m_right;
    std::vector<CButton*> m_zmanChecks;
};

bool DoPrintDays(const std::vector<GregorianDate>& dates, CMainFrame* pFrame)
{
    if (dates.empty())
        return false;

    std::vector<GregorianDate> pages = dates;
    std::sort(pages.begin(), pages.end(), [](const GregorianDate& a, const GregorianDate& b) {
        return GregorianToJDN(a) < GregorianToJDN(b);
    });
    pages.erase(std::unique(pages.begin(), pages.end(), [](const GregorianDate& a, const GregorianDate& b) {
        return a.year == b.year && a.month == b.month && a.day == b.day;
    }), pages.end());

    CMultiDayPageSetupDlg setup(pages, pFrame, pFrame);
    INT_PTR mresult = setup.DoModal();
    // Always persist selections even on cancel
    theApp.m_settings.printDayZmanimMask   = NormalizeDayPrintZmanMask(setup.dayZmanimMask);
    theApp.m_settings.dayDetailLandscape   = setup.opts.landscape;
    theApp.m_settings.dayDetailShowFooter  = setup.showFooter;
    theApp.m_settings.dayDetailMarginTop   = setup.opts.mTop;
    theApp.m_settings.dayDetailMarginBot   = setup.opts.mBot;
    theApp.m_settings.dayDetailMarginLeft  = setup.opts.mLeft;
    theApp.m_settings.dayDetailMarginRight = setup.opts.mRight;
    SaveSettings(theApp.m_settings);
    if (mresult != IDOK) return false;
    return PrintDayPages(pages, pFrame, setup.opts, setup.showFooter,
        setup.dayZmanimMask);
}

// =============================================================================
// CSimplePageSetupDlg — orientation + margins + preview/print for day/zmanim
// =============================================================================

BEGIN_MESSAGE_MAP(CSimplePageSetupDlg, CDialog)
    ON_BN_CLICKED(CSimplePageSetupDlg::IDC_SPS_BTN_PREVIEW, &CSimplePageSetupDlg::OnPreview)
    ON_BN_CLICKED(IDOK, &CSimplePageSetupDlg::OnPrint)
END_MESSAGE_MAP()

CSimplePageSetupDlg::CSimplePageSetupDlg(
    PageRenderFn render,
    const wchar_t* docName, bool defaultLandscape, CWnd* pParent)
    : CDialog(), m_render(std::move(render)), m_docName(docName)
{
    m_opts.landscape = defaultLandscape;
    m_pParentWnd = pParent;
}

INT_PTR CSimplePageSetupDlg::DoModal()
{
    struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[32]; } b = {};
    b.t.style = WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|DS_CENTER;
    b.t.dwExtendedStyle = WS_EX_APPWINDOW;
    b.t.cx = m_pDayZmanimMask ? 470 : 310;
    b.t.cy = m_pDayZmanimMask ? 390 : 220;
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

    // Show Footer checkbox
    m_chkFooter.Create(L"Show footer", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
        CRect(8, y, 200, y+18), this, IDC_SPS_CHK_FOOTER);
    m_chkFooter.SetFont(pF);
    m_chkFooter.SetCheck(m_initialShowFooter ? BST_CHECKED : BST_UNCHECKED);
    y += 26;

    // 12/24-hr checkbox (only when caller passes a shared_ptr)
    if (m_pUse24hr)
    {
        m_chk24hr.Create(L"24-hour time", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
            CRect(8, y, 200, y+18), this, IDC_SPS_CHK_24HR);
        m_chk24hr.SetFont(pF);
        m_chk24hr.SetCheck(*m_pUse24hr ? BST_CHECKED : BST_UNCHECKED);
        y += 26;
    }

    if (m_pDayZmanimMask)
    {
        CButton* group = new CButton;
        group->Create(L"Zmanim to include",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            CRect(8, y, W - 8, y + 222), this, IDC_SPS_DAY_ZMAN_FIRST + 100);
        group->SetFont(pF);
        y += 22;

        uint64_t mask = NormalizeDayPrintZmanMask(*m_pDayZmanimMask);
        m_dayZmanChecks.clear();
        m_dayZmanChecks.reserve(kDayPrintZmanCount);
        for (int i = 0; i < kDayPrintZmanCount; ++i)
        {
            int col = i / 12;
            int row = i % 12;
            int cx0 = 18 + col * 148;
            int cy0 = y + row * 16;
            CButton* chk = new CButton;
            chk->Create(kDayPrintZmanLabels[i],
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                CRect(cx0, cy0, cx0 + 144, cy0 + 16), this,
                IDC_SPS_DAY_ZMAN_FIRST + i);
            chk->SetFont(pF);
            chk->SetCheck((mask & (1ull << i)) ? BST_CHECKED : BST_UNCHECKED);
            m_dayZmanChecks.push_back(chk);
        }
        y += 198;
    }

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
        return (v >= 0.0f && v < 5.0f) ? v : def;
    };
    m_opts.mTop   = getF(m_editTop,   0.0f);
    m_opts.mBot   = getF(m_editBot,   0.0f);
    m_opts.mLeft  = getF(m_editLeft,  0.0f);
    m_opts.mRight = getF(m_editRight, 0.0f);
    if (m_pUse24hr && m_chk24hr.GetSafeHwnd())
        *m_pUse24hr = (m_chk24hr.GetCheck() == BST_CHECKED);
    if (m_pDayZmanimMask)
    {
        uint64_t mask = 0;
        for (int i = 0; i < (int)m_dayZmanChecks.size() && i < kDayPrintZmanCount; ++i)
            if (m_dayZmanChecks[i] && m_dayZmanChecks[i]->GetSafeHwnd() &&
                m_dayZmanChecks[i]->GetCheck() == BST_CHECKED)
                mask |= (1ull << i);
        *m_pDayZmanimMask = NormalizeDayPrintZmanMask(mask);
    }
    m_finalShowFooter = (!m_chkFooter.GetSafeHwnd() || m_chkFooter.GetCheck() == BST_CHECKED);
}

void CSimplePageSetupDlg::OnPreview()
{
    ReadControls();
    bool sf = (!m_chkFooter.GetSafeHwnd() || m_chkFooter.GetCheck() == BST_CHECKED);
    CSimplePreviewDlg prev(m_render, m_docName.c_str(), !m_opts.landscape, sf, nullptr);
    prev.DoModal();
}

void CSimplePageSetupDlg::OnPrint()
{
    ReadControls();
    bool sf = (!m_chkFooter.GetSafeHwnd() || m_chkFooter.GetCheck() == BST_CHECKED);
    SimplePrint(m_docName.c_str(), m_opts, m_render, sf);
    CDialog::OnOK();
}

void CSimplePageSetupDlg::OnCancel()
{
    // Persist whatever the user had checked even if they cancelled
    ReadControls();
    CDialog::OnCancel();
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
    PageRenderFn render,
    const wchar_t* docName, bool portrait, bool showFooter, CWnd* pParent)
    : CDialog(), m_render(std::move(render)), m_docName(docName),
      m_portrait(portrait), m_showFooter(showFooter)
{
    m_pParentWnd = pParent;
}

INT_PTR CSimplePreviewDlg::DoModal()
{
    struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[40]; } b = {};
    b.t.style = WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|DS_CENTER;
    b.t.dwExtendedStyle = WS_EX_APPWINDOW;
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
    m_render(&dc, rcContent, m_showFooter);

    // Button strip background
    CRect btnRc(0, rc.bottom - BTN_STRIP, rc.right, rc.bottom);
    dc.FillSolidRect(btnRc, ::GetSysColor(COLOR_BTNFACE));
}

BOOL CSimplePreviewDlg::OnEraseBkgnd(CDC*) { return TRUE; }

void CSimplePreviewDlg::OnPrint()
{
    SimplePageOpts opts;
    opts.landscape = !m_portrait;
    SimplePrint(m_docName.c_str(), opts, m_render, m_showFooter);
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
    buf.t.style  = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_CENTER;
    buf.t.dwExtendedStyle = WS_EX_APPWINDOW;
    buf.t.cdit   = 0;
    buf.t.cx     = 310;
    buf.t.cy     = 450;
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
        m_opts.includeZmanim  = ps.printWeeklyZmanim;
        m_opts.zmanimColumns  = ps.printZmanimColMask;
        m_opts.showFooter     = ps.printShowFooter;
        m_opts.use24hr        = ps.use24Hour;
        m_opts.twoColumns     = ps.printTwoColumns;
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

    m_chkTwoColumns.Create(L"Use 2 columns for multi-month prints",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        CRect(20, y, W - 10, y + 18), this, IDC_PD_CHK_2COL);
    m_chkTwoColumns.SetFont(pF);
    m_chkTwoColumns.SetCheck(m_opts.twoColumns ? BST_CHECKED : BST_UNCHECKED);
    y += 24;

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
    y += 24;

    m_chk24hr.Create(L"Use 24-hour time for printed zmanim",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        CRect(20, y, W - 10, y + 18), this, IDC_PD_CHK_24HR);
    m_chk24hr.SetFont(pF);
    m_chk24hr.SetCheck(m_opts.use24hr ? BST_CHECKED : BST_UNCHECKED);
    y += 24;

    // ── Zmanim column selection ──────────────────────────────────────────────
    static const wchar_t* kColNames[15] = {
        L"Alos", L"Misheyakir", L"Netz",
        L"Shma MA", L"Shma GRA",
        L"Tfla MA", L"Tfla GRA",
        L"Chatzos", L"Mn. Gedola", L"Mn. Ketana",
        L"Plag", L"Candle", L"Shkia", L"Tzeis", L"Sha'a"
    };
    mkStatic(L"Zmanim columns:", 20, y, 120, 16); y += 18;
    {
        int colsPerRow = 5;
        int chkW = (W - 24) / colsPerRow;
        for (int i = 0; i < 15; i++)
        {
            int col = i % colsPerRow;
            int row = i / colsPerRow;
            int cx = 20 + col * chkW;
            int cy2 = y + row * 21;
            m_chkCol[i].Create(kColNames[i],
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                CRect(cx, cy2, cx + chkW - 4, cy2 + 18), this, IDC_PD_COL_0 + i);
            m_chkCol[i].SetFont(pF);
            m_chkCol[i].SetCheck((m_opts.zmanimColumns >> i) & 1 ? BST_CHECKED : BST_UNCHECKED);
        }
    }
    y += 3 * 21 + 8;

    // ── Show footer checkbox ─────────────────────────────────────────────────
    m_chkShowFooter.Create(L"Show footer",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        CRect(20, y, 130, y + 18), this, IDC_PD_CHK_FOOTER);
    m_chkShowFooter.SetFont(pF);
    m_chkShowFooter.SetCheck(m_opts.showFooter ? BST_CHECKED : BST_UNCHECKED);
    y += 24;

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

    m_opts.zmanimColumns = 0;
    for (int i = 0; i < 15; i++)
        if (m_chkCol[i].GetCheck() == BST_CHECKED)
            m_opts.zmanimColumns |= (1u << i);

    m_opts.showFooter = (m_chkShowFooter.GetCheck() == BST_CHECKED);
    m_opts.use24hr    = (m_chk24hr.GetCheck() == BST_CHECKED);
    m_opts.twoColumns = (m_chkTwoColumns.GetCheck() == BST_CHECKED);

    auto getFloat = [](CEdit& e, float def) {
        CString s; e.GetWindowText(s);
        float v = (float)_wtof(s);
        return (v >= 0.0f && v < 5.0f) ? v : def;
    };
    m_opts.mTop   = getFloat(m_editTop,   0.5f);
    m_opts.mBot   = getFloat(m_editBot,   0.5f);
    m_opts.mLeft  = getFloat(m_editLeft,  0.5f);
    m_opts.mRight = getFloat(m_editRight, 0.5f);
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
    ps.printWeeklyZmanim  = opts.includeZmanim;
    ps.printZmanimColMask = opts.zmanimColumns;
    ps.printShowFooter    = opts.showFooter;
    ps.use24Hour          = opts.use24hr;
    ps.printTwoColumns    = opts.twoColumns;
    SaveSettings(ps);
}

void CCalPrintDlg::OnOK()
{
    ReadControls();
    SavePrintOptsToSettings(m_opts);
    CDialog::OnOK();
}

void CCalPrintDlg::OnCancel()
{
    if (m_radMonth.GetSafeHwnd())
    {
        ReadControls();
        SavePrintOptsToSettings(m_opts);
    }
    CDialog::OnCancel();
}

// =============================================================================
// CCalPrintDlg — OnBnPreview
// =============================================================================

void CCalPrintDlg::OnBnPreview()
{
    ReadControls();
    SavePrintOptsToSettings(m_opts);
    CCalPreviewDlg prev(m_opts, m_pFrame, nullptr);
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
    buf.t.dwExtendedStyle = WS_EX_APPWINDOW;
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
    bool hasNext = (m_curPage < CalendarPrintSheetCount(m_opts, m_pages) - 1);
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
    s.Format(L"Page %d of %d", m_curPage + 1, CalendarPrintSheetCount(m_opts, m_pages));
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
    if (m_curPage < CalendarPrintSheetCount(m_opts, m_pages) - 1)
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
    DrawCalendarPrintSheet(&memDC, rcContent, m_pages, m_curPage, m_pFrame, m_opts);

    // Toolbar background
    CRect rcTool(0, H - toolH, W, H);
    memDC.FillSolidRect(rcTool, GetSysColor(COLOR_3DFACE));

    dc.BitBlt(0, 0, W, H, &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}
