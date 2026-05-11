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

private:
    // Draws a horizontal separator line and advances yOff.
    void DrawSep(CDC* pDC, int x, int& yOff, int w);

    CMainFrame* m_pFrame = nullptr;

    DECLARE_MESSAGE_MAP()
};