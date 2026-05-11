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

BEGIN_MESSAGE_MAP(CLocationDlg, CDialog)
    ON_EN_CHANGE(101, &CLocationDlg::OnSearchChange)
    ON_LBN_SELCHANGE(102, &CLocationDlg::OnListSelChange)
    ON_LBN_DBLCLK(102, &CLocationDlg::OnListDblClk)
    ON_BN_CLICKED(104, &CLocationDlg::OnBtnDelete)
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
        DS_MODALFRAME | DS_CENTER;
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
        return;
    }

    CString coords;
    coords.Format(L"Lat: %.4f  Lon: %.4f  GMT%+d  DST: %s",
        e->loc.latitude, e->loc.longitude,
        e->loc.gmtOffset,
        e->loc.usesDST ? L"Yes" : L"No");
    m_lblCoords.SetWindowText(coords);
    m_btnDelete.EnableWindow(e->isCustom);
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

// Returns the LocationEntry for the currently highlighted list item, or nullptr.
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