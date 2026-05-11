// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    MainFrame.h
// Purpose: Declares the MFC main frame window (CMainFrame).
//          CMainFrame is a CFrameWnd that hosts the calendar view,
//          sidebar panel, and zmanim panel. It owns the menu bar
//          and toolbar, and coordinates all child windows.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial MFC main frame. Hosts CCalendarView, CSidebarPanel,
//          CZmanimPanel. Menu: File, View, Options, Help.
// v0.1.1 - Moved fonts to public section so child panels can access them.
// =============================================================================

#pragma once
#include "pch.h"
#include "HebrewDate.h"
#include "Zmanim.h"
#include "Location.h"
#include "HolidayEngine.h"
#include "Settings.h"
#include <shellapi.h>

// Forward declarations
class CCalendarView;
class CSidebarPanel;
class CZmanimPanel;

struct UserEventInfo
{
    GregorianDate date;
    std::wstring  title;
};

// =============================================================================
// LAYOUT CONSTANTS
// =============================================================================

#define SIDEBAR_W       190    // Day details panel width (pixels)
#define ZMANIM_H        110    // Zmanim panel height (pixels)
#define HEADER_H         84    // Month/year header height
#define DAY_HDR_H        24    // Sun/Mon/... row height

// =============================================================================
// CELL COLORS (COLORREF = 0x00BBGGRR)
// =============================================================================

#define CLR_TODAY           RGB(255, 255, 150)
#define CLR_SHABBOS         RGB(180, 230, 180)
#define CLR_YOM_TOV         RGB(255, 200, 130)
#define CLR_ROSH_CHODESH    RGB(180, 210, 255)
#define CLR_CHOL_HAMOED     RGB(255, 230, 180)
#define CLR_FAST_DAY        RGB(220, 200, 220)
#define CLR_NORMAL          RGB(255, 255, 255)
#define CLR_OTHER_MONTH     RGB(240, 240, 240)
#define CLR_HEADER_BG       RGB( 50,  80, 140)
#define CLR_HEADER_TXT      RGB(255, 255, 255)
#define CLR_ZMANIM_BG       RGB(245, 245, 220)
#define CLR_SIDEBAR_BG      RGB(235, 240, 250)
#define CLR_HOLIDAY_TXT     RGB(180,  30,  30)
#define CLR_HEBREW_TXT      RGB( 30,  30, 180)
#define CLR_GREG_TXT        RGB( 50,  50,  50)
#define CLR_GRID            RGB(180, 180, 180)

// =============================================================================
// MENU / COMMAND IDs
// =============================================================================

#define ID_VIEW_CIVIL       1001
#define ID_VIEW_HEBREW      1002
#define ID_VIEW_TODAY       1003
#define ID_VIEW_PREVMONTH   1004
#define ID_VIEW_NEXTMONTH   1005
#define ID_OPTIONS_LOCATION 1006
#define ID_OPTIONS_PREFS    1007
#define ID_HELP_ABOUT       1008
#define ID_VIEW_PREVDAY     1009
#define ID_VIEW_NEXTDAY     1010
#define ID_VIEW_PREVYEAR    1011
#define ID_VIEW_NEXTYEAR    1012
#define ID_VIEW_PREVDECADE  1013
#define ID_VIEW_NEXTDECADE  1014
#define ID_TRAY_OPEN        1015
#define ID_TRAY_ABOUT       1016
#define ID_TRAY_EXIT        1017
#define ID_CAL_PRINT        1018
#define ID_CAL_PREVIEW      1019
#define ID_FILE_BACKUP      1020
#define ID_FILE_RESTORE     1021
#define ID_VIEW_ZOOM_IN     1022
#define ID_VIEW_ZOOM_OUT    1023
#define ID_VIEW_ZOOM_RESET  1024
#define ID_CAL_GOTO         1025
#define IDC_MONTH_COMBO     2002
#define IDC_YEAR_EDIT       2003
#define IDC_YEAR_SPIN       2004
#define WM_WINLUACH_TRAY    (WM_APP + 101)
#define WM_WEBCAL_DONE      (WM_APP + 102)

// =============================================================================
// CMainFrame
// =============================================================================

class CMainFrame : public CFrameWnd
{
public:
    CMainFrame();
    virtual ~CMainFrame();

    // Creates the window — called from WinLuachApp::InitInstance
    BOOL Create();

    // Navigates to the previous month
    void PrevMonth();

    // Navigates to the next month
    void NextMonth();

    // Jumps to today
    void GoToToday();

    // Sets the selected date and refreshes all panels
    void SelectDate(const GregorianDate& g);

    // Opens the location picker dialog
    void OnPickLocation();

    // Opens the options/preferences dialog
    void OnPickOptions();

    std::vector<std::wstring> GetUserEventsForDate(const GregorianDate& g) const;

    // Returns {candleStr, motzStr} zmanim labels for a calendar cell.
    std::pair<std::wstring, std::wstring> GetCellZmanimLabels(
        const GregorianDate& g, const HebrewDate& h,
        const std::vector<HolidayInfo>& hols) const;

    // Current view state (public so child panels can read it)
    int           m_viewYear = 2026;
    int           m_viewMonth = 5;
    int           m_viewHebrewYear = 5786;
    int           m_viewHebrewMonth = IYAR;
    GregorianDate m_selectedDate;
    HebrewDate    m_selectedHebrew;
    GregorianDate m_today;
    HebrewDate    m_todayHebrew;
    Location      m_location;
    bool          m_isDST = false;
    bool          m_use24hr = false;
    bool          m_isIsrael = false;
    bool          m_hebrewMonthView = false;
    bool          m_showParshios = true;
    bool          m_showMoadim = true;
    bool          m_showDafYomi = true;
    bool          m_showYerushalmi = false;
    bool          m_showHalachaYomit = true;
    bool          m_showMishnaYomit = true;
    bool          m_showTanachYomi = true;
    bool          m_minimizeToTray = false;
    int           m_minimizeTrayWhen = 0;
    ZmanimResult  m_zmanim;

    // Fonts — public so child panels (CalendarView, SidebarPanel, ZmanimPanel)
    // can use them directly without accessors
    CFont  m_fontNormal;
    CFont  m_fontBold;
    CFont  m_fontSmall;
    CFont  m_fontHeader;

    // Navigation buttons in the header bar
    CButton m_btnPrevDecade;
    CButton m_btnPrevYear;
    CButton m_btnPrevMonth;
    CButton m_btnPrevDay;
    CButton m_btnToday;
    CButton m_btnNextDay;
    CButton m_btnNextMonth;
    CButton m_btnNextYear;
    CButton m_btnNextDecade;
    CButton m_btnPrint;

    // Month/year picker: combo + year edit + spin
    CComboBox          m_comboMonth;
    CEdit              m_editYear;
    CSpinButtonCtrl    m_spinYear;
    bool               m_updatingControls = false;

protected:
    // MFC message handlers
    afx_msg int  OnCreate(LPCREATESTRUCT lpcs);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnPaint();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg LRESULT OnTrayNotify(WPARAM wParam, LPARAM lParam);
    afx_msg void OnViewPrevDay();
    afx_msg void OnViewNextDay();
    afx_msg void OnViewToday();
    afx_msg void OnViewPrevMonth();
    afx_msg void OnViewNextMonth();
    afx_msg void OnViewPrevYear();
    afx_msg void OnViewNextYear();
    afx_msg void OnViewPrevDecade();
    afx_msg void OnViewNextDecade();
    afx_msg void OnOptionsLocation();
    afx_msg void OnOptionsPrefs();
    afx_msg void OnHelpAbout();
    afx_msg void OnTrayExit();
    afx_msg void OnDestroy();
    afx_msg void OnClose();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnMonthComboChange();
    afx_msg void OnYearSpinDelta(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnCalPrint();
    afx_msg void OnCalPreview();
    afx_msg void OnFileBackup();
    afx_msg void OnFileRestore();
    afx_msg void OnViewZoomIn();
    afx_msg void OnViewZoomOut();
    afx_msg void OnViewZoomReset();
    afx_msg void OnCalGoTo();
    afx_msg LRESULT OnWebCalDone(WPARAM, LPARAM);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    virtual BOOL PreTranslateMessage(MSG* pMsg);

    // Recalculates zmanim for the selected date
    void RefreshZmanim();

    // Repositions all child panels after a resize
    void LayoutPanels(int cx, int cy);
    void ApplySettings(const AppSettings& s);
    void SyncViewToSelected();
    void ChangeDay(int deltaDays);
    void ChangeMonth(int deltaMonths);
    void ChangeYear(int deltaYears);
    void RefreshWebCalendarEvents();
    bool ShouldHideToTrayOnClose() const;
    void HideToTray();
    void AddTrayIcon();
    void RemoveTrayIcon();
    void UpdateTrayIcon();
    HebrewDate CurrentHebrewDateForTray();
    std::wstring HebrewDayLetters(int day) const;
    HICON CreateTrayDateIcon(const HebrewDate& h) const;
    void RestoreFromTray();

    // Draws the month/year header bar
    void DrawHeader(CDC* pDC, const CRect& rc);

    // Draws the Sun/Mon/.../Shabbos column labels
    void DrawDayHeaders(CDC* pDC, const CRect& rc);

    // Recreates all four fonts from the current fontSize setting
    void RecreateFonts();

    // Repopulates the month combo for the current calendar mode
    void PopulateMonthCombo();
    // Syncs the month combo and year edit to the current view
    void UpdateMonthYearControls();

    // Child panels
    CCalendarView* m_pCalView = nullptr;
    CSidebarPanel* m_pSidebar = nullptr;
    CZmanimPanel* m_pZmanim = nullptr;
    NOTIFYICONDATA m_trayIcon = {};
    bool m_isInTray = false;
    HICON m_hTrayDateIcon = nullptr;
    std::vector<UserEventInfo> m_webEvents;
    std::wstring               m_icsPath;

    DECLARE_MESSAGE_MAP()
};
