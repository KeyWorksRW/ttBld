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
	OPT_EXE_TYPE,		// [window | console | lib | dll]

	// The following are boolean options (true or false)

	OPT_64BIT,			// if true, enable 64-bit support (link with 64-bit libraries).
	OPT_BIT_SUFFIX,		// true means append "64" to target's directory or .exe name
	OPT_DEBUG_RC,		// true means build a -D_DEBUG version of the project's rc file
	OPT_PERMISSIVE, 	// true means add -permissive- compiler flag
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

extern const char* txtSrcFilesFileName;

class CSrcFiles
{
public:
	CSrcFiles();
	~CSrcFiles();

	// Class functions

	void AddFile(const char* pszFile) { m_lstSrcFiles += pszFile; }
	bool ReadFile(const char* pszFile = txtSrcFilesFileName);
	bool ReadTwoFiles(const char* pszMaster, const char* pszPrivate);

	bool IsProcessed() { return m_bRead; }

	bool isCodeBlockIDE()	 { return (GetOption(OPT_IDE) && tt::findStri(GetOption(OPT_IDE), "CodeBlocks")); }
	bool isCodeLiteIDE()	 { return (GetOption(OPT_IDE) && tt::findStri(GetOption(OPT_IDE), "CodeLite")); }
	bool isVisualStudioIDE() { return (GetOption(OPT_IDE) && tt::findStri(GetOption(OPT_IDE), "VisualStudio")); }

	bool isExeTypeConsole()	 { return (!GetOption(OPT_EXE_TYPE) || tt::findStri(GetOption(OPT_EXE_TYPE), "console")); }	// this is the default
	bool isExeTypeDll()		 { return (GetOption(OPT_EXE_TYPE) && tt::findStri(GetOption(OPT_EXE_TYPE), "dll")); }
	bool isExeTypeLib()		 { return (GetOption(OPT_EXE_TYPE) && tt::findStri(GetOption(OPT_EXE_TYPE), "lib")); }
	bool isExeTypeWindow()	 { return (GetOption(OPT_EXE_TYPE) && tt::findStri(GetOption(OPT_EXE_TYPE), "window")); }

	bool isMakeMissing()	 { return (!GetOption(OPT_MAKEFILE) || tt::findStri(GetOption(OPT_MAKEFILE), "missing")); }	// this is the default
	bool isMakeNever()		 { return (GetOption(OPT_MAKEFILE) && tt::findStri(GetOption(OPT_MAKEFILE), "never")); }
	bool isMakeAlways()		 { return (GetOption(OPT_MAKEFILE) && tt::findStri(GetOption(OPT_MAKEFILE), "always")); }

	bool isOptimizeSpeed()	 { return (GetOption(OPT_OPTIMIZE) && tt::findStri(GetOption(OPT_OPTIMIZE), "speed")); }

	const char* getBuildLibs() { return GetOption(OPT_BUILD_LIBS); }

	void AddSourcePattern(const char* pszFilePattern);

	const char* GetOption(OPT_INDEX index);
	bool		GetBoolOption(OPT_INDEX index);	 // boolean options are strings ("true" or "false") but this makes checking the value easier

	const char* GetOptionComment(OPT_INDEX index);
	const char* GetOptionName(OPT_INDEX index);
	bool		isOptionRequired(OPT_INDEX index);	// use to determine if option must be written when .srcfiles is saved

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

protected:
	bool GetOptionParts(char* pszLine, ttCStr& cszName, ttCStr& cszVal, ttCStr& cszComment);

	bool m_bRead;				// file has been read and processed
	bool m_bReadingPrivate;		// true if we are reading a private file

protected:
	ttCMap<OPT_INDEX, CSrcOption*> m_aOptions;
	OPT_INDEX m_lastIndex;
	ptrdiff_t m_pos;

	void AddOption(OPT_INDEX opt, const char* pszName, bool bBoolean = false, bool bRequired = false);
	void SetRequired(OPT_INDEX index);
};

#endif	// __CSRCFILES_H__
