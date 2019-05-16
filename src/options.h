/////////////////////////////////////////////////////////////////////////////
// Name:		CSrcOptions
// Purpose:		Class for storing/retrieving options in a .srcfiles file
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ttarray.h>	// ttCArray

namespace sfopt {			// .srcfiles otpions
	typedef enum {
		OPT_ERROR = 0,

		OPT_PROJECT,		// name of the project--will be used as the base target name (i.e., project: foo, target: foo.exe, fooD.exe, etc.)
		OPT_PCH,			// name of precompiled header file, or "none" if not using precompiled headers
		OPT_PCH_CPP,		// source file used to build precompiled header (default uses same name as PCH option)
		OPT_EXE_TYPE,		// [window | console | lib | dll]

		// The following are boolean options (true or false)

		OPT_DEBUG_RC,		// true means build a -D_DEBUG version of the project's rc file
		OPT_PERMISSIVE,		// true means add -permissive- compiler flag
		OPT_STDCALL,		// use stdcall calling convention
		OPT_STATIC_CRT,		// true means link to static CRT
		OPT_MS_LINKER,		// use link.exe even when compiling with CLANG
		OPT_MS_RC,			// use rc.exe even when compiling with CLANG

		OPT_CFLAGS_CMN,		// flags to pass to the compiler in all build targets
		OPT_CFLAGS_REL,		// flags to pass to the compiler in release builds
		OPT_CFLAGS_DBG,		// flags to pass to the compiler in debug builds

		OPT_COMPILERS,		// [MSVC or CLANG] default is both, set this option to limit it to one
		OPT_NATVIS,			// Specifies a .natvis file to link into the pdb file

		OPT_LINK_CMN,		// flags to pass to the linker in all build targets
		OPT_LINK_REL,		// flags to pass to the linker in release builds
		OPT_LINK_DBG,		// flags to pass to the linker in debug builds

		OPT_RC_CMN,			// flags to pass to the resource compiler in all build targets
		OPT_RC_REL,			// flags to pass to the resource compiler in release builds
		OPT_RC_DBG,			// flags to pass to the resource compiler in debug builds

		OPT_MDL_CMN,		// flags to pass to the midl compiler in all build targets
		OPT_MDL_REL,		// flags to pass to the midl compiler in release builds
		OPT_MDL_DBG,		// flags to pass to the midl compiler in debug builds

		OPT_CLANG_CMN,		// flags to pass to the CLANG compiler in all build targets
		OPT_CLANG_REL,		// flags to pass to the CLANG compiler in release builds
		OPT_CLANG_DBG,		// flags to pass to the CLANG compiler in debug builds

		OPT_64BIT,			// generate scripts for 64-bit build
		OPT_TARGET_DIR64,	// 64-bit target directory

		OPT_32BIT,			// generate scripts for 32-bit build
		OPT_TARGET_DIR32,	// 32-bit target directory

		OPT_64BIT_SUFFIX,	// true means append "64" to target's directory or .exe name
		OPT_32BIT_SUFFIX,	// true means append "32" to target's directory or .exe name

		OPT_INC_DIRS,		// additional directories for header files
		OPT_LIB_DIRS,		// additional directores for lib files
		OPT_LIBS,			// additional libraries to link to (see OPT_BUILD_LIBS to both build and link to a library)
		OPT_IDE,			// [CodeBlocks CodeLite VisualStudio] -- specifies one or more IDE go generate project files for
		OPT_MAKEFILE,		// [never | missing | always] -- default, if not specified, is missing
		OPT_OPTIMIZE,		// [space | speed] optimization (optimizing for speed can actually make the code run slower due to caching issues) -- default, if not specified, is space
		OPT_WARN_LEVEL,		// [1-4] default, if not specified, is 4
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
		bool	bRequired;
	} OPT_UPDATE;

} // end of sfopt namespace

class CSrcOptions
{
public:
	CSrcOptions();
	~CSrcOptions();

	// Class functions

	const char* GetOption(sfopt::OPT_INDEX index);
	bool 		GetBoolOption(sfopt::OPT_INDEX index);
	const char* GetOptComment(sfopt::OPT_INDEX index);
	bool 		GetChanged(sfopt::OPT_INDEX index);		// returns true if the option has changed

	bool 		GetRequired(sfopt::OPT_INDEX index);	// returns true if the option is required
	void 		SetRequired(sfopt::OPT_INDEX index, bool bVal = true);

	bool UpdateOption(sfopt::OPT_INDEX index, const char* pszVal);	// fine to call this for boolean options if pszVal == "true/false" or "yes/no"
	bool UpdateOption(sfopt::OPT_INDEX index, bool bVal);
	bool UpdateReadOption(const char* pszName, const char* pszVal, const char* pszComment);

	const sfopt::OPT_SETTING* GetOrgOptions();

protected:
	// Class members

	// This structure is similar to OPT_SETTINGS, but is used to store changes to an option

	ttCArray<sfopt::OPT_UPDATE> m_aUpdateOpts;
};
