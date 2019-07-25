// This header file is used to create a pre-compiled header for use in the entire project

#pragma once

#if defined(_WIN32) & !defined(_WX_DEFS_H_)
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

    #define WINVER       0x0601     // Windows 7
    #define _WIN32_WINNT 0x0600
    #define _WIN32_IE    0x0700

    #include <windows.h>
    #include <stdlib.h>
    #include <stdio.h>
#endif    // defined(_WIN32) & !defined(_WX_DEFS_H_)

#if defined(_WX_DEFS_H_)
    #define wxUSE_UNICODE     1
    #define wxUSE_GUI         1
    #define wxUSE_NO_MANIFEST 1

    #include "wx/defs.h"    // compiler detection; includes setup.h

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
#endif    // !defined(NOWX)

extern const char* txtVersion;
extern const char* txtCopyRight;

#include <ttlib.h>      // Master header file for ttLib
#include <ttdebug.h>    // ttASSERT macros

using namespace ttch;   // For the CH_ constants

extern const char* txtOptVersion; // The minimum version of MakeNinja required by a .srcfiles

#include "NinjaStringIds.h"   // Maps IDS_NINJA_ strings into string to translate
#include "funcs.h"      // List of function declarations
