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
#include <afxdlgs.h>

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

                // Friday for this row (col 5 = Friday in Sun=0 layout)
                long         fridayJDN = jdn0 + w * 7 + 5;
                GregorianDate fri      = JDNToGregorian(fridayJDN);
                HebrewDate    hShab    = JDNToHebrew(fridayJDN + 1);

                ParashaInfo par = GetParasha(hShab, isIsrael);
                CString lbl;
                lbl.Format(L"Fri %d/%d", fri.month, fri.day);
                if (!par.name.empty())
                {
                    lbl += L"  ";
                    lbl += par.name.c_str();
                    if (par.isCombined && !par.name2.empty())
                    { lbl += L"–"; lbl += par.name2.c_str(); }
                }
                pDC->SelectObject(&fontTbl);
                pDC->SetTextColor(RGB(30,30,80));
                pDC->DrawText(lbl,
                    CRect(x0+2, ry, x0+lblW-2, ry+dataRowH),
                    DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);

                bool         dst = IsDST(fri, pFrame->m_location);
                ZmanimResult z   = CalculateZmanim(fri, pFrame->m_location, dst);

                const TimeOfDay* kT[14] = {
                    &z.alot_GRA,         &z.misheyakir_11,    &z.hanetz,
                    &z.sofShema_MA72,    &z.sofShema_GRA,
                    &z.sofTefilla_MA72,  &z.sofTefilla_GRA,
                    &z.chatzot,          &z.minchaGedola_GRA, &z.minchaKetana_GRA,
                    &z.plagMincha_GRA,   &z.candleLighting,
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
                    CString szs; szs.Format(L"%.0f", z.shaahZmanit_GRA);
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
    buf.t.cy     = 285;
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

    // ── PDF checkbox ─────────────────────────────────────────────────────────
    m_chkPDF.Create(L"Export to PDF  (select 'Microsoft Print to PDF' in next dialog)",
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                    CRect(20, y, W - 10, y + 18), this, IDC_PD_CHK_PDF);
    m_chkPDF.SetFont(pF);
    y += 30;

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
