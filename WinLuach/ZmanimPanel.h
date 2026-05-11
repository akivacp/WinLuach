// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    ZmanimPanel.h
// Purpose: Declares the zmanim panel (CZmanimPanel).
//          Shows all halachic times for the selected date and location
//          in a 4-column layout at the bottom of the window.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial MFC zmanim panel.
// =============================================================================

#pragma once
#include "pch.h"
#include "MainFrame.h"

class CZmanimPanel : public CWnd
{
public:
    explicit CZmanimPanel(CMainFrame* pFrame);
    virtual ~CZmanimPanel();

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

private:
    CMainFrame* m_pFrame = nullptr;

    DECLARE_MESSAGE_MAP()
};