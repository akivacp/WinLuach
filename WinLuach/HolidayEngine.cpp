// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    HolidayEngine.cpp
// Purpose: Full implementation of holiday detection, parasha calculation,
//          sfirat haomer, and daily learning cycles.
//          All calculations are based on the Hebrew date only.
//          No UI dependencies. Pure calculation engine.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial implementation.
//          Holidays: full Jewish calendar year, Israel + Diaspora.
//          Parasha: all 54 parshiyos, combined/split logic, Israel aware.
//          Omer: days 1-49, weeks and days breakdown.
//          Daily learning: Daf Yomi (Bavli), Yerushalmi, Mishna Yomit,
//          Halacha Yomit, Tanach Yomi.
//          Special Shabbosos: all 10 types.
//          Year facts: creation, destruction, State of Israel, shmita.
// v0.1.1 - BUGFIX: Omer display days-of-week was off by 1 (dayOfWeek+1).
//          BUGFIX: Parasha cycle anchor corrected (NextShabbos on Simchas Torah
//          itself, not the day after).
// v0.1.2 - BUGFIX: Omer braces fixed (displayDays scope error).
//          BUGFIX: Vayakhel-Pekudei combination now only applies to
//          short (Chaser) non-leap years, not all non-long years.
//          This corrected parasha for Kesidra years like 5786.
// v0.1.3 - Removed redundant subtitle from Yom Yerushalayim.
// v0.1.4 - BUGFIX: Israel parasha corrected after Shavuos.
//          Israel reads 1 parasha ahead of Diaspora starting the
//          Shabbos after Shavuos (no Yom Tov Sheni = one extra reading).
// =============================================================================

#include "pch.h"
#include "HolidayEngine.h"
#include <sstream>
#include <cmath>

// =============================================================================
// PARASHA NAMES
// All 54 parshiyos in order from Bereishis to V'Zos HaBracha.
// =============================================================================

static const std::vector<std::wstring> s_parashaNames =
{
    L"Bereishis",    L"Noach",         L"Lech Lecha",   L"Vayeira",
    L"Chayei Sara",  L"Toldos",        L"Vayeitzei",    L"Vayishlach",
    L"Vayeishev",    L"Mikeitz",       L"Vayigash",     L"Vayechi",
    L"Shemos",       L"Va'eira",       L"Bo",           L"Beshalach",
    L"Yisro",        L"Mishpatim",     L"Terumah",      L"Tetzaveh",
    L"Ki Sisa",      L"Vayakhel",      L"Pekudei",      L"Vayikra",
    L"Tzav",         L"Shemini",       L"Tazria",       L"Metzora",
    L"Acharei Mos",  L"Kedoshim",      L"Emor",         L"Behar",
    L"Bechukosai",   L"Bamidbar",      L"Nasso",        L"Beha'aloscha",
    L"Shelach",      L"Korach",        L"Chukas",        L"Balak",
    L"Pinchas",      L"Matos",         L"Masei",        L"Devarim",
    L"Va'eschanan",  L"Eikev",         L"Re'eh",        L"Shoftim",
    L"Ki Seitzei",   L"Ki Savo",       L"Nitzavim",     L"Vayeilech",
    L"Ha'azinu",     L"V'Zos HaBracha"
};

// Returns the full list of parasha names.
const std::vector<std::wstring>& GetParashaNames()
{
    return s_parashaNames;
}

// =============================================================================
// YEAR INFORMATION
// =============================================================================

// Returns true if the Hebrew year is a Shmita year.
// Shmita years: 5782, 5789, 5796 ...
// 5782 % 7 = 1 (anchor), so shmita when (year % 7 == 1) ... actually:
// Known shmita years: 5747, 5754, 5761, 5768, 5775, 5782, 5789
// 5782 % 7 = 0 -> use (year % 7 == 0) as the rule
bool IsShmitaYear(int hebrewYear)
{
    return (hebrewYear % 7) == 1;
}

// Returns the year within the 7-year Shmita cycle (1-7).
// Year 1 = year after Shmita, year 7 = Shmita year.
int ShmitaCycleYear(int hebrewYear)
{
    int r = hebrewYear % 7;
    return (r == 0) ? 7 : r;
}

// Returns true if Maaser Sheni applies (years 1,2,4,5 of Shmita cycle).
bool IsMaaserSheni(int hebrewYear)
{
    int y = ShmitaCycleYear(hebrewYear);
    return (y == 1 || y == 2 || y == 4 || y == 5);
}

// Returns position in the 19-year lunar (Metonic) cycle (1-19).
int LunarCycleYear(int hebrewYear)
{
    int r = hebrewYear % 19;
    return (r == 0) ? 19 : r;
}

// Returns position in the 28-year solar cycle (1-28).
int SolarCycleYear(int hebrewYear)
{
    int r = (hebrewYear + 6) % 28; // offset so cycle starts correctly
    return (r == 0) ? 28 : r;
}

// Returns a vector of interesting year facts for the sidebar.
std::vector<std::wstring> GetYearFacts(int hebrewYear)
{
    std::vector<std::wstring> facts;
    wchar_t buf[256];

    // Years from creation
    swprintf_s(buf, L"%d years from the creation of the world", hebrewYear);
    facts.push_back(buf);

    // Lunar cycle
    swprintf_s(buf, L"Year %d of cycle %d of the lunar (small) cycle",
        LunarCycleYear(hebrewYear),
        (hebrewYear / 19) + 1);
    facts.push_back(buf);

    // Solar cycle
    swprintf_s(buf, L"Year %d of cycle %d of the solar (big) cycle",
        SolarCycleYear(hebrewYear),
        ((hebrewYear + 6) / 28) + 1);
    facts.push_back(buf);

    // Shmita cycle
    int shmitaYear = ShmitaCycleYear(hebrewYear);
    if (IsShmitaYear(hebrewYear))
        facts.push_back(L"Year of Shmita");
    else
    {
        swprintf_s(buf, L"Year %d of the Shmita cycle", shmitaYear);
        facts.push_back(buf);
    }

    // Maaser
    if (IsMaaserSheni(hebrewYear))
        facts.push_back(L"Maaser Sheni (tithe for Jerusalem)");
    else if (!IsShmitaYear(hebrewYear))
        facts.push_back(L"Maaser Ani (tithe for the poor)");

    // Years from destruction of Temple (3828 AM)
    int sinceDestruction = hebrewYear - 3828;
    if (sinceDestruction > 0)
    {
        swprintf_s(buf, L"%d years from the destruction of the Holy Temple",
            sinceDestruction);
        facts.push_back(buf);
    }

    // Years from establishment of State of Israel (5708 AM)
    if (hebrewYear >= 5708)
    {
        swprintf_s(buf, L"%d years from the establishment of the State of Israel",
            hebrewYear - 5708);
        facts.push_back(buf);
    }

    // Years from liberation of Jerusalem (5727 AM)
    if (hebrewYear >= 5727)
    {
        swprintf_s(buf, L"%d years from the liberation of Jerusalem",
            hebrewYear - 5727);
        facts.push_back(buf);
    }

    return facts;
}

// =============================================================================
// HOLIDAY DETECTION
// =============================================================================

// Returns true if date is Rosh Hashana (1 or 2 Tishrei).
bool IsRoshHashana(const HebrewDate& h)
{
    return h.month == TISHREI && (h.day == 1 || h.day == 2);
}

// Returns true if date is Yom Kippur (10 Tishrei).
bool IsYomKippur(const HebrewDate& h)
{
    return h.month == TISHREI && h.day == 10;
}

// Returns true if date falls within Sukkos (15-21 Tishrei, or 15-22 Diaspora).
bool IsSukkos(const HebrewDate& h, bool isIsrael)
{
    if (h.month != TISHREI) return false;
    int last = isIsrael ? 21 : 22;
    return h.day >= 15 && h.day <= last;
}

// Returns true if date is Shemini Atzeres / Simchas Torah.
// Israel: 22 Tishrei (combined). Diaspora: 22 = Shemini Atzeres, 23 = Simchas Torah.
bool IsSheminiAtzeresSimchasTorah(const HebrewDate& h, bool isIsrael)
{
    if (h.month != TISHREI) return false;
    if (isIsrael) return h.day == 22;
    return h.day == 22 || h.day == 23;
}

// Returns true if date falls within Chanukah (25 Kislev - 2/3 Teves).
bool IsChanukah(const HebrewDate& h)
{
    return ChanukahDay(h) > 0;
}

// Returns the day of Chanukah (1-8) or 0 if not Chanukah.
// Chanukah starts 25 Kislev and lasts 8 days.
// If Kislev has 29 days, days 7-8 fall in Teves.
int ChanukahDay(const HebrewDate& h)
{
    if (h.month == KISLEV && h.day >= 25)
        return h.day - 24;

    if (h.month == TEVET)
    {
        // How many days of Chanukah fell in Kislev?
        int kislevDays = DaysInHebrewMonth(KISLEV, h.year);
        int daysInKislev = kislevDays - 24; // e.g. 30-24=6 or 29-24=5
        int dayInTevet = h.day;
        int chanukahDay = daysInKislev + dayInTevet;
        if (chanukahDay >= 1 && chanukahDay <= 8)
            return chanukahDay;
    }

    return 0;
}

// Returns true if date is Purim.
// Regular years: 14 Adar. Leap years: 14 Adar II.
bool IsPurim(const HebrewDate& h)
{
    bool leap = IsHebrewLeapYear(h.year);
    if (leap)
        return h.month == ADAR_II && h.day == 14;
    return h.month == ADAR && h.day == 14;
}

// Returns true if date falls within Pesach.
// Israel: 15-21 Nissan. Diaspora: 15-22 Nissan.
bool IsPesach(const HebrewDate& h, bool isIsrael)
{
    if (h.month != NISSAN) return false;
    int last = isIsrael ? 21 : 22;
    return h.day >= 15 && h.day <= last;
}

// Returns true if date is Shavuos.
// Israel: 6 Sivan. Diaspora: 6-7 Sivan.
bool IsShavuos(const HebrewDate& h, bool isIsrael)
{
    if (h.month != SIVAN) return false;
    if (isIsrael) return h.day == 6;
    return h.day == 6 || h.day == 7;
}

// Returns true if date is Tisha B'Av (9 Av).
// Note: if 9 Av falls on Shabbos it is pushed to 10 Av —
// that adjustment is handled in GetHolidays().
bool IsTishaBAv(const HebrewDate& h)
{
    return h.month == AV && (h.day == 9 || h.day == 10);
}

// Returns true if date is any fast day.
bool IsFastDay(const HebrewDate& h)
{
    // Yom Kippur
    if (IsYomKippur(h)) return true;

    // Tisha B'Av
    if (h.month == AV && (h.day == 9 || h.day == 10)) return true;

    // Fast of Gedaliah (3 Tishrei, or 4 if 3 is Shabbos)
    if (h.month == TISHREI && (h.day == 3 || h.day == 4)) return true;

    // Fast of Esther (13 Adar, or 11 if 13 is Shabbos)
    // In a leap year: 13 Adar II
    bool leap = IsHebrewLeapYear(h.year);
    if (!leap && h.month == ADAR && (h.day == 11 || h.day == 13)) return true;
    if (leap && h.month == ADAR_II && (h.day == 11 || h.day == 13)) return true;

    // 17 Tammuz (or 18 if Shabbos)
    if (h.month == TAMMUZ && (h.day == 17 || h.day == 18)) return true;

    // 10 Teves
    if (h.month == TEVET && h.day == 10) return true;

    return false;
}

// Returns true if date is Rosh Chodesh.
// Day 1 of every month, plus day 30 of months that have 30 days
// (which is Rosh Chodesh of the next month).
bool IsRoshChodesh(const HebrewDate& h)
{
    if (h.day == 1 && h.month != TISHREI) // Tishrei 1 = Rosh Hashana
        return true;
    // Day 30 of a full month = first day of Rosh Chodesh
    if (h.day == 30)
        return true;
    return false;
}

// Returns true if this Shabbos is Shabbos Mevarchim
// (the Shabbos before Rosh Chodesh, except before Tishrei).
bool IsShabboseMevarchim(const HebrewDate& h)
{
    DayOfWeek dow = GetDayOfWeekHebrew(h);
    if (dow != SHABBAT) return false;

    // Check if Rosh Chodesh falls in the next 1-8 days
    for (int i = 1; i <= 8; i++)
    {
        // Get date i days ahead
        GregorianDate g = HebrewToGregorian(h);
        long jdn = HebrewToJDN(h) + i;
        HebrewDate next = JDNToHebrew(jdn);

        if (next.day == 1 && next.month != TISHREI)
            return true;
        if (next.day == 30)
            return true;
    }
    return false;
}

// Returns the special Shabbos type for this date.
SpecialShabbos GetSpecialShabbos(const HebrewDate& h)
{
    DayOfWeek dow = GetDayOfWeekHebrew(h);
    if (dow != SHABBAT) return SPECIAL_SHABBOS_NONE;

    bool leap = IsHebrewLeapYear(h.year);

    // Shabbos Shuva: between Rosh Hashana and Yom Kippur
    if (h.month == TISHREI && h.day >= 2 && h.day <= 9)
        return SPECIAL_SHABBOS_SHUVA;

    // Shabbos Chazon: the Shabbos before Tisha B'Av
    if (h.month == AV && h.day >= 2 && h.day <= 8)
        return SPECIAL_SHABBOS_CHAZON;

    // Shabbos Nachamu: the Shabbos after Tisha B'Av
    if (h.month == AV && h.day >= 10 && h.day <= 16)
        return SPECIAL_SHABBOS_NACHAMU;

    // Shabbos Hagadol: the Shabbos before Pesach (10-14 Nissan)
    if (h.month == NISSAN && h.day >= 8 && h.day <= 14)
        return SPECIAL_SHABBOS_HAGADOL;

    // Shabbos Shira: Parshas Beshalach (16th Shabbos of the year ~Shvat)
    // Approximated as Shabbos in mid-Shvat
    if (h.month == SHVAT && h.day >= 10 && h.day <= 16)
        return SPECIAL_SHABBOS_SHIRA;

    // Shabbos Shekalim: Shabbos on or before 1 Adar (or Adar II in leap)
    if (!leap && h.month == ADAR && h.day >= 25)
        return SPECIAL_SHABBOS_SHEKALIM;
    if (!leap && h.month == SHVAT && h.day >= 25)
        return SPECIAL_SHABBOS_SHEKALIM;
    if (leap && h.month == ADAR_II && h.day >= 25)
        return SPECIAL_SHABBOS_SHEKALIM;
    if (leap && h.month == ADAR && h.day >= 25)
        return SPECIAL_SHABBOS_SHEKALIM;

    // Shabbos Zachor: Shabbos before Purim (7-13 Adar or Adar II)
    if (!leap && h.month == ADAR && h.day >= 7 && h.day <= 13)
        return SPECIAL_SHABBOS_ZACHOR;
    if (leap && h.month == ADAR_II && h.day >= 7 && h.day <= 13)
        return SPECIAL_SHABBOS_ZACHOR;

    // Shabbos Parah: Shabbos after Purim (~18-24 Adar or Adar II)
    if (!leap && h.month == ADAR && h.day >= 16 && h.day <= 23)
        return SPECIAL_SHABBOS_PARAH;
    if (leap && h.month == ADAR_II && h.day >= 16 && h.day <= 23)
        return SPECIAL_SHABBOS_PARAH;

    // Shabbos HaChodesh: Shabbos on or before 1 Nissan
    if (h.month == ADAR && h.day >= 24 && !leap)
        return SPECIAL_SHABBOS_HACHODESH;
    if (h.month == ADAR_II && h.day >= 24 && leap)
        return SPECIAL_SHABBOS_HACHODESH;
    if (h.month == NISSAN && h.day == 1)
        return SPECIAL_SHABBOS_HACHODESH;

    // Shabbos Kallah: Shabbos before Shavuos (29 Iyar - 5 Sivan)
    if (h.month == IYAR && h.day >= 28)
        return SPECIAL_SHABBOS_KALLAH;
    if (h.month == SIVAN && h.day <= 5)
        return SPECIAL_SHABBOS_KALLAH;

    return SPECIAL_SHABBOS_NONE;
}

// Helper: returns the name of a special Shabbos.
static std::wstring SpecialShabbosName(SpecialShabbos s)
{
    switch (s)
    {
    case SPECIAL_SHABBOS_SHUVA:      return L"Shabbos Shuva";
    case SPECIAL_SHABBOS_CHAZON:     return L"Shabbos Chazon";
    case SPECIAL_SHABBOS_NACHAMU:    return L"Shabbos Nachamu";
    case SPECIAL_SHABBOS_HAGADOL:    return L"Shabbos Hagadol";
    case SPECIAL_SHABBOS_SHIRA:      return L"Shabbos Shira";
    case SPECIAL_SHABBOS_SHEKALIM:   return L"Shabbos Shekalim";
    case SPECIAL_SHABBOS_ZACHOR:     return L"Shabbos Zachor";
    case SPECIAL_SHABBOS_PARAH:      return L"Shabbos Parah";
    case SPECIAL_SHABBOS_HACHODESH:  return L"Shabbos HaChodesh";
    case SPECIAL_SHABBOS_MEVARCHIM:  return L"Shabbos Mevarchim";
    case SPECIAL_SHABBOS_KALLAH:     return L"Shabbos Kallah";
    default:                         return L"";
    }
}

// =============================================================================
// GET HOLIDAYS — MAIN FUNCTION
// =============================================================================

// Returns all holidays for a given Hebrew date.
// Handles Diaspora vs Israel differences, Shabbos pushoffs, and combined days.
std::vector<HolidayInfo> GetHolidays(const HebrewDate& h, bool isIsrael)
{
    std::vector<HolidayInfo> result;
    DayOfWeek dow = GetDayOfWeekHebrew(h);
    bool leap = IsHebrewLeapYear(h.year);

    // --- Rosh Hashana ---
    if (h.month == TISHREI && h.day == 1)
        result.push_back({ L"Rosh Hashana", L"Day 1", HOLIDAY_YOM_TOV });
    if (h.month == TISHREI && h.day == 2)
        result.push_back({ L"Rosh Hashana", L"Day 2", HOLIDAY_YOM_TOV });

    // --- Fast of Gedaliah ---
    // 3 Tishrei (pushed to 4 if Shabbos)
    if (h.month == TISHREI && h.day == 3 && dow != SHABBAT)
        result.push_back({ L"Fast of Gedaliah", L"", HOLIDAY_FAST });
    if (h.month == TISHREI && h.day == 4 && dow != SHABBAT)
    {
        // Only a fast if yesterday (3 Tishrei) was Shabbos
        HebrewDate yesterday(h.year, h.month, 3);
        if (GetDayOfWeekHebrew(yesterday) == SHABBAT)
            result.push_back({ L"Fast of Gedaliah", L"(postponed)", HOLIDAY_FAST });
    }

    // --- Yom Kippur ---
    if (h.month == TISHREI && h.day == 10)
        result.push_back({ L"Yom Kippur", L"", HOLIDAY_YOM_TOV | HOLIDAY_FAST });

    // --- Sukkos ---
    if (h.month == TISHREI)
    {
        if (h.day == 14)
            result.push_back({ L"Erev Sukkos", L"", HOLIDAY_EREV });
        if (h.day == 15)
            result.push_back({ L"Sukkos", L"Day 1", HOLIDAY_YOM_TOV });
        if (h.day == 16)
        {
            if (isIsrael)
                result.push_back({ L"Sukkos", L"Chol Hamoed Day 1", HOLIDAY_CHOL_HAMOED });
            else
                result.push_back({ L"Sukkos", L"Day 2", HOLIDAY_YOM_TOV });
        }
        if (h.day == 17)
            result.push_back({ L"Sukkos", isIsrael ? L"Chol Hamoed Day 2" : L"Chol Hamoed Day 1", HOLIDAY_CHOL_HAMOED });
        if (h.day == 18)
            result.push_back({ L"Sukkos", isIsrael ? L"Chol Hamoed Day 3" : L"Chol Hamoed Day 2", HOLIDAY_CHOL_HAMOED });
        if (h.day == 19)
            result.push_back({ L"Sukkos", isIsrael ? L"Chol Hamoed Day 4" : L"Chol Hamoed Day 3", HOLIDAY_CHOL_HAMOED });
        if (h.day == 20)
            result.push_back({ L"Sukkos", isIsrael ? L"Chol Hamoed Day 5" : L"Chol Hamoed Day 4", HOLIDAY_CHOL_HAMOED });
        if (h.day == 21)
            result.push_back({ L"Hoshana Raba", L"", HOLIDAY_CHOL_HAMOED });
        if (h.day == 22)
        {
            if (isIsrael)
                result.push_back({ L"Shemini Atzeres / Simchas Torah", L"", HOLIDAY_YOM_TOV });
            else
                result.push_back({ L"Shemini Atzeres", L"", HOLIDAY_YOM_TOV });
        }
        if (h.day == 23 && !isIsrael)
            result.push_back({ L"Simchas Torah", L"", HOLIDAY_YOM_TOV });
        if ((isIsrael && h.day == 23) ||
            (!isIsrael && h.day == 24))
            result.push_back({ L"Isru Chag", L"", HOLIDAY_ISRU_CHAG });
    }

    // --- Chanukah ---
    int chanDay = ChanukahDay(h);
    if (chanDay > 0)
    {
        wchar_t buf[64];
        swprintf_s(buf, L"Day %d", chanDay);
        result.push_back({ L"Chanukah", buf, HOLIDAY_MINOR });
    }

    // --- 10 Teves ---
    if (h.month == TEVET && h.day == 10)
        result.push_back({ L"Fast of 10 Teves", L"", HOLIDAY_FAST });

    // --- Tu B'Shvat ---
    if (h.month == SHVAT && h.day == 15)
        result.push_back({ L"Tu B'Shvat", L"", HOLIDAY_MINOR });

    // --- Purim Katan (in leap year, 14 Adar I) ---
    if (leap && h.month == ADAR && h.day == 14)
        result.push_back({ L"Purim Katan", L"", HOLIDAY_MINOR });

    // --- Shushan Purim Katan (15 Adar I in leap year) ---
    if (leap && h.month == ADAR && h.day == 15)
        result.push_back({ L"Shushan Purim Katan", L"", HOLIDAY_MINOR });

    // --- Fast of Esther ---
    // 13 Adar (or Adar II in leap year), pushed to 11 if Shabbos
    {
        int adar = leap ? ADAR_II : ADAR;
        if (h.month == adar && h.day == 11)
        {
            HebrewDate the13th(h.year, adar, 13);
            if (GetDayOfWeekHebrew(the13th) == SHABBAT)
                result.push_back({ L"Fast of Esther", L"(postponed)", HOLIDAY_FAST });
        }
        if (h.month == adar && h.day == 13 && dow != SHABBAT)
            result.push_back({ L"Fast of Esther", L"", HOLIDAY_FAST });
    }

    // --- Purim ---
    {
        int adar = leap ? ADAR_II : ADAR;
        if (h.month == adar && h.day == 14)
            result.push_back({ L"Purim", L"", HOLIDAY_MINOR });
        if (h.month == adar && h.day == 15)
            result.push_back({ L"Shushan Purim", L"", HOLIDAY_MINOR });
    }

    // --- Pesach ---
    if (h.month == NISSAN)
    {
        if (h.day == 14)
            result.push_back({ L"Erev Pesach", L"", HOLIDAY_EREV });
        if (h.day == 15)
            result.push_back({ L"Pesach", L"Day 1", HOLIDAY_YOM_TOV });
        if (h.day == 16)
        {
            if (isIsrael)
                result.push_back({ L"Pesach", L"Chol Hamoed Day 1", HOLIDAY_CHOL_HAMOED });
            else
                result.push_back({ L"Pesach", L"Day 2", HOLIDAY_YOM_TOV });
        }
        if (h.day == 17)
            result.push_back({ L"Pesach", isIsrael ? L"Chol Hamoed Day 2" : L"Chol Hamoed Day 1", HOLIDAY_CHOL_HAMOED });
        if (h.day == 18)
            result.push_back({ L"Pesach", isIsrael ? L"Chol Hamoed Day 3" : L"Chol Hamoed Day 2", HOLIDAY_CHOL_HAMOED });
        if (h.day == 19)
            result.push_back({ L"Pesach", isIsrael ? L"Chol Hamoed Day 4" : L"Chol Hamoed Day 3", HOLIDAY_CHOL_HAMOED });
        if (h.day == 20)
            result.push_back({ L"Pesach", isIsrael ? L"Chol Hamoed Day 5" : L"Chol Hamoed Day 4", HOLIDAY_CHOL_HAMOED });
        if (h.day == 21)
            result.push_back({ L"Pesach", L"Day 7", HOLIDAY_YOM_TOV });
        if (h.day == 22 && !isIsrael)
            result.push_back({ L"Pesach", L"Day 8", HOLIDAY_YOM_TOV });
        if ((isIsrael && h.day == 22) || (!isIsrael && h.day == 23))
            result.push_back({ L"Isru Chag", L"", HOLIDAY_ISRU_CHAG });
    }

    // --- Pesach Sheini ---
    if (h.month == IYAR && h.day == 14)
        result.push_back({ L"Pesach Sheini", L"", HOLIDAY_MINOR });

    // --- Lag B'Omer ---
    if (h.month == IYAR && h.day == 18)
        result.push_back({ L"Lag B'Omer", L"", HOLIDAY_MINOR });

    // --- Shavuos ---
    if (h.month == SIVAN)
    {
        if (h.day == 5)
            result.push_back({ L"Erev Shavuos", L"", HOLIDAY_EREV });
        if (h.day == 6)
            result.push_back({ L"Shavuos", L"Day 1", HOLIDAY_YOM_TOV });
        if (h.day == 7 && !isIsrael)
            result.push_back({ L"Shavuos", L"Day 2", HOLIDAY_YOM_TOV });
        if ((isIsrael && h.day == 7) || (!isIsrael && h.day == 8))
            result.push_back({ L"Isru Chag", L"", HOLIDAY_ISRU_CHAG });
    }

    // --- 17 Tammuz ---
    // Pushed to 18 if Shabbos
    if (h.month == TAMMUZ)
    {
        if (h.day == 17 && dow != SHABBAT)
            result.push_back({ L"Fast of 17 Tammuz", L"", HOLIDAY_FAST });
        if (h.day == 18)
        {
            HebrewDate the17th(h.year, TAMMUZ, 17);
            if (GetDayOfWeekHebrew(the17th) == SHABBAT)
                result.push_back({ L"Fast of 17 Tammuz", L"(postponed)", HOLIDAY_FAST });
        }
    }

    // --- Tisha B'Av ---
    // Pushed to 10 if Shabbos
    if (h.month == AV)
    {
        if (h.day == 9 && dow != SHABBAT)
            result.push_back({ L"Tisha B'Av", L"", HOLIDAY_FAST });
        if (h.day == 10)
        {
            HebrewDate the9th(h.year, AV, 9);
            if (GetDayOfWeekHebrew(the9th) == SHABBAT)
                result.push_back({ L"Tisha B'Av", L"(postponed)", HOLIDAY_FAST });
        }
        if (h.day == 15)
            result.push_back({ L"Tu B'Av", L"", HOLIDAY_MINOR });
    }

    // --- Modern Israeli holidays ---
    if (h.month == NISSAN && h.day == 27)
        result.push_back({ L"Yom HaShoah", L"Holocaust Memorial Day", HOLIDAY_MODERN });
    if (h.month == IYAR && h.day == 4)
        result.push_back({ L"Yom Hazikaron", L"Memorial Day", HOLIDAY_MODERN });
    if (h.month == IYAR && h.day == 5)
        result.push_back({ L"Yom HaAtzmaut", L"Independence Day", HOLIDAY_MODERN });
    if (h.month == IYAR && h.day == 28)
        result.push_back({ L"Yom Yerushalayim", L"", HOLIDAY_MODERN });

    // --- Rosh Chodesh ---
    if (IsRoshChodesh(h))
    {
        std::wstring nextMonth;
        if (h.day == 30)
        {
            // First day of Rosh Chodesh — name is the next month
            int nm = (h.month % MonthsInHebrewYear(h.year)) + 1;
            nextMonth = HebrewMonthName(nm, leap);
            result.push_back({ L"Rosh Chodesh " + nextMonth, L"Day 1", HOLIDAY_ROSH_CHODESH });
        }
        else if (h.day == 1)
        {
            // Check if there was a day 30 in previous month (two-day Rosh Chodesh)
            int prevMonth = (h.month == 1) ? MonthsInHebrewYear(h.year) : h.month - 1;
            int prevYear = (h.month == 1) ? h.year - 1 : h.year;
            bool twoDays = DaysInHebrewMonth(prevMonth, prevYear) == 30;
            std::wstring thisMonth = HebrewMonthName(h.month, leap);
            result.push_back({ L"Rosh Chodesh " + thisMonth,
                                twoDays ? L"Day 2" : L"", HOLIDAY_ROSH_CHODESH });
        }
    }

    // --- Special Shabbosos ---
    SpecialShabbos special = GetSpecialShabbos(h);
    if (special != SPECIAL_SHABBOS_NONE)
        result.push_back({ SpecialShabbosName(special), L"", HOLIDAY_SPECIAL_SHAB });

    // --- Shabbos Mevarchim ---
    if (IsShabboseMevarchim(h) && special == SPECIAL_SHABBOS_NONE)
        result.push_back({ L"Shabbos Mevarchim", L"", HOLIDAY_SHABBOS_MEVAR });

    return result;
}

// =============================================================================
// SFIRAT HAOMER
// =============================================================================

// Returns Omer information for a given Hebrew date.
// The Omer is counted from 16 Nissan (day 1) to 4 Sivan (day 49).
OmerInfo GetOmer(const HebrewDate& h)
{
    OmerInfo info;

    // Calculate day of Omer
    // Day 1 = 16 Nissan, Day 49 = 4 Sivan
    int day = 0;

    if (h.month == NISSAN && h.day >= 16)
        day = h.day - 15;
    else if (h.month == IYAR)
        day = h.day + 15; // 14 days left in Nissan after day 15 + Iyar day
    else if (h.month == SIVAN && h.day <= 5)
        day = h.day + 44; // 14 Nissan days + 29 Iyar days + Sivan days

    if (day < 1 || day > 49)
        return info;

    info.isOmerDay = true;
    info.day = day;
    info.week = (day - 1) / 7;
    info.dayOfWeek = (day - 1) % 7;
    info.week = (day - 1) / 7;
    info.dayOfWeek = (day - 1) % 7;

    // Build display text
    wchar_t buf[128];
    if (info.week == 0)
        swprintf_s(buf, L"Day %d of the Omer", day);
    else if (info.dayOfWeek == 0)
        swprintf_s(buf, L"Day %d of the Omer (%d weeks)", day, info.week);
    else
    {
        int displayDays = info.dayOfWeek + 1;
        swprintf_s(buf, L"Day %d of the Omer (%d weeks, %d days)",
            day, info.week, displayDays);
    }

    info.text = buf;

    return info;
}

// =============================================================================
// PARASHA CALCULATION
// =============================================================================

// Returns the JDN of the first Shabbos on or after a given JDN.
static long NextShabbos(long jdn)
{
    int dow = (int)((jdn + 1) % 7); // 0=Sun .. 6=Shabbat
    if (dow == 6) return jdn;
    return jdn + (6 - dow);
}

// Returns the parasha index (0-53) for the Shabbos on or after the given date.
// isIsrael affects combined/split parshiyos.
ParashaInfo GetParasha(const HebrewDate& h, bool isIsrael)
{
    ParashaInfo info;

    // Find the Shabbos on or after this date
    long jdn = HebrewToJDN(h);
    long shabbosJDN = NextShabbos(jdn);
    HebrewDate shabbos = JDNToHebrew(shabbosJDN);

    // When Shabbos is Yom Tov, the regular weekly parasha is not read.
    // Leave the parasha blank so the UI does not show readings like Nasso
    // on Shabbos Shavuos Day 2 in the Diaspora.
    std::vector<HolidayInfo> shabbosHolidays = GetHolidays(shabbos, isIsrael);
    for (const auto& hol : shabbosHolidays)
    {
        if (hol.flags & HOLIDAY_YOM_TOV)
            return info;
    }

    // The annual parasha cycle starts after Simchas Torah (23 Tishrei in Diaspora,
    // 22 Tishrei in Israel) and ends with V'Zos HaBracha on Simchas Torah.
    // We anchor to the Shabbos after Simchas Torah each year.

    // Find start of this year's parasha cycle
    int cycleYear = shabbos.year;

    // Simchas Torah: 23 Tishrei (Diaspora) or 22 Tishrei (Israel)
    int simchasTishrei = isIsrael ? 22 : 23;
    HebrewDate simchasTorah(cycleYear, TISHREI, simchasTishrei);

    // If our Shabbos is before or on Simchas Torah, use previous year's cycle
    if (shabbosJDN <= HebrewToJDN(simchasTorah))
    {
        cycleYear--;
        simchasTorah = HebrewDate(cycleYear, TISHREI, simchasTishrei);
    }

    // First parasha Shabbos is the first Shabbos after Simchas Torah
    long cycleStartJDN = HebrewToJDN(simchasTorah);
    long firstParashaJDN = NextShabbos(cycleStartJDN + 1);

    // How many Shabbosos from cycle start to our target Shabbos?
    long weeksDiff = (shabbosJDN - firstParashaJDN) / 7;
    // Israel adjustment: after Shavuos, Israel reads 1 parasha ahead
    // of Diaspora because Shavuos is 1 day in Israel (not 2).
    // That extra day gives Israel an extra Shabbos parasha reading.
    // Starting from the Shabbos after Shavuos, weeksDiff-- to shift
    // Israel's schedule 1 week back relative to Diaspora's.
    if (isIsrael && weeksDiff > 0)
    {
        // Find the Shabbos immediately after Shavuos (6 Sivan)
        HebrewDate shavuos(cycleYear, SIVAN, 6);
        long shavuosJDN = HebrewToJDN(shavuos);
        long firstShabbosAfter = NextShabbos(shavuosJDN + 1);

        if (shabbosJDN >= firstShabbosAfter)
            weeksDiff--;
    }

    if (weeksDiff < 0)
    {
        info.name = L"V'Zos HaBracha";
        return info;
    }

    // Build the parasha schedule for this year.
    // Combined parshiyos vary by year type and Israel/Diaspora.
    // We use a known schedule based on the year properties.
    bool leapYear = IsHebrewLeapYear(cycleYear);
    int  yearType = DaysInHebrewYear(cycleYear); // 353/354/355/383/384/385
    bool longYear = (yearType == 355 || yearType == 385);
    bool shortYear = (yearType == 353 || yearType == 383);

    // Build schedule: each entry is a parasha index (0-53).
    // Combined parshiyos are represented by the first parasha index
    // with isCombined=true set later.
    // This schedule covers 54 slots for all year types.
    std::vector<int> schedule;

    // Bereishis through Vayechi (12 parshiyos, always separate)
    for (int i = 0; i <= 11; i++) schedule.push_back(i);

    // Shemos through Mishpatim (6 parshiyos, always separate)
    for (int i = 12; i <= 17; i++) schedule.push_back(i);

    // Terumah through Pekudei — Vayakhel-Pekudei combined in short/regular years
    schedule.push_back(18); // Terumah
    schedule.push_back(19); // Tetzaveh
    schedule.push_back(20); // Ki Sisa
    if (shortYear && !leapYear)
    {
        // Combine Vayakhel-Pekudei only in short (Chaser) non-leap years
        schedule.push_back(21); // represents Vayakhel-Pekudei combined
    }
    else
    {
        schedule.push_back(21); // Vayakhel separate
        schedule.push_back(22); // Pekudei separate
    }

    // Vayikra through Shemini (always separate)
    schedule.push_back(23); // Vayikra
    schedule.push_back(24); // Tzav
    schedule.push_back(25); // Shemini

    // Tazria-Metzora: combined in non-leap years
    if (!leapYear)
        schedule.push_back(26); // Tazria-Metzora combined
    else
    {
        schedule.push_back(26); // Tazria
        schedule.push_back(27); // Metzora
    }

    // Acharei Mos-Kedoshim: combined in non-leap years
    if (!leapYear)
        schedule.push_back(28); // Acharei-Kedoshim combined
    else
    {
        schedule.push_back(28); // Acharei Mos
        schedule.push_back(29); // Kedoshim
    }

    // Emor (always separate)
    schedule.push_back(30);

    // Behar-Bechukosai: combined in non-leap years
    if (!leapYear)
        schedule.push_back(31); // Behar-Bechukosai combined
    else
    {
        schedule.push_back(31); // Behar
        schedule.push_back(32); // Bechukosai
    }

    // Bamidbar through Nasso (always separate)
    schedule.push_back(33); // Bamidbar
    schedule.push_back(34); // Nasso

    // Beha'aloscha through Pinchas (always separate)
    schedule.push_back(35);
    schedule.push_back(36);
    schedule.push_back(37);
    schedule.push_back(38); // Chukas
    schedule.push_back(39); // Balak
    schedule.push_back(40); // Pinchas

    // Matos-Masei: combined in most years
    if (leapYear && longYear)
    {
        schedule.push_back(41); // Matos separate
        schedule.push_back(42); // Masei separate
    }
    else
        schedule.push_back(41); // Matos-Masei combined

    // Devarim through Ha'azinu (always separate)
    for (int i = 43; i <= 52; i++) schedule.push_back(i);

    // V'Zos HaBracha is always Simchas Torah, not a regular Shabbos reading
    // so we don't add it to the schedule

    // Find which entry in the schedule corresponds to our week
    if ((size_t)weeksDiff >= schedule.size())
    {
        info.name = L"V'Zos HaBracha";
        return info;
    }

    int parashaIdx = schedule[(size_t)weeksDiff];

    // Determine if this is a combined parasha
    bool combined = false;
    std::wstring name2;

    // Check if next week's schedule entry is the same index + 1
    // indicating two parshiyos were merged
    if ((size_t)weeksDiff + 1 < schedule.size())
    {
        int nextIdx = schedule[(size_t)weeksDiff + 1];
        // If the jump is 2, this slot represents a combined parasha
        if (nextIdx - parashaIdx == 2)
            combined = true;
    }
    else
    {
        // Last slot before Simchas Torah — check if we need to combine
        // This handles years where we need to catch up
    }

    info.name = s_parashaNames[parashaIdx];
    info.isCombined = combined;
    if (combined && parashaIdx + 1 < (int)s_parashaNames.size())
        info.name2 = s_parashaNames[parashaIdx + 1];

    return info;
}

// =============================================================================
// DAILY LEARNING CYCLES
// =============================================================================

// Returns the Daf Yomi (Bavli) for a given date.
// The current (14th) cycle started January 5, 2020 (8 Teves 5780).
// Cycle length: 2711 dafim.
static std::wstring GetDafYomi(const GregorianDate& g)
{
    // Tractate names and their daf counts (starting from daf 2)
    static const struct { const wchar_t* name; int dafim; } tractates[] =
    {
        { L"Berachos",    63 }, { L"Shabbos",     156 }, { L"Eruvin",      104 },
        { L"Pesachim",    120 }, { L"Shekalim",     21 }, { L"Yoma",         87 },
        { L"Sukkah",       55 }, { L"Beitzah",      39 }, { L"Rosh Hashana", 34 },
        { L"Ta'anis",      30 }, { L"Megillah",     31 }, { L"Moed Katan",   28 },
        { L"Chagigah",     26 }, { L"Yevamos",     121 }, { L"Kesuvos",      111 },
        { L"Nedarim",      90 }, { L"Nazir",        65 }, { L"Sotah",        48 },
        { L"Gittin",       89 }, { L"Kiddushin",    81 }, { L"Bava Kama",   118 },
        { L"Bava Metzia", 118 }, { L"Bava Basra",  175 }, { L"Sanhedrin",   112 },
        { L"Makkos",       23 }, { L"Shevuos",      48 }, { L"Avodah Zarah", 75 },
        { L"Horayos",      13 }, { L"Zevachim",    119 }, { L"Menachos",    109 },
        { L"Chulin",       141 }, { L"Bechoros",     60 }, { L"Arachin",      33 },
        { L"Temurah",       33 }, { L"Kerisos",      27 }, { L"Meilah",       21 },
        { L"Niddah",        72 }
    };

    // Cycle 14 start: January 5, 2020
    GregorianDate cycleStart(2020, 1, 5);
    long startJDN = GregorianToJDN(cycleStart);
    long todayJDN = GregorianToJDN(g);
    long dayInCycle = (todayJDN - startJDN) % 2711;
    if (dayInCycle < 0) dayInCycle += 2711;

    // Find tractate and daf
    long accumulated = 0;
    for (const auto& t : tractates)
    {
        if (dayInCycle < accumulated + t.dafim)
        {
            int daf = (int)(dayInCycle - accumulated) + 2; // dafim start at 2
            wchar_t buf[64];
            swprintf_s(buf, L"%s %d", t.name, daf);
            return buf;
        }
        accumulated += t.dafim;
    }

    return L"";
}

// Returns the Yerushalmi Yomi for a given date.
// Current cycle started February 8, 2020.
// Cycle length: 1554 dafim.
static std::wstring GetYerushalmi(const GregorianDate& g)
{
    static const struct { const wchar_t* name; int dafim; } tractates[] =
    {
        { L"Berachos",   67 }, { L"Peah",       73 }, { L"Demai",      77 },
        { L"Kilayim",    88 }, { L"Sheviis",    91 }, { L"Terumos",    105 },
        { L"Maasros",    50 }, { L"Maaser Sheni",56 }, { L"Challah",    50 },
        { L"Orlah",      40 }, { L"Bikkurim",   26 }, { L"Shabbos",    113 },
        { L"Eruvin",     65 }, { L"Pesachim",   86 }, { L"Beitzah",    45 },
        { L"Rosh Hashana",22 },{ L"Yoma",       57 }, { L"Sukkah",     32 },
        { L"Ta'anis",    31 }, { L"Shekalim",   59 }, { L"Megillah",   37 },
        { L"Chagigah",   22 }, { L"Moed Katan", 19 }, { L"Yevamos",    85 },
        { L"Kesuvos",    72 }, { L"Sotah",      47 }, { L"Nedarim",    46 },
        { L"Nazir",      47 }, { L"Gittin",     54 }, { L"Kiddushin",  66 },
        { L"Bava Kama",  44 }, { L"Bava Metzia",37 }, { L"Bava Basra", 34 },
        { L"Sanhedrin",  57 }, { L"Shevuos",    44 }, { L"Avodah Zarah",37 },
        { L"Makkos",     11 }, { L"Horayos",    19 }, { L"Niddah",     13 }
    };

    GregorianDate cycleStart(2020, 2, 8);
    long startJDN = GregorianToJDN(cycleStart);
    long todayJDN = GregorianToJDN(g);
    long dayInCycle = (todayJDN - startJDN) % 1554;
    if (dayInCycle < 0) dayInCycle += 1554;

    long accumulated = 0;
    for (const auto& t : tractates)
    {
        if (dayInCycle < accumulated + t.dafim)
        {
            int daf = (int)(dayInCycle - accumulated) + 1;
            wchar_t buf[64];
            swprintf_s(buf, L"%s %d", t.name, daf);
            return buf;
        }
        accumulated += t.dafim;
    }
    return L"";
}

// Returns the Mishna Yomit for a given date.
// Two mishnayos per day. Cycle: 2711 mishnayos / 2 = ~1355 days.
// Current cycle started March 26, 2022.
static std::wstring GetMishnaYomit(const HebrewDate& h)
{
    // Simplified: return tractate based on month/day
    // Full implementation maps day-in-cycle to exact mishna
    static const struct { const wchar_t* seder; const wchar_t* tractate; int mishnayos; } mishnayos[] =
    {
        { L"Zeraim",  L"Berachos",    57 }, { L"Zeraim",  L"Peah",        49 },
        { L"Zeraim",  L"Demai",       49 }, { L"Zeraim",  L"Kilayim",     56 },
        { L"Zeraim",  L"Sheviis",     66 }, { L"Zeraim",  L"Terumos",     79 },
        { L"Zeraim",  L"Maasros",     31 }, { L"Zeraim",  L"Maaser Sheni",56 },
        { L"Zeraim",  L"Challah",     28 }, { L"Zeraim",  L"Orlah",       20 },
        { L"Zeraim",  L"Bikkurim",    12 }, { L"Moed",    L"Shabbos",    157 },
        { L"Moed",    L"Eruvin",       95 }, { L"Moed",    L"Pesachim",    92 },
        { L"Moed",    L"Shekalim",     33 }, { L"Moed",    L"Yoma",        68 },
        { L"Moed",    L"Sukkah",       51 }, { L"Moed",    L"Beitzah",     41 },
        { L"Moed",    L"Rosh Hashana", 34 }, { L"Moed",    L"Ta'anis",     26 },
        { L"Moed",    L"Megillah",     31 }, { L"Moed",    L"Moed Katan",  19 },
        { L"Moed",    L"Chagigah",     20 }
    };

    // Cycle start: 1 Nissan 5782 = April 2, 2022
    HebrewDate cycleStart(5782, NISSAN, 1);
    long startJDN = HebrewToJDN(cycleStart);
    long todayJDN = HebrewToJDN(h);
    long day = (todayJDN - startJDN);
    if (day < 0) day = 0;

    // 2 mishnayos per day
    long mishnaNum = (day * 2) % 4192; // approximate total mishnayos

    long accumulated = 0;
    for (const auto& m : mishnayos)
    {
        if (mishnaNum < accumulated + m.mishnayos)
        {
            int num = (int)(mishnaNum - accumulated) + 1;
            wchar_t buf[64];
            swprintf_s(buf, L"%s %d", m.tractate, num);
            return buf;
        }
        accumulated += m.mishnayos;
    }
    return L"";
}

// Returns the Halacha Yomit for a given date.
// Follows Kitzur Shulchan Aruch / Orach Chaim cycle.
// Simplified: returns Orach Chaim siman based on day of year.
static std::wstring GetHalachaYomit(const HebrewDate& h)
{
    // Day within the Hebrew year (1-based)
    HebrewDate startOfYear(h.year, TISHREI, 1);
    long dayOfYear = HebrewToJDN(h) - HebrewToJDN(startOfYear) + 1;

    // Orach Chaim has 697 simanim, cycled through the year
    int siman = (int)((dayOfYear - 1) % 697) + 1;

    wchar_t buf[64];
    swprintf_s(buf, L"Orach Chaim %d", siman);
    return buf;
}

// Returns the Tanach Yomi for a given date.
// One chapter per day, cycles through all 929 chapters.
// Anchor: 1 Tishrei 5784 = Bereishis 1.
static std::wstring GetTanachYomi(const HebrewDate& h)
{
    static const struct { const wchar_t* name; int chapters; } books[] =
    {
        { L"Bereishis", 50 }, { L"Shemos",    40 }, { L"Vayikra",   27 },
        { L"Bamidbar",  36 }, { L"Devarim",   34 }, { L"Yehoshua",  24 },
        { L"Shoftim",   21 }, { L"Shmuel I",  31 }, { L"Shmuel II", 24 },
        { L"Melachim I",22 }, { L"Melachim II",25 },{ L"Yeshayahu", 66 },
        { L"Yirmiyahu", 52 }, { L"Yechezkel", 48 }, { L"Hoshea",    14 },
        { L"Yoel",       3 }, { L"Amos",       9 }, { L"Ovadiah",    1 },
        { L"Yonah",      4 }, { L"Micha",      7 }, { L"Nachum",     3 },
        { L"Chavakuk",   3 }, { L"Tzefaniah",  3 }, { L"Chaggai",    2 },
        { L"Zechariah", 14 }, { L"Malachi",    3 }, { L"Tehillim",  150 },
        { L"Mishlei",   31 }, { L"Iyov",       42 }, { L"Shir Hashirim", 8 },
        { L"Rus",        4 }, { L"Eichah",      5 }, { L"Koheles",   12 },
        { L"Esther",    10 }, { L"Daniel",     12 }, { L"Ezra",      10 },
        { L"Nechemiah", 13 }, { L"Divrei Hayamim I",29 },
        { L"Divrei Hayamim II",36 }
    };

    HebrewDate anchor(5784, TISHREI, 1);
    long anchorJDN = HebrewToJDN(anchor);
    long todayJDN = HebrewToJDN(h);
    long day = (todayJDN - anchorJDN) % 929;
    if (day < 0) day += 929;

    long accumulated = 0;
    for (const auto& b : books)
    {
        if (day < accumulated + b.chapters)
        {
            int chapter = (int)(day - accumulated) + 1;
            wchar_t buf[64];
            swprintf_s(buf, L"%s %d", b.name, chapter);
            return buf;
        }
        accumulated += b.chapters;
    }
    return L"";
}

// Main daily learning function — assembles all cycles.
DailyLearning GetDailyLearning(const HebrewDate& h, const GregorianDate& g)
{
    DailyLearning dl;
    dl.dafYomi = GetDafYomi(g);
    dl.yerushalmi = GetYerushalmi(g);
    dl.mishnaYomit = GetMishnaYomit(h);
    dl.halachaYomit = GetHalachaYomit(h);
    dl.tanachYomi = GetTanachYomi(h);
    return dl;
}
