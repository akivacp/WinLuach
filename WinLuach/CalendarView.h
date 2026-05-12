// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    CalendarView.h
// Purpose: Declares the owner-draw calendar grid (CCalendarView).
//          Draws 42 cells (6 rows x 7 columns) covering the current month.
//          Each cell shows the Gregorian day number, Hebrew date,
//          holiday names, and omer count.
//          Handles mouse clicks and keyboard navigation.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial MFC owner-draw calendar grid. 42 cells, color coding,
//          Hebrew dates, holiday names, omer. Mouse + keyboard navigation.
// =============================================================================

#pragma once
#include "pch.h"
#include "HebrewDate.h"
#include "HolidayEngine.h"
#include "MainFrame.h"

// =============================================================================
// CELL DATA
// Cached data for one calendar cell (one day).
// =============================================================================

struct CalCellData
{
    GregorianDate            greg;
    HebrewDate               hebrew;
    std::vector<HolidayInfo> holidays;
    OmerInfo                 omer;
    DailyLearning            learning;
    bool                     isCurrentMonth = false;
    std::wstring             candleStr;
    std::wstring             motzStr;
};

// =============================================================================
// CCalendarView
// =============================================================================

class CCalendarView : public CWnd
{
public:
    // pFrame: pointer to the parent main frame (for accessing shared state)
    explicit CCalendarView(CMainFrame* pFrame);
    virtual ~CCalendarView();

    // Rebuilds m_cells for the current view month.
    // Called whenever the month changes.
    void RebuildCells();

protected:
    // MFC message handlers
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
    afx_msg void OnMouseMove(UINT nFlags, CPoint pt);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint pt);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint pt);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint pt);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnSize(UINT nType, int cx, int cy);

private:
    // Draws a single calendar cell.
    void DrawCell(CDC* pDC, const CRect& rc,
        const CalCellData& cell, bool isSelected);

    // Returns the background color for a cell.
    COLORREF GetCellColor(const CalCellData& cell, bool isSelected) const;

    // Returns the cell index (0-41) for the given point, or -1.
    int HitTest(CPoint pt) const;

    // Returns the CRect for a given cell index (0-41).
    CRect GetCellRect(int idx) const;

    // Multi-day selection helpers for Ctrl/Shift calendar picking.
    bool IsSelectedDate(const GregorianDate& g) const;
    bool IsExplicitlySelected(const GregorianDate& g) const;
    std::vector<GregorianDate> GetSelectedDates() const;
    void ClearMultiSelection();
    void ToggleSelectedCell(int idx);
    void SelectRangeToCell(int idx);
    int FindSelectedCellIndex() const;

    // Pointer to the parent frame (not owned — do not delete)
    CMainFrame* m_pFrame = nullptr;

    // Cached cell data for all 42 cells
    std::vector<CalCellData> m_cells;

    // Column x-positions (8 values = 7 left edges + right edge)
    int m_colX[8] = {};

    // Row height (recalculated on resize)
    int m_cellH = 80;

    // Fonts (owned by CMainFrame, not by this class)
    // We borrow them from m_pFrame

    std::vector<long> m_selectedJdns;
    int m_anchorIndex = -1;
    bool m_dragSelecting = false;
    bool m_dragMoved = false;
    int  m_dragStartIndex = -1;
    int  m_lastDragIndex = -1;

    DECLARE_MESSAGE_MAP()
};
