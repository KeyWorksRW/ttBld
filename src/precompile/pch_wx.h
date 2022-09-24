// This file is used to create a pre-compiled header for use in the entire project using ttLib_wx and wxWidgets

#pragma once

#if defined(_WIN32)

// Reduce the number of Windows files that get read

    #define NOATOM
    #define NOCOMM
    #define NODRIVERS
    #define NOEXTDEVMODEPROPSHEET
    #define NOIME
    #define NOKANJI
    #define NOLOGERROR
    #define NOMCX
    #define NOPROFILER
    #define NOSCALABLEFONT
    #define NOSERVICE
    #define NOSOUND
    #define NOWINDOWSX
    #define NOENHMETAFILE

    #define OEMRESOURCE
    #define _CRT_SECURE_NO_WARNINGS

    #define STRICT
    #define WIN32_LEAN_AND_MEAN

#endif  // _WIN32

#define wxUSE_UNICODE     1
#define wxUSE_NO_MANIFEST 1  // This is required for compiling using CLANG 8 and earlier

#ifdef _MSC_VER
    #pragma warning(push)
#endif

#if defined(__clang__)
    // warning: unused typedef 'complete' in scopedptr.h
    #pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif

#include <wx/defs.h>  // Declarations/definitions common to all wx source files

#if !wxCHECK_VERSION(3, 2, 0)
    #error "You must have wxWidgets 3.2.0 or later to build this project."
#endif

#if defined(__WINDOWS__)
    #include <wx/msw/wrapcctl.h>  // Wrapper for the standard <commctrl.h> header

    #if wxUSE_COMMON_DIALOGS
        #include <commdlg.h>
    #endif
#endif

#include <wx/string.h>  // wxString class

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

// Ensure that _DEBUG is defined in non-release builds
#if !defined(NDEBUG) && !defined(_DEBUG)
    #define _DEBUG
#endif

#include <stdexcept>
#include <string>
#include <string_view>

#include <ttlib_wx.h>  // This must be included before any other ttLib header files

#include <ttcstr_wx.h>   // ttlib::cstr -- std::string with additional functions
#include <ttsview_wx.h>  // ttlib::sview -- std::string_view with additional methods

// Version is also set in writesrc.h and ttBld.rc -- if changed, change in all three locations

inline constexpr const auto txtVersion = "ttBld 1.8.2";
inline constexpr const auto txtCopyRight = "Copyright (c) 2002-2022 KeyWorks Software";
inline constexpr const auto txtAppname = "ttBld";

#include "assertion_dlg.h"  // Assertion Dialog and ASSERT macros
