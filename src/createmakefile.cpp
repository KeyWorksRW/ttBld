/////////////////////////////////////////////////////////////////////////////
// Name:      CreateMakeFile()
// Purpose:   Creates an MS nmake and GNU-make compatible makefile
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttfile.h>      // ttCFile
#include <ttenumstr.h>   // ttCEnumStr
#include <ttfindfile.h>  // ttCFindFile

#include "ninja.h"     // CNinja
#include "resource.h"  // IDR_MAKEFILE
#include "funcs.h"     // List of function declarations

extern const char* txtHelpNinja;

bool CNinja::CreateMakeFile(bool bAllVersion, const char* pszDir)
{
    ttCStr cszBuildRoot;
    cszBuildRoot = pszDir ? pszDir : "";

    ttCStr cszMakeFile(cszBuildRoot);
    cszMakeFile.AppendFileName("makefile");

    if (ttFileExists(cszMakeFile))
        return true;  // Don't overwrite an existing makefile

    // We get here if the makefile is missing

    ttCStr cszBuildDir(cszBuildRoot);
    // Some repositories use a bld directory which is added to .gitignore. If it exists, we'll use this directory
    cszBuildDir.AppendFileName("bld");
    if (!ttDirExists(cszBuildDir))
    {
        cszBuildDir = cszBuildRoot;
        cszBuildDir.AppendFileName("build");
    }

    ttCStr cszSrcFiles;
    if (pszDir)
    {
        cszSrcFiles = pszDir;
        if (!LocateSrcFiles(&cszSrcFiles))
        {
            cszSrcFiles.printf(TRANSLATE("Cannot locate .srcfiles.yaml in the %s directory. Makefile not created."),
                               pszDir);
            m_lstErrors += cszSrcFiles;
            return false;
        }
    }
    else
    {
        const char* pszSrcFiles = LocateSrcFiles();  // it's fine if pszDir is a nullptr
        if (!pszSrcFiles)
            bAllVersion = true;  // can't create a dependency on .srcfiles.yaml if we don't know where it is
        else
            cszSrcFiles = pszSrcFiles;
    }

    ttCFile kf;
    if (!kf.ReadResource(bAllVersion ? IDR_MAKEFILE_ALL : IDR_MAKEFILE_SINGLE))
    {
        // TRANSLATORS: Don't change the filename "makefile"
        m_lstErrors +=
            TRANSLATE("ttBld.exe is corrupted -- unable to read the required resource for creating a makefile,");
        return false;
    }

    while (kf.ReplaceStr("%build%", cszBuildDir))
        ;
    //    while (kf.ReplaceStr("%libbuild%", cszBuildDir));

    if (!bAllVersion && cszSrcFiles.IsNonEmpty())
        while (kf.ReplaceStr("%srcfiles%", cszSrcFiles))
            ;

    while (kf.ReplaceStr("%project%", GetProjectName()))
        ;

    // Now we parse the file as if we had read it, changing or adding as needed

    ttCFile kfOut;
    while (kf.ReadLine())
    {
        // Set b32=1 if this is a 32-bit only target
        if (ttIsSameSubStrI(kf, "#b32=1"))
        {
            if (GetBoolOption(OPT_32BIT) && !GetBoolOption(OPT_64BIT))
                kfOut.WriteEol("b32=1");
            else
                kfOut.WriteEol(kf);
        }
        else if (ttIsSameSubStrI(kf, "release:") || ttIsSameSubStrI(kf, "debug:"))
        {
            bool   bDebugTarget = ttIsSameSubStrI(kf, "debug:");  // so we don't have to keep parsing the line
            ttCStr cszNewLine(kf);
            if (!ttIsEmpty(GetHHPName()))
            {
                cszNewLine.ReplaceStr(" ", " ChmHelp ");
                if (!GetBuildLibs())
                {
                    kfOut.WriteEol(cszNewLine);
                    kfOut.printf("\nChmHelp:\n\tninja -f %s\n", txtHelpNinja);
                    continue;
                }
            }

            if (m_dlstTargetDir.GetCount())
            {
                for (size_t pos = 0; m_dlstTargetDir.InRange(pos); ++pos)
                {
                    ttCStr cszTarget;
                    cszTarget.printf(" %s%s ", m_dlstTargetDir.GetKeyAt(pos), bDebugTarget ? "D" : "");

                    // Line is "release: project" so we simply replace the first space with our additional target (which
                    // begins and ends with a space)

                    cszNewLine.ReplaceStr(" ", cszTarget);
                }
                kfOut.WriteEol(cszNewLine);

                if (!ttIsEmpty(GetHHPName()))
                    kfOut.printf("\nChmHelp:\n\tninja -f %s\n", txtHelpNinja);

                // Now that we've added the targets to the release: or debug: line, we need to add the rule

                for (size_t pos = 0; m_dlstTargetDir.InRange(pos); ++pos)
                {
                    kfOut.printf("\n%s%s:\n", m_dlstTargetDir.GetKeyAt(pos), bDebugTarget ? "D" : "");  // the rule

                    // m_dlstTargetDir contains the root directory. We use that to locate .srcfiles.yaml which is the
                    // directory we need to change to in order to build the library.
                    ttCStr cszBuild(m_dlstTargetDir.GetValAt(pos));
                    LocateSrcFiles(&cszBuild);
                    char* pszFile = ttFindFilePortion(cszBuild);
                    if (pszFile && ttIsSameSubStrI(pszFile, ".srcfiles"))
                        pszFile[-1] = 0;
                    // The leading \t before the command is required or make will fail
                    kfOut.printf("\tcd %s & ninja -f $(BldScript%s)\n", (char*) cszBuild, bDebugTarget ? "D" : "");
                }
            }
            else
                kfOut.WriteEol(kf);
        }
        else
            kfOut.WriteEol(kf);
    }

    // If the makefile already exists, don't write to it unless something has actually changed

    if (ttFileExists(cszMakeFile))
    {
        ttCFile kfOrg;
        if (!kfOrg.ReadFile(cszMakeFile) || strcmp(kfOrg, kfOut) != 0)
        {
            if (m_dryrun.IsEnabled())
            {
                m_dryrun.NewFile(cszMakeFile);
                m_dryrun.DisplayFileDiff(kfOrg, kfOut);
            }
            else if (kfOut.WriteFile(cszMakeFile))
            {
                printf(GETSTRING(IDS_FILE_UPDATED), (char*) cszMakeFile);
                return true;
            }
            else
            {
                // TRANSLATORS: Don't change the filename "makefile"
                puts(GETSTRING(IDS_CANT_WRITE_MAKEFILE));
                return false;
            }
        }
    }
    else
    {
        if (kfOut.WriteFile(cszMakeFile))
        {
            // TRANSLATORS: Don't change the filename "makefile"
            printf(GETSTRING(IDS_FILE_CREATED), (char*) cszMakeFile);
            return true;
        }
        else
        {
            // TRANSLATORS: Don't change the filename "makefile"
            puts(GETSTRING(IDS_CANT_WRITE_MAKEFILE));
            return false;
        }
    }

    return true;
}
