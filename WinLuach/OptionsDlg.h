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
// v0.8.0 - Added "Zmanim Bar" tab + Zmanim sub-tab structure (Alos,
//          Misheyakir, Sof Zman MA, Sof Zman GRA, Mincha Gedola/Ketana,
//          Plag, End of Fast, Tzais).  Zman Shitos tab merged in.
// v0.8.1 - Fixed invisible Zmanim sub-page contents (raise widgets to top
//          of z-order in ShowZmanimSubPage so the sibling sub-tab control
//          does not overpaint them).
//        - Fixed Omer notification 'manual time' radio being clipped by
//          the adjacent 'manual:' static label.
// v0.8.2 - Fixed missing Zmanim sub-tab strip ("Alos | Misheyakir | ...").
//          Added WS_CLIPSIBLINGS to main tab + sub-tab so they don't paint
//          over each other, plus explicit Invalidate/UpdateWindow and a
//          z-order lift when showing the sub-tab.
// =============================================================================

#pragma once
#include "pch.h"
#include "Settings.h"

// Opens the Manage Calendars dialog; updates webCalendars in-place and returns true if changed.
bool ShowManageCalendarsDialog(std::vector<WebCalEntry>& calendars, CWnd* pParent);

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
    afx_msg void OnTrayBackColor();
    afx_msg void OnTrayFont();
    afx_msg void OnTrayDefaults();
    afx_msg void OnManageCals();
    afx_msg void OnPreviewNotification();
    afx_msg void OnAdvancedReminders();
    afx_msg void OnTabChanged(NMHDR* pNMHDR, LRESULT* pResult);
    // v0.8.0 - Notification handler for the Zmanim sub-tab control.
    afx_msg void OnZmanimSubTabChanged(NMHDR* pNMHDR, LRESULT* pResult);
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
    CButton m_chkTrayBackEnabled;
    CButton m_btnTrayBackColor;
    CButton m_btnTrayFont;
    CComboBox m_cmbTrayNumber;
    CButton m_btnTrayDefaults;
    CComboBox m_cmbFontSize;
    CButton   m_btnResetCalendar;
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
    CComboBox m_cmbMisheyakirShita;
    CEdit     m_editCustomMisheyakir;
    CButton   m_radMisheyakirDegrees;
    CButton   m_radMisheyakirMinutes;
    CButton   m_radMisheyakirZmanis;
    CComboBox m_cmbSofZmanShita;
    CEdit     m_editCustomSofZman;
    CButton   m_radSofZmanDegrees;
    CButton   m_radSofZmanMinutes;
    CButton   m_radSofZmanZmanis;
    CEdit     m_editCustomTzeit;
    CButton   m_radTzeitDegrees;
    CButton   m_radTzeitMinutes;
    CButton   m_radTzeitZmanis;

    std::vector<CWnd*> m_pageGeneral;
    std::vector<CWnd*> m_pageMonth;
    std::vector<CWnd*> m_pageInterface;
    std::vector<CWnd*> m_pageTrayTooltip;
    std::vector<CWnd*> m_pageColors;
    std::vector<CWnd*> m_pageZmanim;
    std::vector<CWnd*> m_pageZmanShitos;        // legacy; retained for compatibility but not used after v0.8.0
    std::vector<CWnd*> m_pageNotifications;
    std::vector<CWnd*> m_pageYearDetails;
    // v0.8.0 - new Zmanim Bar tab + Zmanim sub-pages.
    std::vector<CWnd*> m_pageZmanimBar;
    std::vector<CWnd*> m_subPageAlos;
    std::vector<CWnd*> m_subPageMisheyakir;
    std::vector<CWnd*> m_subPageSofMA;
    std::vector<CWnd*> m_subPageSofGRA;
    std::vector<CWnd*> m_subPageMinchaGedola;
    std::vector<CWnd*> m_subPageMinchaKetana;
    std::vector<CWnd*> m_subPagePlag;
    std::vector<CWnd*> m_subPageEndFast;
    std::vector<CWnd*> m_subPageTzais;
    CTabCtrl m_zmanimSubTab;
    std::vector<CButton*> m_zmanimBarChecks;     // one per kZmanimBarLabels entry
    // Sub-tab preset radio groups (created dynamically). We keep pointers so
    // ReadControlsIntoResult can ask "which radio is checked" on each tab.
    std::vector<CButton*> m_radAlosPreset;       // 4 entries
    std::vector<CButton*> m_radMisheyakirPreset; // 4 entries
    std::vector<CButton*> m_radSofMAPreset;      // 4 entries
    std::vector<CButton*> m_radSofGRAPreset;     // 4 entries
    std::vector<CButton*> m_radMinchaGedolaPreset; // 4 entries (incl. Custom)
    std::vector<CButton*> m_radMinchaKetanaPreset; // 3 entries
    std::vector<CButton*> m_radPlagPreset;       // 3 entries
    std::vector<CButton*> m_radEndFastPreset;    // 3 entries
    std::vector<CButton*> m_radTzaisPreset;      // 6 entries
    CEdit m_editCustomMinchaGedola;
    CEdit m_editCustomMinchaKetana;
    CEdit m_editCustomPlag;
    CEdit m_editCustomEndFast;
    CButton m_btnOK;
    CButton m_btnCancel;
    CButton m_btnApply;

    void ReadControlsIntoResult();
    bool ApplyToParent();
    void ShowOptionsPage(int page);
    // v0.8.0 - Show only one of the Zmanim sub-pages; hide the rest.
    void ShowZmanimSubPage(int sub);
    // v0.8.0 - Helper that hides every Zmanim sub-page + the sub-tab.
    void HideAllZmanimSubPages();
    void UpdateColorButtons();
    bool ChooseCalendarColor(UINT id);
    void RestoreDefaultCalendarColors();
    void UpdateColorEditorFromSelection();
    void ApplyColorHexFromEditor();
    void SetColorPreviewForPref(int colorIndex);
    void InvalidateColorPreview();
    int CurrentColorIndex() const;
    int CurrentAlotMode() const;
    int CurrentMisheyakirMode() const;
    int CurrentSofZmanMode() const;
    int CurrentTzeitMode() const;
    void SetAlotMode(int mode);
    void SetMisheyakirMode(int mode);
    void SetSofZmanMode(int mode);
    void SetTzeitMode(int mode);
    double GetEditValue(const CEdit& edit, double fallback) const;
    void SetEditValue(CEdit& edit, double value);
    void ApplyAlotPreset();
    void ApplyMisheyakirPreset();
    void ApplySofZmanPreset();
    void ApplyTzeitPreset();
    void ConvertAlotMode(int newMode);
    void ConvertMisheyakirMode(int newMode);
    void ConvertSofZmanMode(int newMode);
    void ConvertTzeitMode(int newMode);
    void OnResetCalendarSettings();

    CComboBox m_cmbColorItem;
    CEdit     m_editColorHex;
    CButton   m_btnColorPicker;
    CComboBox m_cmbColorPreview;
    CColorPreviewWnd m_colorPreview;
    CButton   m_btnRestoreColors;
    bool      m_updatingColorUi = false;
    int       m_lastAlotMode = 0;
    int       m_lastMisheyakirMode = 0;
    int       m_lastSofZmanMode = 0;
    int       m_lastTzeitMode = 0;

    CButton   m_chkYearDetails[12];
    CButton   m_chkTrayTooltipCustomZmanim[7];
    CComboBox m_cmbNotifyZmanim;
    CButton   m_chkNotifyZmanim[24];
    CButton   m_chkTrayTooltipZmanim[31];
    CComboBox m_cmbNotifySefirah;
    CComboBox m_cmbNotifySefirahHour;
    CComboBox m_cmbNotifySefirahMinute;
    CComboBox m_cmbNotifySefirahAmPm;
    CButton   m_radNotifySefirahSunTzais;
    CButton   m_radNotifySefirahOther;
    CButton   m_radNotifySefirahManual;
    CEdit     m_editNotifySefirahOffset;
    CComboBox m_cmbNotifySefirahDir;
    CComboBox m_cmbNotifySefirahBase;
    CComboBox m_cmbNotifySefirahOtherZman;
    CComboBox m_cmbNotifyMoadim;
    CEdit     m_editNotifyMoadimAmount;
    CComboBox m_cmbNotifyMoadimUnit;
    CComboBox m_cmbNotifyParsha;
    CComboBox m_cmbNotifyParshaStyle;
    CEdit     m_editNotifyParshaAmount;
    CComboBox m_cmbNotifyParshaUnit;
    CEdit     m_editNotifyPersonalAmount;
    CComboBox m_cmbNotifyPersonalUnit;
    CButton   m_btnAdvancedReminders;
    void UpdateNotificationControls();
    void SetDirty(bool dirty);

    bool      m_modeless = false;
    bool      m_initialized = false;
    bool      m_dirty = false;
    CToolTipCtrl m_tooltip;

    virtual BOOL PreTranslateMessage(MSG* pMsg) override;
};
