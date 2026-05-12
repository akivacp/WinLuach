// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    OptionsDlg.cpp
// Purpose: MFC preferences dialog implementation.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial implementation.
// v0.1.1 - Fixed: use InitModalIndirect + in-memory template.
// v0.1.2 - Fixed CreateEx call to use 10 arguments (removed 2 extra nullptrs).
//          Moved DoModal() to public in header.
// =============================================================================

#include "pch.h"
#include "OptionsDlg.h"
#include "MainFrame.h"
#include <cmath>

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

BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
    ON_BN_CLICKED(IDC_OPT_TRAY_COLOR,      &COptionsDlg::OnTrayTextColor)
    ON_BN_CLICKED(IDC_OPT_MANAGE_CALS,     &COptionsDlg::OnManageCals)
    ON_BN_CLICKED(IDC_OPT_PREVIEW_NOTIFY,  &COptionsDlg::OnPreviewNotification)
    ON_NOTIFY(TCN_SELCHANGE, IDC_OPT_TAB,  &COptionsDlg::OnTabChanged)
    ON_WM_SIZE()
END_MESSAGE_MAP()

// =============================================================================
// CONSTRUCTION
// =============================================================================

COptionsDlg::COptionsDlg(const AppSettings& current, CWnd* pParent)
    : CDialog(), m_current(current), m_result(current)
{
    m_pParentWnd = pParent;
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

    buf.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
        DS_CENTER;
    buf.t.dwExtendedStyle = 0;
    buf.t.cdit = 0;
    buf.t.x = 0;
    buf.t.y = 0;
    buf.t.cx = 442;
    buf.t.cy = 396;
    wcscpy_s(buf.title, L"Options and Preferences");

    if (!InitModalIndirect((DLGTEMPLATE*)&buf, m_pParentWnd))
        return -1;

    return CDialog::DoModal();
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
    m_pageColors.clear();
    m_pageZmanim.clear();

    m_tab.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_TABS,
        CRect(6, 6, W - 6, H - 48), this, IDC_OPT_TAB);
    m_tab.SetFont(pFont);
    TCITEM ti = {};
    ti.mask = TCIF_TEXT;
    ti.pszText = const_cast<LPWSTR>(L"General");   m_tab.InsertItem(0, &ti);
    ti.pszText = const_cast<LPWSTR>(L"Month View");m_tab.InsertItem(1, &ti);
    ti.pszText = const_cast<LPWSTR>(L"Interface"); m_tab.InsertItem(2, &ti);
    ti.pszText = const_cast<LPWSTR>(L"Colors");    m_tab.InsertItem(3, &ti);
    ti.pszText = const_cast<LPWSTR>(L"Zmanim");    m_tab.InsertItem(4, &ti);

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
    y = 138;

    mkGroup(m_pageGeneral, L"Holiday Schedule", 14, y, W - 28, 54); y += 22;
    m_radDiaspora.Create(L"Diaspora (outside Israel)", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        CRect(28, y, 220, y + 20), this, IDC_OPT_RAD_DIASPORA); track(m_pageGeneral, &m_radDiaspora);
    m_radIsrael.Create(L"Israel", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        CRect(240, y, 330, y + 20), this, IDC_OPT_RAD_ISRAEL); track(m_pageGeneral, &m_radIsrael);
    y = 38;

    mkGroup(m_pageInterface, L"Interface", 14, y, W - 28, 122); y += 20;
    m_chkShowTrayIcon.Create(L"Always show tray icon", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(28, y, 200, y + 20), this, IDC_OPT_SHOW_TRAY); track(m_pageInterface, &m_chkShowTrayIcon); y += 24;
    m_chkMinimizeToTray.Create(L"Minimize to system tray", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(28, y, 185, y + 20), this, IDC_OPT_MIN_TRAY); track(m_pageInterface, &m_chkMinimizeToTray);
    initCombo(m_pageInterface, m_cmbTrayWhen, 190, y - 1, 160, IDC_OPT_TRAY_WHEN,
        { L"when minimized", L"when closed", L"when minimized or closed" },
        max(0, min(2, m_current.minimizeTrayWhen)));
    m_chkMinimizeOnStartup.Create(L"minimize on startup", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(28, y + 24, 190, y + 44), this, IDC_OPT_MIN_STARTUP); track(m_pageInterface, &m_chkMinimizeOnStartup); y += 48;
    m_chkStartWithWindows.Create(L"start with Windows", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(28, y, 170, y + 20), this, IDC_OPT_START_WINDOWS); track(m_pageInterface, &m_chkStartWithWindows);
    m_chkDesktopShortcut.Create(L"desktop shortcut", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(205, y, 350, y + 20), this, IDC_OPT_DESKTOP); track(m_pageInterface, &m_chkDesktopShortcut); y += 28;
    mkStatic(m_pageInterface, L"Tray text color", 28, y, 105, 22);
    m_btnTrayTextColor.Create(L"Choose...", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(135, y, 220, y + 24), this, IDC_OPT_TRAY_COLOR); track(m_pageInterface, &m_btnTrayTextColor);
    m_btnManageCals.Create(L"Manage Calendars...", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(238, y, 395, y + 24), this, IDC_OPT_MANAGE_CALS); track(m_pageInterface, &m_btnManageCals);
    y = 188;

    mkGroup(m_pageInterface, L"Notifications", 14, y, W - 28, 82); y += 22;
    mkStatic(m_pageInterface, L"Personal events:", 28, y, 120, 22);
    initCombo(m_pageInterface, m_cmbNotifyPersonal, 150, y - 1, 125, IDC_OPT_NOTIFY_PERSONAL,
        { L"Off", L"Toast", L"Popup", L"Toast + Popup" },
        max(0, min(3, m_current.notifyPersonalEvents)));
    m_btnPreviewNotify.Create(L"Test Notification", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(300, y, 420, y + 24), this, IDC_OPT_PREVIEW_NOTIFY); track(m_pageInterface, &m_btnPreviewNotify); y += 28;
    mkStatic(m_pageInterface, L"Web calendar events:", 28, y, 140, 22);
    initCombo(m_pageInterface, m_cmbNotifyWebCal, 170, y - 1, 125, IDC_OPT_NOTIFY_WEBCAL,
        { L"Off", L"Toast", L"Popup", L"Toast + Popup" },
        max(0, min(3, m_current.notifyWebCalEvents)));

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
    m_chkChatzosOnFasts.Create(L"Show chatzos on fast-day cells", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(220, y, W - 28, y + 20), this, IDC_OPT_CHATZOS_FASTS); track(m_pageZmanim, &m_chkChatzosOnFasts);
    y += 34;
    mkStatic(m_pageZmanim, L"Sof zman shema/tefilla and mincha shita:", 28, y, 300, 20); y += 24;
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

    // OK / Cancel buttons
    m_btnOK.Create(L"OK",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        CRect(W - 148, H - 38, W - 80, H - 12), this, IDOK);
    m_btnOK.SetFont(pFont);

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
    m_chkShowTrayIcon.SetCheck(m_current.showTrayIcon ? BST_CHECKED : BST_UNCHECKED);
    m_chkMinimizeToTray.SetCheck(m_current.minimizeToTray ? BST_CHECKED : BST_UNCHECKED);
    m_chkMinimizeOnStartup.SetCheck(m_current.minimizeOnStartup ? BST_CHECKED : BST_UNCHECKED);
    m_chkStartWithWindows.SetCheck(m_current.startWithWindows ? BST_CHECKED : BST_UNCHECKED);
    m_chkDesktopShortcut.SetCheck(m_current.desktopShortcut ? BST_CHECKED : BST_UNCHECKED);

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

    if (m_current.customTzeitMode == 1) m_radTzeitMinutes.SetCheck(BST_CHECKED);
    else if (m_current.customTzeitMode == 2) m_radTzeitZmanis.SetCheck(BST_CHECKED);
    else m_radTzeitDegrees.SetCheck(BST_CHECKED);
    m_lastAlotMode = CurrentAlotMode();
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

    UpdateColorButtons();
    ShowOptionsPage(0);

    return TRUE;
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

    showPage(m_pageGeneral, 0);
    showPage(m_pageMonth, 1);
    showPage(m_pageInterface, 2);
    showPage(m_pageColors, 3);
    showPage(m_pageZmanim, 4);
}

void COptionsDlg::OnTabChanged(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    ShowOptionsPage(max(0, m_tab.GetCurSel()));
    *pResult = 0;
}

void COptionsDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);
    if (m_tab.GetSafeHwnd())
        m_tab.MoveWindow(6, 6, max(120, cx - 12), max(120, cy - 54));
    if (m_btnOK.GetSafeHwnd())
        m_btnOK.MoveWindow(max(10, cx - 148), max(10, cy - 38), 68, 26);
    if (m_btnCancel.GetSafeHwnd())
        m_btnCancel.MoveWindow(max(82, cx - 72), max(10, cy - 38), 64, 26);
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

void COptionsDlg::ConvertAlotMode(int newMode)
{
    int oldMode = m_lastAlotMode;
    double oldValue = GetEditValue(m_editCustomAlot, oldMode == 0 ? 16.1 : 72.0);
    SetAlotMode(newMode);
    SetEditValue(m_editCustomAlot, ConvertAlotValue(oldMode, newMode, oldValue));
    m_lastAlotMode = newMode;
}

void COptionsDlg::ConvertTzeitMode(int newMode)
{
    int oldMode = m_lastTzeitMode;
    double oldValue = GetEditValue(m_editCustomTzeit, oldMode == 0 ? 8.5 : 42.0);
    SetTzeitMode(newMode);
    SetEditValue(m_editCustomTzeit, ConvertTzeitValue(oldMode, newMode, oldValue));
    m_lastTzeitMode = newMode;
}

BOOL COptionsDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    UINT id = LOWORD(wParam);
    UINT code = HIWORD(wParam);
    if (id == IDC_OPT_COLOR_RESTORE)
    {
        RestoreDefaultCalendarColors();
        return TRUE;
    }
    if (id == IDC_OPT_COLOR_PICKER)
    {
        if (ChooseCalendarColor(id))
            return TRUE;
    }
    if (id == IDC_OPT_COLOR_ITEM && code == CBN_SELCHANGE)
    {
        UpdateColorEditorFromSelection();
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
        return TRUE;
    }
    if (id == IDC_OPT_ALOT_SHITA && code == CBN_SELCHANGE)
    {
        ApplyAlotPreset();
        return TRUE;
    }
    if (id == IDC_OPT_TZEIT_SHITA && code == CBN_SELCHANGE)
    {
        ApplyTzeitPreset();
        return TRUE;
    }
    if (id == IDC_OPT_ALOT_DEG)    { ConvertAlotMode(0); return TRUE; }
    if (id == IDC_OPT_ALOT_MIN)    { ConvertAlotMode(1); return TRUE; }
    if (id == IDC_OPT_ALOT_ZMANIS) { ConvertAlotMode(2); return TRUE; }
    if (id == IDC_OPT_TZEIT_DEG)    { ConvertTzeitMode(0); return TRUE; }
    if (id == IDC_OPT_TZEIT_MIN)    { ConvertTzeitMode(1); return TRUE; }
    if (id == IDC_OPT_TZEIT_ZMANIS) { ConvertTzeitMode(2); return TRUE; }
    return CDialog::OnCommand(wParam, lParam);
}

void COptionsDlg::OnOK()
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
    m_result.customAlotMode = (m_radAlotMinutes.GetCheck() == BST_CHECKED) ? 1
        : (m_radAlotZmanis.GetCheck() == BST_CHECKED) ? 2 : 0;
    m_result.customTzeitMode = (m_radTzeitMinutes.GetCheck() == BST_CHECKED) ? 1
        : (m_radTzeitZmanis.GetCheck() == BST_CHECKED) ? 2 : 0;

    CString customValue;
    m_editCustomAlot.GetWindowText(customValue);
    m_result.customAlotValue = max(0.1, _wtof(customValue));
    m_editCustomTzeit.GetWindowText(customValue);
    m_result.customTzeitValue = max(0.1, _wtof(customValue));

    CString candle;
    m_cmbCandleMinutes.GetWindowText(candle);
    m_result.candleLightingMinutes = _wtoi(candle);

    m_result.notifyPersonalEvents = max(0, m_cmbNotifyPersonal.GetCurSel());
    m_result.notifyWebCalEvents   = max(0, m_cmbNotifyWebCal.GetCurSel());

    CDialog::OnOK();
}

void COptionsDlg::OnTrayTextColor()
{
    CColorDialog dlg((COLORREF)m_result.trayTextColor, CC_FULLOPEN, this);
    if (dlg.DoModal() == IDOK)
        m_result.trayTextColor = (int)dlg.GetColor();
}

void COptionsDlg::OnManageCals()
{
    CWebCalDlg dlg(m_result.webCalendars, this);
    if (dlg.DoModal() == IDOK)
        m_result.webCalendars = dlg.calendars;
}

void COptionsDlg::OnPreviewNotification()
{
    // Pick the highest-priority style the user has enabled across both combos
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

    pFrame->ShowEventNotification(L"Today's Events  —  Sample", body, style);
}
