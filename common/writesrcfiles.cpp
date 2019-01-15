/////////////////////////////////////////////////////////////////////////////
// Name:		CWriteSrcFiles
// Purpose:		Class for writing out a new or updated version of .srcfiles
// Author:		Ralph Walden
// Copyright:	Copyright (c) 1998-2018 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <stdio.h>	// for sprintf

#include "../ttLib/include/ttfile.h"	// ttFile

#include "writesrcfiles.h"

// At the very least, we preserve any comments the user may have added, as well as any non-blank lines that we don't
// process (such as '------' used to delimit sections). We do prevent multiple blank lines, trailing spaces, and we do
// enforce LF EOL (CR/LF will be replaced with just LF). If it's a line we write, we may change the spacing used
// within the line.

// Put another way, we may change spacing and formating, but we will preserve any custom data like comments

bool CWriteSrcFiles::WriteUpdates(const char* pszFile)
{
	m_lstOriginal.SetFlags(ttList::FLG_ADD_DUPLICATES);
	ttFile kfIn;
	if (!kfIn.ReadFile(pszFile)) {
		return false;
	}
	while (kfIn.readline()) {
		m_lstOriginal += (const char*) kfIn;
	}

	ttString cszOrg;
	ttString cszComment;

	// The calling order below will determine the order Sections appear (if they don't already exist)

	UpdateOptionsSection();

	ttFile kfOut;
	kfOut.SetUnixLF();

	// Write all the lines, but prevent more then one blank line at a time

	size_t cBlankLines = 0;
	for (size_t pos = 0; pos < m_lstOriginal.GetCount(); ++pos)	{
		char* pszLine = (char*) m_lstOriginal[pos];	// since we're reducing the size, and about to delete it, okay to modify
		tt::trim_right(pszLine);
		if (*pszLine)
		   cBlankLines = 0;
		else if (cBlankLines > 0)	// ignore if last line was also blank
			continue;
		else
			++cBlankLines;
		kfOut.WriteEol(m_lstOriginal[pos]);
	}

	kfIn.Delete();
	kfIn.ReadFile(pszFile);
	if (strcmp(kfIn, kfOut) == 0)
		return false;	// nothing has changed
	return kfOut.WriteFile(pszFile);
}

bool CWriteSrcFiles::WriteNew(const char* pszFile)
{
	m_lstOriginal.SetFlags(ttList::FLG_ADD_DUPLICATES);	// required to add blank lines
	m_lstOriginal += "Options:";
	if (m_lstSrcFiles.GetCount() || m_lstIdlFiles.GetCount() ||  m_cszRcName.IsNonEmpty()) {
		ttString cszFile;
		m_lstOriginal += "";
		m_lstOriginal += "Files:";
		if (m_cszRcName.IsNonEmpty()) {
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
	ttFile kfOut;
	kfOut.SetUnixLF();

	for (size_t pos = 0; pos < m_lstOriginal.GetCount(); ++pos)
		kfOut.WriteEol(m_lstOriginal[pos]);
	return kfOut.WriteFile(pszFile);
}

ptrdiff_t CWriteSrcFiles::FindOption(const char* pszOption, ttString& cszDst)
{
	for (size_t pos = 0; pos < m_lstOriginal.GetCount(); ++pos) {
		if (tt::samesubstri(tt::nextnonspace(m_lstOriginal[pos]), pszOption)) {
			cszDst = m_lstOriginal[pos];
			return (ptrdiff_t) pos;
		}
	}
	return -1;
}

ptrdiff_t CWriteSrcFiles::FindSection(const char* pszSection)
{
	for (size_t pos = 0; pos < m_lstOriginal.GetCount(); ++pos) {
		if (tt::samesubstri(m_lstOriginal[pos], pszSection)) {
			return (ptrdiff_t) pos;
		}
	}
	return -1;
}

static const char* pszOptionFmt = "  %-12s %-12s # %s";
static const char* pszLongOptionFmt = "  %-12s %s";

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
			if (tt::strlen(m_lstOriginal[m_posInsert]) < 1)
				break;
			else if (tt::isalpha(m_lstOriginal[m_posInsert][0])) {
				m_lstOriginal.InsertAt(m_posInsert, "");	// insert a blank line
				break;
			}
		}
	}

	UpdateLongOption("Project:", GetProjectName(), "project target name");

	ttString cszTargets;
	CreateTargetsString(cszTargets);
	UpdateLongOption("TargetDirs:", cszTargets);
	if (m_cszSrcPattern.IsNonEmpty())
		UpdateOption("Sources:", m_cszSrcPattern, "source file patterns", true);
	UpdateOption("exe_type:", GetExeType(), "[window | console | lib | dll]", true);

	if (m_b64bit)
		UpdateOption("64Bit:", "true", "create 64-bit project", true);
	else
		UpdateOption("64Bit:", "false", "create 64-bit project", false);	// only update if it already exists

	UpdateOption("bit_suffix:", m_bBitSuffix ? "true" : "false", "add \"64\" to the end of target directory or project (e.g., ../bin64/)", m_bBitSuffix);
	{
		char szNum[10];
		tt::utoa(m_WarningLevel > 0 ? m_WarningLevel : WARNLEVEL_DEFAULT, szNum, sizeof(szNum));
		UpdateOption("WarnLevel:", szNum, "compiler warning level (1-4)", m_WarningLevel != WARNLEVEL_DEFAULT);
	}

	UpdateOption("optimize:", m_bBuildForSpeed ? "speed" : "space", "speed or space (default)", m_bBuildForSpeed);
	UpdateOption("static_crt:", m_bStaticCrt ? "true" : "false", "link to static (true) or DLL (false) version of CRT", m_bStaticCrt);
	UpdateOption("permissive:", m_bPermissive ? "true" : "false", "permissive compiler option", m_bPermissive);
	UpdateOption("stdcall:", m_bStdcall ? "true" : "false", "use stdcall calling convention (default is cdecl)", m_bStdcall);
	UpdateOption("ms_linker:", m_bUseMsvcLinker ? "true" : "false", "use MS link.exe even when compiling with CLANG", m_bUseMsvcLinker);

	// Long options don't have default comments, though we will keep any comments the user may have added. If we pass a
	// null or empty pointer for the value then we completely delete the option if there already was one

	UpdateLongOption("PCH:", 		(char*) m_cszPCHheader);
	UpdateLongOption("Cflags:", 	(char*) m_cszCFlags);
	UpdateLongOption("Midlflags:", 	(char*) m_cszMidlFlags);
	UpdateLongOption("LinkFlags:",	(char*) m_cszLinkFlags);
	UpdateLongOption("Libs:", 		(char*) m_cszLibs);
	UpdateLongOption("BuildLibs:", 	(char*) m_cszBuildLibs);
	UpdateLongOption("IncDirs:", 	(char*) m_cszIncDirs);
	UpdateLongOption("LibDirs:", 	(char*) m_cszLibDirs);

	ttString cszTmp;
	if (m_CompilerType & COMPILER_CLANG)
		cszTmp = "CLANG ";
	if (m_CompilerType & COMPILER_MSVC)
		cszTmp += "MSVC ";
	UpdateOption("Compilers:", cszTmp, "[CLANG and/or MSVC]", m_CompilerType != COMPILER_DEFAULT);

	const char* pszVal;
	if (m_fCreateMakefile == MAKEMAKE_NEVER)
		pszVal = "never";
	else if (m_fCreateMakefile == MAKEMAKE_ALWAYS)
		pszVal = "always";
	else
		pszVal = "missing";
	UpdateOption("Makefile:", pszVal, "[never | missing | always]", m_fCreateMakefile != MAKEMAKE_DEFAULT);

	cszTmp.Delete();
	if (m_IDE & IDE_CODEBLOCK)
		cszTmp += "CodeBlocks ";
	if (m_IDE & IDE_CODELITE)
		cszTmp += "CodeLite ";
	if (m_IDE & IDE_VS)
		cszTmp += "VisualStudio";
	if (cszTmp.IsNonEmpty())
		tt::trim_right(cszTmp);
	UpdateLongOption("IDE:", cszTmp, "[CodeBlocks and/or CodeLite and/or VisualStudio]");
}

void CWriteSrcFiles::UpdateOption(const char* pszOption, const char* pszVal, const char* pszComment, bool bAlwaysWrite)
{
	char szLine[4096];
	ptrdiff_t posOption = GetOptionLine(pszOption);
	if (posOption >= 0) {
		if (m_cszOptComment.IsEmpty())		// we keep any comment that was previously used
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
	char szLine[4096];
	ptrdiff_t posOption = GetOptionLine(pszOption);
	if (posOption >= 0) {
		if (!pszVal || !*pszVal) {
			m_lstOriginal.Remove(posOption);
			return;
		}
		if (m_cszOptComment.IsEmpty() && pszComment)	// we keep any comment that was previously used
			m_cszOptComment = pszComment;				// otherwise we use our own if supplied

		if (tt::strlen(pszVal) <= 12 && m_cszOptComment.IsNonEmpty()) {	// can we use the shorter formatted version?
			sprintf_s(szLine, sizeof(szLine), pszOptionFmt, pszOption, pszVal, (char*) m_cszOptComment);
			m_lstOriginal.Replace(posOption, szLine);
			return;
		}

		sprintf_s(szLine, sizeof(szLine), pszLongOptionFmt, pszOption, pszVal);

		if (m_cszOptComment.IsNonEmpty()) { 			// we keep any comment that was previously used
			tt::strcat_s(szLine, sizeof(szLine), "    # ");
			tt::strcat_s(szLine, sizeof(szLine), m_cszOptComment);
		}
		m_lstOriginal.Replace(posOption, szLine);
	}
	else if (pszVal && *pszVal) {
		if (tt::strlen(pszVal) <= 12 && m_cszOptComment.IsNonEmpty()) {	// can we use the shorter formatted version?
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
		if (tt::isalpha(*m_lstOriginal[pos]))
			break;	// New sections start with an alphabetical character in the first column

		const char* pszLine = tt::nextnonspace(m_lstOriginal[pos]);
		if (tt::samesubstri(pszLine, pszOption)) {
			const char* pszComment = tt::strchr(pszLine, '#');
			if (pszComment)
				m_cszOptComment = tt::nextnonspace(pszComment + 1);
			else
				m_cszOptComment.Delete();
			return pos;
		}
		// Options are supposed to be indented -- any alphabetical character that is non-indented presumably starts a new section
		else if (tt::isalpha(*m_lstOriginal[pos]))
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

void CWriteSrcFiles::CreateTargetsString(ttString& cszTargets)
{
	if (m_cszTarget32.IsEmpty() && m_cszTarget64.IsEmpty()) {
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

	if (m_cszTarget32.IsNonEmpty())	{
		if (!(m_b64bit && !m_bBitSuffix))	// m_b64bit with no suffix means 64-bit only target
			cszTargets = (char*) m_cszTarget32;
		if (m_b64bit && m_cszTarget64.IsNonEmpty())	{
			cszTargets += "; ";
			cszTargets += (char*) m_cszTarget64;
		}
	}
	else if (m_b64bit && m_cszTarget64.IsNonEmpty()) {
		cszTargets = "; ";
		cszTargets += (char*) m_cszTarget64;
	}
}
