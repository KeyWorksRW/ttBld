/////////////////////////////////////////////////////////////////////////////
// Name:		CSrcFiles
// Purpose:		Class for reading/writing .srcfiles (master file used by makemake.exe to generate build scripts)
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <stdio.h>	// for sprintf

#include "../ttLib/include/findfile.h"	// ttFindFile
#include "../ttLib/include/enumstr.h"	// ttEnumStr
#include "../ttLib/include/ttmem.h" 	// ttMem

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

	m_lstSrcFiles.SetFlags(ttList::FLG_URL_STRINGS);
	m_lstLibFiles.SetFlags(ttList::FLG_URL_STRINGS);
	m_lstErrors.SetFlags(ttList::FLG_IGNORE_CASE);
}

bool CSrcFiles::ReadFile(const char* pszFile)
{
	ttFile kfSrcFiles;
	if (!kfSrcFiles.ReadFile(pszFile))
		return false;
	m_bRead = true;

	char* pszLine;
	SRC_SECTION section = SECTION_UNKNOWN;

	// kfSrcFiles.PrepForReadLine();
	while (kfSrcFiles.readline(&pszLine)) {
		char* pszBegin = tt::nextnonspace(pszLine);	// ignore any leading spaces
		if (tt::isempty(pszBegin) || pszBegin[0] == '#' || (pszBegin[0] == '-' && pszBegin[1] == '-' && pszBegin[2] == '-')) {	// ignore empty, comment or divider lines
			continue;
		}
		char* pszComment = tt::findchr(pszBegin, '#');	// strip off any comments
		if (pszComment) {
			*pszComment = 0;
			tt::trim_right(pszBegin);		// remove any trailing white space
		}

		if (tt::samesubstri(pszBegin, "%YAML")) {	// not required, but possible a YAML editor could add this
			continue;
		}

		if (tt::isalpha(*pszLine)) { 	// sections always begin with an alphabetical character
			if (tt::samesubstri(pszBegin, "Files:") || tt::samesubstri(pszBegin, "[FILES]")) {
				section = SECTION_FILES;
			}
			else if (tt::samesubstri(pszBegin, "Options:") || tt::samesubstri(pszBegin, "[OPTIONS]")) {
				section = SECTION_OPTIONS;
			}
			else if (tt::samesubstri(pszBegin, "Lib:")) {
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
		ttString cszProj;
		cszProj.GetCWD();
		char* pszProj = tt::FindFilePortion(cszProj);
		if (pszProj && !tt::samestri(pszProj, "src"))
			m_cszProjectName = pszProj;
		else {
			if (pszProj > cszProj.getptr())
				--pszProj;
			*pszProj = 0;
			pszProj = tt::FindFilePortion(cszProj);
			if (pszProj)
				m_cszProjectName = pszProj;
		}
	}

	if (m_exeType == EXE_UNSPECIFIED)
		m_exeType= EXE_WINDOW;

	AddSourcePattern(m_cszSrcPattern);

	// If no Files: or Sources: we're specified, then we still won't have any files to build. Default to every type of C++
	// source file in the current directory.

	if (m_lstSrcFiles.GetCount() < 1) {
		m_cszSrcPattern = "*.cpp;*.cc;*.cxx;*.rc";
		AddSourcePattern(m_cszSrcPattern);
	}

	return true;
}

void CSrcFiles::ProcessOption(char* pszLine)
{
	char* pszVal = strpbrk(pszLine, ":=");
	ttASSERT_MSG(pszVal, "Invalid Option -- missing ':' or '=' character");
	if (!pszVal)
		return;
	if (*pszVal == '=')
		*pszVal = ':';		// convert old seperator to YAML seperator
	pszVal = tt::nextnonspace(pszVal + 1);

	// remove any comment

	char* pszTmp = tt::findchr(pszVal, '#');
	if (pszTmp)
		*pszTmp = 0;

	tt::trim_right(pszVal);	// remove any trailing whitespace

	if (tt::samesubstri(pszLine, "IDE:")) {
		ttEnumStr cenum(pszVal, ' ');
		m_IDE = 0;
		while (cenum.Enum()) {
			if (tt::samestri(cenum, "CodeBlocks"))
				m_IDE |= IDE_CODEBLOCK;
			else if (tt::samestri(cenum, "CodeLite"))
				m_IDE |= IDE_CODELITE;
			else if (tt::samestri(cenum, "VisualStudio"))
				m_IDE |= IDE_VS;
		}
		return;
	}

	if (tt::samesubstri(pszLine, "Compilers:")) {
		ttEnumStr cenum(pszVal, ' ');
		m_CompilerType = 0;
		while (cenum.Enum()) {
			if (tt::samestri(cenum, "CLANG"))
				m_CompilerType |= COMPILER_CLANG;
			else if (tt::samestri(cenum, "MSVC"))
				m_CompilerType |= COMPILER_MSVC;
		}
		return;
	}

	if (tt::samesubstri(pszLine, "Makefile:")) {
		m_fCreateMakefile = MAKEMAKE_DEFAULT;
		if (tt::samesubstri(pszVal, "never"))
			m_fCreateMakefile = MAKEMAKE_NEVER;
		else if (tt::samesubstri(pszVal, "missing"))
			m_fCreateMakefile = MAKEMAKE_MISSING;
		else if (tt::samesubstri(pszVal, "always"))
			m_fCreateMakefile = MAKEMAKE_ALWAYS;
		return;
	}

	if (tt::samesubstri(pszLine, "Project:")) {
		m_cszProjectName = pszVal;
		return;
	}

	if (tt::samesubstri(pszLine, "TargetDirs:")) {
		ttEnumStr cenum(pszVal, ';');
		if (cenum.Enum())
			m_cszTarget32 = cenum;
		if (cenum.Enum())
			m_cszTarget64 = cenum;
		return;
	}

	if (tt::samesubstri(pszLine, "exe_type:") || tt::samesubstri(pszLine, "EXE:")) {
		if (tt::samesubstri(pszVal, "window"))
			m_exeType = EXE_WINDOW;
		else if (tt::samesubstri(pszVal, "console"))
			m_exeType = EXE_CONSOLE;
		else if (tt::samesubstri(pszVal, "lib"))
			m_exeType = EXE_LIB;
		else if (tt::samesubstri(pszVal, "dll"))
			m_exeType = EXE_DLL;
		return;
	}

	if (tt::samesubstri(pszLine, "optimize:")) {
		m_bBuildForSpeed = tt::samesubstri(pszVal, "speed");
		return;
	}

	if (tt::samesubstri(pszLine, "PCH:")) {
		if (*pszVal) {
			if (tt::samestri(pszVal, "none")) {
				m_cszPCHheader.Delete();
			}
			else if (!tt::samestri(m_cszPCHheader, pszVal)) {
				m_cszPCHheader = pszVal;
			}
		}
		return;
	}

	if (tt::samesubstri(pszLine, "Sources:") && *pszVal) {
		m_cszSrcPattern = pszVal;
		return;
	}

	if (tt::samesubstri(pszLine, "LibPCH:")) {		// REVIEW: [randalphwa - 10/25/2018] currently, MakeNinja doesn't use this
		if (*pszVal) {
			if (tt::samestri(pszVal, "none")) {
				m_cszLibPCHheader.Delete();
			}
			else if (!tt::samestri(m_cszLibPCHheader, pszVal)) {
				m_cszLibPCHheader = pszVal;
			}
		}
		return;
	}

	// Note that .private/.srcfiles can override these -- which could mean using a debug library instead of a release library, or a
	// beta header file, -DBETA flag, stricter warning level, etc.

	if (tt::samesubstri(pszLine, "Libs:")) {
		if (pszVal)
			m_cszLibs = tt::nextnonspace(pszVal);
		return;
	}

	if (tt::samesubstri(pszLine, "BuildLibs:")) {
		if (pszVal)
			m_cszBuildLibs = tt::nextnonspace(pszVal);
		return;
	}

	if (tt::samesubstri(pszLine, "IncDirs:")) {
		if (pszVal)
			m_cszIncDirs = tt::nextnonspace(pszVal);
		return;
	}

	if (tt::samesubstri(pszLine, "LibDirs:")) {
		if (pszVal)
			m_cszLibDirs = tt::nextnonspace(pszVal);
		return;
	}

	if (tt::samesubstri(pszLine, "CFlags:")) {
		m_cszCFlags = pszVal;
		return;
	}

	if (tt::samesubstri(pszLine, "MidlFlags:")) {
		m_cszMidlFlags = pszVal;
		return;
	}

	if (tt::samesubstri(pszLine, "LinkFlags:") || tt::samesubstri(pszLine, "LINK:")) {
		m_cszLinkFlags = pszVal;
		return;
	}

	if (tt::samesubstri(pszLine, "WarnLevel:")) {
		m_WarningLevel = tt::atoi(pszVal);
		return;
	}

	bool bYesNo = tt::samesubstri(pszVal, "true") || tt::samesubstri(pszVal, "YES");

	if (tt::samesubstri(pszLine, "64Bit:"))
		m_b64bit = bYesNo;
	else if (tt::samesubstri(pszLine, "bit_suffix:"))
		m_bBitSuffix = bYesNo;
	else if (tt::samesubstri(pszLine, "permissive:"))
		m_bPermissive = bYesNo;
	else if (tt::samesubstri(pszLine, "stdcall:"))
		m_bStdcall = bYesNo;
	else if (tt::samesubstri(pszLine, "static_crt:"))
		m_bStaticCrt = bYesNo;
	else if (tt::samesubstri(pszLine, "ms_linker:"))
		m_bUseMsvcLinker = bYesNo;
}

void CSrcFiles::AddCompilerFlag(const char* pszFlag)
{
	if (m_cszCFlags.IsEmpty() || !tt::findstri(m_cszCFlags, pszFlag))	{
		m_cszCFlags += pszFlag;
		m_cszCFlags += " ";
	}
}

void CSrcFiles::AddLibrary(const char* pszName)
{
	if (m_cszLibs.IsEmpty() || !tt::findstri(m_cszLibs, pszName)) {
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

	if (tt::findstri(pszLibFile, ".lib"))	// this was used before we created a default name
		return;
	else if (tt::samesubstri(pszLibFile, ".include")) {
		char* pszIncFile = tt::nextnonspace(tt::nextspace(pszLibFile));
		ProcessInclude(pszIncFile, m_lstLibAddSrcFiles, false);
	}
	else {
		m_lstLibFiles += pszLibFile;
		if (!tt::FileExists(pszLibFile)) {
			ttString cszErrMsg;
			cszErrMsg.printf("Cannot locate %s", (char*) pszLibFile);
			m_lstErrors += cszErrMsg;
		}
	}
}

void CSrcFiles::ProcessFile(char* pszFile)
{
	if (tt::samesubstri(pszFile, ".include")) {
		const char* pszIncFile = tt::nextnonspace(tt::nextspace(pszFile));
		ProcessInclude(pszIncFile, m_lstAddSrcFiles, true);
		return;
	}

	m_lstSrcFiles += pszFile;
	if (!tt::FileExists(pszFile)) {
		ttString cszErrMsg;
		cszErrMsg.printf("Cannot locate %s", (char*) pszFile);
		m_lstErrors += cszErrMsg;
	}

	char* pszExt = tt::findstri(pszFile, ".idl");
	if (pszExt)	{
		m_lstIdlFiles += pszFile;
		return;
	}

	pszExt = tt::findstri(pszFile, ".rc");
	if (pszExt && !pszExt[3]) {	// ignore .rc2, .resources, etc.
		m_cszRcName = pszFile;
		return;
	}

	pszExt = tt::findstri(pszFile, ".hhp");
	if (pszExt) {	// ignore .rc2, .resources, etc.
		m_cszHHPName = pszFile;
		return;
	}
}

void CSrcFiles::ProcessInclude(const char* pszFile, ttStrIntList& lstAddSrcFiles, bool bFileSection)
{
	if (tt::samesubstri(pszFile, ".include")) {
		const char* pszIncFile = tt::nextnonspace(tt::nextspace(pszFile));
		ProcessInclude(pszIncFile, m_lstLibAddSrcFiles, false);
		return;
	}

	CSrcFiles cIncSrcFiles;
	if (!cIncSrcFiles.ReadFile(pszFile)) {
		ttString cszMsg;
		cszMsg.printf("Cannot open %s!", pszFile);
		m_lstErrors += cszMsg;
		return;
	}

	ttString cszCWD;
	cszCWD.GetCWD();

	ttString cszFullPath(pszFile);
	cszFullPath.GetFullPathName();

	ttTMem<char*> szPath(1024);
	tt::strcpy_s(szPath, 1024, cszFullPath);
	char* pszFilePortion = tt::FindFilePortion(szPath);

	ttString cszRelative;

	for (size_t pos = 0; pos < (bFileSection ? cIncSrcFiles.m_lstSrcFiles.GetCount() : cIncSrcFiles.m_lstLibFiles.GetCount()); ++pos) {
		tt::strcpy(pszFilePortion, bFileSection ? cIncSrcFiles.m_lstSrcFiles[pos] : cIncSrcFiles.m_lstLibFiles[pos]);
		tt::ConvertToRelative(cszCWD, szPath, cszRelative);
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

	ttString cszPattern(pszFilePattern);
	ttEnumStr enumstr(cszPattern, ';');
	const char* pszPattern;

	while (enumstr.Enum(&pszPattern)) {
		ttFindFile ff(pszPattern);
		while (ff.isValid()) {
			char* psz = tt::strchrR(ff, '.');
			if (psz) {
				if (
						tt::samestri(psz, ".c") ||
						tt::samestri(psz, ".cpp") ||
						tt::samestri(psz, ".cc") ||
						tt::samestri(psz, ".cxx") ||
						tt::samestri(psz, ".hhp") ||
						tt::samestri(psz, ".idl") ||
						tt::samestri(psz, ".rc")
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

	if (pszPrivate && tt::FileExists(pszPrivate))
		ReadFile(pszPrivate);

	// The private file is only used to overwrite options in the master file, so if reading it fails either because it
	// doesn't exist, or some other reason, we can still return true since we onlyt got here if the master file was
	// successfully read.

	return true;
}