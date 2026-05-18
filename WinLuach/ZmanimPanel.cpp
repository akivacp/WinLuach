// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    ZmanimPanel.cpp
// Purpose: Draws the zmanim panel: 12 halachic times in 4 columns.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial implementation.
// v0.8.0 - Bar entries driven by zmanimBarMask + kZmanimBarLabels.  Added
//          Sof Shema MA (custom), Sof Tefilla MA (custom), and End of Fast.
// =============================================================================

#include "pch.h"
#include "ZmanimPanel.h"

// v0.8.0 - Public bit-index table used by both this panel and the Options
// dialog "Zmanim Bar" tab. The bit index matches the column position.
const wchar_t* const kZmanimBarLabels[] = {
    L"Alos",                       //  0
    L"Misheyakir",                 //  1
    L"Hanetz",                     //  2
    L"Sof Shema MA72 prop.",       //  3
    L"Sof Shema MA90 prop.",       //  4
    L"Sof Shema (GRA)",            //  5
    L"Sof Shema MA (custom)",      //  6
    L"Sof Tefilla MA72 prop.",     //  7
    L"Sof Tefilla MA90 prop.",     //  8
    L"Sof Tefilla (GRA)",          //  9
    L"Sof Tefilla MA (custom)",    // 10
    L"Chatzos",                    // 11
    L"Mincha Gedola",              // 12
    L"Mincha Ketana",              // 13
    L"Plag HaMincha",              // 14
    L"Shkiah",                     // 15
    L"Tzeis",                      // 16
    L"Candle Lighting",            // 17
    L"Sha'a Zmanit",               // 18
    L"End of Fast",                // 19
};
const int kZmanimBarLabelCount = (int)(sizeof(kZmanimBarLabels) / sizeof(kZmanimBarLabels[0]));

BEGIN_MESSAGE_MAP(CZmanimPanel, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

CZmanimPanel::CZmanimPanel(CMainFrame* pFrame)
    : m_pFrame(pFrame)
{
}

CZmanimPanel::~CZmanimPanel()
{
}

BOOL CZmanimPanel::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

void CZmanimPanel::OnPaint()
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

    // Background + top border
    memDC.FillSolidRect(rcClient, CLR_ZMANIM_BG);
    CPen penTop(PS_SOLID, 1, RGB(160, 160, 140));
    CPen* pOldPen = memDC.SelectObject(&penTop);
    memDC.MoveTo(0, 0);
    memDC.LineTo(cx, 0);
    memDC.SelectObject(pOldPen);

    memDC.SetBkMode(TRANSPARENT);

    // Header line: "Zmanim | City | Day"
    CString hdr;
    hdr.Format(L"Zmanim  |  %s  |  %s",
        m_pFrame->m_location.name.c_str(),
        DayOfWeekName(GetDayOfWeek(m_pFrame->m_selectedDate)).c_str());
    memDC.SelectObject(&m_pFrame->m_fontBold);
    memDC.SetTextColor(RGB(60, 60, 20));
    CRect rcHdr(8, 3, cx, 20);
    memDC.DrawText(hdr, rcHdr, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Select time variants based on shita settings
    const ZmanimResult& z = m_pFrame->m_zmanim;
    DisplayZmanimTimes d = m_pFrame->BuildDisplayZmanim(
        m_pFrame->m_selectedDate, z, m_pFrame->m_isDST);

    // v0.8.0 - Build the bar entries from kZmanimBarLabels + zmanimBarMask.
    // Each bit decides whether the corresponding zman is visible.  The MA
    // (custom) entries follow the user's selected MA shita (72 vs 90).
    struct ZmanEntry { std::wstring label; TimeOfDay time; bool isSha = false; };
    std::vector<ZmanEntry> entries;
    entries.reserve(kZmanimBarLabelCount);

    uint32_t mask = m_pFrame->m_zmanimBarMask;
    int maShita = m_pFrame->m_zmanimShita; // 0=GRA,1=MA72,2=MA90
    TimeOfDay sofShemaMaCustom  = d.sofShema;
    TimeOfDay sofTefMaCustom    = d.sofTefilla;

    // End-of-Fast time: shkia + 27 min for R' Tukaccinsky, or tzeit_MA72 for
    // R' Moshe Feinstein.  Picked by customEndFastPreset.
    TimeOfDay endFast = (m_pFrame->m_customEndFastPreset == 1)
        ? z.tzeit_MA72
        : AddMinutes(z.shkia, (m_pFrame->m_customEndFastPreset == 2)
            ? (int)round(max(0.0, m_pFrame->m_customEndFastValue))
            : 27);

    auto pushIf = [&](int bit, const wchar_t* label, const TimeOfDay& t, bool isSha = false) {
        if (mask & (1u << bit))
            entries.push_back({ label, t, isSha });
    };

    pushIf(0,  d.alotLabel.c_str(),       d.alot);
    pushIf(1,  d.misheyakirLabel.c_str(), d.misheyakir);
    pushIf(2,  kZmanimBarLabels[2],       z.hanetz);
    pushIf(3,  kZmanimBarLabels[3],       z.sofShema_MA72);
    pushIf(4,  kZmanimBarLabels[4],       z.sofShema_MA90);
    pushIf(5,  kZmanimBarLabels[5],       z.sofShema_GRA);
    pushIf(6,  d.sofShemaLabel.c_str(),    sofShemaMaCustom);
    pushIf(7,  kZmanimBarLabels[7],       z.sofTefilla_MA72);
    pushIf(8,  kZmanimBarLabels[8],       z.sofTefilla_MA90);
    pushIf(9,  kZmanimBarLabels[9],       z.sofTefilla_GRA);
    pushIf(10, d.sofTefillaLabel.c_str(),  sofTefMaCustom);
    pushIf(11, kZmanimBarLabels[11],      z.chatzot);
    pushIf(12, kZmanimBarLabels[12],      d.minchaGedola);
    pushIf(13, kZmanimBarLabels[13],      d.minchaKetana);
    pushIf(14, kZmanimBarLabels[14],      d.plagMincha);
    pushIf(15, kZmanimBarLabels[15],      z.shkia);
    pushIf(16, d.tzeitLabel.c_str(),      d.tzeit);
    pushIf(17, kZmanimBarLabels[17],      z.candleLighting);
    // bit 18 (Sha'a Zmanit) handled on the separate bottom line below.
    pushIf(19, kZmanimBarLabels[19],      endFast);

    int numEntries = (int)entries.size();
    if (numEntries == 0) {
        dcScreen.BitBlt(0, 0, cx, cy, &memDC, 0, 0, SRCCOPY);
        memDC.SelectObject(pOldBmp);
        return;
    }
    int cols = 4;
    int rows = (numEntries + cols - 1) / cols;
    int padding = 16;
    int colGap = 20;
    int colW = (cx - padding * 2 - colGap * (cols - 1)) / cols;
    int startY = 22;
    int rowH = max(15, min(19, (cy - startY - 20) / max(1, rows)));
    int labelW = 110;
    int timeW = 60;

    for (int i = 0; i < numEntries; i++)
    {
        int col = i / rows;
        int row = i % rows;
        int x = padding + col * (colW + colGap);
        int y = startY + row * rowH;

        // Column divider
        if (col > 0 && row == 0)
        {
            CPen penDiv(PS_SOLID, 1, RGB(200, 195, 170));
            CPen* pOld = memDC.SelectObject(&penDiv);
            int divX = x - colGap / 2;
            memDC.MoveTo(divX, 4);
            memDC.LineTo(divX, cy - 4);
            memDC.SelectObject(pOld);
        }

        // Label
        memDC.SelectObject(&m_pFrame->m_fontSmall);
        memDC.SetTextColor(RGB(70, 70, 50));
        CRect rcLabel(x, y, x + labelW, y + rowH);
        memDC.DrawText(entries[i].label.c_str(), -1, rcLabel,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        // Time
        memDC.SelectObject(&m_pFrame->m_fontNormal);
        memDC.SetTextColor(RGB(20, 20, 140));
        std::wstring timeStr = FormatTime(entries[i].time,
            m_pFrame->m_use24hr);
        CRect rcTime(x + labelW, y, x + labelW + timeW, y + rowH);
        memDC.DrawText(timeStr.c_str(), -1, rcTime,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    // Sha'a Zmanit — shown on a separate bottom line spanning all columns
    // (gated by zmanimBarMask bit 18 in v0.8.0).
    double sz = d.shaahZmanit;
    if (sz > 0.0 && (mask & (1u << 18))) {
        int szMin = (int)round(sz);
        CString szStr;
        szStr.Format(L"Sha'a Zmanit:   %d:%02d  (%d min)", szMin/60, szMin%60, szMin);
        int y = startY + rows * rowH + 2;
        memDC.SelectObject(&m_pFrame->m_fontSmall);
        memDC.SetTextColor(RGB(50, 100, 50));
        memDC.DrawText(szStr, CRect(padding, y, cx - padding, y + rowH),
            DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    dcScreen.BitBlt(0, 0, cx, cy, &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}
