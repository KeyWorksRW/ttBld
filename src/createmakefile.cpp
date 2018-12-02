/////////////////////////////////////////////////////////////////////////////
// Name:		CreateMakeFile()
// Purpose:		Creates an MS nmake and GNU-make compatible makefile
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2018 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#include "../ttLib/include/keyfile.h"	// CKeyFile
#include "../ttLib/include/enumstr.h"	// CEnumStr
#include "../ttLib/include/findfile.h"	// CTTFindFile

#include "bldmaster.h"	// CBldMaster
#include "resource.h"	// IDR_MAKEFILE

static bool FindBuildSrc(const char* pszBuildTarget, CStr& cszDir);
extern const char* txtHelpNinja;

bool CBldMaster::CreateMakeFile()
{
	if (m_fCreateMakefile == MAKEMAKE_NEVER)
		return true;	// user doesn't want makefile created at all
	else if (m_fCreateMakefile == MAKEMAKE_MISSING) {
		if (FileExists("makefile"))
			return true;	// file exists, user doesn't want us to update it
	}

	// We get here if MAKEMAKE_ALWAYS is set, or MAKEMAKE_MISSING is set and the makefile doesn't exist

	CKeyFile kf;
	if (!kf.ReadResource(IDR_MAKEFILE)) {
		m_lstErrors += "MakeNinja.exe is corrupted -- unable to read the required resource for creating a makefile!";
		return false;
	}
	while (kf.ReplaceStr("%project%", m_cszProjectName));

	// Now we parse the file as if we had read it, changing or adding as needed

	CKeyFile kfOut;
	while (kf.readline()) {
		if (isSameSubString(kf, "#b64")) {
			if (m_b64bit && !m_bBitSuffix)
				kfOut.WriteEol("b64=1");
			else
				kfOut.WriteEol(kf);
		}
		else if (isSameSubString(kf, "release:") || isSameSubString(kf, "debug:")) {
			bool bDebugTarget = isSameSubString(kf, "debug:");	// so we don't have to keep parsing the line
			CStr cszNewLine(kf);
			if (!IsEmptyString(getHHPName())) {
				cszNewLine.ReplaceStr(" ", " ChmHelp ");
				if (m_cszBuildLibs.IsEmpty()) {
					kfOut.WriteEol(cszNewLine);
					kfOut.printf("\nChmHelp:\n\tninja -f %s\n", txtHelpNinja);
				}
			}

			if (m_cszBuildLibs.IsNonEmpty()) {
				CEnumStr cEnumStr(m_cszBuildLibs, ';');
				const char* pszBuildLib;
				while (cEnumStr.Enum(&pszBuildLib)) {
					CStr cszTarget;
					cszTarget.printf(" %s%s ", FindFilePortion(pszBuildLib), bDebugTarget ? "D" : "");

					// Line is "release: project" so we simply replace the first space with our additional target (which begins and ends with a space

					cszNewLine.ReplaceStr(" ", cszTarget);
				}
				kfOut.WriteEol(cszNewLine);
				if (!IsEmptyString(getHHPName())) {
					kfOut.printf("\nChmHelp:\n\tninja -f %s\n", txtHelpNinja);
				}

				// Now that we've added the targets to the release: or debug: line, we need to add the rule

				cEnumStr.SetNewStr(m_cszBuildLibs, ';');
				while (cEnumStr.Enum(&pszBuildLib)) {
					kfOut.printf("\n%s%s:\n", FindFilePortion(pszBuildLib), bDebugTarget ? "D" : "");
					CStr cszCWD;
					if (!FindBuildSrc(pszBuildLib, cszCWD)) {
						CStr cszMsg;
						cszMsg.printf("Unable to find build/ directory for %s library", FindFilePortion(pszBuildLib));
						m_lstErrors += cszMsg;
						cszCWD = pszBuildLib;
						char* pszLibName = FindFilePortion(cszCWD);
						if (pszLibName)
							*pszLibName = 0;
					}
					kfOut.printf("\tcd %s & ninja -f build/$(cmplr)Build$(bits)%s.ninja\n", (const char*) cszCWD,
																				bDebugTarget ? "D" : "");
				}
			}
			else
				kfOut.WriteEol(kf);
		}
		else
			kfOut.WriteEol(kf);
	}

	// If the makefile already exists, don't write to it unless something has actually changed

	if (FileExists("makefile"))	{
		CKeyFile kfOrg;
		if (!kfOrg.ReadFile("makefile") || strcmp(kfOrg, kfOut) != 0) {
			if (kfOut.WriteFile("makefile")) {
				puts("makefile updated");
				return true;
			}
			else {
				puts("unable to write to makefile");
				return false;
			}
		}
	}
	else {
		if (kfOut.WriteFile("makefile")) {
			puts("makefile created");
			return true;
		}
		else {
			puts("unable to write to makefile");
			return false;
		}
	}

	return true;
}

static bool FindBuildSrc(const char* pszBuildTarget, CStr& cszDir)
{
	ASSERT_MSG(pszBuildTarget, "NULL pointer!");
	ASSERT_MSG(*pszBuildTarget, "Empty string!");

	/*
	 * Typical BuildLib: target name will be "../dir/lib/name". We assume that "name" is both the name of the library We
	 * start by extracting the "name" portion. If the name is preceeded by "lib", we remove that. That should leave us
	 * with "../dir/". We then look at every sub-directory underneath "../dir" until we find a "build/" sub-directory.
	 * Once we find that, we use it's parent as the location of the source files.

	 * There are lots of ways this can get an incorrect directory. If the user specified "lib/name" then the src files
	 * won't be found. If there are multiple sub-projects under "../dir/" then there's a possibility that the wrong one
	 * will be used. If the src files are two sub-directories in ("../dir/mylib/src") then it won't be found. The
	 * assumption is that a broken makefile that the user can easily edit and fix is better then no makefile at all.

	 */

	cszDir = pszBuildTarget;
	char* pszName = FindFilePortion(cszDir);
	if (!pszName)
		return false;	// means a bogus BuildLibs: line in .srcfiles
	CStr cszName(pszName);

	if (pszName > cszDir.getptr())
		--pszName;		// so that we also remove any trailing backslash
	*pszName = 0;
	pszName = FindFilePortion(cszDir);
	if (pszName && isSameString(pszName, "lib")) {
		*pszName = 0;

		// If BuildTarget is "../lib/name" then removing "lib" will leave us with "../" which is unlikely to be what the
		// user wants, so add the name to the end. I.e., "../lib/name" becomes "../name", "../../lib/name" becomes
		// "../../name", etc.

		BackslashToForwardslash(cszDir);
		pszName = kstrstr(cszDir, "../");
		while (pszName && isSameSubString(pszName + 3, "../"))
			pszName += 3;
		if (pszName && !pszName[3])		// catch case where removing "lib" leaves "../ "
			cszDir.AppendFileName(cszName);
	}

	CStr cszPattern((const char*) cszDir);
	cszPattern.AppendFileName("*");
	CTTFindFile ff(cszPattern);
	if (ff.isValid()) {
		do {
			if (ff.isDir() && IsValidFileChar(ff, 0)) {
				if (isSameString(ff, "build"))
					return true;	// we found it, presumably after "lib" was removed from the path
				CStr cszPossible((const char*) cszDir);
				cszPossible.AppendFileName(ff);
				cszPossible.AddTrailingSlash();
				cszPossible.AppendFileName("build");
				if (DirExists(cszPossible))	{	// found it! (we hope)
					cszDir.AppendFileName(ff);
					return true;
				}
			}
		} while(ff.NextFile());
	}
	return false;
}
