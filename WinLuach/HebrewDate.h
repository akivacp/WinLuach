// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    HebrewDate.h
// Purpose: Core Hebrew <-> Gregorian date conversion engine.
//          All calendar math lives here. No UI dependencies.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial file. Defines HebrewDate struct and declares all
//          conversion, validation, and helper functions.
// =============================================================================
//
// VERSION: 0.1.0
// This is the foundational file. Every other feature depends on this.
// A version bump is warranted when:
//   - New conversion functions are added
//   - The HebrewDate struct changes
//   - Bug fixes to calendar math
//
// =============================================================================

#pragma once
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// Hebrew month constants (1-based, Tishrei = 1)
// -----------------------------------------------------------------------------
enum HebrewMonth
{
    TISHREI = 1,
    CHESHVAN = 2,
    KISLEV = 3,
    TEVET = 4,
    SHVAT = 5,
    ADAR = 6,   // In a leap year this is Adar I
    ADAR_II = 7,   // Only exists in leap years
    NISSAN = 8,
    IYAR = 9,
    SIVAN = 10,
    TAMMUZ = 11,
    AV = 12,
    ELUL = 13
};

// -----------------------------------------------------------------------------
// HebrewDate - represents a single date in the Hebrew calendar
// -----------------------------------------------------------------------------
struct HebrewDate
{
    int year;   // Hebrew year  e.g. 5786
    int month;  // 1=Tishrei .. 13=Elul (see HebrewMonth enum)
    int day;    // 1-30

    HebrewDate() : year(0), month(0), day(0) {}
    HebrewDate(int y, int m, int d) : year(y), month(m), day(d) {}
};

// -----------------------------------------------------------------------------
// GregorianDate - represents a single date in the Gregorian calendar
// -----------------------------------------------------------------------------
struct GregorianDate
{
    int year;   // e.g. 2026
    int month;  // 1=January .. 12=December
    int day;    // 1-31

    GregorianDate() : year(0), month(0), day(0) {}
    GregorianDate(int y, int m, int d) : year(y), month(m), day(d) {}
};

// -----------------------------------------------------------------------------
// DayOfWeek - 0=Sunday .. 6=Shabbat
// -----------------------------------------------------------------------------
enum DayOfWeek
{
    SUNDAY = 0,
    MONDAY = 1,
    TUESDAY = 2,
    WEDNESDAY = 3,
    THURSDAY = 4,
    FRIDAY = 5,
    SHABBAT = 6
};

// =============================================================================
// CONVERSION FUNCTIONS
// =============================================================================

// Converts a Gregorian date to its Hebrew equivalent.
// e.g. GregorianDate(2026, 5, 9) -> HebrewDate(5786, IYAR, 21)
HebrewDate    GregorianToHebrew(const GregorianDate& g);

// Converts a Hebrew date to its Gregorian equivalent.
// e.g. HebrewDate(5786, TISHREI, 1) -> first day of Rosh Hashana 2025
GregorianDate HebrewToGregorian(const HebrewDate& h);

// Returns the Julian Day Number for a Gregorian date.
// JDN is a continuous count of days used as the conversion bridge.
long          GregorianToJDN(const GregorianDate& g);

// Converts a Julian Day Number back to a Gregorian date.
GregorianDate JDNToGregorian(long jdn);

// Returns the Julian Day Number for a Hebrew date.
long          HebrewToJDN(const HebrewDate& h);

// Converts a Julian Day Number to a Hebrew date.
HebrewDate    JDNToHebrew(long jdn);

// =============================================================================
// HEBREW YEAR HELPER FUNCTIONS
// =============================================================================

// Returns true if the given Hebrew year is a leap year (13 months).
// Leap years in the 19-year cycle: 3,6,8,11,14,17,19
bool IsHebrewLeapYear(int hebrewYear);

// Returns the number of months in the given Hebrew year (12 or 13).
int  MonthsInHebrewYear(int hebrewYear);

// Returns the number of days in a given Hebrew month.
// Accounts for Cheshvan/Kislev variation and leap year Adar.
int  DaysInHebrewMonth(int month, int hebrewYear);

// Returns the number of days in a complete Hebrew year.
// Hebrew years can be 353, 354, 355, 383, 384, or 385 days.
int  DaysInHebrewYear(int hebrewYear);

// Returns the elapsed days from creation (epoch) to start of Hebrew year.
// This is the core of the Hebrew calendar calculation.
long HebrewElapsedDays(int hebrewYear);

// Returns the type of Hebrew year:
// "Chaser" (deficient), "Kesidra" (regular), or "Shalem" (complete)
// based on whether Kislev/Cheshvan have 29 or 30 days.
std::string HebrewYearType(int hebrewYear);

// =============================================================================
// GREGORIAN YEAR HELPER FUNCTIONS
// =============================================================================

// Returns true if the given Gregorian year is a leap year.
bool IsGregorianLeapYear(int year);

// Returns the number of days in a given Gregorian month.
int  DaysInGregorianMonth(int month, int year);

// =============================================================================
// DAY OF WEEK
// =============================================================================

// Returns the day of week (0=Sunday .. 6=Shabbat) for a Gregorian date.
DayOfWeek GetDayOfWeek(const GregorianDate& g);

// Returns the day of week for a Hebrew date.
DayOfWeek GetDayOfWeekHebrew(const HebrewDate& h);

// =============================================================================
// STRING HELPERS
// =============================================================================

// Returns the English name of a Hebrew month.
// e.g. month=1 -> "Tishrei", handles leap year Adar correctly.
std::wstring HebrewMonthName(int month, bool isLeapYear);

// Returns the English name of a Gregorian month.
// e.g. month=5 -> "May"
std::wstring GregorianMonthName(int month);

// Returns the English name of a day of week.
// e.g. SUNDAY -> "Sunday"
std::wstring DayOfWeekName(DayOfWeek dow);

// Returns today's Gregorian date from the system clock.
GregorianDate GetTodayGregorian();

// Returns today's Hebrew date.
HebrewDate    GetTodayHebrew();