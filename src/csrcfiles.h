/////////////////////////////////////////////////////////////////////////////
// Name:		CSrcFiles
// Purpose:		Class for reading/writing .SrcFile (master file used by makemake.exe to generate build scripts)
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __CSRCFILES_H__
#define __CSRCFILES_H__

#include <ttlist.h>			// ttCList, ttCDblList, ttCStrIntList
#include <ttstr.h>			// ttStr, ttCWD
#include <ttfile.h>			// ttCFile
#include <ttarray.h>		// ttCArray
#include <ttmap.h>			// ttCMap

#include "options.h"		// CSrcOptions

using namespace sfopt;		// OPT_ options are used extensively, hence using the namespace in the header file

extern const char* txtSrcFilesFileName;

class CSrcFiles : public CSrcOptions
{
public:
	CSrcFiles();

	// Class functions

	void AddFile(const char* pszFile) { m_lstSrcFiles += pszFile; }
	bool ReadFile(const char* pszFile = txtSrcFilesFileName);
	bool ReadTwoFiles(const char* pszMaster, const char* pszPrivate);

	bool IsProcessed() { return m_bRead; }

	bool IsCodeBlockIDE()	 { return (tt::FindStrI(GetOption(OPT_IDE), "CodeBlocks")); }
	bool IsCodeLiteIDE()	 { return (tt::FindStrI(GetOption(OPT_IDE), "CodeLite")); }
	bool IsVisualStudioIDE() { return (tt::FindStrI(GetOption(OPT_IDE), "VisualStudio")); }

	bool IsExeTypeConsole()	 { return (tt::FindStrI(GetOption(OPT_EXE_TYPE), "console")); }	// this is the default
	bool IsExeTypeDll()		 { return (tt::FindStrI(GetOption(OPT_EXE_TYPE), "dll")); }
	bool IsExeTypeLib()		 { return (tt::FindStrI(GetOption(OPT_EXE_TYPE), "lib")); }
	bool IsExeTypeWindow()	 { return (tt::FindStrI(GetOption(OPT_EXE_TYPE), "window")); }

	bool IsMakeMissing()	 { return (tt::FindStrI(GetOption(OPT_MAKEFILE), "missing")); }	// this is the default
	bool IsMakeNever()		 { return (tt::FindStrI(GetOption(OPT_MAKEFILE), "never")); }
	bool IsMakeAlways()		 { return (tt::FindStrI(GetOption(OPT_MAKEFILE), "always")); }

	bool IsOptimizeSpeed()	 { return (tt::FindStrI(GetOption(OPT_OPTIMIZE), "speed")); }

	// if not specified, both compilers are used
	bool IsCompilerMSVC()	{ return (!GetOption(OPT_COMPILERS) || tt::FindStrI(GetOption(OPT_COMPILERS), "MSVC")); }
	bool IsCompilerClang()	{ return (!GetOption(OPT_COMPILERS) || tt::FindStrI(GetOption(OPT_COMPILERS), "CLANG")); }

	const char* GetDir32()	{ return GetOption(OPT_TARGET_DIR32); }	// 32-bit target directory
	const char* GetDir64()	{ return GetOption(OPT_TARGET_DIR64); }	// 64-bit target directory

	const char* GetBuildLibs() { return GetOption(OPT_BUILD_LIBS); }

	void AddSourcePattern(const char* pszFilePattern);

	// These are just for convenience--it's fine to call GetOption directly

	const char* GetProjectName() { return GetOption(OPT_PROJECT); }
	const char* GetPchHeader();

protected:
	void ProcessFile(char* pszFile);
	void ProcessInclude(const char* pszFile, ttCStrIntList& lstAddSrcFiles, bool bFileSection);
	void ProcessLibSection(char* pszLibFile);
	void ProcessOption(char* pszLine);
	void ProcessTarget(char* pszLine);

	void AddCompilerFlag(const char* pszFlag);
	void AddLibrary(const char* pszName);

	// Class members (note that these are NOT marked protected or private -- too many callers need to access individual members)

public:
	ttCStr m_cszLibName;		// name and location of any additional library to build (used by Lib: section)
	ttCStr m_cszRcName;			// resource file to build (if any)
	ttCStr m_cszHHPName;		// HTML Help project file

	ttCHeap m_ttHeap;			// all the ttCList files will be attatched to this heap
	ttCList m_lstSrcFiles;
	ttCList m_lstLibFiles;		// list of any files used to build additional library
	ttCList m_lstIdlFiles;		// list of any idl files to compile with midl compiler

	ttCDblList m_lstDepLibs;	// key is library, val is src (if any)

	ttCList m_lstErrors;		// list of any errors that occurred during processing

	ttCStrIntList m_lstAddSrcFiles;		// additional .srcfiles to read into Files: section
	ttCStrIntList m_lstLibAddSrcFiles;	// additional .srcfiles to read into Lib: section
	ttCList		  m_lstSrcIncluded;		// the names of all files included by all ".include path/.srcfiles" directives

protected:
	bool GetOptionParts(char* pszLine, ttCStr& cszName, ttCStr& cszVal, ttCStr& cszComment);

	bool m_bRead;				// file has been read and processed
	bool m_bReadingPrivate;		// true if we are reading a private file
};

#endif	// __CSRCFILES_H__
