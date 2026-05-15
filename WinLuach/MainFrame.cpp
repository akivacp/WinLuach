// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    MainFrame.cpp
// Purpose: MFC main frame window implementation.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial MFC main frame.
// v0.1.1 - Fixed flashing on cell click: only child panels invalidated,
//          not the full frame. Added Invalidate(FALSE) throughout.
// v0.8.0 - Added "Pin WinLuach to Taskbar" command + handler.  Propagated
//          new zmanim bar mask / end-of-fast preset into the frame state.
// =============================================================================

#include "pch.h"
#include "MainFrame.h"
#include "CalendarView.h"
#include "SidebarPanel.h"
#include "ZmanimPanel.h"
#include "LocationDlg.h"
#include "OptionsDlg.h"
#include "CalPrintDlg.h"
#include "WinLuachApp.h"
#include "Resource.h"
#include "Version.h"
#include <urlmon.h>
#include <fstream>
#include <thread>
#include <commctrl.h>
#include <cmath>
#include <cwctype>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>
#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Shlwapi.lib")

#define ID_COUNTDOWN_OPTIONS    6101
#define ID_COUNTDOWN_FULLSCREEN 6102
#define ID_COUNTDOWN_MINIMIZE   6103
#define ID_COUNTDOWN_MAXIMIZE   6104
#define ID_COUNTDOWN_RESTORE    6105
#define ID_COUNTDOWN_TOPMOST    6106

static std::wstring WebCalTrim(const std::wstring& s)
{
    size_t first = s.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return L"";
    size_t last = s.find_last_not_of(L" \t\r\n");
    return s.substr(first, last - first + 1);
}

static std::wstring GetKnownFolderPathString(REFKNOWNFOLDERID folderId)
{
    PWSTR raw = nullptr;
    if (FAILED(SHGetKnownFolderPath(folderId, 0, nullptr, &raw)) || !raw)
        return L"";
    std::wstring path = raw;
    CoTaskMemFree(raw);
    return path;
}

static std::wstring GetExePathString()
{
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, ARRAYSIZE(path));
    return path;
}

static bool WriteWinLuachShortcut(const std::wstring& linkPath)
{
    std::wstring exe = GetExePathString();
    if (exe.empty())
        return false;

    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool shouldUninit = SUCCEEDED(hrInit);
    if (hrInit == RPC_E_CHANGED_MODE)
        shouldUninit = false;

    IShellLinkW* link = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
        IID_IShellLinkW, (void**)&link);
    if (FAILED(hr) || !link)
    {
        if (shouldUninit) CoUninitialize();
        return false;
    }

    link->SetPath(exe.c_str());
    size_t slash = exe.find_last_of(L"\\/");
    if (slash != std::wstring::npos)
        link->SetWorkingDirectory(exe.substr(0, slash).c_str());
    link->SetDescription(L"WinLuach Hebrew Calendar");

    IPersistFile* file = nullptr;
    hr = link->QueryInterface(IID_IPersistFile, (void**)&file);
    if (SUCCEEDED(hr) && file)
    {
        hr = file->Save(linkPath.c_str(), TRUE);
        file->Release();
    }
    link->Release();
    if (shouldUninit) CoUninitialize();
    return SUCCEEDED(hr);
}

static void ApplyWinLuachShortcut(REFKNOWNFOLDERID folderId, const wchar_t* fileName, bool enabled)
{
    std::wstring folder = GetKnownFolderPathString(folderId);
    if (folder.empty())
        return;
    std::wstring path = folder + L"\\" + fileName;
    if (enabled)
        WriteWinLuachShortcut(path);
    else
        DeleteFileW(path.c_str());
}

static std::wstring DecodeUtf8(const std::string& bytes)
{
    if (bytes.empty()) return L"";
    int needed = MultiByteToWideChar(CP_UTF8, 0,
        bytes.data(), (int)bytes.size(), nullptr, 0);
    if (needed <= 0)
        return L"";

    std::wstring out(needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0,
        bytes.data(), (int)bytes.size(), out.data(), needed);
    return out;
}

static std::wstring UnescapeIcsText(const std::wstring& s)
{
    std::wstring out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == L'\\' && i + 1 < s.size())
        {
            wchar_t c = s[++i];
            if (c == L'n' || c == L'N') out += L' ';
            else if (c == L',' || c == L';' || c == L'\\') out += c;
            else { out += L'\\'; out += c; }
        }
        else
        {
            out += s[i];
        }
    }
    return out;
}

static bool ParseIcsDate(const std::wstring& line, GregorianDate& g)
{
    size_t colon = line.find(L':');
    if (colon == std::wstring::npos) return false;
    std::wstring val = line.substr(colon + 1);
    if (val.size() < 8) return false;
    try
    {
        g.year = std::stoi(val.substr(0, 4));
        g.month = std::stoi(val.substr(4, 2));
        g.day = std::stoi(val.substr(6, 2));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

static CString BuildHelpContentsText()
{
    CString text =
LR"(WinLuach Help Guide

Overview
WinLuach is a Hebrew/Gregorian calendar with zmanim, holidays, parsha information, learning schedules, and personal events.

Main Calendar
- The main grid shows the current civil month or Hebrew month.
- Use the Hebrew/Civil toggle in the header to switch month systems.
- Today's date is highlighted.
- Shabbos, Yom Tov, Chol Hamoed, Rosh Chodesh, fast days, and other special days use distinct background colors.
- Click a day to select it and update the sidebar and zmanim panel.
- Double-click a day, or use the day popup, to see full day details.
- Right-click a day to add an event, print day details, or jump to bar/bat mitzvah dates.

Navigation
- Previous/next day: Left and Right arrows.
- Today: Home.
- Go to Date: Ctrl+G.
- Previous/next month: Page Up and Page Down.
- Previous/next year: Ctrl+Left and Ctrl+Right.
- Previous/next decade: Ctrl+Alt+Left and Ctrl+Alt+Right.
- Change the month or year directly from the header controls.

Sidebar
- The sidebar summarizes the selected day.
- It shows the Hebrew date, holidays, parsha, Omer, daily learning, and personal events.
- Resize the sidebar with the splitter, or collapse/expand it with the sidebar button.

Zmanim
- The bottom panel shows zmanim for the selected date and location.
- Preferences control 12/24-hour display, calculation shitos, candle-lighting offset, and which zmanim are shown.
- Location settings control latitude, longitude, time zone, and Israel/Diaspora behavior.

Parsha And Holidays
- Parsha scheduling follows Israel/Diaspora rules.
- On Shabbos Chol Hamoed or Yom Tov, the holiday reading is shown instead of a regular weekly parsha.
- Israel and Diaspora can diverge after Pesach/Shavuos and then resynchronize through combined parshiyos.
- Devarim is kept before Tisha B'Av.

Events
- Open Calendar > Events, or press Ctrl+E.
- Add birthdays, anniversaries, yahrzeits, and custom events.
- Events can be Hebrew-date or Gregorian-date based.
- Hebrew events can be adjusted for leap years and after-sunset observance.
- Use Export iCal to save events to an .ics file.
- Use Print List in the Events dialog to print the full event list.

Web Calendar
- Preferences can import/sync external calendar items.
- Imported items appear in calendar cells and day details.
- Use refresh/import options from the preferences and event tools as needed.

Printing
- File > Print opens the full print dialog.
- File > Print Preview previews the current print setup.
- Calendar > Print Month prints the current month.
- Calendar > Print Zmanim Month prints a detailed monthly zmanim table.
- Print options include month/year/range, weekly zmanim, visible zmanim columns, footer, margins, and 12/24-hour printed zmanim.
- Long text in print preview shrinks to fit cells where possible.

Options
- Options > Location sets the calculation location and Israel/Diaspora mode.
- Options > Preferences controls display, zmanim, learning, parsha/holiday visibility, printing defaults, tray behavior, and notifications.
- File > Backup Settings saves your settings.
- File > Restore Settings restores a saved settings backup.

Tray And Notifications
- WinLuach can show a tray icon and daily event notifications depending on Preferences.
- Tray actions include opening WinLuach, viewing About, and exiting.

Troubleshooting
- For use on another computer, run the Release build of WinLuach.exe.
- Debug builds require Visual Studio debug runtime DLLs and are not meant for normal deployment.
- If a print preview looks crowded, reduce visible zmanim columns or use landscape/month-specific print modes.)";

    text.Replace(L"\n", L"\r\n");
    return text;
}

class CHelpContentsDlg : public CDialog
{
public:
    CHelpContentsDlg(CWnd* pParent = nullptr) : CDialog()
    {
        m_pParentWnd = pParent;
    }

    INT_PTR DoModal() override
    {
        struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[32]; } b = {};
        b.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_CENTER;
        b.t.cx = 520;
        b.t.cy = 390;
        wcscpy_s(b.title, L"WinLuach Help");
        if (!InitModalIndirect((DLGTEMPLATE*)&b, m_pParentWnd))
            return -1;
        return CDialog::DoModal();
    }

protected:
    BOOL OnInitDialog() override
    {
        CDialog::OnInitDialog();
        SetWindowText(L"WinLuach Help Contents");

        HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CFont* pF = CFont::FromHandle(hF);

        CRect rc;
        GetClientRect(&rc);
        m_text.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER |
            ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
            CRect(10, 10, rc.Width() - 10, rc.Height() - 44), this, 7001);
        m_text.SetFont(pF);
        m_text.SetWindowText(BuildHelpContentsText());
        m_text.SetSel(0, 0);

        m_btnClose.Create(L"Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            CRect(rc.Width() - 92, rc.Height() - 32, rc.Width() - 10, rc.Height() - 8),
            this, IDOK);
        m_btnClose.SetFont(pF);
        return TRUE;
    }

    afx_msg void OnSize(UINT nType, int cx, int cy)
    {
        CDialog::OnSize(nType, cx, cy);
        if (m_text.GetSafeHwnd())
            m_text.MoveWindow(10, 10, max(50, cx - 20), max(50, cy - 54));
        if (m_btnClose.GetSafeHwnd())
            m_btnClose.MoveWindow(max(10, cx - 92), max(10, cy - 32), 82, 24);
    }

    CEdit   m_text;
    CButton m_btnClose;

    DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CHelpContentsDlg, CDialog)
    ON_WM_SIZE()
END_MESSAGE_MAP()

static CTime DateTimeForZman(const GregorianDate& g, const TimeOfDay& t)
{
    return CTime(g.year, g.month, g.day, max(0, t.hour), max(0, t.minute), max(0, t.second));
}

static bool ParseUserClockTime(std::wstring text, int& hour, int& minute)
{
    text = WebCalTrim(text);
    if (text.empty()) return false;

    std::wstring lower;
    lower.reserve(text.size());
    for (wchar_t ch : text)
        lower.push_back((wchar_t)towlower(ch));

    bool hasPm = lower.find(L"pm") != std::wstring::npos;
    bool hasAm = lower.find(L"am") != std::wstring::npos;
    std::wstring cleaned;
    for (wchar_t ch : lower)
        if ((ch >= L'0' && ch <= L'9') || ch == L':')
            cleaned.push_back(ch);

    if (cleaned.empty()) return false;
    size_t colon = cleaned.find(L':');
    int h = 0, m = 0;
    try
    {
        if (colon == std::wstring::npos)
        {
            h = std::stoi(cleaned);
        }
        else
        {
            h = std::stoi(cleaned.substr(0, colon));
            m = std::stoi(cleaned.substr(colon + 1));
        }
    }
    catch (...)
    {
        return false;
    }

    if (m < 0 || m > 59) return false;
    if (hasPm && h >= 1 && h <= 11) h += 12;
    if (hasAm && h == 12) h = 0;
    if (h < 0 || h > 23) return false;
    hour = h;
    minute = m;
    return true;
}

static TimeOfDay CustomMorningBoundary(const GregorianDate& g, const Location& loc, bool isDst,
    const ZmanimResult& z, int mode, double value, double fallbackDegrees)
{
    mode = max(0, min(2, mode));
    if (mode == 0)
    {
        if (value <= 0.01)
            return z.hanetz;
        return CalculateSunAtAngle(g, loc, isDst, value > 0.0 ? value : fallbackDegrees, true);
    }
    if (mode == 1)
        return AddMinutes(z.hanetz, -(int)round(max(0.0, value)));
    return AddShaot(z.hanetz, z.shaahZmanit_GRA, -max(0.0, value) / 60.0);
}

static TimeOfDay CustomEveningBoundary(const GregorianDate& g, const Location& loc, bool isDst,
    const ZmanimResult& z, int mode, double value, double fallbackDegrees)
{
    mode = max(0, min(2, mode));
    if (mode == 0)
    {
        if (value <= 0.01)
            return z.shkia;
        return CalculateSunAtAngle(g, loc, isDst, value > 0.0 ? value : fallbackDegrees, false);
    }
    if (mode == 1)
        return AddMinutes(z.shkia, (int)round(max(0.0, value)));
    return AddShaot(z.shkia, z.shaahZmanit_GRA, max(0.0, value) / 60.0);
}

static std::wstring BoundaryLabel(const wchar_t* base, int mode, double value, bool allowGra)
{
    CString s;
    if (allowGra && value <= 0.01)
        s.Format(L"%s (GRA)", base);
    else if (mode == 0)
        s.Format(L"%s (%.2g deg)", base, value);
    else if (mode == 1)
        s.Format(L"%s (%.0f min)", base, value);
    else
        s.Format(L"%s (%.0f z-min)", base, value);
    return (LPCWSTR)s;
}

static void MakeFont(CFont& font, const std::wstring& face, int pointSize, int weight, bool italic = false)
{
    font.DeleteObject();
    font.CreatePointFont(max(60, pointSize * 10), face.empty() ? L"Segoe UI" : face.c_str());
    LOGFONT lf = {};
    if (font.GetLogFont(&lf))
    {
        font.DeleteObject();
        lf.lfWeight = weight;
        lf.lfItalic = italic ? TRUE : FALSE;
        wcscpy_s(lf.lfFaceName, face.empty() ? L"Segoe UI" : face.c_str());
        HDC screen = ::GetDC(nullptr);
        int dpiY = screen ? GetDeviceCaps(screen, LOGPIXELSY) : 96;
        if (screen) ::ReleaseDC(nullptr, screen);
        lf.lfHeight = -MulDiv(max(6, pointSize), dpiY, 72);
        font.CreateFontIndirect(&lf);
    }
}

class CCountdownOptionsDlg : public CDialog
{
    static constexpr UINT IDC_CDOPT_TAB     = 5050;
    static constexpr UINT IDC_CDOPT_APPLY   = 5201;
    static constexpr UINT IDC_CDOPT_RESTORE = 5202;

    static const wchar_t* kZmanLabels(int i)
    {
        static const wchar_t* k[] = {
            L"Alos HaShachar (GRA)",  L"Alos HaShachar (MA72)", L"Alos HaShachar (MA90)",
            L"Misheyakir",            L"Hanetz (Sunrise)",
            L"Sof Shema (GRA)",       L"Sof Shema (MA72)",      L"Sof Shema (MA90)",
            L"Sof Tefilla (GRA)",     L"Sof Tefilla (MA72)",    L"Sof Tefilla (MA90)",
            L"Chatzos",
            L"Mincha Gedola (GRA)",   L"Mincha Gedola (MA72)",  L"Mincha Gedola (MA90)",
            L"Mincha Ketana (GRA)",   L"Mincha Ketana (MA72)",
            L"Plag HaMincha (GRA)",   L"Plag HaMincha (MA72)",  L"Plag HaMincha (MA90)",
            L"Shkiah (Sunset)",
            L"Tzeit (GRA)",           L"Tzeit (MA72)",          L"Tzeit (MA90)",
            L"Candle Lighting"
        };
        return (i >= 0 && i < 25) ? k[i] : L"";
    }

public:
    CCountdownOptionsDlg(CMainFrame* frame, CWnd* parent = nullptr)
        : CDialog(), m_pFrame(frame), m_settings(theApp.m_settings)
    {
        m_pParentWnd = parent;
    }

    INT_PTR DoModal() override
    {
        struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[32]; } b = {};
        b.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_CENTER;
        b.t.dwExtendedStyle = WS_EX_APPWINDOW;
        b.t.cx = 270; b.t.cy = 305;
        wcscpy_s(b.title, L"Countdown Options");
        if (!InitModalIndirect((DLGTEMPLATE*)&b, m_pParentWnd)) return -1;
        return CDialog::DoModal();
    }

protected:
    BOOL OnInitDialog() override
    {
        CDialog::OnInitDialog();
        SetWindowText(L"Countdown Clock Options");
        HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CFont* pF = CFont::FromHandle(hF);
        CRect rc; GetClientRect(&rc);
        int W = rc.Width(), H = rc.Height();

        // Tooltip control
        m_tooltip.Create(this, TTS_ALWAYSTIP | TTS_BALLOON);
        m_tooltip.SetMaxTipWidth(300);
        m_tooltip.Activate(TRUE);

        // Tab control
        m_tabCtrl.Create(WS_CHILD | WS_VISIBLE | TCS_TABS,
            CRect(0, 4, W, 32), this, IDC_CDOPT_TAB);
        m_tabCtrl.SetFont(pF);
        TCITEM ti = {}; ti.mask = TCIF_TEXT;
        ti.pszText = const_cast<LPWSTR>(L"Appearance"); m_tabCtrl.InsertItem(0, &ti);
        ti.pszText = const_cast<LPWSTR>(L"Zmanim");     m_tabCtrl.InsertItem(1, &ti);

        auto mkStaticP = [&](const wchar_t* text, int x, int y, int w, int h, std::vector<CWnd*>& page) {
            CStatic* s = new CStatic;
            s->Create(text, WS_CHILD | SS_LEFT | SS_CENTERIMAGE,
                CRect(x, y, x + w, y + h), this, 5100 + m_nextId++);
            s->SetFont(pF);
            page.push_back(s);
            return s;
        };

        // ── Appearance page ──────────────────────────────────────────
        int y = 40;
        mkStaticP(L"Show", 10, y, 38, 18, m_page1);
        mkStaticP(L"Area", 52, y, 60, 18, m_page1);
        mkStaticP(L"Font", 118, y, 90, 18, m_page1);
        mkStaticP(L"Size", 214, y, 38, 18, m_page1);
        mkStaticP(L"Text", 256, y, 46, 18, m_page1);
        mkStaticP(L"Back", 306, y, 46, 18, m_page1);
        y += 22;

        static const wchar_t* kRowLabels[] = {
            L"Next zman", L"Countdown", L"Zman time", L"Live clock" };
        static const wchar_t* kShowTips[] = {
            L"Show the upcoming zman name in the countdown clock",
            L"Show the HH:MM:SS countdown timer in the countdown clock",
            L"Show the actual clock time of the upcoming zman",
            L"Show the live current time and date" };
        bool* showFlags[] = {
            &m_settings.countdownShowTitle,  &m_settings.countdownShowClock,
            &m_settings.countdownShowZmanTime, &m_settings.countdownShowLive };
        std::wstring* faceFields[] = {
            &m_settings.countdownTitleFontFace,   &m_settings.countdownClockFontFace,
            &m_settings.countdownCurrentFontFace, &m_settings.countdownLiveFontFace };
        int* sizeFields[] = {
            &m_settings.countdownTitleFontSize,   &m_settings.countdownClockFontSize,
            &m_settings.countdownCurrentFontSize, &m_settings.countdownLiveFontSize };
        CEdit* faceEdits[] = { &m_editTitleFace, &m_editClockFace, &m_editCurrentFace, &m_editLiveFace };
        CEdit* sizeEdits[] = { &m_editTitleSize, &m_editClockSize, &m_editCurrentSize, &m_editLiveSize };
        CButton* colorBtns[] = { &m_btnTitleColor, &m_btnClockColor, &m_btnCurrentColor, &m_btnLiveColor };
        CButton* backBtns[]  = { &m_btnTitleBack,  &m_btnClockBack,  &m_btnCurrentBack,  &m_btnLiveBack };
        CButton* showChks[]  = { &m_chkShowTitle, &m_chkShowClock, &m_chkShowZmanTime, &m_chkShowLive };

        for (int i = 0; i < 4; ++i)
        {
            // Show checkbox
            showChks[i]->Create(L"", WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
                CRect(12, y + 3, 48, y + 21), this, 5800 + i);
            showChks[i]->SetFont(pF);
            showChks[i]->SetCheck(*showFlags[i] ? BST_CHECKED : BST_UNCHECKED);
            m_page1.push_back(showChks[i]);
            m_tooltip.AddTool(showChks[i], kShowTips[i]);

            // Area label
            CStatic* lbl = new CStatic;
            lbl->Create(kRowLabels[i], WS_CHILD | SS_LEFT | SS_CENTERIMAGE,
                CRect(52, y, 112, y + 22), this, 5300 + m_nextId++);
            lbl->SetFont(pF);
            m_page1.push_back(lbl);

            // Font face edit
            faceEdits[i]->Create(WS_CHILD | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
                CRect(118, y, 208, y + 22), this, 5400 + m_nextId++);
            faceEdits[i]->SetFont(pF);
            faceEdits[i]->SetWindowText(faceFields[i]->c_str());
            m_page1.push_back(faceEdits[i]);

            // Size edit
            CString sz; sz.Format(L"%d", *sizeFields[i]);
            sizeEdits[i]->Create(WS_CHILD | WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_CENTER,
                CRect(214, y, 252, y + 22), this, 5500 + m_nextId++);
            sizeEdits[i]->SetFont(pF);
            sizeEdits[i]->SetWindowText(sz);
            m_page1.push_back(sizeEdits[i]);

            // Font/color button
            colorBtns[i]->Create(L"Font...", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                CRect(256, y, 302, y + 24), this, 5600 + m_nextId++);
            colorBtns[i]->SetFont(pF);
            m_page1.push_back(colorBtns[i]);
            m_tooltip.AddTool(colorBtns[i], L"Choose font, size, and text color for this area");

            // Back color button
            backBtns[i]->Create(L"Back", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                CRect(306, y, 350, y + 24), this, 5700 + m_nextId++);
            backBtns[i]->SetFont(pF);
            m_page1.push_back(backBtns[i]);
            m_tooltip.AddTool(backBtns[i], L"Choose background color for this area");

            y += 34;
        }

        y += 6;
        mkStaticP(L"Use Font/Back buttons to choose colors for each area.", 10, y, W - 20, 18, m_page1);
        y += 24;

        m_btnRestoreColors.Create(L"Restore Defaults", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
            CRect(10, y, 180, y + 24), this, IDC_CDOPT_RESTORE);
        m_btnRestoreColors.SetFont(pF);
        m_page1.push_back(&m_btnRestoreColors);
        m_tooltip.AddTool(&m_btnRestoreColors, L"Restore all countdown clock fonts and colors to their default values");

        // ── Zmanim page ───────────────────────────────────────────────
        {
            CStatic* znote = new CStatic;
            znote->Create(L"Select which zmanim the clock counts down to:",
                WS_CHILD | SS_LEFT, CRect(10, 40, W - 10, 58), this, 5100 + m_nextId++);
            znote->SetFont(pF);
            m_page2.push_back(znote);
        }
        int colW = (W - 20) / 2;
        for (int i = 0; i < 25; ++i)
        {
            int col = i / 13;
            int row = i % 13;
            int zx = 10 + col * colW;
            int zy = 62 + row * 20;
            m_chkZmanim[i].Create(kZmanLabels(i), WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
                CRect(zx, zy, zx + colW - 4, zy + 18), this, 5900 + i);
            m_chkZmanim[i].SetFont(pF);
            m_chkZmanim[i].SetCheck(((m_settings.countdownZmanimMask >> i) & 1) ? BST_CHECKED : BST_UNCHECKED);
            m_page2.push_back(&m_chkZmanim[i]);
            m_tooltip.AddTool(&m_chkZmanim[i], L"Include this zman when searching for the next countdown target");
        }

        // ── Buttons (always visible) ────────────────────────────────
        m_btnOK.Create(L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            CRect(W - 236, H - 34, W - 164, H - 8), this, IDOK);
        m_btnOK.SetFont(pF);
        m_btnApply.Create(L"Apply", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            CRect(W - 158, H - 34, W - 86, H - 8), this, IDC_CDOPT_APPLY);
        m_btnApply.SetFont(pF);
        m_btnCancel.Create(L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            CRect(W - 80, H - 34, W - 8, H - 8), this, IDCANCEL);
        m_btnCancel.SetFont(pF);

        ShowPage(0);
        return TRUE;
    }

    void ShowPage(int page)
    {
        for (CWnd* c : m_page1) if (c && c->GetSafeHwnd()) c->ShowWindow(page == 0 ? SW_SHOW : SW_HIDE);
        for (CWnd* c : m_page2) if (c && c->GetSafeHwnd()) c->ShowWindow(page == 1 ? SW_SHOW : SW_HIDE);
    }

    BOOL OnNotify(UINT nCtrlID, NMHDR* pNMHDR, LRESULT* pResult) override
    {
        if (pNMHDR && pNMHDR->code == TCN_SELCHANGE &&
            m_tabCtrl.GetSafeHwnd() && pNMHDR->hwndFrom == m_tabCtrl.GetSafeHwnd())
        {
            ShowPage(m_tabCtrl.GetCurSel());
            return TRUE;
        }
        return CDialog::OnNotify(nCtrlID, pNMHDR, pResult);
    }

    BOOL PreTranslateMessage(MSG* pMsg) override
    {
        if (m_tooltip.GetSafeHwnd())
            m_tooltip.RelayEvent(pMsg);
        return CDialog::PreTranslateMessage(pMsg);
    }

    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override
    {
        HWND h = (HWND)lParam;
        if (HIWORD(wParam) == BN_CLICKED && h)
        {
            if (h == m_btnTitleColor.GetSafeHwnd())   return PickFont(m_settings.countdownTitleFontFace,   m_settings.countdownTitleFontSize,   m_settings.countdownTitleTextColor,   m_settings.countdownTitleBold,   m_settings.countdownTitleItalic);
            if (h == m_btnTitleBack.GetSafeHwnd())    return Pick(m_settings.countdownTitleBackColor);
            if (h == m_btnClockColor.GetSafeHwnd())   return PickFont(m_settings.countdownClockFontFace,   m_settings.countdownClockFontSize,   m_settings.countdownClockTextColor,   m_settings.countdownClockBold,   m_settings.countdownClockItalic);
            if (h == m_btnClockBack.GetSafeHwnd())    return Pick(m_settings.countdownClockBackColor);
            if (h == m_btnCurrentColor.GetSafeHwnd()) return PickFont(m_settings.countdownCurrentFontFace, m_settings.countdownCurrentFontSize, m_settings.countdownCurrentTextColor, m_settings.countdownCurrentBold, m_settings.countdownCurrentItalic);
            if (h == m_btnCurrentBack.GetSafeHwnd())  return Pick(m_settings.countdownCurrentBackColor);
            if (h == m_btnLiveColor.GetSafeHwnd())    return PickFont(m_settings.countdownLiveFontFace,    m_settings.countdownLiveFontSize,    m_settings.countdownLiveTextColor,    m_settings.countdownLiveBold,    m_settings.countdownLiveItalic);
            if (h == m_btnLiveBack.GetSafeHwnd())     return Pick(m_settings.countdownLiveBackColor);
        }
        if (LOWORD(wParam) == IDC_CDOPT_APPLY)   { Apply(); return TRUE; }
        if (LOWORD(wParam) == IDC_CDOPT_RESTORE) { RestoreDefaults(); Apply(); return TRUE; }
        return CDialog::OnCommand(wParam, lParam);
    }

    void OnOK() override { Apply(); CDialog::OnOK(); }

    BOOL Pick(int& target)
    {
        CColorDialog dlg((COLORREF)target, CC_FULLOPEN, this);
        if (dlg.DoModal() == IDOK) target = (int)dlg.GetColor();
        return TRUE;
    }

    BOOL PickFont(std::wstring& face, int& size, int& color, bool& bold, bool& italic)
    {
        LOGFONT lf = {};
        wcscpy_s(lf.lfFaceName, face.empty() ? L"Segoe UI" : face.c_str());
        HDC screen = ::GetDC(nullptr);
        int dpiY = screen ? GetDeviceCaps(screen, LOGPIXELSY) : 96;
        if (screen) ::ReleaseDC(nullptr, screen);
        lf.lfHeight = -MulDiv(max(6, size), dpiY, 72);
        lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
        lf.lfItalic = italic ? TRUE : FALSE;
        CFontDialog dlg(&lf, CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT, nullptr, this);
        dlg.m_cf.rgbColors = (COLORREF)color;
        if (dlg.DoModal() == IDOK)
        {
            LOGFONT chosen = {}; dlg.GetCurrentFont(&chosen);
            face = chosen.lfFaceName;
            size = max(6, (int)dlg.GetSize() / 10);
            color = (int)dlg.GetColor();
            bold = chosen.lfWeight >= FW_SEMIBOLD;
            italic = chosen.lfItalic != 0;
            SyncFontEdits();
            if (m_btnApply.GetSafeHwnd()) m_btnApply.EnableWindow(TRUE);
        }
        return TRUE;
    }

    void Read()
    {
        auto readFace = [](CEdit& e, std::wstring& d) { CString s; e.GetWindowText(s); d = (LPCWSTR)s; };
        auto readSize = [](CEdit& e, int& d)           { CString s; e.GetWindowText(s); d = max(6, min(80, _wtoi(s))); };
        readFace(m_editTitleFace,   m_settings.countdownTitleFontFace);
        readSize(m_editTitleSize,   m_settings.countdownTitleFontSize);
        readFace(m_editClockFace,   m_settings.countdownClockFontFace);
        readSize(m_editClockSize,   m_settings.countdownClockFontSize);
        readFace(m_editCurrentFace, m_settings.countdownCurrentFontFace);
        readSize(m_editCurrentSize, m_settings.countdownCurrentFontSize);
        readFace(m_editLiveFace,    m_settings.countdownLiveFontFace);
        readSize(m_editLiveSize,    m_settings.countdownLiveFontSize);

        // Show/hide checkboxes
        bool* showFlags[] = { &m_settings.countdownShowTitle, &m_settings.countdownShowClock,
                               &m_settings.countdownShowZmanTime, &m_settings.countdownShowLive };
        CButton* showChks[] = { &m_chkShowTitle, &m_chkShowClock, &m_chkShowZmanTime, &m_chkShowLive };
        for (int i = 0; i < 4; ++i)
            if (showChks[i]->GetSafeHwnd())
                *showFlags[i] = (showChks[i]->GetCheck() == BST_CHECKED);

        // Zmanim mask
        uint32_t mask = 0;
        for (int i = 0; i < 25; ++i)
            if (m_chkZmanim[i].GetSafeHwnd() && m_chkZmanim[i].GetCheck() == BST_CHECKED)
                mask |= (1u << i);
        m_settings.countdownZmanimMask = mask;
    }

    void Apply()
    {
        Read();
        if (m_pFrame) m_pFrame->ApplyAndSaveSettings(m_settings);
        if (m_btnApply.GetSafeHwnd()) m_btnApply.EnableWindow(FALSE);
    }

    void SyncFontEdits()
    {
        auto setSize = [](CEdit& e, int v) { CString s; s.Format(L"%d", v); e.SetWindowText(s); };
        m_editTitleFace.SetWindowText(m_settings.countdownTitleFontFace.c_str());
        setSize(m_editTitleSize, m_settings.countdownTitleFontSize);
        m_editClockFace.SetWindowText(m_settings.countdownClockFontFace.c_str());
        setSize(m_editClockSize, m_settings.countdownClockFontSize);
        m_editCurrentFace.SetWindowText(m_settings.countdownCurrentFontFace.c_str());
        setSize(m_editCurrentSize, m_settings.countdownCurrentFontSize);
        m_editLiveFace.SetWindowText(m_settings.countdownLiveFontFace.c_str());
        setSize(m_editLiveSize, m_settings.countdownLiveFontSize);
    }

    void RestoreDefaults()
    {
        AppSettings d;
        m_settings.countdownTitleTextColor   = d.countdownTitleTextColor;
        m_settings.countdownTitleBackColor   = d.countdownTitleBackColor;
        m_settings.countdownClockTextColor   = d.countdownClockTextColor;
        m_settings.countdownClockBackColor   = d.countdownClockBackColor;
        m_settings.countdownCurrentTextColor = d.countdownCurrentTextColor;
        m_settings.countdownCurrentBackColor = d.countdownCurrentBackColor;
        m_settings.countdownLiveTextColor    = d.countdownLiveTextColor;
        m_settings.countdownLiveBackColor    = d.countdownLiveBackColor;
        m_settings.countdownTitleFontFace    = d.countdownTitleFontFace;
        m_settings.countdownTitleFontSize    = d.countdownTitleFontSize;
        m_settings.countdownTitleBold        = d.countdownTitleBold;
        m_settings.countdownTitleItalic      = d.countdownTitleItalic;
        m_settings.countdownClockFontFace    = d.countdownClockFontFace;
        m_settings.countdownClockFontSize    = d.countdownClockFontSize;
        m_settings.countdownClockBold        = d.countdownClockBold;
        m_settings.countdownClockItalic      = d.countdownClockItalic;
        m_settings.countdownCurrentFontFace  = d.countdownCurrentFontFace;
        m_settings.countdownCurrentFontSize  = d.countdownCurrentFontSize;
        m_settings.countdownCurrentBold      = d.countdownCurrentBold;
        m_settings.countdownCurrentItalic    = d.countdownCurrentItalic;
        m_settings.countdownLiveFontFace     = d.countdownLiveFontFace;
        m_settings.countdownLiveFontSize     = d.countdownLiveFontSize;
        m_settings.countdownLiveBold         = d.countdownLiveBold;
        m_settings.countdownLiveItalic       = d.countdownLiveItalic;
        SyncFontEdits();
    }

private:
    CMainFrame*  m_pFrame = nullptr;
    AppSettings  m_settings;
    int          m_nextId = 0;
    CTabCtrl     m_tabCtrl;
    CToolTipCtrl m_tooltip;
    std::vector<CWnd*> m_page1, m_page2;

    // Appearance tab rows
    CEdit   m_editTitleFace,   m_editTitleSize;
    CEdit   m_editClockFace,   m_editClockSize;
    CEdit   m_editCurrentFace, m_editCurrentSize;
    CEdit   m_editLiveFace,    m_editLiveSize;
    CButton m_btnTitleColor,   m_btnTitleBack;
    CButton m_btnClockColor,   m_btnClockBack;
    CButton m_btnCurrentColor, m_btnCurrentBack;
    CButton m_btnLiveColor,    m_btnLiveBack;
    CButton m_chkShowTitle, m_chkShowClock, m_chkShowZmanTime, m_chkShowLive;

    // Zmanim tab
    CButton m_chkZmanim[25];

    // Dialog buttons
    CButton m_btnOK, m_btnApply, m_btnCancel, m_btnRestoreColors;
};

class CCountdownClockWnd : public CFrameWnd
{
public:
    explicit CCountdownClockWnd(CMainFrame* frame) : m_pFrame(frame) {}

    BOOL CreateClock()
    {
        LPCTSTR cls = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW,
            ::LoadCursor(nullptr, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1),
            ::LoadIcon(nullptr, IDI_APPLICATION));
        return CFrameWnd::CreateEx(WS_EX_APPWINDOW, cls, L"WinLuach Countdown Clock",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
                WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
            CRect(180, 180, 560, 370), nullptr, 0);
    }

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lp)
    {
        if (CFrameWnd::OnCreate(lp) == -1) return -1;
        RebuildMenuBar();
        UpdateTopmostUi(false);
        SetTimer(1, 1000, nullptr);
        return 0;
    }

    afx_msg void OnDestroy()
    {
        KillTimer(1);
        if (m_pFrame && m_pFrame->m_pCountdownClock == this)
            m_pFrame->m_pCountdownClock = nullptr;
        CFrameWnd::OnDestroy();
    }

    afx_msg void OnTimer(UINT_PTR id)
    {
        if (id == 1)
        {
            Invalidate(FALSE);
            UpdateWindow();
        }
        else CFrameWnd::OnTimer(id);
    }

    afx_msg void OnSize(UINT nType, int cx, int cy)
    {
        CFrameWnd::OnSize(nType, cx, cy);
        if (cx > 0 && cy > 0)
            Invalidate(FALSE);
    }

    afx_msg void OnClockOptions()
    {
        CCountdownOptionsDlg dlg(m_pFrame, nullptr);
        dlg.DoModal();
    }

    afx_msg void OnClockFullscreen()
    {
        ToggleFullscreen();
    }

    afx_msg void OnClockTopmost()
    {
        m_alwaysOnTop = !m_alwaysOnTop;
        SetWindowPos(m_alwaysOnTop ? &wndTopMost : &wndNoTopMost,
            0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        RebuildMenuBar();
        UpdateTopmostUi(true);
    }

    afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
    {
        CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
        UpdateTopmostUi(false);
    }

    afx_msg void OnClockMinimize()
    {
        ShowWindow(SW_MINIMIZE);
    }

    afx_msg void OnClockMaximize()
    {
        if (m_fullScreen)
            ToggleFullscreen();
        ShowWindow(SW_MAXIMIZE);
    }

    afx_msg void OnClockRestore()
    {
        if (m_fullScreen)
            ToggleFullscreen();
        else
            ShowWindow(SW_RESTORE);
    }

    BOOL PreTranslateMessage(MSG* pMsg) override
    {
        if (pMsg && pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F11)
        {
            ToggleFullscreen();
            return TRUE;
        }
        if (pMsg && pMsg->message == WM_SYSKEYDOWN && pMsg->wParam == 'T')
        {
            OnClockTopmost();
            return TRUE;
        }
        return CFrameWnd::PreTranslateMessage(pMsg);
    }

    void ToggleFullscreen()
    {
        if (!m_fullScreen)
        {
            m_restorePlacement.length = sizeof(m_restorePlacement);
            GetWindowPlacement(&m_restorePlacement);
            m_restoreStyle = (DWORD)GetWindowLongPtr(m_hWnd, GWL_STYLE);
            SetWindowLongPtr(m_hWnd, GWL_STYLE,
                m_restoreStyle & ~(WS_CAPTION | WS_THICKFRAME));

            MONITORINFO mi = {};
            mi.cbSize = sizeof(mi);
            HMONITOR mon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
            GetMonitorInfo(mon, &mi);
            SetWindowPos(nullptr, mi.rcMonitor.left, mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            m_fullScreen = true;
        }
        else
        {
            SetWindowLongPtr(m_hWnd, GWL_STYLE, m_restoreStyle);
            SetWindowPlacement(&m_restorePlacement);
            SetWindowPos(nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            m_fullScreen = false;
        }
        Invalidate(FALSE);
    }

    struct UpcomingZman
    {
        std::wstring label;
        CTime time;
        bool valid = false;
    };

    UpcomingZman GetUpcoming() const
    {
        UpcomingZman best;
        if (!m_pFrame) return best;

        CTime now = CTime::GetCurrentTime();
        CTime threshold = now + CTimeSpan(0, 0, 0, 1);
        SYSTEMTIME st = {};
        GetLocalTime(&st);
        GregorianDate today(st.wYear, st.wMonth, st.wDay);
        for (int d = 0; d < 2; ++d)
        {
            GregorianDate g = JDNToGregorian(GregorianToJDN(today) + d);
            bool isDst = IsDST(g, m_pFrame->m_location);
            ZmanimResult z = CalculateZmanim(g, m_pFrame->m_location, isDst);
            z.candleLighting = AddMinutes(z.shkia, -theApp.m_settings.candleLightingMinutes);
            DisplayZmanimTimes dz = m_pFrame->BuildDisplayZmanim(g, z, isDst);
            struct Item { const wchar_t* label; TimeOfDay time; };
            std::vector<Item> items = {
                { L"Alos Hashachar (GRA)",   z.alot_GRA          },
                { L"Alos Hashachar (MA72)",  z.alot_MA72         },
                { L"Alos Hashachar (MA90)",  z.alot_MA90         },
                { L"Misheyakir",             dz.misheyakir       },
                { L"Hanetz",                 z.hanetz            },
                { L"Sof Shema (GRA)",        z.sofShema_GRA      },
                { L"Sof Shema (MA72)",       z.sofShema_MA72     },
                { L"Sof Shema (MA90)",       z.sofShema_MA90     },
                { L"Sof Tefilla (GRA)",      z.sofTefilla_GRA    },
                { L"Sof Tefilla (MA72)",     z.sofTefilla_MA72   },
                { L"Sof Tefilla (MA90)",     z.sofTefilla_MA90   },
                { L"Chatzos",                z.chatzot           },
                { L"Mincha Gedola (GRA)",    z.minchaGedola_GRA  },
                { L"Mincha Gedola (MA72)",   z.minchaGedola_MA72 },
                { L"Mincha Gedola (MA90)",   z.minchaGedola_MA90 },
                { L"Mincha Ketana (GRA)",    z.minchaKetana_GRA  },
                { L"Mincha Ketana (MA72)",   z.minchaKetana_MA72 },
                { L"Plag HaMincha (GRA)",    z.plagMincha_GRA    },
                { L"Plag HaMincha (MA72)",   z.plagMincha_MA72   },
                { L"Plag HaMincha (MA90)",   z.plagMincha_MA90   },
                { L"Shkiah",                 z.shkia             },
                { L"Tzeit (GRA)",            z.tzeit_GRA         },
                { L"Tzeit (MA72)",           z.tzeit_MA72        },
                { L"Tzeit (MA90)",           z.tzeit_MA90        },
                { L"Candle Lighting",        z.candleLighting    },
            };
            uint32_t zmask = theApp.m_settings.countdownZmanimMask;
            for (int idx = 0; idx < (int)items.size(); ++idx)
            {
                if (!((zmask >> idx) & 1)) continue;
                const auto& item = items[idx];
                if (!item.time.IsValid()) continue;
                CTime t = DateTimeForZman(g, item.time);
                if (t <= threshold) continue;
                if (!best.valid || t < best.time)
                    best = { item.label, t, true };
            }
        }
        return best;
    }

    afx_msg void OnPaint()
    {
        CPaintDC dc(this);
        CRect rc; GetClientRect(&rc);
        const AppSettings& s = theApp.m_settings;
        dc.FillSolidRect(rc, (COLORREF)s.countdownClockBackColor);

        UpcomingZman next = GetUpcoming();
        CTime now = CTime::GetCurrentTime();

        // Build text for each section
        CString title    = next.valid ? CString(L"Next: ") + next.label.c_str() : L"No upcoming zman";
        CString countdown = L"--:--:--";
        if (next.valid)
        {
            CTimeSpan span = next.time - now;
            countdown.Format(L"%02I64d:%02d:%02d",
                span.GetTotalHours(), span.GetMinutes(), span.GetSeconds());
        }
        CString zmanTime;
        if (next.valid)
        {
            const wchar_t* timeFmt = s.use24Hour ? L"%H:%M" : L"%I:%M %p";
            zmanTime.Format(L"%s:  %s", next.label.c_str(), next.time.Format(timeFmt).GetString());
        }
        else { zmanTime = L"—"; }

        const wchar_t* liveFmt = s.use24Hour ? L"%H:%M:%S" : L"%I:%M:%S %p";
        CString live;
        live.Format(L"%s     %s",
            now.Format(liveFmt).GetString(),
            now.Format(L"%A, %B %d, %Y").GetString());

        // Determine visible sections and distribute height
        struct Section {
            bool       visible;
            CString    text;
            int        fontWeight;
            bool       italic;
            std::wstring face;
            int        size;
            COLORREF   textColor;
            COLORREF   backColor;
        };
        Section sections[] = {
            { s.countdownShowTitle,    title,    s.countdownTitleBold   ? FW_BOLD : FW_NORMAL, s.countdownTitleItalic,   s.countdownTitleFontFace,   s.countdownTitleFontSize,   (COLORREF)s.countdownTitleTextColor,   (COLORREF)s.countdownTitleBackColor   },
            { s.countdownShowClock,    countdown,s.countdownClockBold   ? FW_BOLD : FW_NORMAL, s.countdownClockItalic,   s.countdownClockFontFace,   s.countdownClockFontSize,   (COLORREF)s.countdownClockTextColor,   (COLORREF)s.countdownClockBackColor   },
            { s.countdownShowZmanTime, zmanTime, s.countdownCurrentBold ? FW_BOLD : FW_NORMAL, s.countdownCurrentItalic, s.countdownCurrentFontFace, s.countdownCurrentFontSize, (COLORREF)s.countdownCurrentTextColor, (COLORREF)s.countdownCurrentBackColor },
            { s.countdownShowLive,     live,     s.countdownLiveBold    ? FW_BOLD : FW_NORMAL, s.countdownLiveItalic,    s.countdownLiveFontFace,    s.countdownLiveFontSize,    (COLORREF)s.countdownLiveTextColor,    (COLORREF)s.countdownLiveBackColor    },
        };
        // Weights: title=2, clock=4, zmanTime=2, live=2 → total 10 when all visible
        int weights[] = { 2, 4, 2, 2 };
        int totalWeight = 0;
        for (int i = 0; i < 4; ++i) if (sections[i].visible) totalWeight += weights[i];
        if (totalWeight == 0) return;

        int totalH = rc.Height();
        int curY   = rc.top;
        double scale = min(max(0.55, rc.Width() / 380.0), max(0.55, totalH / 190.0));

        dc.SetBkMode(OPAQUE);
        auto draw = [&](const Section& sec, int h) {
            CFont f;
            MakeFont(f, sec.face, (int)round(sec.size * scale), sec.fontWeight, sec.italic);
            CRect area(0, curY, rc.Width(), curY + h);
            dc.FillSolidRect(area, sec.backColor);
            dc.SelectObject(&f);
            dc.SetTextColor(sec.textColor);
            dc.SetBkColor(sec.backColor);
            CRect drawRc = area; drawRc.DeflateRect(8, 2);
            dc.DrawText(sec.text, drawRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        };

        int distributed = 0;
        for (int i = 0; i < 4; ++i)
        {
            if (!sections[i].visible) continue;
            int h = (i < 3) ? (totalH * weights[i] / totalWeight) : (totalH - distributed);
            draw(sections[i], h);
            curY += h;
            distributed += h;
        }
    }

    void PostNcDestroy() override
    {
        delete this;
    }

    DECLARE_MESSAGE_MAP()

private:
    void RebuildMenuBar()
    {
        CMenu menu, opt;
        menu.CreateMenu();
        opt.CreatePopupMenu();
        opt.AppendMenu(MF_STRING, ID_COUNTDOWN_OPTIONS, L"&Options...");
        opt.AppendMenu(MF_SEPARATOR);
        opt.AppendMenu(MF_STRING, ID_COUNTDOWN_FULLSCREEN, L"&Full Screen\tF11");
        opt.AppendMenu(MF_STRING, ID_COUNTDOWN_TOPMOST, L"Always on &Top\tAlt+T");
        opt.AppendMenu(MF_SEPARATOR);
        opt.AppendMenu(MF_STRING, ID_COUNTDOWN_MINIMIZE, L"Mi&nimize");
        opt.AppendMenu(MF_STRING, ID_COUNTDOWN_MAXIMIZE, L"Ma&ximize");
        opt.AppendMenu(MF_STRING, ID_COUNTDOWN_RESTORE, L"&Restore");
        menu.AppendMenu(MF_POPUP, (UINT_PTR)opt.Detach(), L"&Clock");
        if (m_alwaysOnTop)
            menu.AppendMenu(MF_STRING, ID_COUNTDOWN_TOPMOST, L"Always on top enabled");

        HMENU oldMenu = ::GetMenu(GetSafeHwnd());
        SetMenu(&menu);
        menu.Detach();
        DrawMenuBar();
        if (oldMenu)
            ::DestroyMenu(oldMenu);
    }

    void UpdateTopmostUi(bool repaint)
    {
        CMenu* menu = GetMenu();
        if (menu)
            menu->CheckMenuItem(ID_COUNTDOWN_TOPMOST,
                MF_BYCOMMAND | (m_alwaysOnTop ? MF_CHECKED : MF_UNCHECKED));
        if (repaint)
            Invalidate(FALSE);
    }

    CMainFrame* m_pFrame = nullptr;
    bool m_fullScreen = false;
    bool m_alwaysOnTop = false;
    DWORD m_restoreStyle = 0;
    WINDOWPLACEMENT m_restorePlacement = { sizeof(WINDOWPLACEMENT) };
};

BEGIN_MESSAGE_MAP(CCountdownClockWnd, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_TIMER()
    ON_WM_SIZE()
    ON_WM_PAINT()
    ON_WM_INITMENUPOPUP()
    ON_COMMAND(ID_COUNTDOWN_OPTIONS, &CCountdownClockWnd::OnClockOptions)
    ON_COMMAND(ID_COUNTDOWN_FULLSCREEN, &CCountdownClockWnd::OnClockFullscreen)
    ON_COMMAND(ID_COUNTDOWN_TOPMOST, &CCountdownClockWnd::OnClockTopmost)
    ON_COMMAND(ID_COUNTDOWN_MINIMIZE, &CCountdownClockWnd::OnClockMinimize)
    ON_COMMAND(ID_COUNTDOWN_MAXIMIZE, &CCountdownClockWnd::OnClockMaximize)
    ON_COMMAND(ID_COUNTDOWN_RESTORE, &CCountdownClockWnd::OnClockRestore)
END_MESSAGE_MAP()

// =============================================================================
// CGotoDateDlg — compact mini-calendar date picker
// =============================================================================

class CGotoDateDlg : public CDialog
{
public:
    GregorianDate selectedDate;

    CGotoDateDlg(const GregorianDate& initial, CWnd* pParent = nullptr)
        : CDialog(), m_year(initial.year), m_month(initial.month), m_day(initial.day)
    {
        m_pParentWnd = pParent;
        selectedDate = initial;
    }

    INT_PTR DoModal() override
    {
        struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[20]; } b = {};
        b.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_CENTER;
        b.t.cx = 230; b.t.cy = 265;
        wcscpy_s(b.title, L"Go to Date");
        if (!InitModalIndirect((DLGTEMPLATE*)&b, m_pParentWnd)) return -1;
        return CDialog::DoModal();
    }

protected:
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override
    {
        CDialog::OnInitDialog();
        SetWindowText(L"Go to Date");
        HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CFont* pF = CFont::FromHandle(hF);
        CRect rc; GetClientRect(&rc);
        int W = rc.Width(), H = rc.Height();

        auto mkBtn = [&](const wchar_t* t, int x, int y, int w, int h, UINT id) {
            CButton* b = new CButton;
            b->Create(t, WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
                CRect(x, y, x+w, y+h), this, id);
            b->SetFont(pF);
        };
        auto mkLbl = [&](const wchar_t* t, int x, int y, int w, int h) {
            CStatic* s = new CStatic;
            s->Create(t, WS_CHILD|WS_VISIBLE|SS_RIGHT|SS_CENTERIMAGE,
                CRect(x, y, x+w, y+h), this);
            s->SetFont(pF);
        };

        // Row 1: nav arrows + month combo
        mkBtn(L"<<", 4,    4, 28, 24, 601);
        mkBtn(L"<",  36,   4, 24, 24, 602);
        mkBtn(L">",  W-60, 4, 24, 24, 603);
        mkBtn(L">>", W-32, 4, 28, 24, 604);

        m_cmbMonth.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,
            CRect(64, 4, W-64, 4+150), this, 610);
        m_cmbMonth.SetFont(pF);
        static const wchar_t* kMon[] = {
            L"January",L"February",L"March",L"April",L"May",L"June",
            L"July",L"August",L"September",L"October",L"November",L"December" };
        for (int i = 0; i < 12; i++) m_cmbMonth.AddString(kMon[i]);
        m_cmbMonth.SetCurSel(m_month - 1);

        // Row 2: day edit/spin + year edit/spin
        mkLbl(L"Day:", 4, 34, 32, 22);
        m_editDay.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_NUMBER|ES_CENTER,
            CRect(38, 34, 78, 56), this, 611);
        m_editDay.SetFont(pF);
        m_spinDay.Create(WS_CHILD|WS_VISIBLE|UDS_ARROWKEYS|UDS_SETBUDDYINT|UDS_NOTHOUSANDS,
            CRect(78, 34, 90, 56), this, 612);
        m_spinDay.SetBuddy(&m_editDay);
        m_spinDay.SetRange(1, DaysInGregorianMonth(m_month, m_year));
        m_spinDay.SetPos(m_day);

        mkLbl(L"Year:", 100, 34, 38, 22);
        m_editYear.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_NUMBER|ES_CENTER,
            CRect(140, 34, 194, 56), this, 613);
        m_editYear.SetFont(pF);
        CString sy; sy.Format(L"%d", m_year);
        m_editYear.SetWindowText(sy);
        m_spinYear.Create(WS_CHILD|WS_VISIBLE|UDS_ARROWKEYS|UDS_SETBUDDYINT|UDS_NOTHOUSANDS,
            CRect(194, 34, 206, 56), this, 614);
        m_spinYear.SetBuddy(&m_editYear);
        m_spinYear.SetRange32(1, 9999);
        m_spinYear.SetPos32(m_year);

        mkBtn(L"Go",     W-128, H-32, 52, 26, IDOK);
        mkBtn(L"Cancel", W-70,  H-32, 64, 26, IDCANCEL);

        return TRUE;
    }

    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override
    {
        switch (LOWORD(wParam)) {
        case 601: NavYear(-1);  return TRUE;
        case 602: NavMonth(-1); return TRUE;
        case 603: NavMonth(1);  return TRUE;
        case 604: NavYear(1);   return TRUE;
        case 610:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                int sel = m_cmbMonth.GetCurSel();
                if (sel >= 0) { m_month = sel + 1; ClampDay(); Invalidate(FALSE); }
            }
            return TRUE;
        case 611:
            if (HIWORD(wParam) == EN_CHANGE) {
                CString sd; m_editDay.GetWindowText(sd);
                int d = _wtoi(sd);
                int maxd = DaysInGregorianMonth(m_month, m_year);
                if (d >= 1 && d <= maxd) { m_day = d; Invalidate(FALSE); }
            }
            return TRUE;
        case 613:
            if (HIWORD(wParam) == EN_CHANGE) {
                CString sy; m_editYear.GetWindowText(sy);
                int y = _wtoi(sy);
                if (y >= 1 && y <= 9999) { m_year = y; ClampDay(); Invalidate(FALSE); }
            }
            return TRUE;
        }
        return CDialog::OnCommand(wParam, lParam);
    }

    void OnOK() override
    {
        // Read month from combo
        int sel = m_cmbMonth.GetCurSel();
        if (sel >= 0) m_month = sel + 1;

        // Read year from edit
        CString sy; m_editYear.GetWindowText(sy);
        int y = _wtoi(sy);
        if (y >= 1 && y <= 9999) m_year = y;

        // Read day from edit (clamp to valid range)
        CString sd; m_editDay.GetWindowText(sd);
        int d = _wtoi(sd);
        int maxd = DaysInGregorianMonth(m_month, m_year);
        m_day = max(1, min(d, maxd));

        selectedDate = GregorianDate(m_year, m_month, m_day);
        CDialog::OnOK();
    }

private:
    int              m_year, m_month, m_day;
    CComboBox        m_cmbMonth;
    CEdit            m_editDay;
    CSpinButtonCtrl  m_spinDay;
    CEdit            m_editYear;
    CSpinButtonCtrl  m_spinYear;

    void NavMonth(int delta) {
        m_month += delta;
        while (m_month < 1)  { m_month += 12; m_year--; }
        while (m_month > 12) { m_month -= 12; m_year++; }
        SyncControls(); ClampDay(); Invalidate(FALSE);
    }
    void NavYear(int delta) { m_year += delta; SyncControls(); ClampDay(); Invalidate(FALSE); }

    void ClampDay() {
        int maxd = DaysInGregorianMonth(m_month, m_year);
        if (m_day > maxd) m_day = maxd;
        if (m_spinDay.GetSafeHwnd()) {
            m_spinDay.SetRange(1, maxd);
            m_spinDay.SetPos(m_day);
        }
    }

    void SyncControls() {
        if (m_cmbMonth.GetSafeHwnd()) m_cmbMonth.SetCurSel(m_month - 1);
        if (m_editYear.GetSafeHwnd()) {
            CString sy; sy.Format(L"%d", m_year);
            m_editYear.SetWindowText(sy);
            if (m_spinYear.GetSafeHwnd()) m_spinYear.SetPos32(m_year);
        }
    }
};

void CGotoDateDlg::OnPaint()
{
    CPaintDC dc(this);
    CRect rc; GetClientRect(&rc);
    int W = rc.Width(), H = rc.Height();

    int hdrY  = 58;
    int gridY = hdrY + 20;
    int cellH = max(18, (H - gridY - 40) / 6);
    int cellW = (W - 8) / 7;

    // Day header row
    dc.FillSolidRect(CRect(4, hdrY, W-4, hdrY+20), RGB(70, 100, 160));
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(RGB(255, 255, 255));
    dc.SelectObject(CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT)));
    static const wchar_t* hdrs[] = { L"Su",L"Mo",L"Tu",L"We",L"Th",L"Fr",L"Sa" };
    for (int i = 0; i < 7; i++) {
        CRect c(4 + i*cellW, hdrY, 4 + (i+1)*cellW, hdrY+20);
        dc.DrawText(hdrs[i], -1, &c, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    }

    // Calendar cells
    GregorianDate first(m_year, m_month, 1);
    int startCol     = (int)GetDayOfWeek(first);
    int daysInMonth  = DaysInGregorianMonth(m_month, m_year);
    GregorianDate today = GetTodayGregorian();

    for (int day = 1; day <= daysInMonth; day++) {
        int idx = startCol + day - 1;
        int col = idx % 7, row = idx / 7;
        CRect cell(4 + col*cellW, gridY + row*cellH,
                   4 + (col+1)*cellW, gridY + (row+1)*cellH);

        COLORREF bg = RGB(255,255,255);
        if (day == m_day)
            bg = RGB(55, 90, 200);
        else if (m_year == today.year && m_month == today.month && day == today.day)
            bg = RGB(255, 255, 140);
        else if (col == 6)
            bg = RGB(228, 244, 228);

        dc.FillSolidRect(&cell, bg);
        dc.DrawEdge(&cell, EDGE_SUNKEN, BF_RECT);
        dc.SetTextColor(day == m_day ? RGB(255,255,255) : RGB(0,0,0));
        wchar_t s[4]; swprintf_s(s, L"%d", day);
        dc.DrawText(s, -1, &cell, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    }
}

void CGotoDateDlg::OnLButtonDown(UINT, CPoint pt)
{
    CRect rc; GetClientRect(&rc);
    int W = rc.Width(), H = rc.Height();
    int hdrY = 58, gridY = hdrY + 20;
    int cellH = max(18, (H - gridY - 40) / 6), cellW = (W - 8) / 7;
    if (pt.y < gridY || pt.x < 4 || pt.x > W-4) return;
    int col = (pt.x - 4) / cellW;
    int row = (pt.y - gridY) / cellH;
    if (col < 0 || col > 6) return;
    int startCol = (int)GetDayOfWeek(GregorianDate(m_year, m_month, 1));
    int day = row * 7 + col - startCol + 1;
    if (day >= 1 && day <= DaysInGregorianMonth(m_month, m_year)) {
        m_day = day;
        if (m_editDay.GetSafeHwnd()) {
            CString sd; sd.Format(L"%d", m_day);
            m_editDay.SetWindowText(sd);
            if (m_spinDay.GetSafeHwnd()) m_spinDay.SetPos(m_day);
        }
        Invalidate(FALSE);
    }
}

BEGIN_MESSAGE_MAP(CGotoDateDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

// =============================================================================
// CEventEditDlg — add / edit a single recurring personal event
// =============================================================================

class CEventEditDlg : public CDialog
{
public:
    UserEventEntry entry;

    explicit CEventEditDlg(const UserEventEntry* existing = nullptr, CWnd* pParent = nullptr)
        : CDialog()
    {
        m_pParentWnd = pParent;
        if (existing) entry = *existing;
    }

    INT_PTR DoModal() override
    {
        struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[32]; } b = {};
        b.t.style = WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|DS_CENTER;
        b.t.cx = 380; b.t.cy = 305;
        wcscpy_s(b.title, L"Event");
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

        auto mkLbl = [&](const wchar_t* t, int x, int y, int w, int h) {
            CStatic* s = new CStatic;
            s->Create(t, WS_CHILD|WS_VISIBLE|SS_LEFT|SS_CENTERIMAGE,
                CRect(x, y, x+w, y+h), this);
            s->SetFont(pF);
        };
        auto mkBtn = [&](const wchar_t* t, int x, int y, int w, int h, UINT id, DWORD extra=0) {
            CButton* b = new CButton;
            b->Create(t, WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON|extra,
                CRect(x, y, x+w, y+h), this, id);
            b->SetFont(pF);
        };

        int y = 10;

        // Name
        mkLbl(L"Name:", 10, y, 60, 20);
        m_editName.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER,
            CRect(72, y, W-10, y+22), this, 700);
        m_editName.SetFont(pF);
        m_editName.SetWindowText(entry.name.c_str());
        y += 28;

        // Type
        mkLbl(L"Type:", 10, y, 60, 20);
        m_cmbType.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST,
            CRect(72, y, 200, y+120), this, 701);
        m_cmbType.SetFont(pF);
        m_cmbType.AddString(L"Birthday");
        m_cmbType.AddString(L"Anniversary");
        m_cmbType.AddString(L"Yahrzeit");
        m_cmbType.AddString(L"Custom");
        m_cmbType.SetCurSel(entry.type);
        y += 28;

        // ── Gregorian date ───────────────────────────────────────────────────
        m_chkGreg.Create(L"Observe on Gregorian date:",
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
            CRect(10, y, 230, y+20), this, 702);
        m_chkGreg.SetFont(pF);
        m_chkGreg.SetCheck(entry.gregMonth > 0 ? BST_CHECKED : BST_UNCHECKED);
        y += 24;

        mkLbl(L"Month:", 20, y, 48, 20);
        m_cmbGregMon.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST,
            CRect(70, y, 190, y+160), this, 703);
        m_cmbGregMon.SetFont(pF);
        static const wchar_t* kGMon[] = {
            L"January",L"February",L"March",L"April",L"May",L"June",
            L"July",L"August",L"September",L"October",L"November",L"December" };
        for (int i = 0; i < 12; i++) m_cmbGregMon.AddString(kGMon[i]);
        m_cmbGregMon.SetCurSel(max(0, entry.gregMonth - 1));

        mkLbl(L"Day:", 196, y, 28, 20);
        m_editGregDay.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_NUMBER|ES_CENTER,
            CRect(226, y, 262, y+22), this, 704);
        m_editGregDay.SetFont(pF);
        m_spinGregDay.Create(WS_CHILD|WS_VISIBLE|UDS_ARROWKEYS|UDS_SETBUDDYINT|UDS_NOTHOUSANDS,
            CRect(262, y, 274, y+22), this, 705);
        m_spinGregDay.SetBuddy(&m_editGregDay);
        m_spinGregDay.SetRange(1, 31);
        m_spinGregDay.SetPos(max(1, entry.gregDay));
        mkLbl(L"Year:", 280, y, 32, 20);
        m_editGregYear.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_NUMBER|ES_CENTER,
            CRect(314, y, W-10, y+22), this, 711);
        m_editGregYear.SetFont(pF);
        if (entry.gregYear > 0) {
            wchar_t yb[8]; swprintf_s(yb, L"%d", entry.gregYear);
            m_editGregYear.SetWindowText(yb);
        }
        y += 28;

        // ── Hebrew date ──────────────────────────────────────────────────────
        m_chkHeb.Create(L"Observe on Hebrew date:",
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
            CRect(10, y, 230, y+20), this, 706);
        m_chkHeb.SetFont(pF);
        m_chkHeb.SetCheck(entry.hebMonth > 0 ? BST_CHECKED : BST_UNCHECKED);
        y += 24;

        mkLbl(L"Month:", 20, y, 48, 20);
        m_cmbHebMon.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST,
            CRect(70, y, 190, y+160), this, 707);
        m_cmbHebMon.SetFont(pF);
        static const wchar_t* kHMon[] = {
            L"Tishrei",L"Cheshvan",L"Kislev",L"Tevet",L"Shvat",
            L"Adar (I)",L"Adar II",L"Nissan",L"Iyar",L"Sivan",
            L"Tammuz",L"Av",L"Elul" };
        for (int i = 0; i < 13; i++) m_cmbHebMon.AddString(kHMon[i]);
        m_cmbHebMon.SetCurSel(max(0, entry.hebMonth - 1));

        mkLbl(L"Day:", 196, y, 28, 20);
        m_editHebDay.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_NUMBER|ES_CENTER,
            CRect(226, y, 262, y+22), this, 708);
        m_editHebDay.SetFont(pF);
        m_spinHebDay.Create(WS_CHILD|WS_VISIBLE|UDS_ARROWKEYS|UDS_SETBUDDYINT|UDS_NOTHOUSANDS,
            CRect(262, y, 274, y+22), this, 709);
        m_spinHebDay.SetBuddy(&m_editHebDay);
        m_spinHebDay.SetRange(1, 30);
        m_spinHebDay.SetPos(max(1, entry.hebDay));
        mkLbl(L"Year:", 280, y, 32, 20);
        m_editHebYear.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_NUMBER|ES_CENTER,
            CRect(314, y, W-10, y+22), this, 712);
        m_editHebYear.SetFont(pF);
        if (entry.hebYear > 0) {
            wchar_t yb[8]; swprintf_s(yb, L"%d", entry.hebYear);
            m_editHebYear.SetWindowText(yb);
        }
        y += 28;

        // After sunset
        m_chkSunset.Create(L"After sunset (evening before — for Yahrzeit)",
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
            CRect(10, y, W-10, y+20), this, 710);
        m_chkSunset.SetFont(pF);
        m_chkSunset.SetCheck(entry.afterSunset ? BST_CHECKED : BST_UNCHECKED);
        y += 26;

        m_chkNotify.Create(L"Notify for this personal event",
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
            CRect(10, y, W-10, y+20), this, 713);
        m_chkNotify.SetFont(pF);
        m_chkNotify.SetCheck(entry.notify ? BST_CHECKED : BST_UNCHECKED);
        y += 24;

        mkLbl(L"Alarms:", 20, y, 56, 20);
        m_editAlarms.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,
            CRect(78, y, W-10, y+22), this, 714);
        m_editAlarms.SetFont(pF);
        m_editAlarms.SetWindowText(entry.alarmOffsets.c_str());

        mkBtn(L"OK",     W-138, H-32, 60, 26, IDOK,     BS_DEFPUSHBUTTON);
        mkBtn(L"Cancel", W-70,  H-32, 60, 26, IDCANCEL);

        return TRUE;
    }

    void OnOK() override
    {
        CString s; m_editName.GetWindowText(s);
        entry.name = s.GetString();
        if (entry.name.empty()) {
            MessageBox(L"Please enter a name.", L"WinLuach", MB_OK|MB_ICONWARNING);
            return;
        }
        entry.type = max(0, m_cmbType.GetCurSel());

        if (m_chkGreg.GetCheck() == BST_CHECKED) {
            entry.gregMonth = m_cmbGregMon.GetCurSel() + 1;
            CString sd; m_editGregDay.GetWindowText(sd);
            entry.gregDay = max(1, min(31, _wtoi(sd)));
            CString sy; m_editGregYear.GetWindowText(sy);
            entry.gregYear = max(0, _wtoi(sy));
        } else {
            entry.gregMonth = 0;
            entry.gregYear  = 0;
        }

        if (m_chkHeb.GetCheck() == BST_CHECKED) {
            entry.hebMonth = m_cmbHebMon.GetCurSel() + 1;
            CString sd; m_editHebDay.GetWindowText(sd);
            entry.hebDay = max(1, min(30, _wtoi(sd)));
            CString sy; m_editHebYear.GetWindowText(sy);
            entry.hebYear = max(0, _wtoi(sy));
        } else {
            entry.hebMonth = 0;
            entry.hebYear  = 0;
        }

        if (entry.gregMonth == 0 && entry.hebMonth == 0) {
            MessageBox(L"Please enable at least one date (Gregorian or Hebrew).",
                L"WinLuach", MB_OK|MB_ICONWARNING);
            return;
        }

        entry.afterSunset = (m_chkSunset.GetCheck() == BST_CHECKED);
        entry.notify = (m_chkNotify.GetCheck() == BST_CHECKED);
        CString alarms;
        m_editAlarms.GetWindowText(alarms);
        entry.alarmOffsets = (LPCWSTR)alarms;
        CDialog::OnOK();
    }

    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override
    {
        if (HIWORD(wParam) == BN_CLICKED) {
            UINT id = LOWORD(wParam);
            // Hebrew checkbox just checked → auto-fill from Gregorian
            if (id == 706 && m_chkHeb.GetCheck() == BST_CHECKED
                          && m_chkGreg.GetCheck() == BST_CHECKED)
                AutoFillHebrew();
            // After-sunset toggled → re-derive Hebrew if checkbox is active
            if (id == 710 && m_chkHeb.GetCheck() == BST_CHECKED
                          && m_chkGreg.GetCheck() == BST_CHECKED)
                AutoFillHebrew();
        }
        return CDialog::OnCommand(wParam, lParam);
    }

    DECLARE_MESSAGE_MAP()

private:
    void AutoFillHebrew()
    {
        // Read the Gregorian date currently in the dialog
        int gregMon = m_cmbGregMon.GetCurSel() + 1;
        if (gregMon < 1 || gregMon > 12) return;
        CString sd; m_editGregDay.GetWindowText(sd);
        int gregDay = max(1, min(31, _wtoi(sd)));
        CString sy; m_editGregYear.GetWindowText(sy);
        int gregYear = _wtoi(sy);
        if (gregYear <= 0) { SYSTEMTIME st; GetLocalTime(&st); gregYear = st.wYear; }

        GregorianDate gd(gregYear, gregMon, gregDay);
        // "After sunset" means the Hebrew day that *starts* at that sunset
        if (m_chkSunset.GetCheck() == BST_CHECKED)
            gd = JDNToGregorian(GregorianToJDN(gd) + 1);

        HebrewDate h = GregorianToHebrew(gd);

        m_cmbHebMon.SetCurSel(h.month - 1);
        wchar_t db[4]; swprintf_s(db, L"%d", h.day);
        m_editHebDay.SetWindowText(db);
        m_spinHebDay.SetPos(h.day);
        // Only write year when the user already specified a Gregorian year
        if (_wtoi(sy) > 0) {
            wchar_t yb[8]; swprintf_s(yb, L"%d", h.year);
            m_editHebYear.SetWindowText(yb);
        } else {
            m_editHebYear.SetWindowText(L"");
        }
    }

    CEdit            m_editName;
    CComboBox        m_cmbType;
    CButton          m_chkGreg;
    CComboBox        m_cmbGregMon;
    CEdit            m_editGregDay;
    CSpinButtonCtrl  m_spinGregDay;
    CEdit            m_editGregYear;
    CButton          m_chkHeb;
    CComboBox        m_cmbHebMon;
    CEdit            m_editHebDay;
    CSpinButtonCtrl  m_spinHebDay;
    CEdit            m_editHebYear;
    CButton          m_chkSunset;
    CButton          m_chkNotify;
    CEdit            m_editAlarms;
};
BEGIN_MESSAGE_MAP(CEventEditDlg, CDialog) END_MESSAGE_MAP()

// =============================================================================
// iCal export — converts recurring personal events to .ics VEVENT entries
// =============================================================================

static void IcalWriteEvent(std::wofstream& f, const std::wstring& summary,
                            const GregorianDate& dt)
{
    GregorianDate end = JDNToGregorian(GregorianToJDN(dt) + 1);
    wchar_t ds[10], de[10];
    swprintf_s(ds, L"%04d%02d%02d", dt.year,  dt.month,  dt.day);
    swprintf_s(de, L"%04d%02d%02d", end.year, end.month, end.day);
    f << L"BEGIN:VEVENT\r\n";
    f << L"DTSTART;VALUE=DATE:" << ds << L"\r\n";
    f << L"DTEND;VALUE=DATE:"   << de << L"\r\n";
    f << L"SUMMARY:" << summary << L"\r\n";
    f << L"END:VEVENT\r\n";
}

static bool ExportEventsToIcal(const std::vector<UserEventEntry>& events,
                                const std::wstring& path,
                                int yearsAhead, bool includeGreg, bool includeHeb)
{
    std::wofstream f(path);
    if (!f.is_open()) return false;

    f << L"BEGIN:VCALENDAR\r\n";
    f << L"VERSION:2.0\r\n";
    f << L"PRODID:-//WinLuach//WinLuach Calendar//EN\r\n";
    f << L"CALSCALE:GREGORIAN\r\n";
    f << L"METHOD:PUBLISH\r\n";

    GregorianDate today = GetTodayGregorian();
    HebrewDate    todayH = GregorianToHebrew(today);
    int endGregYear  = today.year + yearsAhead;
    int endHebYear   = todayH.year + yearsAhead + 1;

    static const wchar_t* kTypes[] = {L"Birthday",L"Anniversary",L"Yahrzeit",L"Event"};

    for (const auto& ev : events)
    {
        const wchar_t* tstr = (ev.type >= 0 && ev.type <= 3) ? kTypes[ev.type] : L"Event";
        std::wstring baseName = std::wstring(tstr) + L": " + ev.name;

        // ── Gregorian recurrence ──────────────────────────────────────────────
        if (includeGreg && ev.gregMonth > 0) {
            for (int y = today.year; y < endGregYear; y++) {
                if (ev.gregDay > DaysInGregorianMonth(ev.gregMonth, y)) continue;
                GregorianDate dt(y, ev.gregMonth, ev.gregDay);
                if (GregorianToJDN(dt) < GregorianToJDN(today)) continue;
                IcalWriteEvent(f, baseName + L" (Gregorian)", dt);
            }
        }

        // ── Hebrew recurrence ─────────────────────────────────────────────────
        if (includeHeb && ev.hebMonth > 0) {
            for (int hy = todayH.year; hy < endHebYear; hy++) {
                int mo = ev.hebMonth;
                // Adar in non-leap years: Adar II (7) → Adar (6)
                if (mo == ADAR_II && !IsHebrewLeapYear(hy)) mo = ADAR;
                if (ev.hebDay > DaysInHebrewMonth(mo, hy)) continue;

                GregorianDate gd = HebrewToGregorian(HebrewDate(hy, mo, ev.hebDay));
                if (gd.year < today.year || gd.year >= endGregYear) continue;
                if (GregorianToJDN(gd) < GregorianToJDN(today)) continue;

                if (ev.afterSunset)
                    gd = JDNToGregorian(GregorianToJDN(gd) - 1); // show on eve

                std::wstring lbl = baseName + L" (Hebrew)";
                if (ev.afterSunset) lbl += L" \x2014 eve"; // em dash
                IcalWriteEvent(f, lbl, gd);
            }
        }
    }

    f << L"END:VCALENDAR\r\n";
    return true;
}

// =============================================================================
// CICalExportDlg — options for iCal export (years, Gregorian/Hebrew)
// =============================================================================

class CICalExportDlg : public CDialog
{
public:
    int  m_years      = 10;
    bool m_inclGreg   = true;
    bool m_inclHeb    = true;

    explicit CICalExportDlg(const std::vector<UserEventEntry>& events, CWnd* pParent = nullptr)
        : CDialog()
    {
        m_pParentWnd = pParent;
        bool hasGreg = false, hasHeb = false;
        for (const auto& e : events) {
            if (e.gregMonth > 0) hasGreg = true;
            if (e.hebMonth  > 0) hasHeb  = true;
        }
        m_inclGreg = hasGreg;
        m_inclHeb  = hasHeb;
    }

    INT_PTR DoModal() override
    {
        struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[32]; } b = {};
        b.t.style = WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|DS_CENTER;
        b.t.cx = 260; b.t.cy = 160;
        wcscpy_s(b.title, L"Export to iCal");
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

        auto mkLbl = [&](const wchar_t* t, int x, int y, int w) {
            CStatic* s = new CStatic;
            s->Create(t, WS_CHILD|WS_VISIBLE|SS_LEFT|SS_CENTERIMAGE,
                CRect(x, y, x+w, y+22), this);
            s->SetFont(pF);
        };
        auto mkChk = [&](const wchar_t* t, int y, UINT id, bool chk) {
            CButton* b = new CButton;
            b->Create(t, WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
                CRect(14, y, W-14, y+20), this, id);
            b->SetFont(pF);
            b->SetCheck(chk ? BST_CHECKED : BST_UNCHECKED);
        };

        mkLbl(L"Years ahead to export:", 14, 12, 148);
        m_editYears.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_NUMBER|ES_CENTER,
            CRect(164, 10, 206, 32), this, 900);
        m_editYears.SetFont(pF);
        m_spinYears.Create(WS_CHILD|WS_VISIBLE|UDS_ARROWKEYS|UDS_SETBUDDYINT|UDS_NOTHOUSANDS,
            CRect(206, 10, 218, 32), this, 901);
        m_spinYears.SetBuddy(&m_editYears);
        m_spinYears.SetRange(1, 250);
        m_spinYears.SetPos(m_years);

        mkChk(L"Include Gregorian date recurrences", 42, 902, m_inclGreg);
        mkChk(L"Include Hebrew date recurrences",    66, 903, m_inclHeb);

        auto mkBtn = [&](const wchar_t* t, int x, int id, DWORD ex=0) {
            CButton* b = new CButton;
            b->Create(t, WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON|ex,
                CRect(x, H-32, x+80, H-8), this, id);
            b->SetFont(pF);
        };
        mkBtn(L"Export",  W-170, IDOK, BS_DEFPUSHBUTTON);
        mkBtn(L"Cancel",  W-84,  IDCANCEL);
        return TRUE;
    }

    void OnOK() override
    {
        CString sy; m_editYears.GetWindowText(sy);
        m_years    = max(1, min(250, _wtoi(sy)));
        m_inclGreg = (((CButton*)GetDlgItem(902))->GetCheck() == BST_CHECKED);
        m_inclHeb  = (((CButton*)GetDlgItem(903))->GetCheck() == BST_CHECKED);
        if (!m_inclGreg && !m_inclHeb) {
            MessageBox(L"Please select at least one date type (Gregorian or Hebrew).",
                L"WinLuach", MB_OK|MB_ICONWARNING);
            return;
        }
        CDialog::OnOK();
    }

    DECLARE_MESSAGE_MAP()
private:
    CEdit           m_editYears;
    CSpinButtonCtrl m_spinYears;
};
BEGIN_MESSAGE_MAP(CICalExportDlg, CDialog) END_MESSAGE_MAP()

// =============================================================================
// CEventsManagerDlg — manage the full list of personal recurring events
// =============================================================================

class CEventsManagerDlg : public CDialog
{
public:
    explicit CEventsManagerDlg(std::vector<UserEventEntry>& events, CWnd* pParent = nullptr)
        : CDialog(), m_events(events)
    {
        m_pParentWnd = pParent;
    }

    INT_PTR DoModal() override
    {
        struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[40]; } b = {};
        b.t.style = WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|DS_CENTER;
        b.t.cx = 320; b.t.cy = 280;
        wcscpy_s(b.title, L"Events - Birthdays, Yahrzeits...");
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

        m_list.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|WS_VSCROLL|LBS_NOTIFY,
            CRect(10, 10, W-100, H-10), this, 800);
        m_list.SetFont(pF);

        auto mkBtn = [&](const wchar_t* t, int y, UINT id) {
            CButton* b = new CButton;
            b->Create(t, WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
                CRect(W-88, y, W-8, y+26), this, id);
            b->SetFont(pF);
        };
        mkBtn(L"Add",         10,   801);
        mkBtn(L"Edit",        42,   802);
        mkBtn(L"Delete",      74,   803);
        mkBtn(L"Export iCal", 114,  804);
        mkBtn(L"Print List",   146,  805);
        mkBtn(L"Close",       H-36, IDOK);

        RebuildList();
        return TRUE;
    }

    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override
    {
        switch (LOWORD(wParam)) {
        case 801: OnAdd();        return TRUE;
        case 802: OnEdit();       return TRUE;
        case 803: OnDelete();     return TRUE;
        case 804: OnExportIcal(); return TRUE;
        case 805: OnPrintList();  return TRUE;
        case 800:
            if (HIWORD(wParam) == LBN_DBLCLK) { OnEdit(); return TRUE; }
            break;
        }
        return CDialog::OnCommand(wParam, lParam);
    }

    void OnOK() override { CDialog::OnOK(); }

    DECLARE_MESSAGE_MAP()

private:
    std::vector<UserEventEntry>& m_events;
    CListBox m_list;

    static const wchar_t* TypeLabel(int t) {
        switch (t) {
        case 0: return L"[Bday]";
        case 1: return L"[Anniv]";
        case 2: return L"[Yahr]";
        default: return L"[Event]";
        }
    }

    std::wstring EventSummary(const UserEventEntry& e) const
    {
        std::wstring s = std::wstring(TypeLabel(e.type)) + L" " + e.name + L"  |";
        if (e.gregMonth > 0) {
            static const wchar_t* kM[] = {
                L"Jan",L"Feb",L"Mar",L"Apr",L"May",L"Jun",
                L"Jul",L"Aug",L"Sep",L"Oct",L"Nov",L"Dec" };
            wchar_t buf[40];
            if (e.gregYear > 0)
                swprintf_s(buf, L" %s %d, %d", kM[e.gregMonth-1], e.gregDay, e.gregYear);
            else
                swprintf_s(buf, L" %s %d", kM[e.gregMonth-1], e.gregDay);
            s += buf;
        }
        if (e.hebMonth > 0) {
            static const wchar_t* kH[] = {
                L"Tishrei",L"Cheshvan",L"Kislev",L"Tevet",L"Shvat",
                L"Adar",L"Adar II",L"Nissan",L"Iyar",L"Sivan",
                L"Tammuz",L"Av",L"Elul" };
            wchar_t buf[48];
            if (e.hebYear > 0)
                swprintf_s(buf, L" %d %s %d", e.hebDay, kH[e.hebMonth-1], e.hebYear);
            else
                swprintf_s(buf, L" %d %s", e.hebDay, kH[e.hebMonth-1]);
            s += buf;
            if (e.afterSunset) s += L" (eve)";
        }
        if (e.notify)
        {
            s += e.alarmOffsets.empty() ? L"  | alarms on" : L"  | alarms: " + e.alarmOffsets;
        }
        return s;
    }

    void RebuildList()
    {
        m_list.ResetContent();
        for (const auto& e : m_events)
            m_list.AddString(EventSummary(e).c_str());
    }

    void OnAdd()
    {
        CEventEditDlg dlg(nullptr, this);
        if (dlg.DoModal() == IDOK) {
            m_events.push_back(dlg.entry);
            RebuildList();
            m_list.SetCurSel((int)m_events.size() - 1);
        }
    }

    void OnEdit()
    {
        int sel = m_list.GetCurSel();
        if (sel < 0 || sel >= (int)m_events.size()) return;
        CEventEditDlg dlg(&m_events[sel], this);
        if (dlg.DoModal() == IDOK) {
            m_events[sel] = dlg.entry;
            RebuildList();
            m_list.SetCurSel(sel);
        }
    }

    void OnDelete()
    {
        int sel = m_list.GetCurSel();
        if (sel < 0 || sel >= (int)m_events.size()) return;
        CString msg;
        msg.Format(L"Delete \"%s\"?", m_events[sel].name.c_str());
        if (MessageBox(msg, L"WinLuach", MB_YESNO|MB_ICONQUESTION) == IDYES) {
            m_events.erase(m_events.begin() + sel);
            RebuildList();
            if (sel < (int)m_events.size()) m_list.SetCurSel(sel);
        }
    }

    void OnExportIcal()
    {
        if (m_events.empty()) {
            MessageBox(L"No events to export.", L"WinLuach", MB_OK|MB_ICONINFORMATION);
            return;
        }

        CICalExportDlg optDlg(m_events, this);
        if (optDlg.DoModal() != IDOK) return;

        wchar_t path[MAX_PATH] = L"WinLuach Events.ics";
        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = m_hWnd;
        ofn.lpstrFilter = L"iCalendar Files (*.ics)\0*.ics\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile   = path;
        ofn.nMaxFile    = MAX_PATH;
        ofn.lpstrTitle  = L"Export Events to iCal";
        ofn.lpstrDefExt = L"ics";
        ofn.Flags       = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        if (!GetSaveFileNameW(&ofn)) return;

        if (ExportEventsToIcal(m_events, path,
                               optDlg.m_years, optDlg.m_inclGreg, optDlg.m_inclHeb)) {
            CString info;
            info.Format(L"Exported %d event(s) over %d years.\n\n"
                        L"Import this .ics file into Google Calendar,\n"
                        L"Apple Calendar, Outlook, etc.",
                        (int)m_events.size(), optDlg.m_years);
            MessageBox(info, L"Export Complete", MB_OK|MB_ICONINFORMATION);
        } else {
            MessageBox(L"Failed to write file.", L"WinLuach", MB_OK|MB_ICONERROR);
        }
    }

    void OnPrintList()
    {
        CMainFrame* pFrame = static_cast<CMainFrame*>(
            m_pParentWnd ? m_pParentWnd : AfxGetMainWnd());
        DoPrintEventsList(pFrame);
    }
};
BEGIN_MESSAGE_MAP(CEventsManagerDlg, CDialog) END_MESSAGE_MAP()

// =============================================================================
// CDayViewDlg — detailed day-view popup (opened on double-click)
// =============================================================================

class CDayViewDlg : public CDialog
{
public:
    CDayViewDlg(const GregorianDate& g, CMainFrame* pFrame, CWnd* pParent = nullptr)
        : CDialog(), m_pFrame(pFrame), m_g(g)
    {
        m_pParentWnd = pParent;
        m_h = GregorianToHebrew(g);
        if (pFrame) {
            bool dst = IsDST(g, pFrame->m_location);
            m_z = CalculateZmanim(g, pFrame->m_location, dst);
            m_use24hr = pFrame->m_use24hr;
            m_hols = GetHolidays(m_h, pFrame->m_isIsrael);
            m_omer = GetOmer(m_h);
            m_learning = GetDailyLearning(m_h, g);
            m_parasha = GetParasha(m_h, pFrame->m_isIsrael);
            m_userEvents = pFrame->GetUserEventsForDate(g);
            // Compute candle lighting
            DayOfWeek dow = GetDayOfWeek(g);
            bool isErevYK = (m_h.month == TISHREI && m_h.day == 9);
            bool hasErev2 = false, hasYT2 = false;
            for (const auto& ho : m_hols) {
                if (ho.flags & HOLIDAY_EREV)    hasErev2 = true;
                if (ho.flags & HOLIDAY_YOM_TOV) hasYT2   = true;
            }
            if (dow == FRIDAY || (dow != SHABBAT && (hasErev2 || isErevYK)))
                m_z.candleLighting = AddMinutes(m_z.shkia, -theApp.m_settings.candleLightingMinutes);
            else if (!pFrame->m_isIsrael && hasYT2 && dow != FRIDAY && dow != SHABBAT) {
                HebrewDate hNext = JDNToHebrew(GregorianToJDN(g) + 1);
                auto holsN = GetHolidays(hNext, pFrame->m_isIsrael);
                for (const auto& ho : holsN)
                    if (ho.flags & HOLIDAY_YOM_TOV) { m_z.candleLighting = m_z.tzeit_GRA; break; }
            }
        }
    }

    INT_PTR DoModal() override {
        struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[32]; } b = {};
        b.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_CENTER;
        b.t.dwExtendedStyle = WS_EX_APPWINDOW;
        b.t.cx = 320; b.t.cy = 520;
        wcscpy_s(b.title, L"Day Details");
        if (!InitModalIndirect((DLGTEMPLATE*)&b, m_pParentWnd)) return -1;
        return CDialog::DoModal();
    }

protected:
    BOOL OnInitDialog() override {
        CDialog::OnInitDialog();
        SetWindowText(L"Day Details");
        HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CFont* pF = CFont::FromHandle(hF);
        CRect rc; GetClientRect(&rc);
        int W = rc.Width(), H = rc.Height();
        m_btnPrint.Create(L"Print", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
            CRect(W/2-110, H-34, W/2-8, H-8), this, ID_PRINT_DAY_VIEW);
        m_btnPrint.SetFont(pF);
        m_btnClose.Create(L"Close", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
            CRect(W/2+8, H-34, W/2+110, H-8), this, IDCANCEL);
        m_btnClose.SetFont(pF);
        return TRUE;
    }

    afx_msg void OnSize(UINT nType, int cx, int cy) {
        CDialog::OnSize(nType, cx, cy);
        if (m_btnPrint.GetSafeHwnd()) {
            m_btnPrint.MoveWindow(cx/2-110, cy-34, 102, 26);
            m_btnClose.MoveWindow(cx/2+8,   cy-34, 102, 26);
        }
        Invalidate(FALSE);
    }

    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override {
        if (LOWORD(wParam) == ID_PRINT_DAY_VIEW) {
            DoPrintDay(m_g, m_pFrame);
            return TRUE;
        }
        return CDialog::OnCommand(wParam, lParam);
    }

    afx_msg void OnPaint() {
        CPaintDC dc(this);
        CRect rcClient; GetClientRect(&rcClient);
        int W = rcClient.Width(), H = rcClient.Height();

        dc.FillSolidRect(rcClient, RGB(250, 250, 252));

        HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CFont* pF = CFont::FromHandle(hF);
        dc.SelectObject(pF);

        // --- Header band ---
        int hdrH = 56;
        dc.FillSolidRect(CRect(0, 0, W, hdrH), RGB(50, 80, 140));

        CFont fBig;
        fBig.CreateFont(-18, 0,0,0, FW_BOLD,0,0,0, DEFAULT_CHARSET,0,0,
            CLEARTYPE_QUALITY, DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
        dc.SelectObject(&fBig);
        dc.SetBkMode(TRANSPARENT);
        dc.SetTextColor(RGB(255,255,255));

        static const wchar_t* kDowN[] = {
            L"Sunday",L"Monday",L"Tuesday",L"Wednesday",L"Thursday",L"Friday",L"Shabbos"};
        CString gregStr;
        gregStr.Format(L"%s,  %s %d,  %d",
            kDowN[(int)GetDayOfWeek(m_g)],
            GregorianMonthName(m_g.month).c_str(), m_g.day, m_g.year);
        dc.DrawText(gregStr, CRect(8, 4, W-4, hdrH/2+4), DT_LEFT|DT_VCENTER|DT_SINGLELINE);

        bool leap = IsHebrewLeapYear(m_h.year);
        CString hebStr;
        hebStr.Format(L"%d %s %d",
            m_h.day, HebrewMonthName(m_h.month, leap).c_str(), m_h.year);
        dc.SelectObject(pF);
        dc.SetTextColor(RGB(200, 220, 255));
        dc.DrawText(hebStr, CRect(8, hdrH/2+4, W-8, hdrH-4), DT_RIGHT|DT_VCENTER|DT_SINGLELINE);

        int y = hdrH + 4;

        // Section header helper
        auto drawSection = [&](const wchar_t* lbl) {
            dc.FillSolidRect(CRect(0, y, W, y+18), RGB(200, 212, 240));
            dc.SetBkMode(TRANSPARENT);
            dc.SetTextColor(RGB(30, 50, 110));
            dc.SelectObject(pF);
            dc.DrawText(lbl, CRect(6, y, W-4, y+18), DT_LEFT|DT_VCENTER|DT_SINGLELINE);
            y += 19;
        };

        // Row helper: label (gray) on left, value (colored) on right portion
        auto drawRow = [&](const wchar_t* lbl, const std::wstring& val, COLORREF clr) {
            if (y + 17 >= H - 40) return;
            dc.SetBkMode(TRANSPARENT);
            dc.SelectObject(pF);
            dc.SetTextColor(RGB(120,120,120));
            dc.DrawText(lbl, CRect(8, y, 150, y+16), DT_LEFT|DT_TOP|DT_SINGLELINE);
            dc.SetTextColor(clr);
            dc.DrawText(val.c_str(), -1, CRect(152, y, W-4, y+16),
                DT_LEFT|DT_TOP|DT_SINGLELINE|DT_END_ELLIPSIS);
            y += 17;
        };

        // --- Holidays & Special Days ---
        bool hasSpecial = !m_hols.empty() || !m_parasha.name.empty() || m_omer.isOmerDay;
        if (hasSpecial) {
            drawSection(L"Holidays & Special Days");
            for (auto& h : m_hols) {
                if (h.flags & (HOLIDAY_SHABBOS_MEVAR | HOLIDAY_SPECIAL_SHAB)) continue;
                std::wstring txt = h.name;
                if (!h.subtitle.empty()) txt += L" - " + h.subtitle;
                drawRow(L"", txt, RGB(170, 25, 25));
            }
            if (!m_parasha.name.empty()) {
                std::wstring pt = L"Parshas " + m_parasha.name;
                if (m_parasha.isCombined && !m_parasha.name2.empty())
                    pt += L" - " + m_parasha.name2;
                drawRow(L"Parasha:", pt, RGB(25, 70, 160));
            }
            if (m_omer.isOmerDay) {
                CString ot; ot.Format(L"Day %d of the Omer", m_omer.day);
                drawRow(L"Omer:", (LPCWSTR)ot, RGB(90, 50, 140));
            }
        }

        // --- Calendar events ---
        if (!m_userEvents.empty()) {
            drawSection(L"Calendar Events");
            for (auto& ev : m_userEvents)
                drawRow(L"", ev, RGB(90, 20, 110));
        }

        // --- Daily Learning ---
        bool anyLearn = m_pFrame &&
            (!m_learning.dafYomi.empty() || !m_learning.yerushalmi.empty() ||
             !m_learning.halachaYomit.empty() || !m_learning.mishnaYomit.empty() ||
             !m_learning.tanachYomi.empty());
        if (anyLearn) {
            drawSection(L"Daily Learning");
                if (!m_learning.dafYomi.empty())
                drawRow(L"Daf Yomi:",      m_learning.dafYomi,      RGB(0, 70, 160));
            if (!m_learning.yerushalmi.empty())
                drawRow(L"Yerushalmi:",    m_learning.yerushalmi,   RGB(0, 100, 100));
            if (!m_learning.halachaYomit.empty())
                drawRow(L"Halacha Yomit:", m_learning.halachaYomit, RGB(90, 50, 0));
            if (!m_learning.mishnaYomit.empty())
                drawRow(L"Mishna Yomit:",  m_learning.mishnaYomit,  RGB(0, 90, 0));
            if (!m_learning.tanachYomi.empty())
                drawRow(L"Tanach Yomi:",   m_learning.tanachYomi,   RGB(70, 0, 70));
        }

        // --- Special Times (above regular zmanim) ---
        {
            DayOfWeek dow = GetDayOfWeek(m_g);
            bool isShabbos     = (dow == SHABBAT);
            bool isFriday      = (dow == FRIDAY);
            bool isErevYK      = (m_h.month == TISHREI && m_h.day == 9);
            bool isErevPesach  = (m_h.month == NISSAN  && m_h.day == 14);
            bool isLagBaOmer   = (m_h.month == IYAR    && m_h.day == 18);
            bool isErevTishaBav= (m_h.month == AV      && m_h.day == 8 && !isShabbos);
            bool isTishaBav    = (m_h.month == AV      && m_h.day == 9 && !isShabbos);
            bool is10Av        = (m_h.month == AV      && m_h.day == 10);
            bool isFriAfterPes = false, isShabbAfterPes = false;
            bool isFriAfterRH  = false, isShabbAfterRH  = false;
            {
                GregorianDate gPrev(m_g); long jn = GregorianToJDN(m_g);
                HebrewDate hp7 = JDNToHebrew(jn - 7);
                bool pesachLastWeek = (hp7.month == NISSAN && hp7.day >= 15 && hp7.day <= 21)
                                   || (m_h.month == NISSAN && m_h.day >= 22 && m_h.day <= 28);
                bool rhLastWeek = (hp7.month == TISHREI && hp7.day == 1)
                               || (m_h.month == TISHREI && m_h.day <= 7);
                for (int i = 1; i <= 7; i++) {
                    HebrewDate hi = JDNToHebrew(jn - i);
                    if (hi.month == NISSAN && hi.day == 22) { pesachLastWeek = true; break; }
                }
                for (int i = 1; i <= 7; i++) {
                    HebrewDate hi = JDNToHebrew(jn - i);
                    if (hi.month == TISHREI && hi.day == 2) { rhLastWeek = true; break; }
                }
                isFriAfterPes   = isFriday  && pesachLastWeek;
                isShabbAfterPes = isShabbos && pesachLastWeek;
                isFriAfterRH    = isFriday  && rhLastWeek;
                isShabbAfterRH  = isShabbos && rhLastWeek;
            }
            bool hasYT = false, hasFast = false;
            for (const auto& ho : m_hols) {
                if (ho.flags & HOLIDAY_YOM_TOV) hasYT   = true;
                if (ho.flags & HOLIDAY_FAST)    hasFast = true;
            }

            bool anySpecial = isShabbos || (hasFast && !isShabbos) || isLagBaOmer
                || isErevTishaBav || isTishaBav || is10Av || isErevYK || isErevPesach
                || isFriAfterPes || isShabbAfterPes || isFriAfterRH || isShabbAfterRH;

            if (anySpecial) {
                drawSection(L"Special Times");
                auto boldRow = [&](const wchar_t* lbl, const TimeOfDay& t, COLORREF clr) {
                    if (!t.IsValid() || y + 17 >= H - 40) return;
                    CFont fB;
                    fB.CreateFont(-14,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,
                        CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
                    dc.SetBkMode(TRANSPARENT);
                    dc.SetTextColor(RGB(120,120,120));
                    dc.SelectObject(pF);
                    dc.DrawText(lbl, CRect(8, y, 150, y+16), DT_LEFT|DT_TOP|DT_SINGLELINE);
                    dc.SelectObject(&fB);
                    dc.SetTextColor(clr);
                    dc.DrawText(FormatTime(t, m_use24hr).c_str(), -1,
                        CRect(152, y, W-4, y+16), DT_LEFT|DT_TOP|DT_SINGLELINE);
                    y += 17;
                };
                auto noteRow = [&](const wchar_t* txt) {
                    if (y + 17 >= H - 40) return;
                    dc.SetBkMode(TRANSPARENT); dc.SelectObject(pF);
                    dc.SetTextColor(RGB(130, 60, 0));
                    dc.DrawText(txt, CRect(8, y, W-4, y+32),
                        DT_LEFT|DT_TOP|DT_WORDBREAK);
                    y += 35;
                };

                if (isShabbos && m_z.tzeitShabbat.IsValid())
                    boldRow(L"Tzeis Shabbos:", m_z.tzeitShabbat, RGB(0, 120, 40));
                if (hasFast && !isShabbos && m_z.tzeit_GRA.IsValid())
                    boldRow(L"Fast Ends:", m_z.tzeit_GRA, RGB(180, 0, 0));
                if (isLagBaOmer && m_z.chatzot.IsValid())
                    boldRow(L"Chatzos:", m_z.chatzot, RGB(100, 50, 0));
                if (isErevTishaBav && m_z.shkia.IsValid())
                    boldRow(L"Shkia (Fast Begins):", m_z.shkia, RGB(180, 0, 0));
                if (isTishaBav && m_z.chatzot.IsValid())
                    boldRow(L"Chatzos:", m_z.chatzot, RGB(100, 50, 0));
                if (is10Av && m_z.chatzot.IsValid())
                    boldRow(L"Chatzos (Midday):", m_z.chatzot, RGB(100, 50, 0));
                if (isErevYK) {
                    if (m_z.chatzot.IsValid())    boldRow(L"Chatzos:",         m_z.chatzot,          RGB(100, 50, 0));
                    if (m_z.shkia.IsValid())       boldRow(L"Shkia (Fast Beg.):", m_z.shkia,          RGB(180, 0, 0));
                    if (m_z.candleLighting.IsValid()) boldRow(L"Candle Lighting:", m_z.candleLighting, RGB(200, 80, 0));
                }
                if (isErevPesach && m_z.hanetz.IsValid() && m_z.shaahZmanit_GRA > 0) {
                    int szMin = (int)round(m_z.shaahZmanit_GRA);
                    auto addSha = [&](int sha) -> TimeOfDay {
                        int tot = m_z.hanetz.hour*60 + m_z.hanetz.minute + sha * szMin;
                        return TimeOfDay{ tot/60, tot%60 };
                    };
                    boldRow(L"Eat Chametz by:", addSha(4), RGB(180, 60, 0));
                    boldRow(L"Burn Chametz by:", addSha(5), RGB(180, 30, 0));
                    if (m_z.chatzot.IsValid()) boldRow(L"Chatzos:", m_z.chatzot, RGB(100, 50, 0));
                }
                if (isFriAfterPes || isShabbAfterPes)
                    noteRow(L"Key Challah / Shlissel Challah — Fri/Shab after Pesach");
                if (isFriAfterRH || isShabbAfterRH)
                    noteRow(L"Round Challah — Fri/Shab after Rosh Hashana");
            }
        }

        // --- Zmanim ---
        drawSection(L"Zmanim");
        DisplayZmanimTimes dz = m_pFrame
            ? m_pFrame->BuildDisplayZmanim(m_g, m_z, IsDST(m_g, m_pFrame->m_location))
            : DisplayZmanimTimes{};

        auto zRow = [&](const wchar_t* lbl, const TimeOfDay& t) {
            if (t.IsValid()) drawRow(lbl, FormatTime(t, m_use24hr), RGB(20, 60, 20));
        };
        zRow(L"Alot (custom):",       dz.alot);
        zRow(L"Alot GRA 16.1:",       m_z.alot_GRA);
        zRow(L"Alot MA72:",           m_z.alot_MA72);
        zRow(L"Alot MA90:",           m_z.alot_MA90);
        zRow(L"Misheyakir (custom):", dz.misheyakir);
        zRow(L"Misheyakir 10.2:",     m_z.misheyakir_10);
        zRow(L"Misheyakir 11.5:",     m_z.misheyakir_11);
        zRow(L"Hanetz (Netz):",       m_z.hanetz);
        zRow(L"Sof Shema custom:",    dz.sofShema);
        zRow(L"Sof Shema GRA:",       m_z.sofShema_GRA);
        zRow(L"Sof Shema MA72:",      m_z.sofShema_MA72);
        zRow(L"Sof Shema MA90:",      m_z.sofShema_MA90);
        zRow(L"Sof Tefilla custom:",  dz.sofTefilla);
        zRow(L"Sof Tefilla GRA:",     m_z.sofTefilla_GRA);
        zRow(L"Sof Tefilla MA72:",    m_z.sofTefilla_MA72);
        zRow(L"Sof Tefilla MA90:",    m_z.sofTefilla_MA90);
        zRow(L"Chatzot:",             m_z.chatzot);
        zRow(L"Mincha Gedola custom:",dz.minchaGedola);
        zRow(L"Mincha Gedola GRA:",   m_z.minchaGedola_GRA);
        zRow(L"Mincha Gedola MA72:",  m_z.minchaGedola_MA72);
        zRow(L"Mincha Gedola MA90:",  m_z.minchaGedola_MA90);
        zRow(L"Mincha Ketana custom:",dz.minchaKetana);
        zRow(L"Mincha Ketana GRA:",   m_z.minchaKetana_GRA);
        zRow(L"Mincha Ketana MA72:",  m_z.minchaKetana_MA72);
        zRow(L"Mincha Ketana MA90:",  m_z.minchaKetana_MA90);
        zRow(L"Plag custom:",         dz.plagMincha);
        zRow(L"Plag GRA:",            m_z.plagMincha_GRA);
        zRow(L"Plag MA72:",           m_z.plagMincha_MA72);
        zRow(L"Plag MA90:",           m_z.plagMincha_MA90);
        if (m_z.candleLighting.IsValid()) zRow(L"Candle Lighting:", m_z.candleLighting);
        zRow(L"Shkia (Sunset):",      m_z.shkia);
        zRow(L"Tzais (custom):",      dz.tzeit);
        zRow(L"Tzais GRA 8.5:",       m_z.tzeit_GRA);
        zRow(L"Tzais MA72:",          m_z.tzeit_MA72);
        zRow(L"Tzais MA90:",          m_z.tzeit_MA90);
        if (y + 17 < H - 40 && dz.shaahZmanit > 0.0) {
            int szMin = (int)round(dz.shaahZmanit);
            CString szs; szs.Format(L"%d:%02d  (%d min)", szMin/60, szMin%60, szMin);
            drawRow(L"Sha’a Zmanit:",  (LPCWSTR)szs, RGB(20, 60, 20));
        }
    }

    DECLARE_MESSAGE_MAP()

private:
    CMainFrame*               m_pFrame;
    GregorianDate             m_g;
    HebrewDate                m_h;
    ZmanimResult              m_z;
    bool                      m_use24hr = false;
    std::vector<HolidayInfo>  m_hols;
    OmerInfo                  m_omer;
    DailyLearning             m_learning;
    ParashaInfo               m_parasha;
    std::vector<std::wstring> m_userEvents;
    CButton                   m_btnPrint, m_btnClose;
};

BEGIN_MESSAGE_MAP(CDayViewDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_SIZE()
END_MESSAGE_MAP()

// =============================================================================
// CZmanimPrintDlg — column selection (tab 1) + page setup (tab 2) + print/preview
// =============================================================================

class CZmanimPrintDlg : public CDialog
{
public:
    CZmanimPrintDlg(int year, int month, CMainFrame* pFrame, CWnd* pParent = nullptr)
        : CDialog(), m_year(year), m_month(month), m_pFrame(pFrame)
        { m_pParentWnd = pParent; }

    INT_PTR DoModal() override {
        struct Tmpl { DLGTEMPLATE t; WORD m, c; wchar_t title[30]; } b = {};
        b.t.style = WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|DS_CENTER;
        b.t.dwExtendedStyle = WS_EX_APPWINDOW;
        b.t.cx = 230; b.t.cy = 440;
        wcscpy_s(b.title, L"Print Zmanim Month");
        if (!InitModalIndirect((DLGTEMPLATE*)&b, m_pParentWnd)) return -1;
        return CDialog::DoModal();
    }

protected:
    BOOL OnInitDialog() override {
        CDialog::OnInitDialog();
        HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CFont* pF = CFont::FromHandle(hF);
        CRect rc; GetClientRect(&rc);
        int W = rc.Width(), H = rc.Height();

        // Tab control
        m_tab.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|TCS_TABS,
            CRect(4, 4, W-4, H-44), this, IDC_ZPD_TAB);
        m_tab.SetFont(pF);
        TCITEM ti = {}; ti.mask = TCIF_TEXT;
        ti.pszText = const_cast<LPWSTR>(L"Columns");    m_tab.InsertItem(0, &ti);
        ti.pszText = const_cast<LPWSTR>(L"Page Setup"); m_tab.InsertItem(1, &ti);

        CRect tabRc(4, 4, W-4, H-44);
        m_tab.AdjustRect(FALSE, &tabRc);
        int tx = tabRc.left, ty = tabRc.top, tw = tabRc.Width();

        // --- Tab 0: 15 column checkboxes ---
        static const wchar_t* kNames[15] = {
            L"Alos HaShachar",   L"Misheyakir",       L"Netz (Sunrise)",
            L"Sof Shema (MA)",   L"Sof Shema (GRA)",
            L"Sof Tefilla (MA)", L"Sof Tefilla (GRA)",
            L"Chatzos",          L"Mincha Gedola",     L"Mincha Ketana",
            L"Plag HaMincha",    L"Candle Lighting",
            L"Shkia (Sunset)",   L"Tzais HaKochavim",  L"Sha'ah Zmanit"
        };
        uint32_t savedMask = theApp.m_settings.printZmanimColMask;
        for (int i = 0; i < 15; i++) {
            m_chk[i].Create(kNames[i],
                WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
                CRect(tx+6, ty+4 + i*17, tx+tw-6, ty+4 + i*17+15), this, IDC_ZPD_CHK+i);
            m_chk[i].SetFont(pF);
            m_chk[i].SetCheck((savedMask & (1u << i)) ? BST_CHECKED : BST_UNCHECKED);
        }
        // Sedra column toggle + holiday sub-options
        int sedY = ty + 4 + 15*17 + 4;
        m_chkSedra.Create(L"Show weekly Sedra (Parasha) column",
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
            CRect(tx+6, sedY, tx+tw-6, sedY+15), this, IDC_ZPD_SEDRA);
        m_chkSedra.SetFont(pF);
        // "Also show in Sedra column:" label
        CStatic* pLbl = new CStatic;
        pLbl->Create(L"Also show in Sedra column:",
            WS_CHILD|WS_VISIBLE|SS_LEFT,
            CRect(tx+6, sedY+19, tx+tw-6, sedY+31), this, IDC_ZPD_SEDRA_LBL);
        pLbl->SetFont(pF);
        // 2×2 grid of holiday type checkboxes
        int hcY1 = sedY + 33, hcY2 = sedY + 51, hcMid = tx + 6 + (tw-12)/2;
        m_chkMajor.Create(L"Major Holidays",
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
            CRect(tx+6, hcY1, hcMid-2, hcY1+15), this, IDC_ZPD_HOL_MAJOR);
        m_chkMajor.SetFont(pF); m_chkMajor.SetCheck(BST_CHECKED);
        m_chkMinor.Create(L"Minor Holidays",
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
            CRect(hcMid+2, hcY1, tx+tw-6, hcY1+15), this, IDC_ZPD_HOL_MINOR);
        m_chkMinor.SetFont(pF); m_chkMinor.SetCheck(BST_CHECKED);
        m_chkFast.Create(L"Fast Days",
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
            CRect(tx+6, hcY2, hcMid-2, hcY2+15), this, IDC_ZPD_HOL_FAST);
        m_chkFast.SetFont(pF); m_chkFast.SetCheck(BST_CHECKED);
        m_chkIsrael.Create(L"Israeli National Days",
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
            CRect(hcMid+2, hcY2, tx+tw-6, hcY2+15), this, IDC_ZPD_HOL_ISRAEL);
        m_chkIsrael.SetFont(pF); m_chkIsrael.SetCheck(BST_CHECKED);

        // --- Tab 1: Page setup ---
        int py = ty + 10;
        m_radPortrait.Create(L"Portrait",
            WS_CHILD|WS_TABSTOP|BS_AUTORADIOBUTTON|WS_GROUP,
            CRect(tx+8, py, tx+tw/2-4, py+16), this, IDC_ZPD_PORT);
        m_radPortrait.SetFont(pF);
        m_radLandscape.Create(L"Landscape",
            WS_CHILD|WS_TABSTOP|BS_AUTORADIOBUTTON,
            CRect(tx+tw/2+4, py, tx+tw-8, py+16), this, IDC_ZPD_LAND);
        m_radLandscape.SetFont(pF);
        m_radLandscape.SetCheck(BST_CHECKED);
        py += 28;

        // 12/24 hour display
        m_rad12hr.Create(L"12-hour",
            WS_CHILD|WS_TABSTOP|BS_AUTORADIOBUTTON|WS_GROUP,
            CRect(tx+8, py, tx+tw/2-4, py+16), this, IDC_ZPD_12HR);
        m_rad12hr.SetFont(pF);
        m_rad24hr.Create(L"24-hour",
            WS_CHILD|WS_TABSTOP|BS_AUTORADIOBUTTON,
            CRect(tx+tw/2+4, py, tx+tw-8, py+16), this, IDC_ZPD_24HR);
        m_rad24hr.SetFont(pF);
        m_use24hr = m_pFrame ? m_pFrame->m_use24hr : true;
        if (m_use24hr) m_rad24hr.SetCheck(BST_CHECKED);
        else           m_rad12hr.SetCheck(BST_CHECKED);
        py += 28;

        auto makeLabel = [&](CStatic& s, const wchar_t* txt, int y2) {
            s.Create(txt, WS_CHILD|WS_VISIBLE|SS_LEFT,
                CRect(tx+8, y2, tx+tw-8, y2+14), this);
            s.SetFont(pF);
        };
        auto makeEdit = [&](CEdit& e, float val, int y2) {
            CString sv; sv.Format(L"%.2f", val);
            e.Create(WS_CHILD|WS_TABSTOP|WS_BORDER|ES_LEFT,
                CRect(tx+8, y2, tx+60, y2+18), this, 0);
            e.SetFont(pF);
            e.SetWindowText(sv);
        };
        makeLabel(m_lblTop,   L"Top margin (in):",    py); makeEdit(m_editTop,   0.50f, py+16); py += 40;
        makeLabel(m_lblBot,   L"Bottom margin (in):", py); makeEdit(m_editBot,   0.50f, py+16); py += 40;
        makeLabel(m_lblLeft,  L"Left margin (in):",   py); makeEdit(m_editLeft,  0.50f, py+16); py += 40;
        makeLabel(m_lblRight, L"Right margin (in):",  py); makeEdit(m_editRight, 0.50f, py+16);
        py += 40;
        m_chkFooter.Create(L"Show footer",
            WS_CHILD|WS_TABSTOP|BS_AUTOCHECKBOX,
            CRect(tx+8, py, tx+tw-8, py+16), this, IDC_ZPD_FOOTER);
        m_chkFooter.SetFont(pF);
        m_chkFooter.SetCheck(BST_CHECKED);

        // Initially hide tab 1 controls
        m_radPortrait.ShowWindow(SW_HIDE);
        m_radLandscape.ShowWindow(SW_HIDE);
        m_rad12hr.ShowWindow(SW_HIDE);
        m_rad24hr.ShowWindow(SW_HIDE);
        m_lblTop.ShowWindow(SW_HIDE);
        m_lblBot.ShowWindow(SW_HIDE);
        m_lblLeft.ShowWindow(SW_HIDE);
        m_lblRight.ShowWindow(SW_HIDE);
        m_editTop.ShowWindow(SW_HIDE);
        m_editBot.ShowWindow(SW_HIDE);
        m_editLeft.ShowWindow(SW_HIDE);
        m_editRight.ShowWindow(SW_HIDE);

        // Bottom buttons
        int bY = H - 36;
        m_btnPrint.Create(L"Print", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
            CRect(4, bY, 74, bY+28), this, IDC_ZPD_PRINT);
        m_btnPrint.SetFont(pF);
        m_btnPreview.Create(L"Preview", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
            CRect(W/2-35, bY, W/2+35, bY+28), this, IDC_ZPD_PREVIEW);
        m_btnPreview.SetFont(pF);
        m_btnCancel.Create(L"Cancel", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
            CRect(W-74, bY, W-4, bY+28), this, IDCANCEL);
        m_btnCancel.SetFont(pF);
        return TRUE;
    }

    afx_msg void OnSize(UINT nType, int cx, int cy) {
        CDialog::OnSize(nType, cx, cy);
        if (m_tab.GetSafeHwnd())
            m_tab.MoveWindow(4, 4, max(120, cx - 8), max(120, cy - 48));
        int bY = max(4, cy - 36);
        if (m_btnPrint.GetSafeHwnd())
            m_btnPrint.MoveWindow(4, bY, 70, 28);
        if (m_btnPreview.GetSafeHwnd())
            m_btnPreview.MoveWindow(max(80, cx / 2 - 35), bY, 70, 28);
        if (m_btnCancel.GetSafeHwnd())
            m_btnCancel.MoveWindow(max(152, cx - 74), bY, 70, 28);
    }

    afx_msg void OnTabChanged(NMHDR* /*pNM*/, LRESULT* pResult) {
        int sel = m_tab.GetCurSel();
        for (int i = 0; i < 15; i++)
            m_chk[i].ShowWindow(sel == 0 ? SW_SHOW : SW_HIDE);
        int s0 = (sel == 0) ? SW_SHOW : SW_HIDE;
        int s1 = (sel == 1) ? SW_SHOW : SW_HIDE;
        m_radPortrait.ShowWindow(s1);
        m_radLandscape.ShowWindow(s1);
        m_rad12hr.ShowWindow(s1);
        m_rad24hr.ShowWindow(s1);
        m_lblTop.ShowWindow(s1);
        m_lblBot.ShowWindow(s1);
        m_lblLeft.ShowWindow(s1);
        m_lblRight.ShowWindow(s1);
        m_editTop.ShowWindow(s1);
        m_editBot.ShowWindow(s1);
        m_editLeft.ShowWindow(s1);
        m_editRight.ShowWindow(s1);
        m_chkFooter.ShowWindow(s1);
        m_chkSedra.ShowWindow(s0);
        m_chkMajor.ShowWindow(s0);
        m_chkMinor.ShowWindow(s0);
        m_chkFast.ShowWindow(s0);
        m_chkIsrael.ShowWindow(s0);
        if (CWnd* pL = GetDlgItem(IDC_ZPD_SEDRA_LBL)) pL->ShowWindow(s0);
        *pResult = 0;
    }

    afx_msg void OnPrint() {
        theApp.m_settings.printZmanimColMask = ReadColMask();
        SaveSettings(theApp.m_settings);
        bool sf = !m_chkFooter.GetSafeHwnd() || m_chkFooter.GetCheck() == BST_CHECKED;
        SimplePrint(L"WinLuach Zmanim", ReadPageOpts(), MakeRender(), sf);
    }

    afx_msg void OnPreview() {
        SimplePageOpts opts = ReadPageOpts();
        bool sf = !m_chkFooter.GetSafeHwnd() || m_chkFooter.GetCheck() == BST_CHECKED;
        CSimplePreviewDlg prev(MakeRender(), L"WinLuach Zmanim", !opts.landscape, sf, this);
        prev.DoModal();
    }

    DECLARE_MESSAGE_MAP()

private:
    int          m_year, m_month;
    CMainFrame*  m_pFrame;
    CTabCtrl     m_tab;
    CButton      m_chk[15];
    CButton      m_radPortrait, m_radLandscape;
    CButton      m_rad12hr, m_rad24hr;
    CStatic      m_lblTop, m_lblBot, m_lblLeft, m_lblRight;
    CEdit        m_editTop, m_editBot, m_editLeft, m_editRight;
    CButton      m_btnPrint, m_btnPreview, m_btnCancel;
    CButton      m_chkFooter, m_chkSedra;
    CButton      m_chkMajor, m_chkMinor, m_chkFast, m_chkIsrael;
    bool         m_use24hr = true;

    enum {
        IDC_ZPD_TAB        = 600,
        IDC_ZPD_CHK        = 601,  // 601..615
        IDC_ZPD_PORT       = 616,
        IDC_ZPD_LAND       = 617,
        IDC_ZPD_PRINT      = 618,
        IDC_ZPD_PREVIEW    = 619,
        IDC_ZPD_12HR       = 620,
        IDC_ZPD_24HR       = 621,
        IDC_ZPD_FOOTER     = 622,
        IDC_ZPD_SEDRA      = 623,
        IDC_ZPD_HOL_MAJOR  = 624,
        IDC_ZPD_HOL_MINOR  = 625,
        IDC_ZPD_HOL_FAST   = 626,
        IDC_ZPD_HOL_ISRAEL = 627,
        IDC_ZPD_SEDRA_LBL  = 628,
    };

    uint32_t ReadColMask() {
        uint32_t mask = 0;
        for (int i = 0; i < 15; i++)
            if (m_chk[i].GetCheck() == BST_CHECKED) mask |= (1u << i);
        return mask;
    }

    SimplePageOpts ReadPageOpts() {
        SimplePageOpts opts;
        opts.landscape = (m_radLandscape.GetCheck() == BST_CHECKED);
        auto getF = [](CEdit& e) -> float {
            CString s; e.GetWindowText(s); return (float)_wtof(s);
        };
        opts.mTop   = getF(m_editTop);
        opts.mBot   = getF(m_editBot);
        opts.mLeft  = getF(m_editLeft);
        opts.mRight = getF(m_editRight);
        return opts;
    }

    PageRenderFn MakeRender() {
        uint32_t mask  = ReadColMask();
        bool u24       = (m_rad24hr.GetSafeHwnd()  && m_rad24hr.GetCheck()  == BST_CHECKED);
        bool sedra     = (m_chkSedra.GetSafeHwnd() && m_chkSedra.GetCheck() == BST_CHECKED);
        uint32_t holMask = 0;
        if (sedra) {
            if (m_chkMajor.GetSafeHwnd()  && m_chkMajor.GetCheck()  == BST_CHECKED) holMask |= HOLIDAY_YOM_TOV;
            if (m_chkMinor.GetSafeHwnd()  && m_chkMinor.GetCheck()  == BST_CHECKED) holMask |= HOLIDAY_MINOR;
            if (m_chkFast.GetSafeHwnd()   && m_chkFast.GetCheck()   == BST_CHECKED) holMask |= HOLIDAY_FAST;
            if (m_chkIsrael.GetSafeHwnd() && m_chkIsrael.GetCheck() == BST_CHECKED) holMask |= HOLIDAY_MODERN;
        }
        int yr = m_year, mo = m_month;
        CMainFrame* pF = m_pFrame;
        return [=](CDC* pDC, const CRect& rc, bool sf) {
            DrawZmanimMonthPage(pDC, rc, yr, mo, pF, mask, u24, sf, sedra, holMask);
        };
    }
};

BEGIN_MESSAGE_MAP(CZmanimPrintDlg, CDialog)
    ON_WM_SIZE()
    ON_NOTIFY(TCN_SELCHANGE, CZmanimPrintDlg::IDC_ZPD_TAB, &CZmanimPrintDlg::OnTabChanged)
    ON_BN_CLICKED(CZmanimPrintDlg::IDC_ZPD_PRINT,   &CZmanimPrintDlg::OnPrint)
    ON_BN_CLICKED(CZmanimPrintDlg::IDC_ZPD_PREVIEW, &CZmanimPrintDlg::OnPreview)
END_MESSAGE_MAP()

// =============================================================================
// CSplitterBar — thin draggable CWnd strip between panels
// =============================================================================

class CSplitterBar : public CWnd
{
public:
    enum SplitType { VERT, HORIZ };

    CSplitterBar(CMainFrame* pFrame, SplitType type)
        : m_pFrame(pFrame), m_type(type) {}

    bool CreateBar(CWnd* pParent, UINT id)
    {
        HCURSOR hCur = ::LoadCursor(nullptr,
            m_type == VERT ? IDC_SIZEWE : IDC_SIZENS);
        LPCTSTR cls = AfxRegisterWndClass(0, hCur,
            (HBRUSH)::GetSysColorBrush(COLOR_BTNSHADOW), nullptr);
        return CWnd::Create(cls, nullptr,
            WS_CHILD | WS_VISIBLE,
            CRect(0, 0, 5, 5), pParent, id) != 0;
    }

protected:
    afx_msg void OnLButtonDown(UINT /*nFlags*/, CPoint pt)
    {
        SetCapture();
        m_dragging = true;
        ClientToScreen(&pt);
        m_pFrame->ScreenToClient(&pt);
        m_dragStart = pt;
        m_startVal = (m_type == VERT) ? m_pFrame->m_sidebarW : m_pFrame->m_zmanimH;
    }

    afx_msg void OnMouseMove(UINT /*nFlags*/, CPoint pt)
    {
        if (!m_dragging) return;
        ClientToScreen(&pt);
        m_pFrame->ScreenToClient(&pt);
        if (m_type == VERT)
            m_pFrame->ResizeSidebar(m_startVal + (pt.x - m_dragStart.x));
        else
            m_pFrame->ResizeZmanim(m_startVal - (pt.y - m_dragStart.y));
    }

    afx_msg void OnLButtonUp(UINT /*nFlags*/, CPoint /*pt*/)
    {
        if (m_dragging) { ReleaseCapture(); m_dragging = false; }
    }

    DECLARE_MESSAGE_MAP()

private:
    CMainFrame* m_pFrame;
    SplitType   m_type;
    bool        m_dragging = false;
    CPoint      m_dragStart;
    int         m_startVal = 0;
};

BEGIN_MESSAGE_MAP(CSplitterBar, CWnd)
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_PAINT()
    ON_WM_DESTROY()
    ON_WM_CLOSE()
    ON_WM_SYSCOMMAND()
    ON_WM_TIMER()
    ON_WM_INITMENUPOPUP()
    ON_MESSAGE(WM_WINLUACH_TRAY, &CMainFrame::OnTrayNotify)
    ON_COMMAND(ID_VIEW_PREVDAY, &CMainFrame::OnViewPrevDay)
    ON_COMMAND(ID_VIEW_NEXTDAY, &CMainFrame::OnViewNextDay)
    ON_COMMAND(ID_VIEW_TODAY, &CMainFrame::OnViewToday)
    ON_COMMAND(ID_VIEW_PREVMONTH, &CMainFrame::OnViewPrevMonth)
    ON_COMMAND(ID_VIEW_NEXTMONTH, &CMainFrame::OnViewNextMonth)
    ON_COMMAND(ID_VIEW_PREVYEAR, &CMainFrame::OnViewPrevYear)
    ON_COMMAND(ID_VIEW_NEXTYEAR, &CMainFrame::OnViewNextYear)
    ON_COMMAND(ID_VIEW_PREVDECADE, &CMainFrame::OnViewPrevDecade)
    ON_COMMAND(ID_VIEW_NEXTDECADE, &CMainFrame::OnViewNextDecade)
    ON_COMMAND(ID_OPTIONS_LOCATION, &CMainFrame::OnOptionsLocation)
    ON_COMMAND(ID_OPTIONS_PREFS, &CMainFrame::OnOptionsPrefs)
    ON_COMMAND(ID_HELP_CONTENTS, &CMainFrame::OnHelpContents)
    ON_COMMAND(ID_HELP_ABOUT, &CMainFrame::OnHelpAbout)
    ON_COMMAND(ID_TRAY_OPEN, &CMainFrame::RestoreFromTray)
    ON_COMMAND(ID_TRAY_ABOUT, &CMainFrame::OnHelpAbout)
    ON_COMMAND(ID_TRAY_EXIT, &CMainFrame::OnTrayExit)
    ON_CBN_SELCHANGE(IDC_MONTH_COMBO, &CMainFrame::OnMonthComboChange)
    ON_NOTIFY(UDN_DELTAPOS, IDC_YEAR_SPIN, &CMainFrame::OnYearSpinDelta)
    ON_COMMAND(ID_CAL_PRINT,    &CMainFrame::OnCalPrint)
    ON_COMMAND(ID_CAL_PREVIEW,  &CMainFrame::OnCalPreview)
    ON_MESSAGE(WM_WEBCAL_DONE,  &CMainFrame::OnWebCalDone)
    ON_WM_MOUSEWHEEL()
    ON_COMMAND(ID_FILE_BACKUP,      &CMainFrame::OnFileBackup)
    ON_COMMAND(ID_FILE_RESTORE,     &CMainFrame::OnFileRestore)
    ON_COMMAND(ID_VIEW_ZOOM_IN,     &CMainFrame::OnViewZoomIn)
    ON_COMMAND(ID_VIEW_ZOOM_OUT,    &CMainFrame::OnViewZoomOut)
    ON_COMMAND(ID_VIEW_ZOOM_RESET,  &CMainFrame::OnViewZoomReset)
    ON_COMMAND(ID_VIEW_COUNTDOWN,   &CMainFrame::OnViewCountdownClock)
    ON_COMMAND(ID_VIEW_PANE_SPECIAL, &CMainFrame::OnViewPaneSpecialTimes)
    ON_COMMAND(ID_VIEW_PANE_YEAR,    &CMainFrame::OnViewPaneYearDetails)
    ON_COMMAND(ID_VIEW_PANE_MOLAD,   &CMainFrame::OnViewPaneMolad)
    ON_COMMAND(ID_CAL_GOTO,          &CMainFrame::OnCalGoTo)
    ON_COMMAND(ID_CAL_EVENTS,        &CMainFrame::OnCalEvents)
    ON_COMMAND(ID_FILE_EXPORT_EVT,   &CMainFrame::OnFileExportEvents)
    ON_COMMAND(ID_FILE_IMPORT_EVT,   &CMainFrame::OnFileImportEvents)
    ON_COMMAND(ID_CAL_PRINT_MONTH,   &CMainFrame::OnCalPrintMonth)
    ON_COMMAND(ID_CAL_PRINT_ZMANIM,  &CMainFrame::OnCalPrintZmanim)
    ON_COMMAND(ID_SIDEBAR_TOGGLE,    &CMainFrame::OnSidebarToggle)
    ON_COMMAND(ID_HEB_CIVIL_TOGGLE,  &CMainFrame::OnHebCivilToggle)
    ON_COMMAND(ID_FILE_PRINT_EVENTS, &CMainFrame::OnFilePrintEvents)
    ON_COMMAND(ID_FILE_PIN_TASKBAR,  &CMainFrame::OnPinToTaskbar)
END_MESSAGE_MAP()

CMainFrame::CMainFrame() {}
CMainFrame::~CMainFrame() {}

// Creates the main frame window.
BOOL CMainFrame::Create()
{
    LPCTSTR cls = AfxRegisterWndClass(
        CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
        ::LoadCursor(nullptr, IDC_ARROW),
        (HBRUSH)(COLOR_WINDOW + 1),
        ::LoadIcon(nullptr, IDI_APPLICATION));

    if (!CFrameWnd::Create(cls,
        L"WinLuach - Hebrew Calendar",
        WS_OVERLAPPEDWINDOW,
        CRect(100, 100, 1200, 800)))
        return FALSE;

    return TRUE;
}

// =============================================================================
// ON CREATE
// =============================================================================

int CMainFrame::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CFrameWnd::OnCreate(lpcs) == -1)
        return -1;

    ModifyStyleEx(0, WS_EX_COMPOSITED);

    // Build menu
    CMenu menu;
    menu.CreateMenu();

    CMenu fileMenu;
    fileMenu.CreatePopupMenu();
    fileMenu.AppendMenu(MF_STRING, ID_CAL_PRINT,    L"&Print...\tCtrl+P");
    fileMenu.AppendMenu(MF_STRING, ID_CAL_PREVIEW,  L"Print Pre&view...");
    fileMenu.AppendMenu(MF_SEPARATOR);
    fileMenu.AppendMenu(MF_STRING, ID_FILE_BACKUP,      L"&Backup Settings...");
    fileMenu.AppendMenu(MF_STRING, ID_FILE_RESTORE,     L"&Restore Settings...");
    fileMenu.AppendMenu(MF_SEPARATOR);
    fileMenu.AppendMenu(MF_STRING, ID_FILE_EXPORT_EVT,  L"Export &Events...");
    fileMenu.AppendMenu(MF_STRING, ID_FILE_IMPORT_EVT,  L"&Import Events...");
    fileMenu.AppendMenu(MF_SEPARATOR);
    fileMenu.AppendMenu(MF_STRING, ID_FILE_PIN_TASKBAR, L"Pin WinLuach to Taskbar");
    fileMenu.AppendMenu(MF_SEPARATOR);
    fileMenu.AppendMenu(MF_STRING, ID_APP_EXIT,         L"E&xit");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)fileMenu.Detach(), L"&File");

    CMenu calMenu;
    calMenu.CreatePopupMenu();
    calMenu.AppendMenu(MF_STRING, ID_VIEW_PREVDAY,    L"Previous &Day\tLeft");
    calMenu.AppendMenu(MF_STRING, ID_VIEW_NEXTDAY,    L"Next D&ay\tRight");
    calMenu.AppendMenu(MF_SEPARATOR);
    calMenu.AppendMenu(MF_STRING, ID_VIEW_TODAY,      L"&Today\tHome");
    calMenu.AppendMenu(MF_STRING, ID_CAL_GOTO,        L"&Go to Date...\tCtrl+G");
    calMenu.AppendMenu(MF_STRING, ID_CAL_EVENTS,      L"&Events (Birthdays, Yahrzeits...)\tCtrl+E");
    calMenu.AppendMenu(MF_SEPARATOR);
    calMenu.AppendMenu(MF_STRING, ID_VIEW_PREVMONTH,  L"&Previous Month\tPage Up");
    calMenu.AppendMenu(MF_STRING, ID_VIEW_NEXTMONTH,  L"&Next Month\tPage Down");
    calMenu.AppendMenu(MF_STRING, ID_VIEW_PREVYEAR,   L"Previous &Year\tCtrl+Left");
    calMenu.AppendMenu(MF_STRING, ID_VIEW_NEXTYEAR,   L"Next Y&ear\tCtrl+Right");
    calMenu.AppendMenu(MF_STRING, ID_VIEW_PREVDECADE, L"Previous De&cade\tCtrl+Alt+Left");
    calMenu.AppendMenu(MF_STRING, ID_VIEW_NEXTDECADE, L"Next Deca&de\tCtrl+Alt+Right");
    calMenu.AppendMenu(MF_SEPARATOR);
    calMenu.AppendMenu(MF_STRING, ID_CAL_PRINT_MONTH,  L"Print &Month\tCtrl+Shift+P");
    calMenu.AppendMenu(MF_STRING, ID_CAL_PRINT_ZMANIM, L"Print &Zmanim Month...\tCtrl+Alt+P");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)calMenu.Detach(), L"&Calendar");

    CMenu viewMenu;
    viewMenu.CreatePopupMenu();
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_ZOOM_IN,    L"Zoom &In\tCtrl++");
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_ZOOM_OUT,   L"Zoom &Out\tCtrl+-");
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_ZOOM_RESET, L"Reset Text &Size\tCtrl+0");
    viewMenu.AppendMenu(MF_SEPARATOR);
    CMenu panesMenu;
    panesMenu.CreatePopupMenu();
    panesMenu.AppendMenu(MF_STRING, ID_VIEW_PANE_SPECIAL, L"&Special Times");
    panesMenu.AppendMenu(MF_STRING, ID_VIEW_PANE_YEAR,    L"&Year Details");
    panesMenu.AppendMenu(MF_STRING, ID_VIEW_PANE_MOLAD,   L"&Molad");
    viewMenu.AppendMenu(MF_POPUP, (UINT_PTR)panesMenu.Detach(), L"&Panes");
    viewMenu.AppendMenu(MF_SEPARATOR);
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_COUNTDOWN,  L"&Countdown Clock...");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)viewMenu.Detach(), L"&View");

    CMenu optMenu;
    optMenu.CreatePopupMenu();
    optMenu.AppendMenu(MF_STRING, ID_OPTIONS_LOCATION, L"&Location...\tCtrl+L");
    optMenu.AppendMenu(MF_STRING, ID_OPTIONS_PREFS,    L"&Preferences...\tCtrl+,");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)optMenu.Detach(), L"&Options");

    CMenu helpMenu;
    helpMenu.CreatePopupMenu();
    helpMenu.AppendMenu(MF_STRING, ID_HELP_ABOUT, L"&About WinLuach...");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)helpMenu.Detach(), L"&Help");

    SetMenu(&menu);
    menu.Detach();

    // Load settings (calls RecreateFonts internally)
    AppSettings& s = theApp.m_settings;
    ApplySettings(s);

    // Today
    m_today = GetTodayGregorian();
    m_todayHebrew = GetTodayHebrew();
    m_viewYear = m_today.year;
    m_viewMonth = m_today.month;
    m_selectedDate = m_today;
    m_selectedHebrew = m_todayHebrew;
    SyncViewToSelected();
    m_isDST = IsDST(m_today, m_location);

    // Child panels
    m_pCalView = new CCalendarView(this);
    m_pCalView->Create(nullptr, nullptr,
        WS_CHILD | WS_VISIBLE,
        CRect(0, 0, 100, 100), this, 1001);

    m_pSidebar = new CSidebarPanel(this);
    m_pSidebar->Create(nullptr, nullptr,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL,
        CRect(0, 0, 100, 100), this, 1002);

    m_pZmanim = new CZmanimPanel(this);
    m_pZmanim->Create(nullptr, nullptr,
        WS_CHILD | WS_VISIBLE,
        CRect(0, 0, 100, 100), this, 1003);

    // Splitter bars
    m_pSplitSidebar = new CSplitterBar(this, CSplitterBar::VERT);
    m_pSplitSidebar->CreateBar(this, 1032);

    m_pSplitZmanim = new CSplitterBar(this, CSplitterBar::HORIZ);
    m_pSplitZmanim->CreateBar(this, 1033);

    // Sidebar toggle button (sits at left edge of nav bar)
    m_btnSidebarToggle.Create(m_sidebarCollapsed ? L">>" : L"<<",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(0, 0, 28, 28), this, ID_SIDEBAR_TOGGLE);

    if (m_sidebarCollapsed) {
        m_pSidebar->ShowWindow(SW_HIDE);
        m_pSplitSidebar->ShowWindow(SW_HIDE);
    }

    // Navigation buttons in the header bar (ASCII-safe labels)
    const DWORD btnStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
    CRect r(0, 0, 64, 28);
    m_btnPrevDecade.Create(L"<< Dec",  btnStyle, r, this, ID_VIEW_PREVDECADE);
    m_btnPrevYear  .Create(L"< Year",  btnStyle, r, this, ID_VIEW_PREVYEAR);
    m_btnPrevMonth .Create(L"< Month", btnStyle, r, this, ID_VIEW_PREVMONTH);
    m_btnPrevDay   .Create(L"< Day",   btnStyle, r, this, ID_VIEW_PREVDAY);
    m_btnToday     .Create(L"Today",   btnStyle, r, this, ID_VIEW_TODAY);
    m_btnNextDay   .Create(L"Day >",   btnStyle, r, this, ID_VIEW_NEXTDAY);
    m_btnNextMonth .Create(L"Month >", btnStyle, r, this, ID_VIEW_NEXTMONTH);
    m_btnNextYear  .Create(L"Year >",  btnStyle, r, this, ID_VIEW_NEXTYEAR);
    m_btnNextDecade.Create(L"Dec >>",  btnStyle, r, this, ID_VIEW_NEXTDECADE);

    m_btnPrint.Create(L"Print", btnStyle, r, this, ID_CAL_PRINT);

    CFont* pNF = &m_fontNormal;
    m_btnPrevDecade.SetFont(pNF);  m_btnPrevYear.SetFont(pNF);
    m_btnPrevMonth .SetFont(pNF);  m_btnPrevDay .SetFont(pNF);
    m_btnToday     .SetFont(&m_fontBold);
    m_btnNextDay   .SetFont(pNF);  m_btnNextMonth.SetFont(pNF);
    m_btnNextYear  .SetFont(pNF);  m_btnNextDecade.SetFont(pNF);
    m_btnPrint     .SetFont(pNF);
    m_btnSidebarToggle.SetFont(pNF);

    // Hebrew/Civil toggle button
    m_btnHebCivil.Create(m_hebrewMonthView ? L"Civil" : L"Heb",
        btnStyle, CRect(0,0,50,24), this, ID_HEB_CIVIL_TOGGLE);
    m_btnHebCivil.SetFont(pNF);

    // Month combo + year edit + year spin
    m_comboMonth.Create(WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL,
        CRect(0, 0, 130, 300), this, IDC_MONTH_COMBO);
    m_comboMonth.SetFont(&m_fontNormal);

    m_editYear.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER | ES_NUMBER,
        CRect(0, 0, 55, 24), this, IDC_YEAR_EDIT);
    m_editYear.SetFont(&m_fontNormal);

    m_spinYear.Create(WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_ARROWKEYS,
        CRect(0, 0, 18, 24), this, IDC_YEAR_SPIN);
    m_spinYear.SetBuddy(&m_editYear);
    m_spinYear.SetRange(1, 9999);

    PopulateMonthCombo();
    UpdateMonthYearControls();

    RefreshZmanim();
    RefreshWebCalendarEvents();
    SetTimer(1, 60000, nullptr);
    if (m_minimizeToTray || m_showTrayIcon)
        AddTrayIcon();
    // Defer today's event notification so window is fully visible first
    SetTimer(3, 1500, nullptr);
    if (m_minimizeToTray && s.minimizeOnStartup)
    {
        ShowWindow(SW_HIDE);
    }
    return 0;
}

// =============================================================================
// ON DESTROY
// =============================================================================

void CMainFrame::OnDestroy()
{
    KillTimer(1);
    if (m_pOptionsDlg && ::IsWindow(m_pOptionsDlg->GetSafeHwnd()))
        m_pOptionsDlg->DestroyWindow();
    if (m_pCountdownClock && ::IsWindow(m_pCountdownClock->GetSafeHwnd()))
        m_pCountdownClock->DestroyWindow();
    RemoveTrayIcon();
    WINDOWPLACEMENT wp = {};
    wp.length = sizeof(wp);
    GetWindowPlacement(&wp);
    AppSettings& s = theApp.m_settings;
    s.windowX = wp.rcNormalPosition.left;
    s.windowY = wp.rcNormalPosition.top;
    s.windowW = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
    s.windowH = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
    s.sidebarCollapsed = m_sidebarCollapsed;
    s.sidebarWidth     = m_sidebarCollapsed ? m_lastSidebarW : m_sidebarW;
    s.zmanimHeight     = m_zmanimH;
    s.paneSpecialTimesVisible = m_paneSpecialTimesVisible;
    s.paneYearDetailsVisible  = m_paneYearDetailsVisible;
    s.paneMoladVisible        = m_paneMoladVisible;
    SaveSettings(s);
    CFrameWnd::OnDestroy();
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 2) { // cleanup temporary tray icon after balloon
        KillTimer(2);
        if (!m_isInTray) RemoveTrayIcon();
        return;
    }
    if (nIDEvent == 3) { // one-shot startup notification
        KillTimer(3);
        CheckTodayEvents();
        return;
    }
    if (nIDEvent == 1)
    {
        GregorianDate today = GetTodayGregorian();
        if (today.year != m_today.year ||
            today.month != m_today.month ||
            today.day != m_today.day)
        {
            m_today = today;
            m_todayHebrew = GetTodayHebrew();
            if (m_isInTray) UpdateTrayIcon();
            if (m_pCalView) m_pCalView->Invalidate(FALSE);
        }
        else if (m_isInTray)
        {
            UpdateTrayIcon();
        }
        CheckZmanNotifications();
        CheckSefirahNotification();
        return;
    }
    CFrameWnd::OnTimer(nIDEvent);
}

void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
    if (!pPopupMenu || bSysMenu)
        return;

    pPopupMenu->CheckMenuItem(ID_VIEW_PANE_SPECIAL,
        MF_BYCOMMAND | (m_paneSpecialTimesVisible ? MF_CHECKED : MF_UNCHECKED));
    pPopupMenu->CheckMenuItem(ID_VIEW_PANE_YEAR,
        MF_BYCOMMAND | (m_paneYearDetailsVisible ? MF_CHECKED : MF_UNCHECKED));
    pPopupMenu->CheckMenuItem(ID_VIEW_PANE_MOLAD,
        MF_BYCOMMAND | (m_paneMoladVisible ? MF_CHECKED : MF_UNCHECKED));
}

void CMainFrame::OnClose()
{
    if (ShouldHideToTrayOnClose())
    {
        HideToTray();
        return;
    }
    CFrameWnd::OnClose();
}

void CMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == SC_CLOSE && ShouldHideToTrayOnClose())
    {
        HideToTray();
        return;
    }
    CFrameWnd::OnSysCommand(nID, lParam);
}

void CMainFrame::OnTrayExit()
{
    RemoveTrayIcon();
    DestroyWindow();
}

bool CMainFrame::ShouldHideToTrayOnClose() const
{
    return m_minimizeToTray &&
        (m_minimizeTrayWhen == 1 || m_minimizeTrayWhen == 2);
}

void CMainFrame::HideToTray()
{
    AddTrayIcon();
    ShowWindow(SW_HIDE);
}

// =============================================================================
// ON SIZE / LAYOUT
// =============================================================================

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
    CFrameWnd::OnSize(nType, cx, cy);
    if (nType == SIZE_MINIMIZED &&
        m_minimizeToTray &&
        (m_minimizeTrayWhen == 0 || m_minimizeTrayWhen == 2))
    {
        AddTrayIcon();
        ShowWindow(SW_HIDE);
        return;
    }
    if (cx > 0 && cy > 0)
        LayoutPanels(cx, cy);
}

void CMainFrame::LayoutPanels(int cx, int cy)
{
    if (!m_pCalView || !m_pSidebar || !m_pZmanim) return;

    int splW = 4;  // splitter bar thickness
    int calTop = HEADER_H + DAY_HDR_H;
    int zmH    = m_zmanimH;
    int sbW    = m_sidebarW;

    int calH = cy - zmH - calTop;
    if (calH < 0) calH = 0;

    if (m_sidebarCollapsed || sbW == 0) {
        if (m_pSidebar->IsWindowVisible())     m_pSidebar->ShowWindow(SW_HIDE);
        if (m_pSplitSidebar && m_pSplitSidebar->IsWindowVisible())
            m_pSplitSidebar->ShowWindow(SW_HIDE);
    } else {
        if (!m_pSidebar->IsWindowVisible())    m_pSidebar->ShowWindow(SW_SHOW);
        if (m_pSplitSidebar && !m_pSplitSidebar->IsWindowVisible())
            m_pSplitSidebar->ShowWindow(SW_SHOW);
        m_pSidebar->MoveWindow(0, 0, sbW, cy);
        if (m_pSplitSidebar) {
            m_pSplitSidebar->MoveWindow(sbW, 0, splW, cy);
            m_pSplitSidebar->BringWindowToTop();
        }
    }

    int sbOff = (m_sidebarCollapsed || sbW == 0) ? 0 : (sbW + splW);

    // Batch all moves atomically to prevent flicker
    HDWP hdwp = BeginDeferWindowPos(4);
    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp, m_pZmanim->GetSafeHwnd(), nullptr,
            sbOff, cy - zmH, cx - sbOff, zmH, SWP_NOZORDER);
        if (m_pSplitZmanim && hdwp)
            hdwp = DeferWindowPos(hdwp, m_pSplitZmanim->GetSafeHwnd(), HWND_TOP,
                sbOff, cy - zmH - splW, cx - sbOff, splW, 0);
        if (hdwp)
            hdwp = DeferWindowPos(hdwp, m_pCalView->GetSafeHwnd(), nullptr,
                sbOff, calTop, cx - sbOff, calH, SWP_NOZORDER);
        if (hdwp)
            EndDeferWindowPos(hdwp);
    }
    else
    {
        m_pZmanim->MoveWindow(sbOff, cy - zmH, cx - sbOff, zmH);
        if (m_pSplitZmanim) {
            m_pSplitZmanim->MoveWindow(sbOff, cy - zmH - splW, cx - sbOff, splW);
            m_pSplitZmanim->BringWindowToTop();
        }
        m_pCalView->MoveWindow(sbOff, calTop, cx - sbOff, calH);
    }

    // ── Sidebar toggle button ─────────────────────────────────────────────────
    if (m_btnSidebarToggle.GetSafeHwnd())
    {
        const int TGL_W = 28, TGL_H = 28;
        int btnY = (38 - TGL_H) / 2;
        m_btnSidebarToggle.MoveWindow(sbOff + 2, btnY, TGL_W, TGL_H);
    }

    // ── Navigation buttons (top strip, y=0..38) ──────────────────────────────
    if (m_btnPrevDecade.GetSafeHwnd())
    {
        const int BTN_W = 64, BTN_H = 28, GAP = 4;
        const int TGL_RESERVE = 34;  // space for toggle button
        int totalW = 9 * BTN_W + 8 * GAP;
        int availW = cx - sbOff - TGL_RESERVE;
        int startX = sbOff + TGL_RESERVE + max(4, (availW - totalW) / 2);
        int btnY   = (38 - BTN_H) / 2;

        auto place = [&](CButton& b) {
            b.MoveWindow(startX, btnY, BTN_W, BTN_H);
            startX += BTN_W + GAP;
        };
        place(m_btnPrevDecade);
        place(m_btnPrevYear);
        place(m_btnPrevMonth);
        place(m_btnPrevDay);
        place(m_btnToday);
        place(m_btnNextDay);
        place(m_btnNextMonth);
        place(m_btnNextYear);
        place(m_btnNextDecade);

        // Print button anchored to the right edge
        if (m_btnPrint.GetSafeHwnd())
            m_btnPrint.MoveWindow(cx - 8 - BTN_W, btnY, BTN_W, BTN_H);
    }

    // ── Month combo + year edit + spin (date display row, y=38..84) ─────────
    if (m_comboMonth.GetSafeHwnd())
    {
        const int COMBO_W = 130, YEAR_W = 55, SPIN_W = 18, CTRL_H = 24, GAP = 4;
        const int TOG_W = 50;  // Heb/Civil toggle
        int totalW  = COMBO_W + GAP + YEAR_W + SPIN_W + GAP + TOG_W;
        int centerX = sbOff + (cx - sbOff) / 2;
        int comboX  = centerX - totalW / 2;
        int yearX   = comboX + COMBO_W + GAP;
        int spinX   = yearX + YEAR_W;
        int togX    = spinX + SPIN_W + GAP;
        int ctrlY   = 38 + (46 - CTRL_H) / 2;
        m_comboMonth.MoveWindow(comboX, ctrlY, COMBO_W, 300);
        m_editYear  .MoveWindow(yearX,  ctrlY, YEAR_W,  CTRL_H);
        m_spinYear  .MoveWindow(spinX,  ctrlY, SPIN_W,  CTRL_H);
        if (m_btnHebCivil.GetSafeHwnd())
            m_btnHebCivil.MoveWindow(togX, ctrlY, TOG_W, CTRL_H);
    }

    Invalidate(FALSE);
}

// =============================================================================
// SPLITTER / SIDEBAR HELPERS
// =============================================================================

void CMainFrame::ResizeSidebar(int w)
{
    w = max(0, min(w, 500));
    if (w > 0 && w < 30) w = 0;  // snap closed when dragged very narrow
    m_sidebarW         = w;
    m_sidebarCollapsed = (w == 0);
    if (w == 0 && m_pSidebar) m_pSidebar->ShowWindow(SW_HIDE);
    else if (w > 0 && m_pSidebar && !m_pSidebar->IsWindowVisible())
        m_pSidebar->ShowWindow(SW_SHOW);
    UpdateSidebarToggleButton();
    CRect rc; GetClientRect(&rc);
    LayoutPanels(rc.Width(), rc.Height());
    Invalidate(FALSE);
}

void CMainFrame::ResizeZmanim(int h)
{
    h = max(50, min(h, 300));
    m_zmanimH = h;
    CRect rc; GetClientRect(&rc);
    LayoutPanels(rc.Width(), rc.Height());
    Invalidate(FALSE);
}

void CMainFrame::UpdateSidebarToggleButton()
{
    if (m_btnSidebarToggle.GetSafeHwnd())
        m_btnSidebarToggle.SetWindowText(m_sidebarCollapsed ? L">>" : L"<<");
}

void CMainFrame::OnSidebarToggle()
{
    if (m_sidebarCollapsed)
    {
        m_sidebarW = (m_lastSidebarW > 10) ? m_lastSidebarW : SIDEBAR_W;
        m_sidebarCollapsed = false;
    }
    else
    {
        m_lastSidebarW = m_sidebarW;
        m_sidebarW     = 0;
        m_sidebarCollapsed = true;
    }
    UpdateSidebarToggleButton();
    CRect rc; GetClientRect(&rc);
    LayoutPanels(rc.Width(), rc.Height());
    Invalidate(FALSE);
}

bool CMainFrame::IsSidebarPaneVisible(SidebarPaneId pane) const
{
    switch (pane)
    {
    case PANE_SPECIAL_TIMES: return m_paneSpecialTimesVisible;
    case PANE_YEAR_DETAILS:  return m_paneYearDetailsVisible;
    case PANE_MOLAD:         return m_paneMoladVisible;
    default:                 return true;
    }
}

void CMainFrame::SetSidebarPaneVisible(SidebarPaneId pane, bool visible)
{
    switch (pane)
    {
    case PANE_SPECIAL_TIMES:
        m_paneSpecialTimesVisible = visible;
        theApp.m_settings.paneSpecialTimesVisible = visible;
        break;
    case PANE_YEAR_DETAILS:
        m_paneYearDetailsVisible = visible;
        theApp.m_settings.paneYearDetailsVisible = visible;
        break;
    case PANE_MOLAD:
        m_paneMoladVisible = visible;
        theApp.m_settings.paneMoladVisible = visible;
        break;
    default:
        return;
    }

    SaveSettings(theApp.m_settings);
    if (m_pSidebar)
        m_pSidebar->Invalidate(FALSE);
}

void CMainFrame::OnViewPaneSpecialTimes()
{
    SetSidebarPaneVisible(PANE_SPECIAL_TIMES, !m_paneSpecialTimesVisible);
}

void CMainFrame::OnViewPaneYearDetails()
{
    SetSidebarPaneVisible(PANE_YEAR_DETAILS, !m_paneYearDetailsVisible);
}

void CMainFrame::OnViewPaneMolad()
{
    SetSidebarPaneVisible(PANE_MOLAD, !m_paneMoladVisible);
}

void CMainFrame::OnCalPrintMonth()
{
    CalPrintOptions opts;
    opts.range         = CalPrintOptions::RANGE_MONTH;
    opts.landscape     = theApp.m_settings.printLandscape;
    opts.mTop          = theApp.m_settings.printMarginTop;
    opts.mBot          = theApp.m_settings.printMarginBot;
    opts.mLeft         = theApp.m_settings.printMarginLeft;
    opts.mRight        = theApp.m_settings.printMarginRight;
    opts.includeZmanim = theApp.m_settings.printWeeklyZmanim;
    opts.zmanimColumns = theApp.m_settings.printZmanimColMask;
    opts.showFooter    = theApp.m_settings.printShowFooter;
    opts.use24hr       = theApp.m_settings.use24Hour;
    opts.twoColumns    = theApp.m_settings.printTwoColumns;
    DoPrint(opts, this);
}

void CMainFrame::OnCalPrintZmanim()
{
    CZmanimPrintDlg dlg(m_viewYear, m_viewMonth, this, nullptr);
    dlg.DoModal();
}

void CMainFrame::OnHebCivilToggle()
{
    // Sync both views to the currently selected day, then flip
    m_viewYear        = m_selectedDate.year;
    m_viewMonth       = m_selectedDate.month;
    m_viewHebrewYear  = m_selectedHebrew.year;
    m_viewHebrewMonth = m_selectedHebrew.month;
    m_hebrewMonthView = !m_hebrewMonthView;
    if (m_btnHebCivil.GetSafeHwnd())
        m_btnHebCivil.SetWindowText(m_hebrewMonthView ? L"Civil" : L"Heb");
    PopulateMonthCombo();
    UpdateMonthYearControls();
    if (m_pCalView) { m_pCalView->RebuildCells(); m_pCalView->Invalidate(FALSE); }
}

// =============================================================================
// ON PAINT
// =============================================================================

void CMainFrame::OnPaint()
{
    CPaintDC dc(this);
    CRect rcClient;
    GetClientRect(&rcClient);
    int cx = rcClient.Width();

    CRect rcHeader(m_sidebarW, 0, cx, HEADER_H);
    DrawHeader(&dc, rcHeader);

    CRect rcDayHdr(m_sidebarW, HEADER_H, cx, HEADER_H + DAY_HDR_H);
    DrawDayHeaders(&dc, rcDayHdr);
}

void CMainFrame::DrawHeader(CDC* pDC, const CRect& rc)
{
    // Top 38 px: nav-button strip (darker navy)
    CRect rcNav(rc.left, rc.top, rc.right, rc.top + 38);
    pDC->FillSolidRect(rcNav, RGB(35, 60, 115));

    // Bottom 46 px: date-label strip (blue)
    CRect rcDate(rc.left, rc.top + 38, rc.right, rc.bottom);
    pDC->FillSolidRect(rcDate, CLR_HEADER_BG);

    // Build date strings
    GregorianDate first, last;
    CString gregStr, hebFull;

    if (m_hebrewMonthView)
    {
        HebrewDate hFirst(m_viewHebrewYear, m_viewHebrewMonth, 1);
        HebrewDate hLast(m_viewHebrewYear, m_viewHebrewMonth,
            DaysInHebrewMonth(m_viewHebrewMonth, m_viewHebrewYear));
        first = HebrewToGregorian(hFirst);
        last  = HebrewToGregorian(hLast);

        hebFull.Format(L"%s  %d",
            HebrewMonthName(m_viewHebrewMonth,
                IsHebrewLeapYear(m_viewHebrewYear)).c_str(),
            m_viewHebrewYear);

        std::wstring gregRange = GregorianMonthName(first.month);
        if (first.month != last.month)
            gregRange += L" - " + GregorianMonthName(last.month);
        gregRange += L" " + std::to_wstring(first.year);
        if (first.year != last.year)
            gregRange += L" - " + std::to_wstring(last.year);
        gregStr = gregRange.c_str();
    }
    else
    {
        first = GregorianDate(m_viewYear, m_viewMonth, 1);
        last  = GregorianDate(m_viewYear, m_viewMonth,
            DaysInGregorianMonth(m_viewMonth, m_viewYear));
        gregStr.Format(L"%s %d",
            GregorianMonthName(m_viewMonth).c_str(), m_viewYear);
    }

    HebrewDate hFirst = GregorianToHebrew(first);
    HebrewDate hLast  = GregorianToHebrew(last);

    std::wstring hebStr = HebrewMonthName(hFirst.month, IsHebrewLeapYear(hFirst.year));
    if (hFirst.month != hLast.month)
        hebStr += L" - " + HebrewMonthName(hLast.month, IsHebrewLeapYear(hLast.year));

    if (!m_hebrewMonthView)
    {
        hebFull.Format(L"%s  %d", hebStr.c_str(), hFirst.year);
        if (hFirst.year != hLast.year)
            hebFull.AppendFormat(L" - %d", hLast.year);
    }

    // Draw labels in the date strip
    pDC->SelectObject(&m_fontHeader);
    pDC->SetTextColor(CLR_HEADER_TXT);
    pDC->SetBkMode(TRANSPARENT);

    int w = rcDate.Width();
    CRect rcLeft = rcDate;
    rcLeft.right = rcDate.left + w * 38 / 100;
    rcLeft.left += 12;
    pDC->DrawText(gregStr, rcLeft, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    CRect rcRight = rcDate;
    rcRight.left  = rcDate.left + w * 62 / 100;
    rcRight.right -= 12;
    pDC->DrawText(hebFull, rcRight, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void CMainFrame::DrawDayHeaders(CDC* pDC, const CRect& rc)
{
    pDC->FillSolidRect(rc, RGB(70, 100, 160));

    static const wchar_t* days[] = {
        L"Sunday", L"Monday", L"Tuesday", L"Wednesday",
        L"Thursday", L"Friday", L"Shabbos"
    };

    int colW = rc.Width() / 7;
    pDC->SelectObject(&m_fontBold);
    pDC->SetTextColor(RGB(255, 255, 255));
    pDC->SetBkMode(TRANSPARENT);

    for (int i = 0; i < 7; i++)
    {
        CRect rcCol(rc.left + i * colW, rc.top,
            rc.left + (i + 1) * colW, rc.bottom);
        pDC->DrawText(days[i], -1, rcCol,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

// =============================================================================
// NAVIGATION
// =============================================================================

void CMainFrame::PrevMonth()
{
    ChangeMonth(-1);
}

void CMainFrame::NextMonth()
{
    ChangeMonth(1);
}

void CMainFrame::ChangeDay(int deltaDays)
{
    SelectDate(JDNToGregorian(GregorianToJDN(m_selectedDate) + deltaDays));
}

void CMainFrame::ChangeMonth(int deltaMonths)
{
    if (m_hebrewMonthView)
    {
        int year = m_viewHebrewYear;
        int month = m_viewHebrewMonth + deltaMonths;
        while (month < 1)
        {
            year--;
            month += MonthsInHebrewYear(year);
        }
        while (month > MonthsInHebrewYear(year))
        {
            month -= MonthsInHebrewYear(year);
            year++;
        }
        m_viewHebrewYear = year;
        m_viewHebrewMonth = month;
    }
    else
    {
        m_viewMonth += deltaMonths;
        while (m_viewMonth < 1) { m_viewMonth += 12; m_viewYear--; }
        while (m_viewMonth > 12) { m_viewMonth -= 12; m_viewYear++; }
    }

    UpdateMonthYearControls();

    if (m_pCalView)
    {
        m_pCalView->RebuildCells();
        m_pCalView->Invalidate(FALSE);
    }
    if (m_pSidebar) m_pSidebar->Invalidate(FALSE);
    if (m_pZmanim) m_pZmanim->Invalidate(FALSE);
    Invalidate(FALSE);
}

void CMainFrame::ChangeYear(int deltaYears)
{
    if (m_hebrewMonthView)
    {
        m_viewHebrewYear += deltaYears;
        int months = MonthsInHebrewYear(m_viewHebrewYear);
        if (m_viewHebrewMonth > months) m_viewHebrewMonth = months;
    }
    else
    {
        m_viewYear += deltaYears;
    }

    UpdateMonthYearControls();

    if (m_pCalView)
    {
        m_pCalView->RebuildCells();
        m_pCalView->Invalidate(FALSE);
    }
    Invalidate(FALSE);
}

void CMainFrame::GoToToday()
{
    m_today = GetTodayGregorian();
    m_todayHebrew = GetTodayHebrew();
    SelectDate(m_today);
}

// Sets selected date — only invalidates child panels, not the full frame.
void CMainFrame::SelectDate(const GregorianDate& g)
{
    m_selectedDate = g;
    m_selectedHebrew = GregorianToHebrew(g);

    bool outsideView = false;
    if (m_hebrewMonthView)
    {
        outsideView = (m_selectedHebrew.year != m_viewHebrewYear ||
            m_selectedHebrew.month != m_viewHebrewMonth);
    }
    else
    {
        outsideView = (m_viewYear != g.year || m_viewMonth != g.month);
    }

    if (outsideView)
    {
        SyncViewToSelected();
        UpdateMonthYearControls();
        if (m_pCalView) m_pCalView->RebuildCells();
        Invalidate(FALSE);
    }

    m_isDST = IsDST(g, m_location);
    RefreshZmanim();

    if (m_pCalView)  m_pCalView->Invalidate(FALSE);
    if (m_pSidebar)  m_pSidebar->Invalidate(FALSE);
    if (m_pZmanim)   m_pZmanim->Invalidate(FALSE);
}

void CMainFrame::RefreshZmanim()
{
    m_isDST = IsDST(m_selectedDate, m_location);
    m_zmanim = CalculateZmanim(m_selectedDate, m_location, m_isDST);
    m_zmanim.candleLighting = AddMinutes(m_zmanim.shkia,
        -theApp.m_settings.candleLightingMinutes);
}

DisplayZmanimTimes CMainFrame::BuildDisplayZmanim(
    const GregorianDate& g, const ZmanimResult& z, bool isDst) const
{
    DisplayZmanimTimes out;
    int zmanSh = max(0, min(2, m_zmanimShita));
    double minchaShaah = (zmanSh == 1) ? z.shaahZmanit_MA72
        : (zmanSh == 2) ? z.shaahZmanit_MA90
        : z.shaahZmanit_GRA;

    out.alot = CustomMorningBoundary(g, m_location, isDst, z,
        m_customAlotMode, m_customAlotValue, 16.1);
    out.tzeit = CustomEveningBoundary(g, m_location, isDst, z,
        m_customTzeitMode, m_customTzeitValue, 8.5);
    out.misheyakir = CustomMorningBoundary(g, m_location, isDst, z,
        m_customMisheyakirMode, m_customMisheyakirValue, 10.2);

    TimeOfDay sofStart = CustomMorningBoundary(g, m_location, isDst, z,
        m_customSofZmanMode, m_customSofZmanValue, 16.1);
    TimeOfDay sofEnd = CustomEveningBoundary(g, m_location, isDst, z,
        m_customSofZmanMode, m_customSofZmanValue, 16.1);
    double sofShaah = CalculateShaahZmanit(sofStart, sofEnd);
    out.shaahZmanit = sofShaah > 0.0 ? sofShaah : minchaShaah;
    out.sofShema = AddShaot(sofStart, out.shaahZmanit, 3.0);
    out.sofTefilla = AddShaot(sofStart, out.shaahZmanit, 4.0);

    switch (max(0, min(3, m_customMinchaGedolaPreset)))
    {
    case 0:  out.minchaGedola = AddMinutes(z.chatzot, 30); break;
    case 1:  out.minchaGedola = z.minchaGedola_GRA; break;
    case 2:  out.minchaGedola = z.minchaGedola_MA72; break;
    case 3:  out.minchaGedola = AddMinutes(z.chatzot, (int)round(max(0.0, m_customMinchaGedolaValue))); break;
    default: out.minchaGedola = (zmanSh == 1) ? z.minchaGedola_MA72
        : (zmanSh == 2) ? z.minchaGedola_MA90 : z.minchaGedola_GRA; break;
    }

    switch (max(0, min(2, m_customMinchaKetanaPreset)))
    {
    case 0:  out.minchaKetana = z.minchaKetana_GRA; break;
    case 1:  out.minchaKetana = z.minchaKetana_MA72; break;
    case 2:  out.minchaKetana = AddShaot(z.hanetz, z.shaahZmanit_GRA, max(0.0, m_customMinchaKetanaValue) / 60.0); break;
    default: out.minchaKetana = (zmanSh == 1) ? z.minchaKetana_MA72
        : (zmanSh == 2) ? z.minchaKetana_MA90 : z.minchaKetana_GRA; break;
    }

    switch (max(0, min(2, m_customPlagPreset)))
    {
    case 0:  out.plagMincha = z.plagMincha_GRA; break;
    case 1:  out.plagMincha = z.plagMincha_MA72; break;
    case 2:  out.plagMincha = AddShaot(z.hanetz, z.shaahZmanit_GRA, max(0.0, m_customPlagValue) / 60.0); break;
    default: out.plagMincha = (zmanSh == 1) ? z.plagMincha_MA72
        : (zmanSh == 2) ? z.plagMincha_MA90 : z.plagMincha_GRA; break;
    }

    out.alotLabel = BoundaryLabel(L"Alos Hashachar", m_customAlotMode, m_customAlotValue, false);
    out.misheyakirLabel = BoundaryLabel(L"Misheyakir", m_customMisheyakirMode, m_customMisheyakirValue, false);
    out.sofShemaLabel = BoundaryLabel(L"Sof Shema", m_customSofZmanMode, m_customSofZmanValue, true);
    out.sofTefillaLabel = BoundaryLabel(L"Sof Tefilla", m_customSofZmanMode, m_customSofZmanValue, true);
    out.tzeitLabel = BoundaryLabel(L"Tzeis", m_customTzeitMode, m_customTzeitValue, false);
    return out;
}

void CMainFrame::ApplySettings(const AppSettings& s)
{
    m_location    = s.ToLocation();
    m_use24hr     = s.use24Hour;
    m_isIsrael    = s.isIsrael;
    m_alotShita   = s.alotShita;
    m_tzeitShita  = s.tzeitShita;
    m_zmanimShita = s.zmanimShita;
    m_customAlotMode = max(0, min(2, s.customAlotMode));
    m_customAlotValue = s.customAlotValue > 0.0 ? s.customAlotValue : 16.1;
    m_customMisheyakirMode = max(0, min(2, s.customMisheyakirMode));
    m_customMisheyakirValue = s.customMisheyakirValue > 0.0 ? s.customMisheyakirValue : 10.2;
    m_customSofZmanMode = max(0, min(2, s.customSofZmanMode));
    if (s.customSofZmanValue >= 0.0)
        m_customSofZmanValue = s.customSofZmanValue;
    else if (s.zmanimShita == 1)
    {
        m_customSofZmanMode = 1;
        m_customSofZmanValue = 72.0;
    }
    else if (s.zmanimShita == 2)
    {
        m_customSofZmanMode = 1;
        m_customSofZmanValue = 90.0;
    }
    else
    {
        m_customSofZmanMode = 0;
        m_customSofZmanValue = 0.0;
    }
    m_customTzeitMode = max(0, min(2, s.customTzeitMode));
    m_customTzeitValue = s.customTzeitValue > 0.0 ? s.customTzeitValue : 8.5;
    // v0.8.0 - mirror zmanim bar mask + end-of-fast preset onto the frame
    // so ZmanimPanel can read them without going back through the settings.
    m_zmanimBarMask = s.zmanimBarMask;
    m_customMinchaGedolaPreset = max(0, min(3, s.customMinchaGedolaPreset));
    m_customMinchaKetanaPreset = max(0, min(2, s.customMinchaKetanaPreset));
    m_customPlagPreset = max(0, min(2, s.customPlagPreset));
    m_customEndFastPreset = max(0, min(2, s.customEndFastPreset));
    m_customMinchaGedolaValue = s.customMinchaGedolaValue > 0.0 ? s.customMinchaGedolaValue : 30.0;
    m_customMinchaKetanaValue = s.customMinchaKetanaValue > 0.0 ? s.customMinchaKetanaValue : 570.0;
    m_customPlagValue = s.customPlagValue > 0.0 ? s.customPlagValue : 645.0;
    m_customEndFastValue = s.customEndFastValue > 0.0 ? s.customEndFastValue : 27.0;

    m_colorNormalCell = (COLORREF)s.colorNormalCell;
    m_colorOtherMonthCell = (COLORREF)s.colorOtherMonthCell;
    m_colorTodayCell = (COLORREF)s.colorTodayCell;
    m_colorShabbosCell = (COLORREF)s.colorShabbosCell;
    m_colorYomTovCell = (COLORREF)s.colorYomTovCell;
    m_colorRoshChodeshCell = (COLORREF)s.colorRoshChodeshCell;
    m_colorCholHamoedCell = (COLORREF)s.colorCholHamoedCell;
    m_colorFastDayCell = (COLORREF)s.colorFastDayCell;
    m_colorGregorianText = (COLORREF)s.colorGregorianText;
    m_colorHebrewText = (COLORREF)s.colorHebrewText;
    m_colorHolidayText = (COLORREF)s.colorHolidayText;
    m_colorParshaText = (COLORREF)s.colorParshaText;
    m_colorCivilEventText = (COLORREF)s.colorCivilEventText;
    m_colorHebrewEventText = (COLORREF)s.colorHebrewEventText;
    m_colorOmerText = (COLORREF)s.colorOmerText;
    m_colorLearningText = (COLORREF)s.colorLearningText;
    m_colorCandleText = (COLORREF)s.colorCandleText;
    m_colorMotzText = (COLORREF)s.colorMotzText;

    // Restore persisted layout
    if (s.sidebarWidth > 0)  m_sidebarW = s.sidebarWidth;
    if (s.zmanimHeight > 0)  m_zmanimH  = s.zmanimHeight;
    m_sidebarCollapsed = s.sidebarCollapsed;
    m_lastSidebarW = m_sidebarW;
    if (m_sidebarCollapsed) m_sidebarW = 0;
    m_showParshios = s.showParshios;
    m_showMoadim = s.showMoadim;
    m_showDafYomi = s.showDafYomi;
    m_showYerushalmi = s.showYerushalmi;
    m_showHalachaYomit = s.showHalachaYomit;
    m_showMishnaYomit = s.showMishnaYomit;
    m_showTanachYomi = s.showTanachYomi;
    m_showTrayIcon   = s.showTrayIcon;
    m_minimizeToTray = s.minimizeToTray;
    m_minimizeTrayWhen = s.minimizeTrayWhen;
    m_paneSpecialTimesVisible = s.paneSpecialTimesVisible;
    m_paneYearDetailsVisible  = s.paneYearDetailsVisible;
    m_paneMoladVisible        = s.paneMoladVisible;
    m_hebrewMonthView = s.defaultHebrewMonth;
    RefreshZmanim();
    RecreateFonts();
    // Reassign fonts to header controls — their HFONT handle becomes stale after RecreateFonts.
    if (m_comboMonth.GetSafeHwnd())    m_comboMonth.SetFont(&m_fontNormal);
    if (m_editYear.GetSafeHwnd())      m_editYear.SetFont(&m_fontNormal);
    auto setNavFont = [this](CButton& b) { if (b.GetSafeHwnd()) b.SetFont(&m_fontNormal); };
    setNavFont(m_btnPrevDecade); setNavFont(m_btnPrevYear);  setNavFont(m_btnPrevMonth);
    setNavFont(m_btnPrevDay);    setNavFont(m_btnNextDay);
    if (m_btnToday.GetSafeHwnd()) m_btnToday.SetFont(&m_fontBold);
    setNavFont(m_btnNextMonth);  setNavFont(m_btnNextYear);  setNavFont(m_btnNextDecade);
    setNavFont(m_btnPrint);      setNavFont(m_btnSidebarToggle);  setNavFont(m_btnHebCivil);
    if (m_btnHebCivil.GetSafeHwnd())
        m_btnHebCivil.SetWindowText(m_hebrewMonthView ? L"Civil" : L"Heb");
    if (m_pCalView) { m_pCalView->RebuildCells(); m_pCalView->Invalidate(FALSE); }
    if (m_pSidebar) m_pSidebar->Invalidate(FALSE);
    if (m_pZmanim)  m_pZmanim->Invalidate(FALSE);
}

void CMainFrame::RecreateFonts()
{
    int sz = theApp.m_settings.fontSize;   // 0-6
    int hNorm  = 13 + sz;                  // 13-19pt (default sz=1 → 14pt)
    int hBold  = hNorm + 1;
    int hSmall = hNorm;                    // buttons same size as normal text
    int hHdr   = hNorm + 6;
    auto make = [](CFont& f, int h, int weight) {
        if (f.GetSafeHandle()) f.DeleteObject();
        f.CreateFont(h, 0, 0, 0, weight, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    };
    make(m_fontNormal, hNorm,  FW_NORMAL);
    make(m_fontBold,   hBold,  FW_BOLD);
    make(m_fontSmall,  hSmall, FW_NORMAL);
    make(m_fontHeader, hHdr,   FW_BOLD);
}

// Parses an ICS file from disk and returns the event list.
static std::vector<UserEventInfo> ParseIcsFromPath(const std::wstring& icsPath)
{
    std::vector<UserEventInfo> events;

    std::ifstream f(icsPath, std::ios::binary);
    if (!f.is_open()) return events;

    std::string bytes((std::istreambuf_iterator<char>(f)),
                       std::istreambuf_iterator<char>());
    std::wstring text = DecodeUtf8(bytes);
    if (!text.empty() && text[0] == 0xFEFF)
        text.erase(text.begin());

    // Unfold continuation lines (RFC 5545 §3.1)
    std::wstringstream raw(text);
    std::vector<std::wstring> lines;
    std::wstring rawLine;
    while (std::getline(raw, rawLine))
    {
        if (!rawLine.empty() && rawLine.back() == L'\r')
            rawLine.pop_back();
        if (!rawLine.empty() &&
            (rawLine[0] == L' ' || rawLine[0] == L'\t') &&
            !lines.empty())
        {
            lines.back() += rawLine.substr(1);
        }
        else
        {
            lines.push_back(rawLine);
        }
    }

    bool inEvent = false;
    GregorianDate eventDate;
    std::wstring summary;
    for (const auto& line : lines)
    {
        if (line == L"BEGIN:VEVENT")
        {
            inEvent = true;
            eventDate = GregorianDate();
            summary.clear();
        }
        else if (line == L"END:VEVENT")
        {
            if (inEvent && eventDate.year > 0 && !summary.empty())
                events.push_back({ eventDate, summary });
            inEvent = false;
        }
        else if (inEvent)
        {
            if (line.rfind(L"DTSTART", 0) == 0)
                ParseIcsDate(line, eventDate);
            else if (line.rfind(L"SUMMARY", 0) == 0)
            {
                size_t colon = line.find(L':');
                if (colon != std::wstring::npos)
                    summary = UnescapeIcsText(WebCalTrim(line.substr(colon + 1)));
            }
        }
    }
    return events;
}

void CMainFrame::RefreshWebCalendarEvents()
{
    m_webEvents.clear();

    const auto& cals = theApp.m_settings.webCalendars;
    if (cals.empty()) return;

    std::wstring sp = GetSettingsFilePath();
    size_t sl = sp.find_last_of(L"\\/");
    std::wstring dir = (sl == std::wstring::npos) ? L"." : sp.substr(0, sl);

    // Collect enabled URLs with their cache paths
    struct CalJob { std::wstring url; std::wstring path; };
    std::vector<CalJob> jobs;
    for (int i = 0; i < (int)cals.size(); i++)
    {
        if (!cals[i].enabled || cals[i].url.empty()) continue;
        std::wstring url = cals[i].url;
        if (url.rfind(L"webcals://", 0) == 0)      url.replace(0, 10, L"https://");
        else if (url.rfind(L"webcal://", 0) == 0)  url.replace(0, 9,  L"https://");
        std::wstring path = dir + L"\\webcalendar" + std::to_wstring(i) + L".ics";
        jobs.push_back({ url, path });
    }
    if (jobs.empty()) return;

    // Keep m_icsPath pointing to first cache file (used in OnWebCalDone)
    m_icsPath = jobs[0].path;

    // Load any already-cached files immediately
    for (const auto& j : jobs)
    {
        auto ev = ParseIcsFromPath(j.path);
        m_webEvents.insert(m_webEvents.end(), ev.begin(), ev.end());
    }
    if (!m_webEvents.empty())
    {
        if (m_pCalView)  m_pCalView->Invalidate(FALSE);
        if (m_pSidebar)  m_pSidebar->Invalidate(FALSE);
    }

    // Download fresh copies in background; post WM_WEBCAL_DONE when all done
    HWND hWnd = GetSafeHwnd();
    std::thread([jobs, hWnd]() {
        for (const auto& j : jobs)
            URLDownloadToFileW(nullptr, j.url.c_str(), j.path.c_str(), 0, nullptr);
        ::PostMessage(hWnd, WM_WEBCAL_DONE, 0, 0);
    }).detach();
}

LRESULT CMainFrame::OnWebCalDone(WPARAM, LPARAM)
{
    m_webEvents.clear();
    const auto& cals = theApp.m_settings.webCalendars;
    std::wstring sp = GetSettingsFilePath();
    size_t sl = sp.find_last_of(L"\\/");
    std::wstring dir = (sl == std::wstring::npos) ? L"." : sp.substr(0, sl);
    for (int i = 0; i < (int)cals.size(); i++)
    {
        if (!cals[i].enabled) continue;
        std::wstring path = dir + L"\\webcalendar" + std::to_wstring(i) + L".ics";
        auto ev = ParseIcsFromPath(path);
        m_webEvents.insert(m_webEvents.end(), ev.begin(), ev.end());
    }
    if (m_pCalView)  m_pCalView->Invalidate(FALSE);
    if (m_pSidebar)  m_pSidebar->Invalidate(FALSE);

    // Notify user of any web calendar events matching today
    int styleWeb = theApp.m_settings.notifyWebCalEvents;
    if (styleWeb > 0) {
        GregorianDate today = GetTodayGregorian();
        std::wstring body;
        for (const auto& ev : m_webEvents)
            if (ev.date.year == today.year && ev.date.month == today.month && ev.date.day == today.day)
                body += ev.title + L"\n";
        ShowEventNotification(L"Calendar Events Today", body, styleWeb);
    }

    return 0;
}

std::vector<CalendarEventLine> CMainFrame::GetCalendarEventLinesForDate(const GregorianDate& g) const
{
    std::vector<CalendarEventLine> events;
    if (!theApp.m_settings.showUserEvents)
        return events;

    // Web calendar events (one-time, exact date match)
    for (const auto& ev : m_webEvents)
    {
        if (ev.date.year == g.year && ev.date.month == g.month && ev.date.day == g.day)
            events.push_back({ ev.title, false, true });
    }

    // Personal recurring events (birthday / anniversary / yahrzeit / custom)
    HebrewDate h = GregorianToHebrew(g);
    for (const auto& ev : theApp.m_settings.userEvents)
    {
        static const wchar_t* kTypePrefix[] = { L"Birthday: ", L"Anniversary: ", L"Yahrzeit: ", L"" };
        const wchar_t* prefix = (ev.type >= 0 && ev.type <= 3) ? kTypePrefix[ev.type] : L"";

        // Gregorian recurrence (gregYear == 0 means any year; >0 means first occurrence year)
        if (ev.gregMonth > 0 && ev.gregMonth == g.month && ev.gregDay == g.day
            && (ev.gregYear == 0 || g.year >= ev.gregYear))
            events.push_back({ std::wstring(prefix) + ev.name + L" (civil)", false, false });

        // Hebrew recurrence
        if (ev.hebMonth > 0)
        {
            if (!ev.afterSunset)
            {
                if (h.month == ev.hebMonth && h.day == ev.hebDay
                    && (ev.hebYear == 0 || h.year >= ev.hebYear))
                    events.push_back({ std::wstring(prefix) + ev.name + L" (hebrew)", true, false });
            }
            else
            {
                // afterSunset: display on the day BEFORE the Hebrew date
                HebrewDate nextDay = JDNToHebrew(GregorianToJDN(g) + 1);
                if (nextDay.month == ev.hebMonth && nextDay.day == ev.hebDay
                    && (ev.hebYear == 0 || nextDay.year >= ev.hebYear))
                    events.push_back({ std::wstring(prefix) + ev.name + L" (hebrew eve)", true, false });
            }
        }
    }

    return events;
}

std::vector<std::wstring> CMainFrame::GetUserEventsForDate(const GregorianDate& g) const
{
    std::vector<std::wstring> events;
    for (const auto& ev : GetCalendarEventLinesForDate(g))
        events.push_back(ev.title);
    return events;
}

void CMainFrame::OnCalEvents()
{
    CEventsManagerDlg dlg(theApp.m_settings.userEvents, this);
    dlg.DoModal();
    // Persist changes immediately
    SaveEvents(theApp.m_settings.userEvents);
    if (m_pCalView)  m_pCalView->Invalidate(FALSE);
    if (m_pSidebar)  m_pSidebar->Invalidate(FALSE);
}

void CMainFrame::AddEventForDate(const GregorianDate& g)
{
    UserEventEntry init;
    init.type      = 3;        // Custom
    init.gregMonth = g.month;
    init.gregDay   = g.day;
    init.gregYear  = g.year;
    CEventEditDlg dlg(&init, this);
    if (dlg.DoModal() == IDOK)
    {
        theApp.m_settings.userEvents.push_back(dlg.entry);
        SaveEvents(theApp.m_settings.userEvents);
        if (m_pCalView)  m_pCalView->Invalidate(FALSE);
        if (m_pSidebar)  m_pSidebar->Invalidate(FALSE);
    }
}

void CMainFrame::OnFileExportEvents()
{
    wchar_t path[MAX_PATH] = L"events.json";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = m_hWnd;
    ofn.lpstrFilter = L"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile   = path;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrTitle  = L"Export Events";
    ofn.lpstrDefExt = L"json";
    ofn.Flags       = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&ofn)) return;

    if (ExportEvents(theApp.m_settings.userEvents, path))
        MessageBox(L"Events exported successfully.", L"WinLuach", MB_OK|MB_ICONINFORMATION);
    else
        MessageBox(L"Failed to export events.", L"WinLuach", MB_OK|MB_ICONERROR);
}

static bool IsDupEvent(const UserEventEntry& a, const UserEventEntry& b)
{
    if (a.name != b.name || a.type != b.type) return false;
    bool gregMatch = (a.gregMonth != 0 && b.gregMonth != 0 &&
                      a.gregMonth == b.gregMonth && a.gregDay == b.gregDay);
    bool hebMatch  = (a.hebMonth  != 0 && b.hebMonth  != 0 &&
                      a.hebMonth  == b.hebMonth  && a.hebDay  == b.hebDay);
    return gregMatch || hebMatch;
}

void CMainFrame::OnFileImportEvents()
{
    wchar_t path[MAX_PATH] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = m_hWnd;
    ofn.lpstrFilter = L"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile   = path;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrTitle  = L"Import Events";
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;

    auto imported = ParseEventsFromFile(path);
    if (imported.empty()) {
        MessageBox(L"No events found in that file.", L"WinLuach", MB_OK|MB_ICONWARNING);
        return;
    }

    auto& existing = theApp.m_settings.userEvents;

    // Identify duplicates
    std::vector<int> dupIdx;
    for (int i = 0; i < (int)imported.size(); ++i)
        for (const auto& e : existing)
            if (IsDupEvent(imported[i], e)) { dupIdx.push_back(i); break; }

    // No duplicates — add everything
    if (dupIdx.empty()) {
        int added = (int)imported.size();
        existing.insert(existing.end(), imported.begin(), imported.end());
        SaveEvents(existing);
        if (m_pCalView) m_pCalView->Invalidate(FALSE);
        if (m_pSidebar) m_pSidebar->Invalidate(FALSE);
        CString msg; msg.Format(L"Imported %d event(s).", added);
        MessageBox(msg, L"WinLuach", MB_OK|MB_ICONINFORMATION);
        return;
    }

    // Prompt user
    CString content;
    content.Format(L"%d duplicate(s) found out of %d event(s) to import.\n\nHow would you like to handle them?",
        (int)dupIdx.size(), (int)imported.size());

    TASKDIALOG_BUTTON dupButtons[] = {
        { 101, L"Skip Duplicates\nImport only new events (skip the duplicates)" },
        { 102, L"Add All\nImport everything, including duplicates" },
        { 103, L"Overwrite Duplicates\nReplace existing events with imported versions" },
        { 104, L"Review Each\nDecide individually for each duplicate" },
        { 105, L"Abort\nCancel the import" },
    };
    TASKDIALOGCONFIG tdc = {};
    tdc.cbSize             = sizeof(tdc);
    tdc.hwndParent         = m_hWnd;
    tdc.pszWindowTitle     = L"Import Events";
    tdc.pszMainInstruction = L"Duplicate Events Found";
    tdc.pszContent         = content;
    tdc.dwFlags            = TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION;
    tdc.pButtons           = dupButtons;
    tdc.cButtons           = _countof(dupButtons);
    tdc.nDefaultButton     = 101;

    int nButton = 105;
    TaskDialogIndirect(&tdc, &nButton, nullptr, nullptr);

    if (nButton == 105 || nButton == 0) return; // Abort / cancelled

    // Track which imported events to skip, and which existing to remove (for overwrite)
    std::vector<bool> skipImport(imported.size(), false);
    std::vector<bool> removeExisting(existing.size(), false);

    // Start with all duplicates marked as skipped
    for (int i : dupIdx) skipImport[i] = true;

    if (nButton == 101) {
        // Skip duplicates — already set above
    }
    else if (nButton == 102) {
        // Add All — clear all skips
        for (int i : dupIdx) skipImport[i] = false;
    }
    else if (nButton == 103) {
        // Overwrite — remove existing duplicates, then add imported
        for (int i : dupIdx) {
            skipImport[i] = false;
            for (int j = 0; j < (int)existing.size(); ++j)
                if (IsDupEvent(imported[i], existing[j]))
                    removeExisting[j] = true;
        }
    }
    else if (nButton == 104) {
        // Review each duplicate
        bool skipRemaining = false;
        for (int idx : dupIdx) {
            if (skipRemaining) { skipImport[idx] = true; continue; }

            CString revMsg;
            revMsg.Format(L"Duplicate: \"%s\"", imported[idx].name.c_str());

            TASKDIALOG_BUTTON revButtons[] = {
                { 201, L"Skip\nKeep existing, don't import this one" },
                { 202, L"Overwrite\nReplace existing with the imported version" },
                { 203, L"Add Anyway\nImport as a duplicate" },
                { 204, L"Continue Adding\nSkip this and all remaining duplicates" },
                { 205, L"Abort\nCancel the entire import" },
            };
            TASKDIALOGCONFIG tdc2 = {};
            tdc2.cbSize             = sizeof(tdc2);
            tdc2.hwndParent         = m_hWnd;
            tdc2.pszWindowTitle     = L"Review Duplicate";
            tdc2.pszMainInstruction = L"Duplicate Event";
            tdc2.pszContent         = revMsg;
            tdc2.dwFlags            = TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION;
            tdc2.pButtons           = revButtons;
            tdc2.cButtons           = _countof(revButtons);
            tdc2.nDefaultButton     = 201;

            int nRev = 201;
            TaskDialogIndirect(&tdc2, &nRev, nullptr, nullptr);

            switch (nRev) {
            default:
            case 201: skipImport[idx] = true; break;
            case 202:
                skipImport[idx] = false;
                for (int j = 0; j < (int)existing.size(); ++j)
                    if (IsDupEvent(imported[idx], existing[j]))
                        removeExisting[j] = true;
                break;
            case 203: skipImport[idx] = false; break;
            case 204: skipImport[idx] = true; skipRemaining = true; break;
            case 205: return; // Abort
            }
        }
    }

    // Apply removals (iterate backwards to preserve indices)
    for (int j = (int)existing.size() - 1; j >= 0; --j)
        if (removeExisting[j]) existing.erase(existing.begin() + j);

    // Add non-skipped
    int added = 0;
    for (int i = 0; i < (int)imported.size(); ++i) {
        if (!skipImport[i]) { existing.push_back(imported[i]); ++added; }
    }

    if (added > 0) {
        SaveEvents(existing);
        if (m_pCalView) m_pCalView->Invalidate(FALSE);
        if (m_pSidebar) m_pSidebar->Invalidate(FALSE);
        CString msg; msg.Format(L"Imported %d event(s).", added);
        MessageBox(msg, L"WinLuach", MB_OK|MB_ICONINFORMATION);
    } else {
        MessageBox(L"No new events were imported.", L"WinLuach", MB_OK|MB_ICONINFORMATION);
    }
}

void CMainFrame::OnFilePrintEvents()
{
    DoPrintEventsList(this);
}

// v0.8.0 - Creates a .lnk shortcut targeting this exe and drops it into the
// user's Quick Launch "User Pinned\TaskBar" folder so WinLuach appears in
// the taskbar pinned items list. Uses IShellLinkW + IPersistFile via COM.
void CMainFrame::OnPinToTaskbar()
{
    wchar_t exePath[MAX_PATH] = {};
    if (!GetModuleFileNameW(nullptr, exePath, MAX_PATH))
    {
        MessageBox(L"Could not determine the WinLuach executable path.",
            L"Pin to Taskbar", MB_OK | MB_ICONERROR);
        return;
    }

    wchar_t workDir[MAX_PATH] = {};
    wcscpy_s(workDir, exePath);
    PathRemoveFileSpecW(workDir);

    wchar_t appData[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appData)))
    {
        MessageBox(L"Could not locate the AppData folder.",
            L"Pin to Taskbar", MB_OK | MB_ICONERROR);
        return;
    }

    std::wstring linkDir = std::wstring(appData) +
        L"\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar";
    SHCreateDirectoryExW(nullptr, linkDir.c_str(), nullptr);
    std::wstring linkPath = linkDir + L"\\WinLuach.lnk";

    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    bool initialized = SUCCEEDED(hrInit);

    HRESULT hr = E_FAIL;
    IShellLinkW* pLink = nullptr;
    hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
        IID_IShellLinkW, reinterpret_cast<void**>(&pLink));
    if (SUCCEEDED(hr) && pLink)
    {
        pLink->SetPath(exePath);
        pLink->SetWorkingDirectory(workDir);
        pLink->SetIconLocation(exePath, 0);
        pLink->SetDescription(L"WinLuach - Hebrew Calendar");

        IPersistFile* pPersist = nullptr;
        hr = pLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&pPersist));
        if (SUCCEEDED(hr) && pPersist)
        {
            hr = pPersist->Save(linkPath.c_str(), TRUE);
            pPersist->Release();
        }
        pLink->Release();
    }

    if (initialized)
        CoUninitialize();

    if (SUCCEEDED(hr))
    {
        MessageBox(
            L"WinLuach has been added to your taskbar pinned items. "
            L"You may need to sign out and back in for it to appear.",
            L"Pin to Taskbar", MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        MessageBox(L"Could not create the taskbar pin shortcut.",
            L"Pin to Taskbar", MB_OK | MB_ICONERROR);
    }
}

void CMainFrame::ShowEventNotification(const std::wstring& title,
                                       const std::wstring& body, int style)
{
    if (style <= 0 || body.empty()) return;

    // Toast via tray balloon tip
    if (style == 1 || style == 3) {
        bool addedTemp = false;
        if (!m_isInTray) { AddTrayIcon(); addedTemp = true; }

        NOTIFYICONDATAW nid = {};
        nid.cbSize = sizeof(nid);
        nid.hWnd   = m_hWnd;
        nid.uID    = 1;
        nid.uFlags = NIF_INFO;
        nid.dwInfoFlags = NIIF_INFO;
        nid.uTimeout    = 8000;
        wcsncpy_s(nid.szInfoTitle, title.c_str(),  ARRAYSIZE(nid.szInfoTitle) - 1);
        wcsncpy_s(nid.szInfo,      body.c_str(),   ARRAYSIZE(nid.szInfo)      - 1);
        Shell_NotifyIconW(NIM_MODIFY, &nid);

        if (addedTemp)
            SetTimer(2, 10000, nullptr); // cleanup timer removes temp icon
    }

    // Popup dialog
    if (style == 2 || style == 3) {
        std::wstring msg = body;
        MessageBox(msg.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
    }
}

void CMainFrame::CheckZmanNotifications()
{
    const AppSettings& s = theApp.m_settings;
    if (s.notifyZmanimStyle <= 0 || s.notifyZmanimMask == 0)
        return;

    SYSTEMTIME st = {};
    GetLocalTime(&st);
    GregorianDate today(st.wYear, st.wMonth, st.wDay);
    bool isDst = IsDST(today, m_location);
    ZmanimResult z = CalculateZmanim(today, m_location, isDst);
    z.candleLighting = AddMinutes(z.shkia, -s.candleLightingMinutes);
    HebrewDate h = GregorianToHebrew(today);
    auto hols = GetHolidays(h, m_isIsrael);

    struct NotifyZman { int bit; const wchar_t* label; TimeOfDay time; };
    std::vector<NotifyZman> items = {
        { 0, L"Alos Hashachar", z.alot_GRA },
        { 1, L"Misheyakir", z.misheyakir_10 },
        { 2, L"Hanetz", z.hanetz },
        { 3, L"Sof Zman Shema (GRA)", z.sofShema_GRA },
        { 4, L"Sof Zman Tefilla (GRA)", z.sofTefilla_GRA },
        { 5, L"Chatzos", z.chatzot },
        { 6, L"Mincha Gedola", z.minchaGedola_GRA },
        { 7, L"Mincha Ketana", z.minchaKetana_GRA },
        { 8, L"Plag HaMincha", z.plagMincha_GRA },
        { 9, L"Shkiah", z.shkia },
        { 10, L"Tzeis", z.tzeit_GRA },
        { 11, L"Candle Lighting", z.candleLighting },
        { 15, L"Sof Zman Shema (MA 72 proportional)", z.sofShema_MA72 },
        { 16, L"Sof Zman Tefilla (MA 72 proportional)", z.sofTefilla_MA72 },
        { 17, L"Sof Zman Shema (MA 90 proportional)", z.sofShema_MA90 },
        { 18, L"Sof Zman Tefilla (MA 90 proportional)", z.sofTefilla_MA90 },
    };

    DayOfWeek dow = GetDayOfWeek(today);
    bool isFast = false;
    for (const auto& ho : hols)
        if (ho.flags & HOLIDAY_FAST) { isFast = true; break; }
    if (isFast)
        items.push_back({ 12, L"Fast ends", z.tzeit_GRA });

    bool isErevTishaBav = (h.month == AV && h.day == 8);
    if (isErevTishaBav && dow != SHABBAT && dow != FRIDAY)
        items.push_back({ 12, L"Fast begins", z.shkia });

    bool isErevPesach = (h.month == NISSAN && h.day == 14);
    if (isErevPesach && z.hanetz.IsValid() && z.shaahZmanit_GRA > 0.0)
    {
        items.push_back({ 13, L"Latest time to eat chametz", AddMinutes(z.hanetz, (int)(4.0 * z.shaahZmanit_GRA)) });
        items.push_back({ 14, L"Latest time to burn chametz", AddMinutes(z.hanetz, (int)(5.0 * z.shaahZmanit_GRA)) });
    }

    for (const auto& item : items)
    {
        if (!(s.notifyZmanimMask & (1u << item.bit)) || !item.time.IsValid())
            continue;
        if (item.time.hour != st.wHour || item.time.minute != st.wMinute)
            continue;

        wchar_t key[80];
        swprintf_s(key, L"%04d%02d%02d-%d-%02d%02d", today.year, today.month, today.day,
            item.bit, item.time.hour, item.time.minute);
        std::wstring k = key;
        if (std::find(m_sentZmanNotificationKeys.begin(), m_sentZmanNotificationKeys.end(), k) !=
            m_sentZmanNotificationKeys.end())
            continue;
        m_sentZmanNotificationKeys.push_back(k);

        std::wstring body = std::wstring(item.label) + L": " + FormatTime(item.time, m_use24hr);
        ShowEventNotification(L"WinLuach Zman", body, s.notifyZmanimStyle);
    }

    if (m_sentZmanNotificationKeys.size() > 256)
        m_sentZmanNotificationKeys.erase(m_sentZmanNotificationKeys.begin(),
            m_sentZmanNotificationKeys.begin() + (m_sentZmanNotificationKeys.size() - 128));
}

static int TimeMinutesOfDay(const TimeOfDay& t)
{
    return t.IsValid() ? (t.hour * 60 + t.minute) : -1;
}

static TimeOfDay PickNotificationZmanByBit(int bit, const ZmanimResult& z, const DisplayZmanimTimes& dz)
{
    switch (bit)
    {
    case 0:  return dz.alot;
    case 1:  return dz.misheyakir;
    case 2:  return z.hanetz;
    case 3:  return z.sofShema_GRA;
    case 4:  return z.sofTefilla_GRA;
    case 5:  return z.chatzot;
    case 6:  return dz.minchaGedola;
    case 7:  return dz.minchaKetana;
    case 8:  return dz.plagMincha;
    case 9:  return z.shkia;
    case 10: return dz.tzeit;
    case 11: return z.candleLighting;
    case 15: return z.sofShema_MA72;
    case 16: return z.sofTefilla_MA72;
    case 17: return z.sofShema_MA90;
    case 18: return z.sofTefilla_MA90;
    default: return TimeOfDay();
    }
}

void CMainFrame::CheckSefirahNotification()
{
    const AppSettings& s = theApp.m_settings;
    if (s.notifySefirahStyle <= 0)
        return;

    SYSTEMTIME st = {};
    GetLocalTime(&st);
    GregorianDate today(st.wYear, st.wMonth, st.wDay);
    bool isDst = IsDST(today, m_location);
    ZmanimResult z = CalculateZmanim(today, m_location, isDst);
    z.candleLighting = AddMinutes(z.shkia, -s.candleLightingMinutes);
    DisplayZmanimTimes dz = BuildDisplayZmanim(today, z, isDst);

    int remindHour = 0;
    int remindMinute = 0;
    bool countTomorrow = false;
    if (s.notifySefirahMode == 1)
    {
        TimeOfDay base;
        if (s.notifySefirahBase == 0)
        {
            base = z.shkia;
            countTomorrow = true;
        }
        else if (s.notifySefirahBase == 1)
        {
            base = dz.tzeit;
            countTomorrow = true;
        }
        else
        {
            base = PickNotificationZmanByBit(s.notifySefirahOtherZman, z, dz);
            int shkiaMin = TimeMinutesOfDay(z.shkia);
            int baseMin = TimeMinutesOfDay(base);
            countTomorrow = (shkiaMin >= 0 && baseMin >= shkiaMin);
        }
        if (!base.IsValid())
            return;
        TimeOfDay remind = AddMinutes(base,
            (s.notifySefirahOffsetDir == 0 ? -1 : 1) * max(0, s.notifySefirahOffsetMinutes));
        if (!remind.IsValid())
            return;
        remindHour = remind.hour;
        remindMinute = remind.minute;
    }
    else
    {
        if (!ParseUserClockTime(s.notifySefirahTime, remindHour, remindMinute))
            return;
        int manualMin = remindHour * 60 + remindMinute;
        int shkiaMin = TimeMinutesOfDay(z.shkia);
        countTomorrow = (shkiaMin >= 0 && manualMin >= shkiaMin);
    }

    if (st.wHour != remindHour || st.wMinute != remindMinute)
        return;

    GregorianDate omerDate = today;
    if (countTomorrow)
        omerDate = JDNToGregorian(GregorianToJDN(today) + 1);
    HebrewDate h = GregorianToHebrew(omerDate);
    OmerInfo omer = GetOmer(h);
    if (!omer.isOmerDay)
        return;

    wchar_t key[80];
    swprintf_s(key, L"%04d%02d%02d-sefirah-%04d%02d%02d-%02d%02d",
        today.year, today.month, today.day,
        omerDate.year, omerDate.month, omerDate.day, remindHour, remindMinute);
    std::wstring k = key;
    if (std::find(m_sentZmanNotificationKeys.begin(), m_sentZmanNotificationKeys.end(), k) !=
        m_sentZmanNotificationKeys.end())
        return;
    m_sentZmanNotificationKeys.push_back(k);

    ShowEventNotification(L"Sefiras HaOmer", omer.text, s.notifySefirahStyle);
}

void CMainFrame::CheckTodayEvents()
{
    GregorianDate today  = GetTodayGregorian();
    HebrewDate    todayH = GregorianToHebrew(today);

    // Personal events (birthday/anniversary/yahrzeit)
    int stylePersonal = theApp.m_settings.notifyPersonalEvents;
    if (stylePersonal > 0) {
        static const wchar_t* kP[] = { L"Birthday: ", L"Anniversary: ", L"Yahrzeit: ", L"" };
        std::wstring body;
        for (const auto& ev : theApp.m_settings.userEvents) {
            if (!ev.notify) continue;
            const wchar_t* pre = (ev.type >= 0 && ev.type <= 3) ? kP[ev.type] : L"";
            if (ev.gregMonth == today.month && ev.gregDay == today.day)
                body += std::wstring(pre) + ev.name + L"\n";
            if (ev.hebMonth > 0) {
                if (!ev.afterSunset) {
                    if (todayH.month == ev.hebMonth && todayH.day == ev.hebDay)
                        body += std::wstring(pre) + ev.name + L"\n";
                } else {
                    HebrewDate next = JDNToHebrew(GregorianToJDN(today) + 1);
                    if (next.month == ev.hebMonth && next.day == ev.hebDay)
                        body += std::wstring(pre) + ev.name + L" (eve)\n";
                }
            }
        }
        ShowEventNotification(L"Today's Events", body, stylePersonal);
    }
}

std::pair<std::wstring, std::wstring> CMainFrame::GetCellZmanimLabels(
    const GregorianDate& g, const HebrewDate& h,
    const std::vector<HolidayInfo>& hols) const
{
    std::wstring candleStr, motzStr;

    bool isDSTLocal = IsDST(g, m_location);
    ZmanimResult z  = CalculateZmanim(g, m_location, isDSTLocal);
    z.candleLighting = AddMinutes(z.shkia, -theApp.m_settings.candleLightingMinutes);

    DayOfWeek dow     = GetDayOfWeek(g);
    bool isFriday     = (dow == FRIDAY);
    bool isShabbat    = (dow == SHABBAT);

    auto hasFlag = [&](int flag) -> bool {
        for (const auto& ho : hols)
            if (ho.flags & flag) return true;
        return false;
    };
    bool hasYomTov = hasFlag(HOLIDAY_YOM_TOV);
    bool hasErev   = hasFlag(HOLIDAY_EREV);

    // Peek at next day to detect multi-day YT
    long      jdn     = GregorianToJDN(g);
    HebrewDate hNext  = JDNToHebrew(jdn + 1);
    auto holsNext     = GetHolidays(hNext, m_isIsrael);
    bool nextDayIsYT  = false;
    for (const auto& ho : holsNext)
        if (ho.flags & HOLIDAY_YOM_TOV) { nextDayIsYT = true; break; }

    // Erev Yom Kippur (9 Tishrei) has no HOLIDAY_EREV in the engine — check explicitly
    bool isErevYomKippur = (h.month == TISHREI && h.day == 9);

    // --- Candle string (orange) ---
    if (isFriday && z.candleLighting.IsValid())
    {
        candleStr = L"Cand " + FormatTime(z.candleLighting, m_use24hr);
    }
    else if (!isFriday && !isShabbat
             && (hasErev || isErevYomKippur) && z.candleLighting.IsValid())
    {
        // Erev YT / Erev Yom Kippur on a weekday
        candleStr = L"Cand " + FormatTime(z.candleLighting, m_use24hr);
    }
    else if (!isFriday && !isShabbat && hasYomTov && !m_isIsrael && nextDayIsYT
             && z.tzeit_GRA.IsValid())
    {
        // 1st day YT diaspora — light candles at tzeis for 2nd day
        candleStr = L"Cand " + FormatTime(z.tzeit_GRA, m_use24hr);
    }

    // --- Motz / Chatz string (blue) ---
    if (isShabbat && z.tzeitShabbat.IsValid())
    {
        motzStr = L"Motz " + FormatTime(z.tzeitShabbat, m_use24hr);
    }
    else if (!isShabbat && !isFriday && hasYomTov && !nextDayIsYT && z.tzeit_GRA.IsValid())
    {
        // Last day of YT
        motzStr = L"Motz " + FormatTime(z.tzeit_GRA, m_use24hr);
    }

    // Erev Tisha B'Av (8 Av) — show shkia (fast begins) when not Shabbos
    if (candleStr.empty() && h.month == AV && h.day == 8
        && !isFriday && !isShabbat && z.shkia.IsValid())
        candleStr = L"Shkia " + FormatTime(z.shkia, m_use24hr);

    // Chatzot / chatzot layla — fill motzStr slot if still empty
    if (motzStr.empty())
    {
        if (h.month == IYAR && h.day == 18 && z.chatzot.IsValid())
            motzStr = L"Chatz " + FormatTime(z.chatzot, m_use24hr);       // Lag BaOmer
        else if (h.month == AV && h.day == 9 && z.chatzot.IsValid())
            motzStr = L"Chatz " + FormatTime(z.chatzot, m_use24hr);       // Tisha B'Av
        else if (h.month == AV && h.day == 10 && z.chatzot.IsValid())
            motzStr = L"Chatz " + FormatTime(z.chatzot, m_use24hr);       // 10 Av (day after Tisha B'Av)
        else if (h.month == NISSAN && h.day == 14 && !m_isIsrael
                 && z.chatzot.IsValid())
            motzStr = L"Chatz " + FormatTime(z.chatzot, m_use24hr);       // Erev Pesach diaspora
        else if (h.month == NISSAN && h.day == 15 && !m_isIsrael
                 && z.chatzotLayla.IsValid())
            motzStr = L"Chatz " + FormatTime(z.chatzotLayla, m_use24hr);  // Pesach night 1 diaspora
        else if (theApp.m_settings.showChatzosOnFasts
                 && hasFlag(HOLIDAY_FAST) && !isShabbat && z.chatzot.IsValid())
            motzStr = L"Chatz " + FormatTime(z.chatzot, m_use24hr);       // public fast days (when enabled)
    }

    return { candleStr, motzStr };
}

void CMainFrame::SyncViewToSelected()
{
    m_viewYear = m_selectedDate.year;
    m_viewMonth = m_selectedDate.month;
    m_viewHebrewYear = m_selectedHebrew.year;
    m_viewHebrewMonth = m_selectedHebrew.month;
}

CString CMainFrame::BuildTrayTooltip()
{
    GregorianDate g = GetTodayGregorian();
    HebrewDate hToday = CurrentHebrewDateForTray();
    bool isDst = IsDST(g, m_location);
    ZmanimResult z = CalculateZmanim(g, m_location, isDst);
    z.candleLighting = AddMinutes(z.shkia, -theApp.m_settings.candleLightingMinutes);

    struct TooltipLine { int bit; const wchar_t* label; std::wstring value; };
    std::vector<TooltipLine> items;
    auto addTime = [&](int bit, const wchar_t* label, const TimeOfDay& time) {
        if (time.IsValid())
            items.push_back({ bit, label, FormatTime(time, m_use24hr) });
    };

    addTime(0,  L"Alos GRA",          z.alot_GRA);
    addTime(1,  L"Alos MA72",         z.alot_MA72);
    addTime(2,  L"Alos MA90",         z.alot_MA90);
    addTime(3,  L"Misheyakir 10.2",   z.misheyakir_10);
    addTime(4,  L"Misheyakir 11.5",   z.misheyakir_11);
    addTime(5,  L"Netz",              z.hanetz);
    addTime(6,  L"Sof Shema MA72",    z.sofShema_MA72);
    addTime(7,  L"Sof Shema MA90",    z.sofShema_MA90);
    addTime(8,  L"Sof Shema GRA",     z.sofShema_GRA);
    addTime(9,  L"Sof Tefilla MA72",  z.sofTefilla_MA72);
    addTime(10, L"Sof Tefilla MA90",  z.sofTefilla_MA90);
    addTime(11, L"Sof Tefilla GRA",   z.sofTefilla_GRA);
    addTime(12, L"Chatzos",           z.chatzot);
    addTime(13, L"Mincha Gedola MA72",z.minchaGedola_MA72);
    addTime(14, L"Mincha Gedola MA90",z.minchaGedola_MA90);
    addTime(15, L"Mincha Gedola GRA", z.minchaGedola_GRA);
    addTime(16, L"Mincha Ketana MA72",z.minchaKetana_MA72);
    addTime(17, L"Mincha Ketana MA90",z.minchaKetana_MA90);
    addTime(18, L"Mincha Ketana GRA", z.minchaKetana_GRA);
    addTime(19, L"Plag MA72",         z.plagMincha_MA72);
    addTime(20, L"Plag MA90",         z.plagMincha_MA90);
    addTime(21, L"Plag GRA",          z.plagMincha_GRA);
    addTime(22, L"Shkiah",            z.shkia);
    addTime(23, L"Tzais GRA",         z.tzeit_GRA);
    addTime(24, L"Tzais MA72",        z.tzeit_MA72);
    addTime(25, L"Tzais MA90",        z.tzeit_MA90);
    addTime(26, L"Candles",           z.candleLighting);

    HebrewDate h = GregorianToHebrew(g);
    auto hols = GetHolidays(h, m_isIsrael);
    bool isFast = false;
    for (const auto& ho : hols)
        if (ho.flags & HOLIDAY_FAST) { isFast = true; break; }
    if (isFast)
        addTime(27, L"Fast ends", z.tzeit_GRA);

    DayOfWeek dow = GetDayOfWeek(g);
    bool isErevTishaBav = (h.month == AV && h.day == 8);
    if (isErevTishaBav && dow != SHABBAT && dow != FRIDAY)
        addTime(27, L"Fast begins", z.shkia);

    bool isErevPesach = (h.month == NISSAN && h.day == 14);
    if (isErevPesach && z.hanetz.IsValid() && z.shaahZmanit_GRA > 0.0)
    {
        addTime(28, L"Eat chametz", AddMinutes(z.hanetz, (int)(4.0 * z.shaahZmanit_GRA)));
        addTime(29, L"Burn chametz", AddMinutes(z.hanetz, (int)(5.0 * z.shaahZmanit_GRA)));
    }
    OmerInfo omer = GetOmer(h);
    if (omer.isOmerDay)
    {
        CString os;
        os.Format(L"Day %d", omer.day);
        items.push_back({ 30, L"Today's Omer", (LPCWSTR)os });
    }

    CString tip;
    tip.Format(L"%d %s %d",
        hToday.day,
        HebrewMonthName(hToday.month, IsHebrewLeapYear(hToday.year)).c_str(),
        hToday.year);

    uint32_t mask = theApp.m_settings.trayTooltipZmanimMask;
    for (const auto& item : items)
    {
        if (!(mask & (1u << item.bit)) || item.value.empty())
            continue;
        CString line;
        line.Format(L"\n%s: %s", item.label, item.value.c_str());
        tip += line;
    }
    return tip;
}

void CMainFrame::AddTrayIcon()
{
    if (m_isInTray)
    {
        UpdateTrayIcon();
        return;
    }

    ZeroMemory(&m_trayIcon, sizeof(m_trayIcon));
    m_trayIcon.cbSize = sizeof(m_trayIcon);
    m_trayIcon.hWnd = GetSafeHwnd();
    m_trayIcon.uID = 1;
    m_trayIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
#ifdef NIF_SHOWTIP
    m_trayIcon.uFlags |= NIF_SHOWTIP;
#endif
    m_trayIcon.uCallbackMessage = WM_WINLUACH_TRAY;
    HebrewDate hToday = CurrentHebrewDateForTray();
    m_hTrayDateIcon = CreateTrayDateIcon(hToday);
    m_trayIcon.hIcon = m_hTrayDateIcon ? m_hTrayDateIcon : AfxGetApp()->LoadIcon(IDI_WINLUACH);
    CString tip = BuildTrayTooltip();
    wcsncpy_s(m_trayIcon.szTip, ARRAYSIZE(m_trayIcon.szTip), tip.GetString(), _TRUNCATE);
    Shell_NotifyIcon(NIM_ADD, &m_trayIcon);
    m_trayIcon.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIcon(NIM_SETVERSION, &m_trayIcon);
    m_isInTray = true;
}

void CMainFrame::UpdateTrayIcon()
{
    if (!m_isInTray) return;
    HebrewDate hToday = CurrentHebrewDateForTray();
    CString tip = BuildTrayTooltip();
    wcsncpy_s(m_trayIcon.szTip, ARRAYSIZE(m_trayIcon.szTip), tip.GetString(), _TRUNCATE);
    HICON hNewIcon = CreateTrayDateIcon(hToday);
    if (hNewIcon)
    {
        if (m_hTrayDateIcon)
            DestroyIcon(m_hTrayDateIcon);
        m_hTrayDateIcon = hNewIcon;
        m_trayIcon.hIcon = m_hTrayDateIcon;
        m_trayIcon.uFlags = NIF_TIP | NIF_ICON;
    }
    else
    {
        m_trayIcon.uFlags = NIF_TIP;
    }
#ifdef NIF_SHOWTIP
    m_trayIcon.uFlags |= NIF_SHOWTIP;
#endif
    Shell_NotifyIcon(NIM_MODIFY, &m_trayIcon);
}

std::wstring CMainFrame::HebrewDayLetters(int day) const
{
    static const wchar_t* ones[] = {
        L"",
        L"\u05D0", L"\u05D1", L"\u05D2", L"\u05D3", L"\u05D4",
        L"\u05D5", L"\u05D6", L"\u05D7", L"\u05D8"
    };
    static const wchar_t* tens[] = {
        L"", L"\u05D9", L"\u05DB", L"\u05DC"
    };

    if (day == 15) return L"\u05D8\u05D5";
    if (day == 16) return L"\u05D8\u05D6";
    if (day >= 1 && day <= 9) return ones[day];
    if (day >= 10 && day <= 30)
        return std::wstring(tens[day / 10]) + ones[day % 10];
    return std::to_wstring(day);
}

HICON CMainFrame::CreateTrayDateIcon(const HebrewDate& h) const
{
    int size = GetSystemMetrics(SM_CXSMICON);
    if (size < 16) size = 16;
    if (size > 24) size = 24;

    HDC hdcScreen = ::GetDC(nullptr);
    if (!hdcScreen) return nullptr;

    HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, size, size);
    ::ReleaseDC(nullptr, hdcScreen);
    if (!hBmp) return nullptr;

    HDC hdc = CreateCompatibleDC(nullptr);
    HBITMAP hMask = CreateBitmap(size, size, 1, 1, nullptr);
    HDC hdcMask = CreateCompatibleDC(nullptr);
    HBITMAP hOld = (HBITMAP)SelectObject(hdc, hBmp);
    HBITMAP hOldMask = (HBITMAP)SelectObject(hdcMask, hMask);

    const AppSettings& s = theApp.m_settings;
    RECT fill = { 0, 0, size, size };
    HBRUSH backBrush = CreateSolidBrush((COLORREF)s.trayBackColor);
    FillRect(hdc, &fill, s.trayBackEnabled && backBrush ? backBrush : (HBRUSH)GetStockObject(BLACK_BRUSH));
    FillRect(hdcMask, &fill, (HBRUSH)GetStockObject(s.trayBackEnabled ? BLACK_BRUSH : WHITE_BRUSH));
    if (backBrush)
        DeleteObject(backBrush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, (COLORREF)s.trayTextColor);
    SetBkMode(hdcMask, TRANSPARENT);
    SetTextColor(hdcMask, RGB(0, 0, 0));

    std::wstring txt = (s.trayNumberStyle == 1) ? std::to_wstring(h.day) : HebrewDayLetters(h.day);
    HFONT hFont = nullptr;
    HFONT hOldFont = nullptr;
    HFONT hOldMaskFont = nullptr;
    SIZE textSize = {};

    int firstHeight = s.trayFontSize > 0 ? max(6, s.trayFontSize) : size + 10;
    int lastHeight = s.trayFontSize > 0 ? max(6, s.trayFontSize) : size - 2;
    int weight = s.trayFontBold ? FW_HEAVY : FW_NORMAL;
    BOOL italic = s.trayFontItalic ? TRUE : FALSE;
    const wchar_t* face = s.trayFontFace.empty() ? L"Arial" : s.trayFontFace.c_str();
    for (int height = firstHeight; height >= lastHeight; height--)
    {
        HFONT testFont = CreateFontW(
            -height, 0, 0, 0, weight, italic, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face);
        HFONT old = (HFONT)SelectObject(hdc, testFont);
        SIZE measured = {};
        GetTextExtentPoint32W(hdc, txt.c_str(), (int)txt.size(), &measured);
        SelectObject(hdc, old);

        if (measured.cx <= size + 2 && measured.cy <= size + 2)
        {
            hFont = testFont;
            textSize = measured;
            break;
        }
        DeleteObject(testFont);
    }

    if (!hFont)
    {
        hFont = CreateFontW(
            -size, 0, 0, 0, weight, italic, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face);
        hOldFont = (HFONT)SelectObject(hdc, hFont);
        GetTextExtentPoint32W(hdc, txt.c_str(), (int)txt.size(), &textSize);
    }
    else
    {
        hOldFont = (HFONT)SelectObject(hdc, hFont);
    }
    hOldMaskFont = (HFONT)SelectObject(hdcMask, hFont);

    int x = (size - textSize.cx) / 2;
    int y = (size - textSize.cy) / 2 - 1;
    if (x < -1) x = -1;
    if (y < -2) y = -2;
    TextOutW(hdc, x, y, txt.c_str(), (int)txt.size());
    TextOutW(hdcMask, x, y, txt.c_str(), (int)txt.size());

    SelectObject(hdc, hOldFont);
    SelectObject(hdcMask, hOldMaskFont);
    DeleteObject(hFont);
    SelectObject(hdc, hOld);
    SelectObject(hdcMask, hOldMask);
    DeleteDC(hdc);
    DeleteDC(hdcMask);

    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmColor = hBmp;
    ii.hbmMask = hMask;
    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hBmp);
    DeleteObject(hMask);
    return hIcon;
}

HebrewDate CMainFrame::CurrentHebrewDateForTray()
{
    GregorianDate g = GetTodayGregorian();
    bool isDST = IsDST(g, m_location);
    ZmanimResult z = CalculateZmanim(g, m_location, isDST);

    SYSTEMTIME st = {};
    GetLocalTime(&st);
    bool afterTzeit = z.tzeit_GRA.IsValid() &&
        (st.wHour > z.tzeit_GRA.hour ||
            (st.wHour == z.tzeit_GRA.hour && st.wMinute >= z.tzeit_GRA.minute));

    if (afterTzeit)
        g = JDNToGregorian(GregorianToJDN(g) + 1);

    return GregorianToHebrew(g);
}

void CMainFrame::RemoveTrayIcon()
{
    if (!m_isInTray) return;
    Shell_NotifyIcon(NIM_DELETE, &m_trayIcon);
    m_isInTray = false;
    if (m_hTrayDateIcon)
    {
        DestroyIcon(m_hTrayDateIcon);
        m_hTrayDateIcon = nullptr;
    }
}

void CMainFrame::RestoreFromTray()
{
    ShowWindow(SW_SHOWNORMAL);
    SetForegroundWindow();
}

LRESULT CMainFrame::OnTrayNotify(WPARAM wParam, LPARAM lParam)
{
    if (wParam != 1) return 0;
    if (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONUP)
        RestoreFromTray();
    else if (lParam == WM_RBUTTONUP)
    {
        CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenu(MF_STRING, ID_TRAY_OPEN, L"&Open WinLuach");
        menu.AppendMenu(MF_STRING, ID_TRAY_ABOUT, L"&About...");
        menu.AppendMenu(MF_SEPARATOR);
        menu.AppendMenu(MF_STRING, ID_TRAY_EXIT, L"E&xit");

        CPoint pt;
        GetCursorPos(&pt);
        SetForegroundWindow();
        menu.TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);
    }
    return 0;
}

// =============================================================================
// MENU HANDLERS
// =============================================================================

void CMainFrame::OnViewPrevDay() { ChangeDay(-1); }
void CMainFrame::OnViewNextDay() { ChangeDay(1); }
void CMainFrame::OnViewToday() { GoToToday(); }
void CMainFrame::OnViewPrevMonth() { PrevMonth(); }
void CMainFrame::OnViewNextMonth() { NextMonth(); }
void CMainFrame::OnViewPrevYear() { ChangeYear(-1); }
void CMainFrame::OnViewNextYear() { ChangeYear(1); }
void CMainFrame::OnViewPrevDecade() { ChangeYear(-10); }
void CMainFrame::OnViewNextDecade() { ChangeYear(10); }

void CMainFrame::OnOptionsLocation()
{
    CLocationDlg dlg(m_location, this);
    if (dlg.DoModal() == IDOK)
    {
        m_location = dlg.GetSelectedLocation();
        AppSettings& s = theApp.m_settings;
        s.locationName = m_location.name;
        s.latitude = m_location.latitude;
        s.longitude = m_location.longitude;
        s.elevation = m_location.elevation;
        s.gmtOffset = m_location.gmtOffset;
        s.usesDST = m_location.usesDST;
        if (dlg.GetSelectedIsCustom())
            s.isIsrael = dlg.GetSelectedIsIsrael();
        SaveSettings(s);
        m_isIsrael = s.isIsrael;
        RefreshZmanim();
        if (m_pCalView) { m_pCalView->RebuildCells(); m_pCalView->Invalidate(FALSE); }
        if (m_pZmanim)  m_pZmanim->Invalidate(FALSE);
        if (m_pSidebar) m_pSidebar->Invalidate(FALSE);
    }
}

void CMainFrame::OnOptionsPrefs()
{
    if (m_pOptionsDlg && ::IsWindow(m_pOptionsDlg->GetSafeHwnd()))
    {
        m_pOptionsDlg->ShowWindow(SW_SHOWNORMAL);
        m_pOptionsDlg->SetForegroundWindow();
        return;
    }

    COptionsDlg* dlg = new COptionsDlg(theApp.m_settings, this);
    if (!dlg->CreateModeless(nullptr))
    {
        delete dlg;
        MessageBox(L"Could not open Preferences.", L"WinLuach", MB_OK | MB_ICONERROR);
        return;
    }
    m_pOptionsDlg = dlg;
    dlg->ShowWindow(SW_SHOW);
}

void CMainFrame::ApplyAndSaveSettings(const AppSettings& s)
{
    theApp.m_settings = s;
    ApplySettings(theApp.m_settings);
    SaveSettings(theApp.m_settings);
    ApplyWinLuachShortcut(FOLDERID_Startup, L"WinLuach.lnk", theApp.m_settings.startWithWindows);
    ApplyWinLuachShortcut(FOLDERID_Desktop, L"WinLuach.lnk", theApp.m_settings.desktopShortcut);
    PopulateMonthCombo();
    UpdateMonthYearControls();
    if (m_minimizeToTray || m_showTrayIcon)
        AddTrayIcon();
    else
        RemoveTrayIcon();
    RefreshWebCalendarEvents();
    if (m_pCalView)
    {
        m_pCalView->RebuildCells();
        m_pCalView->Invalidate(FALSE);
    }
    if (m_pZmanim)   m_pZmanim->Invalidate(FALSE);
    if (m_pSidebar)  m_pSidebar->Invalidate(FALSE);
    if (m_pCountdownClock && ::IsWindow(m_pCountdownClock->GetSafeHwnd()))
        m_pCountdownClock->Invalidate(FALSE);
}

void CMainFrame::OnOptionsDialogClosed(COptionsDlg* dlg)
{
    if (m_pOptionsDlg == dlg)
        m_pOptionsDlg = nullptr;
}

void CMainFrame::OnHelpAbout()
{
    MessageBoxW(
        L"2026 WinLuach - Hebrew Calendar\n\n"
        L"Version " WINLUACH_VERSION_TEXT L"\n\n"
        L"A modern Hebrew/Gregorian calendar\n"
        L"with halachic times (zmanim).\n\n"
        L"Built with C++ and MFC.",
        L"About WinLuach",
        MB_OK | MB_ICONINFORMATION);
}

void CMainFrame::OnHelpContents()
{
    CHelpContentsDlg dlg(this);
    dlg.DoModal();
}

// =============================================================================
// MONTH / YEAR PICKER CONTROLS
// =============================================================================

void CMainFrame::PopulateMonthCombo()
{
    if (!m_comboMonth.GetSafeHwnd()) return;
    m_updatingControls = true;
    m_comboMonth.ResetContent();
    if (m_hebrewMonthView)
    {
        int months = MonthsInHebrewYear(m_viewHebrewYear);
        bool leap  = IsHebrewLeapYear(m_viewHebrewYear);
        for (int m = 1; m <= months; m++)
            m_comboMonth.AddString(HebrewMonthName(m, leap).c_str());
    }
    else
    {
        for (int m = 1; m <= 12; m++)
            m_comboMonth.AddString(GregorianMonthName(m).c_str());
    }
    m_updatingControls = false;
}

void CMainFrame::UpdateMonthYearControls()
{
    if (!m_comboMonth.GetSafeHwnd()) return;
    m_updatingControls = true;

    int month = m_hebrewMonthView ? m_viewHebrewMonth : m_viewMonth;
    int year  = m_hebrewMonthView ? m_viewHebrewYear  : m_viewYear;

    // Re-populate if the number of months changed (Hebrew leap year toggle)
    if (m_hebrewMonthView)
    {
        int expectedMonths = MonthsInHebrewYear(m_viewHebrewYear);
        if (m_comboMonth.GetCount() != expectedMonths)
        {
            m_updatingControls = false;
            PopulateMonthCombo();
            m_updatingControls = true;
        }
    }

    m_comboMonth.SetCurSel(month - 1);  // 0-based index

    CString yearStr;
    yearStr.Format(L"%d", year);
    m_editYear.SetWindowText(yearStr);

    m_updatingControls = false;
}

void CMainFrame::OnMonthComboChange()
{
    if (m_updatingControls) return;
    int sel = m_comboMonth.GetCurSel();
    if (sel == CB_ERR) return;

    int newMonth = sel + 1;  // 1-based
    if (m_hebrewMonthView)
    {
        if (newMonth == m_viewHebrewMonth) return;
        m_viewHebrewMonth = newMonth;
    }
    else
    {
        if (newMonth == m_viewMonth) return;
        m_viewMonth = newMonth;
    }

    if (m_pCalView) { m_pCalView->RebuildCells(); m_pCalView->Invalidate(FALSE); }
    if (m_pSidebar) m_pSidebar->Invalidate(FALSE);
    Invalidate(FALSE);
}

void CMainFrame::OnYearSpinDelta(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMUPDOWN* pUD = reinterpret_cast<NMUPDOWN*>(pNMHDR);
    ChangeYear(pUD->iDelta);          // +1 = up arrow = next year
    *pResult = 1;                     // prevent spin from auto-updating buddy
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
    // Ctrl+Q — exit
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == 'Q' &&
        (GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000))
    {
        PostMessage(WM_CLOSE);
        return TRUE;
    }

    // Esc on the main window — minimize (dialogs handle their own Esc)
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE &&
        pMsg->hwnd == GetSafeHwnd())
    {
        ShowWindow(SW_MINIMIZE);
        return TRUE;
    }

    // Ctrl+P — print
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == 'P' &&
        (GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000))
    {
        OnCalPrint();
        return TRUE;
    }

    // Ctrl+G — go to date
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == 'G' &&
        (GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000))
    {
        OnCalGoTo();
        return TRUE;
    }

    // Ctrl+E — events dialog
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == 'E' &&
        (GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000))
    {
        OnCalEvents();
        return TRUE;
    }

    // Ctrl+L — location dialog
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == 'L' &&
        (GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000))
    {
        OnOptionsLocation();
        return TRUE;
    }

    // Ctrl+, — open preferences
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_OEM_COMMA &&
        (GetKeyState(VK_CONTROL) & 0x8000))
    {
        OnOptionsPrefs();
        return TRUE;
    }

    // Ctrl+Alt+Left — previous decade (Alt held = WM_SYSKEYDOWN)
    if ((pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) &&
        pMsg->wParam == VK_LEFT &&
        (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_MENU) & 0x8000))
    {
        ChangeYear(-10);
        return TRUE;
    }
    // Ctrl+Alt+Right — next decade (Alt held = WM_SYSKEYDOWN)
    if ((pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) &&
        pMsg->wParam == VK_RIGHT &&
        (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_MENU) & 0x8000))
    {
        ChangeYear(10);
        return TRUE;
    }

    // Ctrl+Alt+P — print zmanim month page setup
    if ((pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) &&
        pMsg->wParam == 'P' &&
        (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_MENU) & 0x8000))
    {
        OnCalPrintZmanim();
        return TRUE;
    }

    // Ctrl + Plus/= — larger font
    if (pMsg->message == WM_KEYDOWN &&
        (pMsg->wParam == VK_OEM_PLUS || pMsg->wParam == (WPARAM)'+') &&
        (GetKeyState(VK_CONTROL) & 0x8000))
    {
        OnViewZoomIn();
        return TRUE;
    }
    // Ctrl + Minus — smaller font
    if (pMsg->message == WM_KEYDOWN &&
        (pMsg->wParam == VK_OEM_MINUS || pMsg->wParam == (WPARAM)'-') &&
        (GetKeyState(VK_CONTROL) & 0x8000))
    {
        OnViewZoomOut();
        return TRUE;
    }
    // Ctrl + 0 / Ctrl+Numpad0 — reset font to default (Normal = index 3)
    if (pMsg->message == WM_KEYDOWN &&
        (pMsg->wParam == '0' || pMsg->wParam == VK_NUMPAD0) &&
        (GetKeyState(VK_CONTROL) & 0x8000))
    {
        OnViewZoomReset();
        return TRUE;
    }

    // Enter in year edit: navigate to typed year
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN &&
        m_editYear.GetSafeHwnd() && pMsg->hwnd == m_editYear.GetSafeHwnd())
    {
        CString s;
        m_editYear.GetWindowText(s);
        int year = _wtoi(s);
        if (year >= 1 && year <= 9999)
        {
            int cur = m_hebrewMonthView ? m_viewHebrewYear : m_viewYear;
            if (year != cur) ChangeYear(year - cur);
        }
        UpdateMonthYearControls();
        if (m_pCalView) m_pCalView->SetFocus();
        return TRUE;
    }

    // Mouse-wheel over month combo: scroll months
    if (pMsg->message == WM_MOUSEWHEEL && m_comboMonth.GetSafeHwnd())
    {
        POINT pt;
        GetCursorPos(&pt);
        CRect rc;
        m_comboMonth.GetWindowRect(&rc);
        if (rc.PtInRect(CPoint(pt)))
        {
            int delta = GET_WHEEL_DELTA_WPARAM(pMsg->wParam);
            ChangeMonth(delta > 0 ? -1 : 1);
            return TRUE;
        }

        // Mouse-wheel over year edit or spin: scroll years
        CRect rcY, rcS;
        m_editYear.GetWindowRect(&rcY);
        m_spinYear.GetWindowRect(&rcS);
        rcY.UnionRect(rcY, rcS);
        if (rcY.PtInRect(CPoint(pt)))
        {
            int delta = GET_WHEEL_DELTA_WPARAM(pMsg->wParam);
            ChangeYear(delta > 0 ? 1 : -1);
            return TRUE;
        }
    }

    return CFrameWnd::PreTranslateMessage(pMsg);
}

// =============================================================================
// PRINT
// =============================================================================

void CMainFrame::OnCalPrint()
{
    CCalPrintDlg dlg(this, nullptr);
    if (dlg.DoModal() == IDOK)
        DoPrint(dlg.GetOptions(), this);
}

void CMainFrame::OnCalPreview()
{
    CalPrintOptions opts;
    const auto& ps = theApp.m_settings;
    opts.landscape = ps.printLandscape;
    opts.range     = (CalPrintOptions::Range)ps.printRange;
    opts.mTop      = ps.printMarginTop;
    opts.mBot      = ps.printMarginBot;
    opts.mLeft     = ps.printMarginLeft;
    opts.mRight    = ps.printMarginRight;
    opts.includeZmanim = ps.printWeeklyZmanim;
    opts.zmanimColumns = ps.printZmanimColMask;
    opts.showFooter    = ps.printShowFooter;
    opts.use24hr       = ps.use24Hour;
    opts.twoColumns    = ps.printTwoColumns;
    CCalPreviewDlg dlg(opts, this, nullptr);
    dlg.DoModal();
}

BOOL CMainFrame::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    if (GetKeyState(VK_CONTROL) & 0x8000)
    {
        if (zDelta > 0) OnViewZoomIn(); else OnViewZoomOut();
        return TRUE;
    }
    return CFrameWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CMainFrame::OnFileBackup()
{
    wchar_t file[MAX_PATH] = L"WinLuach_settings_backup.json";
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = GetSafeHwnd();
    ofn.lpstrFilter = L"JSON files\0*.json\0All files\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"json";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&ofn)) return;

    std::wstring src = GetSettingsFilePath();
    if (CopyFileW(src.c_str(), file, FALSE))
        MessageBox(L"Settings backed up successfully.", L"WinLuach", MB_OK | MB_ICONINFORMATION);
    else
        MessageBox(L"Backup failed.", L"WinLuach", MB_OK | MB_ICONERROR);
}

void CMainFrame::OnViewZoomIn()
{
    if (theApp.m_settings.fontSize < 6) {
        theApp.m_settings.fontSize++;
        ApplySettings(theApp.m_settings);
        SaveSettings(theApp.m_settings);
    }
}

void CMainFrame::OnViewZoomOut()
{
    if (theApp.m_settings.fontSize > 0) {
        theApp.m_settings.fontSize--;
        ApplySettings(theApp.m_settings);
        SaveSettings(theApp.m_settings);
    }
}

void CMainFrame::OnViewZoomReset()
{
    theApp.m_settings.fontSize = 1;   // Default (index 1 → 14pt)
    ApplySettings(theApp.m_settings);
    SaveSettings(theApp.m_settings);
}

void CMainFrame::OnViewCountdownClock()
{
    if (m_pCountdownClock && ::IsWindow(m_pCountdownClock->GetSafeHwnd()))
    {
        m_pCountdownClock->ShowWindow(SW_SHOWNORMAL);
        m_pCountdownClock->SetForegroundWindow();
        return;
    }

    m_pCountdownClock = new CCountdownClockWnd(this);
    if (!m_pCountdownClock->CreateClock())
    {
        delete m_pCountdownClock;
        m_pCountdownClock = nullptr;
        MessageBox(L"Could not open the countdown clock.", L"WinLuach", MB_OK | MB_ICONERROR);
        return;
    }
    m_pCountdownClock->ShowWindow(SW_SHOW);
}

void CMainFrame::OnCalGoTo()
{
    CGotoDateDlg dlg(m_selectedDate, this);
    if (dlg.DoModal() == IDOK)
        SelectDate(dlg.selectedDate);
}

void CMainFrame::OpenDayViewForDate(const GregorianDate& g)
{
    CDayViewDlg dlg(g, this, nullptr);
    dlg.DoModal();
}

void CMainFrame::OnFileRestore()
{
    if (MessageBox(L"Restore settings from a backup file? Current settings will be overwritten.",
        L"WinLuach", MB_YESNO | MB_ICONQUESTION) != IDYES) return;

    wchar_t file[MAX_PATH] = {};
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = GetSafeHwnd();
    ofn.lpstrFilter = L"JSON files\0*.json\0All files\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;

    std::wstring dst = GetSettingsFilePath();
    if (CopyFileW(file, dst.c_str(), FALSE))
    {
        LoadSettings(theApp.m_settings);
        ApplySettings(theApp.m_settings);
        PopulateMonthCombo();
        UpdateMonthYearControls();
        MessageBox(L"Settings restored. Restart recommended.", L"WinLuach", MB_OK | MB_ICONINFORMATION);
    }
    else
        MessageBox(L"Restore failed.", L"WinLuach", MB_OK | MB_ICONERROR);
}
