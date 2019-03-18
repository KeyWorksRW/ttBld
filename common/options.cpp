/////////////////////////////////////////////////////////////////////////////
// Name:		CSrcOptions
// Purpose:		Class for storing/retrieving options in a .srcfiles file
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

/*

	Ideally, s_aOptions would be in a namespace. However, the Visual Studio debugger (2017 version) is not able to display
	the array even if fully qualified with the namespace. That makes debugging rather difficult since a lot of the functionality
	of MakeSrcFiles relies on comparison between a modified option and the original.

*/

#include "pch.h"

#include "options.h"

using namespace sfopt;

// Add these in the order you want them written in a new .srcfiles files.

static const OPT_SETTING s_aOptions[] = {
	// { OPT_xxx,			"name",			"value", boolean type, required, "comment" }

	{ OPT_PROJECT,  	"Project",		nullptr,	false, true,  "" },
	{ OPT_PCH,	    	"PCH",			"none",		false, true,  "name of precompiled header file, or \042none\042 if not using precompiled headers" },
	{ OPT_EXE_TYPE, 	"exe_type", 	"console",	false, true,  "[window | console | lib | dll]" },

	{ OPT_64BIT,    	"64Bit", 		"false",	true,  false,  "if true, enable 64-bit support (link with 64-bit libraries)" },
	{ OPT_BIT_SUFFIX,   "bit_suffix", 	"false",	true,  false,  "true means append '64' to target's directory or .exe name" },

	{ OPT_PERMISSIVE,   "permissive", 	"false",	true,  false,  "true means add -permissive- compiler flag" },
	{ OPT_STDCALL,      "stdcall", 		"false",	true,  false,  "true to use stdcall calling convention, false for cdecl (default)" },
	{ OPT_OPTIMIZE,     "optimize", 	"space",	false, false,  "[space | speed] optimization (optimizing for speed can actually make the code run slower due to caching issues)" },
	{ OPT_WARN_LEVEL,   "WarnLevel", 	"4",	    false, false,  "[1-4] default is 4" },

	{ OPT_MAKEFILE, 	"Makefile", 	"missing", 	false, false, "[never | missing | always]" },
	{ OPT_COMPILERS,    "Compilers", 	nullptr,	false, false,  "[MSVC or CLANG] default is both, set this option to limit it to one" },
	{ OPT_CFLAGS,       "CFlags", 	    nullptr,	false, false,  "additional flags to pass to the compiler in all build targets" },
	{ OPT_MIDL_FLAGS,   "MidlFlags", 	nullptr,	false, false,  "flags to pass to the midl compiler" },
	{ OPT_LINK_FLAGS,   "LinkFlags", 	nullptr,	false, false,  "additional flags to pass to the linker in all build targets" },
	{ OPT_RC_FLAGS,     "RCFlags", 	    nullptr,	false, false,  "additional flags to pass to the resource compiler in all build targets" },
	{ OPT_DEBUG_RC,     "DebugRC", 	    "false",	true,  false,  "true means build a -D_DEBUG version of the project's rc file" },
	{ OPT_STATIC_CRT,   "static_crt", 	"false",	true,  false,  "true means link to static CRT" },
	{ OPT_MS_LINKER,    "ms_linker", 	"false",	true,  false,  "true means use link.exe even when compiling with CLANG" },
	{ OPT_IDE,          "IDE", 			nullptr,	false, false,  "[CodeBlocks CodeLite VisualStudio] -- specifies one or more IDEs to generate project files for" },

	{ OPT_INC_DIRS,     "IncDirs",		nullptr,	false, false,  "additional directories for header files" },
	{ OPT_TARGET_DIR32, "TargetDir32",	nullptr,	false, false,  "32-bit target directory (default is bin)" },
	{ OPT_TARGET_DIR64, "TargetDir64",	nullptr,	false, false,  "32-bit target directory (default is bin64)" },
	{ OPT_BUILD_LIBS,   "BuildLibs",	nullptr,	false, false,  "libraries that need to be built (added to makefile generation)" },
	{ OPT_LIB_DIRS,     "LibDirs",		nullptr,	false, false,  "additional directores for lib files" },
	{ OPT_LIBS,         "Libs",			nullptr,	false, false,  "additional libraries to link to (see OPT_BUILD_LIBS to both build and link to a library)" },

	{ OPT_OVERFLOW, "", "", false, false, "" },
};

const OPT_SETTING* CSrcOptions::GetOrgOptions()
{
	return s_aOptions;
}

CSrcOptions::CSrcOptions()
{
	// By adding OPT_UPDATE structures in the same order as OPT_SETTING, a position in m_aUpdateOpts matches the same position in s_aOptions

	for (size_t pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos) {
		size_t posOpt = m_aUpdateOpts.Add();
		m_aUpdateOpts[posOpt].pszVal = s_aOptions[pos].pszVal ? tt::StrDup(s_aOptions[pos].pszVal) : nullptr;
		m_aUpdateOpts[posOpt].pszComment = tt::isNonEmpty(s_aOptions[pos].pszComment) ?  tt::StrDup(s_aOptions[pos].pszComment) : nullptr;
	}
}

CSrcOptions::~CSrcOptions()
{
	for (size_t pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos) {
		tt::Delete(m_aUpdateOpts[pos].pszVal);
		tt::Delete(m_aUpdateOpts[pos].pszComment);
	}
}

bool CSrcOptions::UpdateOption(sfopt::OPT_INDEX index, const char* pszVal)	// fine to call this for boolean options if pszVal == "true/false" or "yes/no"
{
	ttASSERT_MSG(pszVal, "NULL pointer!");
	if (!pszVal)
		return false;

	size_t pos;
	for (pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos) {
		if (s_aOptions[pos].opt == index)
			break;
	}
	ttASSERT(s_aOptions[pos].opt != OPT_OVERFLOW);
	if (s_aOptions[pos].opt == OPT_OVERFLOW)
		return false;	// invalid option
	tt::Delete(m_aUpdateOpts[pos].pszVal);

	// We want boolean values to be consistent, so if "yes" or "no" is specified, convert to "true" or "false"

	if (tt::isSameStri(pszVal, "yes"))
		pszVal = "true";
	else if (tt::isSameStri(pszVal, "no"))
		pszVal = "false";

	m_aUpdateOpts[pos].pszVal = tt::StrDup(pszVal);
	return true;
}

bool CSrcOptions::UpdateOption(sfopt::OPT_INDEX index, bool bVal)
{
	size_t pos;
	for (pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos) {
		if (s_aOptions[pos].opt == index)
			break;
	}
	ttASSERT(s_aOptions[pos].opt != OPT_OVERFLOW);
	if (s_aOptions[pos].opt == OPT_OVERFLOW)
		return false;	// invalid option

	tt::Delete(m_aUpdateOpts[pos].pszVal);
	m_aUpdateOpts[pos].pszVal = tt::StrDup(bVal ? "true" : "false");
	return true;
}

const char* CSrcOptions::GetOption(sfopt::OPT_INDEX index)
{
	for (size_t pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos) {
		if (s_aOptions[pos].opt == index)
			return m_aUpdateOpts[pos].pszVal;
	}
	return nullptr;
}

const char* CSrcOptions::GetOptComment(sfopt::OPT_INDEX index)
{
	for (size_t pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos) {
		if (s_aOptions[pos].opt == index)
			return m_aUpdateOpts[pos].pszComment;
	}
	return nullptr;
}

bool CSrcOptions::GetBoolOption(sfopt::OPT_INDEX index)
{
	for (size_t pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos) {
		if (s_aOptions[pos].opt == index)
			return tt::isSameStri(m_aUpdateOpts[pos].pszVal, "true");
	}
	return false;
}

bool CSrcOptions::GetChanged(sfopt::OPT_INDEX index)
{
	for (size_t pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos) {
		if (s_aOptions[pos].opt == index) {
			if (!s_aOptions[pos].pszVal)
				return tt::isNonEmpty(m_aUpdateOpts[pos].pszVal);
			else
				return !tt::isSameStri(m_aUpdateOpts[pos].pszVal, s_aOptions[pos].pszVal);
		}
	}
	return false;
}

bool CSrcOptions::UpdateReadOption(const char* pszName, const char* pszVal, const char* pszComment)
{
	ttASSERT_NONEMPTY(pszName);
	ttASSERT_MSG(pszVal, "NULL pointer!");

	size_t pos;
	for (pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos) {
		if (tt::isSameStri(s_aOptions[pos].pszName, pszName))
			break;
	}
	if (s_aOptions[pos].opt == OPT_OVERFLOW)
		return false;	// unknown option

	UpdateOption(s_aOptions[pos].opt, pszVal);	// we call this so that "yes" or "no" options get converted to "true" or "false"

	if (pszComment)	{
		tt::Delete(m_aUpdateOpts[pos].pszComment);
		m_aUpdateOpts[pos].pszComment = tt::StrDup(pszComment);
	}
	return true;
}
