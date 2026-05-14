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
// v0.8.0 - Added kZmanimBarLabels[] export so the Options dialog can render
//          one checkbox per zman in the bottom bar.
// =============================================================================

#pragma once
#include "pch.h"
#include "MainFrame.h"

// v0.8.0 - Bit-indexed label list for the bottom zmanim bar.  A label is
// shown in the panel only when the corresponding bit in zmanimBarMask is set.
extern const wchar_t* const kZmanimBarLabels[];
extern const int kZmanimBarLabelCount;

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