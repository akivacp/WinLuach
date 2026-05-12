// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    Settings.cpp
// Purpose: Saves and loads user preferences to/from
//          %APPDATA%\WinLuach\settings.json.
//          Hand-rolled JSON — no external library needed.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial implementation. Saves/loads all AppSettings fields.
//          Creates %APPDATA%\WinLuach\ directory if it doesn't exist.
// =============================================================================

#include "pch.h"
#include "Settings.h"
#include <fstream>
#include <sstream>
#include <Windows.h>
#include <ShlObj.h>
#pragma comment(lib, "Shell32.lib")

// =============================================================================
// FILE PATH
// =============================================================================

// Returns %APPDATA%\WinLuach\settings.json
std::wstring GetSettingsFilePath()
{
    wchar_t path[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path)))
    {
        std::wstring dir = std::wstring(path) + L"\\WinLuach";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir + L"\\settings.json";
    }
    return L"settings.json";
}

// =============================================================================
// JSON HELPERS
// =============================================================================

// Escapes a wstring for JSON output.
static std::wstring JsonEscape(const std::wstring& s)
{
    std::wstring out;
    for (wchar_t c : s)
    {
        if (c == L'"')  out += L"\\\"";
        else if (c == L'\\') out += L"\\\\";
        else                 out += c;
    }
    return out;
}

// Reads a quoted JSON string value from a line like: "key": "value"
static std::wstring ParseJsonString(const std::wstring& line)
{
    size_t colon = line.find(L':');
    if (colon == std::wstring::npos) return L"";
    size_t first = line.find(L'"', colon);
    if (first == std::wstring::npos) return L"";
    first++;
    size_t last = line.find(L'"', first);
    if (last == std::wstring::npos) return L"";
    return line.substr(first, last - first);
}

// Reads a numeric JSON value from a line like: "key": 42
static double ParseJsonNumber(const std::wstring& line)
{
    size_t colon = line.find(L':');
    if (colon == std::wstring::npos) return 0.0;
    std::wstring val = line.substr(colon + 1);
    size_t start = val.find_first_not_of(L" \t\r\n,");
    if (start == std::wstring::npos) return 0.0;
    try { return std::stod(val.substr(start)); }
    catch (...) { return 0.0; }
}

// Reads a boolean JSON value from a line like: "key": true
static bool ParseJsonBool(const std::wstring& line)
{
    return line.find(L"true") != std::wstring::npos;
}

// =============================================================================
// SAVE
// =============================================================================

// Saves all settings to %APPDATA%\WinLuach\settings.json.
bool SaveSettings(const AppSettings& s)
{
    std::wstring path = GetSettingsFilePath();
    std::wofstream f(path);
    if (!f.is_open()) return false;

    f << L"{\n";
    f << L"  \"locationName\": \"" << JsonEscape(s.locationName) << L"\",\n";
    f << L"  \"latitude\": " << s.latitude << L",\n";
    f << L"  \"longitude\": " << s.longitude << L",\n";
    f << L"  \"elevation\": " << s.elevation << L",\n";
    f << L"  \"gmtOffset\": " << s.gmtOffset << L",\n";
    f << L"  \"usesDST\": " << (s.usesDST ? L"true" : L"false") << L",\n";
    f << L"  \"use24Hour\": " << (s.use24Hour ? L"true" : L"false") << L",\n";
    f << L"  \"isIsrael\": " << (s.isIsrael ? L"true" : L"false") << L",\n";
    f << L"  \"defaultHebrewMonth\": " << (s.defaultHebrewMonth ? L"true" : L"false") << L",\n";
    f << L"  \"dateTracking\": " << (s.dateTracking ? L"true" : L"false") << L",\n";
    f << L"  \"showParshios\": " << (s.showParshios ? L"true" : L"false") << L",\n";
    f << L"  \"showMoadim\": " << (s.showMoadim ? L"true" : L"false") << L",\n";
    f << L"  \"showUserEvents\": " << (s.showUserEvents ? L"true" : L"false") << L",\n";
    f << L"  \"showDafYomi\": " << (s.showDafYomi ? L"true" : L"false") << L",\n";
    f << L"  \"showYerushalmi\": " << (s.showYerushalmi ? L"true" : L"false") << L",\n";
    f << L"  \"showHalachaYomit\": " << (s.showHalachaYomit ? L"true" : L"false") << L",\n";
    f << L"  \"showMishnaYomit\": " << (s.showMishnaYomit ? L"true" : L"false") << L",\n";
    f << L"  \"showTanachYomi\": " << (s.showTanachYomi ? L"true" : L"false") << L",\n";
    f << L"  \"haftarahShita\": " << s.haftarahShita << L",\n";
    f << L"  \"fontSize\": " << s.fontSize << L",\n";
    f << L"  \"language\": " << s.language << L",\n";
    f << L"  \"showTrayIcon\": "   << (s.showTrayIcon   ? L"true" : L"false") << L",\n";
    f << L"  \"minimizeToTray\": " << (s.minimizeToTray ? L"true" : L"false") << L",\n";
    f << L"  \"minimizeTrayWhen\": " << s.minimizeTrayWhen << L",\n";
    f << L"  \"trayTextColor\": " << s.trayTextColor << L",\n";
    f << L"  \"trayTooltipZmanimMask\": " << s.trayTooltipZmanimMask << L",\n";
    f << L"  \"minimizeOnStartup\": " << (s.minimizeOnStartup ? L"true" : L"false") << L",\n";
    f << L"  \"startWithWindows\": " << (s.startWithWindows ? L"true" : L"false") << L",\n";
    f << L"  \"desktopShortcut\": " << (s.desktopShortcut ? L"true" : L"false") << L",\n";
    f << L"  \"printWeeklyZmanim\": " << (s.printWeeklyZmanim ? L"true" : L"false") << L",\n";
    f << L"  \"candleLightingMinutes\": " << s.candleLightingMinutes << L",\n";
    f << L"  \"webCalendarUrl\": \"" << JsonEscape(s.webCalendarUrl) << L"\",\n";
    f << L"  \"webCalCount\": " << s.webCalendars.size() << L",\n";
    for (int i = 0; i < (int)s.webCalendars.size(); i++)
    {
        f << L"  \"webCal" << i << L"_url\": \"" << JsonEscape(s.webCalendars[i].url) << L"\",\n";
        f << L"  \"webCal" << i << L"_enabled\": " << (s.webCalendars[i].enabled ? L"true" : L"false") << L",\n";
    }
    f << L"  \"zmanimShita\": " << s.zmanimShita << L",\n";
    f << L"  \"alotShita\": "   << s.alotShita   << L",\n";
    f << L"  \"tzeitShita\": "  << s.tzeitShita  << L",\n";
    f << L"  \"printLandscape\": "   << (s.printLandscape ? L"true" : L"false") << L",\n";
    f << L"  \"printRange\": "       << s.printRange       << L",\n";
    f << L"  \"printMarginTop\": "   << s.printMarginTop   << L",\n";
    f << L"  \"printMarginBot\": "   << s.printMarginBot   << L",\n";
    f << L"  \"printMarginLeft\": "  << s.printMarginLeft  << L",\n";
    f << L"  \"printMarginRight\": " << s.printMarginRight << L",\n";
    f << L"  \"printZmanimColMask\": " << s.printZmanimColMask << L",\n";
    f << L"  \"printShowFooter\": " << (s.printShowFooter ? L"true" : L"false") << L",\n";
    f << L"  \"showChatzosOnFasts\": " << (s.showChatzosOnFasts ? L"true" : L"false") << L",\n";
    f << L"  \"customAlotMode\": " << s.customAlotMode << L",\n";
    f << L"  \"customAlotValue\": " << s.customAlotValue << L",\n";
    f << L"  \"customTzeitMode\": " << s.customTzeitMode << L",\n";
    f << L"  \"customTzeitValue\": " << s.customTzeitValue << L",\n";
    f << L"  \"colorNormalCell\": " << s.colorNormalCell << L",\n";
    f << L"  \"colorOtherMonthCell\": " << s.colorOtherMonthCell << L",\n";
    f << L"  \"colorTodayCell\": " << s.colorTodayCell << L",\n";
    f << L"  \"colorShabbosCell\": " << s.colorShabbosCell << L",\n";
    f << L"  \"colorYomTovCell\": " << s.colorYomTovCell << L",\n";
    f << L"  \"colorRoshChodeshCell\": " << s.colorRoshChodeshCell << L",\n";
    f << L"  \"colorCholHamoedCell\": " << s.colorCholHamoedCell << L",\n";
    f << L"  \"colorFastDayCell\": " << s.colorFastDayCell << L",\n";
    f << L"  \"colorGregorianText\": " << s.colorGregorianText << L",\n";
    f << L"  \"colorHebrewText\": " << s.colorHebrewText << L",\n";
    f << L"  \"colorHolidayText\": " << s.colorHolidayText << L",\n";
    f << L"  \"colorParshaText\": " << s.colorParshaText << L",\n";
    f << L"  \"colorCivilEventText\": " << s.colorCivilEventText << L",\n";
    f << L"  \"colorHebrewEventText\": " << s.colorHebrewEventText << L",\n";
    f << L"  \"colorOmerText\": " << s.colorOmerText << L",\n";
    f << L"  \"colorLearningText\": " << s.colorLearningText << L",\n";
    f << L"  \"colorCandleText\": " << s.colorCandleText << L",\n";
    f << L"  \"colorMotzText\": " << s.colorMotzText << L",\n";
    f << L"  \"notifyPersonalEvents\": " << s.notifyPersonalEvents << L",\n";
    f << L"  \"notifyWebCalEvents\": "   << s.notifyWebCalEvents   << L",\n";
    f << L"  \"notifyZmanimStyle\": " << s.notifyZmanimStyle << L",\n";
    f << L"  \"notifyZmanimMask\": " << s.notifyZmanimMask << L",\n";
    f << L"  \"notifyMoadimStyle\": " << s.notifyMoadimStyle << L",\n";
    f << L"  \"notifyMoadimOffsets\": \"" << JsonEscape(s.notifyMoadimOffsets) << L"\",\n";
    f << L"  \"notifyParshaStyle\": " << s.notifyParshaStyle << L",\n";
    f << L"  \"notifyParshaName\": \"" << JsonEscape(s.notifyParshaName) << L"\",\n";
    f << L"  \"notifyParshaOffsets\": \"" << JsonEscape(s.notifyParshaOffsets) << L"\",\n";
    f << L"  \"notifyPersonalOffsets\": \"" << JsonEscape(s.notifyPersonalOffsets) << L"\",\n";
    f << L"  \"advancedReminderCount\": " << s.advancedReminders.size() << L",\n";
    for (int i = 0; i < (int)s.advancedReminders.size(); ++i)
    {
        const auto& r = s.advancedReminders[i];
        f << L"  \"advancedReminder" << i << L"_enabled\": " << (r.enabled ? L"true" : L"false") << L",\n";
        f << L"  \"advancedReminder" << i << L"_style\": " << r.style << L",\n";
        f << L"  \"advancedReminder" << i << L"_kind\": \"" << JsonEscape(r.kind) << L"\",\n";
        f << L"  \"advancedReminder" << i << L"_target\": \"" << JsonEscape(r.target) << L"\",\n";
        f << L"  \"advancedReminder" << i << L"_offsets\": \"" << JsonEscape(r.offsets) << L"\",\n";
    }
    f << L"  \"sidebarWidth\": "     << s.sidebarWidth                              << L",\n";
    f << L"  \"zmanimHeight\": "     << s.zmanimHeight                              << L",\n";
    f << L"  \"sidebarCollapsed\": " << (s.sidebarCollapsed ? L"true" : L"false")   << L",\n";
    f << L"  \"countdownTitleFontFace\": \"" << JsonEscape(s.countdownTitleFontFace) << L"\",\n";
    f << L"  \"countdownTitleFontSize\": " << s.countdownTitleFontSize << L",\n";
    f << L"  \"countdownTitleTextColor\": " << s.countdownTitleTextColor << L",\n";
    f << L"  \"countdownTitleBackColor\": " << s.countdownTitleBackColor << L",\n";
    f << L"  \"countdownClockFontFace\": \"" << JsonEscape(s.countdownClockFontFace) << L"\",\n";
    f << L"  \"countdownClockFontSize\": " << s.countdownClockFontSize << L",\n";
    f << L"  \"countdownClockTextColor\": " << s.countdownClockTextColor << L",\n";
    f << L"  \"countdownClockBackColor\": " << s.countdownClockBackColor << L",\n";
    f << L"  \"countdownCurrentFontFace\": \"" << JsonEscape(s.countdownCurrentFontFace) << L"\",\n";
    f << L"  \"countdownCurrentFontSize\": " << s.countdownCurrentFontSize << L",\n";
    f << L"  \"countdownCurrentTextColor\": " << s.countdownCurrentTextColor << L",\n";
    f << L"  \"countdownCurrentBackColor\": " << s.countdownCurrentBackColor << L",\n";
    f << L"  \"countdownLiveFontFace\": \"" << JsonEscape(s.countdownLiveFontFace) << L"\",\n";
    f << L"  \"countdownLiveFontSize\": " << s.countdownLiveFontSize << L",\n";
    f << L"  \"countdownLiveTextColor\": " << s.countdownLiveTextColor << L",\n";
    f << L"  \"countdownLiveBackColor\": " << s.countdownLiveBackColor << L",\n";
    f << L"  \"windowX\": " << s.windowX << L",\n";
    f << L"  \"windowY\": " << s.windowY << L",\n";
    f << L"  \"windowW\": " << s.windowW << L",\n";
    f << L"  \"windowH\": " << s.windowH << L"\n";
    f << L"}\n";

    // Save personal events to separate file
    SaveEvents(s.userEvents);

    return true;
}

// =============================================================================
// LOAD
// =============================================================================

// Loads settings from %APPDATA%\WinLuach\settings.json.
// Returns false if file doesn't exist; s is filled with defaults.
bool LoadSettings(AppSettings& s)
{
    // Fill defaults first
    s = AppSettings();

    std::wstring path = GetSettingsFilePath();
    std::wifstream f(path);
    if (!f.is_open()) return false;

    std::wstring line;
    while (std::getline(f, line))
    {
        if (line.find(L"\"locationName\"") != std::wstring::npos) s.locationName = ParseJsonString(line);
        else if (line.find(L"\"latitude\"") != std::wstring::npos) s.latitude = ParseJsonNumber(line);
        else if (line.find(L"\"longitude\"") != std::wstring::npos) s.longitude = ParseJsonNumber(line);
        else if (line.find(L"\"elevation\"") != std::wstring::npos) s.elevation = ParseJsonNumber(line);
        else if (line.find(L"\"gmtOffset\"") != std::wstring::npos) s.gmtOffset = (int)ParseJsonNumber(line);
        else if (line.find(L"\"usesDST\"") != std::wstring::npos) s.usesDST = ParseJsonBool(line);
        else if (line.find(L"\"use24Hour\"") != std::wstring::npos) s.use24Hour = ParseJsonBool(line);
        else if (line.find(L"\"isIsrael\"") != std::wstring::npos) s.isIsrael = ParseJsonBool(line);
        else if (line.find(L"\"defaultHebrewMonth\"") != std::wstring::npos) s.defaultHebrewMonth = ParseJsonBool(line);
        else if (line.find(L"\"dateTracking\"") != std::wstring::npos) s.dateTracking = ParseJsonBool(line);
        else if (line.find(L"\"showParshios\"") != std::wstring::npos) s.showParshios = ParseJsonBool(line);
        else if (line.find(L"\"showMoadim\"") != std::wstring::npos) s.showMoadim = ParseJsonBool(line);
        else if (line.find(L"\"showUserEvents\"") != std::wstring::npos) s.showUserEvents = ParseJsonBool(line);
        else if (line.find(L"\"showDafYomi\"") != std::wstring::npos) s.showDafYomi = ParseJsonBool(line);
        else if (line.find(L"\"showYerushalmi\"") != std::wstring::npos) s.showYerushalmi = ParseJsonBool(line);
        else if (line.find(L"\"showHalachaYomit\"") != std::wstring::npos) s.showHalachaYomit = ParseJsonBool(line);
        else if (line.find(L"\"showMishnaYomit\"") != std::wstring::npos) s.showMishnaYomit = ParseJsonBool(line);
        else if (line.find(L"\"showTanachYomi\"") != std::wstring::npos) s.showTanachYomi = ParseJsonBool(line);
        else if (line.find(L"\"haftarahShita\"") != std::wstring::npos) s.haftarahShita = (int)ParseJsonNumber(line);
        else if (line.find(L"\"fontSize\"") != std::wstring::npos) s.fontSize = (int)ParseJsonNumber(line);
        else if (line.find(L"\"language\"") != std::wstring::npos) s.language = (int)ParseJsonNumber(line);
        else if (line.find(L"\"showTrayIcon\"")   != std::wstring::npos) s.showTrayIcon   = ParseJsonBool(line);
        else if (line.find(L"\"minimizeToTray\"") != std::wstring::npos) s.minimizeToTray = ParseJsonBool(line);
        else if (line.find(L"\"minimizeTrayWhen\"") != std::wstring::npos) s.minimizeTrayWhen = (int)ParseJsonNumber(line);
        else if (line.find(L"\"trayTextColor\"") != std::wstring::npos) s.trayTextColor = (int)ParseJsonNumber(line);
        else if (line.find(L"\"trayTooltipZmanimMask\"") != std::wstring::npos) s.trayTooltipZmanimMask = (uint32_t)ParseJsonNumber(line);
        else if (line.find(L"\"minimizeOnStartup\"") != std::wstring::npos) s.minimizeOnStartup = ParseJsonBool(line);
        else if (line.find(L"\"startWithWindows\"") != std::wstring::npos) s.startWithWindows = ParseJsonBool(line);
        else if (line.find(L"\"desktopShortcut\"") != std::wstring::npos) s.desktopShortcut = ParseJsonBool(line);
        else if (line.find(L"\"printWeeklyZmanim\"") != std::wstring::npos) s.printWeeklyZmanim = ParseJsonBool(line);
        else if (line.find(L"\"candleLightingMinutes\"") != std::wstring::npos) s.candleLightingMinutes = (int)ParseJsonNumber(line);
        else if (line.find(L"\"webCalendarUrl\"") != std::wstring::npos) s.webCalendarUrl = ParseJsonString(line);
        else if (line.find(L"\"webCal") != std::wstring::npos && line.find(L"_url\"") != std::wstring::npos)
        {
            size_t p = line.find(L"\"webCal") + 7;
            size_t q = line.find(L'_', p);
            if (q != std::wstring::npos) {
                int idx = std::stoi(line.substr(p, q - p));
                while ((int)s.webCalendars.size() <= idx) s.webCalendars.push_back({L"", true});
                s.webCalendars[idx].url = ParseJsonString(line);
            }
        }
        else if (line.find(L"\"webCal") != std::wstring::npos && line.find(L"_enabled\"") != std::wstring::npos)
        {
            size_t p = line.find(L"\"webCal") + 7;
            size_t q = line.find(L'_', p);
            if (q != std::wstring::npos) {
                int idx = std::stoi(line.substr(p, q - p));
                while ((int)s.webCalendars.size() <= idx) s.webCalendars.push_back({L"", true});
                s.webCalendars[idx].enabled = ParseJsonBool(line);
            }
        }
        else if (line.find(L"\"notifyPersonalEvents\"") != std::wstring::npos) s.notifyPersonalEvents = (int)ParseJsonNumber(line);
        else if (line.find(L"\"notifyWebCalEvents\"")   != std::wstring::npos) s.notifyWebCalEvents   = (int)ParseJsonNumber(line);
        else if (line.find(L"\"notifyZmanimStyle\"")    != std::wstring::npos) s.notifyZmanimStyle    = (int)ParseJsonNumber(line);
        else if (line.find(L"\"notifyZmanimMask\"")     != std::wstring::npos) s.notifyZmanimMask     = (uint32_t)ParseJsonNumber(line);
        else if (line.find(L"\"notifyMoadimStyle\"")    != std::wstring::npos) s.notifyMoadimStyle    = (int)ParseJsonNumber(line);
        else if (line.find(L"\"notifyMoadimOffsets\"")  != std::wstring::npos) s.notifyMoadimOffsets  = ParseJsonString(line);
        else if (line.find(L"\"notifyParshaStyle\"")    != std::wstring::npos) s.notifyParshaStyle    = (int)ParseJsonNumber(line);
        else if (line.find(L"\"notifyParshaName\"")     != std::wstring::npos) s.notifyParshaName     = ParseJsonString(line);
        else if (line.find(L"\"notifyParshaOffsets\"")  != std::wstring::npos) s.notifyParshaOffsets  = ParseJsonString(line);
        else if (line.find(L"\"notifyPersonalOffsets\"")!= std::wstring::npos) s.notifyPersonalOffsets= ParseJsonString(line);
        else if (line.find(L"\"advancedReminderCount\"") != std::wstring::npos) s.advancedReminders.resize((int)ParseJsonNumber(line));
        else if (line.find(L"\"advancedReminder") != std::wstring::npos)
        {
            size_t p = line.find(L"\"advancedReminder") + 17;
            size_t q = line.find(L'_', p);
            if (q != std::wstring::npos)
            {
                int idx = 0;
                try { idx = std::stoi(line.substr(p, q - p)); } catch (...) { idx = -1; }
                if (idx >= 0)
                {
                    while ((int)s.advancedReminders.size() <= idx) s.advancedReminders.push_back(ReminderRule{});
                    std::wstring field = line.substr(q + 1);
                    field = field.substr(0, field.find(L'"'));
                    if (field == L"enabled") s.advancedReminders[idx].enabled = ParseJsonBool(line);
                    else if (field == L"style") s.advancedReminders[idx].style = (int)ParseJsonNumber(line);
                    else if (field == L"kind") s.advancedReminders[idx].kind = ParseJsonString(line);
                    else if (field == L"target") s.advancedReminders[idx].target = ParseJsonString(line);
                    else if (field == L"offsets") s.advancedReminders[idx].offsets = ParseJsonString(line);
                }
            }
        }
        else if (line.find(L"\"zmanimShita\"")     != std::wstring::npos) s.zmanimShita     = (int)ParseJsonNumber(line);
        else if (line.find(L"\"alotShita\"")       != std::wstring::npos) s.alotShita       = (int)ParseJsonNumber(line);
        else if (line.find(L"\"tzeitShita\"")      != std::wstring::npos) s.tzeitShita      = (int)ParseJsonNumber(line);
        else if (line.find(L"\"printLandscape\"")  != std::wstring::npos) s.printLandscape  = ParseJsonBool(line);
        else if (line.find(L"\"printRange\"")      != std::wstring::npos) s.printRange      = (int)ParseJsonNumber(line);
        else if (line.find(L"\"printMarginTop\"")  != std::wstring::npos) s.printMarginTop  = (float)ParseJsonNumber(line);
        else if (line.find(L"\"printMarginBot\"")  != std::wstring::npos) s.printMarginBot  = (float)ParseJsonNumber(line);
        else if (line.find(L"\"printMarginLeft\"") != std::wstring::npos) s.printMarginLeft = (float)ParseJsonNumber(line);
        else if (line.find(L"\"printMarginRight\"")  != std::wstring::npos) s.printMarginRight    = (float)ParseJsonNumber(line);
        else if (line.find(L"\"printZmanimColMask\"")!= std::wstring::npos) s.printZmanimColMask = (uint32_t)ParseJsonNumber(line);
        else if (line.find(L"\"printShowFooter\"")   != std::wstring::npos) s.printShowFooter   = ParseJsonBool(line);
        else if (line.find(L"\"showChatzosOnFasts\"") != std::wstring::npos) s.showChatzosOnFasts = ParseJsonBool(line);
        else if (line.find(L"\"customAlotMode\"")     != std::wstring::npos) s.customAlotMode = (int)ParseJsonNumber(line);
        else if (line.find(L"\"customAlotValue\"")    != std::wstring::npos) s.customAlotValue = ParseJsonNumber(line);
        else if (line.find(L"\"customTzeitMode\"")    != std::wstring::npos) s.customTzeitMode = (int)ParseJsonNumber(line);
        else if (line.find(L"\"customTzeitValue\"")   != std::wstring::npos) s.customTzeitValue = ParseJsonNumber(line);
        else if (line.find(L"\"colorNormalCell\"")      != std::wstring::npos) s.colorNormalCell = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorOtherMonthCell\"")  != std::wstring::npos) s.colorOtherMonthCell = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorTodayCell\"")       != std::wstring::npos) s.colorTodayCell = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorShabbosCell\"")     != std::wstring::npos) s.colorShabbosCell = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorYomTovCell\"")      != std::wstring::npos) s.colorYomTovCell = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorRoshChodeshCell\"") != std::wstring::npos) s.colorRoshChodeshCell = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorCholHamoedCell\"")  != std::wstring::npos) s.colorCholHamoedCell = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorFastDayCell\"")     != std::wstring::npos) s.colorFastDayCell = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorGregorianText\"")   != std::wstring::npos) s.colorGregorianText = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorHebrewText\"")      != std::wstring::npos) s.colorHebrewText = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorHolidayText\"")     != std::wstring::npos) s.colorHolidayText = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorParshaText\"")      != std::wstring::npos) s.colorParshaText = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorCivilEventText\"")  != std::wstring::npos) s.colorCivilEventText = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorHebrewEventText\"") != std::wstring::npos) s.colorHebrewEventText = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorOmerText\"")        != std::wstring::npos) s.colorOmerText = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorLearningText\"")    != std::wstring::npos) s.colorLearningText = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorCandleText\"")      != std::wstring::npos) s.colorCandleText = (int)ParseJsonNumber(line);
        else if (line.find(L"\"colorMotzText\"")        != std::wstring::npos) s.colorMotzText = (int)ParseJsonNumber(line);
        else if (line.find(L"\"sidebarWidth\"")     != std::wstring::npos) s.sidebarWidth     = (int)ParseJsonNumber(line);
        else if (line.find(L"\"zmanimHeight\"")     != std::wstring::npos) s.zmanimHeight     = (int)ParseJsonNumber(line);
        else if (line.find(L"\"sidebarCollapsed\"") != std::wstring::npos) s.sidebarCollapsed = ParseJsonBool(line);
        else if (line.find(L"\"countdownTitleFontFace\"") != std::wstring::npos) s.countdownTitleFontFace = ParseJsonString(line);
        else if (line.find(L"\"countdownTitleFontSize\"") != std::wstring::npos) s.countdownTitleFontSize = (int)ParseJsonNumber(line);
        else if (line.find(L"\"countdownTitleTextColor\"") != std::wstring::npos) s.countdownTitleTextColor = (int)ParseJsonNumber(line);
        else if (line.find(L"\"countdownTitleBackColor\"") != std::wstring::npos) s.countdownTitleBackColor = (int)ParseJsonNumber(line);
        else if (line.find(L"\"countdownClockFontFace\"") != std::wstring::npos) s.countdownClockFontFace = ParseJsonString(line);
        else if (line.find(L"\"countdownClockFontSize\"") != std::wstring::npos) s.countdownClockFontSize = (int)ParseJsonNumber(line);
        else if (line.find(L"\"countdownClockTextColor\"") != std::wstring::npos) s.countdownClockTextColor = (int)ParseJsonNumber(line);
        else if (line.find(L"\"countdownClockBackColor\"") != std::wstring::npos) s.countdownClockBackColor = (int)ParseJsonNumber(line);
        else if (line.find(L"\"countdownCurrentFontFace\"") != std::wstring::npos) s.countdownCurrentFontFace = ParseJsonString(line);
        else if (line.find(L"\"countdownCurrentFontSize\"") != std::wstring::npos) s.countdownCurrentFontSize = (int)ParseJsonNumber(line);
        else if (line.find(L"\"countdownCurrentTextColor\"") != std::wstring::npos) s.countdownCurrentTextColor = (int)ParseJsonNumber(line);
        else if (line.find(L"\"countdownCurrentBackColor\"") != std::wstring::npos) s.countdownCurrentBackColor = (int)ParseJsonNumber(line);
        else if (line.find(L"\"countdownLiveFontFace\"") != std::wstring::npos) s.countdownLiveFontFace = ParseJsonString(line);
        else if (line.find(L"\"countdownLiveFontSize\"") != std::wstring::npos) s.countdownLiveFontSize = (int)ParseJsonNumber(line);
        else if (line.find(L"\"countdownLiveTextColor\"") != std::wstring::npos) s.countdownLiveTextColor = (int)ParseJsonNumber(line);
        else if (line.find(L"\"countdownLiveBackColor\"") != std::wstring::npos) s.countdownLiveBackColor = (int)ParseJsonNumber(line);
        else if (line.find(L"\"windowX\"")         != std::wstring::npos) s.windowX         = (int)ParseJsonNumber(line);
        else if (line.find(L"\"windowY\"") != std::wstring::npos) s.windowY = (int)ParseJsonNumber(line);
        else if (line.find(L"\"windowW\"") != std::wstring::npos) s.windowW = (int)ParseJsonNumber(line);
        else if (line.find(L"\"windowH\"") != std::wstring::npos) s.windowH = (int)ParseJsonNumber(line);
    }

    // Migrate legacy single URL to new list
    if (s.webCalendars.empty() && !s.webCalendarUrl.empty())
        s.webCalendars.push_back({ s.webCalendarUrl, true });

    // Load personal events from separate file
    LoadEvents(s.userEvents);

    return true;
}

// =============================================================================
// EVENTS FILE PATH
// =============================================================================

std::wstring GetEventsFilePath()
{
    wchar_t path[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path)))
    {
        std::wstring dir = std::wstring(path) + L"\\WinLuach";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir + L"\\events.json";
    }
    return L"events.json";
}

// =============================================================================
// EVENTS SAVE / LOAD
// =============================================================================

static void WriteEvent(std::wofstream& f, const UserEventEntry& e, int i)
{
    f << L"  \"ev" << i << L"_name\": \""       << JsonEscape(e.name) << L"\",\n";
    f << L"  \"ev" << i << L"_type\": "          << e.type            << L",\n";
    f << L"  \"ev" << i << L"_gregMonth\": "     << e.gregMonth       << L",\n";
    f << L"  \"ev" << i << L"_gregDay\": "       << e.gregDay         << L",\n";
    f << L"  \"ev" << i << L"_hebMonth\": "      << e.hebMonth        << L",\n";
    f << L"  \"ev" << i << L"_hebDay\": "        << e.hebDay          << L",\n";
    f << L"  \"ev" << i << L"_afterSunset\": "   << (e.afterSunset ? L"true" : L"false") << L",\n";
    f << L"  \"ev" << i << L"_gregYear\": "      << e.gregYear                           << L",\n";
    f << L"  \"ev" << i << L"_hebYear\": "       << e.hebYear                            << L",\n";
    f << L"  \"ev" << i << L"_notify\": "        << (e.notify ? L"true" : L"false")       << L",\n";
    f << L"  \"ev" << i << L"_alarmOffsets\": \"" << JsonEscape(e.alarmOffsets)           << L"\",\n";
}

bool SaveEvents(const std::vector<UserEventEntry>& events)
{
    return ExportEvents(events, GetEventsFilePath());
}

bool ExportEvents(const std::vector<UserEventEntry>& events, const std::wstring& path)
{
    std::wofstream f(path);
    if (!f.is_open()) return false;
    f << L"{\n";
    f << L"  \"eventCount\": " << events.size() << L",\n";
    for (int i = 0; i < (int)events.size(); i++)
        WriteEvent(f, events[i], i);
    f << L"  \"_end\": 0\n}\n";
    return true;
}

static void ParseEventsFromStream(std::wifstream& f, std::vector<UserEventEntry>& events)
{
    int count = 0;
    std::vector<std::wstring> lines;
    std::wstring ln;
    while (std::getline(f, ln)) lines.push_back(ln);

    for (auto& line : lines)
        if (line.find(L"\"eventCount\"") != std::wstring::npos)
            count = (int)ParseJsonNumber(line);

    events.resize(count);
    for (auto& line : lines)
    {
        // Match "evN_field": value  — extract N and field
        size_t q1 = line.find(L"\"ev");
        if (q1 == std::wstring::npos) continue;
        size_t start = q1 + 3;
        size_t us    = line.find(L'_', start);
        if (us == std::wstring::npos) continue;
        int idx = 0;
        try { idx = std::stoi(line.substr(start, us - start)); } catch (...) { continue; }
        if (idx < 0 || idx >= count) continue;
        std::wstring field = line.substr(us + 1);
        field = field.substr(0, field.find(L'"'));

        if      (field == L"name")        events[idx].name        = ParseJsonString(line);
        else if (field == L"type")        events[idx].type        = (int)ParseJsonNumber(line);
        else if (field == L"gregMonth")   events[idx].gregMonth   = (int)ParseJsonNumber(line);
        else if (field == L"gregDay")     events[idx].gregDay     = (int)ParseJsonNumber(line);
        else if (field == L"hebMonth")    events[idx].hebMonth    = (int)ParseJsonNumber(line);
        else if (field == L"hebDay")      events[idx].hebDay      = (int)ParseJsonNumber(line);
        else if (field == L"afterSunset") events[idx].afterSunset = ParseJsonBool(line);
        else if (field == L"gregYear")    events[idx].gregYear    = (int)ParseJsonNumber(line);
        else if (field == L"hebYear")     events[idx].hebYear     = (int)ParseJsonNumber(line);
        else if (field == L"notify")      events[idx].notify      = ParseJsonBool(line);
        else if (field == L"alarmOffsets") events[idx].alarmOffsets = ParseJsonString(line);
    }
}

bool LoadEvents(std::vector<UserEventEntry>& events)
{
    std::wifstream f(GetEventsFilePath());
    if (!f.is_open()) return false;
    ParseEventsFromStream(f, events);
    return true;
}

int ImportEvents(std::vector<UserEventEntry>& events, const std::wstring& path)
{
    std::wifstream f(path);
    if (!f.is_open()) return 0;
    std::vector<UserEventEntry> imported;
    ParseEventsFromStream(f, imported);
    int added = (int)imported.size();
    events.insert(events.end(), imported.begin(), imported.end());
    return added;
}

std::vector<UserEventEntry> ParseEventsFromFile(const std::wstring& path)
{
    std::wifstream f(path);
    if (!f.is_open()) return {};
    std::vector<UserEventEntry> result;
    ParseEventsFromStream(f, result);
    return result;
}
