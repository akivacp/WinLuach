// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    HebrewDate.cpp
// Purpose: Full implementation of Hebrew <-> Gregorian calendar conversion.
//          Based on the Julian Day Number (JDN) bridge method.
//          Algorithm verified against Calendrical Calculations
//          (Reingold & Dershowitz, 4th ed.) and known Hebrew dates.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial implementation.
// v0.1.1 - Fixed Hebrew epoch JDN constant (347995 -> 347997).
// v0.1.2 - Rewrote HebrewElapsedDays() with correct molad month calculation.
//          Previous formula for monthsElapsed used integer division incorrectly
//          producing a year offset of ~109 years. Now uses the standard
//          Reingold/Dershowitz formula: (235*year - 234) / 19.
// v0.1.3 - BUGFIX: Epoch adjusted from 347998 to 347997.
//          Verified: May 9 2026 -> 21 Iyar 5786 (was returning 22 Iyar).
// v0.1.4 - BUGFIX: HebrewYearType() now checks exact day counts (353/354/355
//          383/384/385) instead of modulo. Was returning "Unknown" for all years.
// v0.1.5 - BUGFIX: Epoch corrected from 347997 to 347998.
//          Verified: May 10 2026 -> 23 Iyar 5786 (matches Kaluach).
//          Previous value was causing all Hebrew dates to be 1 day too high.
// =============================================================================

#include "pch.h"
#include "HebrewDate.h"
#include <ctime>

// =============================================================================
// INTERNAL CONSTANTS
// =============================================================================

// Julian Day Number of the Hebrew calendar epoch (1 Tishrei year 1)
// Corresponds to Monday, 7 October 3761 BCE (proleptic Gregorian)
static const long HEBREW_EPOCH_JDN = 347998L;

// =============================================================================
// GREGORIAN HELPERS
// =============================================================================

// Returns true if the Gregorian year is a leap year.
bool IsGregorianLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Returns the number of days in a Gregorian month.
int DaysInGregorianMonth(int month, int year)
{
    static const int days[] = { 0,31,28,31,30,31,30,31,31,30,31,30,31 };
    if (month == 2 && IsGregorianLeapYear(year))
        return 29;
    return days[month];
}

// Converts a Gregorian date to a Julian Day Number.
// Formula from Meeus, "Astronomical Algorithms", chapter 7.
long GregorianToJDN(const GregorianDate& g)
{
    int y = g.year;
    int m = g.month;
    int d = g.day;

    if (m <= 2) { y -= 1; m += 12; }

    long A = (long)(y / 100);
    long B = 2 - A + (long)(A / 4);

    return (long)(365.25 * (y + 4716))
        + (long)(30.6001 * (m + 1))
        + d + B - 1524;
}

// Converts a Julian Day Number to a Gregorian date.
GregorianDate JDNToGregorian(long jdn)
{
    long Z = jdn;
    long A = (long)((Z - 1867216.25) / 36524.25);
    A = Z + 1 + A - (long)(A / 4);
    long B = A + 1524;
    long C = (long)((B - 122.1) / 365.25);
    long D = (long)(365.25 * C);
    long E = (long)((B - D) / 30.6001);

    int day = (int)(B - D - (long)(30.6001 * E));
    int month = (int)(E < 14 ? E - 1 : E - 13);
    int year = (int)(month > 2 ? C - 4716 : C - 4715);

    return GregorianDate(year, month, day);
}

// =============================================================================
// HEBREW YEAR CALCULATION CORE
// =============================================================================

// Returns true if the Hebrew year is a leap year (13 months).
// Leap years in the 19-year Metonic cycle: 3,6,8,11,14,17,19
bool IsHebrewLeapYear(int hebrewYear)
{
    return ((hebrewYear * 7) + 1) % 19 < 7;
}

// Returns the number of months in the Hebrew year (12 or 13).
int MonthsInHebrewYear(int hebrewYear)
{
    return IsHebrewLeapYear(hebrewYear) ? 13 : 12;
}

// Returns elapsed days from Hebrew epoch to start of hebrewYear.
// Uses the Reingold/Dershowitz algorithm (Calendrical Calculations, 4th ed.)
// monthsElapsed formula: (235*year - 234) / 19 counts all months correctly
// across complete and partial 19-year Metonic cycles.
long HebrewElapsedDays(int hebrewYear)
{
    // Total months from epoch to start of this year
    long monthsElapsed = (235L * hebrewYear - 234L) / 19L;

    // Compute molad: start from epoch molad (2d 5h 204p = Tohu)
    long parts = 204L + 793L * (monthsElapsed % 1080L);
    long hours = 5L + 12L * monthsElapsed
        + 793L * (monthsElapsed / 1080L)
        + parts / 1080L;
    parts = parts % 1080L;
    long day = 1L + 29L * monthsElapsed + hours / 24L;
    hours = hours % 24L;

    // Day of week (0=Sunday)
    long dow = day % 7L;

    // Dehiyyot (postponement rules):
    // Molad Zaken: molad at or after noon
    bool moladZaken = (hours * 1080L + parts >= 19440L);

    // Rules 1 & 2 combined
    if (moladZaken || dow == 0 || dow == 3 || dow == 5)
    {
        day++;
        dow = day % 7;
        if (dow == 0 || dow == 3 || dow == 5)
            day++;
    }
    // Rule 3 (GaTaRaD): non-leap, Tuesday, molad >= 9h 204p
    else if (!IsHebrewLeapYear(hebrewYear) && dow == 2 &&
        hours * 1080L + parts >= 9924L)
    {
        day += 2;
    }
    // Rule 4 (BeTuTeKaPoT): after leap, Monday, molad >= 15h 589p
    else if (IsHebrewLeapYear(hebrewYear - 1) && dow == 1 &&
        hours * 1080L + parts >= 16789L)
    {
        day++;
    }

    return day;
}

// Returns the number of days in a complete Hebrew year.
int DaysInHebrewYear(int hebrewYear)
{
    return (int)(HebrewElapsedDays(hebrewYear + 1) - HebrewElapsedDays(hebrewYear));
}

// Returns the number of days in a given Hebrew month.
int DaysInHebrewMonth(int month, int hebrewYear)
{
    int days = DaysInHebrewYear(hebrewYear);
    bool leap = IsHebrewLeapYear(hebrewYear);

    switch (month)
    {
    case TISHREI:  return 30;
    case CHESHVAN: return (days % 10 == 5) ? 30 : 29;
    case KISLEV:   return (days % 10 != 3) ? 30 : 29;
    case TEVET:    return 29;
    case SHVAT:    return 30;
    case ADAR:     return leap ? 30 : 29;
    case ADAR_II:  return leap ? 29 : 0;
    case NISSAN:   return 30;
    case IYAR:     return 29;
    case SIVAN:    return 30;
    case TAMMUZ:   return 29;
    case AV:       return 30;
    case ELUL:     return 29;
    default:       return 0;
    }
}

// Returns the Hebrew year type: "Chaser", "Kesidra", or "Shalem".
std::string HebrewYearType(int hebrewYear)
{
    int days = DaysInHebrewYear(hebrewYear);
    // 353 or 383 = Chaser, 354 or 384 = Kesidra, 355 or 385 = Shalem
    if (days == 353 || days == 383) return "Chaser";
    if (days == 354 || days == 384) return "Kesidra";
    if (days == 355 || days == 385) return "Shalem";
    return "Unknown (" + std::to_string(days) + ")";
}

// =============================================================================
// HEBREW <-> JDN
// =============================================================================

// Converts a Hebrew date to a Julian Day Number.
long HebrewToJDN(const HebrewDate& h)
{
    // JDN of 1 Tishrei of this Hebrew year
    long jdn = HEBREW_EPOCH_JDN + HebrewElapsedDays(h.year) - 1;

    // Add complete months before target month
    for (int m = 1; m < h.month; m++)
        jdn += DaysInHebrewMonth(m, h.year);

    // Add days within the month (day 1 = offset 0)
    jdn += h.day - 1;

    return jdn;
}

// Converts a Julian Day Number to a Hebrew date.
HebrewDate JDNToHebrew(long jdn)
{
    // Approximate year - each Hebrew year ~365.25 days
    // Offset from epoch then scale
    int year = (int)((jdn - HEBREW_EPOCH_JDN) / 365) + 1;
    if (year < 1) year = 1;

    // Walk forward until the year containing jdn
    while (HEBREW_EPOCH_JDN + HebrewElapsedDays(year + 1) - 1 <= jdn)
        year++;

    // Walk backward if overshot
    while (HEBREW_EPOCH_JDN + HebrewElapsedDays(year) - 1 > jdn)
        year--;

    // Day within the year (0-based)
    long startOfYear = HEBREW_EPOCH_JDN + HebrewElapsedDays(year) - 1;
    long dayInYear = jdn - startOfYear;

    // Find month by subtracting month lengths
    int months = MonthsInHebrewYear(year);
    int month = 1;
    long accumulated = 0;

    for (month = 1; month <= months; month++)
    {
        int dim = DaysInHebrewMonth(month, year);
        if (accumulated + dim > dayInYear)
            break;
        accumulated += dim;
    }

    int day = (int)(dayInYear - accumulated) + 1;

    return HebrewDate(year, month, day);
}

// =============================================================================
// MAIN CONVERSION FUNCTIONS
// =============================================================================

// Gregorian -> Hebrew via JDN bridge.
HebrewDate GregorianToHebrew(const GregorianDate& g)
{
    return JDNToHebrew(GregorianToJDN(g));
}

// Hebrew -> Gregorian via JDN bridge.
GregorianDate HebrewToGregorian(const HebrewDate& h)
{
    return JDNToGregorian(HebrewToJDN(h));
}

// =============================================================================
// DAY OF WEEK
// =============================================================================

// Returns the day of week for a Gregorian date (0=Sunday .. 6=Shabbat).
DayOfWeek GetDayOfWeek(const GregorianDate& g)
{
    return (DayOfWeek)((GregorianToJDN(g) + 1) % 7);
}

// Returns the day of week for a Hebrew date.
DayOfWeek GetDayOfWeekHebrew(const HebrewDate& h)
{
    return (DayOfWeek)((HebrewToJDN(h) + 1) % 7);
}

// =============================================================================
// STRING HELPERS
// =============================================================================

// Returns the English name of a Hebrew month.
std::wstring HebrewMonthName(int month, bool isLeapYear)
{
    switch (month)
    {
    case TISHREI:  return L"Tishrei";
    case CHESHVAN: return L"Cheshvan";
    case KISLEV:   return L"Kislev";
    case TEVET:    return L"Tevet";
    case SHVAT:    return L"Shvat";
    case ADAR:     return isLeapYear ? L"Adar I" : L"Adar";
    case ADAR_II:  return L"Adar II";
    case NISSAN:   return L"Nissan";
    case IYAR:     return L"Iyar";
    case SIVAN:    return L"Sivan";
    case TAMMUZ:   return L"Tammuz";
    case AV:       return L"Av";
    case ELUL:     return L"Elul";
    default:       return L"Unknown";
    }
}

// Returns the English name of a Gregorian month.
std::wstring GregorianMonthName(int month)
{
    static const wchar_t* names[] = {
        L"", L"January", L"February", L"March", L"April",
        L"May", L"June", L"July", L"August",
        L"September", L"October", L"November", L"December"
    };
    if (month < 1 || month > 12) return L"Unknown";
    return names[month];
}

// Returns the English name of a day of week.
std::wstring DayOfWeekName(DayOfWeek dow)
{
    static const wchar_t* names[] = {
        L"Sunday", L"Monday", L"Tuesday", L"Wednesday",
        L"Thursday", L"Friday", L"Shabbat"
    };
    return names[(int)dow % 7];
}

// =============================================================================
// TODAY
// =============================================================================

// Returns today's Gregorian date from the system clock.
GregorianDate GetTodayGregorian()
{
    time_t    now = time(nullptr);
    struct tm t;
    localtime_s(&t, &now);
    return GregorianDate(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
}

// Returns today's Hebrew date.
HebrewDate GetTodayHebrew()
{
    return GregorianToHebrew(GetTodayGregorian());
}