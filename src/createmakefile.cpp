/////////////////////////////////////////////////////////////////////////////
// Name:		CreateMakeFile()
// Purpose:		Creates an MS nmake and GNU-make compatible makefile
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttfile.h> 	// ttCFile
#include <ttenumstr.h>	// ttCEnumStr
#include <ttfindfile.h> // ttCFindFile

#include "bldmaster.h"	// CBldMaster
#include "resource.h"	// IDR_MAKEFILE

static bool FindBuildSrc(const char* pszBuildTarget, ttCStr& cszDir);

extern const char* txtHelpNinja;

bool CBldMaster::CreateMakeFile()
{
	if (isMakeNever())
		return true;	// user doesn't want makefile created at all
	else if (isMakeMissing()) {
		if (tt::FileExists("makefile"))
			return true;	// file exists, user doesn't want us to update it
	}

	// We get here if MAKEMAKE_ALWAYS is set, or MAKEMAKE_MISSING is set and the makefile doesn't exist

	ttCFile kf;
	if (!kf.ReadResource(IDR_MAKEFILE)) {
		m_lstErrors += "MakeNinja.exe is corrupted -- unable to read the required resource for creating a makefile!";
		return false;
	}
	while (kf.ReplaceStr("%project%", GetProjectName()));

	// Now we parse the file as if we had read it, changing or adding as needed

	ttCFile kfOut;
	while (kf.ReadLine()) {
		if (tt::isSameSubStri(kf, "#b64")) {
			if (GetBoolOption(OPT_64BIT) && !GetBoolOption(OPT_BIT_SUFFIX))
				kfOut.WriteEol("b64=1");
			else
				kfOut.WriteEol(kf);
		}
		else if (tt::isSameSubStri(kf, "release:") || tt::isSameSubStri(kf, "debug:")) {
			bool bDebugTarget = tt::isSameSubStri(kf, "debug:");	// so we don't have to keep parsing the line
			ttCStr cszNewLine(kf);
			if (!tt::isEmpty(getHHPName())) {
				cszNewLine.ReplaceStr(" ", " ChmHelp ");
				if (!getBuildLibs()) {
					kfOut.WriteEol(cszNewLine);
					kfOut.printf("\nChmHelp:\n\tninja -f %s\n", txtHelpNinja);
				}
			}

			if (getBuildLibs()) {
				ttCEnumStr cEnumStr(getBuildLibs(), ';');
				const char* pszBuildLib;
				while (cEnumStr.Enum(&pszBuildLib)) {
					ttCStr cszTarget;
					cszTarget.printf(" %s%s ", tt::findFilePortion(pszBuildLib), bDebugTarget ? "D" : "");

					// Line is "release: project" so we simply replace the first space with our additional target (which begins and ends with a space

					cszNewLine.ReplaceStr(" ", cszTarget);
				}
				kfOut.WriteEol(cszNewLine);
				if (!tt::isEmpty(getHHPName())) {
					kfOut.printf("\nChmHelp:\n\tninja -f %s\n", txtHelpNinja);
				}

				// Now that we've added the targets to the release: or debug: line, we need to add the rule

				cEnumStr.SetNewStr(getBuildLibs(), ';');
				while (cEnumStr.Enum(&pszBuildLib)) {
					kfOut.printf("\n%s%s:\n", tt::findFilePortion(pszBuildLib), bDebugTarget ? "D" : "");
					ttCStr cszCWD;
					if (!FindBuildSrc(pszBuildLib, cszCWD)) {
						ttCStr cszMsg;
						cszMsg.printf("Unable to find build/ directory for %s library", tt::findFilePortion(pszBuildLib));
						m_lstErrors += cszMsg;
						cszCWD = pszBuildLib;
						char* pszLibName = tt::findFilePortion(cszCWD);
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

	if (tt::FileExists("makefile"))	{
		ttCFile kfOrg;
		if (!kfOrg.ReadFile("makefile") || strcmp(kfOrg, kfOut) != 0) {
			if (dryrun.isEnabled())	{
				dryrun.NewFile("makefile");
				dryrun.DisplayFileDiff(kfOrg, kfOut);
			}
			else if (kfOut.WriteFile("makefile")) {
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

static bool FindBuildSrc(const char* pszBuildTarget, ttCStr& cszDir)
{
	ttASSERT_MSG(pszBuildTarget, "NULL pointer!");
	ttASSERT_MSG(*pszBuildTarget, "Empty string!");

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
	char* pszName = tt::findFilePortion(cszDir);
	if (!pszName)
		return false;	// means a bogus BuildLibs: line in .srcfiles
	ttCStr cszName(pszName);

	if (pszName > cszDir.getPtr())
		--pszName;		// so that we also remove any trailing backslash
	*pszName = 0;
	pszName = tt::findFilePortion(cszDir);
	if (pszName && tt::isSameStri(pszName, "lib")) {
		*pszName = 0;

		// If BuildTarget is "../lib/name" then removing "lib" will leave us with "../" which is unlikely to be what the
		// user wants, so add the name to the end. I.e., "../lib/name" becomes "../name", "../../lib/name" becomes
		// "../../name", etc.

		tt::BackslashToForwardslash(cszDir);
		pszName = tt::findStr(cszDir, "../");
		while (pszName && tt::isSameSubStri(pszName + 3, "../"))
			pszName += 3;
		if (pszName && !pszName[3])		// catch case where removing "lib" leaves "../ "
			cszDir.AppendFileName(cszName);
	}

	ttCStr cszPattern((const char*) cszDir);
	cszPattern.AppendFileName("*");
	ttCFindFile ff(cszPattern);
	if (ff.isValid()) {
		do {
			if (ff.isDir() && tt::isValidFileChar(ff, 0)) {
				if (tt::isSameStri(ff, "build"))
					return true;	// we found it, presumably after "lib" was removed from the path
				ttCStr cszPossible((const char*) cszDir);
				cszPossible.AppendFileName(ff);
				cszPossible.AddTrailingSlash();
				cszPossible.AppendFileName("build");
				if (tt::DirExists(cszPossible))	{	// found it! (we hope)
					cszDir.AppendFileName(ff);
					return true;
				}
			}
		} while(ff.NextFile());
	}
	return false;
}
