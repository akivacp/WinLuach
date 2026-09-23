// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    Settings.cpp
// Purpose: Saves and loads user preferences to/from
//          the user's OneDrive Documents\WinLuach folder when available.
//          Hand-rolled JSON — no external library needed.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial implementation. Saves/loads all AppSettings fields.
//          Creates the WinLuach data directory if it doesn't exist.
// v0.8.0 - Save/load zmanim bar mask + per-sub-tab preset fields.
// v0.8.129 - Master backup: WriteMasterBackup bundles settings.json,
//            events.json and locations.json into one file; RestoreBackup
//            restores it (and still accepts old settings-only backups).
// =============================================================================

#include "pch.h"
#include "Settings.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <Windows.h>
#include <ShlObj.h>
#pragma comment(lib, "Shell32.lib")

// =============================================================================
// FILE PATH
// =============================================================================

static bool FileExists(const std::wstring& path)
{
    DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

static bool DirectoryExists(const std::wstring& path)
{
    DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

static bool EnsureDirectory(const std::wstring& dir)
{
    if (dir.empty() || dir == L".")
        return true;

    int rc = SHCreateDirectoryExW(nullptr, dir.c_str(), nullptr);
    return rc == ERROR_SUCCESS || rc == ERROR_ALREADY_EXISTS || DirectoryExists(dir);
}

static bool SamePath(const std::wstring& a, const std::wstring& b)
{
    return CompareStringOrdinal(a.c_str(), -1, b.c_str(), -1, TRUE) == CSTR_EQUAL;
}

static std::wstring AppendPath(const std::wstring& dir, const wchar_t* fileName)
{
    if (dir.empty() || dir == L".")
        return fileName;

    wchar_t last = dir.back();
    if (last == L'\\' || last == L'/')
        return dir + fileName;

    return dir + L"\\" + fileName;
}

static std::wstring GetEnvironmentVariableString(const wchar_t* name)
{
    DWORD needed = GetEnvironmentVariableW(name, nullptr, 0);
    if (needed == 0)
        return L"";

    std::wstring value(needed, L'\0');
    DWORD written = GetEnvironmentVariableW(name, value.data(), needed);
    if (written == 0 || written >= needed)
        return L"";

    value.resize(written);
    return value;
}

static std::wstring TrimTrailingSlashes(std::wstring path)
{
    while (path.size() > 3 && (path.back() == L'\\' || path.back() == L'/'))
        path.pop_back();
    return path;
}

static bool IsSubPathOf(const std::wstring& path, const std::wstring& dir)
{
    std::wstring cleanPath = TrimTrailingSlashes(path);
    std::wstring cleanDir = TrimTrailingSlashes(dir);
    if (cleanPath.size() < cleanDir.size() || cleanDir.empty())
        return false;

    int len = (int)cleanDir.size();
    if (CompareStringOrdinal(cleanPath.c_str(), len, cleanDir.c_str(), len, TRUE) != CSTR_EQUAL)
        return false;

    return cleanPath.size() == cleanDir.size() ||
        cleanPath[cleanDir.size()] == L'\\' ||
        cleanPath[cleanDir.size()] == L'/';
}

static std::vector<std::wstring> GetOneDriveRoots()
{
    const wchar_t* vars[] = { L"OneDrive", L"OneDriveCommercial", L"OneDriveConsumer" };
    std::vector<std::wstring> roots;

    for (const wchar_t* var : vars)
    {
        std::wstring root = TrimTrailingSlashes(GetEnvironmentVariableString(var));
        if (root.empty() || !DirectoryExists(root))
            continue;

        bool seen = false;
        for (const auto& existing : roots)
        {
            if (SamePath(root, existing))
            {
                seen = true;
                break;
            }
        }
        if (!seen)
            roots.push_back(root);
    }

    return roots;
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

static std::wstring GetWindowsDocumentsDirectory()
{
    std::wstring docs = GetKnownFolderPathString(FOLDERID_Documents);
    if (!docs.empty())
        return docs;

    wchar_t path[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, 0, path)))
        return path;

    return L"";
}

static std::wstring GetOneDriveDocumentsDirectory(const std::wstring& windowsDocuments)
{
    std::vector<std::wstring> roots = GetOneDriveRoots();

    for (const auto& root : roots)
    {
        if (!windowsDocuments.empty() && IsSubPathOf(windowsDocuments, root))
            return windowsDocuments;
    }

    if (!roots.empty())
        return AppendPath(roots.front(), L"Documents");

    return L"";
}

static std::wstring GetDocumentsDirectory()
{
    std::wstring windowsDocuments = GetWindowsDocumentsDirectory();
    std::wstring oneDriveDocuments = GetOneDriveDocumentsDirectory(windowsDocuments);
    return !oneDriveDocuments.empty() ? oneDriveDocuments : windowsDocuments;
}

static std::wstring GetLegacyWinLuachAppDataDirectory(bool create)
{
    wchar_t path[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path)))
        return L"";

    std::wstring dir = std::wstring(path) + L"\\WinLuach";
    if (!create || EnsureDirectory(dir))
        return dir;

    return L"";
}

std::wstring GetWinLuachDataDirectory()
{
    std::wstring docs = GetDocumentsDirectory();
    if (!docs.empty())
    {
        std::wstring dir = AppendPath(docs, L"WinLuach");
        if (EnsureDirectory(dir))
            return dir;
    }

    std::wstring appData = GetLegacyWinLuachAppDataDirectory(true);
    if (!appData.empty())
        return appData;

    return L".";
}

static void MigrateLegacyDataFile(const wchar_t* fileName, const std::wstring& targetPath)
{
    if (FileExists(targetPath))
        return;

    std::wstring legacyDir = GetLegacyWinLuachAppDataDirectory(false);
    if (legacyDir.empty())
        return;

    std::wstring legacyPath = AppendPath(legacyDir, fileName);
    if (SamePath(legacyPath, targetPath) || !FileExists(legacyPath))
        return;

    CopyFileW(legacyPath.c_str(), targetPath.c_str(), TRUE);
}

std::wstring GetWinLuachDataFilePath(const wchar_t* fileName)
{
    std::wstring path = AppendPath(GetWinLuachDataDirectory(), fileName);
    MigrateLegacyDataFile(fileName, path);
    return path;
}

// Returns OneDrive\Documents\WinLuach\settings.json when OneDrive is available.
std::wstring GetSettingsFilePath()
{
    return GetWinLuachDataFilePath(L"settings.json");
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
    // Read up to the closing quote, undoing JsonEscape (\" and \\)
    std::wstring out;
    for (size_t i = first + 1; i < line.size(); ++i)
    {
        if (line[i] == L'\\' && i + 1 < line.size()) out += line[++i];
        else if (line[i] == L'"') return out;
        else out += line[i];
    }
    return L"";
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

// Saves all settings to the WinLuach data directory.
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
    f << L"  \"useHebrewScript\": " << (s.useHebrewScript ? L"true" : L"false") << L",\n";
    f << L"  \"useHebrewNumerals\": " << (s.useHebrewNumerals ? L"true" : L"false") << L",\n";
    f << L"  \"showTrayIcon\": "   << (s.showTrayIcon   ? L"true" : L"false") << L",\n";
    f << L"  \"minimizeToTray\": " << (s.minimizeToTray ? L"true" : L"false") << L",\n";
    f << L"  \"minimizeTrayWhen\": " << s.minimizeTrayWhen << L",\n";
    f << L"  \"trayTextColor\": " << s.trayTextColor << L",\n";
    f << L"  \"trayBackEnabled\": " << (s.trayBackEnabled ? L"true" : L"false") << L",\n";
    f << L"  \"trayBackColor\": " << s.trayBackColor << L",\n";
    f << L"  \"trayFontFace\": \"" << JsonEscape(s.trayFontFace) << L"\",\n";
    f << L"  \"trayFontSize\": " << s.trayFontSize << L",\n";
    f << L"  \"trayFontBold\": " << (s.trayFontBold ? L"true" : L"false") << L",\n";
    f << L"  \"trayFontItalic\": " << (s.trayFontItalic ? L"true" : L"false") << L",\n";
    f << L"  \"trayNumberStyle\": " << s.trayNumberStyle << L",\n";
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
    f << L"  \"printDayZmanimMask\": " << (unsigned long long)s.printDayZmanimMask << L",\n";
    f << L"  \"printShowFooter\": " << (s.printShowFooter ? L"true" : L"false") << L",\n";
    f << L"  \"printTwoColumns\": " << (s.printTwoColumns ? L"true" : L"false") << L",\n";
    f << L"  \"printHebrewMode\": " << (s.printHebrewMode ? L"true" : L"false") << L",\n";
    f << L"  \"printRtlMode\": "    << (s.printRtlMode    ? L"true" : L"false") << L",\n";
    f << L"  \"printHebrewNumerals\": " << (s.printHebrewNumerals ? L"true" : L"false") << L",\n";
    f << L"  \"showChatzosOnFasts\": "  << (s.showChatzosOnFasts  ? L"true" : L"false") << L",\n";
    f << L"  \"showBeHaB\": "           << (s.showBeHaB           ? L"true" : L"false") << L",\n";
    f << L"  \"showChatzosOnBeHaB\": "  << (s.showChatzosOnBeHaB  ? L"true" : L"false") << L",\n";
    f << L"  \"shaahZmanitShita\": " << s.shaahZmanitShita << L",\n";
    f << L"  \"sofZmanShaahMode\": " << s.sofZmanShaahMode << L",\n";
    f << L"  \"customShaahStartMode\": " << s.customShaahStartMode << L",\n";
    f << L"  \"customShaahStartValue\": " << s.customShaahStartValue << L",\n";
    f << L"  \"customShaahStartDegreesValue\": " << s.customShaahStartDegreesValue << L",\n";
    f << L"  \"customShaahEndMode\": " << s.customShaahEndMode << L",\n";
    f << L"  \"customShaahEndValue\": " << s.customShaahEndValue << L",\n";
    f << L"  \"customShaahEndDegreesValue\": " << s.customShaahEndDegreesValue << L",\n";
    f << L"  \"customAlotMode\": " << s.customAlotMode << L",\n";
    f << L"  \"customAlotValue\": " << s.customAlotValue << L",\n";
    f << L"  \"customAlotDegreesValue\": " << s.customAlotDegreesValue << L",\n";
    f << L"  \"customMisheyakirMode\": " << s.customMisheyakirMode << L",\n";
    f << L"  \"customMisheyakirValue\": " << s.customMisheyakirValue << L",\n";
    f << L"  \"customMisheyakirDegreesValue\": " << s.customMisheyakirDegreesValue << L",\n";
    f << L"  \"customSofZmanMode\": " << s.customSofZmanMode << L",\n";
    f << L"  \"customSofZmanValue\": " << s.customSofZmanValue << L",\n";
    f << L"  \"customSofZmanDegreesValue\": " << s.customSofZmanDegreesValue << L",\n";
    f << L"  \"customTzeitMode\": " << s.customTzeitMode << L",\n";
    f << L"  \"customTzeitValue\": " << s.customTzeitValue << L",\n";
    f << L"  \"customTzeitDegreesValue\": " << s.customTzeitDegreesValue << L",\n";
    f << L"  \"zmanimBarMask\": " << (uint32_t)s.zmanimBarMask << L",\n";
    f << L"  \"customMinchaGedolaPreset\": " << s.customMinchaGedolaPreset << L",\n";
    f << L"  \"customMinchaKetanaPreset\": " << s.customMinchaKetanaPreset << L",\n";
    f << L"  \"customPlagPreset\": " << s.customPlagPreset << L",\n";
    f << L"  \"customEndFastPreset\": " << s.customEndFastPreset << L",\n";
    f << L"  \"customMinchaGedolaValue\": " << s.customMinchaGedolaValue << L",\n";
    f << L"  \"customMinchaKetanaValue\": " << s.customMinchaKetanaValue << L",\n";
    f << L"  \"customPlagValue\": " << s.customPlagValue << L",\n";
    f << L"  \"customEndFastValue\": " << s.customEndFastValue << L",\n";
    f << L"  \"customEndFastMinuteMode\": " << s.customEndFastMinuteMode << L",\n";
    f << L"  \"customSofZmanMaPreset\": " << s.customSofZmanMaPreset << L",\n";
    f << L"  \"customSofZmanGraPreset\": " << s.customSofZmanGraPreset << L",\n";
    f << L"  \"customMisheyakirPreset\": " << s.customMisheyakirPreset << L",\n";
    f << L"  \"customTzeitPreset\": " << s.customTzeitPreset << L",\n";
    f << L"  \"customAlotPreset\": " << s.customAlotPreset << L",\n";
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
    f << L"  \"notifySefirahStyle\": " << s.notifySefirahStyle << L",\n";
    f << L"  \"notifySefirahTime\": \"" << JsonEscape(s.notifySefirahTime) << L"\",\n";
    f << L"  \"notifySefirahMode\": " << s.notifySefirahMode << L",\n";
    f << L"  \"notifySefirahOffsetMinutes\": " << s.notifySefirahOffsetMinutes << L",\n";
    f << L"  \"notifySefirahOffsetDir\": " << s.notifySefirahOffsetDir << L",\n";
    f << L"  \"notifySefirahBase\": " << s.notifySefirahBase << L",\n";
    f << L"  \"notifySefirahOtherZman\": " << s.notifySefirahOtherZman << L",\n";
    f << L"  \"notifyMoadimStyle\": " << s.notifyMoadimStyle << L",\n";
    f << L"  \"notifyMoadimOffsets\": \"" << JsonEscape(s.notifyMoadimOffsets) << L"\",\n";
    f << L"  \"notifyParshaStyle\": " << s.notifyParshaStyle << L",\n";
    f << L"  \"notifyParshaName\": \"" << JsonEscape(s.notifyParshaName) << L"\",\n";
    f << L"  \"notifyParshaOffsets\": \"" << JsonEscape(s.notifyParshaOffsets) << L"\",\n";
    f << L"  \"notifyPersonalOffsets\": \"" << JsonEscape(s.notifyPersonalOffsets) << L"\",\n";
    f << L"  \"useWinLuachToast\": " << (s.useWinLuachToast ? L"true" : L"false") << L",\n";
    f << L"  \"winLuachToastDuration\": " << s.winLuachToastDuration << L",\n";
    f << L"  \"winLuachToastDurationUnit\": " << s.winLuachToastDurationUnit << L",\n";
    f << L"  \"advancedReminderCount\": " << s.advancedReminders.size() << L",\n";
    for (int i = 0; i < (int)s.advancedReminders.size(); ++i)
    {
        const auto& r = s.advancedReminders[i];
        f << L"  \"advancedReminder" << i << L"_enabled\": " << (r.enabled ? L"true" : L"false") << L",\n";
        f << L"  \"advancedReminder" << i << L"_style\": " << r.style << L",\n";
        f << L"  \"advancedReminder" << i << L"_kind\": \"" << JsonEscape(r.kind) << L"\",\n";
        f << L"  \"advancedReminder" << i << L"_target\": \"" << JsonEscape(r.target) << L"\",\n";
        f << L"  \"advancedReminder" << i << L"_offsets\": \"" << JsonEscape(r.offsets) << L"\",\n";
        f << L"  \"advancedReminder" << i << L"_afterEvent\": "   << (r.afterEvent ? L"true" : L"false") << L",\n";
        f << L"  \"advancedReminder" << i << L"_anchor\": "      << r.anchor                              << L",\n";
        f << L"  \"advancedReminder" << i << L"_lastFiredDate\": \"" << JsonEscape(r.lastFiredDate)       << L"\",\n";
    }
    f << L"  \"reminderDailyHour\": "   << s.reminderDailyHour   << L",\n";
    f << L"  \"reminderDailyMinute\": " << s.reminderDailyMinute << L",\n";
    f << L"  \"sidebarWidth\": "     << s.sidebarWidth                              << L",\n";
    f << L"  \"zmanimHeight\": "     << s.zmanimHeight                              << L",\n";
    f << L"  \"sidebarCollapsed\": " << (s.sidebarCollapsed ? L"true" : L"false")   << L",\n";
    f << L"  \"paneSpecialTimesVisible\": " << (s.paneSpecialTimesVisible ? L"true" : L"false") << L",\n";
    f << L"  \"paneYearDetailsVisible\": " << (s.paneYearDetailsVisible ? L"true" : L"false") << L",\n";
    f << L"  \"paneMoladVisible\": " << (s.paneMoladVisible ? L"true" : L"false") << L",\n";
    f << L"  \"countdownTitleFontFace\": \"" << JsonEscape(s.countdownTitleFontFace) << L"\",\n";
    f << L"  \"countdownTitleFontSize\": " << s.countdownTitleFontSize << L",\n";
    f << L"  \"countdownTitleTextColor\": " << s.countdownTitleTextColor << L",\n";
    f << L"  \"countdownTitleBackColor\": " << s.countdownTitleBackColor << L",\n";
    f << L"  \"countdownTitleBold\": " << (s.countdownTitleBold ? L"true" : L"false") << L",\n";
    f << L"  \"countdownTitleItalic\": " << (s.countdownTitleItalic ? L"true" : L"false") << L",\n";
    f << L"  \"countdownClockFontFace\": \"" << JsonEscape(s.countdownClockFontFace) << L"\",\n";
    f << L"  \"countdownClockFontSize\": " << s.countdownClockFontSize << L",\n";
    f << L"  \"countdownClockTextColor\": " << s.countdownClockTextColor << L",\n";
    f << L"  \"countdownClockBackColor\": " << s.countdownClockBackColor << L",\n";
    f << L"  \"countdownClockBold\": " << (s.countdownClockBold ? L"true" : L"false") << L",\n";
    f << L"  \"countdownClockItalic\": " << (s.countdownClockItalic ? L"true" : L"false") << L",\n";
    f << L"  \"countdownCurrentFontFace\": \"" << JsonEscape(s.countdownCurrentFontFace) << L"\",\n";
    f << L"  \"countdownCurrentFontSize\": " << s.countdownCurrentFontSize << L",\n";
    f << L"  \"countdownCurrentTextColor\": " << s.countdownCurrentTextColor << L",\n";
    f << L"  \"countdownCurrentBackColor\": " << s.countdownCurrentBackColor << L",\n";
    f << L"  \"countdownCurrentBold\": " << (s.countdownCurrentBold ? L"true" : L"false") << L",\n";
    f << L"  \"countdownCurrentItalic\": " << (s.countdownCurrentItalic ? L"true" : L"false") << L",\n";
    f << L"  \"countdownLiveFontFace\": \"" << JsonEscape(s.countdownLiveFontFace) << L"\",\n";
    f << L"  \"countdownLiveFontSize\": " << s.countdownLiveFontSize << L",\n";
    f << L"  \"countdownLiveTextColor\": " << s.countdownLiveTextColor << L",\n";
    f << L"  \"countdownLiveBackColor\": " << s.countdownLiveBackColor << L",\n";
    f << L"  \"countdownLiveBold\": " << (s.countdownLiveBold ? L"true" : L"false") << L",\n";
    f << L"  \"countdownLiveItalic\": " << (s.countdownLiveItalic ? L"true" : L"false") << L",\n";
    f << L"  \"countdownZmanimMask\": " << s.countdownZmanimMask << L",\n";
    f << L"  \"countdownShowTitle\": " << (s.countdownShowTitle ? L"true" : L"false") << L",\n";
    f << L"  \"countdownShowClock\": " << (s.countdownShowClock ? L"true" : L"false") << L",\n";
    f << L"  \"countdownShowZmanTime\": " << (s.countdownShowZmanTime ? L"true" : L"false") << L",\n";
    f << L"  \"countdownShowLive\": " << (s.countdownShowLive ? L"true" : L"false") << L",\n";
    f << L"  \"countdownOpenOnStartup\": " << (s.countdownOpenOnStartup ? L"true" : L"false") << L",\n";
    f << L"  \"countdownAlwaysOnTop\": " << (s.countdownAlwaysOnTop ? L"true" : L"false") << L",\n";
    f << L"  \"dayDetailLandscape\": " << (s.dayDetailLandscape ? L"true" : L"false") << L",\n";
    f << L"  \"dayDetailShowFooter\": " << (s.dayDetailShowFooter ? L"true" : L"false") << L",\n";
    f << L"  \"dayDetailMarginTop\": " << s.dayDetailMarginTop << L",\n";
    f << L"  \"dayDetailMarginBot\": " << s.dayDetailMarginBot << L",\n";
    f << L"  \"dayDetailMarginLeft\": " << s.dayDetailMarginLeft << L",\n";
    f << L"  \"dayDetailMarginRight\": " << s.dayDetailMarginRight << L",\n";
    f << L"  \"yearDetailsMask\": " << s.yearDetailsMask << L",\n";
    f << L"  \"printEventCategoryMask\": " << (int)s.printEventCategoryMask << L",\n";
    f << L"  \"printEventSeparateCategories\": " << (s.printEventSeparateCategories ? L"true" : L"false") << L",\n";
    f << L"  \"trayTooltipCustomZmanimMask\": " << s.trayTooltipCustomZmanimMask << L",\n";
    f << L"  \"disableAutoUpdate\": " << (s.disableAutoUpdate ? L"true" : L"false") << L",\n";
    f << L"  \"checkUpdatesAuto\": " << (s.checkUpdatesAuto ? L"true" : L"false") << L",\n";
    f << L"  \"updateCheckFrequency\": " << s.updateCheckFrequency << L",\n";
    f << L"  \"checkUpdatesOnLaunch\": " << (s.checkUpdatesOnLaunch ? L"true" : L"false") << L",\n";
    f << L"  \"lastUpdateCheckTime\": " << s.lastUpdateCheckTime << L",\n";
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

// Loads settings from the WinLuach data directory.
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
        if (line.find(L"\"latitude\"") != std::wstring::npos) s.latitude = ParseJsonNumber(line);
        if (line.find(L"\"longitude\"") != std::wstring::npos) s.longitude = ParseJsonNumber(line);
        if (line.find(L"\"elevation\"") != std::wstring::npos) s.elevation = ParseJsonNumber(line);
        if (line.find(L"\"gmtOffset\"") != std::wstring::npos) s.gmtOffset = (int)ParseJsonNumber(line);
        if (line.find(L"\"usesDST\"") != std::wstring::npos) s.usesDST = ParseJsonBool(line);
        if (line.find(L"\"use24Hour\"") != std::wstring::npos) s.use24Hour = ParseJsonBool(line);
        if (line.find(L"\"isIsrael\"") != std::wstring::npos) s.isIsrael = ParseJsonBool(line);
        if (line.find(L"\"defaultHebrewMonth\"") != std::wstring::npos) s.defaultHebrewMonth = ParseJsonBool(line);
        if (line.find(L"\"dateTracking\"") != std::wstring::npos) s.dateTracking = ParseJsonBool(line);
        if (line.find(L"\"showParshios\"") != std::wstring::npos) s.showParshios = ParseJsonBool(line);
        if (line.find(L"\"showMoadim\"") != std::wstring::npos) s.showMoadim = ParseJsonBool(line);
        if (line.find(L"\"showUserEvents\"") != std::wstring::npos) s.showUserEvents = ParseJsonBool(line);
        if (line.find(L"\"showDafYomi\"") != std::wstring::npos) s.showDafYomi = ParseJsonBool(line);
        if (line.find(L"\"showYerushalmi\"") != std::wstring::npos) s.showYerushalmi = ParseJsonBool(line);
        if (line.find(L"\"showHalachaYomit\"") != std::wstring::npos) s.showHalachaYomit = ParseJsonBool(line);
        if (line.find(L"\"showMishnaYomit\"") != std::wstring::npos) s.showMishnaYomit = ParseJsonBool(line);
        if (line.find(L"\"showTanachYomi\"") != std::wstring::npos) s.showTanachYomi = ParseJsonBool(line);
        if (line.find(L"\"haftarahShita\"") != std::wstring::npos) s.haftarahShita = (int)ParseJsonNumber(line);
        if (line.find(L"\"fontSize\"") != std::wstring::npos) s.fontSize = (int)ParseJsonNumber(line);
        if (line.find(L"\"language\"") != std::wstring::npos) s.language = (int)ParseJsonNumber(line);
        if (line.find(L"\"useHebrewScript\"")   != std::wstring::npos) s.useHebrewScript   = ParseJsonBool(line);
        if (line.find(L"\"useHebrewNumerals\"") != std::wstring::npos) s.useHebrewNumerals = ParseJsonBool(line);
        if (line.find(L"\"showTrayIcon\"")   != std::wstring::npos) s.showTrayIcon   = ParseJsonBool(line);
        if (line.find(L"\"minimizeToTray\"") != std::wstring::npos) s.minimizeToTray = ParseJsonBool(line);
        if (line.find(L"\"minimizeTrayWhen\"") != std::wstring::npos) s.minimizeTrayWhen = (int)ParseJsonNumber(line);
        if (line.find(L"\"trayTextColor\"") != std::wstring::npos) s.trayTextColor = (int)ParseJsonNumber(line);
        if (line.find(L"\"trayBackEnabled\"") != std::wstring::npos) s.trayBackEnabled = ParseJsonBool(line);
        if (line.find(L"\"trayBackColor\"") != std::wstring::npos) s.trayBackColor = (int)ParseJsonNumber(line);
        if (line.find(L"\"trayFontFace\"") != std::wstring::npos) s.trayFontFace = ParseJsonString(line);
        if (line.find(L"\"trayFontSize\"") != std::wstring::npos) s.trayFontSize = (int)ParseJsonNumber(line);
        if (line.find(L"\"trayFontBold\"") != std::wstring::npos) s.trayFontBold = ParseJsonBool(line);
        if (line.find(L"\"trayFontItalic\"") != std::wstring::npos) s.trayFontItalic = ParseJsonBool(line);
        if (line.find(L"\"trayNumberStyle\"") != std::wstring::npos) s.trayNumberStyle = (int)ParseJsonNumber(line);
        if (line.find(L"\"trayTooltipZmanimMask\"") != std::wstring::npos) s.trayTooltipZmanimMask = (uint32_t)ParseJsonNumber(line);
        if (line.find(L"\"minimizeOnStartup\"") != std::wstring::npos) s.minimizeOnStartup = ParseJsonBool(line);
        if (line.find(L"\"startWithWindows\"") != std::wstring::npos) s.startWithWindows = ParseJsonBool(line);
        if (line.find(L"\"desktopShortcut\"") != std::wstring::npos) s.desktopShortcut = ParseJsonBool(line);
        if (line.find(L"\"printWeeklyZmanim\"") != std::wstring::npos) s.printWeeklyZmanim = ParseJsonBool(line);
        if (line.find(L"\"candleLightingMinutes\"") != std::wstring::npos) s.candleLightingMinutes = (int)ParseJsonNumber(line);
        if (line.find(L"\"webCalendarUrl\"") != std::wstring::npos) s.webCalendarUrl = ParseJsonString(line);
        if (line.find(L"\"webCal") != std::wstring::npos && line.find(L"_url\"") != std::wstring::npos)
        {
            size_t p = line.find(L"\"webCal") + 7;
            size_t q = line.find(L'_', p);
            if (q != std::wstring::npos) {
                int idx = std::stoi(line.substr(p, q - p));
                while ((int)s.webCalendars.size() <= idx) s.webCalendars.push_back({L"", true});
                s.webCalendars[idx].url = ParseJsonString(line);
            }
        }
        if (line.find(L"\"webCal") != std::wstring::npos && line.find(L"_enabled\"") != std::wstring::npos)
        {
            size_t p = line.find(L"\"webCal") + 7;
            size_t q = line.find(L'_', p);
            if (q != std::wstring::npos) {
                int idx = std::stoi(line.substr(p, q - p));
                while ((int)s.webCalendars.size() <= idx) s.webCalendars.push_back({L"", true});
                s.webCalendars[idx].enabled = ParseJsonBool(line);
            }
        }
        if (line.find(L"\"notifyPersonalEvents\"") != std::wstring::npos) s.notifyPersonalEvents = (int)ParseJsonNumber(line);
        if (line.find(L"\"notifyWebCalEvents\"")   != std::wstring::npos) s.notifyWebCalEvents   = (int)ParseJsonNumber(line);
        if (line.find(L"\"notifyZmanimStyle\"")    != std::wstring::npos) s.notifyZmanimStyle    = (int)ParseJsonNumber(line);
        if (line.find(L"\"notifyZmanimMask\"")     != std::wstring::npos) s.notifyZmanimMask     = (uint32_t)ParseJsonNumber(line);
        if (line.find(L"\"notifySefirahStyle\"")   != std::wstring::npos) s.notifySefirahStyle   = (int)ParseJsonNumber(line);
        if (line.find(L"\"notifySefirahTime\"")    != std::wstring::npos) s.notifySefirahTime    = ParseJsonString(line);
        if (line.find(L"\"notifySefirahMode\"")    != std::wstring::npos) s.notifySefirahMode    = (int)ParseJsonNumber(line);
        if (line.find(L"\"notifySefirahOffsetMinutes\"") != std::wstring::npos) s.notifySefirahOffsetMinutes = (int)ParseJsonNumber(line);
        if (line.find(L"\"notifySefirahOffsetDir\"") != std::wstring::npos) s.notifySefirahOffsetDir = (int)ParseJsonNumber(line);
        if (line.find(L"\"notifySefirahBase\"") != std::wstring::npos) s.notifySefirahBase = (int)ParseJsonNumber(line);
        if (line.find(L"\"notifySefirahOtherZman\"") != std::wstring::npos) s.notifySefirahOtherZman = (int)ParseJsonNumber(line);
        if (line.find(L"\"notifyMoadimStyle\"")    != std::wstring::npos) s.notifyMoadimStyle    = (int)ParseJsonNumber(line);
        if (line.find(L"\"notifyMoadimOffsets\"")  != std::wstring::npos) s.notifyMoadimOffsets  = ParseJsonString(line);
        if (line.find(L"\"notifyParshaStyle\"")    != std::wstring::npos) s.notifyParshaStyle    = (int)ParseJsonNumber(line);
        if (line.find(L"\"notifyParshaName\"")     != std::wstring::npos) s.notifyParshaName     = ParseJsonString(line);
        if (line.find(L"\"notifyParshaOffsets\"")  != std::wstring::npos) s.notifyParshaOffsets  = ParseJsonString(line);
        if (line.find(L"\"notifyPersonalOffsets\"")!= std::wstring::npos) s.notifyPersonalOffsets= ParseJsonString(line);
        if (line.find(L"\"useWinLuachToast\"") != std::wstring::npos) s.useWinLuachToast = ParseJsonBool(line);
        if (line.find(L"\"winLuachToastDuration\"") != std::wstring::npos) s.winLuachToastDuration = (int)ParseJsonNumber(line);
        if (line.find(L"\"winLuachToastDurationUnit\"") != std::wstring::npos) s.winLuachToastDurationUnit = (int)ParseJsonNumber(line);
        if (line.find(L"\"advancedReminderCount\"") != std::wstring::npos) s.advancedReminders.resize((int)ParseJsonNumber(line));
        if (line.find(L"\"advancedReminder") != std::wstring::npos)
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
                    if (field == L"enabled")    s.advancedReminders[idx].enabled    = ParseJsonBool(line);
                    if (field == L"style")      s.advancedReminders[idx].style      = (int)ParseJsonNumber(line);
                    if (field == L"kind")       s.advancedReminders[idx].kind       = ParseJsonString(line);
                    if (field == L"target")     s.advancedReminders[idx].target     = ParseJsonString(line);
                    if (field == L"offsets")    s.advancedReminders[idx].offsets    = ParseJsonString(line);
                    if (field == L"afterEvent")    s.advancedReminders[idx].afterEvent    = ParseJsonBool(line);
                    if (field == L"anchor")        s.advancedReminders[idx].anchor        = (int)ParseJsonNumber(line);
                    if (field == L"lastFiredDate") s.advancedReminders[idx].lastFiredDate = ParseJsonString(line);
                }
            }
        }
        if (line.find(L"\"zmanimShita\"")     != std::wstring::npos) s.zmanimShita     = (int)ParseJsonNumber(line);
        if (line.find(L"\"alotShita\"")       != std::wstring::npos) s.alotShita       = (int)ParseJsonNumber(line);
        if (line.find(L"\"tzeitShita\"")      != std::wstring::npos) s.tzeitShita      = (int)ParseJsonNumber(line);
        if (line.find(L"\"printLandscape\"")  != std::wstring::npos) s.printLandscape  = ParseJsonBool(line);
        if (line.find(L"\"printRange\"")      != std::wstring::npos) s.printRange      = (int)ParseJsonNumber(line);
        if (line.find(L"\"printMarginTop\"")  != std::wstring::npos) s.printMarginTop  = (float)ParseJsonNumber(line);
        if (line.find(L"\"printMarginBot\"")  != std::wstring::npos) s.printMarginBot  = (float)ParseJsonNumber(line);
        if (line.find(L"\"printMarginLeft\"") != std::wstring::npos) s.printMarginLeft = (float)ParseJsonNumber(line);
        if (line.find(L"\"printMarginRight\"")  != std::wstring::npos) s.printMarginRight    = (float)ParseJsonNumber(line);
        if (line.find(L"\"printZmanimColMask\"")!= std::wstring::npos) s.printZmanimColMask = (uint32_t)ParseJsonNumber(line);
        if (line.find(L"\"printDayZmanimMask\"")!= std::wstring::npos) s.printDayZmanimMask = (uint64_t)ParseJsonNumber(line);
        if (line.find(L"\"printShowFooter\"")   != std::wstring::npos) s.printShowFooter   = ParseJsonBool(line);
        if (line.find(L"\"printTwoColumns\"")   != std::wstring::npos) s.printTwoColumns   = ParseJsonBool(line);
        if (line.find(L"\"printHebrewMode\"")   != std::wstring::npos) s.printHebrewMode   = ParseJsonBool(line);
        if (line.find(L"\"printRtlMode\"")      != std::wstring::npos) s.printRtlMode      = ParseJsonBool(line);
        if (line.find(L"\"printHebrewNumerals\"") != std::wstring::npos) s.printHebrewNumerals = ParseJsonBool(line);
        if (line.find(L"\"showChatzosOnFasts\"")  != std::wstring::npos) s.showChatzosOnFasts  = ParseJsonBool(line);
        if (line.find(L"\"showBeHaB\"")           != std::wstring::npos) s.showBeHaB           = ParseJsonBool(line);
        if (line.find(L"\"showChatzosOnBeHaB\"")  != std::wstring::npos) s.showChatzosOnBeHaB  = ParseJsonBool(line);
        if (line.find(L"\"shaahZmanitShita\"")   != std::wstring::npos) s.shaahZmanitShita = (int)ParseJsonNumber(line);
        if (line.find(L"\"sofZmanShaahMode\"")   != std::wstring::npos) s.sofZmanShaahMode = (int)ParseJsonNumber(line);
        if (line.find(L"\"customShaahStartMode\"") != std::wstring::npos) s.customShaahStartMode = (int)ParseJsonNumber(line);
        if (line.find(L"\"customShaahStartValue\"") != std::wstring::npos) s.customShaahStartValue = ParseJsonNumber(line);
        if (line.find(L"\"customShaahStartDegreesValue\"") != std::wstring::npos) s.customShaahStartDegreesValue = ParseJsonNumber(line);
        if (line.find(L"\"customShaahEndMode\"") != std::wstring::npos) s.customShaahEndMode = (int)ParseJsonNumber(line);
        if (line.find(L"\"customShaahEndValue\"") != std::wstring::npos) s.customShaahEndValue = ParseJsonNumber(line);
        if (line.find(L"\"customShaahEndDegreesValue\"") != std::wstring::npos) s.customShaahEndDegreesValue = ParseJsonNumber(line);
        if (line.find(L"\"customAlotMode\"")     != std::wstring::npos) s.customAlotMode = (int)ParseJsonNumber(line);
        if (line.find(L"\"customAlotValue\"")    != std::wstring::npos) s.customAlotValue = ParseJsonNumber(line);
        if (line.find(L"\"customAlotDegreesValue\"")   != std::wstring::npos) s.customAlotDegreesValue = ParseJsonNumber(line);
        if (line.find(L"\"customMisheyakirMode\"")  != std::wstring::npos) s.customMisheyakirMode = (int)ParseJsonNumber(line);
        if (line.find(L"\"customMisheyakirValue\"") != std::wstring::npos) s.customMisheyakirValue = ParseJsonNumber(line);
        if (line.find(L"\"customMisheyakirDegreesValue\"") != std::wstring::npos) s.customMisheyakirDegreesValue = ParseJsonNumber(line);
        if (line.find(L"\"customSofZmanMode\"")     != std::wstring::npos) s.customSofZmanMode = (int)ParseJsonNumber(line);
        if (line.find(L"\"customSofZmanValue\"")    != std::wstring::npos) s.customSofZmanValue = ParseJsonNumber(line);
        if (line.find(L"\"customSofZmanDegreesValue\"")  != std::wstring::npos) s.customSofZmanDegreesValue = ParseJsonNumber(line);
        if (line.find(L"\"customTzeitMode\"")    != std::wstring::npos) s.customTzeitMode = (int)ParseJsonNumber(line);
        if (line.find(L"\"customTzeitValue\"")   != std::wstring::npos) s.customTzeitValue = ParseJsonNumber(line);
        if (line.find(L"\"customTzeitDegreesValue\"")  != std::wstring::npos) s.customTzeitDegreesValue = ParseJsonNumber(line);
        if (line.find(L"\"zmanimBarMask\"")            != std::wstring::npos) s.zmanimBarMask = (uint32_t)ParseJsonNumber(line);
        if (line.find(L"\"customMinchaGedolaPreset\"") != std::wstring::npos) s.customMinchaGedolaPreset = (int)ParseJsonNumber(line);
        if (line.find(L"\"customMinchaKetanaPreset\"") != std::wstring::npos) s.customMinchaKetanaPreset = (int)ParseJsonNumber(line);
        if (line.find(L"\"customPlagPreset\"")         != std::wstring::npos) s.customPlagPreset         = (int)ParseJsonNumber(line);
        if (line.find(L"\"customEndFastPreset\"")      != std::wstring::npos) s.customEndFastPreset      = (int)ParseJsonNumber(line);
        if (line.find(L"\"customMinchaGedolaValue\"")  != std::wstring::npos) s.customMinchaGedolaValue  = ParseJsonNumber(line);
        if (line.find(L"\"customMinchaKetanaValue\"")  != std::wstring::npos) s.customMinchaKetanaValue  = ParseJsonNumber(line);
        if (line.find(L"\"customPlagValue\"")          != std::wstring::npos) s.customPlagValue          = ParseJsonNumber(line);
        if (line.find(L"\"customEndFastValue\"")       != std::wstring::npos) s.customEndFastValue       = ParseJsonNumber(line);
        if (line.find(L"\"customEndFastMinuteMode\"")  != std::wstring::npos) s.customEndFastMinuteMode  = (int)ParseJsonNumber(line);
        if (line.find(L"\"customSofZmanMaPreset\"")    != std::wstring::npos) s.customSofZmanMaPreset    = (int)ParseJsonNumber(line);
        if (line.find(L"\"customSofZmanGraPreset\"")   != std::wstring::npos) s.customSofZmanGraPreset   = (int)ParseJsonNumber(line);
        if (line.find(L"\"customMisheyakirPreset\"")   != std::wstring::npos) s.customMisheyakirPreset   = (int)ParseJsonNumber(line);
        if (line.find(L"\"customTzeitPreset\"")        != std::wstring::npos) s.customTzeitPreset        = (int)ParseJsonNumber(line);
        if (line.find(L"\"customAlotPreset\"")         != std::wstring::npos) s.customAlotPreset         = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorNormalCell\"")      != std::wstring::npos) s.colorNormalCell = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorOtherMonthCell\"")  != std::wstring::npos) s.colorOtherMonthCell = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorTodayCell\"")       != std::wstring::npos) s.colorTodayCell = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorShabbosCell\"")     != std::wstring::npos) s.colorShabbosCell = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorYomTovCell\"")      != std::wstring::npos) s.colorYomTovCell = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorRoshChodeshCell\"") != std::wstring::npos) s.colorRoshChodeshCell = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorCholHamoedCell\"")  != std::wstring::npos) s.colorCholHamoedCell = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorFastDayCell\"")     != std::wstring::npos) s.colorFastDayCell = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorGregorianText\"")   != std::wstring::npos) s.colorGregorianText = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorHebrewText\"")      != std::wstring::npos) s.colorHebrewText = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorHolidayText\"")     != std::wstring::npos) s.colorHolidayText = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorParshaText\"")      != std::wstring::npos) s.colorParshaText = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorCivilEventText\"")  != std::wstring::npos) s.colorCivilEventText = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorHebrewEventText\"") != std::wstring::npos) s.colorHebrewEventText = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorOmerText\"")        != std::wstring::npos) s.colorOmerText = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorLearningText\"")    != std::wstring::npos) s.colorLearningText = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorCandleText\"")      != std::wstring::npos) s.colorCandleText = (int)ParseJsonNumber(line);
        if (line.find(L"\"colorMotzText\"")        != std::wstring::npos) s.colorMotzText = (int)ParseJsonNumber(line);
        if (line.find(L"\"reminderDailyHour\"")   != std::wstring::npos) s.reminderDailyHour   = (int)ParseJsonNumber(line);
        if (line.find(L"\"reminderDailyMinute\"") != std::wstring::npos) s.reminderDailyMinute = (int)ParseJsonNumber(line);
        if (line.find(L"\"sidebarWidth\"")     != std::wstring::npos) s.sidebarWidth     = (int)ParseJsonNumber(line);
        if (line.find(L"\"zmanimHeight\"")     != std::wstring::npos) s.zmanimHeight     = (int)ParseJsonNumber(line);
        if (line.find(L"\"sidebarCollapsed\"") != std::wstring::npos) s.sidebarCollapsed = ParseJsonBool(line);
        if (line.find(L"\"paneSpecialTimesVisible\"") != std::wstring::npos) s.paneSpecialTimesVisible = ParseJsonBool(line);
        if (line.find(L"\"paneYearDetailsVisible\"")  != std::wstring::npos) s.paneYearDetailsVisible  = ParseJsonBool(line);
        if (line.find(L"\"paneMoladVisible\"")        != std::wstring::npos) s.paneMoladVisible        = ParseJsonBool(line);
        if (line.find(L"\"countdownTitleFontFace\"") != std::wstring::npos) s.countdownTitleFontFace = ParseJsonString(line);
        if (line.find(L"\"countdownTitleFontSize\"") != std::wstring::npos) s.countdownTitleFontSize = (int)ParseJsonNumber(line);
        if (line.find(L"\"countdownTitleTextColor\"") != std::wstring::npos) s.countdownTitleTextColor = (int)ParseJsonNumber(line);
        if (line.find(L"\"countdownTitleBackColor\"") != std::wstring::npos) s.countdownTitleBackColor = (int)ParseJsonNumber(line);
        if (line.find(L"\"countdownTitleBold\"") != std::wstring::npos) s.countdownTitleBold = ParseJsonBool(line);
        if (line.find(L"\"countdownTitleItalic\"") != std::wstring::npos) s.countdownTitleItalic = ParseJsonBool(line);
        if (line.find(L"\"countdownClockFontFace\"") != std::wstring::npos) s.countdownClockFontFace = ParseJsonString(line);
        if (line.find(L"\"countdownClockFontSize\"") != std::wstring::npos) s.countdownClockFontSize = (int)ParseJsonNumber(line);
        if (line.find(L"\"countdownClockTextColor\"") != std::wstring::npos) s.countdownClockTextColor = (int)ParseJsonNumber(line);
        if (line.find(L"\"countdownClockBackColor\"") != std::wstring::npos) s.countdownClockBackColor = (int)ParseJsonNumber(line);
        if (line.find(L"\"countdownClockBold\"") != std::wstring::npos) s.countdownClockBold = ParseJsonBool(line);
        if (line.find(L"\"countdownClockItalic\"") != std::wstring::npos) s.countdownClockItalic = ParseJsonBool(line);
        if (line.find(L"\"countdownCurrentFontFace\"") != std::wstring::npos) s.countdownCurrentFontFace = ParseJsonString(line);
        if (line.find(L"\"countdownCurrentFontSize\"") != std::wstring::npos) s.countdownCurrentFontSize = (int)ParseJsonNumber(line);
        if (line.find(L"\"countdownCurrentTextColor\"") != std::wstring::npos) s.countdownCurrentTextColor = (int)ParseJsonNumber(line);
        if (line.find(L"\"countdownCurrentBackColor\"") != std::wstring::npos) s.countdownCurrentBackColor = (int)ParseJsonNumber(line);
        if (line.find(L"\"countdownCurrentBold\"") != std::wstring::npos) s.countdownCurrentBold = ParseJsonBool(line);
        if (line.find(L"\"countdownCurrentItalic\"") != std::wstring::npos) s.countdownCurrentItalic = ParseJsonBool(line);
        if (line.find(L"\"countdownLiveFontFace\"") != std::wstring::npos) s.countdownLiveFontFace = ParseJsonString(line);
        if (line.find(L"\"countdownLiveFontSize\"") != std::wstring::npos) s.countdownLiveFontSize = (int)ParseJsonNumber(line);
        if (line.find(L"\"countdownLiveTextColor\"") != std::wstring::npos) s.countdownLiveTextColor = (int)ParseJsonNumber(line);
        if (line.find(L"\"countdownLiveBackColor\"") != std::wstring::npos) s.countdownLiveBackColor = (int)ParseJsonNumber(line);
        if (line.find(L"\"countdownLiveBold\"") != std::wstring::npos) s.countdownLiveBold = ParseJsonBool(line);
        if (line.find(L"\"countdownLiveItalic\"") != std::wstring::npos) s.countdownLiveItalic = ParseJsonBool(line);
        if (line.find(L"\"countdownZmanimMask\"") != std::wstring::npos) s.countdownZmanimMask = (uint32_t)ParseJsonNumber(line);
        if (line.find(L"\"countdownShowTitle\"") != std::wstring::npos) s.countdownShowTitle = ParseJsonBool(line);
        if (line.find(L"\"countdownShowClock\"") != std::wstring::npos) s.countdownShowClock = ParseJsonBool(line);
        if (line.find(L"\"countdownShowZmanTime\"") != std::wstring::npos) s.countdownShowZmanTime = ParseJsonBool(line);
        if (line.find(L"\"countdownShowLive\"") != std::wstring::npos) s.countdownShowLive = ParseJsonBool(line);
        if (line.find(L"\"countdownOpenOnStartup\"") != std::wstring::npos) s.countdownOpenOnStartup = ParseJsonBool(line);
        if (line.find(L"\"countdownAlwaysOnTop\"") != std::wstring::npos) s.countdownAlwaysOnTop = ParseJsonBool(line);
        if (line.find(L"\"dayDetailLandscape\"") != std::wstring::npos) s.dayDetailLandscape = ParseJsonBool(line);
        if (line.find(L"\"dayDetailShowFooter\"") != std::wstring::npos) s.dayDetailShowFooter = ParseJsonBool(line);
        if (line.find(L"\"dayDetailMarginTop\"") != std::wstring::npos) s.dayDetailMarginTop = (float)ParseJsonNumber(line);
        if (line.find(L"\"dayDetailMarginBot\"") != std::wstring::npos) s.dayDetailMarginBot = (float)ParseJsonNumber(line);
        if (line.find(L"\"dayDetailMarginLeft\"") != std::wstring::npos) s.dayDetailMarginLeft = (float)ParseJsonNumber(line);
        if (line.find(L"\"dayDetailMarginRight\"") != std::wstring::npos) s.dayDetailMarginRight = (float)ParseJsonNumber(line);
        if (line.find(L"\"yearDetailsMask\"") != std::wstring::npos) s.yearDetailsMask = (uint16_t)ParseJsonNumber(line);
        if (line.find(L"\"printEventCategoryMask\"") != std::wstring::npos) s.printEventCategoryMask = (uint8_t)ParseJsonNumber(line);
        if (line.find(L"\"printEventSeparateCategories\"") != std::wstring::npos) s.printEventSeparateCategories = ParseJsonBool(line);
        if (line.find(L"\"trayTooltipCustomZmanimMask\"") != std::wstring::npos) s.trayTooltipCustomZmanimMask = (uint32_t)ParseJsonNumber(line);
        if (line.find(L"\"disableAutoUpdate\"") != std::wstring::npos) s.disableAutoUpdate = ParseJsonBool(line);
        if (line.find(L"\"checkUpdatesAuto\"") != std::wstring::npos) s.checkUpdatesAuto = ParseJsonBool(line);
        if (line.find(L"\"updateCheckFrequency\"") != std::wstring::npos) s.updateCheckFrequency = (int)ParseJsonNumber(line);
        if (line.find(L"\"checkUpdatesOnLaunch\"") != std::wstring::npos) s.checkUpdatesOnLaunch = ParseJsonBool(line);
        if (line.find(L"\"lastUpdateCheckTime\"") != std::wstring::npos) s.lastUpdateCheckTime = (int64_t)ParseJsonNumber(line);
        if (line.find(L"\"windowX\"")         != std::wstring::npos) s.windowX         = (int)ParseJsonNumber(line);
        if (line.find(L"\"windowY\"") != std::wstring::npos) s.windowY = (int)ParseJsonNumber(line);
        if (line.find(L"\"windowW\"") != std::wstring::npos) s.windowW = (int)ParseJsonNumber(line);
        if (line.find(L"\"windowH\"") != std::wstring::npos) s.windowH = (int)ParseJsonNumber(line);
    }

    // Migrate legacy single URL to new list
    if (s.webCalendars.empty() && !s.webCalendarUrl.empty())
        s.webCalendars.push_back({ s.webCalendarUrl, true });

    // v0.8.82 — migrate old per-zman mode=0 (degrees) values into the new
    // separate degreesValue fields so existing users keep their settings.
    // Old format stored degrees in customXValue when mode==0.
    if (s.customAlotMode == 0 && s.customAlotValue > 0.0 && s.customAlotValue < 30.0)
        s.customAlotDegreesValue = s.customAlotValue;
    if (s.customMisheyakirMode == 0 && s.customMisheyakirValue > 0.0 && s.customMisheyakirValue < 30.0)
        s.customMisheyakirDegreesValue = s.customMisheyakirValue;
    if (s.customSofZmanMode == 0 && s.customSofZmanValue > 0.0 && s.customSofZmanValue < 30.0)
        s.customSofZmanDegreesValue = s.customSofZmanValue;
    if (s.customTzeitMode == 0 && s.customTzeitValue > 0.0 && s.customTzeitValue < 30.0)
        s.customTzeitDegreesValue = s.customTzeitValue;

    // Load personal events from separate file
    LoadEvents(s.userEvents);

    return true;
}

// =============================================================================
// EVENTS FILE PATH
// =============================================================================

std::wstring GetEventsFilePath()
{
    return GetWinLuachDataFilePath(L"events.json");
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
    f << L"  \"ev" << i << L"_observeAnnually\": " << (e.observeAnnually ? L"true" : L"false") << L",\n";
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
        else if (field == L"observeAnnually") events[idx].observeAnnually = ParseJsonBool(line);
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

// =============================================================================
// MASTER BACKUP / RESTORE
// Works on raw bytes so the embedded files round-trip exactly, whatever their
// encoding. Only JSON structure characters (all ASCII) are inspected.
// =============================================================================

static bool ReadFileBytes(const std::wstring& path, std::string& out)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

// Writes to a temp file first, then swaps it in, so a failed write never
// leaves a half-written data file behind.
static bool WriteFileBytesAtomic(const std::wstring& path, const std::string& data)
{
    std::wstring tmp = path + L".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f.is_open()) return false;
        f.write(data.data(), (std::streamsize)data.size());
        if (!f) { f.close(); DeleteFileW(tmp.c_str()); return false; }
    }
    if (!MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        DeleteFileW(tmp.c_str());
        return false;
    }
    return true;
}

static void StripBomAndTrim(std::string& s)
{
    if (s.size() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF)
        s.erase(0, 3);
    size_t b = s.find_first_not_of(" \t\r\n");
    size_t e = s.find_last_not_of(" \t\r\n");
    s = (b == std::string::npos) ? std::string() : s.substr(b, e - b + 1);
}

static size_t SkipJsonWs(const std::string& s, size_t i)
{
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) ++i;
    return i;
}

// s[i] must be '"'. Returns the index just past the closing quote, or npos.
static size_t SkipJsonStringBytes(const std::string& s, size_t i)
{
    for (++i; i < s.size(); ++i)
    {
        if (s[i] == '\\') ++i;
        else if (s[i] == '"') return i + 1;
    }
    return std::string::npos;
}

// Returns the index just past the JSON value starting at s[i], or npos.
static size_t SkipJsonValueBytes(const std::string& s, size_t i)
{
    if (i >= s.size()) return std::string::npos;
    if (s[i] == '"') return SkipJsonStringBytes(s, i);
    if (s[i] == '{' || s[i] == '[')
    {
        int depth = 0;
        while (i < s.size())
        {
            char c = s[i];
            if (c == '"')
            {
                i = SkipJsonStringBytes(s, i);
                if (i == std::string::npos) return i;
                continue;
            }
            if (c == '{' || c == '[') ++depth;
            else if (c == '}' || c == ']')
            {
                if (--depth == 0) return i + 1;
            }
            ++i;
        }
        return std::string::npos;
    }
    // Bare token: number, true, false, null
    size_t start = i;
    while (i < s.size() && s[i] != ',' && s[i] != '}' && s[i] != ']' &&
           s[i] != ' ' && s[i] != '\t' && s[i] != '\r' && s[i] != '\n') ++i;
    return i > start ? i : std::string::npos;
}

// Splits a top-level JSON object into (key, raw value) pairs.
static bool ParseTopLevelMembers(const std::string& s, std::vector<std::pair<std::string, std::string>>& members)
{
    members.clear();
    size_t i = SkipJsonWs(s, 0);
    if (i >= s.size() || s[i] != '{') return false;
    i = SkipJsonWs(s, i + 1);
    if (i < s.size() && s[i] == '}') return SkipJsonWs(s, i + 1) == s.size();

    while (i < s.size())
    {
        if (s[i] != '"') return false;
        size_t keyEnd = SkipJsonStringBytes(s, i);
        if (keyEnd == std::string::npos) return false;
        std::string key = s.substr(i + 1, keyEnd - i - 2);

        i = SkipJsonWs(s, keyEnd);
        if (i >= s.size() || s[i] != ':') return false;
        i = SkipJsonWs(s, i + 1);

        size_t valEnd = SkipJsonValueBytes(s, i);
        if (valEnd == std::string::npos) return false;
        members.emplace_back(key, s.substr(i, valEnd - i));

        i = SkipJsonWs(s, valEnd);
        if (i >= s.size()) return false;
        if (s[i] == '}') return SkipJsonWs(s, i + 1) == s.size();
        if (s[i] != ',') return false;
        i = SkipJsonWs(s, i + 1);
    }
    return false;
}

static const std::string* FindMember(const std::vector<std::pair<std::string, std::string>>& members, const char* key)
{
    for (const auto& m : members)
        if (m.first == key) return &m.second;
    return nullptr;
}

// Reads a data file for embedding; falls back to emptyValue if it is missing.
static std::string ReadDataFileForBackup(const std::wstring& path, const char* emptyValue)
{
    std::string data;
    if (!ReadFileBytes(path, data)) return emptyValue;
    StripBomAndTrim(data);
    return data.empty() ? std::string(emptyValue) : data;
}

bool WriteMasterBackup(const AppSettings& s, const std::wstring& path)
{
    // Make sure the files on disk reflect the current in-memory state
    // (SaveSettings also writes events.json).
    if (!SaveSettings(s)) return false;

    std::string settings  = ReadDataFileForBackup(GetSettingsFilePath(), "");
    std::string events    = ReadDataFileForBackup(GetEventsFilePath(), "{\"eventCount\": 0, \"_end\": 0}");
    std::string locations = ReadDataFileForBackup(LocationDB::GetLocationsFilePath(), "[]");
    if (settings.empty()) return false;

    SYSTEMTIME st;
    GetLocalTime(&st);
    char created[32];
    sprintf_s(created, "%04d-%02d-%02d %02d:%02d:%02d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    std::string bundle;
    bundle += "{\n";
    bundle += "  \"winluachBackup\": 1,\n";
    bundle += "  \"created\": \""; bundle += created; bundle += "\",\n";
    bundle += "  \"settings\": ";  bundle += settings;  bundle += ",\n";
    bundle += "  \"events\": ";    bundle += events;    bundle += ",\n";
    bundle += "  \"locations\": "; bundle += locations; bundle += "\n";
    bundle += "}\n";

    // Refuse to write a backup we would not be able to restore
    std::vector<std::pair<std::string, std::string>> check;
    if (!ParseTopLevelMembers(bundle, check) || !FindMember(check, "settings"))
        return false;

    return WriteFileBytesAtomic(path, bundle);
}

BackupRestoreResult RestoreBackup(const std::wstring& path)
{
    std::string data;
    if (!ReadFileBytes(path, data)) return BackupRestoreResult::Failed;
    StripBomAndTrim(data);

    std::vector<std::pair<std::string, std::string>> members;
    if (!ParseTopLevelMembers(data, members)) return BackupRestoreResult::Failed;

    if (!FindMember(members, "winluachBackup"))
    {
        // Legacy backup: a plain copy of settings.json
        if (!FindMember(members, "locationName")) return BackupRestoreResult::Failed;
        return WriteFileBytesAtomic(GetSettingsFilePath(), data + "\r\n")
            ? BackupRestoreResult::LegacySettingsOnly : BackupRestoreResult::Failed;
    }

    const std::string* settings  = FindMember(members, "settings");
    const std::string* events    = FindMember(members, "events");
    const std::string* locations = FindMember(members, "locations");
    if (!settings || settings->empty() || (*settings)[0] != '{') return BackupRestoreResult::Failed;
    if (events    && (events->empty()    || (*events)[0] != '{'))    return BackupRestoreResult::Failed;
    if (locations && (locations->empty() || (*locations)[0] != '[')) return BackupRestoreResult::Failed;

    // The loaders are line-based; the embedded text keeps its original lines.
    if (!WriteFileBytesAtomic(GetSettingsFilePath(), *settings + "\r\n")) return BackupRestoreResult::Failed;
    if (events    && !WriteFileBytesAtomic(GetEventsFilePath(), *events + "\r\n")) return BackupRestoreResult::Failed;
    if (locations && !WriteFileBytesAtomic(LocationDB::GetLocationsFilePath(), *locations + "\r\n")) return BackupRestoreResult::Failed;
    return BackupRestoreResult::Master;
}
