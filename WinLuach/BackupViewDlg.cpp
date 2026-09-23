// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    BackupViewDlg.cpp
// Purpose: Read-only viewer for backup files. Turns the raw settings keys
//          into readable labels and values, grouped into sections, and lists
//          personal events and custom locations one per row.
// =============================================================================
//
// CHANGELOG:
// v0.8.133 - Initial file.
// =============================================================================

#include "pch.h"
#include "BackupViewDlg.h"
#include "HebrewDate.h"
#include <map>
#include <cwctype>
#include <ctime>

// =============================================================================
// SECTIONS
// =============================================================================

enum BackupSection
{
    SEC_LOCATION, SEC_DISPLAY, SEC_MONTHVIEW, SEC_ZMANIM, SEC_COLORS,
    SEC_NOTIFY, SEC_REMINDERS, SEC_TRAY, SEC_COUNTDOWN, SEC_PRINT,
    SEC_WEBCAL, SEC_LAYOUT, SEC_UPDATES, SEC_EVENTS, SEC_LOCATIONS, SEC_OTHER,
    SEC_COUNT
};

static const wchar_t* kSectionNames[SEC_COUNT] =
{
    L"Location", L"Display", L"Month view", L"Zmanim", L"Calendar colors",
    L"Notifications", L"Advanced reminders", L"Tray & startup", L"Countdown clock", L"Printing",
    L"Web calendars", L"Window & layout", L"Updates", L"Personal events", L"Custom locations", L"Other"
};

static bool StartsWith(const std::wstring& s, const wchar_t* prefix)
{
    return s.rfind(prefix, 0) == 0;
}

static BackupSection SectionForKey(const std::wstring& k)
{
    if (k == L"locationName" || k == L"latitude" || k == L"longitude" || k == L"elevation" ||
        k == L"gmtOffset" || k == L"usesDST" || k == L"isIsrael" || k == L"candleLightingMinutes")
        return SEC_LOCATION;
    if (StartsWith(k, L"showChatzos") || k == L"showBeHaB")              return SEC_MONTHVIEW;
    if (StartsWith(k, L"advancedReminder") || StartsWith(k, L"reminderDaily")) return SEC_REMINDERS;
    if (StartsWith(k, L"notify") || StartsWith(k, L"useWinLuachToast") || StartsWith(k, L"winLuachToast"))
        return SEC_NOTIFY;
    if (StartsWith(k, L"tray") || k == L"showTrayIcon" || StartsWith(k, L"minimize") ||
        k == L"startWithWindows" || k == L"desktopShortcut")
        return SEC_TRAY;
    if (StartsWith(k, L"countdown"))                                     return SEC_COUNTDOWN;
    if (StartsWith(k, L"print") || StartsWith(k, L"dayDetail"))          return SEC_PRINT;
    if (StartsWith(k, L"webCal"))                                        return SEC_WEBCAL;
    if (StartsWith(k, L"color"))                                         return SEC_COLORS;
    if (StartsWith(k, L"sidebar") || k == L"zmanimHeight" || StartsWith(k, L"pane") ||
        k == L"yearDetailsMask" || StartsWith(k, L"window"))
        return SEC_LAYOUT;
    if (StartsWith(k, L"custom") || StartsWith(k, L"zmanim") || StartsWith(k, L"shaahZmanit") ||
        StartsWith(k, L"sofZman") || k == L"alotShita" || k == L"tzeitShita")
        return SEC_ZMANIM;
    if (k == L"disableAutoUpdate" || StartsWith(k, L"checkUpdates") ||
        k == L"updateCheckFrequency" || k == L"lastUpdateCheckTime")
        return SEC_UPDATES;
    if (StartsWith(k, L"show") || k == L"use24Hour" || k == L"defaultHebrewMonth" ||
        k == L"dateTracking" || k == L"haftarahShita" || k == L"fontSize" ||
        k == L"language" || StartsWith(k, L"useHebrew"))
        return SEC_DISPLAY;
    return SEC_OTHER;
}

// =============================================================================
// LABELS
// =============================================================================

static const std::map<std::wstring, std::wstring>& LabelOverrides()
{
    static const std::map<std::wstring, std::wstring> m =
    {
        { L"locationName",          L"Location name" },
        { L"gmtOffset",             L"Time zone" },
        { L"usesDST",               L"Daylight saving time" },
        { L"isIsrael",              L"Israel schedule (not Diaspora)" },
        { L"candleLightingMinutes", L"Candle lighting (minutes before sunset)" },
        { L"elevation",             L"Elevation (meters)" },
        { L"use24Hour",             L"24-hour clock" },
        { L"fontSize",              L"Text size" },
        { L"useHebrewScript",       L"Hebrew script" },
        { L"useHebrewNumerals",     L"Hebrew numerals" },
        { L"defaultHebrewMonth",    L"Start in Hebrew month view" },
        { L"showBeHaB",             L"Highlight BeHaB days" },
        { L"showChatzosOnBeHaB",    L"Show chatzos on BeHaB days" },
        { L"showChatzosOnFasts",    L"Show chatzos on fast days" },
        { L"zmanimShita",           L"Shema / Tefilla shita" },
        { L"shaahZmanitShita",      L"Shaah zmanit shita" },
        { L"startWithWindows",      L"Start with Windows" },
        { L"showTrayIcon",          L"Always show tray icon" },
        { L"reminderDailyHour",     L"Daily reminder time (hour)" },
        { L"reminderDailyMinute",   L"Daily reminder time (minute)" },
        { L"lastUpdateCheckTime",   L"Last update check" },
        { L"disableAutoUpdate",     L"Disable all update checks" },
        { L"checkUpdatesAuto",      L"Check for updates automatically" },
        { L"checkUpdatesOnLaunch",  L"Check for updates at startup" },
        { L"notifySefirahOtherZman", L"Sefirah: other zman (number)" },
        { L"notifyWebCalEvents",    L"Web calendar events" },
        { L"windowX",               L"Window left" },
        { L"windowY",               L"Window top" },
        { L"windowW",               L"Window width" },
        { L"windowH",               L"Window height" },
    };
    return m;
}

// "customAlotDegreesValue" -> "Custom alot degrees value"
static std::wstring Humanize(const std::wstring& key)
{
    std::wstring out;
    for (size_t i = 0; i < key.size(); ++i)
    {
        wchar_t c = key[i];
        bool boundary = i > 0 &&
            ((iswupper(c) && !iswupper(key[i - 1])) ||
             (iswdigit(c) && !iswdigit(key[i - 1])) ||
             (!iswdigit(c) && iswdigit(key[i - 1])));
        if (boundary) out += L' ';
        out += (out.empty() ? (wchar_t)towupper(c) : (wchar_t)towlower(c));
    }
    // Keep common abbreviations readable
    const std::pair<const wchar_t*, const wchar_t*> fixes[] =
    {
        { L" gra", L" GRA" }, { L" ma ", L" MA " }, { L" dst", L" DST" }, { L" gmt", L" GMT" },
        { L" url", L" URL" }, { L" rtl", L" RTL" },
        { L"Win luach", L"WinLuach" }, { L" win luach", L" WinLuach" }, { L"Web cal ", L"Web calendar " }
    };
    for (const auto& f : fixes)
    {
        size_t p;
        while ((p = out.find(f.first)) != std::wstring::npos)
            out.replace(p, wcslen(f.first), f.second);
    }
    if (out.size() > 4 && out.compare(out.size() - 4, 4, L" bot") == 0)
        out += L"tom";   // "Margin bot" -> "Margin bottom"
    return out;
}

static std::wstring LabelForKey(const std::wstring& key, BackupSection sec)
{
    auto it = LabelOverrides().find(key);
    if (it != LabelOverrides().end()) return it->second;

    // Drop a prefix the section header already says
    static const std::pair<BackupSection, const wchar_t*> prefixes[] =
    {
        { SEC_COUNTDOWN, L"countdown" }, { SEC_COLORS, L"color" }, { SEC_NOTIFY, L"notify" },
        { SEC_TRAY, L"tray" }, { SEC_PRINT, L"print" }
    };
    for (const auto& p : prefixes)
    {
        size_t n = wcslen(p.second);
        if (p.first == sec && key.size() > n && StartsWith(key, p.second) && iswupper(key[n]))
            return Humanize(key.substr(n));
    }
    return Humanize(key);
}

// =============================================================================
// VALUES
// =============================================================================

static long long ToInt(const std::wstring& v)
{
    try { return std::stoll(v); } catch (...) { return 0; }
}

static const wchar_t* Pick(long long i, std::initializer_list<const wchar_t*> names)
{
    if (i >= 0 && i < (long long)names.size()) return *(names.begin() + i);
    return nullptr;
}

static std::wstring ValueForKey(const std::wstring& key, const std::wstring& raw)
{
    if (raw == L"true")  return L"Yes";
    if (raw == L"false") return L"No";

    // Named choices
    const wchar_t* named = nullptr;
    long long n = ToInt(raw);
    if (key == L"haftarahShita")        named = Pick(n, { L"Ashkenazi", L"Eidot Mizrach", L"Italian", L"Yemenite" });
    else if (key == L"zmanimShita")     named = Pick(n, { L"GRA", L"Magen Avraham (72 min)", L"Magen Avraham (90 min)" });
    else if (key == L"shaahZmanitShita") named = Pick(n, { L"GRA (netz to shkiah)", L"MA 72", L"MA 90", L"Custom boundaries", L"16.1° to 16.1°" });
    else if (key == L"alotShita")       named = Pick(n, { L"16.1° (GRA)", L"72 minutes", L"90 minutes" });
    else if (key == L"tzeitShita")      named = Pick(n, { L"8.5° (GRA)", L"72 minutes", L"90 minutes", L"72 min proportional", L"90 min proportional" });
    else if (key == L"minimizeTrayWhen") named = Pick(n, { L"When minimized", L"When closed", L"When minimized or closed" });
    else if (key == L"trayNumberStyle") named = Pick(n, { L"Hebrew letters", L"English digits" });
    else if (key == L"updateCheckFrequency") named = Pick(n, { L"Daily", L"Weekly", L"Monthly" });
    else if (key == L"printRange")      named = Pick(n, { L"Month", L"Year", L"Next 12 months" });
    else if (key == L"language")        named = Pick(n, { L"English" });
    else if (key == L"notifySefirahMode")      named = Pick(n, { L"At a fixed time", L"Relative to a zman" });
    else if (key == L"notifySefirahOffsetDir") named = Pick(n, { L"Before", L"After" });
    else if (key == L"notifySefirahBase")      named = Pick(n, { L"Sunset", L"Tzeis", L"Another zman" });
    else if (key == L"winLuachToastDurationUnit") named = Pick(n, { L"Minutes", L"Hours", L"Days", L"Weeks", L"Months" });
    else if (StartsWith(key, L"notify") && key.size() > 5 && key.substr(key.size() - 5) == L"Style")
        named = Pick(n, { L"Off", L"Windows toast", L"Popup", L"Toast and popup" });
    else if (key == L"notifyPersonalEvents" || key == L"notifyWebCalEvents")
        named = Pick(n, { L"Off", L"Windows toast", L"Popup", L"Toast and popup" });
    if (named) return named;

    if (key == L"fontSize")
    {
        const wchar_t* size = Pick(n, { L"Small", L"Medium (default)", L"Large" });
        return size ? std::wstring(size) : L"Zoom level " + std::to_wstring(n);
    }
    if (key == L"gmtOffset")
        return L"GMT" + std::wstring(n >= 0 ? L"+" : L"") + std::to_wstring(n);
    if ((key == L"windowX" || key == L"windowY") && n == -1)
        return L"Default";
    if (key == L"lastUpdateCheckTime")
    {
        if (n <= 0) return L"Never";
        __time64_t t = (__time64_t)n;
        tm local = {};
        if (_localtime64_s(&local, &t) != 0) return raw;
        wchar_t buf[64];
        wcsftime(buf, 64, L"%Y-%m-%d %H:%M", &local);
        return buf;
    }

    // Colors are stored COLORREF-style (0x00BBGGRR)
    bool isColor = StartsWith(key, L"color") ||
        (key.size() > 5 && key.substr(key.size() - 5) == L"Color");
    if (isColor)
    {
        unsigned long long c = (unsigned long long)n;
        wchar_t buf[48];
        swprintf_s(buf, L"RGB(%u, %u, %u)", (unsigned)(c & 0xFF), (unsigned)((c >> 8) & 0xFF), (unsigned)((c >> 16) & 0xFF));
        return buf;
    }

    // Bit masks: say how many items are switched on
    if (key.find(L"Mask") != std::wstring::npos)
    {
        unsigned long long m = 0;
        try { m = std::stoull(raw); } catch (...) { return raw; }
        int bits = 0;
        for (; m; m &= m - 1) ++bits;
        return bits == 0 ? L"None selected" : std::to_wstring(bits) + L" selected";
    }

    if (raw.empty()) return L"(none)";
    return raw;
}

// =============================================================================
// ROWS
// =============================================================================

struct BackupRow
{
    BackupSection section;
    std::wstring  label;
    std::wstring  value;
};

static std::wstring EventSummary(const UserEventEntry& e)
{
    const wchar_t* type = Pick(e.type, { L"Birthday", L"Anniversary", L"Yahrzeit", L"Custom" });
    std::wstring s = type ? type : L"Event";

    if (e.gregMonth > 0)
    {
        s += L" · " + GregorianMonthName(e.gregMonth) + L" " + std::to_wstring(e.gregDay);
        if (e.gregYear > 0) s += L", " + std::to_wstring(e.gregYear);
    }
    if (e.hebMonth > 0)
    {
        s += L" · " + std::to_wstring(e.hebDay) + L" " + HebrewMonthName(e.hebMonth, false);
        if (e.hebYear > 0) s += L" " + std::to_wstring(e.hebYear);
    }
    if (e.afterSunset)       s += L" · starts the evening before";
    if (!e.observeAnnually)  s += L" · one-time";
    if (!e.notify)           s += L" · no notification";
    else if (!e.alarmOffsets.empty()) s += L" · remind " + e.alarmOffsets;
    return s;
}

static std::wstring LocationSummary(const LocationEntry& e)
{
    std::wstring s;
    if (!e.region.empty())  s += e.region;
    if (!e.country.empty()) s += (s.empty() ? L"" : L", ") + e.country;
    wchar_t buf[160];
    swprintf_s(buf, L"%s%.4f, %.4f · %.0f m · GMT%+d%s%s",
        s.empty() ? L"" : L" · ",
        e.loc.latitude, e.loc.longitude, e.loc.elevation, e.loc.gmtOffset,
        e.loc.usesDST ? L" with DST" : L"", e.isIsrael ? L" · Israel" : L"");
    return s + buf;
}

// Splits "advancedReminder3_target" into (3, "target").
static bool SplitIndexed(const std::wstring& key, const wchar_t* prefix, int& idx, std::wstring& field)
{
    if (!StartsWith(key, prefix)) return false;
    size_t start = wcslen(prefix);
    size_t us = key.find(L'_', start);
    if (us == std::wstring::npos || us == start) return false;
    try { idx = std::stoi(key.substr(start, us - start)); } catch (...) { return false; }
    field = key.substr(us + 1);
    return true;
}

static std::vector<BackupRow> BuildRows(const BackupContents& c)
{
    std::vector<BackupRow> rows;
    std::map<int, std::map<std::wstring, std::wstring>> reminders, webcals;

    for (const auto& kv : c.settings)
    {
        const std::wstring& key = kv.first;
        int idx; std::wstring field;
        if (SplitIndexed(key, L"advancedReminder", idx, field)) { reminders[idx][field] = kv.second; continue; }
        if (SplitIndexed(key, L"webCal", idx, field))           { webcals[idx][field]   = kv.second; continue; }
        // Counts and the legacy single URL are covered by the rows above
        if (key == L"advancedReminderCount" || key == L"webCalCount" || key == L"webCalendarUrl")
            continue;

        BackupSection sec = SectionForKey(key);
        rows.push_back({ sec, LabelForKey(key, sec), ValueForKey(key, kv.second) });
    }

    for (auto& r : reminders)
    {
        auto& f = r.second;
        std::wstring v = (f[L"enabled"] == L"false") ? L"Off: " : L"";
        v += f[L"kind"];
        if (!f[L"target"].empty())  v += L" – " + f[L"target"];
        if (!f[L"offsets"].empty()) v += L" · " + f[L"offsets"] + (f[L"afterEvent"] == L"true" ? L" after" : L" before");
        const wchar_t* anchor = Pick(ToInt(f[L"anchor"]), { L"from Tzeit", L"from Shkiah", L"from midnight" });
        if (anchor && (f[L"kind"] == L"Holiday" || f[L"kind"] == L"Parsha")) v += std::wstring(L" (") + anchor + L")";
        const wchar_t* style = Pick(ToInt(f[L"style"]), { L"off", L"toast", L"popup", L"toast and popup" });
        if (style) v += std::wstring(L" · ") + style;
        rows.push_back({ SEC_REMINDERS, L"Reminder " + std::to_wstring(r.first + 1), v });
    }
    for (auto& w : webcals)
    {
        auto& f = w.second;
        std::wstring v = f[L"url"].empty() ? L"(none)" : f[L"url"];
        if (f[L"enabled"] == L"false") v += L"  (disabled)";
        rows.push_back({ SEC_WEBCAL, L"Calendar " + std::to_wstring(w.first + 1), v });
    }

    for (const auto& e : c.events)
        rows.push_back({ SEC_EVENTS, e.name.empty() ? L"(unnamed)" : e.name, EventSummary(e) });
    if (c.hasEvents && c.events.empty())
        rows.push_back({ SEC_EVENTS, L"(none)", L"" });

    for (const auto& l : c.locations)
        rows.push_back({ SEC_LOCATIONS, l.loc.name.empty() ? L"(unnamed)" : l.loc.name, LocationSummary(l) });
    if (c.hasLocations && c.locations.empty())
        rows.push_back({ SEC_LOCATIONS, L"(none)", L"" });

    return rows;
}

// =============================================================================
// DIALOG
// =============================================================================

BEGIN_MESSAGE_MAP(CBackupViewDlg, CDialog)
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()

CBackupViewDlg::CBackupViewDlg(const std::wstring& path, const BackupContents& contents, CWnd* pParent)
    : CDialog(), m_path(path), m_contents(contents)
{
    m_pParentWnd = pParent;
}

INT_PTR CBackupViewDlg::DoModal()
{
    struct Tmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[32]; } b = {};
    b.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX | DS_CENTER;
    b.t.cx = 400; b.t.cy = 300;
    wcscpy_s(b.title, L"Backup Contents");
    if (!InitModalIndirect((DLGTEMPLATE*)&b, m_pParentWnd)) return -1;
    return CDialog::DoModal();
}

BOOL CBackupViewDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    CFont* font = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));

    std::vector<BackupRow> rows = BuildRows(m_contents);

    // --- Header: which file (one line, path shortened), then when / what it holds ---
    std::wstring file = L"File: " + m_path;
    m_file.Create(file.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_PATHELLIPSIS,
        CRect(0, 0, 10, 10), this);
    m_file.SetFont(font);

    std::wstring summary;
    if (m_contents.isMaster)
    {
        if (!m_contents.created.empty()) summary += L"Created: " + m_contents.created + L"      ";
        summary += std::to_wstring(m_contents.settings.size()) + L" preferences · " +
                   std::to_wstring(m_contents.events.size()) + L" personal events · " +
                   std::to_wstring(m_contents.locations.size()) + L" custom locations";
    }
    else
    {
        summary = L"Older settings-only backup: preferences only (no personal events or custom locations).";
    }
    m_header.Create(summary.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
        CRect(0, 0, 10, 10), this);
    m_header.SetFont(font);

    // --- List: Setting | Value, grouped by section ---
    m_list.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        CRect(0, 0, 10, 10), this, 1001);
    m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);
    m_list.SetFont(font);
    m_list.InsertColumn(0, L"Setting", LVCFMT_LEFT, 260);
    m_list.InsertColumn(1, L"Value", LVCFMT_LEFT, 360);
    m_list.EnableGroupView(TRUE);

    int counts[SEC_COUNT] = {};
    for (const auto& r : rows) counts[r.section]++;
    for (int s = 0; s < SEC_COUNT; ++s)
    {
        if (counts[s] == 0) continue;
        std::wstring name = kSectionNames[s];
        if (s == SEC_EVENTS || s == SEC_LOCATIONS || s == SEC_REMINDERS || s == SEC_WEBCAL)
        {
            // Count the listed items only (not "(none)" or the daily-time rows)
            int real = 0;
            for (const auto& r : rows)
            {
                if (r.section != s || r.label == L"(none)") continue;
                if (s == SEC_REMINDERS && !StartsWith(r.label, L"Reminder ")) continue;
                if (s == SEC_WEBCAL && !StartsWith(r.label, L"Calendar ")) continue;
                ++real;
            }
            name += L" (" + std::to_wstring(real) + L")";
        }
        LVGROUP g = { sizeof(g) };
        g.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
        g.pszHeader = const_cast<LPWSTR>(name.c_str());
        g.iGroupId = s;
        g.stateMask = LVGS_COLLAPSIBLE;
        g.state = LVGS_COLLAPSIBLE;
        m_list.InsertGroup(s, &g);
    }

    int item = 0;
    for (int s = 0; s < SEC_COUNT; ++s)
    {
        for (const auto& r : rows)
        {
            if (r.section != s) continue;
            LVITEM it = {};
            it.mask = LVIF_TEXT | LVIF_GROUPID;
            it.iItem = item;
            it.iGroupId = s;
            it.pszText = const_cast<LPWSTR>(r.label.c_str());
            int at = m_list.InsertItem(&it);
            m_list.SetItemText(at, 1, r.value.c_str());
            ++item;
        }
    }

    m_close.Create(L"Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        CRect(0, 0, 10, 10), this, IDOK);
    m_close.SetFont(font);

    Layout();
    m_list.SetFocus();
    return FALSE;
}

void CBackupViewDlg::Layout()
{
    if (!::IsWindow(m_list.GetSafeHwnd())) return;
    CRect rc; GetClientRect(&rc);
    const int pad = 10, lineH = 18, btnW = 90, btnH = 26;

    m_file.MoveWindow(pad, pad, rc.Width() - 2 * pad, lineH);
    m_header.MoveWindow(pad, pad + lineH, rc.Width() - 2 * pad, lineH);
    int listTop = pad + 2 * lineH + 6;
    int listBottom = rc.bottom - pad - btnH - 8;
    m_list.MoveWindow(pad, listTop, rc.Width() - 2 * pad, max(40, listBottom - listTop));
    m_close.MoveWindow(rc.right - pad - btnW, rc.bottom - pad - btnH, btnW, btnH);

    // Value column takes the remaining width
    CRect lr; m_list.GetClientRect(&lr);
    int labelW = m_list.GetColumnWidth(0);
    m_list.SetColumnWidth(1, max(120, lr.Width() - labelW - 4));
}

void CBackupViewDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);
    Layout();
}

void CBackupViewDlg::OnGetMinMaxInfo(MINMAXINFO* mmi)
{
    mmi->ptMinTrackSize.x = 480;
    mmi->ptMinTrackSize.y = 320;
}
