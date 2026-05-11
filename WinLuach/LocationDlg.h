// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    LocationDlg.h
// Purpose: MFC location picker dialog.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial MFC CDialog location picker.
// v0.1.1 - Added DoModal() override using InitModalIndirect.
// v0.1.2 - Moved DoModal() to public so MainFrame can call it.
// =============================================================================

#pragma once
#include "pch.h"
#include "Location.h"

class CLocationDlg : public CDialog
{
public:
    CLocationDlg(const Location& current, CWnd* pParent = nullptr);

    // Returns the location selected by the user (valid after DoModal()==IDOK)
    Location GetSelectedLocation() const { return m_result; }
    bool GetSelectedIsIsrael() const { return m_selectedIsIsrael; }
    bool GetSelectedIsCustom() const { return m_isCustomSelected; }

    // Creates the dialog using an in-memory template (no .rc file needed)
    virtual INT_PTR DoModal() override;

protected:
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override;

    afx_msg void OnSearchChange();
    afx_msg void OnListSelChange();
    afx_msg void OnListDblClk();
    afx_msg void OnBtnDelete();
    afx_msg void OnBtnCustom();
    afx_msg void OnBtnEdit();

    DECLARE_MESSAGE_MAP()

private:
    void PopulateList(const CString& filter);
    const LocationEntry* GetSelectedEntry() const;

    Location  m_current;
    Location  m_result;
    bool      m_selectedIsIsrael = false;
    bool      m_isCustomSelected = false;

    std::vector<int> m_filteredIndices;

    CEdit     m_editSearch;
    CListBox  m_listCities;
    CStatic   m_lblCoords;
    CButton   m_btnDelete;
    CButton   m_btnCustom;
    CButton   m_btnEdit;
};