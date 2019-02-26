/////////////////////////////////////////////////////////////////////////////
// Name:		CParseHHP
// Purpose:		Parse a .HHP file and generate a list of file dependencies
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttfile.h> 	// ttCFile
#include <ttfindfile.h> // ttCFindFile

#include "parsehhp.h"	// CParseHHP
#include "ninja.h"		// CNinja

CParseHHP::CParseHHP(const char* pszHHPName)
{
	m_section = SECTION_UNKNOWN;
	m_cszHHPName = pszHHPName;
	m_cszChmFile = (char*) m_cszHHPName;
	m_cszChmFile.ChangeExtension(".chm");
	m_cszRoot = pszHHPName;
	m_cszRoot.getFullPathName();
	char* pszFile = tt::findFilePortion(m_cszRoot);
	if (pszFile)
		*pszFile = 0;
	m_cszCWD.getCWD();
}

static const char* aOptions[] = {	// array of options that specify files that will be compiled
	"Contents file",
	"Index file",
	"DAT FILE",
	"Default topic",

	nullptr
};

// This function can be called recursively if the .HHP file has a #include directive to #include another .hhp file

void CParseHHP::ParseHhpFile(const char* pszHHP)
{
	if (!pszHHP)
		pszHHP = m_cszHHPName;	// use root name

	ttCFile kf;
	if (!kf.ReadFile(pszHHP)) {
// REVIEW: [randalphwa - 11/29/2018] Need a way to add error msgs to CNinja
//		ttCStr cszMsg;
//		cszMsg.printf("Cannot read the file %s\n", pszHHP);
//		AddError(cszMsg);
		return;
	}

	/*
		The following are possible sections. We only care about sections which will specify files that are to be compiled

		[ALIAS]
		[FILES]
		[INFOTYPES]
		[MAP]
		[MERGE FILES]
		[OPTIONS]
		[SUBSETS]
		[TEXT POPUPS]
		[WINDOWS]
	*/

	while (kf.ReadLine()) {
		if (!kf[0])
			continue;	// blank line
		if (tt::isSameSubStri(kf, "#include")) {
			char* pszFile = tt::findNonSpace(tt::findSpace(kf));
			if (!*pszFile)
				continue;	// invalid #include -- doesn't specify a filename
			ttCStr cszFile;
			if (*pszFile == CH_QUOTE) {
				cszFile.getQuotedString(pszFile);
			}
			else
				cszFile = pszFile;

			// [KeyWorksRW - 11-29-2018] It's not clear from HH docs when comments are allowed on a line -- we remove it
			// here just in case.

			char* pszComment = tt::findChar(cszFile, ';');
			if (pszComment)
				*pszComment = 0;
			tt::trimRight(cszFile);

			// A couple of sections sometimes include .h files -- but we only care about an #included .hhp file

			if (tt::findStri(cszFile, ".hhp"))
				ParseHhpFile(cszFile);
			continue;
		}

		if (kf[0] == '[') {	// sections are placed in brackets
			m_section = SECTION_UNKNOWN;
			if (tt::isSameSubStri(kf, "[ALIAS"))
				m_section = SECTION_ALIAS;
			else if (tt::isSameSubStri(kf, "[FILE"))
				m_section = SECTION_FILES;
			else if (tt::isSameSubStri(kf, "[OPTION"))
				m_section = SECTION_OPTIONS;
			continue;
		}

		switch (m_section) {
			case SECTION_UNKNOWN:
			default:
				break;

			case SECTION_OPTIONS:
				{
					size_t pos;
					for (pos = 0; aOptions[pos]; ++pos) {
						if (tt::isSameSubStri(kf, aOptions[pos]))
							break;
					}
					if (aOptions[pos]) {
						char* pszFile = tt::findChar(kf, '=');
						if (pszFile) {
							pszFile = tt::findNonSpace(pszFile + 1);
							if (*pszFile) {
								// [KeyWorksRW - 11-29-2018] I don't believe the HH compiler from MS supports comments after filenames, but
								// we can't be certain other compilers and/or authoring systems don't use them -- so remove any comment just
								// to be sure.

								char* pszComment = tt::findChar(pszFile, ';');
								if (pszComment)
									*pszComment = 0;
								tt::trimRight(pszFile);
								AddDependency(pszHHP, pszFile);
							}
						}
					}
					else if (tt::isSameSubStri(kf, "Compiled file")) {
						char* pszFile = tt::findChar(kf, '=');
						if (pszFile) {
							char* pszComment = tt::findChar(pszFile, ';');
							if (pszComment)
								*pszComment = 0;
							tt::trimRight(pszFile);
							m_cszChmFile = tt::findNonSpace(pszFile + 1);
						}
					}
				}
				break;

			case SECTION_ALIAS:
				{
					char* pszFile = tt::findChar(kf, ';');	// remove any comment
					if (pszFile) {
						*pszFile = 0;
						tt::trimRight(kf);
					}
					pszFile = tt::findChar(kf, '=');
					if (pszFile) {
						pszFile = tt::findSpace(pszFile + 1);
						if (*pszFile) {
							AddDependency(pszHHP, pszFile);
						}
					}
				}
				break;

			case SECTION_TEXT_POPUPS:
				{
					char* pszFile = tt::findChar(kf, ';');	// remove any comment
					if (pszFile) {
						*pszFile = 0;
						tt::trimRight(kf);
					}

					// This section may include .h files that are parsed, but not compiled, but we need them for dependency checking.

					AddDependency(pszHHP, kf);
				}
				break;

			case SECTION_FILES:
				{
					// [KeyWorksRW - 11-29-2018] I don't think HHC actually allows comments in the [FILES] section,
					// though it should. I'm supporting comments here just to be sure we can support other file formats
					// like YAML which do allow for comments

					char* pszFile = tt::findChar(kf, ';');	// remove any comment
					if (pszFile) {
						*pszFile = 0;
						tt::trimRight(kf);
					}

					// This section may include .h files that are parsed, but not compiled, but we need them for dependency checking.

					AddDependency(pszHHP, kf);
				}
				break;
		}
	}
}

void CParseHHP::AddDependency(const char* pszHHP, const char* pszFile)
{
	/*
		The problem we need to solve with this function is that filename locations in the .hhp file will be relative to the
		.hhp file -- which in turn can be in a different location. So for example, you might have the following:

			../Help/foo.hhp

		In foo.hhp, you might have:

			[FILES]
			html/bar.html

		We need to create a dependency to ../Help/html/bar.html
	*/

	ttCStr cszRelative;
	ttCStr cszHHP;

	if (!tt::isSameStri(pszHHP, m_cszHHPName)) {
		// If we're in a nested .HHP file, then we need to first get the location of the nested .HHP, use that to get the location
		// of the pszFile;

		tt::ConvertToRelative(m_cszRoot, pszHHP, cszHHP);
		cszHHP.getFullPathName();	// now that we have the location relative to our original .hhp file, convert it to a full path
		char* pszFilePortion = tt::findFilePortion(cszHHP);
		*pszFilePortion = 0;
		tt::ConvertToRelative(cszHHP, pszFile, cszRelative);
		cszHHP.AppendFileName(cszRelative);
		cszHHP.getFullPathName();
		tt::ConvertToRelative(m_cszCWD, cszHHP, cszRelative);
	}
	else {
		cszHHP = (char*) m_cszRoot;
		cszHHP.AppendFileName(pszFile);
		tt::ConvertToRelative(m_cszCWD, cszHHP, cszRelative);
	}

	if (tt::findChar(cszRelative, '*') || tt::findChar(cszRelative, '?')) {
		ttCFindFile ff(cszRelative);
		char* pszFilePortion = tt::findFilePortion(cszRelative);
		ptrdiff_t cFilePortion = (pszFilePortion - cszRelative.getPtr());
		if (ff.isValid()) {
			do {
				if (!ff.isDir()) {
					cszRelative.getPtr()[cFilePortion] = 0;
					cszRelative.AppendFileName(ff);
					m_lstDependencies += (const char*) cszRelative;
				}
			} while(ff.NextFile());
		}
		return;
	}

	// TODO: [KeyWorksRW - 11-29-2018]	If this is an HTML file, then we need to parse it to find additional dependencies

	m_lstDependencies += (const char*) cszRelative;
}

const char* txtHelpNinja = "build/ChmHelp.ninja";

bool CNinja::CreateHelpFile()
{
	if (tt::isEmpty(getHHPName()))
		return false;

	if (!tt::DirExists("build")) {
		if (!tt::CreateDir("build")) {
			AddError("Unable to create the build directory -- so no place to put the .ninja files!\n");
			return false;
		}
	}

	CParseHHP chhp(getHHPName());
	chhp.ParseHhpFile();	// this will figure out all of the dependencies
	m_cszChmFile = (const char*) chhp.m_cszChmFile;

	ttCFile file;
	file.SetUnixLF();	// WARNING!!! NINJA doesn't allow \r characters (or \t for that matter)
	file.printf("# WARNING: THIS FILE IS AUTO-GENERATED by %s. CHANGES YOU MAKE WILL BE LOST IF IT IS AUTO-GENERATED AGAIN.\n\n", txtVersion);
	file.WriteEol("ninja_required_version = 1.8\n");
	file.WriteEol("builddir = build\n");

	file.WriteEol("rule compile");
	file.WriteEol("  command = hhc.exe $in ");
	file.WriteEol("  description = compiling $out\n");

	file.printf("build %s : compile %s | $\n", (char*) m_cszChmFile, getHHPName());
	for (size_t pos = 0; pos < chhp.m_lstDependencies.GetCount() - 1; ++pos) {
		file.printf("  %s $\n", chhp.m_lstDependencies[pos]);
	}
	file.printf("  %s\n", chhp.m_lstDependencies[chhp.m_lstDependencies.GetCount() - 1]);	// last one doesn't have trailing '$' character

	// We don't want to touch the build script if it hasn't changed, since that would change the timestamp causing git
	// and other tools that check timestamp to think something is different.

	ttCFile fileOrg;
	if (fileOrg.ReadFile(txtHelpNinja)) {
		if (strcmp(fileOrg, file) == 0)
			return false;
		else if (dryrun.isEnabled()) {
			dryrun.NewFile(txtHelpNinja);
			dryrun.DisplayFileDiff(fileOrg, file);
			return false;	// we didn't actually write anything
		}
	}

	if (!file.WriteFile(txtHelpNinja)) {
		ttCStr cszMsg;
		cszMsg.printf("Cannot write to %s\n", txtHelpNinja);
		AddError(cszMsg);
		return false;
	}

	return true;
}
