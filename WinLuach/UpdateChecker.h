// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    UpdateChecker.h
// Purpose: Checks for application updates from GitHub releases.
//          Compares version numbers and offers to download/install updates.
// =============================================================================
//
// CHANGELOG:
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

    // Gets any error message from the last operation
    std::wstring GetLastError() const { return m_lastError; }

private:
    std::wstring m_lastError;

    // Helper to convert version tag to comparable form
    static SemanticVersion ExtractVersion(const std::wstring& tagName);
};
