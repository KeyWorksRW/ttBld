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

	if (tt::isEmpty(GetProjectName())) {
		ttCStr cszProj;
		cszProj.getCWD();
		char* pszProj = tt::findFilePortion(cszProj);
		if (pszProj && !tt::isSameStri(pszProj, "src"))
			UpdateOption(OPT_PROJECT, pszProj);
		else {
			if (pszProj > cszProj.getPtr())
				--pszProj;
			*pszProj = 0;
			pszProj = tt::findFilePortion(cszProj);
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

	if (tt::isSameStri(cszName, "BuildLibs"))
		while (cszVal.ReplaceStr(".lib", ""));	// we want the target name, not the library filename

	if (UpdateReadOption(cszName, cszVal, cszComment))
		return;

	// If you need to support reading old options, add the code here to convert them into the new options. You will also
	// need to add code in CWriteSrcFiles::WriteUpdates to prevent writing the line out again.

	if (tt::isSameStri(cszName, "TargetDirs")) {	// first target is 32-bit directory, second is 64-bit directory
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
	csz.printf(tt::getResString(IDS_UNKNOWN_OPTION), (char*) cszName);
	m_lstErrors += csz;
}

void CSrcFiles::AddCompilerFlag(const char* pszFlag)
{
	if (!GetOption(OPT_CFLAGS))
		UpdateOption(OPT_CFLAGS, pszFlag);
	// else append the flag if it hasn't already been added
	else if (!tt::findStri(GetOption(OPT_CFLAGS), pszFlag)) {
		ttCStr csz(GetOption(OPT_CFLAGS));
		csz += " ";
		csz += pszFlag;
		UpdateOption(OPT_CFLAGS, (char*) csz);
	}
}

void CSrcFiles::AddLibrary(const char* pszName)
{
	if (!tt::findStri(GetOption(OPT_LIBS), pszName)) {
		ttCStr csz(GetOption(OPT_LIBS));
		if (csz.isNonEmpty())
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

	char* pszComment = tt::findChar(pszFile, '#');
	if (pszComment) {
		*pszComment = 0;
		tt::trimRight(pszFile);
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
