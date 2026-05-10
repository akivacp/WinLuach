// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    HolidayEngine.h
// Purpose: Declares all holiday detection, parasha calculation,
//          sfirat haomer, and daily learning cycle functions.
//          Given a Hebrew date, returns all relevant observances,
//          Torah readings, and daily study for that day.
//          No UI dependencies. Pure calculation engine.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial file. Declares HolidayInfo, DailyLearning structs
//          and all holiday/parasha/omer/learning functions.
//          Supports both Israel and Diaspora holiday schedules.
//          Parasha follows Ashkenazi tradition (combined/split parshiyos).
// =============================================================================
//
// VERSION GUIDE:
// Bump minor (v0.x.0) when new learning cycles or holidays are added.
// Bump patch (v0.x.x) for corrections to existing calculations.
//
// NOTES ON ISRAEL VS DIASPORA:
//   In Israel: Pesach/Sukkos/Shavuos are 1 day shorter (no Yom Tov Sheni).
//   Parasha in Israel can differ from Diaspora for several weeks after
//   Pesach and Sukkos when Israel is one parasha ahead.
//   All functions accept an isIsrael parameter to handle this.
//
// =============================================================================

#pragma once
#include "HebrewDate.h"
#include <string>
#include <vector>

// =============================================================================
// HOLIDAY TYPE FLAGS
// Multiple flags can be combined with bitwise OR.
// e.g. Rosh Hashana is: HOLIDAY_YOM_TOV | HOLIDAY_ROSH_HASHANA
// =============================================================================

enum HolidayFlags
{
    HOLIDAY_NONE = 0,
    HOLIDAY_YOM_TOV = 1 << 0,   // Full Yom Tov (melacha prohibited)
    HOLIDAY_CHOL_HAMOED = 1 << 1,   // Intermediate days
    HOLIDAY_ROSH_CHODESH = 1 << 2,   // New month
    HOLIDAY_FAST = 1 << 3,   // Fast day
    HOLIDAY_MINOR = 1 << 4,   // Minor holiday (Chanukah, Purim, etc.)
    HOLIDAY_MODERN = 1 << 5,   // Modern Israeli holidays
    HOLIDAY_SHABBOS_MEVAR = 1 << 6,   // Shabbos Mevarchim
    HOLIDAY_SPECIAL_SHAB = 1 << 7,   // Special Shabbos (Hagadol, Shuva, etc.)
    HOLIDAY_EREV = 1 << 8,   // Erev (eve of) a holiday
    HOLIDAY_ISRU_CHAG = 1 << 9,   // Day after Yom Tov
    HOLIDAY_SHMITA = 1 << 10,  // Shmita year indicator
};

// =============================================================================
// HOLIDAY INFO
// Describes a single holiday or observance for a given day.
// A day can have multiple HolidayInfo entries (e.g. Rosh Chodesh + Shabbos).
// =============================================================================

struct HolidayInfo
{
    std::wstring name;       // Display name e.g. L"Pesach"
    std::wstring subtitle;   // Extra info e.g. L"Day 3" or L"Chol Hamoed"
    int          flags;      // Combination of HolidayFlags
    bool         isIsrael;   // True if this entry is Israel-specific

    HolidayInfo()
        : flags(HOLIDAY_NONE), isIsrael(false) {
    }

    HolidayInfo(const std::wstring& n,
        const std::wstring& sub,
        int                 f,
        bool                israel = false)
        : name(n), subtitle(sub), flags(f), isIsrael(israel) {
    }
};

// =============================================================================
// SPECIAL SHABBOS NAMES
// =============================================================================

enum SpecialShabbos
{
    SPECIAL_SHABBOS_NONE = 0,
    SPECIAL_SHABBOS_SHUVA,       // Between Rosh Hashana and Yom Kippur
    SPECIAL_SHABBOS_CHAZON,      // Before Tisha B'Av
    SPECIAL_SHABBOS_NACHAMU,     // After Tisha B'Av
    SPECIAL_SHABBOS_HAGADOL,     // Before Pesach
    SPECIAL_SHABBOS_SHIRA,       // Parshas Beshalach
    SPECIAL_SHABBOS_SHEKALIM,    // Before/on Rosh Chodesh Adar
    SPECIAL_SHABBOS_ZACHOR,      // Before Purim
    SPECIAL_SHABBOS_PARAH,       // After Purim
    SPECIAL_SHABBOS_HACHODESH,   // Before/on Rosh Chodesh Nissan
    SPECIAL_SHABBOS_MEVARCHIM,   // Before Rosh Chodesh (most months)
    SPECIAL_SHABBOS_KALLAH,      // Before Shavuos
};

// =============================================================================
// PARASHA INFO
// =============================================================================

struct ParashaInfo
{
    std::wstring name;          // e.g. L"Bereshis"
    std::wstring hebrewName;    // e.g. L"בראשית"
    bool         isCombined;    // True if two parshiyos are combined
    std::wstring name2;         // Second parasha if combined
    std::wstring hebrewName2;   // Second parasha Hebrew if combined
    int          haftarahType;  // 0=Ashkenaz, 1=Sefard, etc. (for future use)

    ParashaInfo() : isCombined(false), haftarahType(0) {}
};

// =============================================================================
// OMER INFO
// =============================================================================

struct OmerInfo
{
    bool         isOmerDay;     // True if today is a day of the Omer
    int          day;           // 1-49 (0 if not Omer)
    int          week;          // Complete weeks (0-6)
    int          dayOfWeek;     // Days beyond complete weeks (0-6)
    std::wstring text;          // Full Omer text e.g. L"Day 33 of the Omer"
    std::wstring hebrewText;    // Hebrew Omer count text

    OmerInfo() : isOmerDay(false), day(0), week(0), dayOfWeek(0) {}
};

// =============================================================================
// DAILY LEARNING
// =============================================================================

struct DailyLearning
{
    std::wstring dafYomi;       // e.g. L"Bava Kama 23"
    std::wstring yerushalmi;    // e.g. L"Brachos 5"
    std::wstring mishnaYomit;   // e.g. L"Brachos 1:3"
    std::wstring halachaYomit;  // e.g. L"Orach Chaim 1:1"
    std::wstring tanachYomi;    // e.g. L"Bereishis 3"
};

// =============================================================================
// MAIN HOLIDAY FUNCTIONS
// =============================================================================

// Returns all holidays/observances for a given Hebrew date.
// isIsrael: true = Israel schedule, false = Diaspora schedule.
// Returns empty vector if no special day.
std::vector<HolidayInfo> GetHolidays(const HebrewDate& h, bool isIsrael);

// Returns the Shabbos parasha for the week containing this date.
// If the date is not Shabbos, returns the upcoming Shabbos parasha.
// isIsrael: affects combined/split parshiyos after certain holidays.
ParashaInfo GetParasha(const HebrewDate& h, bool isIsrael);

// Returns Omer information for a given Hebrew date.
// Returns isOmerDay=false outside of Sfirat HaOmer period.
OmerInfo GetOmer(const HebrewDate& h);

// Returns all daily learning cycle entries for a given date.
DailyLearning GetDailyLearning(const HebrewDate& h,
    const GregorianDate& g);

// =============================================================================
// INDIVIDUAL HOLIDAY DETECTION FUNCTIONS
// =============================================================================

// Returns true if the date is Rosh Hashana (1 or 2 Tishrei).
bool IsRoshHashana(const HebrewDate& h);

// Returns true if the date is Yom Kippur (10 Tishrei).
bool IsYomKippur(const HebrewDate& h);

// Returns true if the date falls within Sukkos.
bool IsSukkos(const HebrewDate& h, bool isIsrael);

// Returns true if the date is Shemini Atzeres or Simchas Torah.
bool IsSheminiAtzeresSimchasTorah(const HebrewDate& h, bool isIsrael);

// Returns true if the date falls within Chanukah.
bool IsChanukah(const HebrewDate& h);

// Returns the day of Chanukah (1-8), or 0 if not Chanukah.
int ChanukahDay(const HebrewDate& h);

// Returns true if the date is Purim (or Shushan Purim).
bool IsPurim(const HebrewDate& h);

// Returns true if the date falls within Pesach.
bool IsPesach(const HebrewDate& h, bool isIsrael);

// Returns true if the date is Shavuos.
bool IsShavuos(const HebrewDate& h, bool isIsrael);

// Returns true if the date is Tisha B'Av.
bool IsTishaBAv(const HebrewDate& h);

// Returns true if the date is any fast day.
bool IsFastDay(const HebrewDate& h);

// Returns true if the date is Rosh Chodesh.
bool IsRoshChodesh(const HebrewDate& h);

// Returns true if this Shabbos is Shabbos Mevarchim.
// (The Shabbos before Rosh Chodesh, except before Tishrei.)
bool IsShabboseMevarchim(const HebrewDate& h);

// Returns the special Shabbos type for this date, or SPECIAL_SHABBOS_NONE.
SpecialShabbos GetSpecialShabbos(const HebrewDate& h);

// =============================================================================
// YEAR INFORMATION FUNCTIONS
// =============================================================================

// Returns true if the Hebrew year is a Shmita year.
// Shmita occurs every 7 years; year 5782 was Shmita (cycle anchor).
bool IsShmitaYear(int hebrewYear);

// Returns the year within the current Shmita cycle (1-7).
int ShmitaCycleYear(int hebrewYear);

// Returns true if this year uses Maaser Sheni (years 1,2,4,5 of Shmita cycle).
bool IsMaaserSheni(int hebrewYear);

// Returns the position in the 19-year lunar cycle (1-19).
int LunarCycleYear(int hebrewYear);

// Returns the position in the 28-year solar cycle (1-28).
int SolarCycleYear(int hebrewYear);

// Returns a string describing years since significant events.
// e.g. L"5786 years from creation"
std::vector<std::wstring> GetYearFacts(int hebrewYear);

// =============================================================================
// PARASHA LIST
// =============================================================================

// Returns the full list of parasha names in order (54 parshiyos).
// Used by GetParasha() internally and by UI for display.
const std::vector<std::wstring>& GetParashaNames();