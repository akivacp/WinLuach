// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    LocationDlg.cpp
// Purpose: MFC location picker dialog implementation.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial implementation.
// v0.1.1 - Fixed dialog creation: use InitModalIndirect with in-memory template.
// v0.1.2 - Fixed CreateEx call to use 10 arguments (removed 2 extra nullptrs).
//          Moved DoModal() to public in header.
// =============================================================================

#include "pch.h"
#include "LocationDlg.h"
#include <winhttp.h>
#pragma comment(lib, "Winhttp.lib")

// =============================================================================
// GeoResult — one Nominatim result
// =============================================================================

struct GeoResult
{
    std::wstring displayName;
    double lat = 0.0;
    double lon = 0.0;
};

// Performs a synchronous WinHTTP GET and returns the response body as wstring.
static std::wstring WinHttpGet(const std::wstring& host, const std::wstring& path)
{
    HINTERNET hSession = WinHttpOpen(
        L"WinLuach/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return L"";

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return L""; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return L"";
    }

    BOOL sent = WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!sent || !WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return L"";
    }

    std::string body;
    DWORD avail = 0;
    while (WinHttpQueryDataAvailable(hRequest, &avail) && avail > 0) {
        std::string buf(avail, '\0');
        DWORD read = 0;
        WinHttpReadData(hRequest, buf.data(), avail, &read);
        body.append(buf.data(), read);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (body.empty()) return L"";
    int needed = MultiByteToWideChar(CP_UTF8, 0, body.data(), (int)body.size(), nullptr, 0);
    if (needed <= 0) return L"";
    std::wstring out(needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, body.data(), (int)body.size(), out.data(), needed);
    return out;
}

// URL-encode a query string (spaces → %20, etc.)
static std::wstring UrlEncode(const std::wstring& s)
{
    std::wstring out;
    for (wchar_t c : s) {
        if ((c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') ||
            (c >= L'0' && c <= L'9') || c == L'-' || c == L'_' || c == L'.' || c == L'~')
        {
            out += c;
        }
        else {
            // encode as UTF-8 percent-encoded
            char mb[8] = {};
            int n = WideCharToMultiByte(CP_UTF8, 0, &c, 1, mb, sizeof(mb), nullptr, nullptr);
            for (int i = 0; i < n; i++) {
                wchar_t hex[4];
                swprintf_s(hex, L"%%%02X", (unsigned char)mb[i]);
                out += hex;
            }
        }
    }
    return out;
}

// Parse Nominatim JSON: extracts up to 5 results with lat/lon/display_name.
// Very lightweight: looks for the pattern sequences without a full JSON parser.
static std::vector<GeoResult> ParseNominatimJson(const std::wstring& json)
{
    std::vector<GeoResult> results;

    size_t pos = 0;
    while (pos < json.size() && results.size() < 5)
    {
        // Find next object start
        size_t ob = json.find(L'{', pos);
        if (ob == std::wstring::npos) break;

        // Find matching closing brace (shallow — works for flat objects)
        size_t cb = json.find(L'}', ob);
        if (cb == std::wstring::npos) break;

        std::wstring obj = json.substr(ob, cb - ob + 1);
        pos = cb + 1;

        GeoResult r;
        // Extract "lat":"VALUE"
        {
            size_t p = obj.find(L"\"lat\":\"");
            if (p != std::wstring::npos) {
                p += 7;
                size_t q = obj.find(L'"', p);
                if (q != std::wstring::npos)
                    r.lat = std::stod(obj.substr(p, q - p));
            }
        }
        // Extract "lon":"VALUE"
        {
            size_t p = obj.find(L"\"lon\":\"");
            if (p != std::wstring::npos) {
                p += 7;
                size_t q = obj.find(L'"', p);
                if (q != std::wstring::npos)
                    r.lon = std::stod(obj.substr(p, q - p));
            }
        }
        // Extract "display_name":"VALUE"
        {
            size_t p = obj.find(L"\"display_name\":\"");
            if (p != std::wstring::npos) {
                p += 16;
                size_t q = p;
                while (q < obj.size() && !(obj[q] == L'"' && obj[q-1] != L'\\')) q++;
                r.displayName = obj.substr(p, q - p);
            }
        }

        if (r.lat != 0.0 || r.lon != 0.0)
            results.push_back(r);
    }
    return results;
}

// Parses standardUtcOffset.seconds from timeapi.io response → GMT hours.
static int ParseTimezoneOffset(const std::wstring& json)
{
    // Try "standardUtcOffset" first, fall back to "currentUtcOffset"
    const wchar_t* keys[] = { L"\"standardUtcOffset\"", L"\"currentUtcOffset\"" };
    for (const wchar_t* key : keys) {
        size_t p = json.find(key);
        if (p == std::wstring::npos) continue;
        size_t q = json.find(L"\"seconds\"", p);
        if (q == std::wstring::npos) continue;
        q += 9;
        while (q < json.size() && (json[q] == L':' || json[q] == L' ')) q++;
        bool neg = (q < json.size() && json[q] == L'-'); if (neg) q++;
        int secs = 0;
        while (q < json.size() && iswdigit(json[q])) secs = secs * 10 + (json[q++] - L'0');
        return neg ? -(secs / 3600) : (secs / 3600);
    }
    return 0;
}

// Returns true if hasDayLightSaving is true in the timeapi.io response.
static bool ParseHasDST(const std::wstring& json)
{
    size_t p = json.find(L"\"hasDayLightSaving\"");
    if (p == std::wstring::npos) return false;
    size_t q = json.find_first_not_of(L": ", p + 19);
    if (q == std::wstring::npos) return false;
    return json.compare(q, 4, L"true") == 0;
}

// Parses elevation from open-elevation.com response.
static double ParseElevation(const std::wstring& json)
{
    size_t p = json.find(L"\"elevation\":");
    if (p == std::wstring::npos) return 0.0;
    p += 12;
    while (p < json.size() && json[p] == L' ') p++;
    size_t start = p;
    while (p < json.size() && (iswdigit(json[p]) || json[p] == L'-' || json[p] == L'.')) p++;
    if (start == p) return 0.0;
    try { return std::stod(json.substr(start, p - start)); } catch (...) { return 0.0; }
}

// =============================================================================
// CGeoDlg — address lookup dialog using Nominatim
// =============================================================================

class CGeoDlg : public CDialog
{
public:
    double  resultLat = 0.0;
    double  resultLon = 0.0;
    std::wstring resultName;

    explicit CGeoDlg(const std::wstring& initialQuery = L"", CWnd* pParent = nullptr)
        : CDialog(), m_initialQuery(initialQuery)
    {
        m_pParentWnd = pParent;
    }

    virtual INT_PTR DoModal() override
    {
        struct DlgTmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[32]; } buf = {};
        buf.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_CENTER;
        buf.t.cx = 380; buf.t.cy = 240;
        wcscpy_s(buf.title, L"Lookup Address");
        if (!InitModalIndirect((DLGTEMPLATE*)&buf, m_pParentWnd)) return -1;
        return CDialog::DoModal();
    }

protected:
    BOOL OnInitDialog() override
    {
        CDialog::OnInitDialog();
        SetWindowText(L"Lookup Address");
        HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CFont* pF = CFont::FromHandle(hF);
        CRect rc; GetClientRect(&rc);
        int W = rc.Width(), H = rc.Height();

        CStatic* lbl = new CStatic;
        lbl->Create(L"Address / City:", WS_CHILD | WS_VISIBLE,
            CRect(8, 12, 110, 30), this);
        lbl->SetFont(pF);

        m_editAddr.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            CRect(115, 8, W - 80, 30), this, 501);
        m_editAddr.SetFont(pF);
        if (!m_initialQuery.empty())
            m_editAddr.SetWindowText(m_initialQuery.c_str());

        CButton* btnSearch = new CButton;
        btnSearch->Create(L"Search", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            CRect(W - 74, 8, W - 8, 30), this, 502);
        btnSearch->SetFont(pF);

        m_listResults.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
            LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            CRect(8, 38, W - 8, H - 42), this, 503);
        m_listResults.SetFont(pF);

        CButton* btnOK = new CButton;
        btnOK->Create(L"Select", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            CRect(W - 144, H - 34, W - 76, H - 8), this, IDOK);
        btnOK->SetFont(pF);

        CButton* btnCan = new CButton;
        btnCan->Create(L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            CRect(W - 70, H - 34, W - 8, H - 8), this, IDCANCEL);
        btnCan->SetFont(pF);

        return TRUE;
    }

    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override
    {
        UINT id = LOWORD(wParam);
        UINT note = HIWORD(wParam);

        if (id == 502 || (id == 501 && note == EN_CHANGE && GetKeyState(VK_RETURN) & 0x8000)) {
            DoSearch();
            return TRUE;
        }
        return CDialog::OnCommand(wParam, lParam);
    }

    BOOL PreTranslateMessage(MSG* pMsg) override
    {
        if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN &&
            pMsg->hwnd == m_editAddr.GetSafeHwnd())
        {
            DoSearch();
            return TRUE;
        }
        return CDialog::PreTranslateMessage(pMsg);
    }

    void OnOK() override
    {
        int sel = m_listResults.GetCurSel();
        if (sel == LB_ERR || sel >= (int)m_results.size()) {
            MessageBox(L"Please select a result.", L"WinLuach", MB_OK);
            return;
        }
        resultLat  = m_results[sel].lat;
        resultLon  = m_results[sel].lon;
        resultName = m_results[sel].displayName;
        // Truncate display name at first comma for a short city name
        size_t comma = resultName.find(L',');
        if (comma != std::wstring::npos)
            resultName = resultName.substr(0, comma);
        CDialog::OnOK();
    }

    DECLARE_MESSAGE_MAP()

private:
    CEdit    m_editAddr;
    CListBox m_listResults;
    std::vector<GeoResult> m_results;
    std::wstring m_initialQuery;

    void DoSearch()
    {
        CString addr;
        m_editAddr.GetWindowText(addr);
        addr.Trim();
        if (addr.IsEmpty()) return;

        m_listResults.ResetContent();
        m_results.clear();

        std::wstring query = UrlEncode((LPCWSTR)addr);
        std::wstring path  = L"/search?q=" + query + L"&format=json&limit=5&addressdetails=1";

        // Block briefly — acceptable in a modal dialog
        SetCursor(LoadCursor(nullptr, IDC_WAIT));
        std::wstring json = WinHttpGet(L"nominatim.openstreetmap.org", path);
        SetCursor(LoadCursor(nullptr, IDC_ARROW));

        if (json.empty()) {
            MessageBox(L"No results (check your internet connection).", L"WinLuach", MB_OK | MB_ICONWARNING);
            return;
        }

        m_results = ParseNominatimJson(json);
        if (m_results.empty()) {
            MessageBox(L"No results found.", L"WinLuach", MB_OK | MB_ICONINFORMATION);
            return;
        }

        for (const auto& r : m_results)
            m_listResults.AddString(r.displayName.c_str());

        m_listResults.SetCurSel(0);
    }
};

BEGIN_MESSAGE_MAP(CGeoDlg, CDialog)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CLocationDlg, CDialog)
    ON_EN_CHANGE(101, &CLocationDlg::OnSearchChange)
    ON_LBN_SELCHANGE(102, &CLocationDlg::OnListSelChange)
    ON_LBN_DBLCLK(102, &CLocationDlg::OnListDblClk)
    ON_BN_CLICKED(104, &CLocationDlg::OnBtnDelete)
    ON_BN_CLICKED(105, &CLocationDlg::OnBtnCustom)
    ON_BN_CLICKED(106, &CLocationDlg::OnBtnEdit)
END_MESSAGE_MAP()

// =============================================================================
// CCustomLocDlg — inline custom coordinates entry dialog
// =============================================================================

class CCustomLocDlg : public CDialog
{
public:
    LocationEntry result;

    explicit CCustomLocDlg(CWnd* pParent = nullptr) : CDialog()
    {
        m_pParentWnd = pParent;
        m_isEdit = false;
    }

    // Constructor for editing an existing entry.
    CCustomLocDlg(const LocationEntry& initial, CWnd* pParent = nullptr) : CDialog()
    {
        m_pParentWnd = pParent;
        m_initial    = initial;
        m_isEdit     = true;
        m_origName   = initial.loc.name;
    }

    virtual INT_PTR DoModal() override
    {
        struct DlgTmpl { DLGTEMPLATE t; WORD menu, cls; wchar_t title[32]; } buf = {};
        buf.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_CENTER;
        buf.t.cx = 320; buf.t.cy = 252;
        wcscpy_s(buf.title, m_isEdit ? L"Edit Location" : L"Add Custom Location");
        if (!InitModalIndirect((DLGTEMPLATE*)&buf, m_pParentWnd)) return -1;
        return CDialog::DoModal();
    }

private:
    bool          m_isEdit   = false;
    std::wstring  m_origName;
    LocationEntry m_initial;

protected:
    BOOL OnInitDialog() override
    {
        CDialog::OnInitDialog();
        SetWindowText(L"Add Custom Location");

        HFONT hF = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CFont* pF = CFont::FromHandle(hF);

        auto mkLabel = [&](const wchar_t* t, int x, int y, int w) {
            CStatic* s = new CStatic;
            s->Create(t, WS_CHILD | WS_VISIBLE | SS_RIGHT,
                      CRect(x, y + 2, x + w, y + 18), this);
            s->SetFont(pF);
        };
        auto mkEdit = [&](CEdit& e, UINT id, const wchar_t* v,
                          int x, int y, int w) {
            e.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                     CRect(x, y, x + w, y + 20), this, id);
            e.SetFont(pF);
            e.SetWindowText(v);
        };

        // Initial values (blank for Add, filled for Edit)
        const wchar_t* initName = m_isEdit ? m_initial.loc.name.c_str()    : L"";
        wchar_t  initLat[32]  = L"0.0",   initLon[32]  = L"0.0";
        wchar_t  initElev[32] = L"0",     initGMT[32]  = L"0";
        if (m_isEdit) {
            swprintf_s(initLat,  L"%.6f", m_initial.loc.latitude);
            swprintf_s(initLon,  L"%.6f", m_initial.loc.longitude);
            swprintf_s(initElev, L"%.0f", m_initial.loc.elevation);
            swprintf_s(initGMT,  L"%d",   m_initial.loc.gmtOffset);
        }

        int lw = 80, ex = 96, ew = 110, y = 10;
        mkLabel(L"Name:",      8, y, lw); mkEdit(m_eName, 301, initName,  ex, y, 144);
        {
            CButton* bGeo = new CButton;
            bGeo->Create(L"Lookup...", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                CRect(244, y, 296, y + 20), this, 308);
            bGeo->SetFont(pF);
        }
        y += 28;
        mkLabel(L"Latitude:",  8, y, lw); mkEdit(m_eLat,  302, initLat,  ex, y, ew);  y += 28;
        mkLabel(L"Longitude:", 8, y, lw); mkEdit(m_eLon,  303, initLon,  ex, y, ew);  y += 28;
        mkLabel(L"Elevation:", 8, y, lw); mkEdit(m_eElev, 304, initElev, ex, y, ew);  y += 28;
        mkLabel(L"GMT Offset:",8, y, lw); mkEdit(m_eGMT,  305, initGMT,  ex, y, 60);
        {
            CButton* bDet = new CButton;
            bDet->Create(L"Detect...", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                CRect(164, y, 238, y + 20), this, 309);
            bDet->SetFont(pF);
        }
        y += 28;

        m_chkDST.Create(L"Uses DST",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            CRect(ex, y, ex + 100, y + 20), this, 306);
        m_chkDST.SetFont(pF);
        bool dstInit = m_isEdit ? m_initial.loc.usesDST : true;
        m_chkDST.SetCheck(dstInit ? BST_CHECKED : BST_UNCHECKED);
        y += 26;

        m_chkIsrael.Create(L"Israel schedule",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            CRect(ex, y, ex + 130, y + 20), this, 307);
        m_chkIsrael.SetFont(pF);
        m_chkIsrael.SetCheck(m_isEdit && m_initial.isIsrael ? BST_CHECKED : BST_UNCHECKED);
        y += 32;

        CRect rc; GetClientRect(&rc);
        int H = rc.Height(), W = rc.Width();
        CButton* pOK = new CButton;
        pOK->Create(m_isEdit ? L"Save" : L"Add",
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                    CRect(W - 154, H - 34, W - 84, H - 8), this, IDOK);
        pOK->SetFont(pF);
        CButton* pCan = new CButton;
        pCan->Create(L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                     CRect(W - 78, H - 34, W - 8, H - 8), this, IDCANCEL);
        pCan->SetFont(pF);
        return TRUE;
    }

    BOOL OnCommand(WPARAM wParam, LPARAM lParam) override
    {
        if (LOWORD(wParam) == 308) { // Lookup Address
            CString sName; m_eName.GetWindowText(sName); sName.Trim();
            CGeoDlg geo(std::wstring((LPCWSTR)sName), this);
            if (geo.DoModal() == IDOK) {
                wchar_t buf[64];
                swprintf_s(buf, L"%.6f", geo.resultLat);
                m_eLat.SetWindowText(buf);
                swprintf_s(buf, L"%.6f", geo.resultLon);
                m_eLon.SetWindowText(buf);
                if (!geo.resultName.empty() && m_eName.GetWindowTextLength() == 0)
                    m_eName.SetWindowText(geo.resultName.c_str());
            }
            return TRUE;
        }
        if (LOWORD(wParam) == 309) { // Auto-detect timezone + elevation
            DoAutoDetect();
            return TRUE;
        }
        return CDialog::OnCommand(wParam, lParam);
    }

    void DoAutoDetect()
    {
        CString sLat, sLon;
        m_eLat.GetWindowText(sLat); sLat.Trim();
        m_eLon.GetWindowText(sLon); sLon.Trim();
        if (sLat.IsEmpty() || sLon.IsEmpty() ||
            (_wtof(sLat) == 0.0 && _wtof(sLon) == 0.0)) {
            MessageBox(L"Enter latitude and longitude first.", L"WinLuach", MB_OK | MB_ICONWARNING);
            return;
        }

        SetCursor(LoadCursor(nullptr, IDC_WAIT));

        // Timezone via timeapi.io
        std::wstring tzPath = L"/api/TimeZone/coordinate?latitude=" +
            std::wstring((LPCWSTR)sLat) + L"&longitude=" + std::wstring((LPCWSTR)sLon);
        std::wstring tzJson = WinHttpGet(L"timeapi.io", tzPath);
        if (!tzJson.empty()) {
            int gmtHours = ParseTimezoneOffset(tzJson);
            bool hasDst  = ParseHasDST(tzJson);
            wchar_t buf[16];
            swprintf_s(buf, L"%d", gmtHours);
            m_eGMT.SetWindowText(buf);
            m_chkDST.SetCheck(hasDst ? BST_CHECKED : BST_UNCHECKED);
        }

        // Elevation via open-elevation.com
        std::wstring elPath = L"/api/v1/lookup?locations=" +
            std::wstring((LPCWSTR)sLat) + L"," + std::wstring((LPCWSTR)sLon);
        std::wstring elJson = WinHttpGet(L"api.open-elevation.com", elPath);
        if (!elJson.empty()) {
            double elev = ParseElevation(elJson);
            wchar_t buf[32];
            swprintf_s(buf, L"%.0f", elev);
            m_eElev.SetWindowText(buf);
        }

        SetCursor(LoadCursor(nullptr, IDC_ARROW));

        if (tzJson.empty() && elJson.empty())
            MessageBox(L"Could not reach detection services. Check your internet connection.",
                L"WinLuach", MB_OK | MB_ICONWARNING);
    }

    void OnOK() override
    {
        CString sName, sLat, sLon, sElev, sGMT;
        m_eName.GetWindowText(sName);
        sName.Trim();
        if (sName.IsEmpty()) {
            MessageBox(L"Please enter a name.", L"WinLuach", MB_OK | MB_ICONWARNING);
            return;
        }
        m_eLat .GetWindowText(sLat);
        m_eLon .GetWindowText(sLon);
        m_eElev.GetWindowText(sElev);
        m_eGMT .GetWindowText(sGMT);

        double lat  = _wtof(sLat);
        double lon  = _wtof(sLon);
        double elev = _wtof(sElev);
        int    gmt  = _wtoi(sGMT);
        bool   dst  = (m_chkDST.GetCheck()    == BST_CHECKED);
        bool   israel = (m_chkIsrael.GetCheck() == BST_CHECKED);

        Location loc((LPCWSTR)sName, lat, lon, elev, gmt, dst);
        result = LocationEntry(loc, L"", L"", true);
        result.isIsrael = israel;
        CDialog::OnOK();
    }

    DECLARE_MESSAGE_MAP()

private:
    CEdit   m_eName, m_eLat, m_eLon, m_eElev, m_eGMT;
    CButton m_chkDST;
    CButton m_chkIsrael;
};

BEGIN_MESSAGE_MAP(CCustomLocDlg, CDialog)
END_MESSAGE_MAP()

// =============================================================================
// CONSTRUCTION
// =============================================================================

CLocationDlg::CLocationDlg(const Location& current, CWnd* pParent)
    : CDialog(), m_current(current), m_result(current)
{
    m_pParentWnd = pParent;
}

// =============================================================================
// DOMODAL — in-memory dialog template, no .rc file needed
// =============================================================================

INT_PTR CLocationDlg::DoModal()
{
    struct DlgTmpl {
        DLGTEMPLATE t;
        WORD menu;
        WORD cls;
        wchar_t title[32];
    } buf = {};

    buf.t.style = WS_POPUP | WS_CAPTION | WS_SYSMENU |
        WS_THICKFRAME | DS_CENTER;
    buf.t.dwExtendedStyle = 0;
    buf.t.cdit = 0;
    buf.t.x = 0;
    buf.t.y = 0;
    buf.t.cx = 380;
    buf.t.cy = 280;
    wcscpy_s(buf.title, L"Choose Location");

    if (!InitModalIndirect((DLGTEMPLATE*)&buf, m_pParentWnd))
        return -1;

    return CDialog::DoModal();
}



// =============================================================================
// ON INIT DIALOG — creates all controls
// =============================================================================

BOOL CLocationDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    SetWindowText(L"Choose Location");

    CRect rcClient;
    GetClientRect(&rcClient);
    int W = rcClient.Width();
    int H = rcClient.Height();

    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    CFont* pFont = CFont::FromHandle(hFont);

    // Helper: create a plain child control (10-arg CreateEx)
    auto Make = [&](const wchar_t* cls, const wchar_t* text,
        DWORD style, int x, int y, int w, int h, UINT id)
        {
            CWnd* p = new CWnd();
            p->CreateEx(0, cls, text, WS_CHILD | WS_VISIBLE | style,
                x, y, w, h, GetSafeHwnd(),
                (HMENU)(UINT_PTR)id);
            p->SetFont(pFont);
        };

    // Search label + edit box
    Make(L"STATIC", L"Search:", 0, 8, 8, 50, 20, 100);
    m_editSearch.Create(
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        CRect(62, 6, W - 10, 28), this, 101);
    m_editSearch.SetFont(pFont);

    // City list box
    m_listCities.Create(
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
        LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
        CRect(8, 34, W - 8, H - 80), this, 102);
    m_listCities.SetFont(pFont);

    // Coordinate display label
    m_lblCoords.Create(L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        CRect(8, H - 74, W - 8, H - 58), this, 103);
    m_lblCoords.SetFont(pFont);

    // Delete button (enabled only for custom locations)
    m_btnDelete.Create(L"Delete",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(8, H - 50, 80, H - 24), this, 104);
    m_btnDelete.SetFont(pFont);

    // Custom location button
    m_btnCustom.Create(L"Custom...",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(88, H - 50, 178, H - 24), this, 105);
    m_btnCustom.SetFont(pFont);

    // Edit custom location button (enabled only for custom entries)
    m_btnEdit.Create(L"Edit...",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        CRect(186, H - 50, 268, H - 24), this, 106);
    m_btnEdit.SetFont(pFont);
    m_btnEdit.EnableWindow(FALSE);

    // OK button
    CButton* btnOK = new CButton();
    btnOK->Create(L"OK",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        CRect(W - 144, H - 50, W - 76, H - 24), this, IDOK);
    btnOK->SetFont(pFont);

    // Cancel button
    CButton* btnCancel = new CButton();
    btnCancel->Create(L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(W - 70, H - 50, W - 8, H - 24), this, IDCANCEL);
    btnCancel->SetFont(pFont);

    // Populate list and pre-select current location
    PopulateList(L"");

    for (int i = 0; i < (int)m_filteredIndices.size(); i++)
    {
        const auto& e = LocationDB::Get().GetAll()[m_filteredIndices[i]];
        if (e.loc.name == m_current.name)
        {
            m_listCities.SetCurSel(i);
            m_listCities.SetTopIndex(max(0, i - 5));
            break;
        }
    }

    OnListSelChange();
    m_editSearch.SetFocus();
    return FALSE;
}

// =============================================================================
// POPULATE LIST
// =============================================================================

// Rebuilds the list box with all cities whose display name contains the filter.
void CLocationDlg::PopulateList(const CString& filter)
{
    m_listCities.ResetContent();
    m_filteredIndices.clear();

    CString lowerFilter = filter;
    lowerFilter.MakeLower();

    const auto& all = LocationDB::Get().GetAll();
    for (int i = 0; i < (int)all.size(); i++)
    {
        const auto& e = all[i];
        std::wstring disp = e.loc.name;
        if (!e.region.empty())  disp += L", " + e.region;
        if (!e.country.empty()) disp += L", " + e.country;
        if (e.isCustom)         disp += L" *";

        if (!lowerFilter.IsEmpty())
        {
            CString dispL = disp.c_str();
            dispL.MakeLower();
            if (dispL.Find(lowerFilter) < 0) continue;
        }

        m_listCities.AddString(disp.c_str());
        m_filteredIndices.push_back(i);
    }
}

// =============================================================================
// EVENT HANDLERS
// =============================================================================

// Filters the list as the user types in the search box.
void CLocationDlg::OnSearchChange()
{
    CString filter;
    m_editSearch.GetWindowText(filter);
    PopulateList(filter);
    if (m_listCities.GetCount() > 0)
        m_listCities.SetCurSel(0);
    OnListSelChange();
}

// Updates the coordinate display when the selection changes.
void CLocationDlg::OnListSelChange()
{
    const LocationEntry* e = GetSelectedEntry();
    if (!e)
    {
        m_lblCoords.SetWindowText(L"");
        m_btnDelete.EnableWindow(FALSE);
        m_btnEdit.EnableWindow(FALSE);
        return;
    }

    CString coords;
    coords.Format(L"Lat: %.4f  Lon: %.4f  GMT%+d  DST: %s",
        e->loc.latitude, e->loc.longitude,
        e->loc.gmtOffset,
        e->loc.usesDST ? L"Yes" : L"No");
    m_lblCoords.SetWindowText(coords);
    m_btnDelete.EnableWindow(e->isCustom);
    m_btnEdit.EnableWindow(e->isCustom);
}

// Double-click accepts the selection.
void CLocationDlg::OnListDblClk()
{
    OnOK();
}

// Copies the selected location to m_result and closes the dialog.
void CLocationDlg::OnOK()
{
    const LocationEntry* e = GetSelectedEntry();
    if (!e)
    {
        MessageBox(L"Please select a location.", L"WinLuach", MB_OK);
        return;
    }
    m_result = e->loc;
    m_selectedIsIsrael = e->isIsrael;
    m_isCustomSelected = e->isCustom;
    CDialog::OnOK();
}

// Deletes the selected custom location after confirmation.
void CLocationDlg::OnBtnDelete()
{
    const LocationEntry* e = GetSelectedEntry();
    if (!e || !e->isCustom) return;

    CString msg;
    msg.Format(L"Delete \"%s\"?", e->loc.name.c_str());
    if (MessageBox(msg, L"Confirm", MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        LocationDB::Get().DeleteCustom(e->loc.name);
        CString filter;
        m_editSearch.GetWindowText(filter);
        PopulateList(filter);
        OnListSelChange();
    }
}

// =============================================================================
// GET SELECTED ENTRY
// =============================================================================

const LocationEntry* CLocationDlg::GetSelectedEntry() const
{
    int sel = m_listCities.GetCurSel();
    if (sel == LB_ERR || sel >= (int)m_filteredIndices.size())
        return nullptr;
    int idx = m_filteredIndices[sel];
    const auto& all = LocationDB::Get().GetAll();
    if (idx < 0 || idx >= (int)all.size()) return nullptr;
    return &all[idx];
}

// =============================================================================
// CUSTOM LOCATION
// =============================================================================

void CLocationDlg::OnBtnCustom()
{
    CCustomLocDlg dlg(this);
    if (dlg.DoModal() != IDOK) return;

    const LocationEntry& entry = dlg.result;
    if (!LocationDB::Get().AddCustom(entry))
    {
        MessageBox(L"A location with that name already exists.",
                   L"WinLuach", MB_OK | MB_ICONWARNING);
        return;
    }

    CString filter;
    m_editSearch.GetWindowText(filter);
    PopulateList(filter);

    const auto& all = LocationDB::Get().GetAll();
    for (int i = 0; i < (int)m_filteredIndices.size(); i++)
    {
        if (all[m_filteredIndices[i]].loc.name == entry.loc.name)
        {
            m_listCities.SetCurSel(i);
            m_listCities.SetTopIndex(max(0, i - 5));
            break;
        }
    }
    OnListSelChange();
}

void CLocationDlg::OnBtnEdit()
{
    const LocationEntry* e = GetSelectedEntry();
    if (!e || !e->isCustom) return;

    std::wstring oldName = e->loc.name;
    CCustomLocDlg dlg(*e, this);
    if (dlg.DoModal() != IDOK) return;

    const LocationEntry& updated = dlg.result;
    if (!LocationDB::Get().UpdateCustom(oldName, updated))
    {
        MessageBox(L"Could not update location.", L"WinLuach", MB_OK | MB_ICONWARNING);
        return;
    }

    CString filter;
    m_editSearch.GetWindowText(filter);
    PopulateList(filter);

    const auto& all = LocationDB::Get().GetAll();
    for (int i = 0; i < (int)m_filteredIndices.size(); i++)
    {
        if (all[m_filteredIndices[i]].loc.name == updated.loc.name)
        {
            m_listCities.SetCurSel(i);
            m_listCities.SetTopIndex(max(0, i - 5));
            break;
        }
    }
    OnListSelChange();
}
