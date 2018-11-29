/////////////////////////////////////////////////////////////////////////////
// Name:		CParseHHP
// Purpose:
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2018 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#include "../ttLib/include/keyfile.h"	// CKeyFile

#include "parsehhp.h"	// CParseHHP
#include "ninja.h"		// CNinja

CParseHHP::CParseHHP(const char* pszHHPName)
{
	m_section = SECTION_UNKNOWN;
	m_cszHHPName = pszHHPName;
	m_cszRoot = pszHHPName;
	m_cszRoot.GetFullPathName();
	char* pszFile = FindFilePortion(m_cszRoot);
	if (pszFile)
		*pszFile = 0;
	m_cszCWD.GetCWD();
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

	CKeyFile kf;
	if (!kf.ReadFile(pszHHP)) {
// REVIEW: [randalphwa - 11/29/2018] Need a way to add error msgs to CNinja
//		CStr cszMsg;
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

	while (kf.readline()) {
		if (isSameSubString(kf, "#include")) {
			char* pszFile = FindNonSpace(FindNextSpace(kf));
			if (!*pszFile)
				continue;	// invalid #include -- doesn't specify a filename
			CStr cszFile;
			if (*pszFile == CH_QUOTE) {
				cszFile.GetQuotedString(pszFile);
			}
			else
				cszFile = pszFile;

			// [KeyWorksRW - 11-29-2018] It's not clear from HH docs when comments are allowed on a line -- we remove it
			// here just in case.

			char* pszComment = kstrchr(cszFile, ';');
			if (pszComment)
				*pszComment = 0;
			trim(cszFile);

			// A couple of sections sometimes include .h files -- but we only care about an #included .hhp file

			if (kstristr(cszFile, ".hhp"))
				ParseHhpFile(cszFile);
			continue;
		}

		if (kf[0] == '[') {	// sections are placed in brackets
			m_section = SECTION_UNKNOWN;
			if (isSameSubString(kf, "[ALIAS"))
				m_section = SECTION_ALIAS;
			else if (isSameSubString(kf, "[FILE"))
				m_section = SECTION_FILES;
			else if (isSameSubString(kf, "[OPTION"))
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
						if (isSameSubString(kf, aOptions[pos]))
							break;
					}
					if (aOptions[pos]) {
						char* pszFile = kstrchr(kf, '=');
						if (pszFile) {
							pszFile = FindNonSpace(FindNextSpace(pszFile));
							if (*pszFile) {
								// [KeyWorksRW - 11-29-2018] I don't believe the HH compiler from MS supports comments after filenames, but
								// we can't be certain other compilers and/or authoring systems don't use them -- so remove any comment just
								// to be sure.

								char* pszComment = kstrchr(pszFile, ';');
								if (pszComment)
									*pszComment = 0;
								trim(pszFile);
								AddDependency(pszHHP, pszFile);
							}
						}
					}
				}
				break;

			case SECTION_ALIAS:
				{
					char* pszFile = kstrchr(kf, ';');	// remove any comment
					if (pszFile) {
						*pszFile = 0;
						trim(kf);
					}
					pszFile = kstrchr(kf, '=');
					if (pszFile) {
						pszFile = FindNonSpace(FindNextSpace(pszFile));
						if (*pszFile) {
							AddDependency(pszHHP, pszFile);
						}
					}
				}
				break;

			case SECTION_TEXT_POPUPS:
				{
					char* pszFile = kstrchr(kf, ';');	// remove any comment
					if (pszFile) {
						*pszFile = 0;
						trim(kf);
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

	CStr cszRelative;
	CStr cszHHP;

	if (!isSameString(pszHHP, m_cszHHPName)) {
		// If we're in a nested .HHP file, then we need to first get the location of the nested .HHP, use that to get the location
		// of the pszFile;

		ConvertToRelative(m_cszRoot, pszHHP, cszHHP);
		cszHHP.GetFullPathName();	// now that we have the location relative to our original .hhp file, convert it to a full path
		char* pszFilePortion = FindFilePortion(cszHHP);
		*pszFilePortion = 0;
		ConvertToRelative(cszHHP, pszFile, cszRelative);
		cszHHP += cszRelative;
		cszHHP.GetFullPathName();
		pszFile = cszHHP;
	}

	ConvertToRelative(m_cszCWD, pszFile, cszRelative);

	// TODO: [KeyWorksRW - 11-29-2018]	The filename might contain wildcards -- if so, we need to add all of them

	m_lstDependencies += (const char*) cszRelative;

	// TODO: [KeyWorksRW - 11-29-2018]	If this is an HTML file, then we need to parse it to find additional dependencies


}

const char* txtHelpNinja = "build/help.ninja";

bool CNinja::CreateHelpFile()
{
	if (IsEmptyString(getHHPName()))
		return false;

	if (!DirExists("build")) {
		if (!CreateDir("build")) {
			AddError("Unable to create the build directory -- so no place to put the .ninja files!\n");
			return false;
		}
	}

	CParseHHP chhp(getHHPName());
	chhp.ParseHhpFile();	// this will figure out all of the dependencies
	m_cszChmFile = (const char*) chhp.m_cszChmFile;

	CKeyFile file;
	file.SetUnixLF();	// WARNING!!! NINJA doesn't allow \r characters (or \t for that matter)
	file.printf("# WARNING: THIS FILE IS AUTO-GENERATED by %s. CHANGES YOU MAKE WILL BE LOST IF IT IS AUTO-GENERATED AGAIN.\n\n", txtVersion);
	file.WriteEol("ninja_required_version = 1.8\n");
	file.WriteEol("builddir = build\n");

	file.WriteEol("rule complile");
	file.WriteEol("  command = hhc.exe $in");
	file.WriteEol("  description = compiling $out");

	file.printf("build %s : compile %s $\n", (char*) m_cszChmFile, getHHPName());
	for (size_t pos = 0; pos < chhp.m_lstDependencies.GetCount(); ++pos) {
		file.printf("  %s $\n", chhp.m_lstDependencies[pos]);
	}

	// We don't want to touch the build script if it hasn't changed, since that would change the timestamp causing git
	// and other tools that check timestamp to think something is different.

	CKeyFile kfOrg;
	if (kfOrg.ReadFile(txtHelpNinja)) {
		if (strcmp(kfOrg, file) == 0)
			return false;
	}

	if (!file.WriteFile(txtHelpNinja)) {
		CStr cszMsg;
		cszMsg.printf("Cannot write to %s\n", txtHelpNinja);
		AddError(cszMsg);
		return false;
	}

	return true;
}
