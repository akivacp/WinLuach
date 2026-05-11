// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    Location.h
// Purpose: Declares the location database and custom location management.
//          Provides a built-in list of world cities with coordinates,
//          timezone, and DST data. Also handles user-defined locations
//          saved to and loaded from a JSON config file.
//          No UI dependencies. Pure data layer.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial file. Declares LocationDB class, built-in city list,
//          and custom location save/load functions.
//          Ships with ~120 built-in cities across all continents.
//          Custom locations stored in locations.json in app data folder.
// =============================================================================
//
// VERSION GUIDE:
// Bump minor (v0.x.0) when new regions or features are added.
// Bump patch (v0.x.x) for coordinate corrections or new cities.
//
// STRUCTURE:
//   LocationDB         — the main database class
//   LocationDB::Get()  — singleton access
//   GetBuiltInCities() — returns the full built-in city list
//   LoadCustomLocations() / SaveCustomLocations() — persistence
//
// =============================================================================

#pragma once
#include "Zmanim.h"
#include <vector>
#include <string>

// =============================================================================
// LOCATION ENTRY
// Extended version of Location (from Zmanim.h) with display metadata.
// =============================================================================

struct LocationEntry
{
    Location     loc;          // Core location data (lat/lon/tz/dst)
    std::wstring country;      // Country name e.g. L"United States"
    std::wstring region;       // State/province e.g. L"New York"
    bool         isCustom  = false;  // True if user-defined, false if built-in
    bool         isIsrael  = false;  // hint: this location uses Israel schedule

    LocationEntry() : isCustom(false), isIsrael(false) {}

    LocationEntry(const Location& l,
        const std::wstring& country_,
        const std::wstring& region_,
        bool                custom = false)
        : loc(l), country(country_), region(region_), isCustom(custom), isIsrael(false) {
    }
};

// =============================================================================
// LOCATION DATABASE
// =============================================================================

class LocationDB
{
public:
    // -------------------------------------------------------------------------
    // Singleton access.
    // Call LocationDB::Get() to get the one instance of the database.
    // -------------------------------------------------------------------------
    static LocationDB& Get();

    // -------------------------------------------------------------------------
    // Returns all locations (built-in + custom) sorted by name.
    // -------------------------------------------------------------------------
    const std::vector<LocationEntry>& GetAll() const;

    // -------------------------------------------------------------------------
    // Returns only built-in locations.
    // -------------------------------------------------------------------------
    std::vector<LocationEntry> GetBuiltIn() const;

    // -------------------------------------------------------------------------
    // Returns only user-defined custom locations.
    // -------------------------------------------------------------------------
    std::vector<LocationEntry> GetCustom() const;

    // -------------------------------------------------------------------------
    // Finds a location by name (case-insensitive).
    // Returns nullptr if not found.
    // -------------------------------------------------------------------------
    const LocationEntry* FindByName(const std::wstring& name) const;

    // -------------------------------------------------------------------------
    // Adds a new custom location to the database and saves to disk.
    // Returns false if a location with the same name already exists.
    // -------------------------------------------------------------------------
    bool AddCustom(const LocationEntry& entry);

    // -------------------------------------------------------------------------
    // Updates an existing custom location by name.
    // Returns false if not found or if it is a built-in location.
    // -------------------------------------------------------------------------
    bool UpdateCustom(const std::wstring& name, const LocationEntry& updated);

    // -------------------------------------------------------------------------
    // Deletes a custom location by name.
    // Returns false if not found or if it is a built-in location.
    // -------------------------------------------------------------------------
    bool DeleteCustom(const std::wstring& name);

    // -------------------------------------------------------------------------
    // Loads custom locations from locations.json in the app data folder.
    // Called automatically on first access.
    // -------------------------------------------------------------------------
    bool LoadCustomLocations(const std::wstring& filePath);

    // -------------------------------------------------------------------------
    // Saves all custom locations to locations.json.
    // Called automatically after any add/update/delete.
    // -------------------------------------------------------------------------
    bool SaveCustomLocations(const std::wstring& filePath) const;

    // -------------------------------------------------------------------------
    // Returns the path to the locations.json file.
    // Stored in %APPDATA%\WinLuach\locations.json
    // -------------------------------------------------------------------------
    static std::wstring GetLocationsFilePath();

    // -------------------------------------------------------------------------
    // Returns the default location (New York City).
    // Used on first run before the user has chosen a location.
    // -------------------------------------------------------------------------
    static LocationEntry GetDefaultLocation();

private:
    // Private constructor — use Get() for access
    LocationDB();

    // Loads the built-in city list into m_entries
    void LoadBuiltIn();

    // All locations: built-in first, then custom
    std::vector<LocationEntry> m_entries;

    // Path to the custom locations file
    std::wstring m_filePath;
};

// =============================================================================
// FREE FUNCTIONS
// =============================================================================

// Returns the full built-in city list.
// Called by LocationDB constructor — can also be used directly for testing.
std::vector<LocationEntry> GetBuiltInCities();

// Converts a LocationEntry to a single-line display string.
// e.g. "New York City, New York, United States"
std::wstring LocationDisplayString(const LocationEntry& entry);