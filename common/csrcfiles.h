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

#include <ttlist.h> 					// ttCList, ttCDblList, ttCStrIntList
#include <ttstr.h>						// ttStr, ttCWD
#include <ttfile.h> 					// ttCFile
#include <ttarray.h>					// ttCArray

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
	void ProcessInclude(const char* pszFile, ttCStrIntList& lstAddSrcFiles, bool bFileSection);
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
	bool m_bDebugRC;			// indicates where or not to build a _DEBUG version of the resource file

	EXE_TYPE m_exeType;			// EXE_WINDOW, EXE_CONSOLE, EXE_LIB, EXE_DLL or EXE_DEFAULT

	size_t m_WarningLevel;		// warning level to use (1-4) -- can also use WARNLVL_1-4 and WARNLEVEL_DEFAULT
	size_t m_CompilerType;		// COMPILER_CLANG, COMPILER_MSVC or COMPILER_DEFAULT
	size_t m_IDE;				// IDE_CODEBLOCK, IDE_CODELITE, IDE_VS or IDE_NONE

	ttCStr m_cszLibs;			// additional libraries to link to
	ttCStr m_cszBuildLibs;		// libraries that need to be built (added to makefile generation)

	ttCStr m_cszLibDirs;		// additional library directories to search
	ttCStr m_cszIncDirs;		// additional include directories to search

	ttCStr m_cszCFlags;			// additional flags to pass to the compiler in all build targets
	ttCStr m_cszLinkFlags;		// additional flags to pass to the linker in all build targets
	ttCStr m_cszRCFlags;		// additional flags to pass to the resource compiler in all build targets
	ttCStr m_cszMidlFlags;		// flags to pass to the midl compiler

	ttCStr m_cszTarget32;		// target directory for non-64 bit builds
	ttCStr m_cszTarget64;		// target directory for 64 bit builds

	ttCStr m_cszLibName;		// name and location of any additional library to build (used by Lib: section)
	ttCStr m_cszLibPCHheader;	// header file to use for Lib precompilation
	ttCStr m_cszPCHheader;		// header file to use for precompilation (defaults to precomp.h). Assumes <name>.cpp is the file to compile
	ttCStr m_cszProjectName;	// name of the project
	ttCStr m_cszRcName;			// resource file to build (if any)
	ttCStr m_cszHHPName;		// HTML Help project file
	ttCStr m_cszSrcPattern;		// Specifies one or more wildcards to add to Files: section

	ttCHeap m_ttHeap;			// all the ttCList files will be attatched to this heap
	ttCList m_lstSrcFiles;
	ttCList m_lstLibFiles;		// list of any files used to build additional library
	ttCList m_lstIdlFiles;		// list of any idl files to compile with midl compiler

	ttCDblList m_lstDepLibs; 	// key is library, val is src (if any)

	ttCList m_lstErrors; 		// list of any errors that occurred during processing

	ttCStrIntList m_lstAddSrcFiles;		// additional .srcfiles to read into Files: section
	ttCStrIntList m_lstLibAddSrcFiles;	// additional .srcfiles to read into Lib: section
	ttCList		 m_lstSrcIncluded;		// the names of all files included by all ".include path/.srcfiles" directives

	// Following are for the makefile: section

	size_t	m_fCreateMakefile;		// MAKEMAKE_NEVER, MAKEMAKE_MISSING or MAKEMAKE_ALWAYS

protected:
	bool m_bRead;				// file has been read and processed
	bool m_bReadingPrivate;		// true if we are reading a private file

	typedef struct {
		const char* pszOption;
		ttCStr*	pcszVal;
	} OPT_VAL;
	ttCArray<OPT_VAL> m_aOptVal;

	void AddOptVal(const char* pszOption, ttCStr*	pcszVal);
	bool UpdateOptVal(const char* pszKey, const char* pszVal);
};

#endif	// __CSRCFILES_H__
