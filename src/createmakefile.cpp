/////////////////////////////////////////////////////////////////////////////
// Name:      CreateMakeFile()
// Purpose:   Creates an MS nmake and GNU-make compatible makefile
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttfile.h>     // ttCFile
#include <ttenumstr.h>  // ttCEnumStr
#include <ttfindfile.h> // ttCFindFile

#include "bldmaster.h"  // CBldMaster
#include "resource.h"   // IDR_MAKEFILE

static bool FindBuildSrc(const char* pszBuildTarget, ttCStr& cszDir);

extern const char* txtHelpNinja;

bool CBldMaster::CreateMakeFile()
{
    if (IsMakeNever())
        return true;    // user doesn't want makefile created at all
    else if (IsMakeMissing()) {
        if (ttFileExists("makefile"))
            return true;    // file exists, user doesn't want us to update it
    }

    // We get here if MAKEMAKE_ALWAYS is set, or MAKEMAKE_MISSING is set and the makefile doesn't exist

    ttCFile kf;
    if (!kf.ReadResource(m_bPrivateBuild ? IDR_PRIVATE_MAKEFILE : IDR_MAKEFILE)) {
        m_lstErrors += "MakeNinja.exe is corrupted -- unable to read the required resource for creating a makefile!";
        return false;
    }

    // .private/.srcfiles might specify a new project name to be used for the executable name, but we don't need that new name in the makefile

    if (m_cszOrgProjName.IsNonEmpty())
        while (kf.ReplaceStr("%project%", m_cszOrgProjName));
    else
        while (kf.ReplaceStr("%project%", GetProjectName()));

    // Now we parse the file as if we had read it, changing or adding as needed

    ttCFile kfOut;
    while (kf.ReadLine()) {
        if (ttIsSameSubStrI(kf, "#b64")) {
            if (GetBoolOption(OPT_64BIT) && !GetBoolOption(OPT_64BIT_SUFFIX))
                kfOut.WriteEol("b64=1");
            else
                kfOut.WriteEol(kf);
        }
        else if (ttIsSameSubStrI(kf, "release:") || ttIsSameSubStrI(kf, "debug:")) {
            bool bDebugTarget = ttIsSameSubStrI(kf, "debug:");  // so we don't have to keep parsing the line
            ttCStr cszNewLine(kf);
            if (!ttIsEmpty(GetHHPName())) {
                cszNewLine.ReplaceStr(" ", " ChmHelp ");
                if (!GetBuildLibs()) {
                    kfOut.WriteEol(cszNewLine);
                    kfOut.printf("\nChmHelp:\n\tninja -f %s\n", txtHelpNinja);
                    continue;
                }
            }

            if (GetBuildLibs()) {
                ttCEnumStr cEnumStr(GetBuildLibs(), ';');
                const char* pszBuildLib;
                ttCStr cszSaveCwd;
                cszSaveCwd.GetCWD();

                while (cEnumStr.Enum(&pszBuildLib)) {
                    ttChDir(pszBuildLib);
                    CSrcFiles cSrcFiles;
                    if (cSrcFiles.ReadFile())
                    {
                        ttCStr cszTarget;
                        cszTarget.printf(" %s%s ", cSrcFiles.GetProjectName(), bDebugTarget ? "D" : "");

                        // Line is "release: project" so we simply replace the first space with our additional target (which begins and ends with a space

                        cszNewLine.ReplaceStr(" ", cszTarget);
                    }
                    else
                    {
                        ttCStr cszTarget;
                        cszTarget.printf(" %s%s ", ttFindFilePortion(pszBuildLib), bDebugTarget ? "D" : "");

                        // Line is "release: project" so we simply replace the first space with our additional target (which begins and ends with a space

                        cszNewLine.ReplaceStr(" ", cszTarget);
                    }
                }
                kfOut.WriteEol(cszNewLine);
                if (!ttIsEmpty(GetHHPName())) {
                    kfOut.printf("\nChmHelp:\n\tninja -f %s\n", txtHelpNinja);
                }

                // Now that we've added the targets to the release: or debug: line, we need to add the rule

                cEnumStr.SetNewStr(GetBuildLibs(), ';');
                while (cEnumStr.Enum(&pszBuildLib)) {
                    CSrcFiles cSrcFiles;
                    ttChDir(pszBuildLib);
                    if (cSrcFiles.ReadFile())
                    {
                        // .srcfiles can either be in the same directory as the .ninja scripts, or up one directory

                        kfOut.printf("\n%s%s:\n", cSrcFiles.GetProjectName(), bDebugTarget ? "D" : "");
                        ttCStr cszBuild, cszCD;
                        ttCStr cszDir(cSrcFiles.m_cszSrcFilePath);
                        cszDir.GetFullPathName();
                        char* pszFile = ttFindFilePortion(cszDir);
                        *pszFile = 0;
                        ttCStr cszRoot(cszDir);  // this is now the root directory where .srcfiles was found
                        cszDir += cSrcFiles.GetBuildScriptDir();    // this is now where the ninja scripts are located
                        ttConvertToRelative(cszRoot, cszDir, cszBuild);
                        cszDir = cSrcFiles.m_cszSrcFilePath;
                        cszDir.GetFullPathName();
                        ttConvertToRelative(cszSaveCwd, cszDir, cszCD);     // figure out how to get to the directory to build in
                        pszFile = ttFindFilePortion(cszCD);
                        *pszFile = 0;
                        kfOut.printf("\tcd %s & ninja -f %s/$(cmplr)Build$(bits)%s.ninja\n", (char*) cszCD, (char*) cszBuild,
                                                                                bDebugTarget ? "D" : "");
                    }
                    else
                    {
                        kfOut.printf("\n%s%s:\n", ttFindFilePortion(pszBuildLib), bDebugTarget ? "D" : "");
                        ttCStr cszCWD;
                        if (!FindBuildSrc(pszBuildLib, cszCWD)) {
                            ttCStr cszMsg;
                            cszMsg.printf("Unable to find build/ directory for %s library", ttFindFilePortion(pszBuildLib));
                            m_lstErrors += cszMsg;
                            cszCWD = pszBuildLib;
                            char* pszLibName = ttFindFilePortion(cszCWD);
                            if (pszLibName)
                                *pszLibName = 0;
                        }
                        kfOut.printf("\tcd %s & ninja -f build/$(cmplr)Build$(bits)%s.ninja\n", (const char*) cszCWD,
                                                                                    bDebugTarget ? "D" : "");

                    }
                }
                ttChDir(cszSaveCwd);
            }
            else
                kfOut.WriteEol(kf);
        }
        else
            kfOut.WriteEol(kf);
    }

    // If the makefile already exists, don't write to it unless something has actually changed

    if (ttFileExists("makefile"))   {
        ttCFile kfOrg;
        if (!kfOrg.ReadFile("makefile") || strcmp(kfOrg, kfOut) != 0) {
            if (dryrun.IsEnabled()) {
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
    char* pszName = ttFindFilePortion(cszDir);
    if (!pszName)
        return false;   // means a bogus BuildLibs: line in .srcfiles
    ttCStr cszName(pszName);

    if (pszName > cszDir.GetPtr())
        --pszName;      // so that we also remove any trailing backslash
    *pszName = 0;
    pszName = ttFindFilePortion(cszDir);
    if (pszName && ttIsSameStrI(pszName, "lib")) {
        *pszName = 0;

        // If BuildTarget is "../lib/name" then removing "lib" will leave us with "../" which is unlikely to be what the
        // user wants, so add the name to the end. I.e., "../lib/name" becomes "../name", "../../lib/name" becomes
        // "../../name", etc.

        ttBackslashToForwardslash(cszDir);
        pszName = ttStrStr(cszDir, "../");
        while (pszName && ttIsSameSubStrI(pszName + 3, "../"))
            pszName += 3;
        if (pszName && !pszName[3])     // catch case where removing "lib" leaves "../ "
            cszDir.AppendFileName(cszName);
    }

    ttCStr cszPattern((const char*) cszDir);
    cszPattern.AppendFileName("*");
    ttCFindFile ff(cszPattern);
    if (ff.IsValid()) {
        do {
            if (ff.IsDir() && ttIsValidFileChar(ff, 0)) {
                if (ttIsSameStrI(ff, "build"))
                    return true;    // we found it, presumably after "lib" was removed from the path
                ttCStr cszPossible((const char*) cszDir);
                cszPossible.AppendFileName(ff);
                cszPossible.AddTrailingSlash();
                cszPossible.AppendFileName("build");
                if (ttDirExists(cszPossible))   {   // found it! (we hope)
                    cszDir.AppendFileName(ff);
                    return true;
                }
            }
        } while(ff.NextFile());
    }
    return false;
}
