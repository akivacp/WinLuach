// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    Zmanim.h
// Purpose: Declares all halachic time (zmanim) calculations.
//          Given a date and location, computes sunrise, sunset,
//          and all prayer/halachic times according to multiple
//          halachic opinions (shitot).
//          No UI dependencies. Pure calculation engine.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial file. Declares Location struct, ZmanimResult struct,
//          and all zmanim calculation functions.
//          Supports GRA, Magen Avraham (72 min and 90 min variants).
// =============================================================================
//
// VERSION GUIDE:
// Bump minor (v0.x.0) when a new shita or zman is added.
// Bump patch (v0.x.x) for bugfixes to existing calculations.
//
// SHITOT (halachic opinions) SUPPORTED:
//   GRA     - Vilna Gaon / Shulchan Aruch HaRav
//             Uses fixed degree-based sunrise/sunset.
//   MA_72   - Magen Avraham, 72 minute offset (equal/fixed minutes)
//   MA_90   - Magen Avraham, 90 minute offset (equal/fixed minutes)
//   MA_72P  - Magen Avraham, 72 minute proportional (zmaniyot)
//   MA_90P  - Magen Avraham, 90 minute proportional (zmaniyot)
//
// =============================================================================

#pragma once
#include "HebrewDate.h"
#include <string>

// =============================================================================
// LOCATION
// =============================================================================

// Holds all geographic and timezone data needed for zmanim calculations.
struct Location
{
    std::wstring name;        // Display name e.g. L"New York"
    double       latitude;    // Decimal degrees, N=positive, S=negative
    double       longitude;   // Decimal degrees, E=positive, W=negative
    double       elevation;   // Meters above sea level (0 = sea level)
    int          gmtOffset;   // Hours offset from GMT e.g. -5 for EST
    bool         usesDST;     // True if location observes daylight saving time

    Location()
        : latitude(0), longitude(0), elevation(0),
        gmtOffset(0), usesDST(false) {
    }

    Location(std::wstring n, double lat, double lon,
        double elev, int gmt, bool dst)
        : name(n), latitude(lat), longitude(lon),
        elevation(elev), gmtOffset(gmt), usesDST(dst) {
    }
};

// =============================================================================
// TIME OF DAY
// =============================================================================

// Represents a time of day as hours and minutes.
// Invalid/undefined times use hour=-1 (e.g. no sunrise at the poles).
struct TimeOfDay
{
    int hour;    // 0-23
    int minute;  // 0-59
    int second;  // 0-59

    TimeOfDay() : hour(-1), minute(0), second(0) {}
    TimeOfDay(int h, int m, int s = 0) : hour(h), minute(m), second(s) {}

    // Returns true if this time is valid (not -1).
    bool IsValid() const { return hour >= 0; }
};

// =============================================================================
// SHITA (halachic opinion) ENUM
// =============================================================================

// Identifies which halachic opinion to use for a given zman.
enum Shita
{
    SHITA_GRA,      // Vilna Gaon - degree-based, no offset
    SHITA_MA_72,    // Magen Avraham - 72 fixed minutes before/after
    SHITA_MA_90,    // Magen Avraham - 90 fixed minutes before/after
    SHITA_MA_72P,   // Magen Avraham - 72 proportional minutes (zmaniyot)
    SHITA_MA_90P    // Magen Avraham - 90 proportional minutes (zmaniyot)
};

// =============================================================================
// ZMANIM RESULT
// =============================================================================

// Holds all computed halachic times for a single day at a single location.
// Times marked with a shita suffix reflect that opinion.
// Times with no suffix are universal (same for all opinions).
struct ZmanimResult
{
    // --- Universal times (same for all opinions) ---
    TimeOfDay hanetz;           // Sunrise (Hanetz HaChama)
    TimeOfDay shkia;            // Sunset (Shkiat HaChama)
    TimeOfDay chatzot;          // Halachic midday (Chatzot HaYom)
    TimeOfDay chatzotLayla;     // Halachic midnight (Chatzot HaLayla)
    TimeOfDay candleLighting;   // Candle lighting (18 min before shkia)
    TimeOfDay daylightHours;    // Total daylight hours (for display)

    // --- Alot HaShachar (dawn) ---
    TimeOfDay alot_GRA;         // 16.1 degrees before sunrise
    TimeOfDay alot_MA72;        // 72 fixed minutes before sunrise
    TimeOfDay alot_MA90;        // 90 fixed minutes before sunrise
    TimeOfDay alot_MA72P;       // 72 proportional minutes before sunrise
    TimeOfDay alot_MA90P;       // 90 proportional minutes before sunrise

    // --- Misheyakir (earliest tallit/tefillin) ---
    TimeOfDay misheyakir_11;    // 11.5 degrees before sunrise
    TimeOfDay misheyakir_10;    // 10.2 degrees before sunrise

    // --- Sof Zman Shema (latest Shema) ---
    TimeOfDay sofShema_GRA;     // End of 3rd halachic hour (GRA)
    TimeOfDay sofShema_MA72;    // End of 3rd halachic hour (MA 72)
    TimeOfDay sofShema_MA90;    // End of 3rd halachic hour (MA 90)

    // --- Sof Zman Tefilla (latest Shacharit) ---
    TimeOfDay sofTefilla_GRA;   // End of 4th halachic hour (GRA)
    TimeOfDay sofTefilla_MA72;  // End of 4th halachic hour (MA 72)
    TimeOfDay sofTefilla_MA90;  // End of 4th halachic hour (MA 90)

    // --- Mincha Gedola (earliest Mincha) ---
    TimeOfDay minchaGedola_GRA;  // 6.5 halachic hours after sunrise (GRA)
    TimeOfDay minchaGedola_MA72; // 6.5 halachic hours after sunrise (MA 72)
    TimeOfDay minchaGedola_MA90; // 6.5 halachic hours after sunrise (MA 90)

    // --- Mincha Ketana ---
    TimeOfDay minchaKetana_GRA;  // 9.5 halachic hours after sunrise (GRA)
    TimeOfDay minchaKetana_MA72; // 9.5 halachic hours after sunrise (MA 72)
    TimeOfDay minchaKetana_MA90; // 9.5 halachic hours after sunrise (MA 90)

    // --- Plag HaMincha ---
    TimeOfDay plagMincha_GRA;    // 10.75 halachic hours after sunrise (GRA)
    TimeOfDay plagMincha_MA72;   // 10.75 halachic hours (MA 72)
    TimeOfDay plagMincha_MA90;   // 10.75 halachic hours (MA 90)

    // --- Tzeit HaKochavim (nightfall) ---
    TimeOfDay tzeit_GRA;         // 8.5 degrees below horizon
    TimeOfDay tzeit_MA72;        // 72 fixed minutes after sunset
    TimeOfDay tzeit_MA90;        // 90 fixed minutes after sunset
    TimeOfDay tzeit_MA72P;       // 72 proportional minutes after sunset
    TimeOfDay tzeit_MA90P;       // 90 proportional minutes after sunset

    // --- Tzeit Shabbat (end of Shabbat) ---
    TimeOfDay tzeitShabbat;      // Same as tzeit but with optional tosefet

    // --- Shaah Zmanit (halachic hour length in minutes) ---
    double shaahZmanit_GRA;      // (shkia - hanetz) / 12
    double shaahZmanit_MA72;     // (shkia+72 - (hanetz-72)) / 12
    double shaahZmanit_MA90;     // (shkia+90 - (hanetz-90)) / 12

    ZmanimResult()
        : shaahZmanit_GRA(0), shaahZmanit_MA72(0), shaahZmanit_MA90(0) {
    }
};

// =============================================================================
// MAIN CALCULATION FUNCTION
// =============================================================================

// Computes all zmanim for a given Gregorian date and location.
// This is the primary entry point — call this to get a ZmanimResult.
// isDST should be true if daylight saving time is in effect on this date.
ZmanimResult CalculateZmanim(const GregorianDate& date,
    const Location& loc,
    bool                 isDST);

// =============================================================================
// ASTRONOMICAL PRIMITIVES
// =============================================================================

// Computes sunrise time for a date and location.
// Returns an invalid TimeOfDay if the sun does not rise (polar regions).
TimeOfDay CalculateSunrise(const GregorianDate& date, const Location& loc, bool isDST);

// Computes sunset time for a date and location.
// Returns an invalid TimeOfDay if the sun does not set (polar regions).
TimeOfDay CalculateSunset(const GregorianDate& date, const Location& loc, bool isDST);

// Computes the time when the sun is at a given angle below the horizon.
// angle > 0 means below the horizon (e.g. 8.5 for tzeit GRA).
// isMorning = true for before sunrise, false for after sunset.
TimeOfDay CalculateSunAtAngle(const GregorianDate& date, const Location& loc,
    bool isDST, double angle, bool isMorning);

// Returns the length of a halachic hour in minutes for a given shita.
// GRA: based on sunrise to sunset.
// MA:  based on alot to tzeit (with the offset for that shita).
double CalculateShaahZmanit(const TimeOfDay& dayStart,
    const TimeOfDay& dayEnd);

// =============================================================================
// OFFSET HELPERS
// =============================================================================

// Adds or subtracts a fixed number of minutes from a TimeOfDay.
// Returns an invalid TimeOfDay if the result goes out of range.
TimeOfDay AddMinutes(const TimeOfDay& t, int minutes);

// Adds a fractional number of halachic hours to a base time.
// e.g. AddShaot(hanetz, shaahZmanit, 3.0) = end of 3rd halachic hour
TimeOfDay AddShaot(const TimeOfDay& base, double shaahMinutes, double numShaot);

// =============================================================================
// STRING HELPERS
// =============================================================================

// Formats a TimeOfDay as a string.
// use24Hour=false -> "7:33 PM", use24Hour=true -> "19:33"
std::wstring FormatTime(const TimeOfDay& t, bool use24Hour);

// Returns true if daylight saving time is in effect for a location on a date.
// Uses US DST rules by default; will be expanded for other regions.
bool IsDST(const GregorianDate& date, const Location& loc);