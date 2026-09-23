// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    BackupViewDlg.h
// Purpose: Read-only viewer that lists everything stored in a backup file
//          (preferences, personal events, custom locations) in plain words,
//          grouped into sections.
// =============================================================================
//
// CHANGELOG:
// v0.8.133 - Initial file.
// =============================================================================

#pragma once
#include "Settings.h"
#include <afxcmn.h>
#include <string>

class CBackupViewDlg : public CDialog
{
public:
    CBackupViewDlg(const std::wstring& path, const BackupContents& contents, CWnd* pParent = nullptr);

    INT_PTR DoModal() override;

protected:
    BOOL OnInitDialog() override;
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* mmi);
    DECLARE_MESSAGE_MAP()

private:
    void Layout();

    std::wstring   m_path;
    BackupContents m_contents;
    CStatic        m_file;
    CStatic        m_header;
    CListCtrl      m_list;
    CButton        m_close;
};
