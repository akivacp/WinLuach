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

class CColorPreviewWnd : public CWnd
{
public:
    void SetPreview(AppSettings* settings, int previewKind);

protected:
    afx_msg void OnPaint();
    DECLARE_MESSAGE_MAP()

private:
    AppSettings* m_settings = nullptr;
    int m_previewKind = 0;
};

class COptionsDlg : public CDialog
{
public:
    COptionsDlg(const AppSettings& current, CWnd* pParent = nullptr);

    // Returns updated settings (valid after DoModal()==IDOK)
    AppSettings GetSettings() const { return m_result; }

    // Creates the dialog using an in-memory template (no .rc file needed)
    virtual INT_PTR DoModal() override;
    BOOL CreateModeless(CWnd* pParent);

protected:
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override;
    virtual void OnCancel() override;
    virtual void PostNcDestroy() override;
    afx_msg void OnApply();
    afx_msg void OnTrayTextColor();
    afx_msg void OnManageCals();
    afx_msg void OnPreviewNotification();
    afx_msg void OnAdvancedReminders();
    afx_msg void OnTabChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;

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
    CTabCtrl m_tab;

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
    CButton   m_chkChatzosOnFasts;
    CEdit     m_editCustomAlot;
    CButton   m_radAlotDegrees;
    CButton   m_radAlotMinutes;
    CButton   m_radAlotZmanis;
    CEdit     m_editCustomTzeit;
    CButton   m_radTzeitDegrees;
    CButton   m_radTzeitMinutes;
    CButton   m_radTzeitZmanis;

    std::vector<CWnd*> m_pageGeneral;
    std::vector<CWnd*> m_pageMonth;
    std::vector<CWnd*> m_pageInterface;
    std::vector<CWnd*> m_pageColors;
    std::vector<CWnd*> m_pageZmanim;
    std::vector<CWnd*> m_pageNotifications;
    CButton m_btnOK;
    CButton m_btnCancel;
    CButton m_btnApply;

    void ReadControlsIntoResult();
    bool ApplyToParent();
    void ShowOptionsPage(int page);
    void UpdateColorButtons();
    bool ChooseCalendarColor(UINT id);
    void RestoreDefaultCalendarColors();
    void UpdateColorEditorFromSelection();
    void ApplyColorHexFromEditor();
    void SetColorPreviewForPref(int colorIndex);
    void InvalidateColorPreview();
    int CurrentColorIndex() const;
    int CurrentAlotMode() const;
    int CurrentTzeitMode() const;
    void SetAlotMode(int mode);
    void SetTzeitMode(int mode);
    double GetEditValue(const CEdit& edit, double fallback) const;
    void SetEditValue(CEdit& edit, double value);
    void ApplyAlotPreset();
    void ApplyTzeitPreset();
    void ConvertAlotMode(int newMode);
    void ConvertTzeitMode(int newMode);

    CComboBox m_cmbColorItem;
    CEdit     m_editColorHex;
    CButton   m_btnColorPicker;
    CComboBox m_cmbColorPreview;
    CColorPreviewWnd m_colorPreview;
    CButton   m_btnRestoreColors;
    bool      m_updatingColorUi = false;
    int       m_lastAlotMode = 0;
    int       m_lastTzeitMode = 0;

    CComboBox m_cmbNotifyZmanim;
    CButton   m_chkNotifyZmanim[15];
    CComboBox m_cmbNotifyMoadim;
    CEdit     m_editNotifyMoadimOffsets;
    CComboBox m_cmbNotifyParsha;
    CComboBox m_cmbNotifyParshaStyle;
    CEdit     m_editNotifyParshaOffsets;
    CEdit     m_editNotifyPersonalOffsets;
    CButton   m_btnAdvancedReminders;
    void UpdateNotificationControls();
    void SetDirty(bool dirty);

    bool      m_modeless = false;
    bool      m_initialized = false;
    bool      m_dirty = false;
};
