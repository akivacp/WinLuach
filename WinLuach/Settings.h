// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    Settings.h
// Purpose: Declares the application settings manager.
//          Saves and loads user preferences to/from
//          %APPDATA%\WinLuach\settings.json.
//          Settings include: last location, clock format,
//          Israel/Diaspora mode, window size/position.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial file. Declares AppSettings struct and
//          SaveSettings / LoadSettings functions.
// =============================================================================

#pragma once
#include "Location.h"
#include <string>
#include <vector>

struct WebCalEntry {
    std::wstring url;
    bool         enabled = true;
};

// ── Personal recurring event (birthday / anniversary / yahrzeit / custom) ────
struct UserEventEntry
{
    std::wstring name;
    int  type        = 0;   // 0=Birthday, 1=Anniversary, 2=Yahrzeit, 3=Custom

    // Gregorian recurrence (0 = disabled)
    int  gregMonth   = 0;
    int  gregDay     = 1;

    // Hebrew recurrence (0 = disabled; use NISAN/IYAR/... constants)
    int  hebMonth    = 0;
    int  hebDay      = 1;

    // For yahrzeit: display on the evening BEFORE the Hebrew date
    bool afterSunset = false;

    // Optional: lock to a specific year (0 = recurring every year)
    int  gregYear    = 0;
    int  hebYear     = 0;

    bool         notify = true;
    std::wstring alarmOffsets; // e.g. "15 minutes; 1 day; 1 week"
};

struct ReminderRule
{
    bool enabled = true;
    int style = 1; // 0=off, 1=toast, 2=popup, 3=both
    std::wstring kind;    // Zman, Parsha, Holiday, Personal Event
    std::wstring target;  // e.g. Shkiah, Emor, Pesach
    std::wstring offsets; // e.g. "15 minutes; 1 day"
};

// =============================================================================
// APP SETTINGS STRUCT
// Holds all user-configurable preferences.
// =============================================================================

struct AppSettings
{
    // --- Location ---
    std::wstring locationName;   // e.g. L"New York City"
    double       latitude = 40.6501;
    double       longitude = -73.9496;
    double       elevation = 10.0;
    int          gmtOffset = -5;
    bool         usesDST = true;

    // --- Display ---
    bool         use24Hour = false;   // true = 24hr, false = AM/PM
    bool         isIsrael = false;   // true = Israel schedule
    bool         defaultHebrewMonth = false;
    bool         dateTracking = true;
    bool         showParshios = true;
    bool         showMoadim = true;
    bool         showUserEvents = true;
    bool         showDafYomi = false;
    bool         showYerushalmi = false;
    bool         showHalachaYomit = false;
    bool         showMishnaYomit = false;
    bool         showTanachYomi = false;
    int          haftarahShita = 0;   // 0=Ashkenazi, 1=Eidot Mizrach, 2=Italian, 3=Yemenite
    int          fontSize = 1;        // 0=Small, 1=Medium, 2=Big
    int          language = 0;        // 0=English, future use

    // --- Interface ---
    bool         showTrayIcon   = false;   // always show tray icon
    bool         minimizeToTray = false;
    int          minimizeTrayWhen = 0; // 0=minimized, 1=closed, 2=minimized or closed
    int          trayTextColor = 0x00FFFF; // COLORREF-style RGB, default yellow
    uint32_t     trayTooltipZmanimMask = (1u << 2) | (1u << 9) | (1u << 10); // netz, shkiah, tzais
    bool         minimizeOnStartup = false;
    bool         startWithWindows = false;
    bool         desktopShortcut = false;

    // --- Personal events ---
    std::vector<UserEventEntry> userEvents;

    // --- Notifications (0=off, 1=toast, 2=popup, 3=both) ---
    int  notifyPersonalEvents = 1;   // birthdays / anniversaries / yahrzeits
    int  notifyWebCalEvents   = 0;   // web calendar events
    int  notifyZmanimStyle    = 0;   // zmanim notifications
    uint32_t notifyZmanimMask = 0;   // selectable zmanim checkboxes
    int  notifyMoadimStyle    = 0;   // moadim reminder notifications
    std::wstring notifyMoadimOffsets; // e.g. "1 day; 1 week"
    int  notifyParshaStyle    = 0;
    std::wstring notifyParshaName;    // e.g. "Emor"
    std::wstring notifyParshaOffsets; // e.g. "1 week"
    std::wstring notifyPersonalOffsets; // global offsets for personal events
    std::vector<ReminderRule> advancedReminders;

    // --- Print / location defaults ---
    bool         printWeeklyZmanim = true;
    int          candleLightingMinutes = 18;
    std::wstring webCalendarUrl;              // legacy single URL (kept for migration)
    std::vector<WebCalEntry> webCalendars;    // multi-calendar list

    // --- Print options (persisted across sessions) ---
    bool         printLandscape    = true;
    int          printRange        = 0;     // 0=month, 1=year, 2=next12
    float        printMarginTop    = 0.5f;
    float        printMarginBot    = 0.5f;
    float        printMarginLeft   = 0.5f;
    float        printMarginRight  = 0.5f;
    uint32_t     printZmanimColMask = 0x7FFF; // bitmask of 15 zmanim columns to print
    bool         printShowFooter   = true;

    // --- Zmanim shita ---
    // zmanimShita: global shita for shema/tefilla/mincha. 0=GRA, 1=MA72, 2=MA90
    int          zmanimShita = 0;
    // alotShita: 0=16.1deg(GRA), 1=72min, 2=90min
    int          alotShita   = 0;
    // tzeitShita: 0=8.5deg(GRA), 1=72min, 2=90min, 3=72min-prop, 4=90min-prop
    int          tzeitShita  = 0;

    // --- Zmanim display ---
    bool         showChatzosOnFasts = false; // show chatzos on public fast day cells
    int          customAlotMode = 0;         // 0=degrees, 1=fixed minutes, 2=shaah zmanit minutes
    double       customAlotValue = 16.1;
    int          customMisheyakirMode = 0;   // 0=degrees, 1=fixed minutes, 2=shaah zmanit minutes
    double       customMisheyakirValue = 10.2;
    int          customSofZmanMode = 0;      // 0=degrees, 1=fixed minutes, 2=shaah zmanit minutes; value 0 means netz-to-shkiah
    double       customSofZmanValue = -1.0;  // -1 migrates from legacy zmanimShita
    int          customTzeitMode = 0;        // 0=degrees, 1=fixed minutes, 2=shaah zmanit minutes
    double       customTzeitValue = 8.5;

    // --- Calendar colors ---
    int          colorNormalCell       = 0x00FFFFFF; // RGB(255,255,255)
    int          colorOtherMonthCell   = 0x00F0F0F0; // RGB(240,240,240)
    int          colorTodayCell        = 0x0096FFFF; // RGB(255,255,150)
    int          colorShabbosCell      = 0x00B4E6B4; // RGB(180,230,180)
    int          colorYomTovCell       = 0x0082C8FF; // RGB(255,200,130)
    int          colorRoshChodeshCell  = 0x00FFD2B4; // RGB(180,210,255)
    int          colorCholHamoedCell   = 0x00B4E6FF; // RGB(255,230,180)
    int          colorFastDayCell      = 0x00DCC8DC; // RGB(220,200,220)
    int          colorGregorianText    = 0x00323232; // RGB(50,50,50)
    int          colorHebrewText       = 0x00B41E1E; // RGB(30,30,180)
    int          colorHolidayText      = 0x001E1EB4; // RGB(180,30,30)
    int          colorParshaText       = 0x00A0501E; // RGB(30,80,160)
    int          colorCivilEventText   = 0x00781E78; // RGB(120,30,120)
    int          colorHebrewEventText  = 0x00506E00; // RGB(0,110,80)
    int          colorOmerText         = 0x00965064; // RGB(100,80,150)
    int          colorLearningText     = 0x00B45A00; // RGB(0,90,180)
    int          colorCandleText       = 0x000050B4; // RGB(180,80,0)
    int          colorMotzText         = 0x00A05000; // RGB(0,80,160)

    // --- Layout (splitters) ---
    int          sidebarWidth     = 190;   // persisted splitter position
    int          zmanimHeight     = 110;   // persisted zmanim panel height
    bool         sidebarCollapsed = false;

    // --- Countdown clock ---
    std::wstring countdownTitleFontFace = L"Segoe UI";
    int          countdownTitleFontSize = 14;
    int          countdownTitleTextColor = 0x00323232;
    int          countdownTitleBackColor = 0x00FFFFFF;
    std::wstring countdownClockFontFace = L"Segoe UI";
    int          countdownClockFontSize = 28;
    int          countdownClockTextColor = 0x000000FF;
    int          countdownClockBackColor = 0x00FFFFFF;
    std::wstring countdownCurrentFontFace = L"Segoe UI";
    int          countdownCurrentFontSize = 13;
    int          countdownCurrentTextColor = 0x00323232;
    int          countdownCurrentBackColor = 0x00FFFFFF;
    std::wstring countdownLiveFontFace = L"Segoe UI";
    int          countdownLiveFontSize = 12;
    int          countdownLiveTextColor = 0x00505050;
    int          countdownLiveBackColor = 0x00FFFFFF;

    // --- Window ---
    int          windowX = -1;   // -1 = use default
    int          windowY = -1;
    int          windowW = 1100;
    int          windowH = 700;

    // Returns the Location struct built from this settings object.
    Location ToLocation() const
    {
        return Location(locationName, latitude, longitude,
            elevation, gmtOffset, usesDST);
    }
};

// =============================================================================
// FUNCTIONS
// =============================================================================

// Returns the path to the settings file: %APPDATA%\WinLuach\settings.json
std::wstring GetSettingsFilePath();

// Saves settings to disk. Returns true on success.
bool SaveSettings(const AppSettings& s);

// Loads settings from disk. Returns true on success.
// If the file doesn't exist, fills s with defaults and returns false.
bool LoadSettings(AppSettings& s);

// Returns the path to the events file: %APPDATA%\WinLuach\events.json
std::wstring GetEventsFilePath();

// Saves user events to events.json. Returns true on success.
bool SaveEvents(const std::vector<UserEventEntry>& events);

// Loads user events from events.json. Returns true on success.
bool LoadEvents(std::vector<UserEventEntry>& events);

// Exports user events to an arbitrary path chosen by the caller.
bool ExportEvents(const std::vector<UserEventEntry>& events, const std::wstring& path);

// Imports user events from a file. Appends to existing list. Returns count added.
int  ImportEvents(std::vector<UserEventEntry>& events, const std::wstring& path);

// Parses user events from a file without appending to any list. Returns parsed events.
std::vector<UserEventEntry> ParseEventsFromFile(const std::wstring& path);
