/////////////////////////////////////////////////////////////////////////////
// Name:		CWriteSrcFiles
// Purpose:		Class for writing out a new or updated version of .srcfiles
// Author:		Ralph Walden
// Copyright:	Copyright (c) 1998-2018 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <stdio.h>	// for sprintf

#include <ttfile.h> // ttCFile

#include "writesrcfiles.h"

// At the very least, we preserve any comments the user may have added, as well as any non-blank lines that we don't
// process (such as '------' used to delimit sections). We do prevent multiple blank lines, trailing spaces, and we do
// enforce LF EOL (CR/LF will be replaced with just LF). If it's a line we write, we may change the spacing used
// within the line.

// Put another way, we may change spacing and formating, but we will preserve any custom data like comments

bool CWriteSrcFiles::WriteUpdates(const char* pszFile)
{
	m_lstOriginal.SetFlags(ttCList::FLG_ADD_DUPLICATES);
	ttCFile kfIn;
	if (!kfIn.ReadFile(pszFile))
		return false;
	kfIn.MakeCopy();

	while (kfIn.ReadLine())
		m_lstOriginal += (const char*) kfIn;

	ttCStr cszOrg;
	ttCStr cszComment;

	// The calling order below will determine the order Sections appear (if they don't already exist)

	UpdateOptionsSection();

	ttCFile kfOut;

	// Write all the lines, but prevent more then one blank line at a time

	size_t cBlankLines = 0;
	for (size_t pos = 0; pos < m_lstOriginal.GetCount(); ++pos)	{
		char* pszLine = (char*) m_lstOriginal[pos];	// since we're reducing the size, and about to delete it, okay to modify
		tt::trimRight(pszLine);
		if (*pszLine)
		   cBlankLines = 0;
		else if (cBlankLines > 0)	// ignore if last line was also blank
			continue;
		else
			++cBlankLines;
		kfOut.WriteEol(m_lstOriginal[pos]);
	}

	kfIn.RestoreCopy();
	if (strcmp(kfIn, kfOut) == 0)
		return false;	// nothing has changed
	if (m_dryrun.isEnabled()) {
		m_dryrun.NewFile(pszFile);
		m_dryrun.DisplayFileDiff(kfIn, kfOut);
		return false;	// since we didn't actually change anything
	}
	return kfOut.WriteFile(pszFile);
}

bool CWriteSrcFiles::WriteNew(const char* pszFile)
{
	m_lstOriginal.SetFlags(ttCList::FLG_ADD_DUPLICATES);	// required to add blank lines
	m_lstOriginal += "Options:";
	if (m_lstSrcFiles.GetCount() || m_lstIdlFiles.GetCount() ||  m_cszRcName.isNonEmpty()) {
		ttCStr cszFile;
		m_lstOriginal += "";
		m_lstOriginal += "Files:";
		if (m_cszRcName.isNonEmpty()) {
			cszFile = "  ";
			cszFile += (const char*) m_cszRcName;
			m_lstOriginal += cszFile;
		}
		for (size_t pos = 0; pos < m_lstIdlFiles.GetCount(); ++pos) {
			cszFile = "  ";
			cszFile += m_lstIdlFiles[pos];
			m_lstOriginal += cszFile;
		}
		for (size_t pos = 0; pos < m_lstSrcFiles.GetCount(); ++pos) {
			cszFile = "  ";
			cszFile += m_lstSrcFiles[pos];
			m_lstOriginal += cszFile;
		}
	}

	UpdateOptionsSection();
	ttCFile kfOut;
	kfOut.SetUnixLF();

	for (size_t pos = 0; pos < m_lstOriginal.GetCount(); ++pos)
		kfOut.WriteEol(m_lstOriginal[pos]);
	return kfOut.WriteFile(pszFile);
}

ptrdiff_t CWriteSrcFiles::FindOption(const char* pszOption, ttCStr& cszDst)
{
	for (size_t pos = 0; pos < m_lstOriginal.GetCount(); ++pos) {
		if (tt::isSameSubStri(tt::findNonSpace(m_lstOriginal[pos]), pszOption)) {
			cszDst = m_lstOriginal[pos];
			return (ptrdiff_t) pos;
		}
	}
	return -1;
}

ptrdiff_t CWriteSrcFiles::FindSection(const char* pszSection)
{
	for (size_t pos = 0; pos < m_lstOriginal.GetCount(); ++pos) {
		if (tt::isSameSubStri(m_lstOriginal[pos], pszSection)) {
			return (ptrdiff_t) pos;
		}
	}
	return -1;
}

static const char* pszOptionFmt = "    %-12s %-12s # %s";
static const char* pszLongOptionFmt = "    %-12s %s";

// The goal here is to see if an option was already specified, and if so update it to it's new value. If the option
// already had a comment, we leave it intact, otherwise we add our own.

// Note that some options must always be written -- those will be created if they don't already exist.

void CWriteSrcFiles::UpdateOptionsSection()
{
	m_posOptions = FindSection("Options:");
	if (m_posOptions < 0) {
		m_posOptions = 0;
		m_lstOriginal.InsertAt(m_posOptions, "Options:");
		m_posInsert = m_posOptions + 1;
		m_lstOriginal.InsertAt(m_posInsert, "");	// insert a blank line
	}
	else {
		// Set the insertion point at the first blank line, or before the next Section

		for (m_posInsert = m_posOptions + 1; m_posInsert < (ptrdiff_t) m_lstOriginal.GetCount(); ++m_posInsert) {
			if (tt::strLen(m_lstOriginal[m_posInsert]) < 1)
				break;
			else if (tt::isAlpha(m_lstOriginal[m_posInsert][0])) {
				m_lstOriginal.InsertAt(m_posInsert, "");	// insert a blank line
				break;
			}
		}
	}

	for (ptrdiff_t pos = 0; pos < m_aOptions.GetCount(); ++pos) {
		CSrcOption* pcls = m_aOptions.GetValueAt(pos);	// just to make the code look a bit cleaner
		UpdateShortOption(pcls->getName(), pcls->getOption(), pcls->getComment(), pcls->isRequired());
	}

	// Long options don't have default comments, though we will keep any comments the user may have added. If we pass a
	// null or empty pointer for the value then we completely delete the option if there already was one

	for (size_t pos = 0; pos < m_aOptVal.GetCount(); ++pos)
		UpdateLongOption(m_aOptVal[pos].pszName, m_aOptVal[pos].pcszVal->getPtr(), m_aOptVal[pos].pszComment);

//	UpdateLongOption("Project:", GetProjectName(), "project target name");

	ttCStr cszTargets;
	CreateTargetsString(cszTargets);
	UpdateShortOption("exe_type:", GetExeType(), "[window | console | lib | dll]", true);

	if (GetOption(OPT_64BIT)) {
		UpdateShortOption("64Bit:", "true", "create 64-bit project", true);
		UpdateShortOption(GetOptionName(OPT_BIT_SUFFIX), GetBoolOption(OPT_BIT_SUFFIX) ? "true" : "false",
			"add \"64\" to the end of target directory or project (e.g., ../bin64/)", GetBoolOption(OPT_BIT_SUFFIX));
	}
	else {
		UpdateShortOption("64Bit:", "false", "create 64-bit project", false);	// only update if it already exists
		UpdateShortOption(GetOptionName(OPT_BIT_SUFFIX), GetBoolOption(OPT_BIT_SUFFIX) ? "true" : "false",
			"add \"64\" to the end of target directory or project (e.g., ../bin64/)", false);
	}

	{
		char szNum[10];
		tt::Utoa(m_WarningLevel > 0 ? m_WarningLevel : WARNLEVEL_DEFAULT, szNum, sizeof(szNum));
		UpdateShortOption("WarnLevel:", szNum, "compiler warning level (1-4)", m_WarningLevel != WARNLEVEL_DEFAULT);
	}

	UpdateShortOption("optimize:", m_bBuildForSpeed ? "speed" : "space", "speed or space (default)", m_bBuildForSpeed);
	UpdateShortOption(GetOptionName(OPT_STATIC_CRT), GetBoolOption(OPT_STATIC_CRT) ? "true" : "false", "link to static (true) or DLL (false) version of CRT", GetBoolOption(OPT_STATIC_CRT));
	UpdateShortOption(GetOptionName(OPT_PERMISSIVE), GetBoolOption(OPT_PERMISSIVE) ? "true" : "false", "permissive compiler option", GetBoolOption(OPT_PERMISSIVE));
	UpdateShortOption(GetOptionName(OPT_STDCALL), GetBoolOption(OPT_STDCALL) ? "true" : "false", "use stdcall calling convention (default is cdecl)", GetBoolOption(OPT_STDCALL));
	UpdateShortOption(GetOptionName(OPT_MS_LINKER), GetBoolOption(OPT_MS_LINKER) ? "true" : "false", "use MS link.exe even when compiling with CLANG", GetBoolOption(OPT_MS_LINKER));

	UpdateLongOption("TargetDirs:", cszTargets);

	// Long options don't have default comments, though we will keep any comments the user may have added. If we pass a
	// null or empty pointer for the value then we completely delete the option if there already was one

#if 0
	UpdateLongOption("PCH:", 		(char*) m_cszPCHheader);
	UpdateLongOption("Cflags:", 	(char*) m_cszCFlags);
	UpdateLongOption("Midlflags:", 	(char*) m_cszMidlFlags);
	UpdateLongOption("LinkFlags:",	(char*) m_cszLinkFlags);
	UpdateLongOption("Libs:", 		(char*) m_cszLibs);
	UpdateLongOption("BuildLibs:", 	(char*) m_cszBuildLibs);
	UpdateLongOption("IncDirs:", 	(char*) m_cszIncDirs);
	UpdateLongOption("LibDirs:", 	(char*) m_cszLibDirs);
#endif

	UpdateShortOption(GetOptionName(OPT_COMPILERS), GetOption(OPT_COMPILERS), "[CLANG and/or MSVC]", GetOption(OPT_COMPILERS));

	const char* pszVal;
	if (m_fCreateMakefile == MAKEMAKE_NEVER)
		pszVal = "never";
	else if (m_fCreateMakefile == MAKEMAKE_ALWAYS)
		pszVal = "always";
	else
		pszVal = "missing";
	UpdateShortOption("Makefile:", pszVal, "[never | missing | always]", m_fCreateMakefile != MAKEMAKE_DEFAULT);

	ttCStr cszTmp;
	if (m_IDE & IDE_CODEBLOCK)
		cszTmp += "CodeBlocks ";
	if (m_IDE & IDE_CODELITE)
		cszTmp += "CodeLite ";
	if (m_IDE & IDE_VS)
		cszTmp += "VisualStudio";
	if (cszTmp.isNonEmpty())
		tt::trimRight(cszTmp);
	UpdateLongOption("IDE:", cszTmp, "[CodeBlocks and/or CodeLite and/or VisualStudio]");
}

void CWriteSrcFiles::UpdateShortOption(const char* pszOption, const char* pszVal, const char* pszComment, bool bAlwaysWrite)
{
	char szLine[4096];
	ptrdiff_t posOption = GetOptionLine(pszOption);
	if (posOption >= 0) {
		if (m_cszOptComment.isEmpty())		// we keep any comment that was previously used
			m_cszOptComment = pszComment;	// otherwise we use our own
		sprintf_s(szLine, sizeof(szLine), pszOptionFmt, pszOption, pszVal, (char*) m_cszOptComment);
		m_lstOriginal.Replace(posOption, szLine);
	}
	else if (bAlwaysWrite) {
		sprintf_s(szLine, sizeof(szLine), pszOptionFmt, pszOption, pszVal, pszComment);
		m_lstOriginal.InsertAt(m_posInsert++, szLine);
	}
}

void CWriteSrcFiles::UpdateLongOption(const char* pszOption, const char* pszVal, const char* pszComment)
{
	// Need to be certain all options are followed with a ':' character
	ttCStr cszOption(pszOption);
	if (!cszOption.findChar(':'))
		cszOption += ":";
	pszOption = cszOption;	// purely for convenience

	char szLine[4096];
	ptrdiff_t posOption = GetOptionLine(pszOption);
	if (posOption >= 0) {
		if (!pszVal || !*pszVal) {
			m_lstOriginal.Remove(posOption);
			return;
		}
		if (m_cszOptComment.isEmpty() && pszComment)	// we keep any comment that was previously used
			m_cszOptComment = pszComment;				// otherwise we use our own if supplied

		if (tt::strLen(pszVal) <= 12 && m_cszOptComment.isNonEmpty()) {	// can we use the shorter formatted version?
			sprintf_s(szLine, sizeof(szLine), pszOptionFmt, pszOption, pszVal, (char*) m_cszOptComment);
			m_lstOriginal.Replace(posOption, szLine);
			return;
		}

		sprintf_s(szLine, sizeof(szLine), pszLongOptionFmt, pszOption, pszVal);

		if (m_cszOptComment.isNonEmpty()) { 			// we keep any comment that was previously used
			tt::strCat_s(szLine, sizeof(szLine), "    # ");
			tt::strCat_s(szLine, sizeof(szLine), m_cszOptComment);
		}
		m_lstOriginal.Replace(posOption, szLine);
	}
	else if (pszVal && *pszVal) {
		if (tt::strLen(pszVal) <= 12 && m_cszOptComment.isNonEmpty()) {	// can we use the shorter formatted version?
			sprintf_s(szLine, sizeof(szLine), pszOptionFmt, pszOption, pszVal, (char*) m_cszOptComment);
			m_lstOriginal.Replace(posOption, szLine);
		}
		else {
			sprintf_s(szLine, sizeof(szLine), pszLongOptionFmt, pszOption, pszVal);
			m_lstOriginal.InsertAt(m_posInsert++, szLine);
		}
	}
}

// If option is found and it has a comment, m_cszOptComment will be set to the comment

ptrdiff_t CWriteSrcFiles::GetOptionLine(const char* pszOption)
{
	m_cszOptComment.Delete();
	for (ptrdiff_t pos = m_posOptions + 1; pos < (ptrdiff_t) m_lstOriginal.GetCount(); ++pos) {
		if (tt::isAlpha(*m_lstOriginal[pos]))
			break;	// New sections start with an alphabetical character in the first column

		const char* pszLine = tt::findNonSpace(m_lstOriginal[pos]);
		if (tt::isSameSubStri(pszLine, pszOption)) {
			const char* pszComment = tt::findChar(pszLine, '#');
			if (pszComment)
				m_cszOptComment = tt::findNonSpace(pszComment + 1);
			else
				m_cszOptComment.Delete();
			return pos;
		}
		// Options are supposed to be indented -- any alphabetical character that is non-indented presumably starts a new section
		else if (tt::isAlpha(*m_lstOriginal[pos]))
			break;
	}
	return -1;
}

const char* CWriteSrcFiles::GetExeType()
{
	switch (m_exeType) {
		case EXE_CONSOLE:
			return "console";
			break;

		case EXE_LIB:
			return "lib";
			break;

		case EXE_DLL:
			return "dll";
			break;

		case EXE_WINDOW:
		default:
			return "window";
			break;
	}
}

void CWriteSrcFiles::CreateTargetsString(ttCStr& cszTargets)
{
	if (m_cszTarget32.isEmpty() && m_cszTarget64.isEmpty()) {
		if (m_exeType == EXE_LIB) {
			if (tt::DirExists("../lib")) {
				m_cszTarget32 = "../lib";
				if (tt::DirExists("../lib64"))
					m_cszTarget64 = "../lib64";
			}
			else {
				m_cszTarget32 = "lib";
				if (tt::DirExists("lib64"))
					m_cszTarget64 = "lib64";
			}
		}

		else {
			if (tt::DirExists("../bin")) {
				m_cszTarget32 = "../bin";
				if (tt::DirExists("../bin64"))
					m_cszTarget64 = "../bin64";
			}
			else {
				m_cszTarget32 = "bin";
				if (tt::DirExists("bin64"))
					m_cszTarget64 = "bin64";
			}
		}
	}

	if (m_cszTarget32.isNonEmpty())	{
		if (!GetOption(OPT_64BIT) && !GetOption(OPT_BIT_SUFFIX))	// m_b64bit with no suffix means 64-bit only target
			cszTargets = (char*) m_cszTarget32;
		if (GetOption(OPT_64BIT) && m_cszTarget64.isNonEmpty())	{
			cszTargets += "; ";
			cszTargets += (char*) m_cszTarget64;
		}
	}
	else if (GetOption(OPT_64BIT) && m_cszTarget64.isNonEmpty()) {
		cszTargets = "; ";
		cszTargets += (char*) m_cszTarget64;
	}
}
