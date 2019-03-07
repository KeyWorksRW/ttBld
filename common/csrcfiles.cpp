/////////////////////////////////////////////////////////////////////////////
// Name:		CSrcFiles
// Purpose:		Class for reading/writing .srcfiles (master file used by makemake.exe to generate build scripts)
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <stdio.h>	// for sprintf

#include <ttfindfile.h> 	// ttCFindFile
#include <ttenumstr.h>		// ttCEnumStr
#include <ttmem.h>			// ttCMem, ttCTMem

#include "csrcfiles.h"		// CSrcFiles
#include "strtable.h"		// String resource IDs

const char* txtSrcFilesFileName = ".srcfiles";

typedef enum {
	SECTION_UNKNOWN,
	SECTION_OPTIONS,
	SECTION_FILES,
	SECTION_LIB,
} SRC_SECTION;

CSrcFiles::CSrcFiles() : m_ttHeap(true),
	// make all ttCList classes use the same sub-heap
	m_lstSrcFiles(m_ttHeap), m_lstLibFiles(m_ttHeap), m_lstIdlFiles(m_ttHeap), m_lstDepLibs(m_ttHeap),
	m_lstErrors(m_ttHeap), m_lstLibAddSrcFiles(m_ttHeap), m_lstSrcIncluded(m_ttHeap)
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

	m_lstSrcFiles.SetFlags(ttCList::FLG_URL_STRINGS);
	m_lstLibFiles.SetFlags(ttCList::FLG_URL_STRINGS);
	m_lstErrors.SetFlags(ttCList::FLG_IGNORE_CASE);

	AddOption(OPT_PROJECT, "Project", true);

	// If the option value is a simple string or true/false value, it can be added here and it will automatically be read
	// and written.

	AddOptVal("Project",   &m_cszProjectName, "project target name");
	AddOptVal("PCH",       &m_cszPCHheader);

	AddOptVal("CFlags",    &m_cszCFlags);
	AddOptVal("MidlFlags", &m_cszMidlFlags);
	AddOptVal("LinkFlags", &m_cszLinkFlags);
	AddOptVal("RCFlags",   &m_cszRCFlags);

	AddOptVal("IncDirs",   &m_cszIncDirs);
	AddOptVal("LibPCH",    &m_cszLibPCHheader);
	AddOptVal("Libs",      &m_cszLibs);
	AddOptVal("LibDirs",   &m_cszLibDirs);
	AddOptVal("Sources",   &m_cszSrcPattern);

	AddOptVal("64Bit",      &m_b64bit);
	AddOptVal("bit_suffix", &m_bBitSuffix);
	AddOptVal("DebugRC",    &m_bDebugRC);
	AddOptVal("permissive", &m_bPermissive);
	AddOptVal("stdcall",    &m_bStdcall);
	AddOptVal("static_crt", &m_bStaticCrt);
	AddOptVal("ms_linker",  &m_bUseMsvcLinker);
}

CSrcFiles::~CSrcFiles()
{
	for (size_t pos = 0; pos < m_aOptVal.GetCount(); ++pos) {
		if (m_aOptVal[pos].pszComment)
			tt::FreeAlloc(m_aOptVal[pos].pszComment);
	}
}

bool CSrcFiles::ReadFile(const char* pszFile)
{
	ttCFile kfSrcFiles;
	if (!kfSrcFiles.ReadFile(pszFile))
		return false;
	m_bRead = true;

	char* pszLine;
	SRC_SECTION section = SECTION_UNKNOWN;

	// kfSrcFiles.PrepForReadLine();
	while (kfSrcFiles.ReadLine(&pszLine)) {
		char* pszBegin = tt::findNonSpace(pszLine);	// ignore any leading spaces
		if (tt::isEmpty(pszBegin) || pszBegin[0] == '#' || (pszBegin[0] == '-' && pszBegin[1] == '-' && pszBegin[2] == '-')) {	// ignore empty, comment or divider lines
			continue;
		}

		if (tt::isSameSubStri(pszBegin, "%YAML")) {	// not required, but possible a YAML editor could add this
			continue;
		}

		if (tt::isAlpha(*pszLine)) { 	// sections always begin with an alphabetical character
			if (tt::isSameSubStri(pszBegin, "Files:") || tt::isSameSubStri(pszBegin, "[FILES]")) {
				section = SECTION_FILES;
			}
			else if (tt::isSameSubStri(pszBegin, "Options:") || tt::isSameSubStri(pszBegin, "[OPTIONS]")) {
				section = SECTION_OPTIONS;
			}
			else if (tt::isSameSubStri(pszBegin, "Lib:")) {
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

	if (m_cszProjectName.isEmpty())	{
		ttCStr cszProj;
		cszProj.getCWD();
		char* pszProj = tt::findFilePortion(cszProj);
		if (pszProj && !tt::isSameStri(pszProj, "src"))
			m_cszProjectName = pszProj;
		else {
			if (pszProj > cszProj.getPtr())
				--pszProj;
			*pszProj = 0;
			pszProj = tt::findFilePortion(cszProj);
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
	ttCStr cszName, cszVal, cszComment;

	if (!GetOptionParts(pszLine, cszName, cszVal, cszComment))
		return;

	if (UpdateOptVal(cszName, (const char*) cszVal, cszComment))	// process all options that have a string value
		return;

	bool bYesNo = tt::isSameStri(cszVal, "true") || tt::isSameStri(cszVal, "yes");
	if (UpdateOptVal(cszName, bYesNo, cszComment))	// process all options that have a boolean value
		return;

	// Everything below requires special handling

	if (tt::isSameStri(cszName, "IDE")) {
		ttCEnumStr cenum(cszVal, ' ');
		m_IDE = 0;
		while (cenum.Enum()) {
			if (tt::isSameStri(cenum, "CodeBlocks"))
				m_IDE |= IDE_CODEBLOCK;
			else if (tt::isSameStri(cenum, "CodeLite"))
				m_IDE |= IDE_CODELITE;
			else if (tt::isSameStri(cenum, "VisualStudio"))
				m_IDE |= IDE_VS;
		}
		return;
	}

	if (tt::isSameStri(cszName, "Compilers")) {
		ttCEnumStr cenum(cszVal, ' ');
		m_CompilerType = 0;
		while (cenum.Enum()) {
			if (tt::isSameStri(cenum, "CLANG"))
				m_CompilerType |= COMPILER_CLANG;
			else if (tt::isSameStri(cenum, "MSVC"))
				m_CompilerType |= COMPILER_MSVC;
		}
		return;
	}

	if (tt::isSameStri(cszName, "Makefile")) {
		m_fCreateMakefile = MAKEMAKE_DEFAULT;
		if (tt::isSameSubStri(cszVal, "never"))
			m_fCreateMakefile = MAKEMAKE_NEVER;
		else if (tt::isSameSubStri(cszVal, "missing"))
			m_fCreateMakefile = MAKEMAKE_MISSING;
		else if (tt::isSameSubStri(cszVal, "always"))
			m_fCreateMakefile = MAKEMAKE_ALWAYS;
		return;
	}

	if (tt::isSameStri(cszName, "TargetDirs")) {	// first target is 32-bit directory, second is 64-bit directory
		ttCEnumStr cenum(cszVal, ';');
		if (cenum.Enum())
			m_cszTarget32 = cenum;
		if (cenum.Enum())
			m_cszTarget64 = cenum;
		return;
	}

	if (tt::isSameStri(cszName, "exe_type")) {
		if (tt::isSameSubStri(cszVal, "window"))
			m_exeType = EXE_WINDOW;
		else if (tt::isSameSubStri(cszVal, "console"))
			m_exeType = EXE_CONSOLE;
		else if (tt::isSameSubStri(cszVal, "lib"))
			m_exeType = EXE_LIB;
		else if (tt::isSameSubStri(cszVal, "dll"))
			m_exeType = EXE_DLL;
		return;
	}

	if (tt::isSameStri(cszName, "optimize")) {
		m_bBuildForSpeed = tt::isSameSubStri(cszVal, "speed");
		return;
	}

	if (tt::isSameStri(cszName, "BuildLibs")) {
		if (cszVal.isNonEmpty()) {
			m_cszBuildLibs = tt::findNonSpace(cszVal);
			// Remove any .lib extension--we are looking for the target name, not the library name
			char* pszLibExt = tt::findStri(m_cszBuildLibs, ".lib");
			while (pszLibExt) {
				tt::strCopy(pszLibExt, pszLibExt + 4);
				pszLibExt = tt::findStri(pszLibExt, ".lib");
			}
		}
		return;
	}

	if (tt::isSameStri(cszName, "WarnLevel")) {
		m_WarningLevel = tt::Atoi(cszVal);
		return;
	}

	ttCStr csz;
	csz.printf(tt::getResString(IDS_UNKNOWN_OPTION), (char*) cszName);
	m_lstErrors += csz;
}

void CSrcFiles::AddCompilerFlag(const char* pszFlag)
{
	if (m_cszCFlags.isEmpty() || !tt::findStri(m_cszCFlags, pszFlag))	{
		m_cszCFlags += pszFlag;
		m_cszCFlags += " ";
	}
}

void CSrcFiles::AddLibrary(const char* pszName)
{
	if (m_cszLibs.isEmpty() || !tt::findStri(m_cszLibs, pszName)) {
		m_cszLibs += pszName;
		m_cszLibs += " ";
	}
}

void CSrcFiles::ProcessLibSection(char* pszLibFile)
{
	// The library is built in the $libout directory, so we don't need to worry about a name conflict -- hence the default name of
	// "tmplib". The name is also to indicate that this is a temporary library -- it's not designed to be linked to outside of the
	// scope of the current project.

	if (m_cszLibName.isEmpty())
		m_cszLibName = "tmplib";

	if (tt::findStri(pszLibFile, ".lib"))	// this was used before we created a default name
		return;
	else if (tt::isSameSubStri(pszLibFile, ".include")) {
		char* pszIncFile = tt::findNonSpace(tt::findSpace(pszLibFile));
		ProcessInclude(pszIncFile, m_lstLibAddSrcFiles, false);
	}
	else {
		m_lstLibFiles += pszLibFile;
		if (!tt::FileExists(pszLibFile)) {
			ttCStr cszErrMsg;
			cszErrMsg.printf("Cannot locate %s", (char*) pszLibFile);
			m_lstErrors += cszErrMsg;
		}
	}
}

void CSrcFiles::ProcessFile(char* pszFile)
{
	if (tt::isSameSubStri(pszFile, ".include")) {
		const char* pszIncFile = tt::findNonSpace(tt::findSpace(pszFile));
		ProcessInclude(pszIncFile, m_lstAddSrcFiles, true);
		return;
	}

	m_lstSrcFiles += pszFile;
	if (!tt::FileExists(pszFile)) {
		ttCStr cszErrMsg;
		cszErrMsg.printf("Cannot locate %s", (char*) pszFile);
		m_lstErrors += cszErrMsg;
	}

	char* pszExt = tt::findStri(pszFile, ".idl");
	if (pszExt)	{
		m_lstIdlFiles += pszFile;
		return;
	}

	pszExt = tt::findStri(pszFile, ".rc");
	if (pszExt && !pszExt[3]) {	// ignore .rc2, .resources, etc.
		m_cszRcName = pszFile;
		return;
	}

	pszExt = tt::findStri(pszFile, ".hhp");
	if (pszExt) {	// ignore .rc2, .resources, etc.
		m_cszHHPName = pszFile;
		return;
	}
}

void CSrcFiles::ProcessInclude(const char* pszFile, ttCStrIntList& lstAddSrcFiles, bool bFileSection)
{
	if (tt::isSameSubStri(pszFile, ".include")) {
		const char* pszIncFile = tt::findNonSpace(tt::findSpace(pszFile));
		ProcessInclude(pszIncFile, m_lstLibAddSrcFiles, false);
		return;
	}

	CSrcFiles cIncSrcFiles;
	if (!cIncSrcFiles.ReadFile(pszFile)) {
		ttCStr cszMsg;
		cszMsg.printf("Cannot open %s!", pszFile);
		m_lstErrors += cszMsg;
		return;
	}

	ttCStr cszCWD;
	cszCWD.getCWD();

	ttCStr cszFullPath(pszFile);
	cszFullPath.getFullPathName();

	ttCTMem<char*> szPath(1024);
	tt::strCopy_s(szPath, 1024, cszFullPath);
	char* pszFilePortion = tt::findFilePortion(szPath);

	ttCStr cszRelative;

	for (size_t pos = 0; pos < (bFileSection ? cIncSrcFiles.m_lstSrcFiles.GetCount() : cIncSrcFiles.m_lstLibFiles.GetCount()); ++pos) {
		tt::strCopy(pszFilePortion, bFileSection ? cIncSrcFiles.m_lstSrcFiles[pos] : cIncSrcFiles.m_lstLibFiles[pos]);
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

	ttCStr cszPattern(pszFilePattern);
	ttCEnumStr enumstr(cszPattern, ';');
	const char* pszPattern;

	while (enumstr.Enum(&pszPattern)) {
		ttCFindFile ff(pszPattern);
		while (ff.isValid()) {
			char* psz = tt::findLastChar(ff, '.');
			if (psz) {
				if (
						tt::isSameStri(psz, ".c") ||
						tt::isSameStri(psz, ".cpp") ||
						tt::isSameStri(psz, ".cc") ||
						tt::isSameStri(psz, ".cxx")
					) {
						m_lstSrcFiles += ff;

				}
				else if (tt::isSameStri(psz, ".rc")) {
					m_lstSrcFiles += ff;
					if (m_cszRcName.isEmpty())
						m_cszRcName = ff;
				}
				else if (tt::isSameStri(psz, ".hhp")) {
					m_lstSrcFiles += ff;
					if (m_cszHHPName.isEmpty())
						m_cszHHPName = ff;
				}
				else if (tt::isSameStri(psz, ".idl")) {
					m_lstSrcFiles += ff;
					m_lstIdlFiles += ff;
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

void CSrcFiles::AddOptVal(const char* pszName, bool* pbVal, const char* pszComment)
{
	OPT_BOOL optBool;
	optBool.pszName = pszName;
	optBool.pbVal = pbVal;
	optBool.pszComment = pszComment ? tt::StrDup(pszComment) : nullptr;
	m_aOptBool += optBool;
}

bool CSrcFiles::UpdateOptVal(const char* pszName, bool bVal, const char* pszComment)
{
	ttASSERT(pszName);
	for (size_t pos = 0; pos < m_aOptBool.GetCount(); ++pos) {
		if (tt::isSameStri(pszName, m_aOptBool[pos].pszName)) {
			*m_aOptBool[pos].pbVal = bVal;
			if (pszComment) {
				if (m_aOptBool[pos].pszComment && !tt::isSameStri(m_aOptBool[pos].pszComment, pszComment)) {
					tt::FreeAlloc(m_aOptBool[pos].pszComment);
					//m_aOptBool[pos].pszComment = tt::StrDup(pszComment);
				}
			}
			return true;
		}
	}
	return false;
}

void CSrcFiles::AddOptVal(const char* pszName, ttCStr* pcszVal, const char* pszComment)
{
	OPT_VAL optVal;
	optVal.pszName = pszName;
	optVal.pcszVal = pcszVal;
	optVal.pszComment = pszComment ? tt::StrDup(pszComment) : nullptr;
	m_aOptVal += optVal;
}

bool CSrcFiles::UpdateOptVal(const char* pszName, const char* pszVal, const char* pszComment)
{
	ttASSERT(pszName && pszVal);
	for (size_t pos = 0; pos < m_aOptVal.GetCount(); ++pos) {
		if (tt::isSameStri(pszName, m_aOptVal[pos].pszName)) {
			m_aOptVal[pos].pcszVal->strCopy(pszVal);
			if (pszComment) {
				if (m_aOptVal[pos].pszComment && !tt::isSameStri(m_aOptVal[pos].pszComment, pszComment)) {
					tt::FreeAlloc(m_aOptVal[pos].pszComment);
					//m_aOptVal[pos].pszComment = tt::StrDup(pszComment);
				}
			}
			return true;
		}
	}
	return false;
}

// .srcfiles is a YAML file, so the value of the option may be within a single or double quote. That means we can't just
// search for '#' to find the comment, we must first step over any opening/closing quote.

bool CSrcFiles::GetOptionParts(char* pszLine, ttCStr& cszName, ttCStr& cszVal, ttCStr& cszComment)
{
	char* pszVal = strpbrk(pszLine, ":=");
	ttASSERT_MSG(pszVal, "Invalid Option -- missing ':' or '=' character");
	if (!pszVal) {
		ttCStr cszTmp;
		cszTmp.printf(tt::getResString(IDS_OPT_MISSING_COLON), pszLine);
		m_lstErrors += cszTmp;
		return false;
	}
	*pszVal = 0;
	cszName = pszLine;
	pszVal = tt::findNonSpace(pszVal + 1);

	if (*pszVal == CH_QUOTE || *pszVal == CH_SQUOTE || *pszVal == CH_START_QUOTE) {
		cszVal.getQuotedString(pszVal);
		pszVal += (cszVal.strLen() + 2);
		char* pszComment = tt::findChar(pszVal + cszVal.strLen() + 2, '#');
		if (pszComment)	{
			pszComment = tt::stepOver(pszComment);
			tt::trimRight(pszComment);	// remove any trailing whitespace
			cszComment = pszComment;
		}
		else {
			cszComment.Delete();
		}
	}
	else {	// non-quoted option
		char* pszComment = tt::findChar(pszVal, '#');
		if (pszComment) {
			*pszComment = 0;
			pszComment = tt::stepOver(pszComment);
			tt::trimRight(pszComment);	// remove any trailing whitespace
			cszComment = pszComment;
		}
		else {
			cszComment.Delete();
		}
		tt::trimRight(pszVal);	// remove any trailing whitespace
		cszVal = pszVal;
	}
	return true;
}

void CSrcFiles::AddOption(OPT_INDEX opt, const char* pszName, bool bRequired)
{
	ptrdiff_t pos = m_aOptions.Add(opt, new CSrcOption);
	m_aOptions.GetValueAt(pos)->AddOption(pszName, bRequired);
}
