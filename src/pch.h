// Precompiled header

#pragma once

/*	// Uncomment this section to use wxWidgets
#pragma warning(push, 1)
	#include <wx/wxprec.h>
	#include <wx/debug.h>
#pragma warning(pop)
*/

#ifndef _WX_WX_H_
	#define NOATOM
	#define NOCOMM
	#define NODRIVERS
	#define NOENHMETAFILE
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
	#define OEMRESOURCE

	#define _CRT_SECURE_NO_WARNINGS
	#define STRICT
	#define WIN32_LEAN_AND_MEAN

	#define WINVER 		 0x0600		// Windows Vista and Windows Server 2008 (use 0x0601 for Windows 7).
	#define _WIN32_WINNT 0x0600
	#define _WIN32_IE 	 0x0700

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

#define __TTLIB_INCLUDE_MAJOR__	// tell ttLib.h to include other major ttLib header files
#include "../ttLib/include/ttlib.h"

using namespace ttch;	// For the CH_ constants

#include "funcs.h"
