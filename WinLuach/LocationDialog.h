// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    LocationDialog.h
// Purpose: Declares the location picker dialog.
//          Shows a searchable list of all built-in cities plus any
//          user-defined custom locations. User can search by name,
//          select a city, and optionally add/edit/delete custom locations.
// =============================================================================
//
// CHANGELOG:
// v0.1.0 - Initial file. Search box + list box + OK/Cancel.
//          Add/Edit/Delete buttons for custom locations.
//          Returns selected LocationEntry to caller.
// =============================================================================

#pragma once
#include "Location.h"
#include <windows.h>
#include <string>

// =============================================================================
// DIALOG CONTROL IDs
// =============================================================================

#define IDC_SEARCH      201   // Search text box
#define IDC_CITY_LIST   202   // City list box
#define IDC_BTN_ADD     203   // Add custom location
#define IDC_BTN_EDIT    204   // Edit custom location
#define IDC_BTN_DELETE  205   // Delete custom location
#define IDC_BTN_OK      IDOK
#define IDC_BTN_CANCEL  IDCANCEL
#define IDC_LBL_SEARCH  206   // "Search:" label
#define IDC_LBL_COORDS  207   // Shows lat/lon of selected city

// =============================================================================
// LOCATION DIALOG CLASS
// =============================================================================

class LocationDialog
{
public:
    // -------------------------------------------------------------------------
    // Shows the location picker dialog modally.
    // parent:       owner window handle
    // currentLoc:   the currently selected location (highlighted on open)
    // selectedOut:  filled with the chosen location if user clicks OK
    // Returns true if user clicked OK, false if cancelled.
    // -------------------------------------------------------------------------
    static bool Show(HWND           parent,
        const Location& currentLoc,
        Location& selectedOut);

private:
    // Dialog procedure (static, registered with Win32)
    static INT_PTR CALLBACK DlgProc(HWND   hwnd,
        UINT   msg,
        WPARAM wParam,
        LPARAM lParam);

    // Called when the dialog is initialised — populates the list box.
    void OnInit(HWND hwnd);

    // Called when the user types in the search box — filters the list.
    void OnSearch(HWND hwnd);

    // Called when the user selects an item in the list — updates coord display.
    void OnSelChange(HWND hwnd);

    // Called when OK is clicked — copies the selected location to m_result.
    bool OnOK(HWND hwnd);

    // Called when Add is clicked — shows a simple input dialog for new city.
    void OnAdd(HWND hwnd);

    // Called when Edit is clicked — edits the selected custom location.
    void OnEdit(HWND hwnd);

    // Called when Delete is clicked — removes the selected custom location.
    void OnDelete(HWND hwnd);

    // Rebuilds the list box contents based on the current search filter.
    // Shows all locations if filter is empty.
    void PopulateList(HWND hwnd, const std::wstring& filter);

    // Returns the LocationEntry for the currently selected list item.
    // Returns nullptr if nothing is selected.
    const LocationEntry* GetSelectedEntry(HWND hwnd) const;

    // The location chosen by the user (set when OK is clicked)
    Location  m_result;
    bool      m_accepted = false;

    // The name of the currently active location (to pre-select on open)
    std::wstring m_currentName;

    // Filtered list: indices into LocationDB::Get().GetAll()
    // rebuilt every time the search changes
    std::vector<int> m_filteredIndices;

    // Static instance pointer for DlgProc routing
    static LocationDialog* s_instance;
};