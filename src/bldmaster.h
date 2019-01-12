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

#include "../ttLib/include/ttlist.h"	// ttList, ttDblList, ttStrIntList

#include "../common/csrcfiles.h"		// CSrcFiles

class CBldMaster : public CSrcFiles
{
public:
	CBldMaster(bool bReadPrivate = true);

	// Class functions

	size_t GetErrorCount() { return m_lstErrors.GetCount(); }
	const char* GetError(size_t pos) { return m_lstErrors[pos]; }
	void AddError(const char* pszErrMsg) { m_lstErrors += pszErrMsg; }

	size_t getSrcCount() { return m_lstSrcFiles.GetCount(); }

	void setProjName(const char* pszName) { if (pszName && *pszName) m_cszProjectName = pszName; }
	bool CreateMakeFile();

	bool isPermissive()	{ return m_bPermissive; }
	bool isStaticCrt()	{ return m_bStaticCrt; }
	bool isBuildSpeed()	{ return m_bBuildForSpeed; }

	size_t getWarnLevel() { return m_WarningLevel > 0 ? m_WarningLevel : WARNLEVEL_DEFAULT; }

	bool isCompilerMSVC()	{ return (m_CompilerType & COMPILER_MSVC); }
	bool isCompilerClang()	{ return (m_CompilerType & COMPILER_CLANG); }

	bool isCodeBlockIDE()	 { return (m_IDE & IDE_CODEBLOCK); }
	bool isCodeLiteIDE()	 { return (m_IDE & IDE_CODELITE); }
	bool isVisualStudioIDE() { return (m_IDE & IDE_VS); }

	const char* getAddCFlags()	{ return m_cszCFlags; }
	const char* getAddLFlags()	{ return m_cszLinkFlags; }
	const char* getAddIncDirs()	{ return m_cszIncDirs; }
	const char* getAddLibDirs()	{ return m_cszLibDirs; }
	const char* getRcFile()		{ return m_cszRcName; }

	const char* getDir32()	{ return m_cszTarget32; }	// 32-bit target directory
	const char* getDir64()	{ return m_cszTarget64; }	// 64-bit target directory

	ttList* getSrcFileList()  { return &m_lstSrcFiles; }
	ttList* getLibFileList()  { return &m_lstLibFiles; }
	ttList* getRcDepList()  	{ return &m_lstRcDependencies; }

	bool is64BitBuild()		{ return m_b64bit; }
	bool is64BitSuffix()	{ return m_bBitSuffix; }
	bool isStdcall()		{ return m_bStdcall; }

	bool isExeTypeConsole()	{ return (m_exeType == EXE_CONSOLE); }
	bool isExeTypeDll()		{ return (m_exeType == EXE_DLL); }
	bool isExeTypeLib()		{ return (m_exeType == EXE_LIB); }
	bool isExeTypeWindow()	{ return (m_exeType == EXE_WINDOW); }

	bool isBin64()			{ return m_bBin64Exists; }

	const char* getAddLibs()	{ return m_cszLibs; }
	const char* getBuildLibs()	{ return m_cszBuildLibs; }

	const char* getCFlags() 	{ return m_cszCFlags; }
	const char* getIncDirs()	{ return m_cszIncDirs; }
	const char* getLibDirs()	{ return m_cszLibDirs; }
	const char* getLibName()	{ return m_cszLibName; }		// name and location of any additional library to build
	const char* getLinkFlags()	{ return m_cszLinkFlags; }

	const char* getProjName()	{ return m_cszProjectName; }
	const char* getPchName()	{ return m_cszPCHheader; }
	const char* getHHPName()	{ return m_cszHHPName; }

	const char* GetTargetDebug();
	const char* GetTargetDebug64();
	const char* GetTargetRelease();
	const char* GetTargetRelease64();

	ttList m_lstRcDependencies;

protected:
	bool FindRcDependencies(const char* pszSrc, const char* pszHdr = nullptr, const char* pszRelPath = nullptr);
	const char* NormalizeHeader(const char* pszBaseFile, ttString& cszHeader);

	// Class members

	ttString cszTargetDebug;
	ttString cszTargetDebug64;
	ttString cszTargetRelease;
	ttString cszTargetRelease64;

	bool m_bBin64Exists;	// if true, the directory ../bin64 exists
};
