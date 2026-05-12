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
#include "Settings.h"
#include <functional>

class CMainFrame;

// ── Simple page options (orientation + margins, no range) ─────────────────────

struct SimplePageOpts
{
    bool  landscape = false;
    float mTop  = 0.0f;
    float mBot  = 0.0f;
    float mLeft = 0.0f;
    float mRight= 0.0f;
    bool  use24hr = false;
};

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
    bool     includeZmanim  = false;
    uint32_t zmanimColumns  = 0x7FFF;  // bitmask: which of 15 columns to include
    bool     showFooter     = true;
};

// Render function type for single-page prints: DC, page rect, showFooter.
using PageRenderFn = std::function<void(CDC*, const CRect&, bool)>;

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

// Low-level single-page print: shows CPrintDialog and invokes render. Returns false if cancelled.
bool SimplePrint(const wchar_t* docName, const SimplePageOpts& opts,
                 PageRenderFn render, bool showFooter = true);

// Renders a daily zmanim table for one month. colMask picks columns; use24hr controls time format.
void DrawZmanimMonthPage(CDC* pDC, const CRect& rcPage,
                         int year, int month, CMainFrame* pFrame,
                         uint32_t colMask = 0x7FFF, bool use24hr = true,
                         bool showFooter = true);

// Renders one day's details (holidays, learning, zmanim) to any DC.
void DrawDayPage(CDC* pDC, const CRect& rcPage,
                 const GregorianDate& g, CMainFrame* pFrame,
                 bool showFooter = true);

// Shows CPrintDialog and prints a zmanim-month page. Returns false if cancelled.
bool DoPrintZmanimMonth(int year, int month, CMainFrame* pFrame,
                        uint32_t colMask = 0x7FFF);

// Shows CPrintDialog and prints a single day detail page.
bool DoPrintDay(const GregorianDate& g, CMainFrame* pFrame);

// Renders a list of events for a year to any DC (portrait, multi-page aware via pageIndex/totalPages).
void DrawEventsListPage(CDC* pDC, const CRect& rcPage,
                        const std::vector<UserEventEntry>& events,
                        int gregYear, int pageIndex, int totalPages,
                        bool showFooter = true);

// Shows a year-picker + page setup dialog and prints all event list pages.
bool DoPrintEventsList(CMainFrame* pFrame);

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
    CButton m_chkZmanim;
    CButton m_btnPreview;
    CButton m_chkCol[15];
    CButton m_chkShowFooter;

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
        IDC_PD_BTN_PREVIEW = 210,
        IDC_PD_CHK_ZMANIM  = 211,
        IDC_PD_COL_0       = 212,   // first of 15 column checkboxes (212..226)
        IDC_PD_CHK_FOOTER  = 227,
    };

    void ReadControls();
};

// ── Page setup + preview dialog (for day details and zmanim-month prints) ─────

class CSimplePageSetupDlg : public CDialog
{
public:
    CSimplePageSetupDlg(PageRenderFn render,
                        const wchar_t* docName, bool defaultLandscape,
                        CWnd* pParent = nullptr);
    virtual INT_PTR DoModal() override;

    // Optional: pass a shared_ptr<bool> to add a 12/24-hr checkbox to the dialog.
    // ReadControls() will update *ptr before each render/print call.
    void SetUse24HrPtr(std::shared_ptr<bool> p) { m_pUse24hr = std::move(p); }

protected:
    BOOL OnInitDialog() override;
    afx_msg void OnPreview();
    afx_msg void OnPrint();

    DECLARE_MESSAGE_MAP()

private:
    PageRenderFn   m_render;
    std::wstring   m_docName;
    SimplePageOpts m_opts;

    CButton m_radPortrait, m_radLandscape;
    CEdit   m_editTop, m_editBot, m_editLeft, m_editRight;
    CButton m_btnPreview;
    CButton m_chkFooter;
    CButton m_chk24hr;
    std::shared_ptr<bool> m_pUse24hr;

    enum {
        IDC_SPS_RAD_PORT    = 240,
        IDC_SPS_RAD_LAND    = 241,
        IDC_SPS_EDT_TOP     = 242,
        IDC_SPS_EDT_BOT     = 243,
        IDC_SPS_EDT_LEFT    = 244,
        IDC_SPS_EDT_RIGHT   = 245,
        IDC_SPS_BTN_PREVIEW = 246,
        IDC_SPS_CHK_FOOTER  = 247,
        IDC_SPS_CHK_24HR    = 248,
    };

    void ReadControls();
};

// ── Single-page preview dialog (used for day and zmanim-month prints) ─────────

class CSimplePreviewDlg : public CDialog
{
public:
    // render receives (DC, pageRect, showFooter)
    CSimplePreviewDlg(PageRenderFn render,
                      const wchar_t* docName, bool portrait,
                      bool showFooter = true, CWnd* pParent = nullptr);
    virtual INT_PTR DoModal() override;

protected:
    BOOL OnInitDialog() override;
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnPrint();
    afx_msg void OnSize(UINT nType, int cx, int cy);

    DECLARE_MESSAGE_MAP()

private:
    PageRenderFn m_render;
    std::wstring m_docName;
    bool         m_portrait;
    bool         m_showFooter;
    CButton      m_btnPrint, m_btnClose;
    enum { IDC_SP_PRINT = 230 };
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
