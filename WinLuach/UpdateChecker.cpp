// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    UpdateChecker.cpp
// Purpose: Implementation of GitHub-based update checker.
// =============================================================================
//
// CHANGELOG:
// v0.8.68 - Fixed GetCurrentVersion() returning "0.0.0". The runtime VERSIONINFO
//           resource query used hardcoded locale 040904B0; when the RC block uses
//           a different locale the VerQueryValue call returns nothing. Replaced
//           with the compile-time WINLUACH_VERSION_TEXT constant from Version.h —
//           always correct, no runtime lookup needed.
// v0.8.67 - Fixed silent failure in FetchLatestRelease asset URL parsing.
//           Was using rfind() (backward search) to locate browser_download_url,
//           but that field comes AFTER the .exe name in the GitHub JSON — so
//           rfind always returned npos and downloadUrl was never populated.
//           Changed to find() (forward search). Also added HTTP status-code
//           check (non-200 now surfaces the actual code in m_lastError) and
//           filled in m_lastError for every previously-silent failure path.
// v0.8.65 - Implemented LaunchUpdateAndExit(). Writes a hidden cmd batch to temp,
//           launches it via ShellExecuteExW (SW_HIDE), then the caller exits.
//           The batch: waits 3s, renames current exe to .old.exe, copies downloaded
//           update to original path, restarts the app, and cleans up all temp files.
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
#include "Version.h"
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
    // Return the version baked in at compile time from Version.h.
    //
    // The previous implementation queried the exe's VERSIONINFO resource
    // using VerQueryValueW with the hardcoded locale path
    // "\\StringFileInfo\\040904B0\\ProductVersion". When the RC block's
    // language/codepage entry differs from 040904B0 (e.g. it uses 000004B0
    // for language-neutral), VerQueryValueW returns FALSE and we fell back
    // to "0.0.0" — causing the update checker to always think a new version
    // was available. Using the compile-time constant is simpler and reliable.
    return WINLUACH_VERSION_TEXT;
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

    // Send request with User-Agent and Accept headers (required by GitHub API)
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

    // Check HTTP status code — anything other than 200 is an error
    // (e.g. 404 = no releases published yet, 403 = rate limited)
    DWORD httpStatus = 0;
    DWORD statusSize = sizeof(httpStatus);
    if (HttpQueryInfoW(hRequest,
                       HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                       &httpStatus, &statusSize, nullptr))
    {
        if (httpStatus != 200)
        {
            wchar_t buf[256];
            _snwprintf_s(buf, _TRUNCATE,
                L"GitHub API returned HTTP %lu. "
                L"(404 = no releases published; 403 = rate limited)", httpStatus);
            m_lastError = buf;
            InternetCloseHandle(hRequest);
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return false;
        }
    }

    // Read the response body
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

    // Parse the JSON response using simple string search.
    // (A full JSON library isn't needed for the two fields we care about.)
    try
    {
        // Helper: convert a UTF-8 std::string slice to std::wstring
        auto utf8ToWide = [](const std::string& s) -> std::wstring
        {
            if (s.empty()) return {};
            int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
            if (n <= 1) return {};
            std::wstring w(n - 1, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
            return w;
        };

        // --- tag_name ---
        size_t tagPos = responseJson.find("\"tag_name\"");
        if (tagPos == std::string::npos)
        {
            m_lastError = L"Could not find \"tag_name\" in GitHub API response";
            return false;
        }
        size_t tagStart = responseJson.find("\"", tagPos + 10); // skip past key
        if (tagStart == std::string::npos) { m_lastError = L"Malformed tag_name value"; return false; }
        tagStart++; // move past opening quote
        size_t tagEnd = responseJson.find("\"", tagStart);
        if (tagEnd == std::string::npos)  { m_lastError = L"Unterminated tag_name value"; return false; }

        outRelease.tagName = utf8ToWide(responseJson.substr(tagStart, tagEnd - tagStart));

        // --- assets[].browser_download_url for the first .exe asset ---
        // GitHub JSON layout inside each asset object:
        //   "name": "WinLuach.exe",          <- .exe appears here first
        //   ...
        //   "browser_download_url": "https://...WinLuach.exe"   <- URL comes AFTER
        //
        // We search FORWARD (find) from the .exe name position to locate the URL.
        // Using rfind (backward) was the previous bug — it looked before the name
        // and always returned npos.

        size_t assetsPos = responseJson.find("\"assets\"");
        if (assetsPos == std::string::npos)
        {
            m_lastError = L"No \"assets\" array found in release (release may have no files)";
            return false;
        }

        // Match ".exe" followed immediately by a quote so we hit the filename,
        // not a partial match inside another word.
        size_t exePos = responseJson.find(".exe\"", assetsPos);
        if (exePos == std::string::npos)
        {
            m_lastError = L"No .exe asset found in the latest release";
            return false;
        }

        // Now search FORWARD from the .exe name for browser_download_url
        size_t urlKeyPos = responseJson.find("\"browser_download_url\"", exePos);
        if (urlKeyPos == std::string::npos)
        {
            m_lastError = L"Could not find browser_download_url for the .exe asset";
            return false;
        }

        size_t urlStart = responseJson.find("\"", urlKeyPos + 22); // skip past key
        if (urlStart == std::string::npos) { m_lastError = L"Malformed browser_download_url"; return false; }
        urlStart++; // move past opening quote
        size_t urlEnd = responseJson.find("\"", urlStart);
        if (urlEnd == std::string::npos)   { m_lastError = L"Unterminated browser_download_url"; return false; }

        std::string downloadUrlStr = responseJson.substr(urlStart, urlEnd - urlStart);
        outRelease.downloadUrl  = utf8ToWide(downloadUrlStr);

        // Derive the filename from the last path segment of the URL
        size_t lastSlash = downloadUrlStr.rfind('/');
        if (lastSlash != std::string::npos)
            outRelease.assetFilename = utf8ToWide(downloadUrlStr.substr(lastSlash + 1));

        // Final guard — both fields must be populated
        if (outRelease.tagName.empty() || outRelease.downloadUrl.empty())
        {
            m_lastError = L"Incomplete data in GitHub API response (tag or URL missing)";
            return false;
        }

        return true;
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

// ============================================================================
// LaunchUpdateAndExit  (v0.8.65)
// Writes a hidden batch script that:
//   1. Waits 3 seconds so the calling process has time to fully exit
//   2. Renames the current exe to WinLuach.old.exe  (Windows allows renaming
//      a running exe even though it won't allow overwriting one)
//   3. Copies the downloaded update to the original exe path
//   4. Launches the freshly copied exe
//   5. Deletes the downloaded temp file, the .old.exe backup, and itself
// After calling this, the caller must exit immediately (e.g. DestroyWindow).
// Returns true if the batch was written and launched successfully.
// ============================================================================
bool CUpdateChecker::LaunchUpdateAndExit(const std::wstring& newExePath)
{
    // --- 1. Determine paths ---

    // Path of the currently running executable
    wchar_t currentExeW[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, currentExeW, MAX_PATH);
    std::wstring currentExePath(currentExeW);

    // Backup path: same folder, same stem, but .old.exe extension
    std::wstring oldExePath = currentExePath;
    size_t dotPos = oldExePath.rfind(L'.');
    if (dotPos != std::wstring::npos)
        oldExePath = oldExePath.substr(0, dotPos) + L".old.exe";
    else
        oldExePath += L".old.exe";

    // Batch file lands in the user temp folder
    wchar_t tempDirW[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, tempDirW);
    std::wstring batchPath = tempDirW;
    batchPath += L"WinLuachUpdater.bat";

    // --- 2. Helper: wstring → ANSI (batch files are not Unicode-aware) ---
    auto toAnsi = [](const std::wstring& ws) -> std::string
    {
        int len = WideCharToMultiByte(CP_ACP, 0, ws.c_str(), -1,
                                      nullptr, 0, nullptr, nullptr);
        if (len <= 1) return {};
        std::string s(len - 1, '\0');
        WideCharToMultiByte(CP_ACP, 0, ws.c_str(), -1,
                            &s[0], len, nullptr, nullptr);
        return s;
    };

    std::string curExe  = toAnsi(currentExePath);
    std::string newExe  = toAnsi(newExePath);
    std::string oldExe  = toAnsi(oldExePath);

    // --- 3. Write the batch file ---
    FILE* f = nullptr;
    if (_wfopen_s(&f, batchPath.c_str(), L"w") != 0 || !f)
        return false;

    // @echo off        — suppress command echo
    // timeout /t 3     — wait 3 s for the old process to fully exit
    // move /y          — rename running exe to .old.exe (permitted on Windows)
    // copy /y          — place the downloaded update at the original path
    // start ""         — launch the new exe (detached)
    // del              — clean up downloaded file and backup
    // del "%~f0"       — batch deletes itself last
    fprintf(f, "@echo off\r\n");
    fprintf(f, "timeout /t 3 /nobreak >nul\r\n");
    fprintf(f, "move /y \"%s\" \"%s\"\r\n", curExe.c_str(), oldExe.c_str());
    fprintf(f, "copy /y \"%s\" \"%s\"\r\n", newExe.c_str(), curExe.c_str());
    fprintf(f, "start \"\" \"%s\"\r\n",     curExe.c_str());
    fprintf(f, "del \"%s\"\r\n",            newExe.c_str());
    fprintf(f, "del \"%s\"\r\n",            oldExe.c_str());
    fprintf(f, "del \"%%~f0\"\r\n");        // %~f0 = full path of the batch itself
    fclose(f);

    // --- 4. Launch the batch hidden via cmd.exe ---
    std::wstring params = L"/c \"" + batchPath + L"\"";

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize      = sizeof(sei);
    sei.fMask       = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb      = L"open";
    sei.lpFile      = L"cmd.exe";
    sei.lpParameters = params.c_str();
    sei.nShow       = SW_HIDE;

    if (!ShellExecuteExW(&sei))
    {
        // Clean up the batch if we couldn't launch it
        DeleteFileW(batchPath.c_str());
        return false;
    }

    // We don't need the process handle — close it to avoid a handle leak
    if (sei.hProcess)
        CloseHandle(sei.hProcess);

    return true;
}
