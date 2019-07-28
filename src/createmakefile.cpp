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

#include "ninja.h"      // CNinja
#include "resource.h"   // IDR_MAKEFILE

extern const char* txtHelpNinja;

bool CNinja::CreateMakeFile()
{
    ttCStr cszMakeFile(IsVsCodeDir() ? ".vscode/makefile" : "makefile");

    if (IsMakeNever())
        return true;        // user doesn't want makefile created at all
    else if (IsMakeMissing())
    {
        if (ttFileExists(cszMakeFile))
            return true;    // file exists, user doesn't want us to update it
    }

    // We get here if the makefile is missing or "Makefile: always" is set

    bool bMinGW = FindFileEnv("PATH", "mingw32-make.exe");

    ttCFile kf;
    if (!kf.ReadResource(bMinGW & IsVsCodeDir() ? IDR_VSCODE_MAKE :  IDR_PRIVATE_MAKEFILE))
    {
        // TRANSLATORS: Don't change the filename "makefile"
        m_lstErrors += TRANSLATE("MakeNinja.exe is corrupted -- unable to read the required resource for creating a makefile,");
        return false;
    }

    while (kf.ReplaceStr("%build%", IsVsCodeDir() ? ".vscode/build" : "build"));
    while (kf.ReplaceStr("%libbuild%", "build"));
    if (IsVsCodeDir() && !ttFileExists(".srcfiles"))
        while (kf.ReplaceStr(".srcfiles", ""));

    // .private/.srcfiles might specify a new project name to be used for the executable name, but we don't need that new
    // name in the makefile

    if (m_cszOrgProjName.IsNonEmpty())
        while (kf.ReplaceStr("%project%", m_cszOrgProjName));
    else
        while (kf.ReplaceStr("%project%", GetProjectName()));

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
            bool bDebugTarget = ttIsSameSubStrI(kf, "debug:");  // so we don't have to keep parsing the line
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
                    kfOut.printf("\n%s%s:\n", m_dlstTargetDir.GetKeyAt(pos), bDebugTarget ? "D" : "");    // the rule
                    ttCStr cszBuild(m_dlstTargetDir.GetValAt(pos));
                    cszBuild.AppendFileName(".vscode/makefile");
                    // The leading \t before the command is required or make will fail
                    if (ttFileExists(cszBuild))
                        kfOut.printf("\tcd %s & ninja -f $(BldScript%s)\n", m_dlstTargetDir.GetValAt(pos), bDebugTarget ? "D" : "");
                    else
                        kfOut.printf("\tcd %s & ninja -f $(LibBldScript%s)\n", m_dlstTargetDir.GetValAt(pos), bDebugTarget ? "D" : "");
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
