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

#include <ttlist.h> 		// ttCList, ttCDblList, ttCStrIntList
#include <ttstr.h>			// ttStr, ttCWD
#include <ttfile.h> 		// ttCFile
#include <ttarray.h>		// ttCArray
#include <ttmap.h>			// ttCMap

#include "srcoptions.h" 	// CSrcOption

typedef enum {
	OPT_ERROR = 0,

	OPT_PROJECT,		// name of the project--will be used as the base target name (i.e., project: foo, target: foo.exe, fooD.exe, etc.)
	OPT_PCH,			// name of precompiled header file, or "none" if not using precompiled headers

	OPT_64BIT,			// if true, enable 64-bit support (link with 64-bit libraries).
	OPT_BIT_SUFFIX,		// true means append "64" to target's directory or .exe name
	OPT_DEBUG_RC,		// true means build a -D_DEBUG version of the project's rc file
	OPT_PERMISSIVE, 	// true means add -permissive- compiler flag
	OPT_STDCALL,		// use stdcall calling convention
	OPT_STATIC_CRT,		// true means link to static CRT
	OPT_MS_LINKER,		// use link.exe even when compiling with CLANG

	OPT_CFLAGS,			// additional flags to pass to the compiler in all build targets
	OPT_MIDL_FLAGS,		// flags to pass to the midl compiler
	OPT_LINK_FLAGS,		// additional flags to pass to the linker in all build targets
	OPT_RC_FLAGS,		// additional flags to pass to the resource compiler in all build targets
	OPT_INC_DIRS,		// additional directories for header files
	OPT_LIB_DIRS,		// additional directores for lib files
	OPT_LIBS,			// additional libraries to link to (see OPT_BUILD_LIBS to both build and link to a library)

	OPT_OVERFLOW
} OPT_INDEX;

extern const char* txtSrcFilesFileName;

class CSrcFiles
{
public:
	CSrcFiles();
	~CSrcFiles();

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

	void AddSourcePattern(const char* pszFilePattern);

public:
	const char* GetOption(OPT_INDEX index);
	bool		GetBoolOption(OPT_INDEX index);	 // boolean options are strings ("true" or "false") but this makes checking the value easier

	const char* GetOptionComment(OPT_INDEX index);
	const char* GetOptionName(OPT_INDEX index);

	OPT_INDEX   UpdateOption(const char* pszName, const char* pszValue, const char* pszComment = nullptr);
	void        UpdateOption(OPT_INDEX index, const char* pszValue, const char* pszComment = nullptr);
	void        UpdateOption(OPT_INDEX index, bool bValue, const char* pszComment = nullptr);

	// These are just for convenience--it's fine to call GetOption directly

	const char* GetProjectName() { return GetOption(OPT_PROJECT); }
	const char* GetPchHeader()   { return GetOption(OPT_PCH); }

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
	bool m_bBuildForSpeed;		// optimize for fast code instead of small code (optimizing for speed can actually make the code run slower due to caching issues)

	EXE_TYPE m_exeType;			// EXE_WINDOW, EXE_CONSOLE, EXE_LIB, EXE_DLL or EXE_DEFAULT

	size_t m_WarningLevel;		// warning level to use (1-4) -- can also use WARNLVL_1-4 and WARNLEVEL_DEFAULT
	size_t m_CompilerType;		// COMPILER_CLANG, COMPILER_MSVC or COMPILER_DEFAULT
	size_t m_IDE;				// IDE_CODEBLOCK, IDE_CODELITE, IDE_VS or IDE_NONE

	ttCStr m_cszBuildLibs;		// libraries that need to be built (added to makefile generation)

	ttCStr m_cszTarget32;		// target directory for non-64 bit builds
	ttCStr m_cszTarget64;		// target directory for 64 bit builds

	ttCStr m_cszLibName;		// name and location of any additional library to build (used by Lib: section)
	ttCStr m_cszRcName;			// resource file to build (if any)
	ttCStr m_cszHHPName;		// HTML Help project file

	ttCHeap m_ttHeap;			// all the ttCList files will be attatched to this heap
	ttCList m_lstSrcFiles;
	ttCList m_lstLibFiles;		// list of any files used to build additional library
	ttCList m_lstIdlFiles;		// list of any idl files to compile with midl compiler

	ttCDblList m_lstDepLibs; 	// key is library, val is src (if any)

	ttCList m_lstErrors; 		// list of any errors that occurred during processing

	ttCStrIntList m_lstAddSrcFiles;		// additional .srcfiles to read into Files: section
	ttCStrIntList m_lstLibAddSrcFiles;	// additional .srcfiles to read into Lib: section
	ttCList		  m_lstSrcIncluded;		// the names of all files included by all ".include path/.srcfiles" directives

	// Following are for the makefile: section

	size_t	m_fCreateMakefile;		// MAKEMAKE_NEVER, MAKEMAKE_MISSING or MAKEMAKE_ALWAYS

protected:
	bool GetOptionParts(char* pszLine, ttCStr& cszName, ttCStr& cszVal, ttCStr& cszComment);

	bool m_bRead;				// file has been read and processed
	bool m_bReadingPrivate;		// true if we are reading a private file

	typedef struct {
		const char* pszName;
		bool* pbVal;
		char* pszComment;
	} OPT_BOOL;
	ttCArray<OPT_BOOL> m_aOptBool;

	typedef struct {
		const char* pszName;
		ttCStr*	pcszVal;
		char*   pszComment;
		bool	bRequired;
	} OPT_VAL;
	ttCArray<OPT_VAL> m_aOptVal;

	void AddOptVal(const char* pszName, bool*	pbVal, const char* pszComment = nullptr);
	bool UpdateOptVal(const char* pszName, bool bVal, const char* pszComment);

	void AddOptVal(const char* pszName, ttCStr* pcszVal, const char* pszComment = nullptr);
	bool UpdateOptVal(const char* pszName, const char* pszVal, const char* pszComment);

protected:
	ttCMap<OPT_INDEX, CSrcOption*> m_aOptions;

	void AddOption(OPT_INDEX opt, const char* pszName, bool bBoolean = false, bool bRequired = false);
};

#endif	// __CSRCFILES_H__
