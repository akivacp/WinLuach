// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    Location.cpp
// Purpose: Full implementation of the location database.
//          Built-in city list covers major Jewish communities worldwide.
//          Custom locations saved/loaded as simple JSON in %APPDATA%\WinLuach.
//          No UI dependencies. Pure data layer.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial implementation. ~120 built-in cities across all continents.
//          Custom location persistence via simple hand-rolled JSON
//          (no external library dependency).
//          Singleton LocationDB with add/update/delete/save/load.
// =============================================================================

#include "pch.h"
#include "Location.h"
#include <fstream>
#include <sstream>
#include <ShlObj.h>

// =============================================================================
// BUILT-IN CITY DATABASE
// =============================================================================
// Format: Location(name, lat, lon, elevation, gmtOffset, usesDST), country, region
//
// Latitude:  N = positive, S = negative
// Longitude: E = positive, W = negative
// Elevation: meters above sea level
// gmtOffset: standard time offset from UTC (not including DST)
// usesDST:   true if the location observes daylight saving time
// =============================================================================

std::vector<LocationEntry> GetBuiltInCities()
{
    std::vector<LocationEntry> cities;

    // -------------------------------------------------------------------------
    // UNITED STATES
    // -------------------------------------------------------------------------
    cities.push_back({ Location(L"New York City",    40.6501, -73.9496,  10, -5, true),  L"United States", L"New York" });
    cities.push_back({ Location(L"Brooklyn",         40.6501, -73.9496,  10, -5, true),  L"United States", L"New York" });
    cities.push_back({ Location(L"Manhattan",        40.7831, -73.9712,  10, -5, true),  L"United States", L"New York" });
    cities.push_back({ Location(L"Monsey",           41.1110, -74.0685,  91, -5, true),  L"United States", L"New York" });
    cities.push_back({ Location(L"Lakewood",         40.0957, -74.2177,  27, -5, true),  L"United States", L"New Jersey" });
    cities.push_back({ Location(L"Teaneck",          40.8926, -74.0132,  15, -5, true),  L"United States", L"New Jersey" });
    cities.push_back({ Location(L"Five Towns",       40.6220, -73.7270,   5, -5, true),  L"United States", L"New York" });
    cities.push_back({ Location(L"Baltimore",        39.2904, -76.6122,  35, -5, true),  L"United States", L"Maryland" });
    cities.push_back({ Location(L"Silver Spring",    38.9907, -77.0261,  90, -5, true),  L"United States", L"Maryland" });
    cities.push_back({ Location(L"Washington DC",    38.9072, -77.0369,  11, -5, true),  L"United States", L"DC" });
    cities.push_back({ Location(L"Philadelphia",     39.9526, -75.1652,  12, -5, true),  L"United States", L"Pennsylvania" });
    cities.push_back({ Location(L"Pittsburgh",       40.4406, -79.9959, 230, -5, true),  L"United States", L"Pennsylvania" });
    cities.push_back({ Location(L"Boston",           42.3601, -71.0589,  10, -5, true),  L"United States", L"Massachusetts" });
    cities.push_back({ Location(L"Providence",       41.8240, -71.4128,  18, -5, true),  L"United States", L"Rhode Island" });
    cities.push_back({ Location(L"Hartford",         41.7637, -72.6851,  18, -5, true),  L"United States", L"Connecticut" });
    cities.push_back({ Location(L"Miami",            25.7617, -80.1918,   2, -5, true),  L"United States", L"Florida" });
    cities.push_back({ Location(L"Boca Raton",       26.3683, -80.1289,   4, -5, true),  L"United States", L"Florida" });
    cities.push_back({ Location(L"Orlando",          28.5383, -81.3792,  22, -5, true),  L"United States", L"Florida" });
    cities.push_back({ Location(L"Atlanta",          33.7490, -84.3880, 320, -5, true),  L"United States", L"Georgia" });
    cities.push_back({ Location(L"Chicago",          41.8781, -87.6298, 179, -6, true),  L"United States", L"Illinois" });
    cities.push_back({ Location(L"Detroit",          42.3314, -83.0458, 183, -5, true),  L"United States", L"Michigan" });
    cities.push_back({ Location(L"Cleveland",        41.4993, -81.6944, 201, -5, true),  L"United States", L"Ohio" });
    cities.push_back({ Location(L"Columbus",         39.9612, -82.9988, 244, -5, true),  L"United States", L"Ohio" });
    cities.push_back({ Location(L"Minneapolis",      44.9778, -93.2650, 260, -6, true),  L"United States", L"Minnesota" });
    cities.push_back({ Location(L"St Louis",         38.6270, -90.1994, 142, -6, true),  L"United States", L"Missouri" });
    cities.push_back({ Location(L"Kansas City",      39.0997, -94.5786, 320, -6, true),  L"United States", L"Missouri" });
    cities.push_back({ Location(L"Dallas",           32.7767, -96.7970, 139, -6, true),  L"United States", L"Texas" });
    cities.push_back({ Location(L"Houston",          29.7604, -95.3698,  13, -6, true),  L"United States", L"Texas" });
    cities.push_back({ Location(L"San Antonio",      29.4241, -98.4936, 198, -6, true),  L"United States", L"Texas" });
    cities.push_back({ Location(L"Denver",           39.7392,-104.9903,1609, -7, true),  L"United States", L"Colorado" });
    cities.push_back({ Location(L"Phoenix",          33.4484,-112.0740, 331, -7, false), L"United States", L"Arizona" });
    cities.push_back({ Location(L"Las Vegas",        36.1699,-115.1398, 620, -8, true),  L"United States", L"Nevada" });
    cities.push_back({ Location(L"Los Angeles",      34.0522,-118.2437,  71, -8, true),  L"United States", L"California" });
    cities.push_back({ Location(L"San Francisco",    37.7749,-122.4194,  16, -8, true),  L"United States", L"California" });
    cities.push_back({ Location(L"San Diego",        32.7157,-117.1611,  18, -8, true),  L"United States", L"California" });
    cities.push_back({ Location(L"Seattle",          47.6062,-122.3321,  56, -8, true),  L"United States", L"Washington" });
    cities.push_back({ Location(L"Portland",         45.5051,-122.6750,  15, -8, true),  L"United States", L"Oregon" });
    cities.push_back({ Location(L"Honolulu",         21.3069,-157.8583,   4,-10, false), L"United States", L"Hawaii" });
    cities.push_back({ Location(L"Anchorage",        61.2181,-149.9003,  30, -9, true),  L"United States", L"Alaska" });

    // -------------------------------------------------------------------------
    // CANADA
    // -------------------------------------------------------------------------
    cities.push_back({ Location(L"Toronto",          43.6532, -79.3832,  76, -5, true),  L"Canada", L"Ontario" });
    cities.push_back({ Location(L"Montreal",         45.5017, -73.5673,  27, -5, true),  L"Canada", L"Quebec" });
    cities.push_back({ Location(L"Ottawa",           45.4215, -75.6972,  70, -5, true),  L"Canada", L"Ontario" });
    cities.push_back({ Location(L"Vancouver",        49.2827,-123.1207,  70, -8, true),  L"Canada", L"British Columbia" });
    cities.push_back({ Location(L"Calgary",          51.0447,-114.0719,1045, -7, true),  L"Canada", L"Alberta" });
    cities.push_back({ Location(L"Winnipeg",         49.8951, -97.1384, 232, -6, true),  L"Canada", L"Manitoba" });

    // -------------------------------------------------------------------------
    // ISRAEL
    // -------------------------------------------------------------------------
    cities.push_back({ Location(L"Jerusalem",        31.7683,  35.2137, 754,  2, true),  L"Israel", L"Jerusalem" });
    cities.push_back({ Location(L"Tel Aviv",         32.0853,  34.7818,   5,  2, true),  L"Israel", L"Tel Aviv" });
    cities.push_back({ Location(L"Bnei Brak",        32.0840,  34.8338,  22,  2, true),  L"Israel", L"Tel Aviv" });
    cities.push_back({ Location(L"Haifa",            32.7940,  34.9896,   5,  2, true),  L"Israel", L"Haifa" });
    cities.push_back({ Location(L"Beer Sheva",       31.2520,  34.7915, 280,  2, true),  L"Israel", L"South" });
    cities.push_back({ Location(L"Ashdod",           31.8044,  34.6553,  30,  2, true),  L"Israel", L"South" });
    cities.push_back({ Location(L"Netanya",          32.3215,  34.8532,  25,  2, true),  L"Israel", L"Center" });
    cities.push_back({ Location(L"Petah Tikva",      32.0868,  34.8878,  50,  2, true),  L"Israel", L"Tel Aviv" });
    cities.push_back({ Location(L"Rehovot",          31.8928,  34.8113,  54,  2, true),  L"Israel", L"Center" });
    cities.push_back({ Location(L"Ramat Gan",        32.0698,  34.8239,  50,  2, true),  L"Israel", L"Tel Aviv" });
    cities.push_back({ Location(L"Tzfat",            32.9646,  35.4960, 860,  2, true),  L"Israel", L"North" });
    cities.push_back({ Location(L"Tiberias",         32.7957,  35.5310, -210, 2, true),  L"Israel", L"North" });
    cities.push_back({ Location(L"Eilat",            29.5577,  34.9519,  18,  2, true),  L"Israel", L"South" });
    cities.push_back({ Location(L"Modiin",           31.8969,  35.0095, 280,  2, true),  L"Israel", L"Center" });
    cities.push_back({ Location(L"Raanana",          32.1836,  34.8700,  48,  2, true),  L"Israel", L"Center" });

    // -------------------------------------------------------------------------
    // UNITED KINGDOM
    // -------------------------------------------------------------------------
    cities.push_back({ Location(L"London",           51.5074,  -0.1278,  11,  0, true),  L"United Kingdom", L"England" });
    cities.push_back({ Location(L"Manchester",       53.4808,  -2.2426,  78,  0, true),  L"United Kingdom", L"England" });
    cities.push_back({ Location(L"Leeds",            53.8008,  -1.5491,  36,  0, true),  L"United Kingdom", L"England" });
    cities.push_back({ Location(L"Birmingham",       52.4862,  -1.8904, 150,  0, true),  L"United Kingdom", L"England" });
    cities.push_back({ Location(L"Glasgow",          55.8642,  -4.2518,  35,  0, true),  L"United Kingdom", L"Scotland" });

    // -------------------------------------------------------------------------
    // EUROPE
    // -------------------------------------------------------------------------
    cities.push_back({ Location(L"Paris",            48.8566,   2.3522,  35,  1, true),  L"France",      L"Ile-de-France" });
    cities.push_back({ Location(L"Strasbourg",       48.5734,   7.7521, 143,  1, true),  L"France",      L"Alsace" });
    cities.push_back({ Location(L"Lyon",             45.7640,   4.8357, 173,  1, true),  L"France",      L"Auvergne" });
    cities.push_back({ Location(L"Marseille",        43.2965,   5.3698,  10,  1, true),  L"France",      L"Provence" });
    cities.push_back({ Location(L"Berlin",           52.5200,  13.4050,  34,  1, true),  L"Germany",     L"Berlin" });
    cities.push_back({ Location(L"Frankfurt",        50.1109,   8.6821, 112,  1, true),  L"Germany",     L"Hesse" });
    cities.push_back({ Location(L"Munich",           48.1351,  11.5820, 519,  1, true),  L"Germany",     L"Bavaria" });
    cities.push_back({ Location(L"Amsterdam",        52.3676,   4.9041,   2,  1, true),  L"Netherlands", L"North Holland" });
    cities.push_back({ Location(L"Antwerp",          51.2194,   4.4025,  10,  1, true),  L"Belgium",     L"Antwerp" });
    cities.push_back({ Location(L"Brussels",         50.8503,   4.3517,  56,  1, true),  L"Belgium",     L"Brussels" });
    cities.push_back({ Location(L"Zurich",           47.3769,   8.5417, 408,  1, true),  L"Switzerland", L"Zurich" });
    cities.push_back({ Location(L"Geneva",           46.2044,   6.1432, 375,  1, true),  L"Switzerland", L"Geneva" });
    cities.push_back({ Location(L"Vienna",           48.2082,  16.3738, 171,  1, true),  L"Austria",     L"Vienna" });
    cities.push_back({ Location(L"Rome",             41.9028,  12.4964,  21,  1, true),  L"Italy",       L"Lazio" });
    cities.push_back({ Location(L"Milan",            45.4654,   9.1859, 122,  1, true),  L"Italy",       L"Lombardy" });
    cities.push_back({ Location(L"Madrid",           40.4168,  -3.7038, 657,  1, true),  L"Spain",       L"Madrid" });
    cities.push_back({ Location(L"Barcelona",        41.3851,   2.1734,  12,  1, true),  L"Spain",       L"Catalonia" });
    cities.push_back({ Location(L"Lisbon",           38.7223,  -9.1393,  45,  0, true),  L"Portugal",    L"Lisbon" });
    cities.push_back({ Location(L"Stockholm",        59.3293,  18.0686,  28,  1, true),  L"Sweden",      L"Stockholm" });
    cities.push_back({ Location(L"Copenhagen",       55.6761,  12.5683,  10,  1, true),  L"Denmark",     L"Capital" });
    cities.push_back({ Location(L"Oslo",             59.9139,  10.7522,  23,  1, true),  L"Norway",      L"Oslo" });
    cities.push_back({ Location(L"Helsinki",         60.1699,  24.9384,  26,  2, true),  L"Finland",     L"Uusimaa" });
    cities.push_back({ Location(L"Warsaw",           52.2297,  21.0122, 113,  1, true),  L"Poland",      L"Masovia" });
    cities.push_back({ Location(L"Krakow",           50.0647,  19.9450, 219,  1, true),  L"Poland",      L"Lesser Poland" });
    cities.push_back({ Location(L"Budapest",         47.4979,  19.0402, 102,  1, true),  L"Hungary",     L"Budapest" });
    cities.push_back({ Location(L"Prague",           50.0755,  14.4378, 200,  1, true),  L"Czech Republic", L"Prague" });
    cities.push_back({ Location(L"Bratislava",       48.1486,  17.1077, 134,  1, true),  L"Slovakia",    L"Bratislava" });
    cities.push_back({ Location(L"Bucharest",        44.4268,  26.1025,  83,  2, true),  L"Romania",     L"Bucharest" });
    cities.push_back({ Location(L"Athens",           37.9838,  23.7275, 100,  2, true),  L"Greece",      L"Attica" });
    cities.push_back({ Location(L"Istanbul",         41.0082,  28.9784,  40,  3, false), L"Turkey",      L"Istanbul" });

    // -------------------------------------------------------------------------
    // FORMER SOVIET UNION
    // -------------------------------------------------------------------------
    cities.push_back({ Location(L"Moscow",           55.7558,  37.6173, 156,  3, false), L"Russia",      L"Moscow" });
    cities.push_back({ Location(L"St Petersburg",    59.9311,  30.3609,   5,  3, false), L"Russia",      L"Leningrad" });
    cities.push_back({ Location(L"Kiev",             50.4501,  30.5234, 179,  2, true),  L"Ukraine",     L"Kiev" });
    cities.push_back({ Location(L"Odessa",           46.4825,  30.7233,  50,  2, true),  L"Ukraine",     L"Odessa" });
    cities.push_back({ Location(L"Minsk",            53.9045,  27.5615, 220,  3, false), L"Belarus",     L"Minsk" });
    cities.push_back({ Location(L"Riga",             56.9496,  24.1052,   5,  2, true),  L"Latvia",      L"Riga" });
    cities.push_back({ Location(L"Vilnius",          54.6872,  25.2797, 112,  2, true),  L"Lithuania",   L"Vilnius" });

    // -------------------------------------------------------------------------
    // SOUTH AMERICA
    // -------------------------------------------------------------------------
    cities.push_back({ Location(L"Buenos Aires",    -34.6037, -58.3816,  25, -3, false), L"Argentina",   L"Buenos Aires" });
    cities.push_back({ Location(L"Sao Paulo",       -23.5505, -46.6333, 760, -3, true),  L"Brazil",      L"Sao Paulo" });
    cities.push_back({ Location(L"Rio de Janeiro",  -22.9068, -43.1729,  10, -3, true),  L"Brazil",      L"Rio de Janeiro" });
    cities.push_back({ Location(L"Santiago",        -33.4489, -70.6693, 520, -4, true),  L"Chile",       L"Santiago" });
    cities.push_back({ Location(L"Montevideo",      -34.9011, -56.1645,  43, -3, true),  L"Uruguay",     L"Montevideo" });
    cities.push_back({ Location(L"Bogota",            4.7110, -74.0721,2600, -5, false), L"Colombia",    L"Bogota" });
    cities.push_back({ Location(L"Caracas",          10.4806, -66.9036, 900, -4, false), L"Venezuela",   L"Caracas" });

    // -------------------------------------------------------------------------
    // AUSTRALIA & NEW ZEALAND
    // -------------------------------------------------------------------------
    cities.push_back({ Location(L"Sydney",          -33.8688,  151.2093,  25, 10, true),  L"Australia",   L"New South Wales" });
    cities.push_back({ Location(L"Melbourne",       -37.8136,  144.9631,  25, 10, true),  L"Australia",   L"Victoria" });
    cities.push_back({ Location(L"Brisbane",        -27.4698,  153.0251,  10, 10, false), L"Australia",   L"Queensland" });
    cities.push_back({ Location(L"Perth",           -31.9505,  115.8605,  10,  8, true),  L"Australia",   L"Western Australia" });
    cities.push_back({ Location(L"Auckland",        -36.8509,  174.7645,  15, 12, true),  L"New Zealand", L"Auckland" });

    // -------------------------------------------------------------------------
    // SOUTH AFRICA
    // -------------------------------------------------------------------------
    cities.push_back({ Location(L"Johannesburg",    -26.2041,   28.0473,1753,  2, false), L"South Africa", L"Gauteng" });
    cities.push_back({ Location(L"Cape Town",       -33.9249,   18.4241,  17,  2, false), L"South Africa", L"Western Cape" });

    // -------------------------------------------------------------------------
    // MIDDLE EAST & NORTH AFRICA
    // -------------------------------------------------------------------------
    cities.push_back({ Location(L"Cairo",            30.0444,   31.2357,  23,  2, false), L"Egypt",       L"Cairo" });
    cities.push_back({ Location(L"Casablanca",       33.5731,  -7.5898,  50,  0, false), L"Morocco",     L"Casablanca" });
    cities.push_back({ Location(L"Tunis",            36.8065,  10.1815,  10,  1, false), L"Tunisia",     L"Tunis" });
    cities.push_back({ Location(L"Dubai",            25.2048,  55.2708,  10,  4, false), L"UAE",         L"Dubai" });

    // -------------------------------------------------------------------------
    // SORT by name
    // -------------------------------------------------------------------------
    std::sort(cities.begin(), cities.end(),
        [](const LocationEntry& a, const LocationEntry& b)
        {
            return a.loc.name < b.loc.name;
        });

    return cities;
}

// =============================================================================
// LOCATION DATABASE — IMPLEMENTATION
// =============================================================================

// Private constructor — loads built-in cities and then custom locations.
LocationDB::LocationDB()
{
    LoadBuiltIn();

    m_filePath = GetLocationsFilePath();
    LoadCustomLocations(m_filePath);
}

// Returns the singleton instance.
LocationDB& LocationDB::Get()
{
    static LocationDB instance;
    return instance;
}

// Loads all built-in cities into m_entries.
void LocationDB::LoadBuiltIn()
{
    m_entries = GetBuiltInCities();
}

// Returns all locations (built-in + custom).
const std::vector<LocationEntry>& LocationDB::GetAll() const
{
    return m_entries;
}

// Returns only built-in locations.
std::vector<LocationEntry> LocationDB::GetBuiltIn() const
{
    std::vector<LocationEntry> result;
    for (const auto& e : m_entries)
        if (!e.isCustom) result.push_back(e);
    return result;
}

// Returns only custom locations.
std::vector<LocationEntry> LocationDB::GetCustom() const
{
    std::vector<LocationEntry> result;
    for (const auto& e : m_entries)
        if (e.isCustom) result.push_back(e);
    return result;
}

// Finds a location by name (case-insensitive).
const LocationEntry* LocationDB::FindByName(const std::wstring& name) const
{
    std::wstring lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    for (const auto& e : m_entries)
    {
        std::wstring eName = e.loc.name;
        std::transform(eName.begin(), eName.end(), eName.begin(), ::towlower);
        if (eName == lower) return &e;
    }
    return nullptr;
}

// Adds a new custom location. Returns false if name already exists.
bool LocationDB::AddCustom(const LocationEntry& entry)
{
    if (FindByName(entry.loc.name)) return false;

    LocationEntry custom = entry;
    custom.isCustom = true;
    m_entries.push_back(custom);

    // Keep sorted by name
    std::sort(m_entries.begin(), m_entries.end(),
        [](const LocationEntry& a, const LocationEntry& b)
        { return a.loc.name < b.loc.name; });

    SaveCustomLocations(m_filePath);
    return true;
}

// Updates a custom location by name. Returns false if not found or built-in.
bool LocationDB::UpdateCustom(const std::wstring& name, const LocationEntry& updated)
{
    for (auto& e : m_entries)
    {
        if (e.loc.name == name)
        {
            if (!e.isCustom) return false; // can't edit built-in
            e = updated;
            e.isCustom = true;
            SaveCustomLocations(m_filePath);
            return true;
        }
    }
    return false;
}

// Deletes a custom location by name. Returns false if not found or built-in.
bool LocationDB::DeleteCustom(const std::wstring& name)
{
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
    {
        if (it->loc.name == name)
        {
            if (!it->isCustom) return false;
            m_entries.erase(it);
            SaveCustomLocations(m_filePath);
            return true;
        }
    }
    return false;
}

// =============================================================================
// FILE PATH
// =============================================================================

// Returns %APPDATA%\WinLuach\locations.json
std::wstring LocationDB::GetLocationsFilePath()
{
    wchar_t path[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path)))
    {
        std::wstring dir = std::wstring(path) + L"\\WinLuach";
        CreateDirectoryW(dir.c_str(), nullptr); // creates if not exists
        return dir + L"\\locations.json";
    }
    return L"locations.json"; // fallback to current directory
}

// =============================================================================
// JSON SAVE / LOAD
// Hand-rolled minimal JSON — no external library needed.
// Format:
// [
//   {
//     "name": "My City",
//     "country": "United States",
//     "region": "New York",
//     "lat": 40.65,
//     "lon": -73.94,
//     "elev": 10,
//     "gmt": -5,
//     "dst": true
//   }
// ]
// =============================================================================

// Escapes a wstring for JSON output (handles quotes and backslashes).
static std::wstring JsonEscape(const std::wstring& s)
{
    std::wstring out;
    for (wchar_t c : s)
    {
        if (c == L'"')  out += L"\\\"";
        else if (c == L'\\') out += L"\\\\";
        else                 out += c;
    }
    return out;
}

// Saves all custom locations to a JSON file.
bool LocationDB::SaveCustomLocations(const std::wstring& filePath) const
{
    std::wofstream f(filePath);
    if (!f.is_open()) return false;

    f << L"[\n";
    bool first = true;

    for (const auto& e : m_entries)
    {
        if (!e.isCustom) continue;

        if (!first) f << L",\n";
        first = false;

        f << L"  {\n";
        f << L"    \"name\": \"" << JsonEscape(e.loc.name) << L"\",\n";
        f << L"    \"country\": \"" << JsonEscape(e.country) << L"\",\n";
        f << L"    \"region\": \"" << JsonEscape(e.region) << L"\",\n";
        f << L"    \"lat\": " << e.loc.latitude << L",\n";
        f << L"    \"lon\": " << e.loc.longitude << L",\n";
        f << L"    \"elev\": " << e.loc.elevation << L",\n";
        f << L"    \"gmt\": " << e.loc.gmtOffset << L",\n";
        f << L"    \"dst\": " << (e.loc.usesDST ? L"true" : L"false") << L",\n";
        f << L"    \"israel\": " << (e.isIsrael ? L"true" : L"false") << L"\n";
        f << L"  }";
    }

    f << L"\n]\n";
    return true;
}

// Reads a quoted JSON string value from a line like: "key": "value"
static std::wstring ParseJsonString(const std::wstring& line)
{
    size_t first = line.find(L'"', line.find(L':'));
    if (first == std::wstring::npos) return L"";
    first++;
    size_t last = line.find(L'"', first);
    if (last == std::wstring::npos) return L"";
    return line.substr(first, last - first);
}

// Reads a numeric JSON value from a line like: "key": 42
static double ParseJsonNumber(const std::wstring& line)
{
    size_t colon = line.find(L':');
    if (colon == std::wstring::npos) return 0.0;
    std::wstring val = line.substr(colon + 1);
    // trim whitespace
    size_t start = val.find_first_not_of(L" \t\r\n,");
    if (start == std::wstring::npos) return 0.0;
    return std::stod(val.substr(start));
}

// Reads a boolean JSON value from a line like: "key": true
static bool ParseJsonBool(const std::wstring& line)
{
    return line.find(L"true") != std::wstring::npos;
}

// Loads custom locations from a JSON file.
bool LocationDB::LoadCustomLocations(const std::wstring& filePath)
{
    std::wifstream f(filePath);
    if (!f.is_open()) return false;

    LocationEntry current;
    bool inObject = false;

    std::wstring line;
    while (std::getline(f, line))
    {
        // Trim whitespace
        size_t start = line.find_first_not_of(L" \t\r\n");
        if (start == std::wstring::npos) continue;
        line = line.substr(start);

        if (line[0] == L'{')
        {
            // Start of a new location object
            current = LocationEntry();
            inObject = true;
        }
        else if (line[0] == L'}' && inObject)
        {
            // End of object — add to database if not already there
            current.isCustom = true;
            if (!FindByName(current.loc.name))
                m_entries.push_back(current);
            inObject = false;
        }
        else if (inObject)
        {
            if (line.find(L"\"name\"") != std::wstring::npos) current.loc.name = ParseJsonString(line);
            else if (line.find(L"\"country\"") != std::wstring::npos) current.country = ParseJsonString(line);
            else if (line.find(L"\"region\"") != std::wstring::npos) current.region = ParseJsonString(line);
            else if (line.find(L"\"lat\"") != std::wstring::npos) current.loc.latitude = ParseJsonNumber(line);
            else if (line.find(L"\"lon\"") != std::wstring::npos) current.loc.longitude = ParseJsonNumber(line);
            else if (line.find(L"\"elev\"") != std::wstring::npos) current.loc.elevation = ParseJsonNumber(line);
            else if (line.find(L"\"gmt\"") != std::wstring::npos) current.loc.gmtOffset = (int)ParseJsonNumber(line);
            else if (line.find(L"\"dst\"") != std::wstring::npos) current.loc.usesDST = ParseJsonBool(line);
            else if (line.find(L"\"israel\"") != std::wstring::npos) current.isIsrael = ParseJsonBool(line);
        }
    }

    // Re-sort after loading custom entries
    std::sort(m_entries.begin(), m_entries.end(),
        [](const LocationEntry& a, const LocationEntry& b)
        { return a.loc.name < b.loc.name; });

    return true;
}

// =============================================================================
// DEFAULT LOCATION
// =============================================================================

// Returns New York City as the default location for first-run.
LocationEntry LocationDB::GetDefaultLocation()
{
    const LocationEntry* nyc = Get().FindByName(L"New York City");
    if (nyc) return *nyc;

    // Fallback if somehow not found in database
    return LocationEntry(
        Location(L"New York City", 40.6501, -73.9496, 10.0, -5, true),
        L"United States", L"New York"
    );
}

// =============================================================================
// FREE FUNCTIONS
// =============================================================================

// Returns a display string for a location entry.
std::wstring LocationDisplayString(const LocationEntry& entry)
{
    std::wstring s = entry.loc.name;
    if (!entry.region.empty())   s += L", " + entry.region;
    if (!entry.country.empty())  s += L", " + entry.country;
    if (entry.isCustom)          s += L" *"; // asterisk marks custom
    return s;
}