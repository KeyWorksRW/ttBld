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

#include "../ttLib/include/ttlist.h"	// ttList, ttDblList, ttStrIntList
#include "../ttLib/include/ttstring.h"	// ttString
#include "../ttLib/include/ttfile.h"	// ttFile

extern const char* txtSrcFilesFileName;

class CSrcFiles
{
public:
	CSrcFiles();

	typedef enum {
		EXE_UNSPECIFIED,
		EXE_WINDOW,			// CSrcFiles::EXE_WINDOW
		EXE_CONSOLE,		// CSrcFiles::EXE_CONSOLE
		EXE_LIB,			// CSrcFiles::EXE_LIB
		EXE_DLL,			// CSrcFiles::EXE_DLL
		EXE_DEFAULT = EXE_CONSOLE
	} EXE_TYPE;

	enum {
		COMPILER_CLANG	 = 1 << 0,
		COMPILER_MSVC	 = 1 << 1,
		COMPILER_DEFAULT = (COMPILER_CLANG | COMPILER_MSVC)
	};

	enum {
		IDE_NONE	  = 0,
		IDE_CODEBLOCK = 1 << 1,
		IDE_CODELITE  = 1 << 2,
		IDE_VS		  = 1 << 3,
	};

	enum {
		MAKEMAKE_NEVER = 0,
		MAKEMAKE_MISSING = 1,
		MAKEMAKE_ALWAYS = 2,
		MAKEMAKE_DEFAULT = MAKEMAKE_MISSING		// ../MakeSrcFiles/writeupdates.cpp doesn't fully this, so if it's not MAKEMAKE_MISSING, will need to be changed
	};

	enum {
		WARNLEVEL_1 = 1,
		WARNLEVEL_2 = 2,
		WARNLEVEL_3 = 3,
		WARNLEVEL_4 = 4,
		WARNLEVEL_DEFAULT = WARNLEVEL_4
	};

public:
	// Class functions

	void AddFile(const char* pszFile) { m_lstSrcFiles += pszFile; }
	bool ReadFile(const char* pszFile = txtSrcFilesFileName);
	bool ReadTwoFiles(const char* pszMaster, const char* pszPrivate);

	bool IsProcessed() { return m_bRead; }

	bool isWindowApp() { return m_exeType == EXE_WINDOW; }
	bool isConsoleApp() { return m_exeType == EXE_CONSOLE; }
	bool isLibApp() { return m_exeType == EXE_LIB; }
	bool isDllApp() { return m_exeType == EXE_DLL; }

	const char* GetProjectName() { return m_cszProjectName; }

	void AddSourcePattern(const char* pszFilePattern);

protected:
	void ProcessFile(char* pszFile);
	void ProcessInclude(const char* pszFile, ttStrIntList& lstAddSrcFiles, bool bFileSection);
	void ProcessLibSection(char* pszLibFile);
	void ProcessOption(char* pszLine);
	void ProcessTarget(char* pszLine);

	void AddCompilerFlag(const char* pszFlag);
	void AddLibrary(const char* pszName);

	// Class members (note that these are NOT marked protected or private -- too many callers need to access individual members)

public:
	bool m_b64bit;				// if true, enable 64-bit support (link with 64-bit libraries).
	bool m_bBitSuffix;			// true means append "64" to target's directory or .exe name
	bool m_bBuildForSpeed;		// optimize for fast code instead of small code (optimizing for speed can actually make the code run slower due to caching issues)
	bool m_bPermissive;			// true means add -permissive- compiler flag
	bool m_bStaticCrt;			// true means link to static CRT
	bool m_bStdcall;			// use stdcall calling convention
	bool m_bUseMsvcLinker;		// use link.exe even when compiling with CLANG

	EXE_TYPE m_exeType;			// EXE_WINDOW, EXE_CONSOLE, EXE_LIB, EXE_DLL or EXE_DEFAULT

	size_t m_WarningLevel;		// warning level to use (1-4) -- can also use WARNLVL_1-4 and WARNLEVEL_DEFAULT
	size_t m_CompilerType;		// COMPILER_CLANG, COMPILER_MSVC or COMPILER_DEFAULT
	size_t m_IDE;				// IDE_CODEBLOCK, IDE_CODELITE, IDE_VS or IDE_NONE

	ttString m_cszLibs;				// additional libraries to link to
	ttString m_cszBuildLibs;		// libraries that need to be built (added to makefile generation)

	ttString m_cszLibDirs;			// additional library directories to search
	ttString m_cszIncDirs;			// additional include directories to search

	ttString m_cszCFlags;			// additional flags to pass to the compiler in all build targets
	ttString m_cszLinkFlags;		// additional flags to pass to the linker in all build targets

	ttString m_cszTarget32;			// target directory for non-64 bit builds
	ttString m_cszTarget64;			// target directory for 64 bit builds

	ttString m_cszMidlFlags;		// flags to pass to the midl compiler
	ttString m_cszLibName;			// name and location of any additional library to build (used by Lib: section)
	ttString m_cszLibPCHheader;		// header file to use for Lib precompilation
	ttString m_cszPCHheader;		// header file to use for precompilation (defaults to precomp.h). Assumes <name>.cpp is the file to compile
	ttString m_cszProjectName;		// name of the project
	ttString m_cszRcName;			// resource file to build (if any)
	ttString m_cszHHPName;			// HTML Help project file
	ttString m_cszSrcPattern;		// Specifies one or more wildcards to add to Files: section

	ttList m_lstSrcFiles;
	ttList m_lstLibFiles;		// list of any files used to build additional library
	ttList m_lstIdlFiles;		// list of any idl files to compile with midl compiler

	ttDblList m_lstDepLibs; 	// key is library, val is src (if any)

	ttList m_lstErrors; 		// list of any errors that occurred during processing

	ttStrIntList m_lstAddSrcFiles;		// additional .srcfiles to read into Files: section
	ttStrIntList m_lstLibAddSrcFiles;	// additional .srcfiles to read into Lib: section
	ttList		 m_lstSrcIncluded;		// the names of all files included by all ".include path/.srcfiles" directives

	// Following are for the makefile: section

	size_t	m_fCreateMakefile;		// MAKEMAKE_NEVER, MAKEMAKE_MISSING or MAKEMAKE_ALWAYS

protected:
	bool m_bRead;				// file has been read and processed
	bool m_bReadingPrivate;		// true if we are reading a private file
};

#endif	// __CSRCFILES_H__
