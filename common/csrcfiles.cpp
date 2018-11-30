/////////////////////////////////////////////////////////////////////////////
// Name:		CSrcFiles
// Purpose:		Class for reading/writing .srcfiles (master file used by makemake.exe to generate build scripts)
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2018 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#include <stdio.h>	// for sprintf

#include "../ttLib/include/findfile.h"	// CTTFindFile
#include "../ttLib/include/enumstr.h"	// CEnumStr

#include "csrcfiles.h"					// CSrcFiles

const char* txtSrcFilesFileName = ".srcfiles";

typedef enum {
	SECTION_UNKNOWN,
	SECTION_OPTIONS,
	SECTION_FILES,
	SECTION_LIB,
} SRC_SECTION;

CSrcFiles::CSrcFiles()
{
	m_bRead = false;
	m_bReadingPrivate = false;

	m_b64bit = false;
	m_bBitSuffix = false;
	m_bBuildForSpeed = false;
	m_bPermissive = false;
	m_bStaticCrt = false;
	m_bStdcall = false;
	m_bUseMsvcLinker = false;

	m_WarningLevel = WARNLEVEL_DEFAULT;
	m_IDE = IDE_NONE;
	m_CompilerType = COMPILER_DEFAULT;
	m_exeType = EXE_DEFAULT;

	m_fCreateMakefile = MAKEMAKE_DEFAULT;	// create makefile only if it doesn't already exist

	m_lstSrcFiles.SetFlags(CStrList::FLG_URL_STRINGS);
	m_lstLibFiles.SetFlags(CStrList::FLG_URL_STRINGS);
	m_lstErrors.SetFlags(CStrList::FLG_IGNORE_CASE);
}

bool CSrcFiles::ReadFile(const char* pszFile)
{
	CKeyFile kfSrcFiles;
	if (!kfSrcFiles.ReadFile(pszFile))
		return false;
	m_bRead = true;

	char* pszLine;
	SRC_SECTION section = SECTION_UNKNOWN;

	// kfSrcFiles.PrepForReadLine();
	while (kfSrcFiles.readline(&pszLine)) {
		char* pszBegin = FindNonSpace(pszLine);	// ignore any leading spaces
		if (IsEmptyString(pszBegin) || pszBegin[0] == '#' || (pszBegin[0] == '-' && pszBegin[1] == '-' && pszBegin[2] == '-')) {	// ignore empty, comment or divider lines
			continue;
		}
		char* pszComment = kstrchr(pszBegin, '#');	// strip off any comments
		if (pszComment) {
			*pszComment = 0;
			trim(pszBegin);		// remove any trailing white space
		}

		if (IsSameSubString(pszBegin, "%YAML")) {	// not required, but possible a YAML editor could add this
			continue;
		}

		if (IsAlpha(*pszLine)) { 	// sections always begin with an alphabetical character
			if (IsSameSubString(pszBegin, "Files:") || IsSameSubString(pszBegin, "[FILES]")) {
				section = SECTION_FILES;
			}
			else if (IsSameSubString(pszBegin, "Options:") || IsSameSubString(pszBegin, "[OPTIONS]")) {
				section = SECTION_OPTIONS;
			}
			else if (IsSameSubString(pszBegin, "Lib:")) {
				section = SECTION_LIB;
			}
			else
				section = SECTION_UNKNOWN;
			continue;
		}

		switch (section) {
			case SECTION_FILES:
				ProcessFile(pszBegin);
				break;

			case SECTION_LIB:
				ProcessLibSection(pszBegin);
				break;

			case SECTION_OPTIONS:
				ProcessOption(pszBegin);
				break;

			case SECTION_UNKNOWN:
			default:
				break;	// ignore it since we don't know what it's supposed to be used for
		}
	}

	// Everything has been processed, if options were not specified that are needed, make some default assumptions

	if (m_cszProjectName.IsEmpty())	{
		CStr cszProj;
		cszProj.GetCWD();
		char* pszProj = FindFilePortion(cszProj);
		if (pszProj && !IsSameString(pszProj, "src"))
			m_cszProjectName = pszProj;
		else {
			if (pszProj > cszProj.getptr())
				--pszProj;
			*pszProj = 0;
			pszProj = FindFilePortion(cszProj);
			if (pszProj)
				m_cszProjectName = pszProj;
		}
	}

	if (m_exeType == EXE_UNSPECIFIED)
		m_exeType= EXE_WINDOW;

	AddSourcePattern(m_cszSourcePattern);

	// If no Files: or Sources: we're specified, then we still won't have any files to build. Default to every type of C++
	// source file in the current directory.

	if (m_lstSrcFiles.GetCount() < 1) {
		m_cszSourcePattern = "*.cpp;*.cc;*.cxx;*.rc";
		AddSourcePattern(m_cszSourcePattern);
	}

	return true;
}

void CSrcFiles::ProcessOption(char* pszLine)
{
	char* pszVal = strpbrk(pszLine, ":=");
	ASSERT_COMMENT(pszVal, "Invalid Option -- missing ':' or '=' character");
	if (!pszVal)
		return;
	if (*pszVal == '=')
		*pszVal = ':';		// convert old seperator to YAML seperator
	pszVal = FindNonSpace(pszVal + 1);

	// remove any comment

	char* pszTmp = kstrchr(pszVal, '#');
	if (pszTmp)
		*pszTmp = 0;

	trim(pszVal);	// remove any trailing whitespace

	if (IsSameSubString(pszLine, "IDE:")) {
		CEnumStr cenum(pszVal, ' ');
		m_IDE = 0;
		while (cenum.Enum()) {
			if (isSameString(cenum, "CodeBlocks"))
				m_IDE |= IDE_CODEBLOCK;
			else if (isSameString(cenum, "CodeLite"))
				m_IDE |= IDE_CODELITE;
			else if (isSameString(cenum, "VisualStudio"))
				m_IDE |= IDE_VS;
		}
		return;
	}

	if (IsSameSubString(pszLine, "Compilers:")) {
		CEnumStr cenum(pszVal, ' ');
		m_CompilerType = 0;
		while (cenum.Enum()) {
			if (isSameString(cenum, "CLANG"))
				m_CompilerType |= COMPILER_CLANG;
			else if (isSameString(cenum, "MSVC"))
				m_CompilerType |= COMPILER_MSVC;
		}
		return;
	}

	if (IsSameSubString(pszLine, "Makefile:")) {
		m_fCreateMakefile = MAKEMAKE_DEFAULT;
		if (isSameSubString(pszVal, "never"))
			m_fCreateMakefile = MAKEMAKE_NEVER;
		else if (isSameSubString(pszVal, "missing"))
			m_fCreateMakefile = MAKEMAKE_MISSING;
		else if (isSameSubString(pszVal, "always"))
			m_fCreateMakefile = MAKEMAKE_ALWAYS;
		return;
	}

	if (IsSameSubString(pszLine, "Project:")) {
		m_cszProjectName = pszVal;
		return;
	}

	if (IsSameSubString(pszLine, "TargetDirs:")) {
		CEnumStr cenum(pszVal, ';');
		if (cenum.Enum())
			m_cszTarget32 = cenum;
		if (cenum.Enum())
			m_cszTarget64 = cenum;
		return;
	}

	if (IsSameSubString(pszLine, "exe_type:") || IsSameSubString(pszLine, "EXE:")) {
		if (IsSameSubString(pszVal, "window"))
			m_exeType = EXE_WINDOW;
		else if (IsSameSubString(pszVal, "console"))
			m_exeType = EXE_CONSOLE;
		else if (IsSameSubString(pszVal, "lib"))
			m_exeType = EXE_LIB;
		else if (IsSameSubString(pszVal, "dll"))
			m_exeType = EXE_DLL;
		return;
	}

	if (IsSameSubString(pszLine, "optimize:")) {
		m_bBuildForSpeed = IsSameSubString(pszVal, "speed");
		return;
	}

	if (IsSameSubString(pszLine, "PCH:")) {
		if (*pszVal) {
			if (IsSameString(pszVal, "none")) {
				m_cszPCHheader.Delete();
			}
			else if (!IsSameString(m_cszPCHheader, pszVal)) {
				m_cszPCHheader = pszVal;
			}
		}
		return;
	}

	if (IsSameSubString(pszLine, "Sources:") && *pszVal) {
		m_cszSourcePattern = pszVal;
		return;
	}

	if (IsSameSubString(pszLine, "LibPCH:")) {		// REVIEW: [randalphwa - 10/25/2018] currently, MakeNinja doesn't use this
		if (*pszVal) {
			if (IsSameString(pszVal, "none")) {
				m_cszLibPCHheader.Delete();
			}
			else if (!IsSameString(m_cszLibPCHheader, pszVal)) {
				m_cszLibPCHheader = pszVal;
			}
		}
		return;
	}

	// Note that .private/.srcfiles can override these -- which could mean using a debug library instead of a release library, or a
	// beta header file, -DBETA flag, stricter warning level, etc.

	if (IsSameSubString(pszLine, "Libs:")) {
		if (pszVal)
			m_cszLibs = FindNonSpace(pszVal);
		return;
	}

	if (IsSameSubString(pszLine, "BuildLibs:")) {
		if (pszVal)
			m_cszBuildLibs = FindNonSpace(pszVal);
		return;
	}

	if (IsSameSubString(pszLine, "IncDirs:")) {
		if (pszVal)
			m_cszIncDirs = FindNonSpace(pszVal);
		return;
	}

	if (IsSameSubString(pszLine, "LibDirs:")) {
		if (pszVal)
			m_cszLibDirs = FindNonSpace(pszVal);
		return;
	}

	if (IsSameSubString(pszLine, "CFlags:")) {
		m_cszCFlags = pszVal;
		return;
	}

	if (IsSameSubString(pszLine, "MidlFlags:")) {
		m_cszMidlFlags = pszVal;
		return;
	}

	if (IsSameSubString(pszLine, "LinkFlags:") || IsSameSubString(pszLine, "LINK:")) {
		m_cszLinkFlags = pszVal;
		return;
	}

	if (IsSameSubString(pszLine, "WarnLevel:")) {
		m_WarningLevel = Atoi(pszVal);
		return;
	}

	bool bYesNo = IsSameSubString(pszVal, "true") || IsSameSubString(pszVal, "YES");

	if (IsSameSubString(pszLine, "64Bit:"))
		m_b64bit = bYesNo;
	else if (IsSameSubString(pszLine, "bit_suffix:"))
		m_bBitSuffix = bYesNo;
	else if (IsSameSubString(pszLine, "permissive:"))
		m_bPermissive = bYesNo;
	else if (IsSameSubString(pszLine, "stdcall:"))
		m_bStdcall = bYesNo;
	else if (IsSameSubString(pszLine, "static_crt:"))
		m_bStaticCrt = bYesNo;
	else if (IsSameSubString(pszLine, "ms_linker:"))
		m_bUseMsvcLinker = bYesNo;
}

void CSrcFiles::AddCompilerFlag(const char* pszFlag)
{
	if (m_cszCFlags.IsEmpty() || !kstristr(m_cszCFlags, pszFlag))	{
		m_cszCFlags += pszFlag;
		m_cszCFlags += " ";
	}
}

void CSrcFiles::AddLibrary(const char* pszName)
{
	if (m_cszLibs.IsEmpty() || !kstristr(m_cszLibs, pszName)) {
		m_cszLibs += pszName;
		m_cszLibs += " ";
	}
}

void CSrcFiles::ProcessLibSection(char* pszLibFile)
{
	// The library is built in the $libout directory, so we don't need to worry about a name conflict -- hence the default name of
	// "tmplib". The name is also to indicate that this is a temporary library -- it's not designed to be linked to outside of the
	// scope of the current project.

	if (m_cszLibName.IsEmpty())
		m_cszLibName = "tmplib";

	if (kstristr(pszLibFile, ".lib"))	// this was used before we created a default name
		return;
	else if (IsSameSubString(pszLibFile, ".include")) {
		char* pszIncFile = FindNonSpace(FindNextSpace(pszLibFile));
		ProcessInclude(pszIncFile, m_lstLibAddSrcFiles, false);
	}
	else {
		m_lstLibFiles += pszLibFile;
		if (!FileExists(pszLibFile)) {
			CStr cszErrMsg;
			cszErrMsg.printf("Cannot locate %s", (char*) pszLibFile);
			m_lstErrors += cszErrMsg;
		}
	}
}

void CSrcFiles::ProcessFile(char* pszFile)
{
	if (IsSameSubString(pszFile, ".include")) {
		char* pszIncFile = FindNonSpace(FindNextSpace(pszFile));
		ProcessInclude(pszIncFile, m_lstAddSrcFiles, true);
		return;
	}

	m_lstSrcFiles += pszFile;
	if (!FileExists(pszFile)) {
		CStr cszErrMsg;
		cszErrMsg.printf("Cannot locate %s", (char*) pszFile);
		m_lstErrors += cszErrMsg;
	}

	char* pszExt = kstristr(pszFile, ".idl");
	if (pszExt)	{
		m_lstIdlFiles += pszFile;
		return;
	}

	pszExt = kstristr(pszFile, ".rc");
	if (pszExt && !pszExt[3]) {	// ignore .rc2, .resources, etc.
		m_cszRcName = pszFile;
		return;
	}

	pszExt = kstristr(pszFile, ".hhp");
	if (pszExt) {	// ignore .rc2, .resources, etc.
		m_cszHHPName = pszFile;
		return;
	}
}

void CSrcFiles::ProcessInclude(const char* pszFile, CStrIntList& lstAddSrcFiles, bool bFileSection)
{
	if (IsSameSubString(pszFile, ".include")) {
		char* pszIncFile = FindNonSpace(FindNextSpace(pszFile));
		ProcessInclude(pszIncFile, m_lstLibAddSrcFiles, false);
		return;
	}

	CSrcFiles cIncSrcFiles;
	if (!cIncSrcFiles.ReadFile(pszFile)) {
		CStr cszMsg;
		cszMsg.printf("Cannot open %s!", pszFile);
		m_lstErrors += cszMsg;
		return;
	}

	CStr cszCWD;
	cszCWD.GetCWD();

	CStr cszFullPath(pszFile);
	cszFullPath.GetFullPathName();

	CTMem<char*> szPath(1024);
	kstrcpy(szPath, 1024, cszFullPath);
	char* pszFilePortion = FindFilePortion(szPath);

	CStr cszRelative;

	for (size_t pos = 0; pos < (bFileSection ? cIncSrcFiles.m_lstSrcFiles.GetCount() : cIncSrcFiles.m_lstLibFiles.GetCount()); ++pos) {
		kstrcpy(pszFilePortion, bFileSection ? cIncSrcFiles.m_lstSrcFiles[pos] : cIncSrcFiles.m_lstLibFiles[pos]);
		ConvertToRelative(cszCWD, szPath, cszRelative);
		size_t posAdd;
		posAdd = m_lstSrcIncluded.Add(cszRelative);
		lstAddSrcFiles.Add(pszFile, posAdd);
		if (bFileSection)
			m_lstSrcFiles += cszRelative;
		else
			m_lstLibFiles += cszRelative;
	}
}

void CSrcFiles::AddSourcePattern(const char* pszFilePattern)
{
	if (!pszFilePattern || !*pszFilePattern)
		return;

	CStr cszPattern(pszFilePattern);
	CEnumStr enumstr(cszPattern, ';');
	const char* pszPattern;

	while (enumstr.Enum(&pszPattern)) {
		CTTFindFile ff(pszPattern);
		while (ff.isValid()) {
			char* psz = kstrchrR(ff, '.');
			if (psz) {
				if (
						IsSameString(psz, ".c") ||
						IsSameString(psz, ".cpp") ||
						IsSameString(psz, ".cc") ||
						IsSameString(psz, ".cxx") ||
						IsSameString(psz, ".hhp") ||
						IsSameString(psz, ".idl") ||
						IsSameString(psz, ".rc")
					) {
						m_lstSrcFiles.Add(ff);
				}
			}
			if (!ff.NextFile())
				break;
		}
	}
}

// It's fine if pszPrivate is NULL or the file doesn't exists -- only failure condition is if pszMaster can't be read

bool CSrcFiles::ReadTwoFiles(const char* pszMaster, const char* pszPrivate)
{
	if (!ReadFile(pszMaster))
		return false;
	m_bReadingPrivate = true;	// just in case some processing method needs to know

	if (pszPrivate && FileExists(pszPrivate))
		ReadFile(pszPrivate);

	// The private file is only used to overwrite options in the master file, so if reading it fails either because it
	// doesn't exist, or some other reason, we can still return true since we onlyt got here if the master file was
	// successfully read.

	return true;
}