// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    UpdateChecker.cpp
// Purpose: Implementation of GitHub-based update checker.
// =============================================================================
//
// CHANGELOG:
// v0.8.64 - Fix build error: replaced unqualified GetLastError() calls with
//           ::GetLastError() in FetchLatestRelease(). The class method
//           CUpdateChecker::GetLastError() (returning std::wstring) was shadowing
//           the Win32 API ::GetLastError() (returning DWORD), causing C2440 errors.
// v0.8.61 - Enhanced error reporting. All network operations now capture Windows
//           error codes for better diagnostics. Added User-Agent and Accept headers
//           to GitHub API requests (required by GitHub API).
// v0.8.54 - Initial implementation. Fetches release info, compares versions,
//           downloads updates, shows dialogs.
// =============================================================================

#include "pch.h"
#include "UpdateChecker.h"
#include <shlwapi.h>
#include <wininet.h>
#include <fstream>
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Wininet.lib")

// ============================================================================
// SemanticVersion Implementation
// ============================================================================

SemanticVersion::SemanticVersion(const std::wstring& versionStr)
{
    Parse(versionStr);
}

void SemanticVersion::Parse(const std::wstring& versionStr)
{
    // Strip leading 'v' if present
    std::wstring v = versionStr;
    if (!v.empty() && (v[0] == L'v' || v[0] == L'V'))
        v = v.substr(1);

    // Parse "major.minor.patch"
    size_t pos1 = v.find(L'.');
    size_t pos2 = (pos1 != std::wstring::npos) ? v.find(L'.', pos1 + 1) : std::wstring::npos;

    try
    {
        if (pos1 != std::wstring::npos)
            major = std::stoi(v.substr(0, pos1));
        if (pos1 != std::wstring::npos && pos2 != std::wstring::npos)
            minor = std::stoi(v.substr(pos1 + 1, pos2 - pos1 - 1));
        if (pos2 != std::wstring::npos)
            patch = std::stoi(v.substr(pos2 + 1));
    }
    catch (...)
    {
        // Use defaults on parse error
        major = minor = patch = 0;
    }
}

bool SemanticVersion::IsNewerThan(const SemanticVersion& other) const
{
    if (major != other.major)
        return major > other.major;
    if (minor != other.minor)
        return minor > other.minor;
    return patch > other.patch;
}

bool SemanticVersion::Equals(const SemanticVersion& other) const
{
    return major == other.major && minor == other.minor && patch == other.patch;
}

std::wstring SemanticVersion::ToString() const
{
    wchar_t buf[32];
    _snwprintf_s(buf, _TRUNCATE, L"%d.%d.%d", major, minor, patch);
    return buf;
}

// ============================================================================
// CUpdateChecker Implementation
// ============================================================================

CUpdateChecker::CUpdateChecker()
{
}

CUpdateChecker::~CUpdateChecker()
{
}

std::wstring CUpdateChecker::GetCurrentVersion()
{
    // Get the current executable's version info
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    DWORD dummyHandle = 0;
    DWORD versionInfoSize = GetFileVersionInfoSizeW(exePath, &dummyHandle);
    if (versionInfoSize == 0)
        return L"0.0.0";

    std::vector<uint8_t> buffer(versionInfoSize);
    if (!GetFileVersionInfoW(exePath, 0, versionInfoSize, buffer.data()))
        return L"0.0.0";

    // Query product version
    LPWSTR productVersion = nullptr;
    UINT productVersionLen = 0;
    if (!VerQueryValueW(buffer.data(), L"\\StringFileInfo\\040904B0\\ProductVersion",
                       (LPVOID*)&productVersion, &productVersionLen))
        return L"0.0.0";

    if (productVersion && productVersionLen > 0)
        return std::wstring(productVersion, productVersionLen - 1); // -1 to skip null terminator

    return L"0.0.0";
}

bool CUpdateChecker::FetchLatestRelease(GitHubRelease& outRelease)
{
    m_lastError = L"";

    // Use the public GitHub API to get the latest release
    HINTERNET hInternet = InternetOpenW(L"WinLuach UpdateChecker",
                                        INTERNET_OPEN_TYPE_PRECONFIG,
                                        nullptr, nullptr, 0);
    if (!hInternet)
    {
        DWORD dwError = ::GetLastError();
        wchar_t buf[256];
        _snwprintf_s(buf, _TRUNCATE, L"Failed to initialize internet connection (Error: 0x%X)", dwError);
        m_lastError = buf;
        return false;
    }

    HINTERNET hConnect = InternetConnectW(hInternet, L"api.github.com",
                                          INTERNET_DEFAULT_HTTPS_PORT,
                                          nullptr, nullptr,
                                          INTERNET_SERVICE_HTTP,
                                          INTERNET_FLAG_SECURE, 0);
    if (!hConnect)
    {
        DWORD dwError = ::GetLastError();
        wchar_t buf[256];
        _snwprintf_s(buf, _TRUNCATE, L"Failed to connect to GitHub API (Error: 0x%X)", dwError);
        m_lastError = buf;
        InternetCloseHandle(hInternet);
        return false;
    }

    // Request the latest release
    HINTERNET hRequest = HttpOpenRequestW(hConnect,
                                          L"GET",
                                          L"/repos/akivacp/WinLuach/releases/latest",
                                          L"HTTP/1.1",
                                          nullptr,
                                          nullptr,
                                          INTERNET_FLAG_SECURE,
                                          0);
    if (!hRequest)
    {
        DWORD dwError = ::GetLastError();
        wchar_t buf[256];
        _snwprintf_s(buf, _TRUNCATE, L"Failed to create HTTP request (Error: 0x%X)", dwError);
        m_lastError = buf;
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    // Send request with User-Agent header (required by GitHub API)
    LPCWSTR headers = L"User-Agent: WinLuach/0.8\r\nAccept: application/vnd.github.v3+json\r\n";
    BOOL result = HttpSendRequestW(hRequest, headers, static_cast<DWORD>(wcslen(headers)), nullptr, 0);
    if (!result)
    {
        DWORD dwError = ::GetLastError();
        wchar_t buf[256];
        _snwprintf_s(buf, _TRUNCATE, L"Failed to send HTTP request (Error: 0x%X)", dwError);
        m_lastError = buf;
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    // Read response
    std::string responseJson;
    char buffer[4096];
    DWORD bytesRead = 0;

    while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
    {
        responseJson.append(buffer, bytesRead);
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    if (responseJson.empty())
    {
        m_lastError = L"Empty response from GitHub API";
        return false;
    }

    // Parse JSON response (simplified JSON parsing for the fields we need)
    // In production, consider using a proper JSON library
    // For now, we'll use simple string search for the key fields
    try
    {
        // Find tag_name
        size_t tagPos = responseJson.find("\"tag_name\"");
        if (tagPos == std::string::npos)
        {
            m_lastError = L"Could not find tag_name in API response";
            return false;
        }

        size_t tagStart = responseJson.find("\"", tagPos + 11);
        size_t tagEnd = responseJson.find("\"", tagStart + 1);
        if (tagStart == std::string::npos || tagEnd == std::string::npos)
        {
            m_lastError = L"Could not parse tag_name from API response";
            return false;
        }

        std::string tagNameStr = responseJson.substr(tagStart + 1, tagEnd - tagStart - 1);

        // Convert to wstring
        int wchars = MultiByteToWideChar(CP_UTF8, 0, tagNameStr.c_str(), -1, nullptr, 0);
        std::wstring tagNameW(wchars - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, tagNameStr.c_str(), -1, &tagNameW[0], wchars);

        outRelease.tagName = tagNameW;

        // Find first asset with .exe extension
        size_t assetsPos = responseJson.find("\"assets\"");
        if (assetsPos != std::string::npos)
        {
            size_t exePos = responseJson.find(".exe", assetsPos);
            if (exePos != std::string::npos)
            {
                // Find the URL for this asset
                size_t urlStart = responseJson.rfind("\"browser_download_url\"", exePos);
                if (urlStart != std::string::npos)
                {
                    urlStart = responseJson.find("\"", urlStart + 23);
                    size_t urlEnd = responseJson.find("\"", urlStart + 1);
                    if (urlStart != std::string::npos && urlEnd != std::string::npos)
                    {
                        std::string downloadUrlStr = responseJson.substr(urlStart + 1, urlEnd - urlStart - 1);

                        // Convert to wstring
                        wchars = MultiByteToWideChar(CP_UTF8, 0, downloadUrlStr.c_str(), -1, nullptr, 0);
                        std::wstring downloadUrlW(wchars - 1, L'\0');
                        MultiByteToWideChar(CP_UTF8, 0, downloadUrlStr.c_str(), -1, &downloadUrlW[0], wchars);

                        outRelease.downloadUrl = downloadUrlW;

                        // Extract filename
                        size_t lastSlash = downloadUrlStr.rfind('/');
                        if (lastSlash != std::string::npos)
                        {
                            std::string filenameStr = downloadUrlStr.substr(lastSlash + 1);
                            wchars = MultiByteToWideChar(CP_UTF8, 0, filenameStr.c_str(), -1, nullptr, 0);
                            std::wstring filenameW(wchars - 1, L'\0');
                            MultiByteToWideChar(CP_UTF8, 0, filenameStr.c_str(), -1, &filenameW[0], wchars);
                            outRelease.assetFilename = filenameW;
                        }
                    }
                }
            }
        }

        return !outRelease.tagName.empty() && !outRelease.downloadUrl.empty();
    }
    catch (...)
    {
        m_lastError = L"Exception while parsing GitHub API response";
        return false;
    }
}

bool CUpdateChecker::DownloadAsset(const std::wstring& downloadUrl,
                                   const std::wstring& outputPath,
                                   bool showProgress)
{
    m_lastError = L"";

    // Simple file download using URLDownloadToFileW
    HRESULT hr = URLDownloadToFileW(nullptr, downloadUrl.c_str(), outputPath.c_str(),
                                    0, nullptr);

    if (FAILED(hr))
    {
        wchar_t buf[256];
        _snwprintf_s(buf, _TRUNCATE, L"Download failed with error code 0x%08X", hr);
        m_lastError = buf;
        return false;
    }

    return true;
}

int CUpdateChecker::ShowUpdateDialog(const GitHubRelease& release,
                                      const std::wstring& currentVersion,
                                      CWnd* pParent)
{
    // Build dialog text
    std::wstring message = L"A new version of WinLuach is available!\n\n";
    message += L"Current version: " + currentVersion + L"\n";
    message += L"Latest version: " + release.tagName + L"\n\n";
    message += L"Would you like to download and install the update?";

    return AfxMessageBox(message.c_str(), MB_YESNO | MB_ICONINFORMATION);
}

SemanticVersion CUpdateChecker::ExtractVersion(const std::wstring& tagName)
{
    return SemanticVersion(tagName);
}
