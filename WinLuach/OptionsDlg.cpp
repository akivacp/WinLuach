// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    OptionsDlg.cpp
// Purpose: MFC preferences dialog implementation.
// =============================================================================
//
// CHANGELOG:
// v0.8.70 - Advanced Reminders dialog:
//           (a) Added Before/After direction dropdown — reminders can now fire
//               either before OR after their trigger (stored as afterEvent in
//               ReminderRule; label changed from "Before:" to "Offset:").
//           (b) Expanded Holiday targets: Rosh Chodesh (Day 1 & 2), Chol HaMoed
//               Sukkos/Pesach, all fast days, all minor holidays (Pesach Sheini,
//               Tu B'Av, Purim Katan, Shushan Purim, Hoshana Raba, Isru Chag,
//               Simchas Torah), Israeli holidays (Yom HaShoah, Yom Hazikaron,
//               Yom HaAtzmaut, Yom Yerushalayim).
//           (c) Added 7 custom zmanim to Zman targets (Custom Alos, Custom Sof
//               Shema, Custom Sof Tefilla, Custom Mincha Gedola, Custom Mincha
//               Ketana, Custom Plag HaMincha, Custom Tzeit).
// v0.8.69 - Wired OnCheckNow() (Options → Updates tab "Check Now" button) to
//           CMainFrame::CheckForUpdates(). Previously showed a placeholder
//           "coming soon" message. Now runs the full update flow: GitHub API
//           fetch, version compare, download, and auto-replace + restart.
// v0.1.0 - Initial implementation.
// v0.1.1 - Fixed: use InitModalIndirect + in-memory template.
// v0.1.2 - Fixed CreateEx call to use 10 arguments (removed 2 extra nullptrs).
//          Moved DoModal() to public in header.
// v0.8.2 - Restored Zmanim sub-tab strip visibility (WS_CLIPSIBLINGS on
//          both tab controls + explicit Invalidate/UpdateWindow + z-order
//          lift on the sub-tab when the Zmanim main tab is activated).
// v0.8.1 - Sub-page widgets are raised to the top of z-order on show so the
//          Zmanim sub-tab control doesn't paint over them, and the Omer
//          'manual time' radio is no longer clipped by its sibling label.
// v0.8.84 - Sha'a Zmanit sub-tab polish:
//           (1) Sub-tab strip now uses TCS_MULTILINE (wraps to 2 rows) so
//               all 10 tabs are always visible without scroll arrows.
//               zmanPageTop raised from 64 → 90 to clear the 2-row strip.
//           (2) Radio buttons explicitly set to BST_UNCHECKED before
//               applying the selected state, fixing the double-selected
//               visual glitch in the Sha'a Zmanit sub-page.
//           (3) Removed location-specific example from radio button labels;
//               replaced with seasonal context ("shorter in winter, longer
//               in summer").
// v0.8.83 - Added independent Sha'a Zmanit sub-tab (GRA / MA72 / MA90)
//           so the displayed halachic hour and the time-unit used for Sof
//           Shema / Tefilla are decoupled from the Sof Zman shita.
// v0.8.82 - (1) End-of-Fast custom tab: added "Minutes type" dropdown
//               (fixed vs shaah zmanit) for custom preset.
//           (2) All zman custom options: replaced 3-radio
//               (degrees/fixed/shaah) with separate degrees checkbox+value
//               and minutes value+type dropdown.
//           (3) Holiday Schedule group moved below Updates group on General
//               tab (was overlapping Reset group at same y position).
//           (4) Tray tooltip: custom Sof Shema/Tefilla checkboxes shown
//               before GRA entries; max-5 limit with instruction + warning.
//           (5) Zmanim Bar checkbox order: custom Sof Shema/Tefilla listed
//               before their GRA counterparts.
// v0.8.0 - Added "Zmanim Bar" tab with one checkbox per kZmanimBarLabels
//          entry; restructured "Zmanim" tab into a TCS sub-tab with pages
//          for Alos, Misheyakir, Sof Zman MA, Sof Zman GRA, Mincha Gedola,
//          Mincha Ketana, Plag, End of Fast, and Tzais.  "Zman Shitos" tab
//          dropped from main tab list (absorbed into Zmanim sub-tabs).
// =============================================================================

#include "pch.h"
#include "OptionsDlg.h"
#include "MainFrame.h"
#include "UpdateChecker.h"
#include "Resource.h"
#include "ZmanimPanel.h"
#include <cmath>
#include <cwctype>
#include <fstream>

#define IDC_OPT_RAD_AMPM       301
#define IDC_OPT_RAD_24HR       302
#define IDC_OPT_RAD_DIASPORA   303
#define IDC_OPT_RAD_ISRAEL     304
#define IDC_OPT_RAD_GRA        305
#define IDC_OPT_RAD_MA72       306
#define IDC_OPT_RAD_MA90       307
#define IDC_OPT_RAD_CIVIL      311
#define IDC_OPT_RAD_HEBREW     312
#define IDC_OPT_TRACKING       313
#define IDC_OPT_PARSHIOS       314
#define IDC_OPT_MOADIM         315
#define IDC_OPT_USER_EVENTS    316
#define IDC_OPT_DAF            317
#define IDC_OPT_YERUSHALMI     318
#define IDC_OPT_HALACHA        319
#define IDC_OPT_MISHNA         320
#define IDC_OPT_TANACH         321
#define IDC_OPT_MIN_TRAY       322
#define IDC_OPT_MIN_STARTUP    323
#define IDC_OPT_START_WINDOWS  324
#define IDC_OPT_DESKTOP        325
#define IDC_OPT_HAFTARAH       327
#define IDC_OPT_FONT_SIZE      328
#define IDC_OPT_LANGUAGE       329
#define IDC_OPT_TRAY_WHEN      330
#define IDC_OPT_CANDLE         331
#define IDC_OPT_TRAY_COLOR     332
#define IDC_OPT_MANAGE_CALS    333
#define IDC_OPT_NOTIFY_PERSONAL 334
#define IDC_OPT_NOTIFY_WEBCAL   335
#define IDC_OPT_PREVIEW_NOTIFY  336
#define IDC_OPT_SHOW_TRAY       337
#define IDC_OPT_ALOT_SHITA      338
#define IDC_OPT_TZEIT_SHITA     339
#define IDC_OPT_CHATZOS_FASTS   340
#define IDC_OPT_SHOW_BEHAB      411   // v0.8.78
#define IDC_OPT_CHATZOS_BEHAB   412   // v0.8.78
#define IDC_OPT_TAB             341
#define IDC_OPT_COLOR_RESTORE   342
#define IDC_OPT_ALOT_VALUE      343
#define IDC_OPT_ALOT_DEG        344
#define IDC_OPT_ALOT_MIN        345
#define IDC_OPT_ALOT_ZMANIS     346
#define IDC_OPT_TZEIT_VALUE     347
#define IDC_OPT_TZEIT_DEG       348
#define IDC_OPT_TZEIT_MIN       349
#define IDC_OPT_TZEIT_ZMANIS    350
#define IDC_OPT_COLOR_ITEM      351
#define IDC_OPT_COLOR_HEX       352
#define IDC_OPT_COLOR_PICKER    353
#define IDC_OPT_COLOR_PREVIEW   354
#define IDC_OPT_COLOR_SAMPLE    355
#define IDC_OPT_APPLY           356
#define IDC_OPT_NOTIFY_ZMAN_STYLE 357
#define IDC_OPT_NOTIFY_MOADIM_STYLE 358
#define IDC_OPT_NOTIFY_MOADIM_OFFSETS 359
#define IDC_OPT_NOTIFY_PARSHA_NAME 360
#define IDC_OPT_NOTIFY_PARSHA_STYLE 361
#define IDC_OPT_NOTIFY_PARSHA_OFFSETS 362
#define IDC_OPT_NOTIFY_PERSONAL_OFFSETS 363
#define IDC_OPT_ADV_REMINDERS 364
#define IDC_OPT_MISHEYAKIR_SHITA 365
#define IDC_OPT_MISHEYAKIR_VALUE 366
#define IDC_OPT_MISHEYAKIR_DEG   367
#define IDC_OPT_MISHEYAKIR_MIN   368
#define IDC_OPT_MISHEYAKIR_ZMANIS 369
#define IDC_OPT_SOFZMAN_SHITA    370
#define IDC_OPT_SOFZMAN_VALUE    371
#define IDC_OPT_SOFZMAN_DEG      372
#define IDC_OPT_SOFZMAN_MIN      373
#define IDC_OPT_SOFZMAN_ZMANIS   374
#define IDC_OPT_NOTIFY_SEFIRAH_STYLE 375
#define IDC_OPT_NOTIFY_SEFIRAH_TIME 376
#define IDC_OPT_TRAY_BACK_ENABLED 377
#define IDC_OPT_TRAY_BACK_COLOR   378
#define IDC_OPT_TRAY_FONT         379
#define IDC_OPT_NOTIFY_ZMAN_FIRST 460
#define IDC_OPT_TRAY_NUMBER       397
#define IDC_OPT_TRAY_DEFAULTS     398
#define IDC_OPT_NOTIFY_SEFIRAH_HOUR 399
#define IDC_OPT_NOTIFY_SEFIRAH_MIN  400
#define IDC_OPT_NOTIFY_SEFIRAH_AMPM 401
#define IDC_OPT_NOTIFY_SEFIRAH_MODE 402
#define IDC_OPT_NOTIFY_SEFIRAH_OFFSET 403
#define IDC_OPT_NOTIFY_SEFIRAH_DIR 404
#define IDC_OPT_NOTIFY_SEFIRAH_BASE 405
#define IDC_OPT_NOTIFY_SEFIRAH_OTHER 406
#define IDC_OPT_NOTIFY_MOADIM_UNIT 407
#define IDC_OPT_NOTIFY_PERSONAL_UNIT 408
#define IDC_OPT_NOTIFY_PARSHA_UNIT 409
#define IDC_OPT_RESET_CALENDAR 410
#define IDC_OPT_TRAY_TIP_ZMAN_FIRST       420
#define IDC_OPT_TRAY_TIP_CUSTOM_ZMAN_FIRST 451  // 451..457 (7 custom zmanim)
#define IDC_OPT_YEAR_DETAIL_FIRST          500  // 500..511 (12 year facts)
#define IDC_OPT_RAD_WINNOTIFY      520
#define IDC_OPT_RAD_WINLUACHTOAST  521
#define IDC_OPT_TOAST_DURATION     522
#define IDC_OPT_TOAST_UNIT         523
// Daily reminder time (for day/week/month offset rules) — v0.8.71
#define IDC_OPT_REMIND_DAILY_HOUR  530
#define IDC_OPT_REMIND_DAILY_MIN   531
#define IDC_OPT_REMIND_DAILY_AMPM  532
#define IDC_OPT_NOTIFY_SUBTAB      541

enum ColorPreviewKind
{
    CP_NORMAL = 0,
    CP_OTHER_MONTH,
    CP_TODAY,
    CP_SHABBOS,
    CP_YOM_TOV,
    CP_ROSH_CHODESH,
    CP_CHOL_HAMOED,
    CP_FAST_DAY,
    CP_CIVIL_EVENT,
    CP_HEBREW_EVENT,
    CP_LEARNING,
    CP_ZMANIM,
};

struct CalendarColorPref
{
    const wchar_t* label;
    int AppSettings::* field;
    int defaultValue;
    int previewKind;
};

static const CalendarColorPref kCalendarColorPrefs[] = {
    { L"Normal cell",        &AppSettings::colorNormalCell,      0x00FFFFFF, CP_NORMAL },
    { L"Other-month cell",   &AppSettings::colorOtherMonthCell,  0x00F0F0F0, CP_OTHER_MONTH },
    { L"Today cell",         &AppSettings::colorTodayCell,       0x0096FFFF, CP_TODAY },
    { L"Shabbos cell",       &AppSettings::colorShabbosCell,     0x00B4E6B4, CP_SHABBOS },
    { L"Yom Tov cell",       &AppSettings::colorYomTovCell,      0x0082C8FF, CP_YOM_TOV },
    { L"Rosh Chodesh cell",  &AppSettings::colorRoshChodeshCell, 0x00FFD2B4, CP_ROSH_CHODESH },
    { L"Chol Hamoed cell",   &AppSettings::colorCholHamoedCell,  0x00B4E6FF, CP_CHOL_HAMOED },
    { L"Fast-day cell",      &AppSettings::colorFastDayCell,     0x00DCC8DC, CP_FAST_DAY },
    { L"Gregorian text",     &AppSettings::colorGregorianText,   0x00323232, CP_NORMAL },
    { L"Hebrew text",        &AppSettings::colorHebrewText,      0x00B41E1E, CP_NORMAL },
    { L"Holiday text",       &AppSettings::colorHolidayText,     0x001E1EB4, CP_YOM_TOV },
    { L"Parsha text",        &AppSettings::colorParshaText,      0x00A0501E, CP_SHABBOS },
    { L"Civil event text",   &AppSettings::colorCivilEventText,  0x00781E78, CP_CIVIL_EVENT },
    { L"Hebrew event text",  &AppSettings::colorHebrewEventText, 0x00506E00, CP_HEBREW_EVENT },
    { L"Omer text",          &AppSettings::colorOmerText,        0x00965064, CP_NORMAL },
    { L"Learning text",      &AppSettings::colorLearningText,    0x00B45A00, CP_LEARNING },
    { L"Candle text",        &AppSettings::colorCandleText,      0x000050B4, CP_ZMANIM },
    { L"Motzaei text",       &AppSettings::colorMotzText,        0x00A05000, CP_ZMANIM },
};

static const wchar_t* kColorPreviewNames[] = {
    L"Regular day",
    L"Other-month day",
    L"Today",
    L"Shabbos",
    L"Yom Tov",
    L"Rosh Chodesh",
    L"Chol Hamoed",
    L"Fast day",
    L"Civil event",
    L"Hebrew event",
    L"Learning",
    L"Zmanim",
};

static const wchar_t* kZmanCheckboxLabels[] = {
    L"Alos", L"Misheyakir", L"Netz (Hanetz)", L"Sof Shema (GRA)", L"Sof Tefilla (GRA)",
    L"Chatzos", L"Mincha Gedola", L"Mincha Ketana", L"Plag", L"Shkiah",
    L"Tzais", L"Candles", L"Fast start/end", L"Eat chametz", L"Burn chametz",
    L"Sof Shema (MA 72 prop.)", L"Sof Tefilla (MA 72 prop.)",
    L"Sof Shema (MA 90 prop.)", L"Sof Tefilla (MA 90 prop.)"
};

static constexpr int kZmanCheckboxCount =
    (int)(sizeof(kZmanCheckboxLabels) / sizeof(kZmanCheckboxLabels[0]));

static const wchar_t* kTrayTooltipZmanLabels[] = {
    L"Alos GRA", L"Alos MA72", L"Alos MA90",
    L"Misheyakir 10.2", L"Misheyakir 11.5", L"Netz",
    L"Sof Shema MA72", L"Sof Shema MA90", L"Sof Shema GRA",
    L"Sof Tefilla MA72", L"Sof Tefilla MA90", L"Sof Tefilla GRA",
    L"Chatzos",
    L"Mincha Gedola MA72", L"Mincha Gedola MA90", L"Mincha Gedola GRA",
    L"Mincha Ketana MA72", L"Mincha Ketana MA90", L"Mincha Ketana GRA",
    L"Plag MA72", L"Plag MA90", L"Plag GRA",
    L"Shkiah", L"Tzais GRA", L"Tzais MA72", L"Tzais MA90",
    L"Candles", L"Fast start/end", L"Eat chametz", L"Burn chametz",
    L"Today's Omer"
};

static constexpr int kTrayTooltipZmanCount =
    (int)(sizeof(kTrayTooltipZmanLabels) / sizeof(kTrayTooltipZmanLabels[0]));

static const wchar_t* kTrayTooltipCustomZmanLabels[] = {
    L"Custom Alos",         L"Custom Sof Shema",    L"Custom Sof Tefilla",
    L"Custom Mincha Gedola",L"Custom Mincha Ketana", L"Custom Plag HaMincha",
    L"Custom Tzeit"
};
static constexpr int kTrayTooltipCustomZmanCount = 7;

static const int kShaahZmanitPresetSettingByUi[] = { 0, 1, 2, 4, 3 };
static constexpr int kShaahZmanitPresetCount =
    (int)(sizeof(kShaahZmanitPresetSettingByUi) / sizeof(kShaahZmanitPresetSettingByUi[0]));
static constexpr int kShaahZmanitCustomUiIndex = 4;

static const wchar_t* kYearDetailLabels[] = {
    L"Years from creation",
    L"Lunar cycle position",
    L"Shmita cycle year",
    L"Maaser type",
    L"Solar cycle position",
    L"Years since Temple destruction",
    L"Cheshvan days",
    L"Kislev days",
    L"Leap / simple year",
    L"Full / deficient / regular",
    L"State of Israel anniversary",
    L"Jerusalem liberation anniversary",
};
static constexpr int kYearDetailCount = 12;

static CString ColorToHex(COLORREF c)
{
    CString s;
    s.Format(L"#%02X%02X%02X", GetRValue(c), GetGValue(c), GetBValue(c));
    return s;
}

static bool ParseHexColor(CString text, COLORREF& out)
{
    text.Trim();
    if (!text.IsEmpty() && text[0] == L'#')
        text = text.Mid(1);
    if (text.GetLength() != 6)
        return false;

    int value = 0;
    for (int i = 0; i < 6; ++i)
    {
        wchar_t ch = text[i];
        int digit = -1;
        if (ch >= L'0' && ch <= L'9') digit = ch - L'0';
        else if (ch >= L'a' && ch <= L'f') digit = ch - L'a' + 10;
        else if (ch >= L'A' && ch <= L'F') digit = ch - L'A' + 10;
        if (digit < 0) return false;
        value = (value << 4) | digit;
    }

    out = RGB((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
    return true;
}

BEGIN_MESSAGE_MAP(CColorPreviewWnd, CWnd)
    ON_WM_PAINT()
END_MESSAGE_MAP()

void CColorPreviewWnd::SetPreview(AppSettings* settings, int previewKind)
{
    m_settings = settings;
    m_previewKind = previewKind;
    if (GetSafeHwnd())
        Invalidate(FALSE);
}

void CColorPreviewWnd::OnPaint()
{
    CPaintDC dc(this);
    CRect rc;
    GetClientRect(&rc);
    dc.FillSolidRect(rc, RGB(255, 255, 255));

    if (!m_settings)
        return;

    CRect cell = rc;
    cell.DeflateRect(10, 10);
    COLORREF bg = (COLORREF)m_settings->colorNormalCell;
    switch (m_previewKind)
    {
    case CP_OTHER_MONTH:  bg = (COLORREF)m_settings->colorOtherMonthCell; break;
    case CP_TODAY:        bg = (COLORREF)m_settings->colorTodayCell; break;
    case CP_SHABBOS:      bg = (COLORREF)m_settings->colorShabbosCell; break;
    case CP_YOM_TOV:      bg = (COLORREF)m_settings->colorYomTovCell; break;
    case CP_ROSH_CHODESH: bg = (COLORREF)m_settings->colorRoshChodeshCell; break;
    case CP_CHOL_HAMOED:  bg = (COLORREF)m_settings->colorCholHamoedCell; break;
    case CP_FAST_DAY:     bg = (COLORREF)m_settings->colorFastDayCell; break;
    default: break;
    }

    dc.FillSolidRect(cell, bg);
    CPen border(PS_SOLID, 1, RGB(120, 120, 120));
    CPen* oldPen = dc.SelectObject(&border);
    CBrush* oldBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);
    dc.Rectangle(cell);
    dc.SelectObject(oldBrush);
    dc.SelectObject(oldPen);

    CFont fontSmall;
    fontSmall.CreatePointFont(82, L"Segoe UI");
    CFont fontBold;
    fontBold.CreatePointFont(96, L"Segoe UI");

    dc.SetBkMode(TRANSPARENT);
    int x = cell.left + 8;
    int y = cell.top + 6;
    int right = cell.right - 8;

    dc.SelectObject(&fontBold);
    dc.SetTextColor((m_previewKind == CP_OTHER_MONTH)
        ? RGB(165, 165, 165)
        : (COLORREF)m_settings->colorGregorianText);
    dc.DrawText(L"14", CRect(x, y, x + 44, y + 22), DT_LEFT | DT_TOP | DT_SINGLELINE);

    dc.SelectObject(&fontSmall);
    dc.SetTextColor((m_previewKind == CP_OTHER_MONTH)
        ? RGB(170, 170, 190)
        : (COLORREF)m_settings->colorHebrewText);
    dc.DrawText(L"28 Iyar", CRect(x, y + 2, right, y + 20), DT_RIGHT | DT_TOP | DT_SINGLELINE);

    y += 28;
    auto line = [&](const wchar_t* text, COLORREF clr)
    {
        dc.SetTextColor(clr);
        dc.DrawText(text, -1, CRect(x, y, right, y + 18),
            DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        y += 18;
    };

    switch (m_previewKind)
    {
    case CP_SHABBOS:
        line(L"Parshas Naso", (COLORREF)m_settings->colorParshaText);
        line(L"Motz 9:12 PM", (COLORREF)m_settings->colorMotzText);
        break;
    case CP_YOM_TOV:
        line(L"Shavuos", (COLORREF)m_settings->colorHolidayText);
        line(L"Candles 8:02 PM", (COLORREF)m_settings->colorCandleText);
        break;
    case CP_ROSH_CHODESH:
        line(L"Rosh Chodesh Sivan", (COLORREF)m_settings->colorHolidayText);
        break;
    case CP_CHOL_HAMOED:
        line(L"Chol Hamoed Pesach", (COLORREF)m_settings->colorHolidayText);
        line(L"Day 3 Omer", (COLORREF)m_settings->colorOmerText);
        break;
    case CP_FAST_DAY:
        line(L"Fast of Tammuz", (COLORREF)m_settings->colorHolidayText);
        line(L"Fast ends 9:01 PM", (COLORREF)m_settings->colorMotzText);
        break;
    case CP_CIVIL_EVENT:
        line(L"Birthday: Sarah (civil)", (COLORREF)m_settings->colorCivilEventText);
        break;
    case CP_HEBREW_EVENT:
        line(L"Yahrzeit: Moshe (hebrew)", (COLORREF)m_settings->colorHebrewEventText);
        break;
    case CP_LEARNING:
        line(L"Daf Yomi: Bava Kama 23", (COLORREF)m_settings->colorLearningText);
        line(L"Mishna Yomit: Peah 2:1", (COLORREF)m_settings->colorLearningText);
        break;
    case CP_ZMANIM:
        line(L"Candles 8:02 PM", (COLORREF)m_settings->colorCandleText);
        line(L"Motz 9:12 PM", (COLORREF)m_settings->colorMotzText);
        break;
    case CP_OTHER_MONTH:
        line(L"Other month", RGB(150, 150, 150));
        break;
    case CP_TODAY:
        line(L"Today", (COLORREF)m_settings->colorHolidayText);
        line(L"Day 44 Omer", (COLORREF)m_settings->colorOmerText);
        break;
    default:
        line(L"Day 44 Omer", (COLORREF)m_settings->colorOmerText);
        line(L"Custom reminder", (COLORREF)m_settings->colorCivilEventText);
        break;
    }
}

// =============================================================================
// CWebCalDlg — inline multi-calendar manager
// =============================================================================

class CWebCalDlg : public CDialog
{
public:
    std::vector<WebCalEntry> calendars;

    explicit CWebCalDlg(const std::vector<WebCalEntry>& initial, CWnd* pParent = nullptr)
        : CDialog(), calendars(initial)
    {
        m_pParentWnd = pParent;
    }

    virtual INT_PTR DoModal() override
    {
        struct DlgTmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[32]; } buf = {};
        buf.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_CENTER;
        buf.t.dwExtendedStyle = WS_EX_APPWINDOW;
        buf.t.cx = 380; buf.t.cy = 260;
        wcscpy_s(buf.title, L"Manage Calendars");
        if (!InitModalIndirect((DLGTEMPLATE*)&buf, m_pParentWnd)) return -1;
        return CDialog::DoModal();
    }

protected:
    BOOL OnInitDialog() override
    {
        CDialog::OnInitDialog();
        SetWindowText(L"Manage Calendars");

        HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CFont* pF = CFont::FromHandle(hF);
        CRect rc; GetClientRect(&rc);
        int W = rc.Width(), H = rc.Height();

        m_listCals.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            CRect(8, 8, W - 8, H - 80), this, 401);
        m_listCals.SetFont(pF);

        m_editUrl.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            CRect(8, H - 68, W - 8, H - 48), this, 402);
        m_editUrl.SetFont(pF);
        ::SendMessage(m_editUrl.GetSafeHwnd(), EM_SETCUEBANNER, TRUE,
            (LPARAM)L"Paste calendar URL here (https://...)  then click Add");

        auto mkBtn = [&](const wchar_t* t, int x, int w, UINT id) {
            CButton* b = new CButton;
            b->Create(t, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                CRect(x, H - 38, x + w, H - 8), this, id);
            b->SetFont(pF);
        };
        mkBtn(L"Add",    8,   54, 411);
        mkBtn(L"Remove", 66,  64, 412);
        mkBtn(L"Toggle", 134, 58, 415);
        mkBtn(L"Up",     196, 40, 413);
        mkBtn(L"Down",   240, 48, 414);
        mkBtn(L"OK",    W - 138, 60, IDOK);
        mkBtn(L"Cancel",W - 74,  66, IDCANCEL);

        RefreshList();
        return TRUE;
    }

    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override
    {
        UINT id = LOWORD(wParam);
        if (id == 411) { // Add
            CString s; m_editUrl.GetWindowText(s); s.Trim();
            if (!s.IsEmpty()) {
                calendars.push_back({ (LPCWSTR)s, true });
                RefreshList();
                m_listCals.SetCurSel((int)calendars.size() - 1);
                m_editUrl.SetWindowText(L"");
            }
            return TRUE;
        }
        if (id == 412) { // Remove
            int sel = m_listCals.GetCurSel();
            if (sel != LB_ERR && sel < (int)calendars.size()) {
                calendars.erase(calendars.begin() + sel);
                RefreshList();
                if (sel < (int)calendars.size()) m_listCals.SetCurSel(sel);
                else if (!calendars.empty()) m_listCals.SetCurSel((int)calendars.size() - 1);
            }
            return TRUE;
        }
        if (id == 413) { // Up
            int sel = m_listCals.GetCurSel();
            if (sel > 0 && sel < (int)calendars.size()) {
                std::swap(calendars[sel], calendars[sel - 1]);
                RefreshList(); m_listCals.SetCurSel(sel - 1);
            }
            return TRUE;
        }
        if (id == 414) { // Down
            int sel = m_listCals.GetCurSel();
            if (sel >= 0 && sel + 1 < (int)calendars.size()) {
                std::swap(calendars[sel], calendars[sel + 1]);
                RefreshList(); m_listCals.SetCurSel(sel + 1);
            }
            return TRUE;
        }
        if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == 401) {
            int sel = m_listCals.GetCurSel();
            if (sel != LB_ERR && sel < (int)calendars.size())
                m_editUrl.SetWindowText(calendars[sel].url.c_str());
            return TRUE;
        }
        if (HIWORD(wParam) == LBN_DBLCLK && LOWORD(wParam) == 401) {
            DoToggle();
            return TRUE;
        }
        if (id == 415) { // Toggle on/off
            DoToggle();
            return TRUE;
        }
        return CDialog::OnCommand(wParam, lParam);
    }

    void OnOK() override { CDialog::OnOK(); }
    DECLARE_MESSAGE_MAP()

private:
    CListBox m_listCals;
    CEdit    m_editUrl;

    void DoToggle()
    {
        int sel = m_listCals.GetCurSel();
        if (sel != LB_ERR && sel < (int)calendars.size()) {
            calendars[sel].enabled = !calendars[sel].enabled;
            RefreshList();
            m_listCals.SetCurSel(sel);
        }
    }

    void RefreshList()
    {
        m_listCals.ResetContent();
        for (const auto& c : calendars)
        {
            std::wstring disp = (c.enabled ? L"[ON]  " : L"[OFF] ") + c.url;
            m_listCals.AddString(disp.c_str());
        }
    }
};

BEGIN_MESSAGE_MAP(CWebCalDlg, CDialog)
END_MESSAGE_MAP()

static std::wstring ReminderEscape(const std::wstring& s)
{
    std::wstring out;
    for (wchar_t ch : s)
    {
        if (ch == L'\\' || ch == L'|') out += L'\\';
        out += ch;
    }
    return out;
}

static std::vector<std::wstring> ReminderSplit(const std::wstring& line)
{
    std::vector<std::wstring> parts;
    std::wstring cur;
    bool esc = false;
    for (wchar_t ch : line)
    {
        if (esc) { cur += ch; esc = false; continue; }
        if (ch == L'\\') { esc = true; continue; }
        if (ch == L'|') { parts.push_back(cur); cur.clear(); continue; }
        cur += ch;
    }
    parts.push_back(cur);
    return parts;
}

static void ParseReminderOffsetText(const std::wstring& text, CString& amount, int& unitSel)
{
    amount = L"15";
    unitSel = 0;
    if (text.empty()) return;

    std::wstring lower;
    lower.reserve(text.size());
    for (wchar_t ch : text)
        lower.push_back((wchar_t)towlower(ch));

    std::wstring digits;
    for (wchar_t ch : lower)
    {
        if ((ch >= L'0' && ch <= L'9') || ch == L'.')
            digits.push_back(ch);
        else if (!digits.empty())
            break;
    }
    if (!digits.empty())
        amount = digits.c_str();

    if (lower.find(L"year") != std::wstring::npos) unitSel = 5;
    else if (lower.find(L"month") != std::wstring::npos) unitSel = 4;
    else if (lower.find(L"week") != std::wstring::npos) unitSel = 3;
    else if (lower.find(L"day") != std::wstring::npos) unitSel = 2;
    else if (lower.find(L"hour") != std::wstring::npos) unitSel = 1;
    else unitSel = 0;
}

static bool ParseClockText(const std::wstring& text, int& hour24, int& minute)
{
    std::wstring s;
    for (wchar_t ch : text)
        if (!iswspace(ch))
            s.push_back((wchar_t)towlower(ch));

    bool pm = s.find(L"pm") != std::wstring::npos;
    bool am = s.find(L"am") != std::wstring::npos;
    size_t colon = s.find(L':');
    if (colon == std::wstring::npos)
        return false;

    hour24 = _wtoi(s.substr(0, colon).c_str());
    minute = _wtoi(s.substr(colon + 1).c_str());
    if (minute < 0 || minute > 59)
        return false;

    if (pm && hour24 < 12) hour24 += 12;
    if (am && hour24 == 12) hour24 = 0;
    if (hour24 < 0 || hour24 > 23)
        return false;
    return true;
}

static void FillClockCombos(CComboBox& hour, CComboBox& minute, CComboBox& ampm)
{
    for (int i = 1; i <= 12; ++i)
    {
        CString text;
        text.Format(L"%d", i);
        hour.AddString(text);
    }
    for (int i = 0; i < 60; ++i)
    {
        CString text;
        text.Format(L"%02d", i);
        minute.AddString(text);
    }
    ampm.AddString(L"AM");
    ampm.AddString(L"PM");
}

static void SetClockCombos(CComboBox& hour, CComboBox& minute, CComboBox& ampm,
    const std::wstring& value, int fallbackHour24, int fallbackMinute)
{
    int h = fallbackHour24;
    int m = fallbackMinute;
    ParseClockText(value, h, m);
    bool isPm = h >= 12;
    int displayHour = h % 12;
    if (displayHour == 0) displayHour = 12;
    hour.SetCurSel(max(0, min(11, displayHour - 1)));
    minute.SetCurSel(max(0, min(59, m)));
    ampm.SetCurSel(isPm ? 1 : 0);
}

static std::wstring ReadClockCombos(const CComboBox& hour, const CComboBox& minute,
    const CComboBox& ampm)
{
    int hSel = max(0, hour.GetCurSel());
    int mSel = max(0, minute.GetCurSel());
    int aSel = max(0, ampm.GetCurSel());
    int h = hSel + 1;
    CString out;
    out.Format(L"%d:%02d %s", h, mSel, aSel == 1 ? L"PM" : L"AM");
    return (LPCWSTR)out;
}

static void SetOffsetControls(const std::wstring& value, CEdit& amount, CComboBox& unit)
{
    CString amt;
    int unitSel = 0;
    ParseReminderOffsetText(value, amt, unitSel);
    amount.SetWindowText(amt);
    int count = unit.GetCount();
    unit.SetCurSel(max(0, min(max(0, count - 1), unitSel)));
}

static std::wstring ReadOffsetControls(const CEdit& amount, const CComboBox& unit)
{
    CString amt;
    amount.GetWindowText(amt);
    amt.Trim();
    if (amt.IsEmpty())
        amt = L"15";
    int unitSel = max(0, unit.GetCurSel());
    if (unitSel >= unit.GetCount())
        unitSel = max(0, unit.GetCount() - 1);
    CString unitText;
    unit.GetLBText(unitSel, unitText);
    return std::wstring((LPCWSTR)amt) + L" " + std::wstring((LPCWSTR)unitText);
}

class CReminderEditDlg : public CDialog
{
public:
    ReminderRule rule;

    CReminderEditDlg(const ReminderRule* existing, CWnd* parent = nullptr) : CDialog()
    {
        if (existing) rule = *existing;
        if (rule.kind.empty()) rule.kind = L"Zman";
        if (rule.target.empty()) {
            if (rule.kind == L"Shmita Year") rule.target = L"Rosh Hashana of Shmita Year";
            else if (rule.kind == L"Simple Year (12 months)") rule.target = L"Rosh Hashana";
            else if (rule.kind == L"Full/Leap Year (13 months)") rule.target = L"Rosh Hashana";
            else if (rule.kind == L"28-Year Solar Cycle") rule.target = L"Birchas Hachama (April 8)";
            else rule.target = L"Shkiah";
        }
        if (rule.offsets.empty()) rule.offsets = L"15 minutes";
        m_pParentWnd = parent;
    }

    INT_PTR DoModal() override
    {
        struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[32]; } b = {};
        b.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_CENTER;
        b.t.dwExtendedStyle = WS_EX_APPWINDOW;
        b.t.cx = 360; b.t.cy = 235;  // expanded for anchor row (v0.8.71)
        wcscpy_s(b.title, L"Reminder");
        if (!InitModalIndirect((DLGTEMPLATE*)&b, m_pParentWnd)) return -1;
        return CDialog::DoModal();
    }

protected:
    BOOL OnInitDialog() override
    {
        CDialog::OnInitDialog();
        HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CFont* pF = CFont::FromHandle(hF);
        CRect rc; GetClientRect(&rc);
        int W = rc.Width(), H = rc.Height();
        auto label = [&](const wchar_t* text, int x, int y, int w) {
            CStatic* s = new CStatic;
            s->Create(text, WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                CRect(x, y, x + w, y + 22), this, 6200 + m_nextId++);
            s->SetFont(pF);
        };
        label(L"Type:", 10, 12, 70);
        m_kind.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            CRect(82, 12, W - 10, 150), this, 6101);
        m_kind.SetFont(pF);
        for (const wchar_t* k : { L"Zman", L"Parsha", L"Holiday", L"Personal Event",
                                   L"Shmita Year", L"Simple Year (12 months)", L"Full/Leap Year (13 months)",
                                   L"28-Year Solar Cycle" })
            m_kind.AddString(k);
        m_kind.SetCurSel(0);
        for (int i = 0; i < m_kind.GetCount(); ++i) {
            CString v; m_kind.GetLBText(i, v);
            if (std::wstring((LPCWSTR)v) == rule.kind) { m_kind.SetCurSel(i); break; }
        }

        label(L"Target:", 10, 44, 70);
        m_target.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_VSCROLL,
            CRect(82, 44, W - 10, 220), this, 6102);
        m_target.SetFont(pF);
        FillTargets();
        m_target.SetWindowText(rule.target.c_str());

        // "Offset:" label (replaces the old hardcoded "Before:")
        label(L"Offset:", 10, 76, 70);
        CString offsetAmount;
        int offsetUnit = 0;
        ParseReminderOffsetText(rule.offsets, offsetAmount, offsetUnit);
        m_offsetAmount.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
            CRect(82, 76, 136, 98), this, 6103);
        m_offsetAmount.SetFont(pF);
        m_offsetAmount.SetWindowText(offsetAmount);
        // Unit dropdown — narrowed to make room for the direction dropdown
        m_offsetUnit.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            CRect(145, 76, 240, 210), this, 6106);
        m_offsetUnit.SetFont(pF);
        for (const wchar_t* unit : { L"minutes", L"hours", L"days", L"weeks", L"months", L"years" })
            m_offsetUnit.AddString(unit);
        m_offsetUnit.SetCurSel(max(0, min(5, offsetUnit)));
        // Before / After direction dropdown (v0.8.70)
        m_direction.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            CRect(248, 76, W - 10, 200), this, 6107);
        m_direction.SetFont(pF);
        m_direction.AddString(L"Before");
        m_direction.AddString(L"After");
        m_direction.SetCurSel(rule.afterEvent ? 1 : 0);

        label(L"Notify:", 10, 108, 70);
        m_style.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            CRect(82, 108, 210, 230), this, 6104);
        m_style.SetFont(pF);
        for (const wchar_t* s : { L"Off", L"Toast", L"Popup", L"Toast + Popup" })
            m_style.AddString(s);
        m_style.SetCurSel(max(0, min(3, rule.style)));
        m_enabled.Create(L"Enabled", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            CRect(220, 108, W - 10, 130), this, 6105);
        m_enabled.SetFont(pF);
        m_enabled.SetCheck(rule.enabled ? BST_CHECKED : BST_UNCHECKED);

        // Anchor row (v0.8.71) — currently used only by Holiday reminders.
        m_anchorLabel.Create(L"Anchor:", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
            CRect(10, 140, 80, 162), this, 6109);
        m_anchorLabel.SetFont(pF);
        m_anchor.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            CRect(82, 140, W - 10, 260), this, 6108);
        m_anchor.SetFont(pF);
        m_anchor.AddString(L"Tzais (custom)");   // 0 — default
        m_anchor.AddString(L"Shkia");             // 1
        m_anchor.AddString(L"Civil midnight");    // 2
        m_anchor.SetCurSel(max(0, min(2, rule.anchor)));
        UpdateAnchorEnabled();

        CButton* ok = new CButton;
        ok->Create(L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            CRect(W - 146, H - 34, W - 78, H - 8), this, IDOK);
        ok->SetFont(pF);
        CButton* cancel = new CButton;
        cancel->Create(L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            CRect(W - 72, H - 34, W - 8, H - 8), this, IDCANCEL);
        cancel->SetFont(pF);
        return TRUE;
    }

    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override
    {
        if (LOWORD(wParam) == 6101 && HIWORD(wParam) == CBN_SELCHANGE)
        {
            FillTargets();
            // Set a sensible default target for the new kind
            CString kind; m_kind.GetWindowText(kind);
            if (kind == L"Shmita Year")
                m_target.SetWindowText(L"Rosh Hashana of Shmita Year");
            else if (kind == L"Simple Year (12 months)" || kind == L"Full/Leap Year (13 months)")
                m_target.SetWindowText(L"Rosh Hashana");
            else if (kind == L"28-Year Solar Cycle")
                m_target.SetWindowText(L"Birchas Hachama (April 8)");
            else if (kind == L"Holiday")
                m_target.SetWindowText(L"Rosh Hashana");
            UpdateAnchorEnabled();
            return TRUE;
        }
        return CDialog::OnCommand(wParam, lParam);
    }

    void OnOK() override
    {
        CString text;
        m_kind.GetWindowText(text); rule.kind = (LPCWSTR)text;
        m_target.GetWindowText(text); rule.target = (LPCWSTR)text;
        CString amount;
        m_offsetAmount.GetWindowText(amount);
        amount.Trim();
        int unitSel = max(0, m_offsetUnit.GetCurSel());
        CString unit;
        m_offsetUnit.GetLBText(unitSel, unit);
        rule.offsets = std::wstring((LPCWSTR)amount) + L" " + std::wstring((LPCWSTR)unit);
        rule.afterEvent = (m_direction.GetCurSel() == 1); // 0=Before, 1=After
        rule.anchor     = max(0, m_anchor.GetCurSel());   // 0=Tzais, 1=Shkia, 2=Midnight
        rule.style = max(0, m_style.GetCurSel());
        rule.enabled = (m_enabled.GetCheck() == BST_CHECKED);
        if (rule.target.empty() || amount.IsEmpty()) {
            MessageBox(L"Please enter a target and reminder time.", L"WinLuach", MB_OK | MB_ICONWARNING);
            return;
        }
        CDialog::OnOK();
    }

private:
    void FillTargets()
    {
        CString kind; if (m_kind.GetSafeHwnd()) m_kind.GetWindowText(kind);
        CString cur; if (m_target.GetSafeHwnd()) m_target.GetWindowText(cur);
        if (!m_target.GetSafeHwnd()) return;
        m_target.ResetContent();

        auto addMany = [&](std::initializer_list<const wchar_t*> values) {
            for (const wchar_t* v : values) m_target.AddString(v);
        };
        if (kind == L"Parsha")
            addMany({ L"Bereshis", L"Noach", L"Lech Lecha", L"Vayera", L"Chayei Sarah", L"Toldos",
                L"Vayetzei", L"Vayishlach", L"Vayeshev", L"Miketz", L"Vayigash", L"Vayechi",
                L"Shemos", L"Vaera", L"Bo", L"Beshalach", L"Yisro", L"Mishpatim", L"Terumah",
                L"Tetzaveh", L"Ki Sisa", L"Vayakhel", L"Pekudei", L"Vayikra", L"Tzav", L"Shemini",
                L"Tazria", L"Metzora", L"Acharei Mos", L"Kedoshim", L"Emor", L"Behar", L"Bechukosai",
                L"Bamidbar", L"Naso", L"Behaaloscha", L"Shelach", L"Korach", L"Chukas", L"Balak",
                L"Pinchas", L"Matos", L"Masei", L"Devarim", L"Vaeschanan", L"Eikev", L"Reeh",
                L"Shoftim", L"Ki Seitzei", L"Ki Savo", L"Nitzavim", L"Vayelech", L"Haazinu" });
        else if (kind == L"Holiday")
            // v0.8.70: expanded to include all holiday categories from HolidayEngine
            addMany({
                // --- Major Yom Tov ---
                L"Rosh Hashana", L"Yom Kippur",
                L"Sukkos", L"Shemini Atzeres", L"Simchas Torah",
                L"Pesach", L"Shavuos",
                // --- Chol HaMoed ---
                L"Chol HaMoed Sukkos", L"Chol HaMoed Pesach", L"Hoshana Raba",
                // --- Rosh Chodesh ---
                L"Rosh Chodesh (Day 1)", L"Rosh Chodesh (Day 2)",
                // --- Fast Days ---
                L"Fast of Gedaliah", L"Fast of 10 Teves", L"Fast of Esther",
                L"Fast of 17 Tammuz", L"Tisha B'Av",
                // --- Minor Holidays ---
                L"Chanukah", L"Tu BiShvat", L"Purim", L"Shushan Purim",
                L"Purim Katan", L"Shushan Purim Katan",
                L"Pesach Sheini", L"Lag BaOmer", L"Tu B'Av",
                L"Isru Chag",
                // --- Israeli / Modern Holidays ---
                L"Yom HaShoah", L"Yom Hazikaron", L"Yom HaAtzmaut", L"Yom Yerushalayim",
            });
        else if (kind == L"Personal Event")
            addMany({ L"Any personal event", L"Birthday", L"Anniversary", L"Yahrzeit", L"Custom" });
        else if (kind == L"Shmita Year")
            addMany({ L"Rosh Hashana of Shmita Year", L"Erev Rosh Hashana of Shmita Year",
                      L"Start of Shmita (1 Tishrei)", L"End of Shmita (29 Elul)" });
        else if (kind == L"Simple Year (12 months)")
            addMany({ L"Rosh Hashana", L"Erev Rosh Hashana", L"1 Tishrei" });
        else if (kind == L"Full/Leap Year (13 months)")
            addMany({ L"Rosh Hashana", L"Erev Rosh Hashana", L"1 Tishrei",
                      L"Rosh Chodesh Adar I", L"Rosh Chodesh Adar II" });
        else if (kind == L"28-Year Solar Cycle")
            addMany({ L"Birchas Hachama (April 8)", L"1 Nissan of Cycle Year",
                      L"Start of New Solar Cycle" });
        else
            // v0.8.70: added 7 custom zmanim entries at the end
            addMany({
                // Standard / preset zmanim
                L"Alos (GRA 16.1 deg)", L"Alos (MA 72)", L"Alos (MA 90)",
                L"Alos (MA 72 proportional)", L"Alos (MA 90 proportional)",
                L"Misheyakir (10.2 deg)", L"Misheyakir (11.5 deg)",
                L"Hanetz",
                L"Sof Shema (GRA)", L"Sof Shema (MA 72)", L"Sof Shema (MA 90)",
                L"Sof Tefilla (GRA)", L"Sof Tefilla (MA 72)", L"Sof Tefilla (MA 90)",
                L"Chatzos",
                L"Mincha Gedola (GRA)", L"Mincha Gedola (MA 72)", L"Mincha Gedola (MA 90)",
                L"Mincha Ketana (GRA)", L"Mincha Ketana (MA 72)", L"Mincha Ketana (MA 90)",
                L"Plag (GRA)", L"Plag (MA 72)", L"Plag (MA 90)",
                L"Shkiah",
                L"Tzeis (GRA 8.5 deg)", L"Tzeis (MA 72)", L"Tzeis (MA 90)",
                L"Tzeis (MA 72 proportional)", L"Tzeis (MA 90 proportional)",
                L"Candle Lighting", L"Fast start/end", L"Eat chametz", L"Burn chametz",
                // Custom zmanim (use the user's chosen shita from Settings → Zmanim)
                L"Custom Alos", L"Custom Sof Shema", L"Custom Sof Tefilla",
                L"Custom Mincha Gedola", L"Custom Mincha Ketana",
                L"Custom Plag HaMincha", L"Custom Tzeit"
            });
        if (!cur.IsEmpty()) m_target.SetWindowText(cur);
    }

    bool CurrentSelectionUsesAnchor() const
    {
        CString kind;
        if (m_kind.GetSafeHwnd())
            m_kind.GetWindowText(kind);
        return kind == L"Holiday";
    }

    void UpdateAnchorEnabled()
    {
        const BOOL enabled = CurrentSelectionUsesAnchor() ? TRUE : FALSE;
        if (m_anchorLabel.GetSafeHwnd())
            m_anchorLabel.EnableWindow(enabled);
        if (m_anchor.GetSafeHwnd())
            m_anchor.EnableWindow(enabled);
    }

    int m_nextId = 0;
    CComboBox m_kind, m_target, m_style;
    CComboBox m_offsetUnit;
    CComboBox m_direction;  // Before / After (v0.8.70)
    CComboBox m_anchor;     // Tzais / Shkia / Civil midnight (v0.8.71)
    CEdit m_offsetAmount;
    CButton m_enabled;
    CStatic m_anchorLabel;
};

class CAdvancedRemindersDlg : public CDialog
{
public:
    explicit CAdvancedRemindersDlg(std::vector<ReminderRule>& reminders, CWnd* parent = nullptr)
        : CDialog(), m_reminders(reminders)
    {
        m_pParentWnd = parent;
    }

    INT_PTR DoModal() override
    {
        struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[40]; } b = {};
        b.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_CENTER;
        b.t.dwExtendedStyle = WS_EX_APPWINDOW;
        b.t.cx = 430; b.t.cy = 260;
        wcscpy_s(b.title, L"Advanced Reminders");
        if (!InitModalIndirect((DLGTEMPLATE*)&b, m_pParentWnd)) return -1;
        return CDialog::DoModal();
    }

protected:
    BOOL OnInitDialog() override
    {
        CDialog::OnInitDialog();
        HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CFont* pF = CFont::FromHandle(hF);
        CRect rc; GetClientRect(&rc);
        int W = rc.Width(), H = rc.Height();
        m_list.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY,
            CRect(10, 10, W - 104, H - 10), this, 6300);
        m_list.SetFont(pF);
        auto btn = [&](const wchar_t* t, int y, UINT id) {
            CButton* b = new CButton;
            b->Create(t, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                CRect(W - 92, y, W - 8, y + 25), this, id);
            b->SetFont(pF);
        };
        btn(L"Add", 10, 6301);
        btn(L"Edit", 40, 6302);
        btn(L"Delete", 70, 6303);
        btn(L"Import", 110, 6304);
        btn(L"Export", 140, 6305);
        btn(L"Close", H - 35, IDOK);
        Refresh();
        return TRUE;
    }

    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override
    {
        switch (LOWORD(wParam))
        {
        case 6301: Add(); return TRUE;
        case 6302: Edit(); return TRUE;
        case 6303: Delete(); return TRUE;
        case 6304: Import(); return TRUE;
        case 6305: Export(); return TRUE;
        case 6300:
            if (HIWORD(wParam) == LBN_DBLCLK) { Edit(); return TRUE; }
            break;
        }
        return CDialog::OnCommand(wParam, lParam);
    }

private:
    std::wstring Summary(const ReminderRule& r) const
    {
        std::wstring s = r.enabled ? L"[on] " : L"[off] ";
        s += r.kind + L": " + r.target + L"  before " + r.offsets;
        return s;
    }

    void Refresh()
    {
        m_list.ResetContent();
        for (const auto& r : m_reminders)
            m_list.AddString(Summary(r).c_str());
    }

    void Add()
    {
        CReminderEditDlg dlg(nullptr, this);
        if (dlg.DoModal() == IDOK) { m_reminders.push_back(dlg.rule); Refresh(); }
    }

    void Edit()
    {
        int sel = m_list.GetCurSel();
        if (sel < 0 || sel >= (int)m_reminders.size()) return;
        CReminderEditDlg dlg(&m_reminders[sel], this);
        if (dlg.DoModal() == IDOK) { m_reminders[sel] = dlg.rule; Refresh(); m_list.SetCurSel(sel); }
    }

    void Delete()
    {
        int sel = m_list.GetCurSel();
        if (sel < 0 || sel >= (int)m_reminders.size()) return;
        m_reminders.erase(m_reminders.begin() + sel);
        Refresh();
    }

    void Export()
    {
        wchar_t path[MAX_PATH] = L"WinLuach reminders.txt";
        OPENFILENAMEW ofn = { sizeof(ofn) };
        ofn.hwndOwner = GetSafeHwnd();
        ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = path;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrDefExt = L"txt";
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        if (!GetSaveFileNameW(&ofn)) return;
        std::wofstream f(path);
        if (!f.is_open()) return;
        for (const auto& r : m_reminders)
            f << (r.enabled ? 1 : 0) << L"|" << r.style << L"|"
              << ReminderEscape(r.kind) << L"|" << ReminderEscape(r.target) << L"|"
              << ReminderEscape(r.offsets) << L"\n";
    }

    void Import()
    {
        wchar_t path[MAX_PATH] = {};
        OPENFILENAMEW ofn = { sizeof(ofn) };
        ofn.hwndOwner = GetSafeHwnd();
        ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = path;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (!GetOpenFileNameW(&ofn)) return;
        std::wifstream f(path);
        if (!f.is_open()) return;
        std::wstring line;
        while (std::getline(f, line))
        {
            auto p = ReminderSplit(line);
            if (p.size() < 5) continue;
            ReminderRule r;
            r.enabled = (p[0] == L"1");
            r.style = _wtoi(p[1].c_str());
            r.kind = p[2];
            r.target = p[3];
            r.offsets = p[4];
            m_reminders.push_back(r);
        }
        Refresh();
    }

    std::vector<ReminderRule>& m_reminders;
    CListBox m_list;
};

BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
    ON_BN_CLICKED(IDC_OPT_TRAY_COLOR,      &COptionsDlg::OnTrayTextColor)
    ON_BN_CLICKED(IDC_OPT_TRAY_BACK_COLOR, &COptionsDlg::OnTrayBackColor)
    ON_BN_CLICKED(IDC_OPT_TRAY_FONT,       &COptionsDlg::OnTrayFont)
    ON_BN_CLICKED(IDC_OPT_TRAY_DEFAULTS,   &COptionsDlg::OnTrayDefaults)
    ON_BN_CLICKED(IDC_OPT_MANAGE_CALS,     &COptionsDlg::OnManageCals)
    ON_BN_CLICKED(IDC_OPT_PREVIEW_NOTIFY,  &COptionsDlg::OnPreviewNotification)
    ON_BN_CLICKED(IDC_OPT_ADV_REMINDERS,   &COptionsDlg::OnAdvancedReminders)
    ON_BN_CLICKED(IDC_OPT_APPLY,           &COptionsDlg::OnApply)
    ON_BN_CLICKED(IDC_OPT_DISABLE_AUTOUPDATE, &COptionsDlg::OnDisableAutoUpdate)
    ON_BN_CLICKED(IDC_OPT_CHECK_NOW,       &COptionsDlg::OnCheckNow)
    ON_BN_CLICKED(IDC_OPT_SHOW_BEHAB,     &COptionsDlg::OnShowBeHaBChanged) // v0.8.78
    ON_CONTROL_RANGE(BN_CLICKED, IDC_OPT_TRAY_TIP_ZMAN_FIRST, IDC_OPT_TRAY_TIP_ZMAN_FIRST + 30, OnTrayTipZmanClicked)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_OPT_TRAY_TIP_CUSTOM_ZMAN_FIRST, IDC_OPT_TRAY_TIP_CUSTOM_ZMAN_FIRST + 6, OnTrayTipZmanClicked)
    ON_NOTIFY(TCN_SELCHANGE, IDC_OPT_TAB,    &COptionsDlg::OnTabChanged)
    ON_NOTIFY(TCN_SELCHANGE, IDC_OPT_ZSUBTAB, &COptionsDlg::OnZmanimSubTabChanged)
    ON_NOTIFY(TCN_SELCHANGE, IDC_OPT_NOTIFY_SUBTAB, &COptionsDlg::OnNotifySubTabChanged)
    ON_WM_SIZE()
END_MESSAGE_MAP()

// =============================================================================
// CONSTRUCTION
// =============================================================================

static void NormalizeCustomZmanSettings(AppSettings& s)
{
    s.customAlotMode = max(0, min(2, s.customAlotMode));
    if (s.customAlotValue <= 0.0) s.customAlotValue = 16.1;
    s.customMisheyakirMode = max(0, min(2, s.customMisheyakirMode));
    if (s.customMisheyakirValue <= 0.0) s.customMisheyakirValue = 10.2;
    s.customTzeitMode = max(0, min(2, s.customTzeitMode));
    if (s.customTzeitValue <= 0.0) s.customTzeitValue = 8.5;
    s.shaahZmanitShita = max(0, min(4, s.shaahZmanitShita));
    s.sofZmanShaahMode = max(0, min(1, s.sofZmanShaahMode));
    s.customShaahStartMode = max(0, min(1, s.customShaahStartMode));
    if (s.customShaahStartValue <= 0.0) s.customShaahStartValue = 72.0;
    if (s.customShaahStartDegreesValue <= 0.0) s.customShaahStartDegreesValue = 16.1;
    s.customShaahEndMode = max(0, min(1, s.customShaahEndMode));
    if (s.customShaahEndValue <= 0.0) s.customShaahEndValue = 72.0;
    if (s.customShaahEndDegreesValue <= 0.0) s.customShaahEndDegreesValue = 8.5;
    s.customSofZmanMode = max(0, min(2, s.customSofZmanMode));
    if (s.customSofZmanValue < 0.0)
    {
        if (s.zmanimShita == 1) { s.customSofZmanMode = 1; s.customSofZmanValue = 72.0; }
        else if (s.zmanimShita == 2) { s.customSofZmanMode = 1; s.customSofZmanValue = 90.0; }
        else { s.customSofZmanMode = 0; s.customSofZmanValue = 0.0; }
    }
}

// =============================================================================
// ShowManageCalendarsDialog — simple Add/Remove/Enable dialog for ICS feeds
// =============================================================================

#define IDC_MNGCAL_LIST    1001
#define IDC_MNGCAL_ADD     1002
#define IDC_MNGCAL_REMOVE  1003
#define IDC_MNGCAL_TOGGLE  1004
#define IDC_MNGCAL_URL     1005

class CManageCalendarsDlg : public CDialog
{
public:
    std::vector<WebCalEntry>& m_cals;
    CWnd* m_pParent = nullptr;
    bool m_changed = false;

    CListBox  m_list;
    CEdit     m_editUrl;
    CButton   m_btnAdd, m_btnRemove, m_btnToggle, m_btnClose;

    CManageCalendarsDlg(std::vector<WebCalEntry>& cals, CWnd* pParent)
        : CDialog(), m_cals(cals), m_pParent(pParent)
    {
    }

    virtual INT_PTR DoModal() override
    {
        DLGTEMPLATE tpl = {};
        tpl.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER;
        tpl.cx = 280; tpl.cy = 200;
        return InitModalIndirect(&tpl, m_pParent);
    }

    virtual BOOL OnInitDialog() override
    {
        CDialog::OnInitDialog();
        SetWindowText(L"Manage Web Calendars");
        CRect rc; GetClientRect(&rc);
        int W = rc.Width(), H = rc.Height();
        int p = 8;

        m_list.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
            CRect(p, p, W - p, H - 70), this, IDC_MNGCAL_LIST);
        m_editUrl.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            CRect(p, H - 62, W - 76, H - 42), this, IDC_MNGCAL_URL);
        m_editUrl.SetFont(GetFont());
        m_btnAdd.Create(L"Add", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            CRect(W - 72, H - 62, W - p, H - 42), this, IDC_MNGCAL_ADD);
        m_btnToggle.Create(L"Enable/Disable", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            CRect(p, H - 36, 110, H - p), this, IDC_MNGCAL_TOGGLE);
        m_btnRemove.Create(L"Remove", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            CRect(114, H - 36, 190, H - p), this, IDC_MNGCAL_REMOVE);
        m_btnClose.Create(L"Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            CRect(W - 72, H - 36, W - p, H - p), this, IDOK);

        CFont* f = GetFont();
        m_list.SetFont(f); m_btnAdd.SetFont(f); m_btnToggle.SetFont(f);
        m_btnRemove.SetFont(f); m_btnClose.SetFont(f);
        RebuildList();
        return TRUE;
    }

    void RebuildList()
    {
        m_list.ResetContent();
        for (const auto& e : m_cals)
        {
            CString s;
            s.Format(L"%s  %s", e.enabled ? L"[on] " : L"[off]", e.url.c_str());
            m_list.AddString(s);
        }
    }

    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam) override
    {
        UINT id = LOWORD(wParam), code = HIWORD(wParam);
        if (id == IDC_MNGCAL_ADD && code == BN_CLICKED)
        {
            CString url; m_editUrl.GetWindowText(url); url.Trim();
            if (!url.IsEmpty())
            {
                WebCalEntry e; e.url = (LPCWSTR)url; e.enabled = true;
                m_cals.push_back(e);
                m_editUrl.SetWindowText(L"");
                RebuildList();
                m_changed = true;
            }
            return TRUE;
        }
        if (id == IDC_MNGCAL_REMOVE && code == BN_CLICKED)
        {
            int sel = m_list.GetCurSel();
            if (sel >= 0 && sel < (int)m_cals.size())
            {
                m_cals.erase(m_cals.begin() + sel);
                RebuildList();
                m_changed = true;
            }
            return TRUE;
        }
        if (id == IDC_MNGCAL_TOGGLE && code == BN_CLICKED)
        {
            int sel = m_list.GetCurSel();
            if (sel >= 0 && sel < (int)m_cals.size())
            {
                m_cals[sel].enabled = !m_cals[sel].enabled;
                RebuildList();
                m_list.SetCurSel(sel);
                m_changed = true;
            }
            return TRUE;
        }
        return CDialog::OnCommand(wParam, lParam);
    }

    DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CManageCalendarsDlg, CDialog)
END_MESSAGE_MAP()

bool ShowManageCalendarsDialog(std::vector<WebCalEntry>& calendars, CWnd* pParent)
{
    CManageCalendarsDlg dlg(calendars, pParent);
    dlg.DoModal();
    return dlg.m_changed;
}

// =============================================================================

COptionsDlg::COptionsDlg(const AppSettings& current, CWnd* pParent)
    : CDialog(), m_current(current), m_result(current)
{
    m_pParentWnd = pParent;
    NormalizeCustomZmanSettings(m_current);
    m_result = m_current;
}

static void FillOptionsTemplate(DLGTEMPLATE& t, wchar_t* title)
{
    t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_CENTER;
    t.dwExtendedStyle = WS_EX_APPWINDOW;
    t.cdit = 0;
    t.x = 0;
    t.y = 0;
    t.cx = 442;
    t.cy = 420;
    wcscpy_s(title, 32, L"Options and Preferences");
}

// =============================================================================
// DOMODAL — in-memory dialog template
// =============================================================================

INT_PTR COptionsDlg::DoModal()
{
    struct DlgTmpl {
        DLGTEMPLATE t;
        WORD menu;
        WORD cls;
        wchar_t title[32];
    } buf = {};

    m_modeless = false;
    FillOptionsTemplate(buf.t, buf.title);

    if (!InitModalIndirect((DLGTEMPLATE*)&buf, m_pParentWnd))
        return -1;

    return CDialog::DoModal();
}

BOOL COptionsDlg::CreateModeless(CWnd* pParent)
{
    struct DlgTmpl {
        DLGTEMPLATE t;
        WORD menu;
        WORD cls;
        wchar_t title[32];
    } buf = {};

    m_modeless = true;
    m_pParentWnd = pParent;
    FillOptionsTemplate(buf.t, buf.title);
    return CreateIndirect((DLGTEMPLATE*)&buf, pParent);
}



// =============================================================================
// ON INIT DIALOG — creates all controls
// =============================================================================

BOOL COptionsDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    SetWindowText(L"Options and Preferences");

    CRect rcClient;
    GetClientRect(&rcClient);
    int W = rcClient.Width();
    int H = rcClient.Height();

    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    CFont* pFont = CFont::FromHandle(hFont);

    m_pageGeneral.clear();
    m_pageMonth.clear();
    m_pageInterface.clear();
    m_pageTrayTooltip.clear();
    m_pageColors.clear();
    m_pageZmanim.clear();
    m_pageZmanShitos.clear();
    m_pageNotifications.clear();
    m_pageYearDetails.clear();
    // v0.8.0 sub-pages + zmanim bar page.
    m_pageZmanimBar.clear();
    m_subPageAlos.clear();
    m_subPageMisheyakir.clear();
    m_subPageSofMA.clear();
    m_subPageSofGRA.clear();
    m_subPageMinchaGedola.clear();
    m_subPageMinchaKetana.clear();
    m_subPagePlag.clear();
    m_subPageEndFast.clear();
    m_subPageTzais.clear();
    m_subPageShaahZmanit.clear();      // v0.8.83
    m_pageNotifyGeneral.clear();
    m_pageNotifyZmanim.clear();
    m_pageNotifyReminders.clear();
    m_zmanimBarChecks.clear();
    m_radAlosPreset.clear();
    m_radMisheyakirPreset.clear();
    m_radSofMAPreset.clear();
    m_radSofGRAPreset.clear();
    m_radMinchaGedolaPreset.clear();
    m_radMinchaKetanaPreset.clear();
    m_radPlagPreset.clear();
    m_radEndFastPreset.clear();
    m_radTzaisPreset.clear();
    m_radShaahZmanitPreset.clear();    // v0.8.83

    // v0.8.2 - WS_CLIPSIBLINGS so this main tab control does not paint over
    // its sibling sub-tab control inside the Zmanim page area.
    m_tab.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | TCS_TABS,
        CRect(6, 6, W - 6, H - 48), this, IDC_OPT_TAB);
    m_tab.SetFont(pFont);
    TCITEM ti = {};
    ti.mask = TCIF_TEXT;
    // v0.8.0 tab order: General, Month View, Interface, Tray Tooltip,
    // Zmanim Bar, Colors, Zmanim, Notifications.  "Zman Shitos" merged
    // into the Zmanim tab as sub-tabs.
    ti.pszText = const_cast<LPWSTR>(L"General");      m_tab.InsertItem(0, &ti);
    ti.pszText = const_cast<LPWSTR>(L"Month View");   m_tab.InsertItem(1, &ti);
    ti.pszText = const_cast<LPWSTR>(L"Interface");    m_tab.InsertItem(2, &ti);
    ti.pszText = const_cast<LPWSTR>(L"Tray Tooltip"); m_tab.InsertItem(3, &ti);
    ti.pszText = const_cast<LPWSTR>(L"Zmanim Bar");   m_tab.InsertItem(4, &ti);
    ti.pszText = const_cast<LPWSTR>(L"Colors");       m_tab.InsertItem(5, &ti);
    ti.pszText = const_cast<LPWSTR>(L"Zmanim");       m_tab.InsertItem(6, &ti);
    ti.pszText = const_cast<LPWSTR>(L"Notifications"); m_tab.InsertItem(7, &ti);
    ti.pszText = const_cast<LPWSTR>(L"Year Details");  m_tab.InsertItem(8, &ti);

    auto track = [&](std::vector<CWnd*>& page, CWnd* wnd) {
        page.push_back(wnd);
        wnd->SetFont(pFont);
    };
    int staticId = 8000;
    auto mkGroup = [&](std::vector<CWnd*>& page, const wchar_t* text, int x, int y, int w, int h) {
        CButton* b = new CButton;
        b->Create(text, WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            CRect(x, y, x + w, y + h), this, staticId++);
        track(page, b);
    };
    auto mkStatic = [&](std::vector<CWnd*>& page, const wchar_t* text, int x, int y, int w, int h) {
        CStatic* s = new CStatic;
        s->Create(text, WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
            CRect(x, y, x + w, y + h), this, staticId++);
        track(page, s);
    };
    auto initCombo = [&](std::vector<CWnd*>& page, CComboBox& combo, int x, int y, int w, UINT id,
        std::initializer_list<const wchar_t*> items, int sel)
    {
        combo.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
            CRect(x, y, x + w, y + 160), this, id);
        track(page, &combo);
        for (const wchar_t* item : items)
            combo.AddString(item);
        combo.SetCurSel(max(0, min((int)items.size() - 1, sel)));
    };
    auto setEdit = [&](std::vector<CWnd*>& page, CEdit& edit, int x, int y, int w, UINT id, double value) {
        CString s; s.Format(L"%.2f", value);
        edit.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
            CRect(x, y, x + w, y + 22), this, id);
        track(page, &edit);
        edit.SetWindowText(s);
    };

    // General tab
    int y = 38;
    mkGroup(m_pageGeneral, L"Display", 14, y, W - 28, 92); y += 20;
    m_radAmPm.Create(L"AM / PM", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        CRect(28, y, 110, y + 20), this, IDC_OPT_RAD_AMPM); track(m_pageGeneral, &m_radAmPm);
    m_rad24hr.Create(L"24 Hour", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(118, y, 205, y + 20), this, IDC_OPT_RAD_24HR); track(m_pageGeneral, &m_rad24hr);
    m_radCivilMonth.Create(L"Civil month", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        CRect(215, y, 310, y + 20), this, IDC_OPT_RAD_CIVIL); track(m_pageGeneral, &m_radCivilMonth);
    m_radHebrewMonth.Create(L"Hebrew month", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(315, y, W - 28, y + 20), this, IDC_OPT_RAD_HEBREW); track(m_pageGeneral, &m_radHebrewMonth);
    y += 26;
    m_chkDateTracking.Create(L"Date tracking (on mouse hover)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(28, y, 240, y + 20), this, IDC_OPT_TRACKING); track(m_pageGeneral, &m_chkDateTracking);
    mkStatic(m_pageGeneral, L"Font size", 250, y, 65, 20);
    initCombo(m_pageGeneral, m_cmbFontSize, 318, y - 1, 96, IDC_OPT_FONT_SIZE,
        { L"Tiny (13)", L"Normal (14)", L"Medium (15)", L"Large (16)",
          L"Larger (17)", L"Big (18)", L"Biggest (19)" },
        max(0, min(6, m_current.fontSize)));
    y = 138;
    mkGroup(m_pageGeneral, L"Reset", 14, y, W - 28, 78); y += 24;
    mkStatic(m_pageGeneral, L"Restore all calendar settings to their defaults.", 28, y, W - 56, 20);
    y += 26;
    m_btnResetCalendar.Create(L"Reset Calendar Settings...",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(28, y, 210, y + 26), this, IDC_OPT_RESET_CALENDAR);
    track(m_pageGeneral, &m_btnResetCalendar);

    // Auto-Update group
    y = 216;
    mkGroup(m_pageGeneral, L"Updates", 14, y, W - 28, 110); y += 22;
    m_chkDisableAutoUpdate.Create(L"Disable all update checks", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(28, y, 220, y + 20), this, IDC_OPT_DISABLE_AUTOUPDATE);
    track(m_pageGeneral, &m_chkDisableAutoUpdate); y += 26;
    m_chkCheckUpdatesAuto.Create(L"Check for updates automatically", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(28, y, 230, y + 20), this, IDC_OPT_CHECK_AUTO);
    track(m_pageGeneral, &m_chkCheckUpdatesAuto); y += 24;
    mkStatic(m_pageGeneral, L"Frequency:", 28, y, 60, 20);
    initCombo(m_pageGeneral, m_cmbUpdateFrequency, 90, y - 1, 120, IDC_OPT_UPDATE_FREQ,
        { L"Daily", L"Weekly", L"Monthly" }, m_current.updateCheckFrequency);
    y += 26;
    m_chkCheckOnLaunch.Create(L"Check on launch", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(28, y, 180, y + 20), this, IDC_OPT_CHECK_LAUNCH);
    track(m_pageGeneral, &m_chkCheckOnLaunch);
    m_btnCheckUpdatesNow.Create(L"Check Now",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(190, y - 2, 270, y + 22), this, IDC_OPT_CHECK_NOW);
    track(m_pageGeneral, &m_btnCheckUpdatesNow);

    y = 38;
    mkGroup(m_pageMonth, L"Month View", 14, y, W - 28, 118); y += 20;
    m_chkParshios.Create(L"Parshios", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(28, y, 150, y + 20), this, IDC_OPT_PARSHIOS); track(m_pageMonth, &m_chkParshios);
    m_chkDafYomi.Create(L"Daf yomi", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(240, y, 345, y + 20), this, IDC_OPT_DAF); track(m_pageMonth, &m_chkDafYomi); y += 24;
    m_chkMoadim.Create(L"Moadim", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(28, y, 150, y + 20), this, IDC_OPT_MOADIM); track(m_pageMonth, &m_chkMoadim);
    m_chkYerushalmi.Create(L"Yerushalmi yomi", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(240, y, 395, y + 20), this, IDC_OPT_YERUSHALMI); track(m_pageMonth, &m_chkYerushalmi); y += 24;
    m_chkUserEvents.Create(L"Personal events", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(28, y, 160, y + 20), this, IDC_OPT_USER_EVENTS); track(m_pageMonth, &m_chkUserEvents);
    m_chkHalacha.Create(L"Halacha yomit", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(240, y, 395, y + 20), this, IDC_OPT_HALACHA); track(m_pageMonth, &m_chkHalacha); y += 24;
    m_chkMishna.Create(L"Mishna yomit", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(240, y, 395, y + 20), this, IDC_OPT_MISHNA); track(m_pageMonth, &m_chkMishna); y += 24;
    m_chkTanach.Create(L"Tanach yomi", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(240, y, 395, y + 20), this, IDC_OPT_TANACH); track(m_pageMonth, &m_chkTanach);

    // --- v0.8.78: Fast Days group (moved from Zmanim tab + new BeHaB options) ---
    y = 164;
    mkGroup(m_pageMonth, L"Fast Days", 14, y, W - 28, 96); y += 22;

    // Chatzos on fast days (moved from Zmanim tab)
    m_chkChatzosOnFasts.Create(L"Show chatzos on fast-day cells",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        CRect(28, y, W - 28, y + 20), this, IDC_OPT_CHATZOS_FASTS);
    track(m_pageMonth, &m_chkChatzosOnFasts);
    y += 26;

    // BeHaB (Mon-Thu-Mon of Cheshvan / Iyar)
    m_chkShowBeHaB.Create(L"Show BeHaB days  (Mon-Thu-Mon after Sukkot & Pesach)",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        CRect(28, y, W - 28, y + 20), this, IDC_OPT_SHOW_BEHAB);
    track(m_pageMonth, &m_chkShowBeHaB);
    y += 26;

    // Chatzos on BeHaB — grayed out when BeHaB is off
    m_chkChatzosOnBeHaB.Create(L"  └ Show chatzos on BeHaB cells",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        CRect(28, y, W - 28, y + 20), this, IDC_OPT_CHATZOS_BEHAB);
    track(m_pageMonth, &m_chkChatzosOnBeHaB);

    // v0.8.82 — moved Holiday Schedule below the Updates group (y=330)
    // to fix overlap with Reset group (both were at y=138 previously).
    y = 330;

    mkGroup(m_pageGeneral, L"Holiday Schedule", 14, y, W - 28, 54); y += 22;
    m_radDiaspora.Create(L"Diaspora (outside Israel)", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        CRect(28, y, 220, y + 20), this, IDC_OPT_RAD_DIASPORA); track(m_pageGeneral, &m_radDiaspora);
    m_radIsrael.Create(L"Israel", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(240, y, 330, y + 20), this, IDC_OPT_RAD_ISRAEL); track(m_pageGeneral, &m_radIsrael);
    y = 38;

    mkGroup(m_pageInterface, L"Interface", 14, y, W - 28, 280); y += 20;

    // ── General (non-tray) options ────────────────────────────────────────────
    m_btnManageCals.Create(L"Manage Calendars...", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(28, y, 200, y + 24), this, IDC_OPT_MANAGE_CALS); track(m_pageInterface, &m_btnManageCals);
    m_chkStartWithWindows.Create(L"start with Windows", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(215, y + 4, 360, y + 24), this, IDC_OPT_START_WINDOWS); track(m_pageInterface, &m_chkStartWithWindows);
    y += 30;
    m_chkDesktopShortcut.Create(L"desktop shortcut", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(28, y, 180, y + 20), this, IDC_OPT_DESKTOP); track(m_pageInterface, &m_chkDesktopShortcut);
    y += 30;

    // ── System Tray ───────────────────────────────────────────────────────────
    mkGroup(m_pageInterface, L"System Tray", 20, y, W - 40, 192); y += 20;
    m_chkShowTrayIcon.Create(L"Pin WinLuach to system tray", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(28, y, 230, y + 20), this, IDC_OPT_SHOW_TRAY); track(m_pageInterface, &m_chkShowTrayIcon); y += 24;
    m_chkMinimizeToTray.Create(L"Minimize to system tray", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(28, y, 185, y + 20), this, IDC_OPT_MIN_TRAY); track(m_pageInterface, &m_chkMinimizeToTray);
    initCombo(m_pageInterface, m_cmbTrayWhen, 190, y - 1, 160, IDC_OPT_TRAY_WHEN,
        { L"when minimized", L"when closed", L"when minimized or closed" },
        max(0, min(2, m_current.minimizeTrayWhen)));
    m_chkMinimizeOnStartup.Create(L"minimize on startup", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(28, y + 24, 190, y + 44), this, IDC_OPT_MIN_STARTUP); track(m_pageInterface, &m_chkMinimizeOnStartup); y += 48;
    mkStatic(m_pageInterface, L"Tray text", 28, y, 75, 22);
    m_btnTrayTextColor.Create(L"Color...", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(105, y, 180, y + 24), this, IDC_OPT_TRAY_COLOR); track(m_pageInterface, &m_btnTrayTextColor);
    m_btnTrayFont.Create(L"Font...", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(188, y, 262, y + 24), this, IDC_OPT_TRAY_FONT); track(m_pageInterface, &m_btnTrayFont);
    y += 30;
    m_chkTrayBackEnabled.Create(L"tray background", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        CRect(28, y, 152, y + 20), this, IDC_OPT_TRAY_BACK_ENABLED); track(m_pageInterface, &m_chkTrayBackEnabled);
    m_btnTrayBackColor.Create(L"Background...", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(160, y - 2, 270, y + 22), this, IDC_OPT_TRAY_BACK_COLOR); track(m_pageInterface, &m_btnTrayBackColor);
    mkStatic(m_pageInterface, L"Tray date:", 282, y, 70, 22);
    initCombo(m_pageInterface, m_cmbTrayNumber, 350, y - 1, 66, IDC_OPT_TRAY_NUMBER,
        { L"Heb", L"Eng" }, max(0, min(1, m_current.trayNumberStyle)));
    y += 32;
    m_btnTrayDefaults.Create(L"Restore Tray Defaults", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(28, y, 185, y + 24), this, IDC_OPT_TRAY_DEFAULTS); track(m_pageInterface, &m_btnTrayDefaults);

    // Tray Tooltip tab
    y = 38;
    mkGroup(m_pageTrayTooltip, L"Items Shown In Today's Tray Tooltip", 14, y, W - 28, 358); y += 22;
    // v0.8.82 — instruction label + max-5 warning
    m_statTrayTipInstruction.Create(L"Select up to 5 zmanim to display in the tray tooltip:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        CRect(28, y, W - 28, y + 16), this, IDC_STATIC);
    m_statTrayTipInstruction.SetFont(pFont);
    track(m_pageTrayTooltip, &m_statTrayTipInstruction);
    y += 18;
    m_statTrayTipMaxMsg.Create(L"  ⚠ Maximum of 5 zmanim selected.  Uncheck one to select another.",
        WS_CHILD | SS_LEFT,          // hidden initially — shown when max reached
        CRect(28, y, W - 28, y + 16), this, IDC_OPT_TRAY_TIP_LIMIT_MSG);
    m_statTrayTipMaxMsg.SetFont(pFont);
    track(m_pageTrayTooltip, &m_statTrayTipMaxMsg);
    y += 20;
    for (int i = 0; i < kTrayTooltipZmanCount; ++i)
    {
        int col = i / 16;
        int row = i % 16;
        int x0 = 28 + col * 196;
        int yy = y + row * 19;
        m_chkTrayTooltipZmanim[i].Create(kTrayTooltipZmanLabels[i],
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            CRect(x0, yy, x0 + 190, yy + 18), this, IDC_OPT_TRAY_TIP_ZMAN_FIRST + i);
        track(m_pageTrayTooltip, &m_chkTrayTooltipZmanim[i]);
        m_chkTrayTooltipZmanim[i].SetCheck((m_current.trayTooltipZmanimMask & (1u << i)) ? BST_CHECKED : BST_UNCHECKED);
    }
    // Custom (user-configured shita) zmanim for tray tooltip
    y = 382;
    mkGroup(m_pageTrayTooltip, L"Custom Zmanim (your configured shita)", 14, y, W - 28, 112); y += 22;
    for (int i = 0; i < kTrayTooltipCustomZmanCount; ++i)
    {
        int col = i / 4;
        int row = i % 4;
        int x0 = 28 + col * 196;
        int yy = y + row * 20;
        m_chkTrayTooltipCustomZmanim[i].Create(kTrayTooltipCustomZmanLabels[i],
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            CRect(x0, yy, x0 + 190, yy + 18), this, IDC_OPT_TRAY_TIP_CUSTOM_ZMAN_FIRST + i);
        track(m_pageTrayTooltip, &m_chkTrayTooltipCustomZmanim[i]);
        m_chkTrayTooltipCustomZmanim[i].SetCheck((m_current.trayTooltipCustomZmanimMask & (1u << i)) ? BST_CHECKED : BST_UNCHECKED);
    }

    // Year Details tab
    y = 38;
    mkGroup(m_pageYearDetails, L"Which Year Facts to Show in the Sidebar", 14, y, W - 28, 188); y += 24;
    for (int i = 0; i < kYearDetailCount; ++i)
    {
        int col = i / 6;
        int row = i % 6;
        int x0 = 28 + col * ((W - 56) / 2);
        int yy = y + row * 22;
        m_chkYearDetails[i].Create(kYearDetailLabels[i],
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            CRect(x0, yy, x0 + (W - 56) / 2 - 4, yy + 20), this, IDC_OPT_YEAR_DETAIL_FIRST + i);
        track(m_pageYearDetails, &m_chkYearDetails[i]);
        m_chkYearDetails[i].SetCheck((m_current.yearDetailsMask & (1u << i)) ? BST_CHECKED : BST_UNCHECKED);
    }

    // Colors tab
    mkGroup(m_pageColors, L"Calendar Cell and Text Colors", 14, 38, W - 28, 304);
    mkStatic(m_pageColors, L"Text", 28, 63, 55, 20);
    m_cmbColorItem.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        CRect(86, 60, 256, 220), this, IDC_OPT_COLOR_ITEM);
    track(m_pageColors, &m_cmbColorItem);
    for (const auto& pref : kCalendarColorPrefs)
        m_cmbColorItem.AddString(pref.label);
    m_cmbColorItem.SetCurSel(0);

    mkStatic(m_pageColors, L"Hex", 270, 63, 38, 20);
    m_editColorHex.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
        CRect(310, 60, 388, 82), this, IDC_OPT_COLOR_HEX);
    track(m_pageColors, &m_editColorHex);

    m_btnColorPicker.Create(L"Choose...", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(28, 92, 110, 117), this, IDC_OPT_COLOR_PICKER);
    track(m_pageColors, &m_btnColorPicker);

    mkStatic(m_pageColors, L"Sample day", 126, 95, 82, 20);
    m_cmbColorPreview.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        CRect(210, 92, 388, 252), this, IDC_OPT_COLOR_PREVIEW);
    track(m_pageColors, &m_cmbColorPreview);
    for (const wchar_t* name : kColorPreviewNames)
        m_cmbColorPreview.AddString(name);
    m_cmbColorPreview.SetCurSel(0);

    CString previewClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW,
        ::LoadCursor(nullptr, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), nullptr);
    m_colorPreview.Create(previewClass, L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
        CRect(28, 130, W - 28, 302), this, IDC_OPT_COLOR_SAMPLE);
    track(m_pageColors, &m_colorPreview);

    m_btnRestoreColors.Create(L"Restore Default Colors", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(28, 318, 205, 344), this, IDC_OPT_COLOR_RESTORE);
    track(m_pageColors, &m_btnRestoreColors);

    // Zmanim tab
    y = 38;
    mkGroup(m_pageZmanim, L"Zmanim Display", 14, y, W - 28, 306); y += 24;
    mkStatic(m_pageZmanim, L"Candle lighting", 28, y, 105, 22);
    initCombo(m_pageZmanim, m_cmbCandleMinutes, 138, y - 1, 70, IDC_OPT_CANDLE,
        { L"15", L"18", L"21", L"22", L"30", L"40" }, 1);
    // m_chkChatzosOnFasts moved to Month View tab (v0.8.78)
    y += 34;
    mkStatic(m_pageZmanim, L"Mincha and shaah zmanis shita:", 28, y, 300, 20); y += 24;
    m_radGRA.Create(L"GRA / Baal HaTanya", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        CRect(40, y, 190, y + 20), this, IDC_OPT_RAD_GRA); track(m_pageZmanim, &m_radGRA);
    m_radMA72.Create(L"Magen Avraham (72 min)", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(205, y, 385, y + 20), this, IDC_OPT_RAD_MA72); track(m_pageZmanim, &m_radMA72); y += 24;
    m_radMA90.Create(L"Magen Avraham (90 min)", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(40, y, 220, y + 20), this, IDC_OPT_RAD_MA90); track(m_pageZmanim, &m_radMA90); y += 34;
    mkStatic(m_pageZmanim, L"Preset alos:", 28, y, 95, 22);
    initCombo(m_pageZmanim, m_cmbAlotShita, 125, y - 1, 210, IDC_OPT_ALOT_SHITA,
        { L"GRA (16.1 degrees)", L"Magen Avraham (72 min)", L"Magen Avraham (90 min)" },
        max(0, min(2, m_current.alotShita)));
    y += 30;
    mkStatic(m_pageZmanim, L"Preset tzeis:", 28, y, 95, 22);
    initCombo(m_pageZmanim, m_cmbTzeitShita, 125, y - 1, 250, IDC_OPT_TZEIT_SHITA,
        { L"GRA (8.5 degrees)", L"Magen Avraham (72 min)", L"Magen Avraham (90 min)",
          L"Magen Avraham (72 min prop.)", L"Magen Avraham (90 min prop.)",
          L"42 minutes", L"50 minutes", L"60 minutes" },
        max(0, min(7, m_current.tzeitShita)));
    y += 40;
    mkStatic(m_pageZmanim, L"Displayed alos value:", 28, y, 140, 22);
    setEdit(m_pageZmanim, m_editCustomAlot, 160, y, 60, IDC_OPT_ALOT_VALUE, m_current.customAlotValue);
    m_radAlotDegrees.Create(L"degrees", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        CRect(230, y, 305, y + 20), this, IDC_OPT_ALOT_DEG); track(m_pageZmanim, &m_radAlotDegrees);
    m_radAlotMinutes.Create(L"minutes", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(310, y, 385, y + 20), this, IDC_OPT_ALOT_MIN); track(m_pageZmanim, &m_radAlotMinutes);
    m_radAlotZmanis.Create(L"shaah zmanis min", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(230, y + 22, W - 28, y + 42), this, IDC_OPT_ALOT_ZMANIS); track(m_pageZmanim, &m_radAlotZmanis);
    y += 48;
    mkStatic(m_pageZmanim, L"Displayed tzeis value:", 28, y, 140, 22);
    setEdit(m_pageZmanim, m_editCustomTzeit, 160, y, 60, IDC_OPT_TZEIT_VALUE, m_current.customTzeitValue);
    m_radTzeitDegrees.Create(L"degrees", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        CRect(230, y, 305, y + 20), this, IDC_OPT_TZEIT_DEG); track(m_pageZmanim, &m_radTzeitDegrees);
    m_radTzeitMinutes.Create(L"minutes", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(310, y, 385, y + 20), this, IDC_OPT_TZEIT_MIN); track(m_pageZmanim, &m_radTzeitMinutes);
    m_radTzeitZmanis.Create(L"shaah zmanis min", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(230, y + 22, W - 28, y + 42), this, IDC_OPT_TZEIT_ZMANIS); track(m_pageZmanim, &m_radTzeitZmanis);

    // Zman Shitos tab
    y = 38;
    mkGroup(m_pageZmanShitos, L"Sof Zman Shema and Tefilla", 14, y, W - 28, 118); y += 24;
    mkStatic(m_pageZmanShitos, L"Preset:", 28, y, 68, 22);
    initCombo(m_pageZmanShitos, m_cmbSofZmanShita, 100, y - 1, 238, IDC_OPT_SOFZMAN_SHITA,
        { L"GRA (netz to shkiah)", L"90 minutes as degrees", L"Fixed 72 minutes",
          L"72 minutes as 16.1 degrees", L"Custom" },
        0);
    y += 32;
    mkStatic(m_pageZmanShitos, L"Value:", 28, y, 68, 22);
    setEdit(m_pageZmanShitos, m_editCustomSofZman, 100, y, 60, IDC_OPT_SOFZMAN_VALUE, m_current.customSofZmanValue);
    m_radSofZmanDegrees.Create(L"degrees", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        CRect(175, y, 250, y + 20), this, IDC_OPT_SOFZMAN_DEG); track(m_pageZmanShitos, &m_radSofZmanDegrees);
    m_radSofZmanMinutes.Create(L"minutes", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(255, y, 330, y + 20), this, IDC_OPT_SOFZMAN_MIN); track(m_pageZmanShitos, &m_radSofZmanMinutes);
    m_radSofZmanZmanis.Create(L"shaah zmanis min", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(175, y + 24, W - 28, y + 44), this, IDC_OPT_SOFZMAN_ZMANIS); track(m_pageZmanShitos, &m_radSofZmanZmanis);

    y = 172;
    mkGroup(m_pageZmanShitos, L"Misheyakir", 14, y, W - 28, 118); y += 24;
    mkStatic(m_pageZmanShitos, L"Preset:", 28, y, 68, 22);
    initCombo(m_pageZmanShitos, m_cmbMisheyakirShita, 100, y - 1, 238, IDC_OPT_MISHEYAKIR_SHITA,
        { L"10.2 degrees", L"11.5 degrees", L"Fixed 60 minutes", L"Fixed 72 minutes", L"Custom" },
        0);
    y += 32;
    mkStatic(m_pageZmanShitos, L"Value:", 28, y, 68, 22);
    setEdit(m_pageZmanShitos, m_editCustomMisheyakir, 100, y, 60, IDC_OPT_MISHEYAKIR_VALUE, m_current.customMisheyakirValue);
    m_radMisheyakirDegrees.Create(L"degrees", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        CRect(175, y, 250, y + 20), this, IDC_OPT_MISHEYAKIR_DEG); track(m_pageZmanShitos, &m_radMisheyakirDegrees);
    m_radMisheyakirMinutes.Create(L"minutes", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(255, y, 330, y + 20), this, IDC_OPT_MISHEYAKIR_MIN); track(m_pageZmanShitos, &m_radMisheyakirMinutes);
    m_radMisheyakirZmanis.Create(L"shaah zmanis min", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(175, y + 24, W - 28, y + 44), this, IDC_OPT_MISHEYAKIR_ZMANIS); track(m_pageZmanShitos, &m_radMisheyakirZmanis);

    // Notifications tab — 3 sub-tabs: General | Zmanim | Reminders
    {
        int notifyPageTop = 64;
        int notifyPageBottom = H - 56;
        m_notifySubTab.Create(
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | TCS_TABS,
            CRect(20, 38, W - 20, notifyPageBottom), this, IDC_OPT_NOTIFY_SUBTAB);
        m_notifySubTab.SetFont(pFont);
        {
            TCITEM sti = {};
            sti.mask = TCIF_TEXT;
            const wchar_t* subNames[] = { L"General", L"Zmanim", L"Reminders" };
            for (int i = 0; i < 3; ++i) {
                sti.pszText = const_cast<LPWSTR>(subNames[i]);
                m_notifySubTab.InsertItem(i, &sti);
            }
        }

        // ── General sub-page ──────────────────────────────────────────────
        y = notifyPageTop;
        mkGroup(m_pageNotifyGeneral, L"Toast notification type", 28, y, W - 56, 76); y += 22;
        m_radUseWindowsNotify.Create(L"Windows built-in notifications",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON | WS_GROUP,
            CRect(42, y, 242, y + 20), this, IDC_OPT_RAD_WINNOTIFY);
        track(m_pageNotifyGeneral, &m_radUseWindowsNotify);
        m_radUseWinLuachToast.Create(L"WinLuach Toast",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
            CRect(248, y, 400, y + 20), this, IDC_OPT_RAD_WINLUACHTOAST);
        track(m_pageNotifyGeneral, &m_radUseWinLuachToast);
        bool isWLToast = m_current.useWinLuachToast;
        m_radUseWindowsNotify.SetCheck(!isWLToast ? BST_CHECKED : BST_UNCHECKED);
        m_radUseWinLuachToast.SetCheck( isWLToast ? BST_CHECKED : BST_UNCHECKED);
        y += 26;
        mkStatic(m_pageNotifyGeneral, L"Display for:", 42, y, 80, 22);
        m_editToastDuration.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_NUMBER,
            CRect(124, y, 166, y + 22), this, IDC_OPT_TOAST_DURATION);
        track(m_pageNotifyGeneral, &m_editToastDuration);
        { CString dur; dur.Format(L"%d", max(1, m_current.winLuachToastDuration)); m_editToastDuration.SetWindowText(dur); }
        initCombo(m_pageNotifyGeneral, m_cmbToastDurationUnit, 172, y - 1, 100, IDC_OPT_TOAST_UNIT,
            { L"minutes", L"hours", L"days", L"weeks", L"months" },
            max(0, min(4, m_current.winLuachToastDurationUnit)));
        m_editToastDuration.EnableWindow(isWLToast ? TRUE : FALSE);
        m_cmbToastDurationUnit.EnableWindow(isWLToast ? TRUE : FALSE);
        y += 30;

        mkGroup(m_pageNotifyGeneral, L"Notification Style", 28, y, W - 56, 212); y += 22;
        mkStatic(m_pageNotifyGeneral, L"Personal events:", 42, y, 112, 22);
        initCombo(m_pageNotifyGeneral, m_cmbNotifyPersonal, 158, y - 1, 126, IDC_OPT_NOTIFY_PERSONAL,
            { L"Off", L"Toast", L"Popup", L"Toast + Popup" },
            max(0, min(3, m_current.notifyPersonalEvents)));
        m_btnPreviewNotify.Create(L"Test", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            CRect(300, y, 352, y + 24), this, IDC_OPT_PREVIEW_NOTIFY);
        track(m_pageNotifyGeneral, &m_btnPreviewNotify);
        y += 28;
        mkStatic(m_pageNotifyGeneral, L"Web calendars:", 42, y, 112, 22);
        initCombo(m_pageNotifyGeneral, m_cmbNotifyWebCal, 158, y - 1, 126, IDC_OPT_NOTIFY_WEBCAL,
            { L"Off", L"Toast", L"Popup", L"Toast + Popup" },
            max(0, min(3, m_current.notifyWebCalEvents)));
        y += 28;
        // Sefiras HaOmer — redesigned layout: each mode is self-contained on its own row
        mkStatic(m_pageNotifyGeneral, L"Sefiras HaOmer:", 42, y, 112, 22);
        initCombo(m_pageNotifyGeneral, m_cmbNotifySefirah, 158, y - 1, 126, IDC_OPT_NOTIFY_SEFIRAH_STYLE,
            { L"Off", L"Toast", L"Popup", L"Toast + Popup" },
            max(0, min(3, m_current.notifySefirahStyle)));
        y += 28;
        bool isManualOmer = (m_current.notifySefirahMode == 0);
        bool isOtherOmer  = (m_current.notifySefirahMode == 1 && m_current.notifySefirahBase == 2);
        // Row 1: relative to sunset / tzais
        m_radNotifySefirahSunTzais.Create(L"relative to:", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON | WS_GROUP,
            CRect(42, y, 138, y + 20), this, IDC_OPT_NOTIFY_SEFIRAH_MODE);
        track(m_pageNotifyGeneral, &m_radNotifySefirahSunTzais);
        m_radNotifySefirahSunTzais.SetCheck(!isManualOmer && !isOtherOmer ? BST_CHECKED : BST_UNCHECKED);
        initCombo(m_pageNotifyGeneral, m_cmbNotifySefirahBase, 142, y - 1, 82, IDC_OPT_NOTIFY_SEFIRAH_BASE,
            { L"sunset", L"tzais" }, max(0, min(1, m_current.notifySefirahBase)));
        y += 26;
        // Row 2: other zman
        m_radNotifySefirahOther.Create(L"other zman:", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
            CRect(42, y, 138, y + 20), this, IDC_OPT_NOTIFY_SEFIRAH_MODE + 1);
        track(m_pageNotifyGeneral, &m_radNotifySefirahOther);
        m_radNotifySefirahOther.SetCheck(isOtherOmer ? BST_CHECKED : BST_UNCHECKED);
        m_cmbNotifySefirahOtherZman.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
            CRect(142, y - 1, W - 42, y + 160), this, IDC_OPT_NOTIFY_SEFIRAH_OTHER);
        track(m_pageNotifyGeneral, &m_cmbNotifySefirahOtherZman);
        for (const wchar_t* label : kZmanCheckboxLabels)
            m_cmbNotifySefirahOtherZman.AddString(label);
        m_cmbNotifySefirahOtherZman.SetCurSel(max(0, min(kZmanCheckboxCount - 1, m_current.notifySefirahOtherZman)));
        y += 26;
        // Row 3: manual time
        m_radNotifySefirahManual.Create(L"manual time:", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
            CRect(42, y, 138, y + 20), this, IDC_OPT_NOTIFY_SEFIRAH_MODE + 2);
        track(m_pageNotifyGeneral, &m_radNotifySefirahManual);
        m_radNotifySefirahManual.SetCheck(isManualOmer ? BST_CHECKED : BST_UNCHECKED);
        mkStatic(m_pageNotifyGeneral, L"at:", 144, y, 24, 22);
        m_cmbNotifySefirahHour.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            CRect(172, y - 1, 212, y + 140), this, IDC_OPT_NOTIFY_SEFIRAH_HOUR);
        track(m_pageNotifyGeneral, &m_cmbNotifySefirahHour);
        m_cmbNotifySefirahMinute.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            CRect(216, y - 1, 256, y + 140), this, IDC_OPT_NOTIFY_SEFIRAH_MIN);
        track(m_pageNotifyGeneral, &m_cmbNotifySefirahMinute);
        m_cmbNotifySefirahAmPm.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            CRect(260, y - 1, 304, y + 90), this, IDC_OPT_NOTIFY_SEFIRAH_AMPM);
        track(m_pageNotifyGeneral, &m_cmbNotifySefirahAmPm);
        FillClockCombos(m_cmbNotifySefirahHour, m_cmbNotifySefirahMinute, m_cmbNotifySefirahAmPm);
        SetClockCombos(m_cmbNotifySefirahHour, m_cmbNotifySefirahMinute, m_cmbNotifySefirahAmPm,
            m_current.notifySefirahTime, 21, 0);
        y += 26;
        // Row 4: offset
        mkStatic(m_pageNotifyGeneral, L"offset:", 42, y, 48, 22);
        m_editNotifySefirahOffset.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_NUMBER,
            CRect(92, y, 132, y + 22), this, IDC_OPT_NOTIFY_SEFIRAH_OFFSET);
        track(m_pageNotifyGeneral, &m_editNotifySefirahOffset);
        {
            CString off;
            off.Format(L"%d", max(0, m_current.notifySefirahOffsetMinutes));
            m_editNotifySefirahOffset.SetWindowText(off);
        }
        mkStatic(m_pageNotifyGeneral, L"minutes", 136, y, 55, 22);
        initCombo(m_pageNotifyGeneral, m_cmbNotifySefirahDir, 194, y - 1, 72, IDC_OPT_NOTIFY_SEFIRAH_DIR,
            { L"before", L"after" }, max(0, min(1, m_current.notifySefirahOffsetDir)));
        y += 30;

        // ── Zmanim sub-page ───────────────────────────────────────────────
        y = notifyPageTop;
        mkGroup(m_pageNotifyZmanim, L"Zmanim Notifications", 28, y, W - 56, 168); y += 22;
        mkStatic(m_pageNotifyZmanim, L"Zmanim:", 42, y, 70, 22);
        initCombo(m_pageNotifyZmanim, m_cmbNotifyZmanim, 114, y - 1, 126, IDC_OPT_NOTIFY_ZMAN_STYLE,
            { L"Off", L"Toast", L"Popup", L"Toast + Popup" },
            max(0, min(3, m_current.notifyZmanimStyle)));
        y += 28;
        for (int i = 0; i < kZmanCheckboxCount; ++i) {
            int col = i / 9;
            int row = i % 9;
            int x0 = 42 + col * 190;
            int yy = y + row * 20;
            m_chkNotifyZmanim[i].Create(kZmanCheckboxLabels[i],
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                CRect(x0, yy, x0 + 170, yy + 18), this, IDC_OPT_NOTIFY_ZMAN_FIRST + i);
            track(m_pageNotifyZmanim, &m_chkNotifyZmanim[i]);
            m_chkNotifyZmanim[i].SetCheck((m_current.notifyZmanimMask & (1u << i)) ? BST_CHECKED : BST_UNCHECKED);
        }
        y += 188;

        // ── Reminders sub-page ────────────────────────────────────────────
        y = notifyPageTop;
        mkGroup(m_pageNotifyReminders, L"Advance Reminders", 28, y, W - 56, 112); y += 22;
        mkStatic(m_pageNotifyReminders, L"Moadim:", 42, y, 70, 22);
        initCombo(m_pageNotifyReminders, m_cmbNotifyMoadim, 114, y - 1, 126, IDC_OPT_NOTIFY_MOADIM_STYLE,
            { L"Off", L"Toast", L"Popup", L"Toast + Popup" },
            max(0, min(3, m_current.notifyMoadimStyle)));
        mkStatic(m_pageNotifyReminders, L"before:", 252, y, 50, 22);
        m_editNotifyMoadimAmount.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_NUMBER,
            CRect(304, y, 346, y + 22), this, IDC_OPT_NOTIFY_MOADIM_OFFSETS);
        track(m_pageNotifyReminders, &m_editNotifyMoadimAmount);
        initCombo(m_pageNotifyReminders, m_cmbNotifyMoadimUnit, 352, y - 1, 84, IDC_OPT_NOTIFY_MOADIM_UNIT,
            { L"minutes", L"hours", L"days", L"weeks", L"months", L"years" }, 2);
        SetOffsetControls(m_current.notifyMoadimOffsets, m_editNotifyMoadimAmount, m_cmbNotifyMoadimUnit);
        y += 28;
        mkStatic(m_pageNotifyReminders, L"Parsha:", 42, y, 70, 22);
        initCombo(m_pageNotifyReminders, m_cmbNotifyParsha, 114, y - 1, 126, IDC_OPT_NOTIFY_PARSHA_NAME,
            { L"Any", L"Bereshis", L"Noach", L"Lech Lecha", L"Vayera", L"Chayei Sarah",
              L"Toldos", L"Vayetzei", L"Vayishlach", L"Vayeshev", L"Miketz", L"Vayigash",
              L"Vayechi", L"Shemos", L"Vaera", L"Bo", L"Beshalach", L"Yisro", L"Mishpatim",
              L"Terumah", L"Tetzaveh", L"Ki Sisa", L"Vayakhel", L"Pekudei", L"Vayikra",
              L"Tzav", L"Shemini", L"Tazria", L"Metzora", L"Acharei Mos", L"Kedoshim",
              L"Emor", L"Behar", L"Bechukosai", L"Bamidbar", L"Naso", L"Behaaloscha",
              L"Shelach", L"Korach", L"Chukas", L"Balak", L"Pinchas", L"Matos", L"Masei",
              L"Devarim", L"Vaeschanan", L"Eikev", L"Reeh", L"Shoftim", L"Ki Seitzei",
              L"Ki Savo", L"Nitzavim", L"Vayelech", L"Haazinu", L"Vezos Haberacha" },
            0);
        {
            int parSel = 0;
            for (int i = 0; i < m_cmbNotifyParsha.GetCount(); ++i) {
                CString val; m_cmbNotifyParsha.GetLBText(i, val);
                if (std::wstring((LPCWSTR)val) == m_current.notifyParshaName) { parSel = i; break; }
            }
            m_cmbNotifyParsha.SetCurSel(parSel);
        }
        initCombo(m_pageNotifyReminders, m_cmbNotifyParshaStyle, 252, y - 1, 126, IDC_OPT_NOTIFY_PARSHA_STYLE,
            { L"Off", L"Toast", L"Popup", L"Toast + Popup" },
            max(0, min(3, m_current.notifyParshaStyle)));
        y += 28;
        mkStatic(m_pageNotifyReminders, L"Parsha before:", 42, y, 100, 22);
        m_editNotifyParshaAmount.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_NUMBER,
            CRect(144, y, 184, y + 22), this, IDC_OPT_NOTIFY_PARSHA_OFFSETS);
        track(m_pageNotifyReminders, &m_editNotifyParshaAmount);
        initCombo(m_pageNotifyReminders, m_cmbNotifyParshaUnit, 190, y - 1, 56, IDC_OPT_NOTIFY_PARSHA_UNIT,
            { L"minutes", L"hours", L"days", L"weeks", L"months" }, 3);
        SetOffsetControls(m_current.notifyParshaOffsets, m_editNotifyParshaAmount, m_cmbNotifyParshaUnit);
        mkStatic(m_pageNotifyReminders, L"Personal before:", 250, y, 96, 22);
        m_editNotifyPersonalAmount.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_NUMBER,
            CRect(342, y, 376, y + 22), this, IDC_OPT_NOTIFY_PERSONAL_OFFSETS);
        track(m_pageNotifyReminders, &m_editNotifyPersonalAmount);
        initCombo(m_pageNotifyReminders, m_cmbNotifyPersonalUnit, 380, y - 1, 60, IDC_OPT_NOTIFY_PERSONAL_UNIT,
            { L"minutes", L"hours", L"days", L"weeks", L"months", L"years" }, 0);
        SetOffsetControls(m_current.notifyPersonalOffsets, m_editNotifyPersonalAmount, m_cmbNotifyPersonalUnit);
        y += 32;
        m_btnAdvancedReminders.Create(L"Manage Advanced Reminders...",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            CRect(42, y, 254, y + 26), this, IDC_OPT_ADV_REMINDERS);
        track(m_pageNotifyReminders, &m_btnAdvancedReminders);
        y += 34;
        mkStatic(m_pageNotifyReminders, L"Daily reminder time:", 42, y + 3, 140, 22);
        m_cmbReminderDailyHour.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            CRect(184, y, 224, y + 140), this, IDC_OPT_REMIND_DAILY_HOUR);
        m_cmbReminderDailyMin.Create( WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            CRect(230, y, 270, y + 140), this, IDC_OPT_REMIND_DAILY_MIN);
        m_cmbReminderDailyAmpm.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            CRect(276, y, 316, y + 140), this, IDC_OPT_REMIND_DAILY_AMPM);
        FillClockCombos(m_cmbReminderDailyHour, m_cmbReminderDailyMin, m_cmbReminderDailyAmpm);
        {
            int h = m_current.reminderDailyHour;
            int m = m_current.reminderDailyMinute;
            bool pm = (h >= 12);
            wchar_t buf[16];
            _snwprintf_s(buf, _TRUNCATE, L"%d:%02d %s", (h % 12 == 0 ? 12 : h % 12), m, pm ? L"PM" : L"AM");
            SetClockCombos(m_cmbReminderDailyHour, m_cmbReminderDailyMin, m_cmbReminderDailyAmpm, buf, h, m);
        }
        track(m_pageNotifyReminders, &m_cmbReminderDailyHour);
        track(m_pageNotifyReminders, &m_cmbReminderDailyMin);
        track(m_pageNotifyReminders, &m_cmbReminderDailyAmpm);
    }

    // =========================================================================
    // v0.8.0 - Zmanim Bar tab: one checkbox per kZmanimBarLabels entry,
    // backed by the bits in zmanimBarMask.
    // =========================================================================
    y = 38;
    mkGroup(m_pageZmanimBar, L"Show in bottom Zmanim bar", 14, y, W - 28, 360); y += 24;
    // v0.8.82 — display order puts custom Sof Shema (bit 6) before GRA (bit 5),
    // and custom Sof Tefilla (bit 10) before GRA (bit 9), for chronological order.
    static const int kZBarDisplayOrder[] = {
        0, 1, 2,   // Alos, Misheyakir, Hanetz
        3, 4, 6, 5, // Shma MA72, MA90, custom, GRA
        7, 8, 10, 9, // Tefilla MA72, MA90, custom, GRA
        11, 12, 13, 14, 15, 16, 17, 18, 19  // Chatzos, Mincha Gedola/Ketana, Plag, Shkiah, Tzeis, Candles, Sha'a, End of Fast
    };
    m_zmanimBarChecks.resize(kZmanimBarLabelCount, nullptr);
    for (int idx = 0; idx < kZmanimBarLabelCount; ++idx)
    {
        int i   = kZBarDisplayOrder[idx];
        int col = idx / 10;
        int row = idx % 10;
        int x0 = 28 + col * 220;
        int yy = y + row * 22;
        CButton* chk = new CButton;
        chk->Create(kZmanimBarLabels[i],
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            CRect(x0, yy, x0 + 210, yy + 20), this,
            IDC_OPT_ZBAR_FIRST + i);
        track(m_pageZmanimBar, chk);
        chk->SetCheck((m_current.zmanimBarMask & (1u << i)) ? BST_CHECKED : BST_UNCHECKED);
        m_zmanimBarChecks[i] = chk;
    }

    // =========================================================================
    // v0.8.0 - Zmanim tab sub-tab control + 9 sub-pages.  Each sub-page is a
    // preset-radio list plus an optional custom value edit.  The original
    // shaah-zmanis-aware sub-pages (Alos, Misheyakir, Sof MA, Sof GRA, Tzais)
    // reuse the existing CButton/CEdit members from the legacy Zmanim tab.
    // =========================================================================
    // v0.8.83 — TCS_MULTILINE: 10 sub-tabs wrap to 2 rows instead of showing
    // scroll arrows. zmanPageTop raised to 90 to clear the 2-row tab strip.
    int zmanPageTop    = 90;
    int zmanPageBottom = H - 56;
    m_zmanimSubTab.Create(
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | TCS_TABS | TCS_MULTILINE,
        CRect(20, 38, W - 20, zmanPageBottom), this, IDC_OPT_ZSUBTAB);
    m_zmanimSubTab.SetFont(pFont);
    {
        TCITEM sti = {};
        sti.mask = TCIF_TEXT;
        const wchar_t* subNames[] = {
            L"Alos", L"Misheyakir", L"Sof Zman MA", L"Sof Zman GRA",
            L"Mincha Gedola", L"Mincha Ketana", L"Plag", L"End of Fast", L"Tzais",
            L"Sha'a Zmanit"   // v0.8.83 — independent halachic-hour setting
        };
        for (int i = 0; i < (int)(sizeof(subNames) / sizeof(subNames[0])); ++i)
        {
            sti.pszText = const_cast<LPWSTR>(subNames[i]);
            m_zmanimSubTab.InsertItem(i, &sti);
        }
    }

    // Helper for laying out a preset radio list + custom value edit on a
    // single sub-page. Returns the next free Y position.
    int subId = IDC_OPT_ZSUB_FIRST;
    auto makePresetSubPage =
        [&](std::vector<CWnd*>& page,
            std::vector<CButton*>& radios,
            std::initializer_list<const wchar_t*> presets,
            int startY) -> int
    {
        int yy = startY + 14;
        bool first = true;
        for (const wchar_t* name : presets)
        {
            DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON;
            if (first) { style |= WS_GROUP; first = false; }
            CButton* r = new CButton;
            r->Create(name, style,
                CRect(40, yy, 360, yy + 20), this, subId++);
            r->SetFont(pFont);
            page.push_back(r);
            radios.push_back(r);
            yy += 24;
        }
        return yy;
    };

    // v0.8.82 — revised custom controls: degrees checkbox+value on one line,
    // minutes value+mode-dropdown on a second line.
    // chkDeg / editDeg: new checkbox and degrees value box.
    // editMin / cmbMode: minutes value box and fixed/shaah dropdown.
    // The old deg/min/zmanis radio buttons are still created (kept for
    // legacy Zman Shitos tab compatibility) but moved off-screen at (-1,-1).
    auto attachCustomZmanControls =
        [&](std::vector<CWnd*>& page,
            CEdit& editMin,
            CButton& degRad,
            CButton& minutesRad,
            CButton& zmanisRad,
            int startY,
            CButton& chkDeg,
            CEdit& editDeg,
            CComboBox& cmbMode,
            int degChkId,
            int degValId,
            int modeId) -> int
    {
        // Hide the old radio buttons off-screen (kept for legacy compatibility)
        degRad.MoveWindow(-1, -1, 1, 1);
        minutesRad.MoveWindow(-1, -1, 1, 1);
        zmanisRad.MoveWindow(-1, -1, 1, 1);

        int yy = startY + 8;
        // Row 1: [x] Custom degrees: [value edit]
        chkDeg.Create(L"Custom degrees:", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            CRect(40, yy, 168, yy + 20), this, degChkId);
        chkDeg.SetFont(pFont);
        chkDeg.SetCheck(BST_UNCHECKED);
        page.push_back(&chkDeg);
        editDeg.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
            CRect(170, yy, 234, yy + 22), this, degValId);
        editDeg.SetFont(pFont);
        page.push_back(&editDeg);
        yy += 28;
        // Row 2: Custom minutes: [value] [Fixed minutes / Shaah zmanit v]
        mkStatic(page, L"Custom minutes:", 40, yy, 108, 20);
        editMin.MoveWindow(150, yy, 62, 22);
        page.push_back(&editMin);
        cmbMode.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
            CRect(218, yy - 1, 360, yy + 60), this, modeId);
        cmbMode.SetFont(pFont);
        cmbMode.AddString(L"Fixed minutes");
        cmbMode.AddString(L"Shaah zmanit min");
        cmbMode.SetCurSel(0);
        page.push_back(&cmbMode);
        return yy + 30;
    };

    auto attachCustomNumber =
        [&](std::vector<CWnd*>& page,
            CEdit& edit,
            const wchar_t* label,
            double value,
            int startY) -> int
    {
        int yy = startY + 12;
        mkStatic(page, label, 40, yy, 168, 20);
        setEdit(page, edit, 214, yy, 64, subId++, value);
        return yy + 34;
    };

    // ----- Alos sub-page (reuses m_editCustomAlot + degrees/min/zmanis radios)
    {
        int yy = zmanPageTop;
        mkStatic(m_subPageAlos, L"Preset method for Alos HaShachar:", 28, yy, 320, 20);
        yy = makePresetSubPage(m_subPageAlos, m_radAlosPreset,
            { L"GRA (16.1 degrees)", L"Magen Avraham (72 min)",
              L"Magen Avraham (90 min)", L"Custom" }, yy);
        // pre-select from settings
        int sel = max(0, min(3, m_current.customAlotPreset));
        m_radAlosPreset[sel]->SetCheck(BST_CHECKED);
        attachCustomZmanControls(m_subPageAlos, m_editCustomAlot,
            m_radAlotDegrees, m_radAlotMinutes, m_radAlotZmanis, yy,
            m_chkAlotCustomDeg, m_editCustomAlotDeg, m_cmbAlotMinutesMode,
            IDC_OPT_ALOT_DEG_CHK, IDC_OPT_ALOT_DEG_VAL, IDC_OPT_ALOT_MIN_MODE);
        // Set degrees checkbox + value from settings
        m_chkAlotCustomDeg.SetCheck(m_current.customAlotMode == 0 ? BST_CHECKED : BST_UNCHECKED);
        { CString sv; sv.Format(L"%.4g", m_current.customAlotDegreesValue); m_editCustomAlotDeg.SetWindowText(sv); }
        m_cmbAlotMinutesMode.SetCurSel(m_current.customAlotMode == 2 ? 1 : 0);
        { CString sv; sv.Format(L"%.4g", m_current.customAlotValue > 0 ? m_current.customAlotValue : 72.0); m_editCustomAlot.SetWindowText(sv); }
    }

    // ----- Misheyakir sub-page
    {
        int yy = zmanPageTop;
        mkStatic(m_subPageMisheyakir, L"Preset method for Misheyakir:", 28, yy, 320, 20);
        yy = makePresetSubPage(m_subPageMisheyakir, m_radMisheyakirPreset,
            { L"11.5 degrees", L"11 degrees", L"10.2 degrees", L"Custom" }, yy);
        int sel = max(0, min(3, m_current.customMisheyakirPreset));
        m_radMisheyakirPreset[sel]->SetCheck(BST_CHECKED);
        attachCustomZmanControls(m_subPageMisheyakir, m_editCustomMisheyakir,
            m_radMisheyakirDegrees, m_radMisheyakirMinutes, m_radMisheyakirZmanis, yy,
            m_chkMishCustomDeg, m_editCustomMishDeg, m_cmbMishMinutesMode,
            IDC_OPT_MISH_DEG_CHK, IDC_OPT_MISH_DEG_VAL, IDC_OPT_MISH_MIN_MODE);
        m_chkMishCustomDeg.SetCheck(m_current.customMisheyakirMode == 0 ? BST_CHECKED : BST_UNCHECKED);
        { CString sv; sv.Format(L"%.4g", m_current.customMisheyakirDegreesValue); m_editCustomMishDeg.SetWindowText(sv); }
        m_cmbMishMinutesMode.SetCurSel(m_current.customMisheyakirMode == 2 ? 1 : 0);
        { CString sv; sv.Format(L"%.4g", m_current.customMisheyakirValue > 0 ? m_current.customMisheyakirValue : 50.0); m_editCustomMisheyakir.SetWindowText(sv); }
    }

    // ----- Sof Zman MA sub-page (Shma MA + Tfila MA)
    {
        int yy = zmanPageTop;
        mkStatic(m_subPageSofMA, L"Preset method for Sof Zman Shema/Tefilla (MA):", 28, yy, 360, 20);
        yy = makePresetSubPage(m_subPageSofMA, m_radSofMAPreset,
            { L"90 minutes as degrees", L"Fixed 72 minutes",
              L"72 minutes as 16.1 degrees", L"Custom" }, yy);
        int sel = max(0, min(3, m_current.customSofZmanMaPreset));
        m_radSofMAPreset[sel]->SetCheck(BST_CHECKED);
        attachCustomZmanControls(m_subPageSofMA, m_editCustomSofZman,
            m_radSofZmanDegrees, m_radSofZmanMinutes, m_radSofZmanZmanis, yy,
            m_chkSofCustomDeg, m_editCustomSofDeg, m_cmbSofMinutesMode,
            IDC_OPT_SOF_DEG_CHK, IDC_OPT_SOF_DEG_VAL, IDC_OPT_SOF_MIN_MODE);
        m_chkSofCustomDeg.SetCheck(m_current.customSofZmanMode == 0 ? BST_CHECKED : BST_UNCHECKED);
        { CString sv; sv.Format(L"%.4g", m_current.customSofZmanDegreesValue); m_editCustomSofDeg.SetWindowText(sv); }
        m_cmbSofMinutesMode.SetCurSel(m_current.customSofZmanMode == 2 ? 1 : 0);
        { double mv = m_current.customSofZmanValue > 0 ? m_current.customSofZmanValue : 72.0; CString sv; sv.Format(L"%.4g", mv); m_editCustomSofZman.SetWindowText(sv); }
    }

    // ----- Sof Zman GRA sub-page
    {
        int yy = zmanPageTop;
        mkStatic(m_subPageSofGRA, L"Preset method for Sof Zman Shema/Tefilla (GRA):", 28, yy, 360, 20);
        yy = makePresetSubPage(m_subPageSofGRA, m_radSofGRAPreset,
            { L"90 minutes as degrees", L"Fixed 72 minutes",
              L"72 minutes as 16.1 degrees", L"Custom" }, yy);
        int sel = max(0, min(3, m_current.customSofZmanGraPreset));
        m_radSofGRAPreset[sel]->SetCheck(BST_CHECKED);
        attachCustomZmanControls(m_subPageSofGRA, m_editCustomSofZman,
            m_radSofZmanDegrees, m_radSofZmanMinutes, m_radSofZmanZmanis, yy,
            m_chkSofCustomDeg, m_editCustomSofDeg, m_cmbSofMinutesMode,
            IDC_OPT_SOF_DEG_CHK, IDC_OPT_SOF_DEG_VAL, IDC_OPT_SOF_MIN_MODE);
    }

    // ----- Mincha Gedola sub-page (simple preset + 'Custom (minutes)')
    {
        int yy = zmanPageTop;
        mkStatic(m_subPageMinchaGedola, L"Preset method for Mincha Gedola:", 28, yy, 320, 20);
        yy = makePresetSubPage(m_subPageMinchaGedola, m_radMinchaGedolaPreset,
            { L"30 minutes", L"GRA (default)", L"MA (72 min)",
              L"Custom (minutes)" }, yy);
        int sel = max(0, min(3, m_current.customMinchaGedolaPreset));
        m_radMinchaGedolaPreset[sel]->SetCheck(BST_CHECKED);
        attachCustomNumber(m_subPageMinchaGedola, m_editCustomMinchaGedola,
            L"Custom min after chatzos:", m_current.customMinchaGedolaValue, yy);
    }

    // ----- Mincha Ketana sub-page
    {
        int yy = zmanPageTop;
        mkStatic(m_subPageMinchaKetana, L"Preset method for Mincha Ketana:", 28, yy, 320, 20);
        yy = makePresetSubPage(m_subPageMinchaKetana, m_radMinchaKetanaPreset,
            { L"GRA", L"MA (72 min)", L"Custom (minutes)" }, yy);
        int sel = max(0, min(2, m_current.customMinchaKetanaPreset));
        m_radMinchaKetanaPreset[sel]->SetCheck(BST_CHECKED);
        attachCustomNumber(m_subPageMinchaKetana, m_editCustomMinchaKetana,
            L"Custom z-min after hanetz:", m_current.customMinchaKetanaValue, yy);
    }

    // ----- Plag HaMincha sub-page
    {
        int yy = zmanPageTop;
        mkStatic(m_subPagePlag, L"Preset method for Plag HaMincha:", 28, yy, 320, 20);
        yy = makePresetSubPage(m_subPagePlag, m_radPlagPreset,
            { L"GRA", L"MA (72 min)", L"Custom (minutes)" }, yy);
        int sel = max(0, min(2, m_current.customPlagPreset));
        m_radPlagPreset[sel]->SetCheck(BST_CHECKED);
        attachCustomNumber(m_subPagePlag, m_editCustomPlag,
            L"Custom z-min after hanetz:", m_current.customPlagValue, yy);
    }

    // ----- End of Fast sub-page
    {
        int yy = zmanPageTop;
        mkStatic(m_subPageEndFast, L"Preset method for End of Fast:", 28, yy, 320, 20);
        yy = makePresetSubPage(m_subPageEndFast, m_radEndFastPreset,
            { L"27 minutes (R' Tukaccinsky)",
              L"R' Moshe Feinstein",
              L"Custom (minutes)" }, yy);
        int sel = max(0, min(2, m_current.customEndFastPreset));
        m_radEndFastPreset[sel]->SetCheck(BST_CHECKED);
        yy = attachCustomNumber(m_subPageEndFast, m_editCustomEndFast,
            L"Custom min after shkiah:", m_current.customEndFastValue, yy);
        // v0.8.82 — minutes type dropdown for custom End of Fast
        mkStatic(m_subPageEndFast, L"Minutes type:", 40, yy + 4, 100, 20);
        m_cmbEndFastMinuteMode.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
            CRect(144, yy + 2, 296, yy + 62), this, IDC_OPT_END_FAST_MIN_MODE);
        m_cmbEndFastMinuteMode.SetFont(pFont);
        m_cmbEndFastMinuteMode.AddString(L"Fixed minutes");
        m_cmbEndFastMinuteMode.AddString(L"Shaah zmanit min");
        m_cmbEndFastMinuteMode.SetCurSel(max(0, min(1, m_current.customEndFastMinuteMode)));
        track(m_subPageEndFast, &m_cmbEndFastMinuteMode);
    }

    // ----- Tzais sub-page
    {
        int yy = zmanPageTop;
        mkStatic(m_subPageTzais, L"Preset method for Tzais HaKochavim:", 28, yy, 320, 20);
        yy = makePresetSubPage(m_subPageTzais, m_radTzaisPreset,
            { L"42 minutes", L"50 minutes", L"60 minutes",
              L"72 minutes", L"72 minutes as degrees", L"Custom" }, yy);
        int sel = max(0, min(5, m_current.customTzeitPreset));
        m_radTzaisPreset[sel]->SetCheck(BST_CHECKED);
        attachCustomZmanControls(m_subPageTzais, m_editCustomTzeit,
            m_radTzeitDegrees, m_radTzeitMinutes, m_radTzeitZmanis, yy,
            m_chkTzeitCustomDeg, m_editCustomTzeitDeg, m_cmbTzeitMinutesMode,
            IDC_OPT_TZEIT_DEG_CHK, IDC_OPT_TZEIT_DEG_VAL, IDC_OPT_TZEIT_MIN_MODE);
        m_chkTzeitCustomDeg.SetCheck(m_current.customTzeitMode == 0 ? BST_CHECKED : BST_UNCHECKED);
        { CString sv; sv.Format(L"%.4g", m_current.customTzeitDegreesValue); m_editCustomTzeitDeg.SetWindowText(sv); }
        m_cmbTzeitMinutesMode.SetCurSel(m_current.customTzeitMode == 2 ? 1 : 0);
        { double mv = m_current.customTzeitValue > 0 ? m_current.customTzeitValue : 42.0; CString sv; sv.Format(L"%.4g", mv); m_editCustomTzeit.SetWindowText(sv); }
    }

    // ----- Sha'a Zmanit sub-page (v0.8.83) — independent halachic-hour selector
    {
        int yy = zmanPageTop;
        mkStatic(m_subPageShaahZmanit,
            L"Select the halachic hour (Sha'a Zmanit) used for the bar display "
            L"and as the time-unit for Sof Shema / Sof Tefilla calculations:",
            28, yy, W - 56, 28);
        yy += 34;
        mkStatic(m_subPageShaahZmanit,
            L"This is independent of the Sof Zman start-of-day. "
            L"For example: start from Hanetz (GRA), but measure each hour "
            L"using MA72 lengths — giving a later Sof Shema.",
            28, yy, W - 56, 32);
        yy += 40;
        yy = makePresetSubPage(m_subPageShaahZmanit, m_radShaahZmanitPreset,
            { L"GRA — Hanetz to Shkia \xF7 12  (shorter in winter, longer in summer)",
              L"MA 72 min — (Hanetz\x2212" L"72) to (Shkia+72) \xF7 12",
              L"MA 90 min — (Hanetz\x2212" L"90) to (Shkia+90) \xF7 12",
              L"16.1 degrees — dawn to nightfall \xF7 12",
              L"Custom boundaries" },
            yy);
        // v0.8.83 — explicitly uncheck all before checking the selected one
        // so radio-group exclusivity is enforced even before the dialog shows.
        int selectedShaahSetting = max(0, min(4, m_current.shaahZmanitShita));
        int sel = 0;
        for (int i = 0; i < kShaahZmanitPresetCount; ++i)
            if (kShaahZmanitPresetSettingByUi[i] == selectedShaahSetting)
                sel = i;
        for (CButton* r : m_radShaahZmanitPreset) r->SetCheck(BST_UNCHECKED);
        m_radShaahZmanitPreset[sel]->SetCheck(BST_CHECKED);
        yy += 12;

        mkStatic(m_subPageShaahZmanit, L"Sof Shema / Tefilla hour length:", 40, yy, 220, 20);
        yy += 22;
        m_radSofShaahDisplayed.Create(L"Use selected Sha'a Zmanit above",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON | WS_GROUP,
            CRect(54, yy, 300, yy + 20), this, IDC_OPT_SOF_SHA_USE_DISPLAYED);
        track(m_subPageShaahZmanit, &m_radSofShaahDisplayed);
        m_radSofShaahDisplayed.SetCheck(m_current.sofZmanShaahMode == 0 ? BST_CHECKED : BST_UNCHECKED);
        yy += 24;
        m_radSofShaahBoundary.Create(L"Use Sof Zman method boundaries",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
            CRect(54, yy, 300, yy + 20), this, IDC_OPT_SOF_SHA_USE_BOUNDARY);
        track(m_subPageShaahZmanit, &m_radSofShaahBoundary);
        m_radSofShaahBoundary.SetCheck(m_current.sofZmanShaahMode == 1 ? BST_CHECKED : BST_UNCHECKED);
        yy += 28;

        mkStatic(m_subPageShaahZmanit, L"Custom start boundary:", 40, yy, 160, 20);
        yy += 22;
        m_radShaahStartDegrees.Create(L"Detect by degrees:", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON | WS_GROUP,
            CRect(54, yy, 178, yy + 20), this, IDC_OPT_SHAAH_START_DEG_RAD);
        track(m_subPageShaahZmanit, &m_radShaahStartDegrees);
        m_radShaahStartDegrees.SetCheck(m_current.customShaahStartMode == 0 ? BST_CHECKED : BST_UNCHECKED);
        setEdit(m_subPageShaahZmanit, m_editCustomShaahStartDeg, 184, yy - 1, 58,
            IDC_OPT_SHAAH_START_DEG_VAL, m_current.customShaahStartDegreesValue);
        mkStatic(m_subPageShaahZmanit, L"degrees before Hanetz", 248, yy, 160, 20);
        yy += 26;
        m_radShaahStartFixed.Create(L"Fixed offset:", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
            CRect(54, yy, 178, yy + 20), this, IDC_OPT_SHAAH_START_FIXED_RAD);
        track(m_subPageShaahZmanit, &m_radShaahStartFixed);
        m_radShaahStartFixed.SetCheck(m_current.customShaahStartMode == 1 ? BST_CHECKED : BST_UNCHECKED);
        setEdit(m_subPageShaahZmanit, m_editCustomShaahStart, 184, yy - 1, 58,
            IDC_OPT_SHAAH_START_VALUE, m_current.customShaahStartValue);
        mkStatic(m_subPageShaahZmanit, L"minutes before Hanetz", 248, yy, 160, 20);
        yy += 32;

        mkStatic(m_subPageShaahZmanit, L"Custom end boundary:", 40, yy, 160, 20);
        yy += 22;
        m_radShaahEndDegrees.Create(L"Detect by degrees:", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON | WS_GROUP,
            CRect(54, yy, 178, yy + 20), this, IDC_OPT_SHAAH_END_DEG_RAD);
        track(m_subPageShaahZmanit, &m_radShaahEndDegrees);
        m_radShaahEndDegrees.SetCheck(m_current.customShaahEndMode == 0 ? BST_CHECKED : BST_UNCHECKED);
        setEdit(m_subPageShaahZmanit, m_editCustomShaahEndDeg, 184, yy - 1, 58,
            IDC_OPT_SHAAH_END_DEG_VAL, m_current.customShaahEndDegreesValue);
        mkStatic(m_subPageShaahZmanit, L"degrees after Shkia", 248, yy, 150, 20);
        yy += 26;
        m_radShaahEndFixed.Create(L"Fixed offset:", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
            CRect(54, yy, 178, yy + 20), this, IDC_OPT_SHAAH_END_FIXED_RAD);
        track(m_subPageShaahZmanit, &m_radShaahEndFixed);
        m_radShaahEndFixed.SetCheck(m_current.customShaahEndMode == 1 ? BST_CHECKED : BST_UNCHECKED);
        setEdit(m_subPageShaahZmanit, m_editCustomShaahEnd, 184, yy - 1, 58,
            IDC_OPT_SHAAH_END_VALUE, m_current.customShaahEndValue);
        mkStatic(m_subPageShaahZmanit, L"minutes after Shkia", 248, yy, 150, 20);
    }

    // OK / Cancel buttons
    m_btnOK.Create(L"OK",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        CRect(W - 224, H - 38, W - 156, H - 12), this, IDOK);
    m_btnOK.SetFont(pFont);

    m_btnApply.Create(L"Apply",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(W - 148, H - 38, W - 80, H - 12), this, IDC_OPT_APPLY);
    m_btnApply.SetFont(pFont);

    m_btnCancel.Create(L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(W - 72, H - 38, W - 8, H - 12), this, IDCANCEL);
    m_btnCancel.SetFont(pFont);

    // Set initial radio button states from current settings
    m_radAmPm.SetCheck(!m_current.use24Hour ? BST_CHECKED : BST_UNCHECKED);
    m_rad24hr.SetCheck(m_current.use24Hour ? BST_CHECKED : BST_UNCHECKED);
    m_radCivilMonth.SetCheck(!m_current.defaultHebrewMonth ? BST_CHECKED : BST_UNCHECKED);
    m_radHebrewMonth.SetCheck(m_current.defaultHebrewMonth ? BST_CHECKED : BST_UNCHECKED);
    m_chkDateTracking.SetCheck(m_current.dateTracking ? BST_CHECKED : BST_UNCHECKED);
    m_radDiaspora.SetCheck(!m_current.isIsrael ? BST_CHECKED : BST_UNCHECKED);
    m_radIsrael.SetCheck(m_current.isIsrael ? BST_CHECKED : BST_UNCHECKED);
    m_chkParshios.SetCheck(m_current.showParshios ? BST_CHECKED : BST_UNCHECKED);
    m_chkMoadim.SetCheck(m_current.showMoadim ? BST_CHECKED : BST_UNCHECKED);
    m_chkUserEvents.SetCheck(m_current.showUserEvents ? BST_CHECKED : BST_UNCHECKED);
    m_chkDafYomi.SetCheck(m_current.showDafYomi ? BST_CHECKED : BST_UNCHECKED);
    m_chkYerushalmi.SetCheck(m_current.showYerushalmi ? BST_CHECKED : BST_UNCHECKED);
    m_chkHalacha.SetCheck(m_current.showHalachaYomit ? BST_CHECKED : BST_UNCHECKED);
    m_chkMishna.SetCheck(m_current.showMishnaYomit ? BST_CHECKED : BST_UNCHECKED);
    m_chkTanach.SetCheck(m_current.showTanachYomi ? BST_CHECKED : BST_UNCHECKED);
    m_chkChatzosOnFasts.SetCheck(m_current.showChatzosOnFasts ? BST_CHECKED : BST_UNCHECKED);
    if (m_chkShowBeHaB.GetSafeHwnd())
    {
        m_chkShowBeHaB.SetCheck(m_current.showBeHaB ? BST_CHECKED : BST_UNCHECKED);
        m_chkChatzosOnBeHaB.SetCheck(m_current.showChatzosOnBeHaB ? BST_CHECKED : BST_UNCHECKED);
        m_chkChatzosOnBeHaB.EnableWindow(m_current.showBeHaB);
    }
    m_chkShowTrayIcon.SetCheck(m_current.showTrayIcon ? BST_CHECKED : BST_UNCHECKED);
    m_chkMinimizeToTray.SetCheck(m_current.minimizeToTray ? BST_CHECKED : BST_UNCHECKED);
    m_chkMinimizeOnStartup.SetCheck(m_current.minimizeOnStartup ? BST_CHECKED : BST_UNCHECKED);
    m_chkStartWithWindows.SetCheck(m_current.startWithWindows ? BST_CHECKED : BST_UNCHECKED);
    m_chkDesktopShortcut.SetCheck(m_current.desktopShortcut ? BST_CHECKED : BST_UNCHECKED);
    m_chkTrayBackEnabled.SetCheck(m_current.trayBackEnabled ? BST_CHECKED : BST_UNCHECKED);
    m_btnTrayBackColor.EnableWindow(m_current.trayBackEnabled ? TRUE : FALSE);

    if (m_current.zmanimShita == 1) m_radMA72.SetCheck(BST_CHECKED);
    else if (m_current.zmanimShita == 2) m_radMA90.SetCheck(BST_CHECKED);
    else                                  m_radGRA.SetCheck(BST_CHECKED);

    int candleSel = 1;
    int candleValues[] = { 15, 18, 21, 22, 30, 40 };
    for (int i = 0; i < 6; i++)
        if (m_current.candleLightingMinutes == candleValues[i]) candleSel = i;
    m_cmbCandleMinutes.SetCurSel(candleSel);

    if (m_current.customAlotMode == 1) m_radAlotMinutes.SetCheck(BST_CHECKED);
    else if (m_current.customAlotMode == 2) m_radAlotZmanis.SetCheck(BST_CHECKED);
    else m_radAlotDegrees.SetCheck(BST_CHECKED);

    if (m_current.customMisheyakirMode == 1) m_radMisheyakirMinutes.SetCheck(BST_CHECKED);
    else if (m_current.customMisheyakirMode == 2) m_radMisheyakirZmanis.SetCheck(BST_CHECKED);
    else m_radMisheyakirDegrees.SetCheck(BST_CHECKED);

    if (m_current.customSofZmanMode == 1) m_radSofZmanMinutes.SetCheck(BST_CHECKED);
    else if (m_current.customSofZmanMode == 2) m_radSofZmanZmanis.SetCheck(BST_CHECKED);
    else m_radSofZmanDegrees.SetCheck(BST_CHECKED);

    if (m_current.customTzeitMode == 1) m_radTzeitMinutes.SetCheck(BST_CHECKED);
    else if (m_current.customTzeitMode == 2) m_radTzeitZmanis.SetCheck(BST_CHECKED);
    else m_radTzeitDegrees.SetCheck(BST_CHECKED);
    m_lastAlotMode = CurrentAlotMode();
    m_lastMisheyakirMode = CurrentMisheyakirMode();
    m_lastSofZmanMode = CurrentSofZmanMode();
    m_lastTzeitMode = CurrentTzeitMode();

    auto nearValue = [](double a, double b) { return std::fabs(a - b) < 0.5; };
    int alotPreset = max(0, min(2, m_current.alotShita));
    if (m_current.customAlotMode == 0 && nearValue(m_current.customAlotValue, 16.1))
        alotPreset = 0;
    else if (m_current.customAlotMode == 1 && nearValue(m_current.customAlotValue, 72.0))
        alotPreset = 1;
    else if (m_current.customAlotMode == 1 && nearValue(m_current.customAlotValue, 90.0))
        alotPreset = 2;
    m_cmbAlotShita.SetCurSel(alotPreset);

    int misheyakirPreset = 4;
    if (m_current.customMisheyakirMode == 0 && nearValue(m_current.customMisheyakirValue, 10.2))
        misheyakirPreset = 0;
    else if (m_current.customMisheyakirMode == 0 && nearValue(m_current.customMisheyakirValue, 11.5))
        misheyakirPreset = 1;
    else if (m_current.customMisheyakirMode == 1 && nearValue(m_current.customMisheyakirValue, 60.0))
        misheyakirPreset = 2;
    else if (m_current.customMisheyakirMode == 1 && nearValue(m_current.customMisheyakirValue, 72.0))
        misheyakirPreset = 3;
    m_cmbMisheyakirShita.SetCurSel(misheyakirPreset);

    int sofPreset = 4;
    if (m_current.customSofZmanMode == 0 && nearValue(m_current.customSofZmanValue, 0.0))
        sofPreset = 0;
    else if (m_current.customSofZmanMode == 0 && nearValue(m_current.customSofZmanValue, 19.8))
        sofPreset = 1;
    else if (m_current.customSofZmanMode == 1 && nearValue(m_current.customSofZmanValue, 72.0))
        sofPreset = 2;
    else if (m_current.customSofZmanMode == 0 && nearValue(m_current.customSofZmanValue, 16.1))
        sofPreset = 3;
    m_cmbSofZmanShita.SetCurSel(sofPreset);

    int tzeitPreset = max(0, min(4, m_current.tzeitShita));
    if (m_current.customTzeitMode == 0 && nearValue(m_current.customTzeitValue, 8.5))
        tzeitPreset = 0;
    else if (m_current.customTzeitMode == 1 && nearValue(m_current.customTzeitValue, 72.0))
        tzeitPreset = 1;
    else if (m_current.customTzeitMode == 1 && nearValue(m_current.customTzeitValue, 90.0))
        tzeitPreset = 2;
    else if (m_current.customTzeitMode == 2 && nearValue(m_current.customTzeitValue, 72.0))
        tzeitPreset = 3;
    else if (m_current.customTzeitMode == 2 && nearValue(m_current.customTzeitValue, 90.0))
        tzeitPreset = 4;
    else if (m_current.customTzeitMode == 1 && nearValue(m_current.customTzeitValue, 42.0))
        tzeitPreset = 5;
    else if (m_current.customTzeitMode == 1 && nearValue(m_current.customTzeitValue, 50.0))
        tzeitPreset = 6;
    else if (m_current.customTzeitMode == 1 && nearValue(m_current.customTzeitValue, 60.0))
        tzeitPreset = 7;
    m_cmbTzeitShita.SetCurSel(tzeitPreset);

    // v0.8.54 - Auto-update settings initialization
    m_chkDisableAutoUpdate.SetCheck(m_current.disableAutoUpdate ? BST_CHECKED : BST_UNCHECKED);
    m_chkCheckUpdatesAuto.SetCheck(m_current.checkUpdatesAuto ? BST_CHECKED : BST_UNCHECKED);
    m_cmbUpdateFrequency.SetCurSel(max(0, min(2, m_current.updateCheckFrequency)));
    m_chkCheckOnLaunch.SetCheck(m_current.checkUpdatesOnLaunch ? BST_CHECKED : BST_UNCHECKED);
    // Enable/disable the frequency and launch checkboxes based on the disable flag
    bool isDisabled = m_current.disableAutoUpdate;
    m_chkCheckUpdatesAuto.EnableWindow(!isDisabled);
    m_cmbUpdateFrequency.EnableWindow(!isDisabled);
    m_chkCheckOnLaunch.EnableWindow(!isDisabled);
    m_btnCheckUpdatesNow.EnableWindow(!isDisabled);

    UpdateColorButtons();
    UpdateNotificationControls();
    ShowOptionsPage(0);
    m_initialized = true;
    SetDirty(false);

    // Initialize tooltips
    m_tooltip.Create(this, TTS_ALWAYSTIP | TTS_BALLOON);
    m_tooltip.SetMaxTipWidth(300);
    m_tooltip.Activate(TRUE);

    // General tab
    m_tooltip.AddTool(&m_radAmPm,           L"Display times in 12-hour AM/PM format");
    m_tooltip.AddTool(&m_rad24hr,           L"Display times in 24-hour format");
    m_tooltip.AddTool(&m_radCivilMonth,     L"Navigate the calendar by civil (Gregorian) month");
    m_tooltip.AddTool(&m_radHebrewMonth,    L"Navigate the calendar by Hebrew month");
    m_tooltip.AddTool(&m_chkDateTracking,   L"Highlight the date cell under the mouse as you hover");
    m_tooltip.AddTool(&m_cmbFontSize,       L"Font size for the calendar cell text");
    m_tooltip.AddTool(&m_radDiaspora,       L"Diaspora: follows 2-day Yom Tov outside Israel");
    m_tooltip.AddTool(&m_radIsrael,         L"Israel: follows 1-day Yom Tov as observed in Israel");

    // Month View tab
    m_tooltip.AddTool(&m_chkParshios,       L"Show the weekly Torah portion (parasha) on the calendar");
    m_tooltip.AddTool(&m_chkMoadim,         L"Show Jewish holidays and Moadim on the calendar");
    m_tooltip.AddTool(&m_chkUserEvents,     L"Show personal events added by the user");
    m_tooltip.AddTool(&m_chkDafYomi,        L"Show the Daf Yomi (Babylonian Talmud) daily page");
    m_tooltip.AddTool(&m_chkYerushalmi,     L"Show the Yerushalmi (Jerusalem Talmud) daily page");
    m_tooltip.AddTool(&m_chkHalacha,        L"Show the Halacha Yomit daily halacha study");
    m_tooltip.AddTool(&m_chkMishna,         L"Show the Mishna Yomit daily Mishna study");
    m_tooltip.AddTool(&m_chkTanach,         L"Show the Tanach Yomi daily Tanach study");

    // Interface tab
    m_tooltip.AddTool(&m_chkShowTrayIcon,   L"Keep the WinLuach icon pinned in the system tray while the app is open");
    m_tooltip.AddTool(&m_chkMinimizeToTray, L"Send the window to the system tray when minimized or closed");
    m_tooltip.AddTool(&m_cmbTrayWhen,       L"Choose when the window minimizes to tray");
    m_tooltip.AddTool(&m_chkMinimizeOnStartup, L"Start the application minimized to the system tray");
    m_tooltip.AddTool(&m_chkStartWithWindows,  L"Automatically launch WinLuach when Windows starts");
    m_tooltip.AddTool(&m_chkDesktopShortcut,   L"Create a desktop shortcut for WinLuach");
    m_tooltip.AddTool(&m_btnTrayTextColor,  L"Choose the color for the tray icon clock text");
    m_tooltip.AddTool(&m_btnTrayFont,       L"Choose the tray icon font and formatting");
    m_tooltip.AddTool(&m_chkTrayBackEnabled,L"Draw a colored background behind the tray icon date");
    m_tooltip.AddTool(&m_btnTrayBackColor,  L"Choose the tray icon background color");
    m_tooltip.AddTool(&m_cmbTrayNumber,     L"Choose Hebrew letters or English digits for the tray date");
    m_tooltip.AddTool(&m_btnTrayDefaults,   L"Restore the default tray icon text, background, and font settings");
    m_tooltip.AddTool(&m_btnManageCals,     L"Add, remove, or edit web calendar (ICS) subscriptions");

    // Zmanim tab
    m_tooltip.AddTool(&m_radGRA,            L"Use Vilna Gaon (GRA) calculation: day runs from sunrise to sunset");
    m_tooltip.AddTool(&m_radMA72,           L"Use Magen Avraham with 72-minute shaah zmanit before sunrise/after sunset");
    m_tooltip.AddTool(&m_radMA90,           L"Use Magen Avraham with 90-minute shaah zmanit before sunrise/after sunset (default)");
    m_tooltip.AddTool(&m_cmbAlotShita,      L"Preset method for calculating Alos HaShachar (dawn)");
    m_tooltip.AddTool(&m_cmbTzeitShita,     L"Preset method for calculating Tzeis HaKochavim (nightfall)");
    m_tooltip.AddTool(&m_chkChatzosOnFasts,    L"Show Chatzos time on public fast day calendar cells");
    if (m_chkShowBeHaB.GetSafeHwnd())
    {
        m_tooltip.AddTool(&m_chkShowBeHaB,      L"Highlight BeHaB days (Mon-Thu-Mon of Cheshvan and Iyar) with fast-day cell color");
        m_tooltip.AddTool(&m_chkChatzosOnBeHaB, L"Show Chatzos time on BeHaB day calendar cells (requires Show BeHaB days)");
    }
    m_tooltip.AddTool(&m_cmbCandleMinutes,  L"Minutes before sunset for Shabbos/Yom Tov candle lighting");
    m_tooltip.AddTool(&m_editCustomAlot,    L"Custom value for Alos HaShachar (degrees below horizon or minutes)");
    m_tooltip.AddTool(&m_radAlotDegrees,    L"Express Alos as degrees below the horizon");
    m_tooltip.AddTool(&m_radAlotMinutes,    L"Express Alos as fixed minutes before sunrise");
    m_tooltip.AddTool(&m_radAlotZmanis,     L"Express Alos as shaah zmanit (proportional) minutes before sunrise");
    m_tooltip.AddTool(&m_editCustomTzeit,   L"Custom value for Tzeis HaKochavim (degrees or minutes)");
    m_tooltip.AddTool(&m_radTzeitDegrees,   L"Express Tzeis as degrees below the horizon");
    m_tooltip.AddTool(&m_radTzeitMinutes,   L"Express Tzeis as fixed minutes after sunset");
    m_tooltip.AddTool(&m_radTzeitZmanis,    L"Express Tzeis as shaah zmanit (proportional) minutes after sunset");

    // Zman Shitos tab
    m_tooltip.AddTool(&m_cmbSofZmanShita,       L"Preset calculation method for Sof Zman Shema and Tefilla");
    m_tooltip.AddTool(&m_editCustomSofZman,     L"Custom value for the Sof Zman period (degrees or minutes)");
    m_tooltip.AddTool(&m_radSofZmanDegrees,     L"Express Sof Zman offset as degrees below horizon");
    m_tooltip.AddTool(&m_radSofZmanMinutes,     L"Express Sof Zman offset as fixed minutes");
    m_tooltip.AddTool(&m_radSofZmanZmanis,      L"Express Sof Zman offset as shaah zmanit (proportional) minutes");
    m_tooltip.AddTool(&m_cmbMisheyakirShita,    L"Preset calculation method for Misheyakir");
    m_tooltip.AddTool(&m_editCustomMisheyakir,  L"Custom value for Misheyakir (degrees or minutes)");
    m_tooltip.AddTool(&m_radMisheyakirDegrees,  L"Express Misheyakir as degrees below the horizon");
    m_tooltip.AddTool(&m_radMisheyakirMinutes,  L"Express Misheyakir as fixed minutes before sunrise");
    m_tooltip.AddTool(&m_radMisheyakirZmanis,   L"Express Misheyakir as shaah zmanit (proportional) minutes");

    // Notifications tab
    m_tooltip.AddTool(&m_cmbNotifyPersonal,     L"Notification style for personal calendar events");
    m_tooltip.AddTool(&m_cmbNotifyWebCal,       L"Notification style for web calendar (ICS) events");
    m_tooltip.AddTool(&m_cmbNotifySefirah,      L"Notification style for the nightly Sefiras HaOmer reminder");
    m_tooltip.AddTool(&m_cmbNotifySefirahHour,  L"Hour for a manual Sefiras HaOmer reminder");
    m_tooltip.AddTool(&m_cmbNotifySefirahMinute,L"Minute for a manual Sefiras HaOmer reminder");
    m_tooltip.AddTool(&m_cmbNotifySefirahAmPm,  L"AM or PM for a manual Sefiras HaOmer reminder");
    m_tooltip.AddTool(&m_radNotifySefirahSunTzais, L"Fire the Omer reminder relative to sunset or tzais — pick which one in the dropdown to the right");
    m_tooltip.AddTool(&m_radNotifySefirahOther,    L"Fire the Omer reminder relative to another zman — pick the zman in the dropdown to the right");
    m_tooltip.AddTool(&m_radNotifySefirahManual,   L"Fire the Omer reminder at a fixed clock time — set the time in the controls to the right");
    m_tooltip.AddTool(&m_editNotifySefirahOffset, L"Offset in minutes from the chosen zman (ignored for manual time)");
    m_tooltip.AddTool(&m_cmbNotifySefirahDir,    L"Whether the offset fires before or after the reference zman (ignored for manual time)");
    m_tooltip.AddTool(&m_cmbNotifySefirahBase,   L"Choose sunset or tzais as the base for the Omer reminder");
    m_tooltip.AddTool(&m_cmbNotifySefirahOtherZman, L"The specific zman to use as the base for the Omer reminder");
    m_tooltip.AddTool(&m_cmbNotifyZmanim,       L"Notification style for upcoming zmanim alerts");
    m_tooltip.AddTool(&m_cmbNotifyMoadim,       L"Notification style for upcoming holidays and Moadim");
    m_tooltip.AddTool(&m_editNotifyMoadimAmount, L"How far before a holiday or moed to remind you");
    m_tooltip.AddTool(&m_cmbNotifyMoadimUnit,    L"Units for the holiday advance reminder (minutes, hours, days, etc.)");
    m_tooltip.AddTool(&m_cmbNotifyParsha,       L"Notify for a specific parasha, or any parasha");
    m_tooltip.AddTool(&m_cmbNotifyParshaStyle,  L"Notification style for Shabbos parasha reminders");
    m_tooltip.AddTool(&m_editNotifyParshaAmount, L"How far before Shabbos to remind you about the selected parasha");
    m_tooltip.AddTool(&m_cmbNotifyParshaUnit,    L"Units for the parasha advance reminder (minutes, hours, days, etc.)");
    m_tooltip.AddTool(&m_editNotifyPersonalAmount, L"How far before personal events to remind you");
    m_tooltip.AddTool(&m_cmbNotifyPersonalUnit,  L"Units for the personal event advance reminder");
    m_tooltip.AddTool(&m_btnPreviewNotify,      L"Preview the selected notification style");
    m_tooltip.AddTool(&m_btnAdvancedReminders,  L"Configure advanced reminder rules and schedules");

    // Colors tab
    m_tooltip.AddTool(&m_cmbColorItem,      L"Select which calendar element to customize the color for");
    m_tooltip.AddTool(&m_editColorHex,      L"Enter a hex color code (e.g. #FF8800) to set precisely");
    m_tooltip.AddTool(&m_btnColorPicker,    L"Open the color picker dialog to choose a color visually");
    m_tooltip.AddTool(&m_cmbColorPreview,   L"Preview colors using a sample day type");
    m_tooltip.AddTool(&m_btnRestoreColors,  L"Reset all calendar cell colors back to their defaults");
    m_tooltip.AddTool(&m_btnResetCalendar,  L"Reset all saved WinLuach calendar settings after offering to back them up");

    // Tray Tooltip tab — zmanim checkboxes
    for (int i = 0; i < kTrayTooltipZmanCount; ++i)
        if (m_chkTrayTooltipZmanim[i].GetSafeHwnd())
            m_tooltip.AddTool(&m_chkTrayTooltipZmanim[i],
                L"Show this zman in the tray icon tooltip when hovering over the tray icon");

    // Zmanim Bar tab — per-zman checkboxes
    for (CButton* pChk : m_zmanimBarChecks)
        if (pChk && pChk->GetSafeHwnd())
            m_tooltip.AddTool(pChk,
                L"Show this zman in the bottom zmanim panel");

    // Bottom buttons
    m_tooltip.AddTool(&m_btnOK,     L"Save settings and close the Options dialog");
    m_tooltip.AddTool(&m_btnApply,  L"Apply settings immediately without closing");
    m_tooltip.AddTool(&m_btnCancel, L"Close without saving any changes");

    return TRUE;
}

BOOL COptionsDlg::PreTranslateMessage(MSG* pMsg)
{
    if (m_tooltip.GetSafeHwnd())
        m_tooltip.RelayEvent(pMsg);
    return CDialog::PreTranslateMessage(pMsg);
}

// =============================================================================
// ON OK — reads radio button states into m_result
// =============================================================================

void COptionsDlg::ShowOptionsPage(int page)
{
    auto showPage = [page](std::vector<CWnd*>& ctrls, int pageIndex) {
        int cmd = (page == pageIndex) ? SW_SHOW : SW_HIDE;
        for (CWnd* ctrl : ctrls)
            if (ctrl && ctrl->GetSafeHwnd())
                ctrl->ShowWindow(cmd);
    };

    // Tab order:
    //   0=General  1=Month  2=Interface  3=TrayTooltip
    //   4=ZmanimBar  5=Colors  6=Zmanim  7=Notifications  8=YearDetails
    showPage(m_pageGeneral,      0);
    showPage(m_pageMonth,        1);
    showPage(m_pageInterface,    2);
    showPage(m_pageTrayTooltip,  3);
    showPage(m_pageZmanimBar,    4);
    showPage(m_pageColors,       5);
    showPage(m_pageNotifications,7);
    showPage(m_pageYearDetails,  8);
    // m_pageZmanim is the legacy flat Zmanim layout; in v0.8.0 it has been
    // superseded by the Zmanim sub-tab so always hide it.
    showPage(m_pageZmanim,      -1);
    // Legacy ZmanShitos page is no longer exposed via a tab but kept in the
    // vector for migration; always hide it before showing any reused controls
    // inside the visible Zmanim sub-tabs.
    showPage(m_pageZmanShitos, -1);

    // Zmanim sub-tab + sub-pages: only visible when the Zmanim tab is active.
    // v0.8.2 - explicitly raise the sub-tab control to the top of z-order
    // and force a repaint so its tab strip ("Alos | Misheyakir | ...") is
    // visible.  Sub-page widgets are then raised on top of the sub-tab in
    // ShowZmanimSubPage so they remain visible inside the tab body.
    if (page == 6)
    {
        if (m_zmanimSubTab.GetSafeHwnd())
        {
            m_zmanimSubTab.ShowWindow(SW_SHOW);
            m_zmanimSubTab.SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            m_zmanimSubTab.Invalidate();
            m_zmanimSubTab.UpdateWindow();
        }
        ShowZmanimSubPage(max(0, m_zmanimSubTab.GetCurSel()));
        HideAllNotifySubPages();
    }
    else if (page == 7)
    {
        if (m_notifySubTab.GetSafeHwnd())
        {
            m_notifySubTab.ShowWindow(SW_SHOW);
            m_notifySubTab.SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            m_notifySubTab.Invalidate();
            m_notifySubTab.UpdateWindow();
        }
        ShowNotifySubPage(max(0, m_notifySubTab.GetCurSel()));
        HideAllZmanimSubPages();
    }
    else
    {
        HideAllZmanimSubPages();
        HideAllNotifySubPages();
    }
}

void COptionsDlg::HideAllZmanimSubPages()
{
    if (m_zmanimSubTab.GetSafeHwnd())
        m_zmanimSubTab.ShowWindow(SW_HIDE);
    auto hide = [](std::vector<CWnd*>& v) {
        for (CWnd* c : v) if (c && c->GetSafeHwnd()) c->ShowWindow(SW_HIDE);
    };
    hide(m_subPageAlos);
    hide(m_subPageMisheyakir);
    hide(m_subPageSofMA);
    hide(m_subPageSofGRA);
    hide(m_subPageMinchaGedola);
    hide(m_subPageMinchaKetana);
    hide(m_subPagePlag);
    hide(m_subPageEndFast);
    hide(m_subPageTzais);
    hide(m_subPageShaahZmanit);  // v0.8.83
}

// v0.8.1 - Show the requested Zmanim sub-page and ALSO raise each of its
// widgets to the top of the dialog's z-order.  Without this lift, the sibling
// CTabCtrl (m_zmanimSubTab) repaints its body over the radios so they look
// invisible even though they were set SW_SHOW (the bug reported in v0.8.0).
void COptionsDlg::ShowZmanimSubPage(int sub)
{
    std::vector<CWnd*>* pages[] = {
        &m_subPageAlos, &m_subPageMisheyakir,
        &m_subPageSofMA, &m_subPageSofGRA,
        &m_subPageMinchaGedola, &m_subPageMinchaKetana,
        &m_subPagePlag, &m_subPageEndFast, &m_subPageTzais,
        &m_subPageShaahZmanit   // v0.8.83 — index 9
    };
    const int count = (int)(sizeof(pages) / sizeof(pages[0]));
    for (int i = 0; i < count; ++i)
    {
        for (CWnd* c : *pages[i])
            if (c && c->GetSafeHwnd())
                c->ShowWindow(SW_HIDE);
    }
    if (sub < 0 || sub >= count)
        return;
    for (CWnd* c : *pages[sub])
    {
        if (!c || !c->GetSafeHwnd()) continue;
        c->ShowWindow(SW_SHOW);
        // Raise each widget above the sub-tab control so the tab body does
        // not overpaint it. SWP_NOACTIVATE keeps focus where it was.
        c->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        c->Invalidate();
    }
}

void COptionsDlg::OnTabChanged(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    ShowOptionsPage(max(0, m_tab.GetCurSel()));
    *pResult = 0;
}

// v0.8.0 - Triggered when the user clicks a different Zmanim sub-tab.
void COptionsDlg::OnZmanimSubTabChanged(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    ShowZmanimSubPage(max(0, m_zmanimSubTab.GetCurSel()));
    *pResult = 0;
}

void COptionsDlg::HideAllNotifySubPages()
{
    if (m_notifySubTab.GetSafeHwnd())
        m_notifySubTab.ShowWindow(SW_HIDE);
    auto hide = [](std::vector<CWnd*>& v) {
        for (CWnd* c : v) if (c && c->GetSafeHwnd()) c->ShowWindow(SW_HIDE);
    };
    hide(m_pageNotifyGeneral);
    hide(m_pageNotifyZmanim);
    hide(m_pageNotifyReminders);
}

void COptionsDlg::ShowNotifySubPage(int sub)
{
    std::vector<CWnd*>* pages[] = {
        &m_pageNotifyGeneral, &m_pageNotifyZmanim, &m_pageNotifyReminders
    };
    const int count = 3;
    for (int i = 0; i < count; ++i)
        for (CWnd* c : *pages[i])
            if (c && c->GetSafeHwnd()) c->ShowWindow(SW_HIDE);
    if (sub < 0 || sub >= count) return;
    // Place the first control at the very top of z-order, then chain each
    // subsequent control just below the previous one.  This preserves the
    // creation-order z-order so that WS_GROUP radio-button brackets stay
    // intact (all-to-wndTop reverses the order, mixing radio groups).
    CWnd* insertAfter = const_cast<CWnd*>(&CWnd::wndTop);
    for (CWnd* c : *pages[sub]) {
        if (!c || !c->GetSafeHwnd()) continue;
        c->ShowWindow(SW_SHOW);
        c->SetWindowPos(insertAfter, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        c->Invalidate();
        insertAfter = c;
    }
    if (sub == 0) UpdateSefirahControls();
    if (sub == 1) UpdateNotificationControls();
}

void COptionsDlg::OnNotifySubTabChanged(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    ShowNotifySubPage(max(0, m_notifySubTab.GetCurSel()));
    *pResult = 0;
}

void COptionsDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);
    if (m_tab.GetSafeHwnd())
        m_tab.MoveWindow(6, 6, max(120, cx - 12), max(120, cy - 54));
    if (m_btnOK.GetSafeHwnd())
        m_btnOK.MoveWindow(max(10, cx - 224), max(10, cy - 38), 68, 26);
    if (m_btnApply.GetSafeHwnd())
        m_btnApply.MoveWindow(max(82, cx - 148), max(10, cy - 38), 68, 26);
    if (m_btnCancel.GetSafeHwnd())
        m_btnCancel.MoveWindow(max(82, cx - 72), max(10, cy - 38), 64, 26);
}

void COptionsDlg::SetDirty(bool dirty)
{
    m_dirty = dirty;
    if (m_btnApply.GetSafeHwnd())
        m_btnApply.EnableWindow(m_dirty ? TRUE : FALSE);
}

void COptionsDlg::UpdateColorButtons()
{
    UpdateColorEditorFromSelection();
    InvalidateColorPreview();
}

bool COptionsDlg::ChooseCalendarColor(UINT id)
{
    if (id != IDC_OPT_COLOR_PICKER)
        return false;

    int idx = CurrentColorIndex();
    if (idx < 0)
        return false;

    const auto& pref = kCalendarColorPrefs[idx];
    CColorDialog dlg((COLORREF)(m_result.*(pref.field)), CC_FULLOPEN, this);
    if (dlg.DoModal() == IDOK)
    {
        m_result.*(pref.field) = (int)dlg.GetColor();
        SetColorPreviewForPref(idx);
        UpdateColorEditorFromSelection();
    }
    return true;
}

void COptionsDlg::RestoreDefaultCalendarColors()
{
    for (const auto& pref : kCalendarColorPrefs)
        m_result.*(pref.field) = pref.defaultValue;
    UpdateColorButtons();
}

void COptionsDlg::UpdateColorEditorFromSelection()
{
    if (!m_editColorHex.GetSafeHwnd())
        return;

    int idx = CurrentColorIndex();
    if (idx < 0)
        return;

    m_updatingColorUi = true;
    const auto& pref = kCalendarColorPrefs[idx];
    m_editColorHex.SetWindowText(ColorToHex((COLORREF)(m_result.*(pref.field))));
    SetColorPreviewForPref(idx);
    m_updatingColorUi = false;
}

void COptionsDlg::ApplyColorHexFromEditor()
{
    if (m_updatingColorUi)
        return;

    int idx = CurrentColorIndex();
    if (idx < 0)
        return;

    CString text;
    m_editColorHex.GetWindowText(text);
    COLORREF color = 0;
    if (!ParseHexColor(text, color))
        return;

    const auto& pref = kCalendarColorPrefs[idx];
    m_result.*(pref.field) = (int)color;
    SetColorPreviewForPref(idx);
    InvalidateColorPreview();
}

void COptionsDlg::SetColorPreviewForPref(int colorIndex)
{
    if (colorIndex < 0 || colorIndex >= (int)(sizeof(kCalendarColorPrefs) / sizeof(kCalendarColorPrefs[0])))
        return;

    int kind = kCalendarColorPrefs[colorIndex].previewKind;
    if (m_cmbColorPreview.GetSafeHwnd())
        m_cmbColorPreview.SetCurSel(max(0, min((int)(sizeof(kColorPreviewNames) / sizeof(kColorPreviewNames[0])) - 1, kind)));
    if (m_colorPreview.GetSafeHwnd())
        m_colorPreview.SetPreview(&m_result, kind);
}

void COptionsDlg::InvalidateColorPreview()
{
    if (!m_colorPreview.GetSafeHwnd())
        return;

    int kind = max(0, m_cmbColorPreview.GetCurSel());
    m_colorPreview.SetPreview(&m_result, kind);
}

int COptionsDlg::CurrentColorIndex() const
{
    if (!m_cmbColorItem.GetSafeHwnd())
        return -1;
    int idx = m_cmbColorItem.GetCurSel();
    int count = (int)(sizeof(kCalendarColorPrefs) / sizeof(kCalendarColorPrefs[0]));
    return (idx >= 0 && idx < count) ? idx : -1;
}

int COptionsDlg::CurrentAlotMode() const
{
    if (m_radAlotMinutes.GetCheck() == BST_CHECKED) return 1;
    if (m_radAlotZmanis.GetCheck() == BST_CHECKED) return 2;
    return 0;
}

int COptionsDlg::CurrentMisheyakirMode() const
{
    if (m_radMisheyakirMinutes.GetCheck() == BST_CHECKED) return 1;
    if (m_radMisheyakirZmanis.GetCheck() == BST_CHECKED) return 2;
    return 0;
}

int COptionsDlg::CurrentSofZmanMode() const
{
    if (m_radSofZmanMinutes.GetCheck() == BST_CHECKED) return 1;
    if (m_radSofZmanZmanis.GetCheck() == BST_CHECKED) return 2;
    return 0;
}

int COptionsDlg::CurrentTzeitMode() const
{
    if (m_radTzeitMinutes.GetCheck() == BST_CHECKED) return 1;
    if (m_radTzeitZmanis.GetCheck() == BST_CHECKED) return 2;
    return 0;
}

void COptionsDlg::SetAlotMode(int mode)
{
    m_radAlotDegrees.SetCheck(mode == 0 ? BST_CHECKED : BST_UNCHECKED);
    m_radAlotMinutes.SetCheck(mode == 1 ? BST_CHECKED : BST_UNCHECKED);
    m_radAlotZmanis.SetCheck(mode == 2 ? BST_CHECKED : BST_UNCHECKED);
}

void COptionsDlg::SetMisheyakirMode(int mode)
{
    m_radMisheyakirDegrees.SetCheck(mode == 0 ? BST_CHECKED : BST_UNCHECKED);
    m_radMisheyakirMinutes.SetCheck(mode == 1 ? BST_CHECKED : BST_UNCHECKED);
    m_radMisheyakirZmanis.SetCheck(mode == 2 ? BST_CHECKED : BST_UNCHECKED);
}

void COptionsDlg::SetSofZmanMode(int mode)
{
    m_radSofZmanDegrees.SetCheck(mode == 0 ? BST_CHECKED : BST_UNCHECKED);
    m_radSofZmanMinutes.SetCheck(mode == 1 ? BST_CHECKED : BST_UNCHECKED);
    m_radSofZmanZmanis.SetCheck(mode == 2 ? BST_CHECKED : BST_UNCHECKED);
}

void COptionsDlg::SetTzeitMode(int mode)
{
    m_radTzeitDegrees.SetCheck(mode == 0 ? BST_CHECKED : BST_UNCHECKED);
    m_radTzeitMinutes.SetCheck(mode == 1 ? BST_CHECKED : BST_UNCHECKED);
    m_radTzeitZmanis.SetCheck(mode == 2 ? BST_CHECKED : BST_UNCHECKED);
}

double COptionsDlg::GetEditValue(const CEdit& edit, double fallback) const
{
    CString text;
    edit.GetWindowText(text);
    double value = _wtof(text);
    return value > 0.0 ? value : fallback;
}

void COptionsDlg::SetEditValue(CEdit& edit, double value)
{
    CString text;
    if (std::fabs(value - std::round(value)) < 0.01)
        text.Format(L"%.0f", value);
    else
        text.Format(L"%.2f", value);
    edit.SetWindowText(text);
}

void COptionsDlg::ApplyAlotPreset()
{
    int sel = max(0, m_cmbAlotShita.GetCurSel());
    if (sel == 0) { SetAlotMode(0); SetEditValue(m_editCustomAlot, 16.1); }
    else if (sel == 1) { SetAlotMode(1); SetEditValue(m_editCustomAlot, 72.0); }
    else { SetAlotMode(1); SetEditValue(m_editCustomAlot, 90.0); }
    m_lastAlotMode = CurrentAlotMode();
}

void COptionsDlg::ApplyMisheyakirPreset()
{
    int sel = max(0, m_cmbMisheyakirShita.GetCurSel());
    switch (sel)
    {
    case 0: SetMisheyakirMode(0); SetEditValue(m_editCustomMisheyakir, 10.2); break;
    case 1: SetMisheyakirMode(0); SetEditValue(m_editCustomMisheyakir, 11.5); break;
    case 2: SetMisheyakirMode(1); SetEditValue(m_editCustomMisheyakir, 60.0); break;
    case 3: SetMisheyakirMode(1); SetEditValue(m_editCustomMisheyakir, 72.0); break;
    default: break;
    }
    m_lastMisheyakirMode = CurrentMisheyakirMode();
}

void COptionsDlg::ApplySofZmanPreset()
{
    int sel = max(0, m_cmbSofZmanShita.GetCurSel());
    switch (sel)
    {
    case 0: SetSofZmanMode(0); SetEditValue(m_editCustomSofZman, 0.0); break;
    case 1: SetSofZmanMode(0); SetEditValue(m_editCustomSofZman, 19.8); break;
    case 2: SetSofZmanMode(1); SetEditValue(m_editCustomSofZman, 72.0); break;
    case 3: SetSofZmanMode(0); SetEditValue(m_editCustomSofZman, 16.1); break;
    default: break;
    }
    m_lastSofZmanMode = CurrentSofZmanMode();
}

void COptionsDlg::ApplyTzeitPreset()
{
    int sel = max(0, m_cmbTzeitShita.GetCurSel());
    switch (sel)
    {
    case 0: SetTzeitMode(0); SetEditValue(m_editCustomTzeit, 8.5); break;
    case 1: SetTzeitMode(1); SetEditValue(m_editCustomTzeit, 72.0); break;
    case 2: SetTzeitMode(1); SetEditValue(m_editCustomTzeit, 90.0); break;
    case 3: SetTzeitMode(2); SetEditValue(m_editCustomTzeit, 72.0); break;
    case 4: SetTzeitMode(2); SetEditValue(m_editCustomTzeit, 90.0); break;
    case 5: SetTzeitMode(1); SetEditValue(m_editCustomTzeit, 42.0); break;
    case 6: SetTzeitMode(1); SetEditValue(m_editCustomTzeit, 50.0); break;
    case 7: SetTzeitMode(1); SetEditValue(m_editCustomTzeit, 60.0); break;
    default: break;
    }
    m_lastTzeitMode = CurrentTzeitMode();
}

static double ConvertAlotValue(int oldMode, int newMode, double value)
{
    if (oldMode == newMode)
        return value;
    if (newMode == 0)
        return (value >= 85.0) ? 19.8 : 16.1;
    if (oldMode == 0)
        return (value >= 18.0) ? 90.0 : 72.0;
    return value;
}

static double ConvertTzeitValue(int oldMode, int newMode, double value)
{
    if (oldMode == newMode)
        return value;
    if (newMode == 0)
    {
        if (value >= 85.0) return 16.1;
        if (value >= 55.0) return 12.0;
        return 8.5;
    }
    if (oldMode == 0)
    {
        if (value >= 15.0) return 90.0;
        if (value >= 10.0) return 60.0;
        return 42.0;
    }
    return value;
}

static double ConvertMisheyakirValue(int oldMode, int newMode, double value)
{
    if (oldMode == newMode)
        return value;
    if (newMode == 0)
        return (value >= 70.0) ? 16.1 : 10.2;
    if (oldMode == 0)
        return (value >= 15.0) ? 72.0 : 60.0;
    return value;
}

static double ConvertSofZmanValue(int oldMode, int newMode, double value)
{
    if (oldMode == newMode)
        return value;
    if (newMode == 0)
    {
        if (value <= 0.01) return 0.0;
        return (value >= 85.0) ? 19.8 : 16.1;
    }
    if (oldMode == 0)
    {
        if (value <= 0.01) return 0.0;
        return (value >= 18.0) ? 90.0 : 72.0;
    }
    return value;
}

void COptionsDlg::ConvertAlotMode(int newMode)
{
    int oldMode = m_lastAlotMode;
    double oldValue = GetEditValue(m_editCustomAlot, oldMode == 0 ? 16.1 : 72.0);
    SetAlotMode(newMode);
    SetEditValue(m_editCustomAlot, ConvertAlotValue(oldMode, newMode, oldValue));
    m_lastAlotMode = newMode;
}

void COptionsDlg::ConvertMisheyakirMode(int newMode)
{
    int oldMode = m_lastMisheyakirMode;
    double oldValue = GetEditValue(m_editCustomMisheyakir, oldMode == 0 ? 10.2 : 60.0);
    SetMisheyakirMode(newMode);
    SetEditValue(m_editCustomMisheyakir, ConvertMisheyakirValue(oldMode, newMode, oldValue));
    m_lastMisheyakirMode = newMode;
}

void COptionsDlg::ConvertSofZmanMode(int newMode)
{
    int oldMode = m_lastSofZmanMode;
    double oldValue = GetEditValue(m_editCustomSofZman, oldMode == 0 ? 0.0 : 72.0);
    SetSofZmanMode(newMode);
    SetEditValue(m_editCustomSofZman, ConvertSofZmanValue(oldMode, newMode, oldValue));
    m_lastSofZmanMode = newMode;
}

void COptionsDlg::ConvertTzeitMode(int newMode)
{
    int oldMode = m_lastTzeitMode;
    double oldValue = GetEditValue(m_editCustomTzeit, oldMode == 0 ? 8.5 : 42.0);
    SetTzeitMode(newMode);
    SetEditValue(m_editCustomTzeit, ConvertTzeitValue(oldMode, newMode, oldValue));
    m_lastTzeitMode = newMode;
}

static bool BackupCurrentSettingsFromOptions(CWnd* owner)
{
    wchar_t file[MAX_PATH] = L"WinLuach_settings_backup.json";
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = owner ? owner->GetSafeHwnd() : nullptr;
    ofn.lpstrFilter = L"JSON files\0*.json\0All files\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"json";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&ofn))
        return false;

    std::wstring src = GetSettingsFilePath();
    if (CopyFileW(src.c_str(), file, FALSE))
    {
        if (owner)
            owner->MessageBox(L"Settings backed up successfully.", L"WinLuach", MB_OK | MB_ICONINFORMATION);
        return true;
    }

    if (owner)
        owner->MessageBox(L"Backup failed. The reset was not performed.", L"WinLuach", MB_OK | MB_ICONERROR);
    return false;
}

void COptionsDlg::OnResetCalendarSettings()
{
    int choice = MessageBox(
        L"This will reset all calendar settings to their defaults and clear the board.\n\n"
        L"Back up all current settings before continuing?",
        L"Reset Calendar Settings",
        MB_YESNOCANCEL | MB_ICONWARNING | MB_DEFBUTTON1);

    if (choice == IDCANCEL)
        return;
    if (choice == IDYES && !BackupCurrentSettingsFromOptions(this))
        return;

    if (MessageBox(L"Reset all calendar settings now?",
        L"Reset Calendar Settings",
        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
        return;

    AppSettings defaults;
    CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
    if (frame)
        frame->ApplyAndSaveSettings(defaults);
    else
        SaveSettings(defaults);

    m_current = defaults;
    m_result = defaults;
    SetDirty(false);
    MessageBox(L"Calendar settings have been reset.", L"WinLuach", MB_OK | MB_ICONINFORMATION);

    if (m_modeless)
        DestroyWindow();
    else
        CDialog::OnOK();
}

BOOL COptionsDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    UINT id = LOWORD(wParam);
    UINT code = HIWORD(wParam);
    if (id == IDC_OPT_RESET_CALENDAR && code == BN_CLICKED)
    {
        OnResetCalendarSettings();
        return TRUE;
    }
    if (id == IDC_OPT_COLOR_RESTORE)
    {
        RestoreDefaultCalendarColors();
        SetDirty(true);
        return TRUE;
    }
    if (id == IDC_OPT_COLOR_PICKER)
    {
        if (ChooseCalendarColor(id))
        {
            SetDirty(true);
            return TRUE;
        }
    }
    if (id == IDC_OPT_COLOR_ITEM && code == CBN_SELCHANGE)
    {
        UpdateColorEditorFromSelection();
        SetDirty(true);
        return TRUE;
    }
    if (id == IDC_OPT_COLOR_PREVIEW && code == CBN_SELCHANGE)
    {
        InvalidateColorPreview();
        return TRUE;
    }
    if (id == IDC_OPT_COLOR_HEX && code == EN_CHANGE)
    {
        ApplyColorHexFromEditor();
        SetDirty(true);
        return TRUE;
    }
    if (id == IDC_OPT_ALOT_SHITA && code == CBN_SELCHANGE)
    {
        ApplyAlotPreset();
        SetDirty(true);
        return TRUE;
    }
    if (id == IDC_OPT_MISHEYAKIR_SHITA && code == CBN_SELCHANGE)
    {
        ApplyMisheyakirPreset();
        SetDirty(true);
        return TRUE;
    }
    if (id == IDC_OPT_SOFZMAN_SHITA && code == CBN_SELCHANGE)
    {
        ApplySofZmanPreset();
        SetDirty(true);
        return TRUE;
    }
    if (id == IDC_OPT_TZEIT_SHITA && code == CBN_SELCHANGE)
    {
        ApplyTzeitPreset();
        SetDirty(true);
        return TRUE;
    }
    if (id == IDC_OPT_NOTIFY_ZMAN_STYLE && code == CBN_SELCHANGE)
    {
        UpdateNotificationControls();
        SetDirty(true);
        return TRUE;
    }
    if (code == BN_CLICKED &&
        (id == IDC_OPT_NOTIFY_SEFIRAH_MODE ||
         id == IDC_OPT_NOTIFY_SEFIRAH_MODE + 1 ||
         id == IDC_OPT_NOTIFY_SEFIRAH_MODE + 2))
    {
        UpdateSefirahControls();
        SetDirty(true);
        return TRUE;
    }
    if (code == BN_CLICKED && (id == IDC_OPT_RAD_WINNOTIFY || id == IDC_OPT_RAD_WINLUACHTOAST))
    {
        bool wlToast = (id == IDC_OPT_RAD_WINLUACHTOAST);
        if (m_editToastDuration.GetSafeHwnd()) m_editToastDuration.EnableWindow(wlToast ? TRUE : FALSE);
        if (m_cmbToastDurationUnit.GetSafeHwnd()) m_cmbToastDurationUnit.EnableWindow(wlToast ? TRUE : FALSE);
        SetDirty(true);
        return TRUE;
    }
    if (id == IDC_OPT_TRAY_BACK_ENABLED && code == BN_CLICKED)
    {
        if (m_btnTrayBackColor.GetSafeHwnd())
            m_btnTrayBackColor.EnableWindow(m_chkTrayBackEnabled.GetCheck() == BST_CHECKED ? TRUE : FALSE);
        SetDirty(true);
        return TRUE;
    }
    auto setPresetRadio = [](std::vector<CButton*>& radios, int sel) {
        for (int i = 0; i < (int)radios.size(); ++i)
            if (radios[i] && radios[i]->GetSafeHwnd())
                radios[i]->SetCheck(i == sel ? BST_CHECKED : BST_UNCHECKED);
    };
    auto selectCustomPresetForUnit = [&]() {
        int sub = m_zmanimSubTab.GetSafeHwnd() ? m_zmanimSubTab.GetCurSel() : -1;
        if (id == IDC_OPT_ALOT_DEG || id == IDC_OPT_ALOT_MIN || id == IDC_OPT_ALOT_ZMANIS)
            setPresetRadio(m_radAlosPreset, 3);
        else if (id == IDC_OPT_MISHEYAKIR_DEG || id == IDC_OPT_MISHEYAKIR_MIN || id == IDC_OPT_MISHEYAKIR_ZMANIS)
            setPresetRadio(m_radMisheyakirPreset, 3);
        else if (id == IDC_OPT_SOFZMAN_DEG || id == IDC_OPT_SOFZMAN_MIN || id == IDC_OPT_SOFZMAN_ZMANIS)
        {
            if (sub == 2) setPresetRadio(m_radSofMAPreset, 3);
            else if (sub == 3) setPresetRadio(m_radSofGRAPreset, 3);
        }
        else if (id == IDC_OPT_TZEIT_DEG || id == IDC_OPT_TZEIT_MIN || id == IDC_OPT_TZEIT_ZMANIS)
            setPresetRadio(m_radTzaisPreset, 5);
    };
    auto isShaahCustomControl = [](UINT ctlId) {
        return ctlId == IDC_OPT_SHAAH_START_DEG_RAD ||
               ctlId == IDC_OPT_SHAAH_START_FIXED_RAD ||
               ctlId == IDC_OPT_SHAAH_START_DEG_VAL ||
               ctlId == IDC_OPT_SHAAH_START_VALUE ||
               ctlId == IDC_OPT_SHAAH_END_DEG_RAD ||
               ctlId == IDC_OPT_SHAAH_END_FIXED_RAD ||
               ctlId == IDC_OPT_SHAAH_END_DEG_VAL ||
               ctlId == IDC_OPT_SHAAH_END_VALUE;
    };
    if (m_initialized && (code == BN_CLICKED || code == EN_CHANGE) &&
        isShaahCustomControl(id))
    {
        if (id == IDC_OPT_SHAAH_START_DEG_RAD || id == IDC_OPT_SHAAH_START_FIXED_RAD)
        {
            m_radShaahStartDegrees.SetCheck(id == IDC_OPT_SHAAH_START_DEG_RAD ? BST_CHECKED : BST_UNCHECKED);
            m_radShaahStartFixed.SetCheck(id == IDC_OPT_SHAAH_START_FIXED_RAD ? BST_CHECKED : BST_UNCHECKED);
        }
        if (id == IDC_OPT_SHAAH_END_DEG_RAD || id == IDC_OPT_SHAAH_END_FIXED_RAD)
        {
            m_radShaahEndDegrees.SetCheck(id == IDC_OPT_SHAAH_END_DEG_RAD ? BST_CHECKED : BST_UNCHECKED);
            m_radShaahEndFixed.SetCheck(id == IDC_OPT_SHAAH_END_FIXED_RAD ? BST_CHECKED : BST_UNCHECKED);
        }
        setPresetRadio(m_radShaahZmanitPreset, kShaahZmanitCustomUiIndex);
        SetDirty(true);
        return TRUE;
    }
    if (code == BN_CLICKED &&
        (id == IDC_OPT_SOF_SHA_USE_DISPLAYED || id == IDC_OPT_SOF_SHA_USE_BOUNDARY))
    {
        m_radSofShaahDisplayed.SetCheck(id == IDC_OPT_SOF_SHA_USE_DISPLAYED ? BST_CHECKED : BST_UNCHECKED);
        m_radSofShaahBoundary.SetCheck(id == IDC_OPT_SOF_SHA_USE_BOUNDARY ? BST_CHECKED : BST_UNCHECKED);
        SetDirty(true);
        return TRUE;
    }
    if (id == IDC_OPT_ALOT_DEG)    { selectCustomPresetForUnit(); ConvertAlotMode(0); SetDirty(true); return TRUE; }
    if (id == IDC_OPT_ALOT_MIN)    { selectCustomPresetForUnit(); ConvertAlotMode(1); SetDirty(true); return TRUE; }
    if (id == IDC_OPT_ALOT_ZMANIS) { selectCustomPresetForUnit(); ConvertAlotMode(2); SetDirty(true); return TRUE; }
    if (id == IDC_OPT_MISHEYAKIR_DEG)    { selectCustomPresetForUnit(); ConvertMisheyakirMode(0); SetDirty(true); return TRUE; }
    if (id == IDC_OPT_MISHEYAKIR_MIN)    { selectCustomPresetForUnit(); ConvertMisheyakirMode(1); SetDirty(true); return TRUE; }
    if (id == IDC_OPT_MISHEYAKIR_ZMANIS) { selectCustomPresetForUnit(); ConvertMisheyakirMode(2); SetDirty(true); return TRUE; }
    if (id == IDC_OPT_SOFZMAN_DEG)    { selectCustomPresetForUnit(); ConvertSofZmanMode(0); SetDirty(true); return TRUE; }
    if (id == IDC_OPT_SOFZMAN_MIN)    { selectCustomPresetForUnit(); ConvertSofZmanMode(1); SetDirty(true); return TRUE; }
    if (id == IDC_OPT_SOFZMAN_ZMANIS) { selectCustomPresetForUnit(); ConvertSofZmanMode(2); SetDirty(true); return TRUE; }
    if (id == IDC_OPT_TZEIT_DEG)    { selectCustomPresetForUnit(); ConvertTzeitMode(0); SetDirty(true); return TRUE; }
    if (id == IDC_OPT_TZEIT_MIN)    { selectCustomPresetForUnit(); ConvertTzeitMode(1); SetDirty(true); return TRUE; }
    if (id == IDC_OPT_TZEIT_ZMANIS) { selectCustomPresetForUnit(); ConvertTzeitMode(2); SetDirty(true); return TRUE; }
    if (code == BN_CLICKED &&
        (id == IDC_OPT_RAD_GRA || id == IDC_OPT_RAD_MA72 || id == IDC_OPT_RAD_MA90))
    {
        m_radGRA.SetCheck(id == IDC_OPT_RAD_GRA ? BST_CHECKED : BST_UNCHECKED);
        m_radMA72.SetCheck(id == IDC_OPT_RAD_MA72 ? BST_CHECKED : BST_UNCHECKED);
        m_radMA90.SetCheck(id == IDC_OPT_RAD_MA90 ? BST_CHECKED : BST_UNCHECKED);
        SetDirty(true);
        return TRUE;
    }
    if (code == BN_CLICKED && id >= IDC_OPT_ZSUB_FIRST && id <= IDC_OPT_ZSUB_LAST)
    {
        auto enforceRadioGroup = [id](const std::vector<CButton*>& radios) -> int {
            int clicked = -1;
            for (int i = 0; i < (int)radios.size(); ++i)
            {
                CButton* r = radios[i];
                if (r && r->GetSafeHwnd() && r->GetDlgCtrlID() == (int)id)
                {
                    clicked = i;
                    break;
                }
            }
            if (clicked < 0)
                return -1;
            for (int i = 0; i < (int)radios.size(); ++i)
            {
                CButton* r = radios[i];
                if (r && r->GetSafeHwnd())
                    r->SetCheck(i == clicked ? BST_CHECKED : BST_UNCHECKED);
            }
            return clicked;
        };
        int picked = -1;
        if ((picked = enforceRadioGroup(m_radAlosPreset)) >= 0)
        {
            if (picked == 3) SetAlotMode(m_lastAlotMode);
            SetDirty(true);
            return TRUE;
        }
        if ((picked = enforceRadioGroup(m_radMisheyakirPreset)) >= 0)
        {
            if (picked == 3) SetMisheyakirMode(m_lastMisheyakirMode);
            SetDirty(true);
            return TRUE;
        }
        if ((picked = enforceRadioGroup(m_radSofMAPreset)) >= 0)
        {
            if (picked == 3) SetSofZmanMode(m_lastSofZmanMode);
            SetDirty(true);
            return TRUE;
        }
        if ((picked = enforceRadioGroup(m_radSofGRAPreset)) >= 0)
        {
            if (picked == 3) SetSofZmanMode(m_lastSofZmanMode);
            SetDirty(true);
            return TRUE;
        }
        if ((picked = enforceRadioGroup(m_radTzaisPreset)) >= 0)
        {
            if (picked == 5) SetTzeitMode(m_lastTzeitMode);
            SetDirty(true);
            return TRUE;
        }
        if (enforceRadioGroup(m_radShaahZmanitPreset) >= 0)
        {
            SetDirty(true);
            return TRUE;
        }
        if (enforceRadioGroup(m_radMinchaGedolaPreset) >= 0 ||
            enforceRadioGroup(m_radMinchaKetanaPreset) >= 0 ||
            enforceRadioGroup(m_radPlagPreset) >= 0 ||
            enforceRadioGroup(m_radEndFastPreset) >= 0)
        {
            SetDirty(true);
            return TRUE;
        }
    }
    if (m_initialized && !m_updatingColorUi &&
        (code == BN_CLICKED || code == CBN_SELCHANGE || code == EN_CHANGE))
        SetDirty(true);
    return CDialog::OnCommand(wParam, lParam);
}

// v0.8.78 — Enable/disable the "Show chatzos on BeHaB" checkbox based on
// whether the "Show BeHaB days" master checkbox is currently checked.
void COptionsDlg::OnShowBeHaBChanged()
{
    if (m_chkShowBeHaB.GetSafeHwnd() && m_chkChatzosOnBeHaB.GetSafeHwnd())
    {
        bool behabOn = (m_chkShowBeHaB.GetCheck() == BST_CHECKED);
        m_chkChatzosOnBeHaB.EnableWindow(behabOn ? TRUE : FALSE);
        if (!behabOn)
            m_chkChatzosOnBeHaB.SetCheck(BST_UNCHECKED);
    }
    SetDirty(true);
}

// v0.8.82 — OnTrayTipZmanClicked: counts all checked tray tooltip zmanim and
// enforces a maximum of 5. If max is reached, disables all unchecked items and
// shows the warning label; re-enables them and hides the label when below max.
void COptionsDlg::OnTrayTipZmanClicked(UINT /*nID*/)
{
    // Count all currently checked items across both groups
    int checked = 0;
    for (int i = 0; i < kTrayTooltipZmanCount; ++i)
        if (m_chkTrayTooltipZmanim[i].GetSafeHwnd() &&
            m_chkTrayTooltipZmanim[i].GetCheck() == BST_CHECKED)
            ++checked;
    for (int i = 0; i < kTrayTooltipCustomZmanCount; ++i)
        if (m_chkTrayTooltipCustomZmanim[i].GetSafeHwnd() &&
            m_chkTrayTooltipCustomZmanim[i].GetCheck() == BST_CHECKED)
            ++checked;

    constexpr int kMaxTrayZmanim = 5;
    bool atMax = (checked >= kMaxTrayZmanim);

    // Show or hide the warning label
    if (m_statTrayTipMaxMsg.GetSafeHwnd())
        m_statTrayTipMaxMsg.ShowWindow(atMax ? SW_SHOW : SW_HIDE);

    // Enable/disable unchecked items
    for (int i = 0; i < kTrayTooltipZmanCount; ++i)
    {
        if (!m_chkTrayTooltipZmanim[i].GetSafeHwnd()) continue;
        if (m_chkTrayTooltipZmanim[i].GetCheck() != BST_CHECKED)
            m_chkTrayTooltipZmanim[i].EnableWindow(!atMax);
    }
    for (int i = 0; i < kTrayTooltipCustomZmanCount; ++i)
    {
        if (!m_chkTrayTooltipCustomZmanim[i].GetSafeHwnd()) continue;
        if (m_chkTrayTooltipCustomZmanim[i].GetCheck() != BST_CHECKED)
            m_chkTrayTooltipCustomZmanim[i].EnableWindow(!atMax);
    }
}

void COptionsDlg::UpdateNotificationControls()
{
    bool enableZmanim = m_cmbNotifyZmanim.GetSafeHwnd() && m_cmbNotifyZmanim.GetCurSel() > 0;
    for (auto& chk : m_chkNotifyZmanim)
        if (chk.GetSafeHwnd())
            chk.EnableWindow(enableZmanim ? TRUE : FALSE);
    UpdateSefirahControls();
}

void COptionsDlg::UpdateSefirahControls()
{
    if (!m_radNotifySefirahSunTzais.GetSafeHwnd()) return;
    bool isSunTzais = m_radNotifySefirahSunTzais.GetCheck() == BST_CHECKED;
    bool isOther    = m_radNotifySefirahOther.GetCheck()    == BST_CHECKED;
    bool isManual   = m_radNotifySefirahManual.GetCheck()   == BST_CHECKED;
    bool isZman     = isSunTzais || isOther;
    m_cmbNotifySefirahBase.EnableWindow(isSunTzais ? TRUE : FALSE);
    m_cmbNotifySefirahOtherZman.EnableWindow(isOther ? TRUE : FALSE);
    m_cmbNotifySefirahHour.EnableWindow(isManual ? TRUE : FALSE);
    m_cmbNotifySefirahMinute.EnableWindow(isManual ? TRUE : FALSE);
    m_cmbNotifySefirahAmPm.EnableWindow(isManual ? TRUE : FALSE);
    m_editNotifySefirahOffset.EnableWindow(isZman ? TRUE : FALSE);
    m_cmbNotifySefirahDir.EnableWindow(isZman ? TRUE : FALSE);
}

void COptionsDlg::ReadControlsIntoResult()
{
    m_result.use24Hour = (m_rad24hr.GetCheck() == BST_CHECKED);
    m_result.defaultHebrewMonth = (m_radHebrewMonth.GetCheck() == BST_CHECKED);
    m_result.dateTracking = (m_chkDateTracking.GetCheck() == BST_CHECKED);
    m_result.isIsrael = (m_radIsrael.GetCheck() == BST_CHECKED);
    m_result.showParshios = (m_chkParshios.GetCheck() == BST_CHECKED);
    m_result.showMoadim = (m_chkMoadim.GetCheck() == BST_CHECKED);
    m_result.showUserEvents = (m_chkUserEvents.GetCheck() == BST_CHECKED);
    m_result.showDafYomi = (m_chkDafYomi.GetCheck() == BST_CHECKED);
    m_result.showYerushalmi = (m_chkYerushalmi.GetCheck() == BST_CHECKED);
    m_result.showHalachaYomit = (m_chkHalacha.GetCheck() == BST_CHECKED);
    m_result.showMishnaYomit = (m_chkMishna.GetCheck() == BST_CHECKED);
    m_result.showTanachYomi = (m_chkTanach.GetCheck() == BST_CHECKED);
    m_result.haftarahShita = m_current.haftarahShita;
    m_result.fontSize = max(0, min(6, m_cmbFontSize.GetCurSel()));
    m_result.language = 0;
    m_result.showTrayIcon   = (m_chkShowTrayIcon.GetCheck()   == BST_CHECKED);
    m_result.minimizeToTray = (m_chkMinimizeToTray.GetCheck() == BST_CHECKED);
    m_result.minimizeTrayWhen = m_cmbTrayWhen.GetCurSel();
    m_result.minimizeOnStartup = (m_chkMinimizeOnStartup.GetCheck() == BST_CHECKED);
    m_result.startWithWindows = (m_chkStartWithWindows.GetCheck() == BST_CHECKED);
    m_result.desktopShortcut = (m_chkDesktopShortcut.GetCheck() == BST_CHECKED);
    m_result.trayBackEnabled = (m_chkTrayBackEnabled.GetCheck() == BST_CHECKED);
    m_result.trayNumberStyle = max(0, min(1, m_cmbTrayNumber.GetCurSel()));
    m_result.trayTooltipZmanimMask = 0;
    for (int i = 0; i < kTrayTooltipZmanCount; ++i)
        if (m_chkTrayTooltipZmanim[i].GetSafeHwnd() &&
            m_chkTrayTooltipZmanim[i].GetCheck() == BST_CHECKED)
            m_result.trayTooltipZmanimMask |= (1u << i);
    m_result.trayTooltipCustomZmanimMask = 0;
    for (int i = 0; i < kTrayTooltipCustomZmanCount; ++i)
        if (m_chkTrayTooltipCustomZmanim[i].GetSafeHwnd() &&
            m_chkTrayTooltipCustomZmanim[i].GetCheck() == BST_CHECKED)
            m_result.trayTooltipCustomZmanimMask |= (1u << i);
    m_result.yearDetailsMask = 0;
    for (int i = 0; i < kYearDetailCount; ++i)
        if (m_chkYearDetails[i].GetSafeHwnd() &&
            m_chkYearDetails[i].GetCheck() == BST_CHECKED)
            m_result.yearDetailsMask |= (uint16_t)(1u << i);
    if (m_radMA72.GetCheck() == BST_CHECKED) m_result.zmanimShita = 1;
    else if (m_radMA90.GetCheck() == BST_CHECKED) m_result.zmanimShita = 2;
    else                                           m_result.zmanimShita = 0;

    m_result.alotShita        = max(0, min(2, m_cmbAlotShita.GetCurSel()));
    {
        int tzeitSel = m_cmbTzeitShita.GetCurSel();
        m_result.tzeitShita = (tzeitSel >= 0 && tzeitSel <= 4)
            ? tzeitSel
            : max(0, min(4, m_current.tzeitShita));
    }
    m_result.showChatzosOnFasts = (m_chkChatzosOnFasts.GetCheck() == BST_CHECKED);
    if (m_chkShowBeHaB.GetSafeHwnd())
    {
        m_result.showBeHaB          = (m_chkShowBeHaB.GetCheck()      == BST_CHECKED);
        m_result.showChatzosOnBeHaB = (m_chkChatzosOnBeHaB.GetCheck() == BST_CHECKED);
    }
    // v0.8.82 — read from new degrees checkbox + minutes mode dropdown
    if (m_chkAlotCustomDeg.GetSafeHwnd() && m_chkAlotCustomDeg.GetCheck() == BST_CHECKED)
        m_result.customAlotMode = 0;  // degrees
    else
        m_result.customAlotMode = (m_cmbAlotMinutesMode.GetSafeHwnd() && m_cmbAlotMinutesMode.GetCurSel() == 1) ? 2 : 1;
    m_result.customAlotDegreesValue = GetEditValue(m_editCustomAlotDeg, 16.1);
    m_result.customAlotValue = GetEditValue(m_editCustomAlot, 72.0);

    if (m_chkMishCustomDeg.GetSafeHwnd() && m_chkMishCustomDeg.GetCheck() == BST_CHECKED)
        m_result.customMisheyakirMode = 0;
    else
        m_result.customMisheyakirMode = (m_cmbMishMinutesMode.GetSafeHwnd() && m_cmbMishMinutesMode.GetCurSel() == 1) ? 2 : 1;
    m_result.customMisheyakirDegreesValue = GetEditValue(m_editCustomMishDeg, 10.2);
    m_result.customMisheyakirValue = GetEditValue(m_editCustomMisheyakir, 50.0);

    if (m_chkSofCustomDeg.GetSafeHwnd() && m_chkSofCustomDeg.GetCheck() == BST_CHECKED)
        m_result.customSofZmanMode = 0;
    else
        m_result.customSofZmanMode = (m_cmbSofMinutesMode.GetSafeHwnd() && m_cmbSofMinutesMode.GetCurSel() == 1) ? 2 : 1;
    m_result.customSofZmanDegreesValue = GetEditValue(m_editCustomSofDeg, 16.1);
    m_result.customSofZmanValue = GetEditValue(m_editCustomSofZman, 72.0);

    if (m_chkTzeitCustomDeg.GetSafeHwnd() && m_chkTzeitCustomDeg.GetCheck() == BST_CHECKED)
        m_result.customTzeitMode = 0;
    else
        m_result.customTzeitMode = (m_cmbTzeitMinutesMode.GetSafeHwnd() && m_cmbTzeitMinutesMode.GetCurSel() == 1) ? 2 : 1;
    m_result.customTzeitDegreesValue = GetEditValue(m_editCustomTzeitDeg, 8.5);
    m_result.customTzeitValue = GetEditValue(m_editCustomTzeit, 42.0);

    m_result.sofZmanShaahMode =
        (m_radSofShaahBoundary.GetSafeHwnd() &&
         m_radSofShaahBoundary.GetCheck() == BST_CHECKED) ? 1 : 0;

    if (m_radShaahStartDegrees.GetSafeHwnd() &&
        m_radShaahStartDegrees.GetCheck() == BST_CHECKED)
        m_result.customShaahStartMode = 0;
    else
        m_result.customShaahStartMode = 1;
    m_result.customShaahStartDegreesValue = GetEditValue(m_editCustomShaahStartDeg, 16.1);
    m_result.customShaahStartValue = GetEditValue(m_editCustomShaahStart, 72.0);

    if (m_radShaahEndDegrees.GetSafeHwnd() &&
        m_radShaahEndDegrees.GetCheck() == BST_CHECKED)
        m_result.customShaahEndMode = 0;
    else
        m_result.customShaahEndMode = 1;
    m_result.customShaahEndDegreesValue = GetEditValue(m_editCustomShaahEndDeg, 8.5);
    m_result.customShaahEndValue = GetEditValue(m_editCustomShaahEnd, 72.0);

    CString customValue;

    CString candle;
    m_cmbCandleMinutes.GetWindowText(candle);
    m_result.candleLightingMinutes = _wtoi(candle);

    m_result.notifyPersonalEvents = max(0, m_cmbNotifyPersonal.GetCurSel());
    m_result.notifyWebCalEvents   = max(0, m_cmbNotifyWebCal.GetCurSel());
    m_result.notifySefirahStyle = max(0, m_cmbNotifySefirah.GetCurSel());
    m_result.notifyZmanimStyle = max(0, m_cmbNotifyZmanim.GetCurSel());
    m_result.notifyZmanimMask = 0;
    for (int i = 0; i < kZmanCheckboxCount; ++i)
        if (m_chkNotifyZmanim[i].GetSafeHwnd() &&
            m_chkNotifyZmanim[i].GetCheck() == BST_CHECKED)
            m_result.notifyZmanimMask |= (1u << i);
    m_result.notifyMoadimStyle = max(0, m_cmbNotifyMoadim.GetCurSel());
    CString txt;
    m_result.notifySefirahTime = ReadClockCombos(m_cmbNotifySefirahHour, m_cmbNotifySefirahMinute, m_cmbNotifySefirahAmPm);
    if (m_radNotifySefirahManual.GetCheck() == BST_CHECKED)
        m_result.notifySefirahMode = 0;
    else
        m_result.notifySefirahMode = 1;
    m_editNotifySefirahOffset.GetWindowText(txt);
    m_result.notifySefirahOffsetMinutes = max(0, _wtoi(txt));
    m_result.notifySefirahOffsetDir = max(0, min(1, m_cmbNotifySefirahDir.GetCurSel()));
    m_result.notifySefirahBase = (m_radNotifySefirahOther.GetCheck() == BST_CHECKED)
        ? 2 : max(0, min(1, m_cmbNotifySefirahBase.GetCurSel()));
    m_result.notifySefirahOtherZman = max(0, min(kZmanCheckboxCount - 1, m_cmbNotifySefirahOtherZman.GetCurSel()));
    m_result.notifyMoadimOffsets = ReadOffsetControls(m_editNotifyMoadimAmount, m_cmbNotifyMoadimUnit);
    m_result.notifyParshaStyle = max(0, m_cmbNotifyParshaStyle.GetCurSel());
    int parSel = m_cmbNotifyParsha.GetCurSel();
    if (parSel > 0)
    {
        CString parsha;
        m_cmbNotifyParsha.GetLBText(parSel, parsha);
        m_result.notifyParshaName = (LPCWSTR)parsha;
    }
    else
    {
        m_result.notifyParshaName.clear();
    }
    m_result.notifyParshaOffsets = ReadOffsetControls(m_editNotifyParshaAmount, m_cmbNotifyParshaUnit);
    m_result.notifyPersonalOffsets = ReadOffsetControls(m_editNotifyPersonalAmount, m_cmbNotifyPersonalUnit);
    m_result.useWinLuachToast = (m_radUseWinLuachToast.GetSafeHwnd() &&
                                  m_radUseWinLuachToast.GetCheck() == BST_CHECKED);
    { CString dur; m_editToastDuration.GetWindowText(dur);
      m_result.winLuachToastDuration = max(1, _wtoi(dur)); }
    m_result.winLuachToastDurationUnit = max(0, min(4, m_cmbToastDurationUnit.GetSafeHwnd() ?
        m_cmbToastDurationUnit.GetCurSel() : 0));
    // Daily reminder time (v0.8.71) — read H:MM AM/PM string and convert to hour24 + minute
    if (m_cmbReminderDailyHour.GetSafeHwnd())
    {
        std::wstring ts = ReadClockCombos(m_cmbReminderDailyHour, m_cmbReminderDailyMin, m_cmbReminderDailyAmpm);
        int h24 = 9, min0 = 0;
        ParseClockText(ts, h24, min0);
        m_result.reminderDailyHour   = max(0, min(23, h24));
        m_result.reminderDailyMinute = max(0, min(59, min0));
    }

    // v0.8.0 - Zmanim bar mask: one bit per checkbox.
    uint32_t mask = 0;
    for (int i = 0; i < (int)m_zmanimBarChecks.size() && i < 32; ++i)
        if (m_zmanimBarChecks[i] && m_zmanimBarChecks[i]->GetSafeHwnd() &&
            m_zmanimBarChecks[i]->GetCheck() == BST_CHECKED)
            mask |= (1u << i);
    m_result.zmanimBarMask = mask;

    // v0.8.0 - per-sub-tab preset (which radio is checked).  -1 fallback
    // means the page wasn't created; keep the prior value in that case.
    auto pickedIndex = [](const std::vector<CButton*>& radios) -> int {
        for (int i = 0; i < (int)radios.size(); ++i)
            if (radios[i] && radios[i]->GetSafeHwnd() &&
                radios[i]->GetCheck() == BST_CHECKED)
                return i;
        return -1;
    };
    int v;
    if ((v = pickedIndex(m_radAlosPreset)) >= 0)
    {
        m_result.customAlotPreset = v;
        if (v == 0) { m_result.alotShita = 0; m_result.customAlotMode = 0; m_result.customAlotValue = 16.1; }
        else if (v == 1) { m_result.alotShita = 1; m_result.customAlotMode = 1; m_result.customAlotValue = 72.0; }
        else if (v == 2) { m_result.alotShita = 2; m_result.customAlotMode = 1; m_result.customAlotValue = 90.0; }
    }
    if ((v = pickedIndex(m_radMisheyakirPreset)) >= 0)
    {
        m_result.customMisheyakirPreset = v;
        if (v == 0) { m_result.customMisheyakirMode = 0; m_result.customMisheyakirValue = 11.5; }
        else if (v == 1) { m_result.customMisheyakirMode = 0; m_result.customMisheyakirValue = 11.0; }
        else if (v == 2) { m_result.customMisheyakirMode = 0; m_result.customMisheyakirValue = 10.2; }
    }
    int sofMaPreset = pickedIndex(m_radSofMAPreset);
    int sofGraPreset = pickedIndex(m_radSofGRAPreset);
    if (sofMaPreset >= 0) m_result.customSofZmanMaPreset = sofMaPreset;
    if (sofGraPreset >= 0) m_result.customSofZmanGraPreset = sofGraPreset;
    v = (m_result.zmanimShita == 0) ? sofGraPreset : sofMaPreset;
    if (v >= 0)
    {
        if (v == 0) { m_result.customSofZmanMode = 0; m_result.customSofZmanValue = 19.8; }
        else if (v == 1) { m_result.customSofZmanMode = 1; m_result.customSofZmanValue = 72.0; }
        else if (v == 2) { m_result.customSofZmanMode = 0; m_result.customSofZmanValue = 16.1; }
    }
    if ((v = pickedIndex(m_radMinchaGedolaPreset)) >= 0) m_result.customMinchaGedolaPreset = v;
    if ((v = pickedIndex(m_radMinchaKetanaPreset)) >= 0) m_result.customMinchaKetanaPreset = v;
    if ((v = pickedIndex(m_radPlagPreset))         >= 0) m_result.customPlagPreset         = v;
    if ((v = pickedIndex(m_radEndFastPreset))      >= 0) m_result.customEndFastPreset      = v;
    if (m_editCustomMinchaGedola.GetSafeHwnd())
    {
        m_editCustomMinchaGedola.GetWindowText(customValue);
        m_result.customMinchaGedolaValue = max(0.0, _wtof(customValue));
    }
    if (m_editCustomMinchaKetana.GetSafeHwnd())
    {
        m_editCustomMinchaKetana.GetWindowText(customValue);
        m_result.customMinchaKetanaValue = max(0.0, _wtof(customValue));
    }
    if (m_editCustomPlag.GetSafeHwnd())
    {
        m_editCustomPlag.GetWindowText(customValue);
        m_result.customPlagValue = max(0.0, _wtof(customValue));
    }
    if (m_editCustomEndFast.GetSafeHwnd())
    {
        m_editCustomEndFast.GetWindowText(customValue);
        m_result.customEndFastValue = max(0.0, _wtof(customValue));
    }
    if (m_cmbEndFastMinuteMode.GetSafeHwnd())
        m_result.customEndFastMinuteMode = max(0, min(1, m_cmbEndFastMinuteMode.GetCurSel()));
    if ((v = pickedIndex(m_radTzaisPreset)) >= 0)
    {
        m_result.customTzeitPreset = v;
        if (v == 0) { m_result.customTzeitMode = 1; m_result.customTzeitValue = 42.0; }
        else if (v == 1) { m_result.customTzeitMode = 1; m_result.customTzeitValue = 50.0; }
        else if (v == 2) { m_result.customTzeitMode = 1; m_result.customTzeitValue = 60.0; }
        else if (v == 3) { m_result.tzeitShita = 1; m_result.customTzeitMode = 1; m_result.customTzeitValue = 72.0; }
        else if (v == 4) { m_result.customTzeitMode = 0; m_result.customTzeitValue = 16.1; }
    }

    // Sha'a Zmanit independent shita. UI order differs from persisted values
    // so the custom setting can keep its original value (3).
    if ((v = pickedIndex(m_radShaahZmanitPreset)) >= 0)
        m_result.shaahZmanitShita = (v < kShaahZmanitPresetCount)
            ? kShaahZmanitPresetSettingByUi[v]
            : 0;

    // v0.8.54 - Auto-update settings
    m_result.disableAutoUpdate = (m_chkDisableAutoUpdate.GetCheck() == BST_CHECKED);
    m_result.checkUpdatesAuto = (m_chkCheckUpdatesAuto.GetCheck() == BST_CHECKED);
    m_result.updateCheckFrequency = max(0, min(2, m_cmbUpdateFrequency.GetCurSel()));
    m_result.checkUpdatesOnLaunch = (m_chkCheckOnLaunch.GetCheck() == BST_CHECKED);
}

bool COptionsDlg::ApplyToParent()
{
    ReadControlsIntoResult();
    CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
    if (!frame)
        return false;
    frame->ApplyAndSaveSettings(m_result);
    m_current = m_result;
    SetDirty(false);
    return true;
}

void COptionsDlg::OnApply()
{
    ApplyToParent();
}

void COptionsDlg::OnOK()
{
    if (m_modeless)
    {
        ApplyToParent();
        DestroyWindow();
    }
    else
    {
        ReadControlsIntoResult();
        CDialog::OnOK();
    }
}

void COptionsDlg::OnCancel()
{
    if (m_modeless)
        DestroyWindow();
    else
        CDialog::OnCancel();
}

void COptionsDlg::PostNcDestroy()
{
    if (m_modeless)
    {
        CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
        if (frame)
            frame->OnOptionsDialogClosed(this);
        delete this;
        return;
    }
    CDialog::PostNcDestroy();
}

void COptionsDlg::OnTrayTextColor()
{
    CColorDialog dlg((COLORREF)m_result.trayTextColor, CC_FULLOPEN, this);
    if (dlg.DoModal() == IDOK)
    {
        m_result.trayTextColor = (int)dlg.GetColor();
        SetDirty(true);
    }
}

void COptionsDlg::OnTrayBackColor()
{
    CColorDialog dlg((COLORREF)m_result.trayBackColor, CC_FULLOPEN, this);
    if (dlg.DoModal() == IDOK)
    {
        m_result.trayBackColor = (int)dlg.GetColor();
        SetDirty(true);
    }
}

void COptionsDlg::OnTrayFont()
{
    LOGFONT lf = {};
    wcscpy_s(lf.lfFaceName, m_result.trayFontFace.empty() ? L"Arial" : m_result.trayFontFace.c_str());
    HDC screen = ::GetDC(nullptr);
    int dpiY = screen ? GetDeviceCaps(screen, LOGPIXELSY) : 96;
    if (screen) ::ReleaseDC(nullptr, screen);
    int pointSize = m_result.trayFontSize > 0 ? m_result.trayFontSize : 12;
    lf.lfHeight = -MulDiv(max(6, pointSize), dpiY, 72);
    lf.lfWeight = m_result.trayFontBold ? FW_BOLD : FW_NORMAL;
    lf.lfItalic = m_result.trayFontItalic ? TRUE : FALSE;

    CFontDialog dlg(&lf, CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT, nullptr, this);
    dlg.m_cf.rgbColors = (COLORREF)m_result.trayTextColor;
    if (dlg.DoModal() == IDOK)
    {
        LOGFONT chosen = {};
        dlg.GetCurrentFont(&chosen);
        m_result.trayFontFace = chosen.lfFaceName;
        m_result.trayFontSize = max(6, (int)dlg.GetSize() / 10);
        m_result.trayFontBold = chosen.lfWeight >= FW_SEMIBOLD;
        m_result.trayFontItalic = chosen.lfItalic != 0;
        m_result.trayTextColor = (int)dlg.GetColor();
        SetDirty(true);
    }
}

void COptionsDlg::OnTrayDefaults()
{
    m_result.trayTextColor = 0x00FFFF;
    m_result.trayBackEnabled = false;
    m_result.trayBackColor = 0x000000;
    m_result.trayFontFace = L"Arial";
    m_result.trayFontSize = 0;
    m_result.trayFontBold = true;
    m_result.trayFontItalic = false;
    m_result.trayNumberStyle = 0;
    if (m_chkTrayBackEnabled.GetSafeHwnd())
        m_chkTrayBackEnabled.SetCheck(BST_UNCHECKED);
    if (m_btnTrayBackColor.GetSafeHwnd())
        m_btnTrayBackColor.EnableWindow(FALSE);
    if (m_cmbTrayNumber.GetSafeHwnd())
        m_cmbTrayNumber.SetCurSel(0);
    SetDirty(true);
}

void COptionsDlg::OnDisableAutoUpdate()
{
    // Enable/disable the update-related controls based on the "Disable all update checks" checkbox state
    bool isDisabled = (m_chkDisableAutoUpdate.GetCheck() == BST_CHECKED);

    if (m_chkCheckUpdatesAuto.GetSafeHwnd())
        m_chkCheckUpdatesAuto.EnableWindow(!isDisabled);
    if (m_cmbUpdateFrequency.GetSafeHwnd())
        m_cmbUpdateFrequency.EnableWindow(!isDisabled);
    if (m_chkCheckOnLaunch.GetSafeHwnd())
        m_chkCheckOnLaunch.EnableWindow(!isDisabled);
    if (m_btnCheckUpdatesNow.GetSafeHwnd())
        m_btnCheckUpdatesNow.EnableWindow(!isDisabled);

    SetDirty(true);
}

void COptionsDlg::OnCheckNow()
{
    // v0.8.69 - Forward to CMainFrame::OnHelpCheckUpdates(), which owns the
    // full update flow: fetch GitHub release, compare versions, offer download,
    // and auto-replace + restart if the user accepts.
    // MainFrame.h is already included so the cast is safe.
    // If an update is accepted and the main frame calls DestroyWindow(),
    // this modeless dialog is destroyed along with it automatically.
    CMainFrame* pFrame = dynamic_cast<CMainFrame*>(AfxGetMainWnd());
    if (pFrame)
        pFrame->CheckForUpdates();
}

void COptionsDlg::OnManageCals()
{
    if (ShowManageCalendarsDialog(m_result.webCalendars, this))
        SetDirty(true);
}

// OnAdvancedReminders — opens the advanced reminders manager dialog.
void COptionsDlg::OnAdvancedReminders()
{
    CAdvancedRemindersDlg dlg(m_result.advancedReminders, this);
    dlg.DoModal();
    SetDirty(true);
}

// OnPreviewNotification — fires a sample notification so the user can preview
// how their selected style (toast/popup) will look.
void COptionsDlg::OnPreviewNotification()
{
    int styleP = max(0, m_cmbNotifyPersonal.GetCurSel());
    int styleW = max(0, m_cmbNotifyWebCal.GetCurSel());
    int style  = max(styleP, styleW);

    if (style == 0) {
        MessageBox(
            L"Both notification settings are set to Off.\n\n"
            L"Change one to Toast, Popup, or Toast + Popup to see a sample.",
            L"WinLuach – Notifications", MB_OK | MB_ICONINFORMATION);
        return;
    }

    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    if (!pFrame) return;

    std::wstring body =
        L"Birthday: Moshe Levy\n"
        L"Yahrzeit: Rivka Cohen (eve)\n"
        L"Anniversary: Yitzchak & Sara";

    pFrame->ShowEventNotification(L"Today's Events — Sample", body, style);
}
