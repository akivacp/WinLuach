// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    OptionsDlg.h
// Purpose: MFC preferences dialog (clock format, Israel/Diaspora, shita).
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial MFC CDialog options dialog.
// v0.1.1 - Added DoModal() override using InitModalIndirect.
// v0.1.2 - Moved DoModal() to public so MainFrame can call it.
// =============================================================================

#pragma once
#include "pch.h"
#include "Settings.h"

class COptionsDlg : public CDialog
{
public:
    COptionsDlg(const AppSettings& current, CWnd* pParent = nullptr);

    // Returns updated settings (valid after DoModal()==IDOK)
    AppSettings GetSettings() const { return m_result; }

    // Creates the dialog using an in-memory template (no .rc file needed)
    virtual INT_PTR DoModal() override;

protected:
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override;
    afx_msg void OnTrayTextColor();
    afx_msg void OnManageCals();
    afx_msg void OnPreviewNotification();

    DECLARE_MESSAGE_MAP()

private:
    AppSettings m_current;
    AppSettings m_result;

    CButton m_radAmPm;
    CButton m_rad24hr;
    CButton m_radDiaspora;
    CButton m_radIsrael;
    CButton m_radGRA;
    CButton m_radMA72;
    CButton m_radMA90;

    CButton m_radCivilMonth;
    CButton m_radHebrewMonth;
    CButton m_chkDateTracking;
    CButton m_chkParshios;
    CButton m_chkMoadim;
    CButton m_chkUserEvents;
    CButton m_chkDafYomi;
    CButton m_chkYerushalmi;
    CButton m_chkHalacha;
    CButton m_chkMishna;
    CButton m_chkTanach;
    CButton m_chkShowTrayIcon;
    CButton m_chkMinimizeToTray;
    CButton m_chkMinimizeOnStartup;
    CButton m_chkStartWithWindows;
    CButton m_chkDesktopShortcut;
    CButton m_btnTrayTextColor;
    CComboBox m_cmbFontSize;
    CComboBox m_cmbTrayWhen;
    CComboBox m_cmbCandleMinutes;
    CButton   m_btnManageCals;
    CButton   m_btnPreviewNotify;
    CComboBox m_cmbNotifyPersonal;
    CComboBox m_cmbNotifyWebCal;
    CComboBox m_cmbAlotShita;
    CComboBox m_cmbTzeitShita;
};
