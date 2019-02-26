// This header file is used to create a pre-compiled header for use in the entire project

#pragma once

// Uncomment the following section to use wxWidgets (https://github.com/wxWidgets/wxWidgets)
/*
#pragma warning(push, 1)
	#include <wx/wxprec.h>
	#include <wx/debug.h>
#pragma warning(pop)
*/

#ifndef _WX_WX_H_
	#define STRICT

#ifndef _WIN32_WINNT_VISTA
	#define _WIN32_WINNT_NT4    0x0400
	#define _WIN32_WINNT_WINXP  0x0501
	#define _WIN32_WINNT_VISTA  0x0600
	#define _WIN32_WINNT_WIN7   0x0601
	#define _WIN32_WINNT_WIN8   0x0602
	#define _WIN32_WINNT_WIN10  0x0A00
#endif

	#define WINVER 		 _WIN32_WINNT_VISTA		// minimum OS required
	#define _WIN32_WINNT _WIN32_WINNT_VISTA

	#include <windows.h>
#endif	// _WX_WX_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WINDOWS_
	#pragma comment(lib, "advapi32.lib")
	#pragma comment(lib, "comdlg32.lib")
	#pragma comment(lib, "gdi32.lib")
	#pragma comment(lib, "kernel32.lib")
	#pragma comment(lib, "shell32.lib")
	#pragma comment(lib, "user32.lib")
#endif	// _WINDOWS_

extern const char* txtVersion;
extern const char* txtCopyRight;

#include <ttlib.h>		// Master header file for ttLib
#include <ttdebug.h>	// ttASSERT macros

using namespace ttch;	// For the CH_ constants

#include "funcs.h"
