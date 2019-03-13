/////////////////////////////////////////////////////////////////////////////
// Name:		CSrcOptions
// Purpose:		Class for storing/retrieving options in a .srcfiles file
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

/*

 * This class is designed for use by CSrcFiles and CWriteSrcFiles.

 */

#pragma once

#include <ttarray.h>	// ttCArray

namespace sfopt {	// .srcfiles otpion
	typedef enum {
		OPT_ERROR = 0,

		OPT_PROJECT,		// name of the project--will be used as the base target name (i.e., project: foo, target: foo.exe, fooD.exe, etc.)
		OPT_PCH,			// name of precompiled header file, or "none" if not using precompiled headers
		OPT_EXE_TYPE,		// [window | console | lib | dll]

		// The following are boolean options (true or false)

		OPT_64BIT,			// if true, enable 64-bit support (link with 64-bit libraries).
		OPT_BIT_SUFFIX,		// true means append "64" to target's directory or .exe name
		OPT_DEBUG_RC,		// true means build a -D_DEBUG version of the project's rc file
		OPT_PERMISSIVE,		// true means add -permissive- compiler flag
		OPT_STDCALL,		// use stdcall calling convention
		OPT_STATIC_CRT,		// true means link to static CRT
		OPT_MS_LINKER,		// use link.exe even when compiling with CLANG

		// The following are strings--multiple strings are separated with a semi-colon

		OPT_COMPILERS,		// [MSVC or CLANG] default is both, set this option to limit it to one
		OPT_CFLAGS,			// additional flags to pass to the compiler in all build targets
		OPT_MIDL_FLAGS,		// flags to pass to the midl compiler
		OPT_LINK_FLAGS,		// additional flags to pass to the linker in all build targets
		OPT_RC_FLAGS,		// additional flags to pass to the resource compiler in all build targets
		OPT_INC_DIRS,		// additional directories for header files
		OPT_LIB_DIRS,		// additional directores for lib files
		OPT_LIBS,			// additional libraries to link to (see OPT_BUILD_LIBS to both build and link to a library)
		OPT_IDE,			// [CodeBlocks CodeLite VisualStudio] -- specifies one or more IDE go generate project files for
		OPT_MAKEFILE,		// [never | missing | always] -- default, if not specified, is missing
		OPT_OPTIMIZE,		// [space | speed] optimization (optimizing for speed can actually make the code run slower due to caching issues) -- default, if not specified, is space
		OPT_WARN_LEVEL,		// [1-4] default, if not specified, is 4
		OPT_TARGET_DIR32,	// 32-bit target directory (default is bin)
		OPT_TARGET_DIR64,	// 32-bit target directory (default is bin64)
		OPT_BUILD_LIBS,		// libraries that need to be built (added to makefile generation)

		OPT_OVERFLOW
	} OPT_INDEX;

	// { OPT_xxx, "name", "def value", boolean, required, "def comment" }

	typedef struct {	// the original default settings for an option
		OPT_INDEX	opt;
		const char* pszName;
		const char* pszVal;
		bool		bBoolType;
		bool		bRequired;
		const char* pszComment;
	} OPT_SETTING;

	typedef struct {	// the updated version of the option
		char*	pszVal;
		char*	pszComment;
	} OPT_UPDATE;

} // end of sfopt namespace

namespace sfarray {
	extern const sfopt::OPT_SETTING aOptions[];
}

class CSrcOptions
{
public:
	CSrcOptions();
	~CSrcOptions();

	// Class functions

	const char* GetOption(sfopt::OPT_INDEX index);
	bool 		GetBoolOption(sfopt::OPT_INDEX index);
	const char* GetComment(sfopt::OPT_INDEX index);
	bool 		GetChanged(sfopt::OPT_INDEX index);

	bool UpdateOption(sfopt::OPT_INDEX index, const char* pszVal);	// fine to call this for boolean options if pszVal == "true/false" or "yes/no"
	bool UpdateOption(sfopt::OPT_INDEX index, bool bVal);

protected:
	// Class members

	// This structure is similar to OPT_SETTINGS, but is used to store changes to an option

	ttCArray<sfopt::OPT_UPDATE> m_aUpdateOpts;
};
