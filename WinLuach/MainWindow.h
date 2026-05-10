// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    MainWindow.h
// Purpose: Declares the main application window.
//          Owns the calendar grid, day details sidebar,
//          zmanim panel, and toolbar.
//          Connects the engine layer to the UI layer.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial file. Main window with calendar grid,
//          day details sidebar, zmanim panel, toolbar.
//          Color scheme: yellow=today, green=Shabbos,
//          orange=Yom Tov, blue=Rosh Chodesh.
// =============================================================================

#pragma once
#include "HebrewDate.h"
#include "Zmanim.h"
#include "Location.h"
#include "HolidayEngine.h"
#include <windows.h>
#include <string>
#include <vector>

// =============================================================================
// COLORS
// All calendar cell colors. Defined here so they can be
// changed in one place later when we add a preferences dialog.
// =============================================================================

// Cell background colors (COLORREF = 0x00BBGGRR)
#define CLR_TODAY           RGB(255, 255, 150)  // Yellow
#define CLR_SHABBOS         RGB(180, 230, 180)  // Green
#define CLR_YOM_TOV         RGB(255, 200, 130)  // Orange
#define CLR_ROSH_CHODESH    RGB(180, 210, 255)  // Blue
#define CLR_CHOL_HAMOED     RGB(255, 230, 180)  // Light orange
#define CLR_FAST            RGB(220, 200, 220)  // Light purple
#define CLR_NORMAL          RGB(255, 255, 255)  // White
#define CLR_OTHER_MONTH     RGB(240, 240, 240)  // Light grey
#define CLR_HEADER          RGB( 50,  80, 140)  // Dark blue header
#define CLR_HEADER_TEXT     RGB(255, 255, 255)  // White header text
#define CLR_ZMANIM_BG       RGB(245, 245, 220)  // Cream zmanim panel
#define CLR_SIDEBAR_BG      RGB(235, 240, 250)  // Light blue sidebar
#define CLR_HOLIDAY_TEXT    RGB(180,  30,  30)  // Dark red holiday names
#define CLR_HEBREW_TEXT     RGB( 30,  30, 180)  // Dark blue Hebrew date
#define CLR_GREG_TEXT       RGB( 50,  50,  50)  // Dark grey Gregorian date
#define CLR_SHABBOS_TEXT    RGB( 30, 100,  30)  // Dark green Shabbos text
#define CLR_GRID_LINE       RGB(180, 180, 180)  // Grid lines

// =============================================================================
// LAYOUT CONSTANTS
// =============================================================================

#define SIDEBAR_WIDTH       190     // Day details panel width (pixels)
#define TOOLBAR_HEIGHT       44     // Toolbar height
#define HEADER_HEIGHT        56     // Month/year header height
#define DAY_HEADER_HEIGHT    24     // Sun/Mon/Tue... row height
#define CELL_HEIGHT          80     // Calendar cell height
#define ZMANIM_HEIGHT       110     // Zmanim panel height at bottom
#define SHABBOS_COL_EXTRA    10     // Shabbos column slightly wider

// =============================================================================
// MAIN WINDOW CLASS
// =============================================================================

class MainWindow
{
public:
    // -------------------------------------------------------------------------
    // Creates and registers the window class, creates the main window.
    // Call once from WinMain.
    // -------------------------------------------------------------------------
    static bool Create(HINSTANCE hInstance, int nCmdShow);

    // -------------------------------------------------------------------------
    // Returns the singleton HWND of the main window.
    // -------------------------------------------------------------------------
    static HWND GetHWND() { return s_hwnd; }

    // -------------------------------------------------------------------------
    // Windows message procedure (static, registered with Win32).
    // -------------------------------------------------------------------------
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg,
        WPARAM wParam, LPARAM lParam);

private:
    // -------------------------------------------------------------------------
    // Internal message handlers — called from WndProc.
    // -------------------------------------------------------------------------

    // Called when the window is first created. Sets up fonts, brushes, layout.
    void OnCreate(HWND hwnd);

    // Called when the window needs to be repainted.
    void OnPaint(HWND hwnd);

    // Called when the window is resized. Recalculates layout.
    void OnSize(HWND hwnd, int width, int height);

    // Called when the user clicks inside the calendar grid.
    void OnLButtonDown(HWND hwnd, int x, int y);

    // Called when the user double-clicks a calendar cell.
    void OnLButtonDblClk(HWND hwnd, int x, int y);

    // Called for keyboard navigation (arrow keys, Page Up/Down).
    void OnKeyDown(HWND hwnd, WPARAM key);

    // Called when a toolbar/menu command is triggered.
    void OnCommand(HWND hwnd, WPARAM wParam);

    // Called when the window is destroyed.
    void OnDestroy(HWND hwnd);

    // -------------------------------------------------------------------------
    // Drawing routines — each draws one region of the window.
    // -------------------------------------------------------------------------

    // Draws the toolbar at the top.
    void DrawToolbar(HDC hdc, const RECT& rc);

    // Draws the month/year header (e.g. "May 2026 / Iyar-Sivan 5786").
    void DrawMonthHeader(HDC hdc, const RECT& rc);

    // Draws the day-of-week header row (Sun, Mon, ... Shabbos).
    void DrawDayHeaders(HDC hdc, const RECT& rc);

    // Draws all calendar cells for the current month.
    void DrawCalendarGrid(HDC hdc, const RECT& rc);

    // Draws a single calendar cell.
    void DrawCell(HDC hdc, const RECT& rc,
        const GregorianDate& g, const HebrewDate& h,
        bool isSelected, bool isCurrentMonth);

    // Draws the day details sidebar on the left.
    void DrawSidebar(HDC hdc, const RECT& rc);

    // Draws the zmanim panel at the bottom.
    void DrawZmanimPanel(HDC hdc, const RECT& rc);

    // -------------------------------------------------------------------------
    // Navigation helpers
    // -------------------------------------------------------------------------

    // Moves the view to the previous month.
    void PrevMonth();

    // Moves the view to the next month.
    void NextMonth();

    // Moves the view to today's date.
    void GoToToday();

    // Sets the selected date and refreshes the display.
    void SelectDate(const GregorianDate& g);

    // Returns the cell RECT for a given day index (0-41) in the grid.
    RECT GetCellRect(int cellIndex) const;

    // Returns which cell index (0-41) contains the given point, or -1.
    int HitTestCell(int x, int y) const;

    // Returns the background color for a cell based on its date/holidays.
    COLORREF GetCellColor(const GregorianDate& g, const HebrewDate& h,
        bool isSelected, bool isCurrentMonth) const;

    // -------------------------------------------------------------------------
    // Data helpers
    // -------------------------------------------------------------------------

    // Recalculates all cached data for the current view month.
    // Called whenever the month changes.
    void RefreshMonthData();

    // Recalculates zmanim for the selected date.
    void RefreshZmanim();

    // -------------------------------------------------------------------------
    // Member variables
    // -------------------------------------------------------------------------

    HWND     m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;

    // Current view: which month/year the calendar is showing
    int      m_viewYear = 2026;
    int      m_viewMonth = 5;    // Gregorian month

    // Selected date (highlighted cell)
    GregorianDate m_selectedDate;
    HebrewDate    m_selectedHebrew;

    // Today's date (for highlighting)
    GregorianDate m_today;
    HebrewDate    m_todayHebrew;

    // Current location for zmanim
    Location      m_location;
    bool          m_isDST = false;
    bool          m_use24hr = false;
    bool          m_isIsrael = false;

    // Cached zmanim for selected date
    ZmanimResult  m_zmanim;

    // Cached holidays for every day in the current view month
    // Index 0-41 (6 weeks x 7 days)
    struct CellData
    {
        GregorianDate             greg;
        HebrewDate                hebrew;
        std::vector<HolidayInfo>  holidays;
        OmerInfo                  omer;
        bool                      isCurrentMonth = false;
    };
    std::vector<CellData> m_cells; // 42 entries

    // Layout rectangles (recalculated on resize)
    RECT m_rcToolbar = {};
    RECT m_rcHeader = {};
    RECT m_rcDayHeaders = {};
    RECT m_rcGrid = {};
    RECT m_rcSidebar = {};
    RECT m_rcZmanim = {};

    // Column x-positions for the 7 day columns
    int  m_colX[8] = {};   // 8 values = 7 left edges + 1 right edge

    // GDI resources (created in OnCreate, deleted in OnDestroy)
    HFONT  m_fontNormal = nullptr;  // Standard cell text
    HFONT  m_fontBold = nullptr;  // Day numbers, headers
    HFONT  m_fontSmall = nullptr;  // Hebrew dates, holiday names
    HFONT  m_fontHeader = nullptr;  // Month/year header
    HBRUSH m_brToday = nullptr;
    HBRUSH m_brShabbos = nullptr;
    HBRUSH m_brYomTov = nullptr;
    HBRUSH m_brRoshChodesh = nullptr;
    HBRUSH m_brNormal = nullptr;
    HBRUSH m_brSidebar = nullptr;
    HBRUSH m_brZmanim = nullptr;
    HPEN   m_penGrid = nullptr;
    HPEN   m_penHeader = nullptr;

    // Static instance pointer for WndProc routing
    static MainWindow* s_instance;
    static HWND        s_hwnd;

    // Window class name
    static const wchar_t* CLASS_NAME;
};