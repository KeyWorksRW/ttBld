/////////////////////////////////////////////////////////////////////////////
// Name:		CWriteSrcFiles
// Purpose:		Class for writing out a new or updated version of .srcfiles
// Author:		Ralph Walden
// Copyright:	Copyright (c) 1998-2019 KeyWorks Software (Ralph Walden)
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

	while (kfIn.ReadLine()) {
		// Don't add any old options that have been replaced
		if (tt::FindStr(kfIn, "TargetDirs:"))
			continue;
		else if (tt::FindStr(kfIn, "LinkFlags:"))
			continue;
		else if (tt::FindStr(kfIn, "bit_suffix:"))
			continue;

		m_lstOriginal += (const char*) kfIn;
	}

	ttCStr cszOrg;
	ttCStr cszComment;

	// The calling order below will determine the order Sections appear (if they don't already exist)

	PreProcessOptions();
	UpdateOptionsSection();

	ttCFile kfOut;

	// Write all the lines, but prevent more then one blank line at a time

	size_t cBlankLines = 0;
	for (size_t pos = 0; pos < m_lstOriginal.GetCount(); ++pos)	{
		char* pszLine = (char*) m_lstOriginal[pos];	// since we're reducing the size, and about to delete it, okay to modify
		tt::TrimRight(pszLine);
		if (*pszLine)
		   cBlankLines = 0;
		else if (cBlankLines > 0)	// ignore if last line was also blank
			continue;
		else
			++cBlankLines;

		// Add a blank line before 64Bit so that platform settings are a bit easier to spot

		if (!cBlankLines && tt::IsSameSubStr(tt::FindNonSpace(m_lstOriginal[pos]), "64Bit")) {
			++cBlankLines;
			kfOut.WriteEol();
		}
		kfOut.WriteEol(m_lstOriginal[pos]);
	}

	kfIn.RestoreCopy();
	if (strcmp(kfIn, kfOut) == 0)
		return false;	// nothing has changed
	if (m_dryrun.IsEnabled()) {
		m_dryrun.NewFile(pszFile);
		m_dryrun.DisplayFileDiff(kfIn, kfOut);
		return false;	// since we didn't actually change anything
	}
	return kfOut.WriteFile(pszFile);
}

bool CWriteSrcFiles::WriteNew(const char* pszFile, const char* pszCommentHdr)
{
	m_lstOriginal.SetFlags(ttCList::FLG_ADD_DUPLICATES);	// required to add blank lines
	if (pszCommentHdr) {
		m_lstOriginal += pszCommentHdr;
		m_lstOriginal += "";
	}
	m_lstOriginal += "Options:";
	if (m_lstSrcFiles.GetCount() || m_lstIdlFiles.GetCount() ||  m_cszRcName.IsNonEmpty()) {
		ttCStr cszFile;
		m_lstOriginal += "";
		m_lstOriginal += "Files:";
		if (m_cszRcName.IsNonEmpty()) {
			cszFile = "    ";
			cszFile += (const char*) m_cszRcName;
			m_lstOriginal += cszFile;
		}
		for (size_t pos = 0; pos < m_lstIdlFiles.GetCount(); ++pos) {
			cszFile = "    ";
			cszFile += m_lstIdlFiles[pos];
			m_lstOriginal += cszFile;
		}
		for (size_t pos = 0; pos < m_lstSrcFiles.GetCount(); ++pos) {
			cszFile = "    ";
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
		if (tt::IsSameSubStrI(tt::FindNonSpace(m_lstOriginal[pos]), pszOption)) {
			cszDst = m_lstOriginal[pos];
			return (ptrdiff_t) pos;
		}
	}
	return -1;
}

ptrdiff_t CWriteSrcFiles::FindSection(const char* pszSection)
{
	for (size_t pos = 0; pos < m_lstOriginal.GetCount(); ++pos) {
		if (tt::IsSameSubStrI(m_lstOriginal[pos], pszSection)) {
			return (ptrdiff_t) pos;
		}
	}
	return -1;
}

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
			if (ttstrlen(m_lstOriginal[m_posInsert]) < 1)
				break;
			else if (tt::IsAlpha(m_lstOriginal[m_posInsert][0])) {
				m_lstOriginal.InsertAt(m_posInsert, "");	// insert a blank line
				break;
			}
		}
	}

	const sfopt::OPT_SETTING* aOptions = GetOrgOptions();
	for (size_t pos = 0; aOptions[pos].opt != OPT_OVERFLOW; ++pos)
		UpdateWriteOption(pos);
}

// If the option already exists then update it. If it doesn't exist, only write it if bAlwaysWrite is true

static const char* pszOptionFmt =      "    %-12s %-12s # %s";
static const char* pszLongOptionFmt =  "    %-12s %-30s    # %s";
static const char* pszNoCmtOptionFmt = "    %-12s %s";	// used when there isn't a comment

void CWriteSrcFiles::UpdateWriteOption(size_t pos)
{
	const sfopt::OPT_SETTING* aOptions = GetOrgOptions();
	ttCStr cszName(aOptions[pos].pszName);
	cszName += ":";	   // add the colon that separates a YAML key/value pair

	char szLine[4096];
	ptrdiff_t posOption = GetOptionLine(aOptions[pos].pszName);	// this will set m_cszOptComment
	if (posOption >= 0) {
		// The option already exists, so add any changes that might have been made

		if (m_cszOptComment.IsEmpty())		// we keep any comment that was previously used
			// REVIEW: [randalphwa - 3/13/2019] we don't allow changing the comment after it has been read in
			m_cszOptComment = aOptions[pos].pszComment;

		if (m_cszOptComment.IsNonEmpty() && ttstrlen(m_aUpdateOpts[pos].pszVal) > 12)
			// we use sprintf instead of ttCStr::printf because ttCStr doesn't support the %-12s width format specifier that we need
			sprintf_s(szLine, sizeof(szLine), pszLongOptionFmt,
				(char*) cszName, m_aUpdateOpts[pos].pszVal, (char*) m_cszOptComment);
		else
			// we use sprintf instead of ttCStr::printf because ttCStr doesn't support the %-12s width format specifier that we need
			sprintf_s(szLine, sizeof(szLine), m_cszOptComment.IsNonEmpty() ? pszOptionFmt : pszNoCmtOptionFmt,
				(char*) cszName, m_aUpdateOpts[pos].pszVal, (char*) m_cszOptComment);

		m_lstOriginal.Replace(posOption, szLine);	// replace the original line
	}
	else if (GetChanged(aOptions[pos].opt) || GetRequired(aOptions[pos].opt)) {
		sprintf_s(szLine, sizeof(szLine), ttstrlen(m_aUpdateOpts[pos].pszVal) > 12 ? pszLongOptionFmt : pszOptionFmt,
			(char*) cszName,
				m_aUpdateOpts[pos].pszVal ? m_aUpdateOpts[pos].pszVal : "",
				aOptions[pos].pszComment ? aOptions[pos].pszComment : "");
		m_lstOriginal.InsertAt(m_posInsert++, szLine);
	}
}

// If option is found and it has a comment, m_cszOptComment will be set to the comment

ptrdiff_t CWriteSrcFiles::GetOptionLine(const char* pszOption)
{
	m_cszOptComment.Delete();
	for (ptrdiff_t pos = m_posOptions + 1; pos < (ptrdiff_t) m_lstOriginal.GetCount(); ++pos) {
		if (tt::IsAlpha(*m_lstOriginal[pos]))
			break;	// New sections start with an alphabetical character in the first column

		const char* pszLine = tt::FindNonSpace(m_lstOriginal[pos]);
		if (tt::IsSameSubStrI(pszLine, pszOption)) {
			const char* pszComment = tt::FindChar(pszLine, '#');
			if (pszComment)
				m_cszOptComment = tt::FindNonSpace(pszComment + 1);
			else
				m_cszOptComment.Delete();
			return pos;
		}
		// Options are supposed to be indented -- any alphabetical character that is non-indented presumably starts a new section
		else if (tt::IsAlpha(*m_lstOriginal[pos]))
			break;
	}
	return -1;
}

void CWriteSrcFiles::PreProcessOptions()
{
	if (GetBoolOption(OPT_64BIT)) {
		SetRequired(OPT_64BIT, true);
		SetRequired(OPT_TARGET_DIR64, true);
	}
	if (GetBoolOption(OPT_32BIT)) {
		SetRequired(OPT_32BIT, true);
		SetRequired(OPT_TARGET_DIR32, true);
	}
}
