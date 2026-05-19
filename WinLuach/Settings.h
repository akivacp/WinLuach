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
// v0.8.71 - Added anchor (int) to ReminderRule: 0=custom Tzeit, 1=Shkia,
//           2=Civil midnight. Added lastFiredDate (wstring "YYYY-MM-DD") for
//           deduplication. Added reminderDailyHour / reminderDailyMinute to
//           AppSettings for the configurable morning fire time used with
//           day/week/month offsets.
// v0.8.70 - Added afterEvent bool to ReminderRule so reminders can fire after
//           (not just before) their trigger event. Persisted as
//           advancedReminderN_afterEvent in settings.json.
// v0.1.0 - Initial file. Declares AppSettings struct and
//          SaveSettings / LoadSettings functions.
// v0.8.0 - Added zmanim bar mask + per-sub-tab preset fields.
// =============================================================================

#pragma once
#include "Location.h"
#include <cstdint>
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

    // Optional: year of first occurrence (0 = recurring every year; >0 = show from this year onward)
    int  gregYear    = 0;
    int  hebYear     = 0;

    bool         observeAnnually = true; // false = one-time event (only in the specified year)
    bool         notify = true;
    std::wstring alarmOffsets; // e.g. "15 minutes; 1 day; 1 week"
};

struct ReminderRule
{
    bool enabled = true;
    int  style      = 1;    // 0=off, 1=toast, 2=popup, 3=both
    std::wstring kind;      // Zman, Parsha, Holiday, Personal Event
    std::wstring target;    // e.g. Shkiah, Emor, Pesach
    std::wstring offsets;   // e.g. "15 minutes"
    bool afterEvent = false; // false=notify before trigger, true=notify after
    int  anchor     = 0;    // Holiday/Parsha anchor: 0=custom Tzeit, 1=Shkia, 2=Civil midnight
    std::wstring lastFiredDate; // "YYYY-MM-DD" — prevents double-firing within one day
};

// =============================================================================
// APP SETTINGS STRUCT
// Holds all user-configurable preferences.
// =============================================================================
// CHANGELOG (Settings.h):
// v0.8.83 - Added shaahZmanitShita (0=GRA, 1=MA72, 2=MA90): independent
//           setting that controls which shaah zmanit is displayed in the bar
//           and used as the time-unit for Sof Shema/Tefilla calculations.
// v0.8.82 - Added customEndFastMinuteMode (0=fixed, 1=shaah zmanit) for the
//           custom End-of-Fast value.
//         - Added per-zman separate degrees values: customAlotDegreesValue,
//           customMisheyakirDegreesValue, customSofZmanDegreesValue,
//           customTzeitDegreesValue.  customXValue fields now store the
//           minutes value; degrees have their own field.
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
    bool         trayBackEnabled = false;
    int          trayBackColor = 0x000000; // COLORREF-style RGB
    std::wstring trayFontFace = L"Arial";
    int          trayFontSize = 0; // 0 = fit icon automatically
    bool         trayFontBold = true;
    bool         trayFontItalic = false;
    int          trayNumberStyle = 0; // 0=Hebrew letters, 1=English digits
    uint32_t     trayTooltipZmanimMask = (1u << 5) | (1u << 22) | (1u << 23); // netz, shkiah, tzais GRA
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
    int  notifySefirahStyle   = 0;   // sefirah reminders
    std::wstring notifySefirahTime = L"9:00 PM";
    int  notifySefirahMode = 0;      // 0=manual time, 1=relative to zman
    int  notifySefirahOffsetMinutes = 0;
    int  notifySefirahOffsetDir = 1; // 0=before, 1=after
    int  notifySefirahBase = 1;      // 0=sunset, 1=tzais, 2=other selected zman
    int  notifySefirahOtherZman = 9; // zman bit index when base=other
    int  notifyMoadimStyle    = 0;   // moadim reminder notifications
    std::wstring notifyMoadimOffsets; // e.g. "1 day; 1 week"
    int  notifyParshaStyle    = 0;
    std::wstring notifyParshaName;    // e.g. "Emor"
    std::wstring notifyParshaOffsets; // e.g. "1 week"
    std::wstring notifyPersonalOffsets; // global offsets for personal events
    // --- WinLuach custom toast ---
    bool useWinLuachToast         = false; // false=Windows built-in, true=WinLuach custom
    int  winLuachToastDuration    = 5;     // numeric duration
    int  winLuachToastDurationUnit = 0;   // 0=minutes,1=hours,2=days,3=weeks,4=months
    std::vector<ReminderRule> advancedReminders;
    // Time of day used when a reminder offset is in days/weeks/months.
    // e.g. "1 day before Pesach" fires at this time on the prior civil day.
    int reminderDailyHour   = 9;   // 0-23
    int reminderDailyMinute = 0;   // 0-59

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
    uint64_t     printDayZmanimMask = 0xFFFFFFFFFull; // bitmask of day-detail zmanim rows
    bool         printShowFooter   = true;
    bool         printTwoColumns   = false;

    // --- Zmanim shita ---
    // zmanimShita: global shita for shema/tefilla/mincha. 0=GRA, 1=MA72, 2=MA90 (default)
    int          zmanimShita = 2;
    // shaahZmanitShita: which shaah zmanit to display and use as the
    // time-unit for all shaah-based calculations (independent of zmanimShita).
    // 0=GRA (hanetz->shkia)/12, 1=MA72, 2=MA90, 3=custom boundaries,
    // 4=16.1 degrees before hanetz to 16.1 degrees after shkia
    int          shaahZmanitShita = 0;
    int          customShaahStartMode = 1; // 0=degrees before hanetz, 1=fixed minutes before hanetz
    double       customShaahStartValue = 72.0;
    double       customShaahStartDegreesValue = 16.1;
    int          customShaahEndMode = 1;   // 0=degrees after shkia, 1=fixed minutes after shkia
    double       customShaahEndValue = 72.0;
    double       customShaahEndDegreesValue = 8.5;
    // alotShita: 0=16.1deg(GRA), 1=72min, 2=90min
    int          alotShita   = 0;
    // tzeitShita: 0=8.5deg(GRA), 1=72min, 2=90min, 3=72min-prop, 4=90min-prop
    int          tzeitShita  = 0;

    // --- Month View display ---
    bool         showChatzosOnFasts  = false; // show chatzos on public fast day cells
    bool         showBeHaB           = false; // highlight Mon-Thu-Mon in Cheshvan/Iyar (BeHaB)
    bool         showChatzosOnBeHaB  = false; // show chatzos on BeHaB cells (requires showBeHaB)

    // --- Zmanim display ---
    // customXMode: 0=degrees, 1=fixed minutes, 2=shaah zmanit minutes
    // customXValue: minutes value (used when mode=1 or 2)
    // customXDegreesValue: degrees value (used when mode=0; defaults to shita default)
    int          customAlotMode = 0;
    double       customAlotValue = 72.0;       // minutes value for mode 1/2
    double       customAlotDegreesValue = 16.1; // degrees value for mode 0 (v0.8.82)
    int          customMisheyakirMode = 0;
    double       customMisheyakirValue = 50.0;       // minutes value for mode 1/2
    double       customMisheyakirDegreesValue = 10.2; // degrees value for mode 0 (v0.8.82)
    int          customSofZmanMode = 0;      // value 0 means netz-to-shkiah
    double       customSofZmanValue = -1.0;  // -1 migrates from legacy zmanimShita (minutes for mode 1/2)
    double       customSofZmanDegreesValue = 16.1; // degrees value for mode 0 (v0.8.82)
    int          customTzeitMode = 0;
    double       customTzeitValue = 42.0;        // minutes value for mode 1/2
    double       customTzeitDegreesValue = 8.5;  // degrees value for mode 0 (v0.8.82)

    // --- Zmanim bar (which zmanim show in the bottom bar) ---
    uint32_t     zmanimBarMask = 0xFFFFFFFFu;  // bit per zman, see kZmanimBarLabels
    // --- Mincha Gedola / Ketana / Plag / End-of-Fast custom modes & values ---
    int          customMinchaGedolaPreset = 1;   // 0=30 min, 1=GRA default, 2=MA 72
    int          customMinchaKetanaPreset = 0;   // 0=GRA, 1=MA 72
    int          customPlagPreset         = 0;   // 0=GRA, 1=MA 72
    int          customEndFastPreset      = 0;   // 0=R' Tukaccinsky (27 min), 1=R' Moshe Feinstein
    double       customMinchaGedolaValue  = 30.0;  // fixed minutes after chatzos when preset=custom
    double       customMinchaKetanaValue  = 570.0; // zmanis minutes after hanetz when preset=custom
    double       customPlagValue          = 645.0; // zmanis minutes after hanetz when preset=custom
    double       customEndFastValue       = 27.0;  // minutes after shkiah when preset=custom
    int          customEndFastMinuteMode  = 0;     // v0.8.82: 0=fixed minutes, 1=shaah zmanit minutes
    // --- Per-sub-tab custom Sof Zman MA vs GRA (in addition to existing customSofZman*) ---
    int          customSofZmanMaPreset    = 0;   // 0=90 min as degrees, 1=fixed 72 min, 2=72 min as 16.1 deg, 3=custom
    int          customSofZmanGraPreset   = 0;   // 0=90 min as degrees, 1=fixed 72 min, 2=72 min as 16.1 deg, 3=custom
    int          customMisheyakirPreset   = 0;   // 0=11.5 deg, 1=11 deg, 2=10.2 deg, 3=custom
    int          customTzeitPreset        = 0;   // 0=42 min, 1=50 min, 2=60 min, 3=72 min, 4=72 min as degrees, 5=custom
    int          customAlotPreset         = 0;   // 0=GRA 16.1 deg, 1=MA72 fixed, 2=MA90 fixed, 3=custom

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
    bool         paneSpecialTimesVisible = true;
    bool         paneYearDetailsVisible  = true;
    bool         paneMoladVisible        = true;

    // --- Countdown clock ---
    std::wstring countdownTitleFontFace = L"Segoe UI";
    int          countdownTitleFontSize = 14;
    int          countdownTitleTextColor = 0x00323232;
    int          countdownTitleBackColor = 0x00FFFFFF;
    bool         countdownTitleBold = true;
    bool         countdownTitleItalic = false;
    std::wstring countdownClockFontFace = L"Segoe UI";
    int          countdownClockFontSize = 28;
    int          countdownClockTextColor = 0x000000FF;
    int          countdownClockBackColor = 0x00FFFFFF;
    bool         countdownClockBold = true;
    bool         countdownClockItalic = false;
    std::wstring countdownCurrentFontFace = L"Segoe UI";
    int          countdownCurrentFontSize = 13;
    int          countdownCurrentTextColor = 0x00323232;
    int          countdownCurrentBackColor = 0x00FFFFFF;
    bool         countdownCurrentBold = false;
    bool         countdownCurrentItalic = false;
    std::wstring countdownLiveFontFace = L"Segoe UI";
    int          countdownLiveFontSize = 12;
    int          countdownLiveTextColor = 0x00505050;
    int          countdownLiveBackColor = 0x00FFFFFF;
    bool         countdownLiveBold = false;
    bool         countdownLiveItalic = false;
    // Which of 32 zmanim appear in countdown (0-24 = built-in, 25-31 = custom shita zmanim)
    uint32_t     countdownZmanimMask  = 0x1FFFFFFu; // built-in 25 on by default; custom 7 off
    // Per-area visibility in the countdown clock window
    bool         countdownShowTitle    = true;
    bool         countdownShowClock    = true;
    bool         countdownShowZmanTime = true;  // "zman time" area (actual time of upcoming zman)
    bool         countdownShowLive     = true;  // live time + date area

    // --- Day-detail print options (persisted separately from monthly print) ---
    bool         dayDetailLandscape   = false;
    bool         dayDetailShowFooter  = true;
    float        dayDetailMarginTop   = 0.5f;
    float        dayDetailMarginBot   = 0.5f;
    float        dayDetailMarginLeft  = 0.5f;
    float        dayDetailMarginRight = 0.5f;

    // --- Year Details sidebar mask (12 facts, bit i = show fact i) ---
    uint16_t     yearDetailsMask      = 0x0FFF;  // all 12 facts shown by default

    // --- Print events settings ---
    uint8_t      printEventCategoryMask       = 0x0F;   // bits 0-3: Birthday/Anniversary/Yahrzeit/Custom
    bool         printEventSeparateCategories = false;  // if true, sort and group by category

    // --- Tray tooltip custom zmanim (user-configured shita) ---
    uint32_t     trayTooltipCustomZmanimMask  = 0x7F;   // bits 0-6: 7 custom zmanim

    // --- Auto-Update Settings ---
    bool         disableAutoUpdate        = false;  // master disable; when true, all update checks are disabled
    bool         checkUpdatesAuto         = true;   // check for updates automatically
    int          updateCheckFrequency     = 1;      // 0=daily, 1=weekly, 2=monthly
    bool         checkUpdatesOnLaunch     = true;   // check on app launch
    int64_t      lastUpdateCheckTime      = 0;      // timestamp of last update check (prevents excessive checks)

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
