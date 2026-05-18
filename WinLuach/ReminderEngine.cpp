// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    ReminderEngine.cpp
// Purpose: Advanced reminder firing engine implementation.
// =============================================================================
//
// CHANGELOG:
// v0.8.71 - Initial implementation. See ReminderEngine.h for function docs.
// =============================================================================

#include "pch.h"
#include "ReminderEngine.h"
#include "Zmanim.h"          // ZmanimResult, DisplayZmanimTimes, TimeOfDay, CalculateZmanim
#include "HebrewDate.h"      // JDNToHebrew, JDNToGregorian, GregorianToJDN, GetTodayGregorian
#include "HolidayEngine.h"   // GetHolidays, HolidayInfo, HOLIDAY_* flags

// ============================================================================
// Internal helpers
// ============================================================================

// Converts a GregorianDate + TimeOfDay to a CTime value.
// (Mirrors the static DateTimeForZman() defined in MainFrame.cpp.)
static CTime DateTimeForZmanLocal(const GregorianDate& g, const TimeOfDay& t)
{
    if (!t.IsValid()) return CTime(0);
    return CTime(g.year, g.month, g.day,
                 max(0, t.hour), max(0, t.minute), max(0, t.second));
}

// Returns a CTime for hour:minute on the given civil date.
static CTime TimeOnDate(const GregorianDate& g, int hour, int minute)
{
    return CTime(g.year, g.month, g.day, max(0, min(23, hour)), max(0, min(59, minute)), 0);
}

// Advances a GregorianDate by offsetDays (positive or negative).
static GregorianDate ShiftDate(const GregorianDate& g, int offsetDays)
{
    return JDNToGregorian(GregorianToJDN(g) + offsetDays);
}

// Returns today as a "YYYY-MM-DD" wstring for last-fired deduplication.
std::wstring TodayDateString()
{
    SYSTEMTIME st = {};
    GetLocalTime(&st);
    wchar_t buf[32];
    _snwprintf_s(buf, _TRUNCATE, L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
    return buf;
}

// ============================================================================
// ParseReminderOffset
// Splits "15 minutes" → amount=15, unit=L"minutes".
// ============================================================================
bool ParseReminderOffset(const std::wstring& offsets, int& outAmount, std::wstring& outUnit)
{
    outAmount = 0;
    outUnit   = L"minutes";
    if (offsets.empty()) return false;

    // Ignore anything after the first semicolon (single-offset per rule)
    std::wstring s = offsets.substr(0, offsets.find(L';'));
    // Trim leading whitespace
    size_t start = s.find_first_not_of(L" \t");
    if (start == std::wstring::npos) return false;
    s = s.substr(start);

    // Split on first space: "<number> <unit>"
    size_t sp = s.find(L' ');
    if (sp == std::wstring::npos)
    {
        // Numeric only → treat as minutes
        try { outAmount = std::stoi(s); } catch (...) { return false; }
        return true;
    }

    try { outAmount = std::stoi(s.substr(0, sp)); } catch (...) { return false; }

    // Trim unit
    size_t us = s.find_first_not_of(L" \t", sp + 1);
    if (us != std::wstring::npos)
        outUnit = s.substr(us);

    return outAmount > 0;
}

// ============================================================================
// MatchesReminderTarget
// Maps UI target strings to HolidayInfo records from the engine.
// ============================================================================
bool MatchesReminderTarget(const HolidayInfo& ho, const std::wstring& target)
{
    const std::wstring& n = ho.name;
    const std::wstring& s = ho.subtitle;
    const int           f = ho.flags;

    // --- Rosh Chodesh ---
    if (target == L"Rosh Chodesh (Day 1)")
        return (f & HOLIDAY_ROSH_CHODESH) && s == L"Day 1";
    if (target == L"Rosh Chodesh (Day 2)")
        return (f & HOLIDAY_ROSH_CHODESH) && s == L"Day 2";

    // --- Chol HaMoed ---
    if (target == L"Chol HaMoed Sukkos")
        return (f & HOLIDAY_CHOL_HAMOED) && n == L"Sukkos";
    if (target == L"Chol HaMoed Pesach")
        return (f & HOLIDAY_CHOL_HAMOED) && n == L"Pesach";

    // --- Name normalisation for UI vs engine spelling differences ---
    if (target == L"Tu BiShvat")    return n == L"Tu B'Shvat";
    if (target == L"Lag BaOmer")    return n == L"Lag B'Omer";
    if (target == L"Tisha B'Av")    return n == L"Tisha B'Av";

    // --- Isru Chag (matches any of the three) ---
    if (target == L"Isru Chag")
        return (f & HOLIDAY_ISRU_CHAG);

    // --- Default: exact name match ---
    return n == target;
}

// ============================================================================
// FindNextHolidayDate
// Scans forward day-by-day up to maxDays to find the first civil date whose
// Hebrew date produces a HolidayInfo matching the target string.
// Returns the civil GregorianDate of that Hebrew day (the "morning" side),
// or {0,0,0} if not found.
// ============================================================================
GregorianDate FindNextHolidayDate(const std::wstring& target,
                                  const GregorianDate& from,
                                  bool isIsrael,
                                  int maxDays)
{
    long fromJDN = GregorianToJDN(from);
    for (int d = 0; d <= maxDays; ++d)
    {
        long jdn          = fromJDN + d;
        HebrewDate heb    = JDNToHebrew(jdn);
        auto hols         = GetHolidays(heb, isIsrael);
        for (const auto& ho : hols)
            if (MatchesReminderTarget(ho, target))
                return JDNToGregorian(jdn);
    }
    GregorianDate notFound = {};  // {0,0,0}
    return notFound;
}

// ============================================================================
// ZmanTimeForTarget
// Maps a Zman target name string to a TimeOfDay.
// Covers all entries in FillTargets() → Zman branch.
// ============================================================================
TimeOfDay ZmanTimeForTarget(const std::wstring& target,
                            const ZmanimResult& z,
                            const DisplayZmanimTimes& dz)
{
    // Standard GRA / MA zmanim
    if (target == L"Alos (GRA 16.1 deg)")          return z.alot_GRA;
    if (target == L"Alos (MA 72)")                  return z.alot_MA72;
    if (target == L"Alos (MA 90)")                  return z.alot_MA90;
    if (target == L"Alos (MA 72 proportional)")     return z.alot_MA72;   // best available
    if (target == L"Alos (MA 90 proportional)")     return z.alot_MA90;
    if (target == L"Misheyakir (10.2 deg)")         return z.misheyakir_10;
    if (target == L"Misheyakir (11.5 deg)")         return z.misheyakir_11;
    if (target == L"Hanetz")                        return z.hanetz;
    if (target == L"Sof Shema (GRA)")               return z.sofShema_GRA;
    if (target == L"Sof Shema (MA 72)")             return z.sofShema_MA72;
    if (target == L"Sof Shema (MA 90)")             return z.sofShema_MA90;
    if (target == L"Sof Tefilla (GRA)")             return z.sofTefilla_GRA;
    if (target == L"Sof Tefilla (MA 72)")           return z.sofTefilla_MA72;
    if (target == L"Sof Tefilla (MA 90)")           return z.sofTefilla_MA90;
    if (target == L"Chatzos")                       return z.chatzot;
    if (target == L"Mincha Gedola (GRA)")           return z.minchaGedola_GRA;
    if (target == L"Mincha Gedola (MA 72)")         return z.minchaGedola_MA72;
    if (target == L"Mincha Gedola (MA 90)")         return z.minchaGedola_MA90;
    if (target == L"Mincha Ketana (GRA)")           return z.minchaKetana_GRA;
    if (target == L"Mincha Ketana (MA 72)")         return z.minchaKetana_MA72;
    if (target == L"Plag (GRA)")                    return z.plagMincha_GRA;
    if (target == L"Plag (MA 72)")                  return z.plagMincha_MA72;
    if (target == L"Plag (MA 90)")                  return z.plagMincha_MA90;
    if (target == L"Shkiah")                        return z.shkia;
    if (target == L"Tzeis (GRA 8.5 deg)")           return z.tzeit_GRA;
    if (target == L"Tzeis (MA 72)")                 return z.tzeit_MA72;
    if (target == L"Tzeis (MA 90)")                 return z.tzeit_MA90;
    if (target == L"Tzeis (MA 72 proportional)")    return z.tzeit_MA72;
    if (target == L"Tzeis (MA 90 proportional)")    return z.tzeit_MA90;
    if (target == L"Candle Lighting")               return z.candleLighting;
    if (target == L"Fast start/end")                return z.shkia;       // fast starts at sunrise / ends at tzeit; use shkia as a proxy
    if (target == L"Eat chametz")                   return z.sofTefilla_GRA;
    if (target == L"Burn chametz")                  return z.sofTefilla_GRA;

    // Custom shita zmanim (user's configured preference)
    if (target == L"Custom Alos")                   return dz.alot;
    if (target == L"Custom Sof Shema")              return dz.sofShema;
    if (target == L"Custom Sof Tefilla")            return dz.sofTefilla;
    if (target == L"Custom Mincha Gedola")          return dz.minchaGedola;
    if (target == L"Custom Mincha Ketana")          return dz.minchaKetana;
    if (target == L"Custom Plag HaMincha")          return dz.plagMincha;
    if (target == L"Custom Tzeit")                  return dz.tzeit;

    TimeOfDay invalid; invalid.hour = -1; invalid.minute = -1; invalid.second = -1;
    return invalid;
}

// ============================================================================
// Internal: compute offset in seconds (positive) from amount + unit.
// Returns -1 for non-time units (days / weeks / months / years).
// ============================================================================
static long OffsetSeconds(int amount, const std::wstring& unit)
{
    if (unit == L"minutes") return (long)amount * 60;
    if (unit == L"hours")   return (long)amount * 3600;
    return -1; // day-granularity units handled separately
}

// ============================================================================
// Internal: number of civil days for day-granularity offsets.
// Returns -1 for time-granularity units (minutes / hours).
// ============================================================================
static int OffsetDays(int amount, const std::wstring& unit)
{
    if (unit == L"days")   return amount;
    if (unit == L"weeks")  return amount * 7;
    if (unit == L"months") return amount * 30;  // approximate
    if (unit == L"years")  return amount * 365; // approximate
    return -1;
}

// ============================================================================
// ComputeReminderFireTime
// ============================================================================
CTime ComputeReminderFireTime(const ReminderRule&       rule,
                              const AppSettings&        settings,
                              bool                      isIsrael,
                              const Location&           location,
                              const ZmanimResult&       todayZ,
                              const DisplayZmanimTimes& todayDz,
                              const GregorianDate&      today)
{
    if (!rule.enabled) return CTime(0);

    int          amount = 0;
    std::wstring unit;
    if (!ParseReminderOffset(rule.offsets, amount, unit)) return CTime(0);

    bool after      = rule.afterEvent;
    long secOffset  = OffsetSeconds(amount, unit);  // -1 if day-granularity
    int  dayOffset  = OffsetDays(amount, unit);      // -1 if time-granularity
    bool isTimeUnit = (secOffset >= 0);

    // -------------------------------------------------------------------------
    // ZMAN KIND — fires relative to today's (or tomorrow's) zman
    // -------------------------------------------------------------------------
    if (rule.kind == L"Zman")
    {
        // Check today and tomorrow so we catch zmanim that cross midnight
        for (int d = 0; d < 2; ++d)
        {
            GregorianDate g = ShiftDate(today, d);
            bool isDst = IsDST(g, location);
            ZmanimResult z = (d == 0) ? todayZ : CalculateZmanim(g, location, isDst);
            DisplayZmanimTimes dz = todayDz; // custom zmanim only computed for today by caller; use for both days

            TimeOfDay zt = ZmanTimeForTarget(rule.target, z, dz);
            if (!zt.IsValid()) continue;

            CTime base = DateTimeForZmanLocal(g, zt);
            if (base.GetTime() == 0) continue;

            CTime fire;
            if (isTimeUnit)
                fire = after ? (base + CTimeSpan(0, 0, 0, (int)secOffset))
                             : (base - CTimeSpan(0, 0, 0, (int)secOffset));
            else
            {
                // Day-granularity for a zman: fire at daily reminder time on
                // the date that is dayOffset civil days before/after the zman date
                GregorianDate fireDate = after ? ShiftDate(g, dayOffset) : ShiftDate(g, -dayOffset);
                fire = TimeOnDate(fireDate, settings.reminderDailyHour, settings.reminderDailyMinute);
            }

            if (fire.GetTime() != 0) return fire;
        }
        return CTime(0);
    }

    // -------------------------------------------------------------------------
    // HOLIDAY KIND — fires relative to the anchor time on the eve of the holiday
    // -------------------------------------------------------------------------
    if (rule.kind == L"Holiday")
    {
        // Scan from yesterday so we don't miss a holiday that starts tonight
        GregorianDate scanFrom = ShiftDate(today, -1);
        GregorianDate holDate  = FindNextHolidayDate(rule.target, scanFrom, isIsrael, 366);
        if (holDate.year == 0) return CTime(0);

        // eveDate: the civil day on which the holiday BEGINS at nightfall
        // = holDate - 1 (because holDate is the civil day whose morning is the holiday)
        GregorianDate eveDate = ShiftDate(holDate, -1);

        CTime anchorTime(0);
        if (rule.anchor == 2)
        {
            // Civil midnight of the holiday civil day
            anchorTime = TimeOnDate(holDate, 0, 0);
        }
        else
        {
            bool isDst = IsDST(eveDate, location);
            ZmanimResult ez = CalculateZmanim(eveDate, location, isDst);
            TimeOfDay zt = (rule.anchor == 1) ? ez.shkia : ez.tzeit_GRA; // 0=Tzais, 1=Shkia
            anchorTime = DateTimeForZmanLocal(eveDate, zt);
        }

        if (anchorTime.GetTime() == 0) return CTime(0);

        CTime fire;
        if (isTimeUnit)
        {
            fire = after ? (anchorTime + CTimeSpan(0, 0, 0, (int)secOffset))
                         : (anchorTime - CTimeSpan(0, 0, 0, (int)secOffset));
        }
        else
        {
            // Day-granularity: fire at daily reminder time on the offset civil date
            GregorianDate fireDate = after ? ShiftDate(eveDate, dayOffset) : ShiftDate(eveDate, -dayOffset);
            fire = TimeOnDate(fireDate, settings.reminderDailyHour, settings.reminderDailyMinute);
        }

        return fire;
    }

    // -------------------------------------------------------------------------
    // PARSHA KIND — fires at candle lighting on the upcoming Friday (±offset)
    // -------------------------------------------------------------------------
    if (rule.kind == L"Parsha")
    {
        // Find the next Friday (0 = today if today is Friday)
        DayOfWeek dow = GetDayOfWeek(today);
        int daysToFri = (FRIDAY - (int)dow + 7) % 7;
        GregorianDate friday = ShiftDate(today, daysToFri);

        bool isDst = IsDST(friday, location);
        ZmanimResult fz = CalculateZmanim(friday, location, isDst);
        // Candle lighting = shkia - user offset
        TimeOfDay cl = fz.candleLighting.IsValid()
            ? fz.candleLighting
            : AddMinutes(fz.shkia, -settings.candleLightingMinutes);
        CTime base = DateTimeForZmanLocal(friday, cl);
        if (base.GetTime() == 0) return CTime(0);

        CTime fire;
        if (isTimeUnit)
            fire = after ? (base + CTimeSpan(0, 0, 0, (int)secOffset))
                         : (base - CTimeSpan(0, 0, 0, (int)secOffset));
        else
        {
            GregorianDate fireDate = after ? ShiftDate(friday, dayOffset) : ShiftDate(friday, -dayOffset);
            fire = TimeOnDate(fireDate, settings.reminderDailyHour, settings.reminderDailyMinute);
        }
        return fire;
    }

    return CTime(0);
}
