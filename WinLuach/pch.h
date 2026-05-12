// =============================================================================
// WinLuach - Hebrew Calendar Application
// File:    pch.h
// Purpose: Precompiled header. Includes MFC and standard headers
//          that are used throughout the project.
//          Every .cpp file must include this as its first include.
// =============================================================================

#pragma once
#include <afxwin.h>
#include <afxext.h>
#include <afxdlgs.h>
#include <afxcmn.h>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

// Request ComCtl32 v6 so TaskDialogIndirect and visual styles are available.
#pragma comment(linker, "\"/manifestdependency:type='win32' " \
    "name='Microsoft.Windows.Common-Controls' version='6.0.0.0' " \
    "processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")