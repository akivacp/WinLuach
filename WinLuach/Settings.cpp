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
    f << L"  \"minimizeToTray\": " << (s.minimizeToTray ? L"true" : L"false") << L",\n";
    f << L"  \"minimizeTrayWhen\": " << s.minimizeTrayWhen << L",\n";
    f << L"  \"trayTextColor\": " << s.trayTextColor << L",\n";
    f << L"  \"minimizeOnStartup\": " << (s.minimizeOnStartup ? L"true" : L"false") << L",\n";
    f << L"  \"startWithWindows\": " << (s.startWithWindows ? L"true" : L"false") << L",\n";
    f << L"  \"desktopShortcut\": " << (s.desktopShortcut ? L"true" : L"false") << L",\n";
    f << L"  \"printWeeklyZmanim\": " << (s.printWeeklyZmanim ? L"true" : L"false") << L",\n";
    f << L"  \"candleLightingMinutes\": " << s.candleLightingMinutes << L",\n";
    f << L"  \"webCalendarUrl\": \"" << JsonEscape(s.webCalendarUrl) << L"\",\n";
    f << L"  \"zmanimShita\": " << s.zmanimShita << L",\n";
    f << L"  \"windowX\": " << s.windowX << L",\n";
    f << L"  \"windowY\": " << s.windowY << L",\n";
    f << L"  \"windowW\": " << s.windowW << L",\n";
    f << L"  \"windowH\": " << s.windowH << L"\n";
    f << L"}\n";

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
        else if (line.find(L"\"minimizeToTray\"") != std::wstring::npos) s.minimizeToTray = ParseJsonBool(line);
        else if (line.find(L"\"minimizeTrayWhen\"") != std::wstring::npos) s.minimizeTrayWhen = (int)ParseJsonNumber(line);
        else if (line.find(L"\"trayTextColor\"") != std::wstring::npos) s.trayTextColor = (int)ParseJsonNumber(line);
        else if (line.find(L"\"minimizeOnStartup\"") != std::wstring::npos) s.minimizeOnStartup = ParseJsonBool(line);
        else if (line.find(L"\"startWithWindows\"") != std::wstring::npos) s.startWithWindows = ParseJsonBool(line);
        else if (line.find(L"\"desktopShortcut\"") != std::wstring::npos) s.desktopShortcut = ParseJsonBool(line);
        else if (line.find(L"\"printWeeklyZmanim\"") != std::wstring::npos) s.printWeeklyZmanim = ParseJsonBool(line);
        else if (line.find(L"\"candleLightingMinutes\"") != std::wstring::npos) s.candleLightingMinutes = (int)ParseJsonNumber(line);
        else if (line.find(L"\"webCalendarUrl\"") != std::wstring::npos) s.webCalendarUrl = ParseJsonString(line);
        else if (line.find(L"\"zmanimShita\"") != std::wstring::npos) s.zmanimShita = (int)ParseJsonNumber(line);
        else if (line.find(L"\"windowX\"") != std::wstring::npos) s.windowX = (int)ParseJsonNumber(line);
        else if (line.find(L"\"windowY\"") != std::wstring::npos) s.windowY = (int)ParseJsonNumber(line);
        else if (line.find(L"\"windowW\"") != std::wstring::npos) s.windowW = (int)ParseJsonNumber(line);
        else if (line.find(L"\"windowH\"") != std::wstring::npos) s.windowH = (int)ParseJsonNumber(line);
    }

    return true;
}
