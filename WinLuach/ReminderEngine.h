// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    ReminderEngine.h
// Purpose: Advanced reminder firing engine.
//          Provides helpers to compute when a ReminderRule should next fire
//          and to match reminder targets to HolidayEngine records.
// =============================================================================
//
// CHANGELOG:
// v0.8.71 - Initial implementation.
//           ParseReminderOffset      - parses "15 minutes" → (15, unit)
//           MatchesReminderTarget    - maps target strings to HolidayInfo
//           FindNextHolidayDate      - scans forward for next holiday occurrence
//           ZmanTimeForTarget        - maps target name to ZmanimResult/DisplayZmanimTimes
//           ComputeReminderFireTime  - top-level: returns next CTime to fire, or CTime(0)
// =============================================================================

#pragma once
#include "Settings.h"
#include "HolidayEngine.h"
#include "Zmanim.h"
#include "Location.h"
#include <atltime.h>

// DisplayZmanimTimes is defined in Zmanim.h (included above)

// ---------------------------------------------------------------------------
// ParseReminderOffset
// Splits a rule's offsets string (e.g. "15 minutes") into an integer amount
// and a unit string ("minutes", "hours", "days", "weeks", "months", "years").
// Returns false if the string is empty or unparseable (amount stays 0).
// ---------------------------------------------------------------------------
bool ParseReminderOffset(const std::wstring& offsets, int& outAmount, std::wstring& outUnit);

// ---------------------------------------------------------------------------
// MatchesReminderTarget
// Returns true if the HolidayInfo record corresponds to the reminder target
// string selected in the UI (e.g. "Rosh Chodesh (Day 1)", "Chol HaMoed Pesach").
// ---------------------------------------------------------------------------
bool MatchesReminderTarget(const HolidayInfo& ho, const std::wstring& target);

// ---------------------------------------------------------------------------
// FindNextHolidayDate
// Scans forward from today up to maxDays days to find the first civil date
// on which the target holiday falls.  Returns {0,0,0} if not found.
// "Civil date" here is the Hebrew date's corresponding Gregorian day (the
// morning side), so Erev Rosh Hashana (tzais of JDN n) lives at JDN n.
// ---------------------------------------------------------------------------
GregorianDate FindNextHolidayDate(const std::wstring& target,
                                  const GregorianDate& from,
                                  bool isIsrael,
                                  int maxDays = 366);

// ---------------------------------------------------------------------------
// ZmanTimeForTarget
// Maps a Zman target string (e.g. "Shkiah", "Tzeis (GRA 8.5 deg)") to a
// TimeOfDay from either z (standard zmanim) or dz (custom shita zmanim).
// Returns an invalid TimeOfDay{-1,-1,-1} if the target is not found.
// ---------------------------------------------------------------------------
TimeOfDay ZmanTimeForTarget(const std::wstring& target,
                            const ZmanimResult& z,
                            const DisplayZmanimTimes& dz);

// ---------------------------------------------------------------------------
// ComputeReminderFireTime
// Top-level function called by CMainFrame::CheckAdvancedReminders() every
// minute.  Returns the CTime this rule should fire, or CTime(0) if it is not
// applicable within the next ~25 hours (today + tomorrow window).
//
// Anchor values:  0 = custom Tzeit (user preference)
//                 1 = Shkia
//                 2 = Civil midnight (00:00 of the holiday's civil date)
//
// Offset units:
//   minutes / hours  → subtract (or add for afterEvent) from the anchor time
//   days / weeks /   → fire at AppSettings::reminderDailyHour:Minute on the
//   months / years     civil date N units before (or after) the anchor date
// ---------------------------------------------------------------------------
CTime ComputeReminderFireTime(const ReminderRule&       rule,
                              const AppSettings&        settings,
                              bool                      isIsrael,
                              const Location&           location,
                              const ZmanimResult&       todayZ,
                              const DisplayZmanimTimes& todayDz,
                              const GregorianDate&      today);
