// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    Zmanim.cpp
// Purpose: Full implementation of halachic time (zmanim) calculations.
//          Astronomical sunrise/sunset uses the NOAA solar algorithm
//          (Jean Meeus, "Astronomical Algorithms", 2nd ed.).
//          Halachic times follow GRA and Magen Avraham opinions.
//          No UI dependencies. Pure calculation engine.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial implementation. Solar position algorithm (NOAA/Meeus).
//          All zmanim: alot, misheyakir, shema, tefilla, chatzot,
//          mincha gedola/ketana, plag, tzeit, candle lighting.
//          Supports GRA, MA 72, MA 90, MA 72P, MA 90P shitot.
//          DST detection for US rules (to be expanded per location).
// v0.1.1 - BUGFIX: CalcSunriseSunsetUTC() rewritten with correct NOAA formula.
//          Sunrise = solarNoon - 4*HA, Sunset = solarNoon + 4*HA.
//          Previous delta-based formula inverted sunrise/sunset results.
// =============================================================================

#include "pch.h"
#include "Zmanim.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
// INTERNAL MATH HELPERS
// =============================================================================

// Converts degrees to radians.
static double DegToRad(double deg) { return deg * M_PI / 180.0; }

// Converts radians to degrees.
static double RadToDeg(double rad) { return rad * 180.0 / M_PI; }

// Normalizes an angle in degrees to the range [0, 360).
static double NormalizeDeg(double deg)
{
    deg = fmod(deg, 360.0);
    if (deg < 0) deg += 360.0;
    return deg;
}

// Converts a fractional hour (e.g. 7.5) to a TimeOfDay (7:30:00).
static TimeOfDay FractionalHourToTime(double hours)
{
    if (hours < 0 || hours >= 24.0)
        return TimeOfDay(); // invalid

    int h = (int)hours;
    double rem = (hours - h) * 60.0;
    int m = (int)rem;
    int s = (int)((rem - m) * 60.0);

    return TimeOfDay(h, m, s);
}

// Converts a TimeOfDay to fractional hours (e.g. 7:30 -> 7.5).
// Returns -1.0 if the time is invalid.
static double TimeToFractionalHour(const TimeOfDay& t)
{
    if (!t.IsValid()) return -1.0;
    return t.hour + t.minute / 60.0 + t.second / 3600.0;
}

// =============================================================================
// JULIAN DATE
// =============================================================================

// Returns the Julian Date (continuous day + fraction) for a Gregorian date.
// This is the floating-point version used by the solar algorithm.
// Note: this is different from the integer JDN used in HebrewDate.cpp.
static double CalcJulianDate(const GregorianDate& g)
{
    int y = g.year;
    int m = g.month;
    int d = g.day;

    if (m <= 2) { y--; m += 12; }

    long A = (long)(y / 100);
    long B = 2 - A + (long)(A / 4);

    return (long)(365.25 * (y + 4716))
        + (long)(30.6001 * (m + 1))
        + d + B - 1524.5;
}

// Returns Julian centuries from J2000.0 epoch.
// J2000.0 = January 1, 2000 at 12:00 TT = JD 2451545.0
static double CalcJulianCentury(double julianDate)
{
    return (julianDate - 2451545.0) / 36525.0;
}

// =============================================================================
// SOLAR POSITION ALGORITHM (NOAA / Meeus)
// =============================================================================

// Returns the mean longitude of the sun in degrees.
static double CalcGeomMeanLongSun(double t)
{
    return NormalizeDeg(280.46646 + t * (36000.76983 + t * 0.0003032));
}

// Returns the mean anomaly of the sun in degrees.
static double CalcGeomMeanAnomalySun(double t)
{
    return NormalizeDeg(357.52911 + t * (35999.05029 - t * 0.0001537));
}

// Returns the eccentricity of Earth's orbit.
static double CalcEccentricityEarthOrbit(double t)
{
    return 0.016708634 - t * (0.000042037 + t * 0.0000001267);
}

// Returns the equation of center for the sun in degrees.
static double CalcSunEqOfCenter(double t)
{
    double m = DegToRad(CalcGeomMeanAnomalySun(t));
    return sin(m) * (1.914602 - t * (0.004817 + 0.000014 * t))
        + sin(2.0 * m) * (0.019993 - 0.000101 * t)
        + sin(3.0 * m) * 0.000289;
}

// Returns the true longitude of the sun in degrees.
static double CalcSunTrueLong(double t)
{
    return CalcGeomMeanLongSun(t) + CalcSunEqOfCenter(t);
}

// Returns the apparent longitude of the sun in degrees.
// Corrects for aberration and nutation.
static double CalcSunApparentLong(double t)
{
    double trueLong = CalcSunTrueLong(t);
    double omega = DegToRad(125.04 - 1934.136 * t);
    return trueLong - 0.00569 - 0.00478 * sin(omega);
}

// Returns the mean obliquity of the ecliptic in degrees.
static double CalcMeanObliquityOfEcliptic(double t)
{
    return 23.0 + (26.0 + (21.448 - t * (46.8150 + t * (0.00059 - t * 0.001813))) / 60.0) / 60.0;
}

// Returns the corrected obliquity of the ecliptic in degrees.
static double CalcObliquityCorrection(double t)
{
    double e0 = CalcMeanObliquityOfEcliptic(t);
    double omega = DegToRad(125.04 - 1934.136 * t);
    return e0 + 0.00256 * cos(omega);
}

// Returns the declination of the sun in degrees.
// Declination is the angle of the sun above/below the celestial equator.
static double CalcSunDeclination(double t)
{
    double e = DegToRad(CalcObliquityCorrection(t));
    double lambda = DegToRad(CalcSunApparentLong(t));
    return RadToDeg(asin(sin(e) * sin(lambda)));
}

// Returns the equation of time in minutes.
// The equation of time is the difference between apparent solar time
// and mean solar time.
static double CalcEquationOfTime(double t)
{
    double epsilon = DegToRad(CalcObliquityCorrection(t));
    double l0 = DegToRad(CalcGeomMeanLongSun(t));
    double e = CalcEccentricityEarthOrbit(t);
    double m = DegToRad(CalcGeomMeanAnomalySun(t));
    double y = tan(epsilon / 2.0);
    y *= y;

    double eqTime = y * sin(2.0 * l0)
        - 2.0 * e * sin(m)
        + 4.0 * e * y * sin(m) * cos(2.0 * l0)
        - 0.5 * y * y * sin(4.0 * l0)
        - 1.25 * e * e * sin(2.0 * m);

    return RadToDeg(eqTime) * 4.0; // convert to minutes
}

// =============================================================================
// HOUR ANGLE CALCULATION
// =============================================================================

// Returns the hour angle at sunrise/sunset for a given solar zenith angle.
// zenith: 90.833 for standard sunrise/sunset (accounts for refraction).
//         For degree-based zmanim: 90 + angle below horizon.
// latitude: observer's latitude in degrees.
// decl: sun's declination in degrees.
// Returns NaN if the sun never rises/sets (polar regions).
static double CalcHourAngle(double zenith, double latitude, double decl)
{
    double latRad = DegToRad(latitude);
    double declRad = DegToRad(decl);
    double zenRad = DegToRad(zenith);

    double cosHA = (cos(zenRad) - sin(latRad) * sin(declRad))
        / (cos(latRad) * cos(declRad));

    // cosHA outside [-1, 1] means sun never rises or never sets
    if (cosHA < -1.0 || cosHA > 1.0)
        return NAN;

    return RadToDeg(acos(cosHA));
}

// =============================================================================
// CORE SUNRISE / SUNSET ENGINE
// =============================================================================

// Internal function: calculates sunrise or sunset as fractional UTC hours.
// zenith: 90.833 for standard sunrise/sunset.
// isSunrise: true for sunrise, false for sunset.
// Returns -1.0 if sun doesn't rise/set.
static double CalcSunriseSunsetUTC(const GregorianDate& g,
    double latitude,
    double longitude,
    double zenith,
    bool   isSunrise)
{
    double jd = CalcJulianDate(g);
    double t = CalcJulianCentury(jd);

    // Solar noon UTC in minutes from midnight.
    // NOAA formula: 720 - 4*longitude - eqTime
    // longitude is east-positive / west-negative (New York = -73.9496)
    double eqTime = CalcEquationOfTime(t);
    double solarNoonUTC = 720.0 - 4.0 * longitude - eqTime;

    // Refine declination at solar noon
    double noonJD = jd + solarNoonUTC / 1440.0;
    double tNoon = CalcJulianCentury(noonJD);
    double decl = CalcSunDeclination(tNoon);

    double hourAngle = CalcHourAngle(zenith, latitude, decl);
    if (std::isnan(hourAngle)) return -1.0;

    // Sunrise = solarNoon - 4*HA
    // Sunset  = solarNoon + 4*HA
    // (multiply by 4 converts degrees of hour angle to minutes of time)
    double timeUTC = isSunrise
        ? solarNoonUTC - 4.0 * hourAngle
        : solarNoonUTC + 4.0 * hourAngle;

    // Second pass for accuracy using refined solar position
    double eventJD = jd + timeUTC / 1440.0;
    double tEvent = CalcJulianCentury(eventJD);
    double eqTime2 = CalcEquationOfTime(tEvent);
    double decl2 = CalcSunDeclination(tEvent);
    double ha2 = CalcHourAngle(zenith, latitude, decl2);
    if (std::isnan(ha2)) return -1.0;

    double noon2 = 720.0 - 4.0 * longitude - eqTime2;
    double timeUTC2 = isSunrise
        ? noon2 - 4.0 * ha2
        : noon2 + 4.0 * ha2;

    // Convert minutes to fractional hours
    return timeUTC2 / 60.0;
}

// Converts UTC fractional hours to local fractional hours,
// applying GMT offset and DST.
static double UTCToLocal(double utcHours, int gmtOffset, bool isDST)
{
    double local = utcHours + gmtOffset + (isDST ? 1.0 : 0.0);
    // Normalize to [0, 24)
    while (local < 0.0)  local += 24.0;
    while (local >= 24.0) local -= 24.0;
    return local;
}

// =============================================================================
// PUBLIC ASTRONOMICAL FUNCTIONS
// =============================================================================

// Returns sunrise as a TimeOfDay for a given date and location.
TimeOfDay CalculateSunrise(const GregorianDate& date,
    const Location& loc,
    bool                 isDST)
{
    // Standard zenith 90.833 = 90deg + 50' for atmospheric refraction
    double utc = CalcSunriseSunsetUTC(date, loc.latitude, loc.longitude,
        90.833, true);
    if (utc < 0) return TimeOfDay(); // invalid (polar)

    // Elevation correction: about 0.0347 degrees per meter of elevation
    // This shifts sunrise earlier for elevated observers.
    if (loc.elevation > 0)
    {
        double elevCorr = 0.0347 * sqrt(loc.elevation) / 60.0; // hours
        utc -= elevCorr;
    }

    return FractionalHourToTime(UTCToLocal(utc, loc.gmtOffset, isDST));
}

// Returns sunset as a TimeOfDay for a given date and location.
TimeOfDay CalculateSunset(const GregorianDate& date,
    const Location& loc,
    bool                 isDST)
{
    double utc = CalcSunriseSunsetUTC(date, loc.latitude, loc.longitude,
        90.833, false);
    if (utc < 0) return TimeOfDay();

    if (loc.elevation > 0)
    {
        double elevCorr = 0.0347 * sqrt(loc.elevation) / 60.0;
        utc += elevCorr;
    }

    return FractionalHourToTime(UTCToLocal(utc, loc.gmtOffset, isDST));
}

// Returns the time when the sun is at a given angle below the horizon.
// angle: degrees below horizon (positive number, e.g. 8.5 for tzeit GRA)
// isMorning: true = before sunrise (alot), false = after sunset (tzeit)
TimeOfDay CalculateSunAtAngle(const GregorianDate& date,
    const Location& loc,
    bool                 isDST,
    double               angle,
    bool                 isMorning)
{
    // zenith = 90 + angle (angle below horizon)
    double zenith = 90.0 + angle;
    double utc = CalcSunriseSunsetUTC(date, loc.latitude, loc.longitude,
        zenith, isMorning);
    if (utc < 0) return TimeOfDay();

    return FractionalHourToTime(UTCToLocal(utc, loc.gmtOffset, isDST));
}

// =============================================================================
// TIME ARITHMETIC HELPERS
// =============================================================================

// Adds a fixed number of minutes to a TimeOfDay.
TimeOfDay AddMinutes(const TimeOfDay& t, int minutes)
{
    if (!t.IsValid()) return TimeOfDay();

    int totalSeconds = t.hour * 3600 + t.minute * 60 + t.second
        + minutes * 60;

    if (totalSeconds < 0 || totalSeconds >= 86400)
        return TimeOfDay(); // out of range

    int h = totalSeconds / 3600;
    int m = (totalSeconds % 3600) / 60;
    int s = totalSeconds % 60;

    return TimeOfDay(h, m, s);
}

// Returns the length of a halachic hour in minutes.
// dayStart and dayEnd define the boundaries of the halachic day.
double CalculateShaahZmanit(const TimeOfDay& dayStart, const TimeOfDay& dayEnd)
{
    if (!dayStart.IsValid() || !dayEnd.IsValid()) return 60.0; // fallback

    double start = TimeToFractionalHour(dayStart);
    double end = TimeToFractionalHour(dayEnd);

    return (end - start) * 60.0 / 12.0; // total minutes / 12 hours
}

// Adds a number of halachic hours (shaot) to a base time.
// base: the start of the halachic day (sunrise or alot)
// shaahMinutes: length of one halachic hour in minutes
// numShaot: how many halachic hours to add (can be fractional, e.g. 3.5)
TimeOfDay AddShaot(const TimeOfDay& base,
    double           shaahMinutes,
    double           numShaot)
{
    if (!base.IsValid()) return TimeOfDay();

    int totalSeconds = base.hour * 3600
        + base.minute * 60
        + base.second
        + (int)(shaahMinutes * numShaot * 60.0);

    if (totalSeconds < 0 || totalSeconds >= 86400)
        return TimeOfDay();

    int h = totalSeconds / 3600;
    int m = (totalSeconds % 3600) / 60;
    int s = totalSeconds % 60;

    return TimeOfDay(h, m, s);
}

// =============================================================================
// DST DETECTION (US RULES)
// =============================================================================

// Returns the day of week for a Gregorian date (0=Sun .. 6=Sat).
// Used internally for DST calculation.
static int DayOfWeekInt(int y, int m, int d)
{
    // Tomohiko Sakamoto's algorithm
    static const int t[] = { 0,3,2,5,0,3,5,1,4,6,2,4 };
    if (m < 3) y--;
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

// Returns the Gregorian date of the Nth occurrence of a weekday in a month.
// e.g. getNthWeekday(2026, 3, 0, 2) = 2nd Sunday in March 2026
static int GetNthWeekday(int year, int month, int weekday, int n)
{
    // Find first occurrence of weekday in month
    int firstDay = DayOfWeekInt(year, month, 1);
    int offset = (weekday - firstDay + 7) % 7;
    return 1 + offset + (n - 1) * 7;
}

// Returns the last occurrence of a weekday in a month.
static int GetLastWeekday(int year, int month, int weekday)
{
    // Days in month
    int dim = (month == 2) ? (((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) ? 29 : 28)
        : (month <= 7) ? (month % 2 == 0 ? 30 : 31)
        : (month % 2 == 0 ? 31 : 30);

    int lastDay = DayOfWeekInt(year, month, dim);
    int offset = (lastDay - weekday + 7) % 7;
    return dim - offset;
}

// Returns true if the given date falls within US DST.
// US DST: 2nd Sunday in March (2:00 AM) to 1st Sunday in November (2:00 AM).
// (Pre-2007 rules not implemented; 2007+ rules used.)
bool IsDST(const GregorianDate& date, const Location& loc)
{
    if (!loc.usesDST) return false;

    int y = date.year;
    int m = date.month;
    int d = date.day;

    // DST start: 2nd Sunday in March
    int dstStart = GetNthWeekday(y, 3, 0, 2);

    // DST end: 1st Sunday in November
    int dstEnd = GetNthWeekday(y, 11, 0, 1);

    if (m > 3 && m < 11)  return true;
    if (m == 3 && d > dstStart) return true;
    if (m == 3 && d == dstStart) return true; // simplified: after midnight
    if (m == 11 && d < dstEnd)  return true;

    return false;
}

// =============================================================================
// STRING FORMATTING
// =============================================================================

// Formats a TimeOfDay as a display string.
std::wstring FormatTime(const TimeOfDay& t, bool use24Hour)
{
    if (!t.IsValid()) return L"--:--";

    wchar_t buf[32];

    if (use24Hour)
    {
        swprintf_s(buf, L"%02d:%02d", t.hour, t.minute);
    }
    else
    {
        int displayHour = t.hour % 12;
        if (displayHour == 0) displayHour = 12;
        const wchar_t* ampm = (t.hour < 12) ? L"AM" : L"PM";
        swprintf_s(buf, L"%d:%02d %s", displayHour, t.minute, ampm);
    }

    return std::wstring(buf);
}

// =============================================================================
// MAIN ZMANIM CALCULATOR
// =============================================================================

// Computes all halachic times for a given date and location.
// This is the single entry point called by the UI layer.
ZmanimResult CalculateZmanim(const GregorianDate& date,
    const Location& loc,
    bool                 isDST)
{
    ZmanimResult z;

    // --- Universal times ---

    // Sunrise and sunset (standard, degree-based)
    z.hanetz = CalculateSunrise(date, loc, isDST);
    z.shkia = CalculateSunset(date, loc, isDST);

    // Chatzot: midpoint between sunrise and sunset
    if (z.hanetz.IsValid() && z.shkia.IsValid())
    {
        double rise = TimeToFractionalHour(z.hanetz);
        double set = TimeToFractionalHour(z.shkia);
        z.chatzot = FractionalHourToTime((rise + set) / 2.0);
        z.chatzotLayla = FractionalHourToTime(fmod((rise + set) / 2.0 + 12.0, 24.0));
    }

    // Candle lighting: 18 minutes before sunset
    z.candleLighting = AddMinutes(z.shkia, -18);

    // Daylight hours: store as hours in the hour field, minutes in minute
    if (z.hanetz.IsValid() && z.shkia.IsValid())
    {
        double dayLen = TimeToFractionalHour(z.shkia)
            - TimeToFractionalHour(z.hanetz);
        z.daylightHours = FractionalHourToTime(dayLen);
    }

    // --- Shaah Zmanit (halachic hour lengths) ---

    // GRA: sunrise to sunset
    z.shaahZmanit_GRA = CalculateShaahZmanit(z.hanetz, z.shkia);

    // --- Alot HaShachar (dawn) ---

    // GRA / standard: 16.1 degrees before sunrise
    z.alot_GRA = CalculateSunAtAngle(date, loc, isDST, 16.1, true);

    // MA 72: 72 fixed minutes before sunrise
    z.alot_MA72 = AddMinutes(z.hanetz, -72);

    // MA 90: 90 fixed minutes before sunrise
    z.alot_MA90 = AddMinutes(z.hanetz, -90);

    // MA 72 proportional: 72 min as fraction of shaah zmanit MA 72
    // MA 72P shaah = (shkia+72 - (hanetz-72)) / 12 = shaah + 12 min
    {
        double shaahMA72 = CalculateShaahZmanit(
            AddMinutes(z.hanetz, -72), AddMinutes(z.shkia, 72));
        z.shaahZmanit_MA72 = shaahMA72;
        // Alot = hanetz - 1 shaah zmanit MA72 proportional
        z.alot_MA72P = AddShaot(z.hanetz, shaahMA72, -1.0);
    }

    // MA 90 proportional
    {
        double shaahMA90 = CalculateShaahZmanit(
            AddMinutes(z.hanetz, -90), AddMinutes(z.shkia, 90));
        z.shaahZmanit_MA90 = shaahMA90;
        z.alot_MA90P = AddShaot(z.hanetz, shaahMA90, -1.0);
    }

    // --- Misheyakir ---

    // 11.5 degrees before sunrise
    z.misheyakir_11 = CalculateSunAtAngle(date, loc, isDST, 11.5, true);

    // 10.2 degrees before sunrise
    z.misheyakir_10 = CalculateSunAtAngle(date, loc, isDST, 10.2, true);

    // --- Sof Zman Shema (end of 3rd halachic hour) ---

    // GRA: 3 shaot from sunrise
    z.sofShema_GRA = AddShaot(z.hanetz, z.shaahZmanit_GRA, 3.0);

    // MA 72: 3 shaot from alot MA72
    z.sofShema_MA72 = AddShaot(z.alot_MA72, z.shaahZmanit_MA72, 3.0);

    // MA 90: 3 shaot from alot MA90
    z.sofShema_MA90 = AddShaot(z.alot_MA90, z.shaahZmanit_MA90, 3.0);

    // --- Sof Zman Tefilla (end of 4th halachic hour) ---

    z.sofTefilla_GRA = AddShaot(z.hanetz, z.shaahZmanit_GRA, 4.0);
    z.sofTefilla_MA72 = AddShaot(z.alot_MA72, z.shaahZmanit_MA72, 4.0);
    z.sofTefilla_MA90 = AddShaot(z.alot_MA90, z.shaahZmanit_MA90, 4.0);

    // --- Mincha Gedola (6.5 halachic hours after start of day) ---

    z.minchaGedola_GRA = AddShaot(z.hanetz, z.shaahZmanit_GRA, 6.5);
    z.minchaGedola_MA72 = AddShaot(z.alot_MA72, z.shaahZmanit_MA72, 6.5);
    z.minchaGedola_MA90 = AddShaot(z.alot_MA90, z.shaahZmanit_MA90, 6.5);

    // --- Mincha Ketana (9.5 halachic hours after start of day) ---

    z.minchaKetana_GRA = AddShaot(z.hanetz, z.shaahZmanit_GRA, 9.5);
    z.minchaKetana_MA72 = AddShaot(z.alot_MA72, z.shaahZmanit_MA72, 9.5);
    z.minchaKetana_MA90 = AddShaot(z.alot_MA90, z.shaahZmanit_MA90, 9.5);

    // --- Plag HaMincha (10.75 halachic hours after start of day) ---

    z.plagMincha_GRA = AddShaot(z.hanetz, z.shaahZmanit_GRA, 10.75);
    z.plagMincha_MA72 = AddShaot(z.alot_MA72, z.shaahZmanit_MA72, 10.75);
    z.plagMincha_MA90 = AddShaot(z.alot_MA90, z.shaahZmanit_MA90, 10.75);

    // --- Tzeit HaKochavim (nightfall) ---

    // GRA: 8.5 degrees below horizon
    z.tzeit_GRA = CalculateSunAtAngle(date, loc, isDST, 8.5, false);

    // MA 72: 72 fixed minutes after sunset
    z.tzeit_MA72 = AddMinutes(z.shkia, 72);

    // MA 90: 90 fixed minutes after sunset
    z.tzeit_MA90 = AddMinutes(z.shkia, 90);

    // MA 72 proportional: 1 shaah zmanit MA72 after sunset
    z.tzeit_MA72P = AddShaot(z.shkia, z.shaahZmanit_MA72, 1.0);

    // MA 90 proportional: 1 shaah zmanit MA90 after sunset
    z.tzeit_MA90P = AddShaot(z.shkia, z.shaahZmanit_MA90, 1.0);

    // --- Tzeit Shabbat ---
    // Default: same as tzeit GRA (UI layer adds tosefet if configured)
    z.tzeitShabbat = z.tzeit_GRA;

    return z;
}