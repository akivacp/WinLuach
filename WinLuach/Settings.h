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
    bool         minimizeToTray = false;
    int          minimizeTrayWhen = 0; // 0=minimized, 1=closed, 2=minimized or closed
    int          trayTextColor = 0x00FFFF; // COLORREF-style RGB, default yellow
    bool         minimizeOnStartup = false;
    bool         startWithWindows = false;
    bool         desktopShortcut = false;

    // --- Print / location defaults ---
    bool         printWeeklyZmanim = true;
    int          candleLightingMinutes = 18;
    std::wstring webCalendarUrl;  // ICS/web calendar URL, future sync source

    // --- Zmanim shita ---
    // Which opinion to use for alot/tzeit in the main display
    // 0=GRA, 1=MA72, 2=MA90
    int          zmanimShita = 0;

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
