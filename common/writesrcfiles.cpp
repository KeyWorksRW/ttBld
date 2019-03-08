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

	UpdateShortOption(GetOptionName(OPT_EXE_TYPE), GetOptionName(OPT_EXE_TYPE), "[window | console | lib | dll]", true);
	UpdateShortOption(GetOptionName(OPT_TARGET_DIR32), GetOptionName(OPT_TARGET_DIR32), nullptr, false);
	UpdateShortOption(GetOptionName(OPT_TARGET_DIR64), GetOptionName(OPT_TARGET_DIR64), nullptr, false);

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

	UpdateShortOption(GetOptionName(OPT_STATIC_CRT), GetBoolOption(OPT_STATIC_CRT) ? "true" : "false", "link to static (true) or DLL (false) version of CRT", GetBoolOption(OPT_STATIC_CRT));
	UpdateShortOption(GetOptionName(OPT_PERMISSIVE), GetBoolOption(OPT_PERMISSIVE) ? "true" : "false", "permissive compiler option", GetBoolOption(OPT_PERMISSIVE));
	UpdateShortOption(GetOptionName(OPT_STDCALL), GetBoolOption(OPT_STDCALL) ? "true" : "false", "use stdcall calling convention (default is cdecl)", GetBoolOption(OPT_STDCALL));
	UpdateShortOption(GetOptionName(OPT_MS_LINKER), GetBoolOption(OPT_MS_LINKER) ? "true" : "false", "use MS link.exe even when compiling with CLANG", GetBoolOption(OPT_MS_LINKER));

	UpdateShortOption(GetOptionName(OPT_COMPILERS), GetOption(OPT_COMPILERS), "[CLANG and/or MSVC]", GetOption(OPT_COMPILERS));
	UpdateShortOption(GetOptionName(OPT_MAKEFILE), GetOption(OPT_MAKEFILE), "[never | missing | always]", GetOption(OPT_MAKEFILE) && !tt::findStri(GetOption(OPT_MAKEFILE), "never"));
	UpdateShortOption(GetOptionName(OPT_IDE), GetOption(OPT_IDE), "[CodeBlocks and/or CodeLite and/or VisualStudio]", GetOptionName(OPT_IDE));
	UpdateShortOption(GetOptionName(OPT_OPTIMIZE), GetOption(OPT_OPTIMIZE), "[space | speed] (default is space)", GetOptionName(OPT_OPTIMIZE) && tt::findStri(GetOptionName(OPT_OPTIMIZE), "speed"));
	UpdateShortOption(GetOptionName(OPT_WARN_LEVEL), GetOption(OPT_WARN_LEVEL), "[1-4] default, if not specified, is 4", GetOption(OPT_WARN_LEVEL) && !tt::findStri(GetOption(OPT_WARN_LEVEL), "4"));
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
