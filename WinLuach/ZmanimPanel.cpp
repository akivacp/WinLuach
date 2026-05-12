// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    ZmanimPanel.cpp
// Purpose: Draws the zmanim panel: 12 halachic times in 4 columns.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial implementation.
// =============================================================================

#include "pch.h"
#include "ZmanimPanel.h"

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
    int alotSh  = m_pFrame->m_alotShita;
    int tzeitSh = m_pFrame->m_tzeitShita;
    int zmanSh  = m_pFrame->m_zmanimShita;

    TimeOfDay alotTime  = (alotSh == 1) ? z.alot_MA72  : (alotSh == 2) ? z.alot_MA90  : z.alot_GRA;
    TimeOfDay tzeitTime = (tzeitSh == 1) ? z.tzeit_MA72 : (tzeitSh == 2) ? z.tzeit_MA90
                        : (tzeitSh == 3) ? z.tzeit_MA72P : (tzeitSh == 4) ? z.tzeit_MA90P : z.tzeit_GRA;
    TimeOfDay sofShemaTime    = (zmanSh == 1) ? z.sofShema_MA72    : (zmanSh == 2) ? z.sofShema_MA90    : z.sofShema_GRA;
    TimeOfDay sofTefillaTime  = (zmanSh == 1) ? z.sofTefilla_MA72  : (zmanSh == 2) ? z.sofTefilla_MA90  : z.sofTefilla_GRA;
    TimeOfDay minchaGTime     = (zmanSh == 1) ? z.minchaGedola_MA72: (zmanSh == 2) ? z.minchaGedola_MA90: z.minchaGedola_GRA;
    TimeOfDay minchaKTime     = (zmanSh == 1) ? z.minchaKetana_MA72: (zmanSh == 2) ? z.minchaKetana_MA90: z.minchaKetana_GRA;
    TimeOfDay plagTime        = (zmanSh == 1) ? z.plagMincha_MA72  : (zmanSh == 2) ? z.plagMincha_MA90  : z.plagMincha_GRA;
    double    shaahMin        = (zmanSh == 1) ? z.shaahZmanit_MA72 : (zmanSh == 2) ? z.shaahZmanit_MA90 : z.shaahZmanit_GRA;

    static const wchar_t* kAlotLabels[]  = { L"Alos Hashachar (16.1°)", L"Alos Hashachar (72m)", L"Alos Hashachar (90m)" };
    static const wchar_t* kTzeitLabels[] = { L"Tzeis (8.5°)", L"Tzeis (72m)", L"Tzeis (90m)", L"Tzeis (72m prop)", L"Tzeis (90m prop)" };
    static const wchar_t* kShemaLabels[] = { L"Sof Shema (GRA)", L"Sof Shema (MA72)", L"Sof Shema (MA90)" };
    static const wchar_t* kMinchaGLabels[] = { L"Mincha Gedola", L"Mincha Gedola", L"Mincha Gedola" };

    // 12 zmanim entries
    struct ZmanEntry { const wchar_t* label; TimeOfDay time; };
    ZmanEntry entries[] = {
        { kAlotLabels[alotSh],          alotTime        },
        { L"Misheyakir",                z.misheyakir_10 },
        { L"Hanetz",                    z.hanetz        },
        { kShemaLabels[zmanSh],         sofShemaTime    },
        { L"Sof Tefilla",               sofTefillaTime  },
        { L"Chatzos",                   z.chatzot       },
        { kMinchaGLabels[zmanSh],       minchaGTime     },
        { L"Mincha Ketana",             minchaKTime     },
        { L"Plag HaMincha",             plagTime        },
        { L"Shkiah",                    z.shkia         },
        { kTzeitLabels[tzeitSh],        tzeitTime       },
        { L"Candle Lighting",           z.candleLighting},
    };

    int numEntries = sizeof(entries) / sizeof(entries[0]);
    int cols = 4;
    int rows = (numEntries + cols - 1) / cols;
    int padding = 16;
    int colGap = 20;
    int colW = (cx - padding * 2 - colGap * (cols - 1)) / cols;
    int startY = 22;
    int rowH = 19;
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
        memDC.DrawText(entries[i].label, -1, rcLabel,
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
    double sz = shaahMin;
    if (sz > 0.0) {
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