/////////////////////////////////////////////////////////////////////////////
// Name:		bldmaster.h
// Purpose:		Class providing interface to CSrcFiles
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

// All CSrcFiles members are public so they can be accessed directly, however this class provides a level of abstraction
// that allows CSrcFiles to change without breaking any of it's callers.

// In addition to the abstraction layer, this class also adds specific initialization and methods needed for actually
// creating build scripts. Note that nothing in this class is specific to building .ninja scripts -- which is why it's
// used by the classes that create IDE project files.

#pragma once

#include <ttlist.h> 					// ttCList, ttCDblList, ttCStrIntList

#include "csrcfiles.h"		// CSrcFiles
#include "dryrun.h"			// CDryRun

class CBldMaster : public CSrcFiles
{
public:
	CBldMaster(bool bReadPrivate = true);

	// Class functions

	size_t GetErrorCount() { return m_lstErrors.GetCount(); }
	const char* GetError(size_t pos) { return m_lstErrors[pos]; }
	void AddError(const char* pszErrMsg) { m_lstErrors += pszErrMsg; }

	size_t getSrcCount() { return m_lstSrcFiles.GetCount(); }

	bool CreateMakeFile();

	// if not specified, both compilers are used
	bool IsCompilerMSVC()	{ return (!GetOption(OPT_COMPILERS) || tt::FindStrI(GetOption(OPT_COMPILERS), "MSVC")); }
	bool IsCompilerClang()	{ return (!GetOption(OPT_COMPILERS) || tt::FindStrI(GetOption(OPT_COMPILERS), "CLANG")); }

	const char* GetRcFile()		{ return m_cszRcName; }

	const char* GetDir32()	{ return GetOption(OPT_TARGET_DIR32); }	// 32-bit target directory
	const char* GetDir64()	{ return GetOption(OPT_TARGET_DIR64); }	// 64-bit target directory

	ttCList* GetSrcFileList()  { return &m_lstSrcFiles; }
	ttCList* GetLibFileList()  { return &m_lstLibFiles; }
	ttCList* GetRcDepList()  	{ return &m_lstRcDependencies; }

	bool IsBin64()			{ return m_bBin64Exists; }

	const char* GetLibName()	{ return m_cszLibName; }		// name and location of any additional library to build

	const char* GetHHPName()	{ return m_cszHHPName; }

	const char* GetTargetDebug();
	const char* GetTargetDebug64();
	const char* GetTargetRelease();
	const char* GetTargetRelease64();

	void EnableDryRun() { dryrun.Enable(); }

	ttCList m_lstRcDependencies;

protected:
	bool FindRcDependencies(const char* pszSrc, const char* pszHdr = nullptr, const char* pszRelPath = nullptr);
	const char* NormalizeHeader(const char* pszBaseFile, ttCStr& cszHeader);

	// Class members

	ttCStr cszTargetDebug;
	ttCStr cszTargetDebug64;
	ttCStr cszTargetRelease;
	ttCStr cszTargetRelease64;

	CDryRun dryrun;

	bool m_bBin64Exists;	// if true, the directory ../bin64 exists
};
