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

	m_lstSrcFiles.SetFlags(ttCList::FLG_URL_STRINGS);
	m_lstLibFiles.SetFlags(ttCList::FLG_URL_STRINGS);
	m_lstErrors.SetFlags(ttCList::FLG_IGNORE_CASE);
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
		char* pszBegin = tt::FindNonSpace(pszLine);	// ignore any leading spaces
		if (tt::IsEmpty(pszBegin) || pszBegin[0] == '#' || (pszBegin[0] == '-' && pszBegin[1] == '-' && pszBegin[2] == '-')) {	// ignore empty, comment or divider lines
			continue;
		}

		if (tt::IsSameSubStrI(pszBegin, "%YAML")) {	// not required, but possible a YAML editor could add this
			continue;
		}

		if (tt::IsAlpha(*pszLine)) { 	// sections always begin with an alphabetical character
			if (tt::IsSameSubStrI(pszBegin, "Files:") || tt::IsSameSubStrI(pszBegin, "[FILES]")) {
				section = SECTION_FILES;
			}
			else if (tt::IsSameSubStrI(pszBegin, "Options:") || tt::IsSameSubStrI(pszBegin, "[OPTIONS]")) {
				section = SECTION_OPTIONS;
			}
			else if (tt::IsSameSubStrI(pszBegin, "Lib:")) {
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

	if (tt::IsEmpty(GetProjectName())) {
		ttCStr cszProj;
		cszProj.GetCWD();
		char* pszProj = tt::FindFilePortion(cszProj);
		if (pszProj && !tt::IsSameStrI(pszProj, "src"))
			UpdateOption(OPT_PROJECT, pszProj);
		else {
			if (pszProj > cszProj.GetPtr())
				--pszProj;
			*pszProj = 0;
			pszProj = tt::FindFilePortion(cszProj);
			if (pszProj)
				UpdateOption(OPT_PROJECT, pszProj);
		}
	}

	// If no Files: were specified, then we still won't have any files to build. Default to every type of C++ source file
	// in the current directory.

	if (m_lstSrcFiles.GetCount() < 1) {
		AddSourcePattern("*.cpp;*.cc;*.cxx;*.rc");
	}

	return true;
}

void CSrcFiles::ProcessOption(char* pszLine)
{
	ttCStr cszName, cszVal, cszComment;

	if (!GetOptionParts(pszLine, cszName, cszVal, cszComment))
		return;

	if (tt::IsSameStrI(cszName, "BuildLibs"))
		while (cszVal.ReplaceStr(".lib", ""));	// we want the target name, not the library filename

	if (UpdateReadOption(cszName, cszVal, cszComment))
		return;

	// If you need to support reading old options, add the code here to convert them into the new options. You will also
	// need to add code in CWriteSrcFiles::WriteUpdates to prevent writing the line out again.

	if (tt::IsSameStrI(cszName, "TargetDirs")) {	// first target is 32-bit directory, second is 64-bit directory
		ttCEnumStr cenum(cszVal, ';');
		if (cenum.Enum()) {
			UpdateOption(OPT_TARGET_DIR32, (char*) cenum);
		}
		if (cenum.Enum()) {
			UpdateOption(OPT_TARGET_DIR64, (char*) cenum);
		}
		return;
	}

	ttCStr csz;
	csz.printf(tt::GetResString(IDS_CS_UNKNOWN_OPTION), (char*) cszName);
	m_lstErrors += csz;
}

void CSrcFiles::AddCompilerFlag(const char* pszFlag)
{
	if (!GetOption(OPT_CFLAGS))
		UpdateOption(OPT_CFLAGS, pszFlag);
	// else append the flag if it hasn't already been added
	else if (!tt::FindStrI(GetOption(OPT_CFLAGS), pszFlag)) {
		ttCStr csz(GetOption(OPT_CFLAGS));
		csz += " ";
		csz += pszFlag;
		UpdateOption(OPT_CFLAGS, (char*) csz);
	}
}

void CSrcFiles::AddLibrary(const char* pszName)
{
	if (!tt::FindStrI(GetOption(OPT_LIBS), pszName)) {
		ttCStr csz(GetOption(OPT_LIBS));
		if (csz.IsNonEmpty())
			csz += " ";
		csz += pszName;
		UpdateOption(OPT_LIBS, (char*) csz);
	}
}

void CSrcFiles::ProcessLibSection(char* pszLibFile)
{
	// The library is built in the $libout directory, so we don't need to worry about a name conflict -- hence the default name of
	// "tmplib". The name is also to indicate that this is a temporary library -- it's not designed to be linked to outside of the
	// scope of the current project.

	if (m_cszLibName.IsEmpty())
		m_cszLibName = "tmplib";

	if (tt::FindStrI(pszLibFile, ".lib"))	// this was used before we created a default name
		return;
	else if (tt::IsSameSubStrI(pszLibFile, ".include")) {
		char* pszIncFile = tt::FindNonSpace(tt::FindSpace(pszLibFile));
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
	if (tt::IsSameSubStrI(pszFile, ".include")) {
		const char* pszIncFile = tt::FindNonSpace(tt::FindSpace(pszFile));
		ProcessInclude(pszIncFile, m_lstAddSrcFiles, true);
		return;
	}

	char* pszComment = tt::FindChar(pszFile, '#');
	if (pszComment) {
		*pszComment = 0;
		tt::TrimRight(pszFile);
	}

	m_lstSrcFiles += pszFile;
	if (!tt::FileExists(pszFile)) {
		ttCStr cszErrMsg;
		cszErrMsg.printf("Cannot locate %s", (char*) pszFile);
		m_lstErrors += cszErrMsg;
	}

	char* pszExt = tt::FindStrI(pszFile, ".idl");
	if (pszExt)	{
		m_lstIdlFiles += pszFile;
		return;
	}

	pszExt = tt::FindStrI(pszFile, ".rc");
	if (pszExt && !pszExt[3]) {	// ignore .rc2, .resources, etc.
		m_cszRcName = pszFile;
		return;
	}

	pszExt = tt::FindStrI(pszFile, ".hhp");
	if (pszExt) {	// ignore .rc2, .resources, etc.
		m_cszHHPName = pszFile;
		return;
	}
}

void CSrcFiles::ProcessInclude(const char* pszFile, ttCStrIntList& lstAddSrcFiles, bool bFileSection)
{
	if (tt::IsSameSubStrI(pszFile, ".include")) {
		const char* pszIncFile = tt::FindNonSpace(tt::FindSpace(pszFile));
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
	cszCWD.GetCWD();

	ttCStr cszFullPath(pszFile);
	cszFullPath.GetFullPathName();

	ttCTMem<char*> szPath(1024);
	tt::StrCopy(szPath, 1024, cszFullPath);
	char* pszFilePortion = tt::FindFilePortion(szPath);

	ttCStr cszRelative;

	for (size_t pos = 0; pos < (bFileSection ? cIncSrcFiles.m_lstSrcFiles.GetCount() : cIncSrcFiles.m_lstLibFiles.GetCount()); ++pos) {
		tt::StrCopy(pszFilePortion, bFileSection ? cIncSrcFiles.m_lstSrcFiles[pos] : cIncSrcFiles.m_lstLibFiles[pos]);
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
		while (ff.IsValid()) {
			char* psz = tt::FindLastChar(ff, '.');
			if (psz) {
				if (
						tt::IsSameStrI(psz, ".c") ||
						tt::IsSameStrI(psz, ".cpp") ||
						tt::IsSameStrI(psz, ".cc") ||
						tt::IsSameStrI(psz, ".cxx")
					) {
						m_lstSrcFiles += ff;

				}
				else if (tt::IsSameStrI(psz, ".rc")) {
					m_lstSrcFiles += ff;
					if (m_cszRcName.IsEmpty())
						m_cszRcName = ff;
				}
				else if (tt::IsSameStrI(psz, ".hhp")) {
					m_lstSrcFiles += ff;
					if (m_cszHHPName.IsEmpty())
						m_cszHHPName = ff;
				}
				else if (tt::IsSameStrI(psz, ".idl")) {
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

// .srcfiles is a YAML file, so the value of the option may be within a single or double quote. That means we can't just
// search for '#' to find the comment, we must first step over any opening/closing quote.

bool CSrcFiles::GetOptionParts(char* pszLine, ttCStr& cszName, ttCStr& cszVal, ttCStr& cszComment)
{
	char* pszVal = strpbrk(pszLine, ":=");
	ttASSERT_MSG(pszVal, "Invalid Option -- missing ':' or '=' character");
	if (!pszVal) {
		ttCStr cszTmp;
		cszTmp.printf(tt::GetResString(IDS_CS_OPT_MISSING_COLON), pszLine);
		m_lstErrors += cszTmp;
		return false;
	}
	*pszVal = 0;
	cszName = pszLine;
	pszVal = tt::FindNonSpace(pszVal + 1);

	if (*pszVal == CH_QUOTE || *pszVal == CH_SQUOTE || *pszVal == CH_START_QUOTE) {
		cszVal.GetQuotedString(pszVal);
		pszVal += (cszVal.StrLen() + 2);
		char* pszComment = tt::FindChar(pszVal + cszVal.StrLen() + 2, '#');
		if (pszComment)	{
			pszComment = tt::StepOver(pszComment);
			tt::TrimRight(pszComment);	// remove any trailing whitespace
			cszComment = pszComment;
		}
		else {
			cszComment.Delete();
		}
	}
	else {	// non-quoted option
		char* pszComment = tt::FindChar(pszVal, '#');
		if (pszComment) {
			*pszComment = 0;
			pszComment = tt::StepOver(pszComment);
			tt::TrimRight(pszComment);	// remove any trailing whitespace
			cszComment = pszComment;
		}
		else {
			cszComment.Delete();
		}
		tt::TrimRight(pszVal);	// remove any trailing whitespace
		cszVal = pszVal;
	}
	return true;
}

const char* CSrcFiles::GetPchHeader()
{
	const char* pszPch = GetOption(OPT_PCH);
	if (pszPch && tt::IsSameStrI(pszPch, "none"))
		return nullptr;
	return pszPch;
}
