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

#include "../common/csrcfiles.h"		// CSrcFiles
#include "../common/dryrun.h"			// CDryRun

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

	bool isBuildSpeed()	{ return m_bBuildForSpeed; }

	size_t getWarnLevel() { return m_WarningLevel > 0 ? m_WarningLevel : WARNLEVEL_DEFAULT; }

	bool isCompilerMSVC()	{ return (m_CompilerType & COMPILER_MSVC); }
	bool isCompilerClang()	{ return (m_CompilerType & COMPILER_CLANG); }

	bool isCodeBlockIDE()	 { return (m_IDE & IDE_CODEBLOCK); }
	bool isCodeLiteIDE()	 { return (m_IDE & IDE_CODELITE); }
	bool isVisualStudioIDE() { return (m_IDE & IDE_VS); }

	const char* getRcFile()		{ return m_cszRcName; }

	const char* getDir32()	{ return m_cszTarget32; }	// 32-bit target directory
	const char* getDir64()	{ return m_cszTarget64; }	// 64-bit target directory

	ttCList* getSrcFileList()  { return &m_lstSrcFiles; }
	ttCList* getLibFileList()  { return &m_lstLibFiles; }
	ttCList* getRcDepList()  	{ return &m_lstRcDependencies; }

	bool isExeTypeConsole()	{ return (m_exeType == EXE_CONSOLE); }
	bool isExeTypeDll()		{ return (m_exeType == EXE_DLL); }
	bool isExeTypeLib()		{ return (m_exeType == EXE_LIB); }
	bool isExeTypeWindow()	{ return (m_exeType == EXE_WINDOW); }

	bool isBin64()			{ return m_bBin64Exists; }

	const char* getBuildLibs()	{ return m_cszBuildLibs; }

	const char* getLibName()	{ return m_cszLibName; }		// name and location of any additional library to build

	const char* getHHPName()	{ return m_cszHHPName; }

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
