/////////////////////////////////////////////////////////////////////////////
// Name:		CreateMakeFile()
// Purpose:		Creates an MS nmake and GNU-make compatible makefile
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2018 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#include "../ttLib/include/keyfile.h"	// CKeyFile

#include "bldmaster.h"	// CBldMaster
#include "resource.h"	// IDR_MAKEFILE

bool CBldMaster::CreateMakeFile()
{
	if (m_fCreateMakefile == MAKEMAKE_NEVER)
		return true;	// user doesn't want makefile created at all
	else if (m_fCreateMakefile == MAKEMAKE_MISSING) {
		if (FileExists("makefile"))
			return true;	// file exists, user doesn't want us to update it
	}

	// We get here if MAKEMAKE_ALWAYS is set, or MAKEMAKE_MISSING is set and the makefile doesn't exist

	CKeyFile kf;
	// 64-bit only builds use the _64 variation
	DWORD resID = (is64BitBuild() && !is64BitSuffix()) ? IDR_MAKEFILE_64 : IDR_MAKEFILE;

	// We special-case ttLib since it's used in almost all KeyWorksRW/Randalph repositories

	if (DirExists("../ttLib/") && !IsSameString(m_cszProjectName, "ttLib"))
		resID = (is64BitBuild() && !is64BitSuffix()) ? IDR_MAKEFILE_TTLIB_64 : IDR_MAKEFILE_TTLIB;

	if (!kf.ReadResource(resID)) {
		m_lstErrors += "MakeNinja.exe is corrupted -- unable to read the required resource for creating a makefile!";
		return false;
	}

	if (m_MakeFileCompiler == COMPILER_DEFAULT)	{
		if (m_CompilerType == COMPILER_DEFAULT)
			m_MakeFileCompiler = COMPILER_CLANG;	// default to CLANG compiler
		else {
			if (m_CompilerType & COMPILER_CLANG)
				m_MakeFileCompiler = COMPILER_CLANG;	// default to CLANG compiler
			else
				m_MakeFileCompiler = COMPILER_MSVC;
		}
	}

	const char* pszCompiler = "clang";
	if (m_MakeFileCompiler == COMPILER_MSVC)
		pszCompiler = "msvc";

	while (kf.ReplaceStr("%defgoal%", m_cszDefaultTarget.IsNonEmpty() && !isSameString(m_cszDefaultTarget, "default") ?
		(char*) m_cszDefaultTarget : "release"));

	// The first %compiler% will be in the nmake section. If the user generate .ninja scripts for the MSVC compiler,
	// then we default to using the MSVC compiler in the nmake section. For the mingw32-make section, we use the CLANG
	// compiler if the user generated .ninja scripts for it -- and if not, we default to the MSVC compiler.

	if (m_CompilerType & COMPILER_MSVC)	{
		kf.ReplaceStr("%compiler%", "msvc");
		while (kf.ReplaceStr("%compiler%", m_CompilerType & COMPILER_CLANG ? "clang" : "msvc"));
	}
	else
		while (kf.ReplaceStr("%compiler%", "clang"));

	while (kf.ReplaceStr("%project%", m_cszProjectName));

	// If the makefile already exists, don't write to it unless something has actually changed

	if (FileExists("makefile"))	{
		CKeyFile kfOrg;
		if (!kfOrg.ReadFile("makefile") || strcmp(kfOrg, kf) != 0) {
			if (kf.WriteFile("makefile")) {
				puts("makefile updated");
				return true;
			}
			else {
				puts("unable to write to makefile");
				return false;
			}
		}
	}
	else {
		if (kf.WriteFile("makefile")) {
			puts("makefile created");
			return true;
		}
		else {
			puts("unable to write to makefile");
			return false;
		}
	}

	return true;
}
