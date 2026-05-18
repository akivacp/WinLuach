// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    UpdateChecker.h
// Purpose: Checks for application updates from GitHub releases.
//          Compares version numbers and offers to download/install updates.
// =============================================================================
//
// CHANGELOG:
// v0.8.68 - Fixed GetCurrentVersion() returning "0.0.0". Runtime VerQueryValueW
//           call with hardcoded locale 040904B0 fails when RC uses a different
//           codepage. Replaced with compile-time WINLUACH_VERSION_TEXT constant.
// v0.8.67 - Fixed asset URL parsing bug in FetchLatestRelease: rfind() searched
//           backward for browser_download_url but that field follows the .exe name
//           in the JSON; switched to find() (forward). Added HTTP status-code check
//           and filled m_lastError for every previously-silent failure path.
// v0.8.65 - Added LaunchUpdateAndExit(). Writes a hidden batch file that waits
//           for the app to exit, renames the old exe, copies the new one into
//           place, restarts WinLuach, then cleans up. Enables true one-click
//           auto-update with no manual steps required.
// v0.8.54 - Initial implementation. Fetches latest release from GitHub API,
//           compares semantic versions, downloads updates, shows dialogs.
// =============================================================================

#pragma once
#include <string>
#include <cstdint>

// Parses semantic version strings like "v0.8.53" and provides comparison
class SemanticVersion
{
public:
    SemanticVersion(const std::wstring& versionStr = L"0.0.0");

    // Returns true if this version is greater than other
    bool IsNewerThan(const SemanticVersion& other) const;

    // Returns true if versions are equal
    bool Equals(const SemanticVersion& other) const;

    // Returns formatted version string (e.g., "0.8.53")
    std::wstring ToString() const;

private:
    int major = 0;
    int minor = 0;
    int patch = 0;

    // Parses a version string like "v0.8.53" or "0.8.53"
    void Parse(const std::wstring& versionStr);
};

// Release information fetched from GitHub
struct GitHubRelease
{
    std::wstring tagName;           // e.g., "v0.8.53"
    std::wstring name;              // e.g., "Pre-Shabbos Release: WinLuachToasts"
    std::wstring body;              // release notes / description
    std::wstring downloadUrl;       // URL to download the asset
    std::wstring assetFilename;     // e.g., "WinLuach.exe"
    int64_t      assetSizeBytes = 0;
};

// Handles checking for updates and downloading new versions
class CUpdateChecker
{
public:
    CUpdateChecker();
    ~CUpdateChecker();

    // Gets the current application version (read from file version info)
    static std::wstring GetCurrentVersion();

    // Fetches latest release info from GitHub
    // Returns true if successful and newer version found
    bool FetchLatestRelease(GitHubRelease& outRelease);

    // Downloads the asset to the specified path
    // Shows progress dialog if showProgress=true
    // Returns true on success
    bool DownloadAsset(const std::wstring& downloadUrl,
                       const std::wstring& outputPath,
                       bool showProgress = true);

    // Shows a dialog offering to download and install the update
    // Returns IDYES if user wants to update, IDNO otherwise
    int ShowUpdateDialog(const GitHubRelease& release,
                         const std::wstring& currentVersion,
                         CWnd* pParent = nullptr);

    // Writes a hidden batch file that:
    //   1. Waits for this process to exit
    //   2. Renames the running exe to .old.exe (Windows allows renaming a running exe)
    //   3. Copies newExePath to the original exe location
    //   4. Launches the new exe
    //   5. Cleans up the downloaded file, the .old.exe, and the batch itself
    // After calling this, immediately call DestroyWindow() to close the app.
    // Returns true if the batch was launched successfully.
    static bool LaunchUpdateAndExit(const std::wstring& newExePath);

    // Gets any error message from the last operation
    std::wstring GetLastError() const { return m_lastError; }

private:
    std::wstring m_lastError;

    // Helper to convert version tag to comparable form
    static SemanticVersion ExtractVersion(const std::wstring& tagName);
};
