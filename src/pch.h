// This header file is used to create a pre-compiled header for use in the entire project

#pragma once

#if defined(_WIN32)
    #define wxMSVC_VERSION_AUTO
#endif

#define wxUSE_UNICODE     1
#define wxUSE_NO_MANIFEST 1  // this is required for clang-cl to work
#define wxUSE_INTL        1

#include "wx/defs.h"
#include "wx/chartype.h"

#if defined(__WINDOWS__)
    #include "wx/msw/wrapwin.h"
    #include "wx/msw/private.h"
#endif

#if defined(__WXMSW__)
    #include "wx/msw/wrapcctl.h"
    #include "wx/msw/wrapcdlg.h"
    #include "wx/msw/missing.h"
#endif

#include <wx/intl.h>

#include <stdexcept>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <ttwx.h>      // Master header file for ttwx.lib
#include <ttassert.h>  // Provides an alternative to wxASSERT macros

#include <ttlib.h>    // Master header file for ttLib
#include <ttdebug.h>  // ttASSERT macros

using namespace ttch;  // For the CH_ and CHW_ constants

constexpr std::string_view g_txtVersion("ttBld 1.3.0.8295");
constexpr std::string_view g_txtCopyright("Copyright (c) 2002-2020 KeyWorks Software");
constexpr std::string_view g_txtAppname("ttBld");

extern const char* txtVersion;
extern const char* txtCopyRight;
extern const char* txtAppName;
