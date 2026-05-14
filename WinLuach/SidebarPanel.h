// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    SidebarPanel.h
// Purpose: Declares the day details sidebar panel (CSidebarPanel).
//          Shows the selected date in Gregorian and Hebrew, holidays,
//          omer count, parasha, daily learning cycles, and year facts.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial MFC sidebar panel.
// =============================================================================

#pragma once
#include "pch.h"
#include "MainFrame.h"

class CSidebarPanel : public CWnd
{
public:
    explicit CSidebarPanel(CMainFrame* pFrame);
    virtual ~CSidebarPanel();

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
    afx_msg void OnMouseMove(UINT nFlags, CPoint pt);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint pt);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnSize(UINT nType, int cx, int cy);

private:
    void DrawSep(CDC* pDC, int x, int& yOff, int w);

    CMainFrame* m_pFrame  = nullptr;
    int         m_yearHdrY = -1;   // Y of Year details header (set during paint)
    float       m_splitFrac = 0.46f; // fraction of height where year section begins
    bool        m_dragging  = false;
    int         m_dragStartY = 0;
    int         m_scrollPos = 0;
    int         m_contentHeight = 0;

    void UpdateScrollBar(int clientHeight);

    DECLARE_MESSAGE_MAP()
};
