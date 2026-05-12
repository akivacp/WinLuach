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
    bool         showDafYomi = true;
    bool         showYerushalmi = false;
    bool         showHalachaYomit = true;
    bool         showMishnaYomit = true;
    bool         showTanachYomi = true;
    int          haftarahShita = 0;   // 0=Ashkenazi, 1=Eidot Mizrach, 2=Italian, 3=Yemenite
    int          fontSize = 1;        // 0=Small, 1=Medium, 2=Big
    int          language = 0;        // 0=English, future use

    // --- Interface ---
    bool         showTrayIcon   = false;   // always show tray icon
    bool         minimizeToTray = false;
    int          minimizeTrayWhen = 0; // 0=minimized, 1=closed, 2=minimized or closed
    int          trayTextColor = 0x00FFFF; // COLORREF-style RGB, default yellow
    bool         minimizeOnStartup = false;
    bool         startWithWindows = false;
    bool         desktopShortcut = false;

    // --- Personal events ---
    std::vector<UserEventEntry> userEvents;

    // --- Notifications (0=off, 1=toast, 2=popup, 3=both) ---
    int  notifyPersonalEvents = 1;   // birthdays / anniversaries / yahrzeits
    int  notifyWebCalEvents   = 0;   // web calendar events

    // --- Print / location defaults ---
    bool         printWeeklyZmanim = true;
    int          candleLightingMinutes = 18;
    std::wstring webCalendarUrl;              // legacy single URL (kept for migration)
    std::vector<WebCalEntry> webCalendars;    // multi-calendar list

    // --- Print options (persisted across sessions) ---
    bool         printLandscape    = true;
    int          printRange        = 0;     // 0=month, 1=year, 2=next12
    float        printMarginTop    = 0.0f;
    float        printMarginBot    = 0.0f;
    float        printMarginLeft   = 0.0f;
    float        printMarginRight  = 0.0f;
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

    // --- Layout (splitters) ---
    int          sidebarWidth     = 190;   // persisted splitter position
    int          zmanimHeight     = 110;   // persisted zmanim panel height
    bool         sidebarCollapsed = false;

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
