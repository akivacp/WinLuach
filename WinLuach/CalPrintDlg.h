// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    CalPrintDlg.h
// Purpose: Calendar printing and print-preview.
//          Provides:
//            - CalPrintOptions  (range, orientation, margins)
//            - DrawCalMonthPage (renders one month to any DC)
//            - BuildPageList    (enumerates pages from options)
//            - DoPrint          (shows CPrintDialog and prints)
//            - CCalPrintDlg     (print options dialog)
//            - CCalPreviewDlg   (full print-preview window)
// =============================================================================

#pragma once
#include "pch.h"
#include "HebrewDate.h"
#include "HolidayEngine.h"

class CMainFrame;

// ── Print options ─────────────────────────────────────────────────────────────

struct CalPrintOptions
{
    enum Range { RANGE_MONTH = 0, RANGE_YEAR = 1, RANGE_12 = 2 };

    Range range         = RANGE_MONTH;
    bool  landscape     = true;
    float mTop          = 0.75f;
    float mBot          = 0.75f;
    float mLeft         = 0.50f;
    float mRight        = 0.50f;
    bool  includeZmanim = false;
};

// ── Free functions ────────────────────────────────────────────────────────────

// Renders one calendar month nicely to any DC, filling rcPage.
// When opts.includeZmanim is true, a weekly zmanim table is drawn below the grid.
void DrawCalMonthPage(CDC* pDC, const CRect& rcPage,
                      int year, int month, CMainFrame* pFrame,
                      const CalPrintOptions& opts = CalPrintOptions{});

// Returns the ordered list of (year, month) pages for the given options.
std::vector<std::pair<int, int>> BuildPageList(
    const CalPrintOptions& opts, int viewYear, int viewMonth);

// Shows CPrintDialog and prints all pages. Returns false if cancelled.
bool DoPrint(const CalPrintOptions& opts, CMainFrame* pFrame);

// ── Print options dialog ──────────────────────────────────────────────────────

class CCalPrintDlg : public CDialog
{
public:
    explicit CCalPrintDlg(CMainFrame* pFrame, CWnd* pParent = nullptr);
    virtual INT_PTR DoModal() override;

    CalPrintOptions GetOptions() const { return m_opts; }

protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
    afx_msg void OnBnPreview();

    DECLARE_MESSAGE_MAP()

private:
    CMainFrame*     m_pFrame;
    CalPrintOptions m_opts;

    CButton m_radMonth, m_radYear, m_rad12;
    CButton m_radPortrait, m_radLandscape;
    CEdit   m_editTop, m_editBot, m_editLeft, m_editRight;
    CButton m_chkPDF;
    CButton m_chkZmanim;
    CButton m_btnPreview;

    enum {
        IDC_PD_RAD_MONTH   = 200,
        IDC_PD_RAD_YEAR    = 201,
        IDC_PD_RAD_12      = 202,
        IDC_PD_RAD_PORT    = 203,
        IDC_PD_RAD_LAND    = 204,
        IDC_PD_EDT_TOP     = 205,
        IDC_PD_EDT_BOT     = 206,
        IDC_PD_EDT_LEFT    = 207,
        IDC_PD_EDT_RIGHT   = 208,
        IDC_PD_CHK_PDF     = 209,
        IDC_PD_BTN_PREVIEW = 210,
        IDC_PD_CHK_ZMANIM  = 211,
    };

    void ReadControls();
};

// ── Print-preview dialog ──────────────────────────────────────────────────────

class CCalPreviewDlg : public CDialog
{
public:
    CCalPreviewDlg(const CalPrintOptions& opts, CMainFrame* pFrame,
                   CWnd* pParent = nullptr);
    virtual INT_PTR DoModal() override;

protected:
    BOOL OnInitDialog() override;
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnPrevPage();
    afx_msg void OnNextPage();
    afx_msg void OnPrint();
    afx_msg void OnSize(UINT nType, int cx, int cy);

    DECLARE_MESSAGE_MAP()

private:
    CalPrintOptions                  m_opts;
    CMainFrame*                      m_pFrame;
    std::vector<std::pair<int, int>> m_pages;
    int                              m_curPage = 0;

    CButton m_btnPrev, m_btnNext, m_btnPrint, m_btnClose;
    CStatic m_lblPage;

    enum {
        IDC_PRV_PREV  = 220,
        IDC_PRV_NEXT  = 221,
        IDC_PRV_PRINT = 222,
        IDC_PRV_LBL   = 223,
    };

    void UpdateNavButtons();
    void UpdatePageLabel();
    void LayoutButtons();
};
