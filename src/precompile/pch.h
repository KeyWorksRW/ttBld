// This header file is used to create a pre-compiled header for use in the entire project

// Caller should '#define wxMSVC_VERSION_AUTO' to use wxWidgets libraries

#if defined(_WIN32) && !defined(wxMSVC_VERSION_AUTO)
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
    #define WIN32_LEAN_AND_MEAN
    #define _CRT_SECURE_NO_WARNINGS

    #define WINVER       0x0601  // Windows 7
    #define _WIN32_WINNT 0x0600
    #define _WIN32_IE    0x0700

    #include <windows.h>
    #include <stdint.h>
    #include <stdio.h>
    #include <stdlib.h>
#endif  // defined(_WIN32)

#if defined(wxMSVC_VERSION_AUTO)
    #if defined(_WIN32)
        #define wxMSVC_VERSION_AUTO
    #endif

    #define wxUSE_UNICODE     1
    #define wxUSE_NO_MANIFEST 1
    #define WXUSINGDLL        1

    #include "wx/defs.h"  // compiler detection; includes setup.h

    #include "wx/chartype.h"

    #if defined(__WINDOWS__)
        #include "wx/msw/wrapcctl.h"
        #include "wx/msw/private.h"

        #if wxUSE_COMMON_DIALOGS
            #include <commdlg.h>
        #endif

        #if wxUSE_COMMON_DIALOGS
            #include <commdlg.h>
        #endif

        #include "wx/msw/winundef.h"
    #endif
#endif

#include <cassert>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <filesystem>
namespace fs = std::filesystem;

#include <ttlibwin.h>  // Master header file for ttLibwin
#include <ttTR.h>

// WARNING: ttstr.h must appear before ttstring.h!
#include <ttstr.h>
#include <ttstring.h>

using namespace ttch;  // For the CH_ and CHW_ constants

constexpr const char* txtVersion = "ttBld 1.4.0.8295";
constexpr const char* txtCopyright = "Copyright (c) 2002-2020 KeyWorks Software";
constexpr const char* txtAppname = "ttBld";
