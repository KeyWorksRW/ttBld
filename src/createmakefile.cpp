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

extern const char* txtHelpNinja;

bool CBldMaster::CreateMakeFile()
{
    if (IsMakeNever())
        return true;        // user doesn't want makefile created at all
    else if (IsMakeMissing())
    {
        if (ttFileExists("makefile"))
            return true;    // file exists, user doesn't want us to update it
    }

    // We get here if the makefile is missing or "Makefile: always" is set

    ttCFile kf;
    if (!kf.ReadResource(IDR_PRIVATE_MAKEFILE))
    {
        // TRANSLATORS: Don't change the filename "makefile"
        m_lstErrors += TRANSLATE("MakeNinja.exe is corrupted -- unable to read the required resource for creating a makefile,");
        return false;
    }

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

            if (GetBuildLibs())
            {
                ttCEnumStr cEnumStr(GetBuildLibs(), ';');

                while (cEnumStr.Enum())
                {
                    ttCStr cszTarget;
                    cszTarget.printf(" %s%s ", ttFindFilePortion(cEnumStr), bDebugTarget ? "D" : "");

                    // Line is "release: project" so we simply replace the first space with our additional target (which
                    // begins and ends with a space)

                    cszNewLine.ReplaceStr(" ", cszTarget);
                }
                kfOut.WriteEol(cszNewLine);

                if (!ttIsEmpty(GetHHPName()))
                    kfOut.printf("\nChmHelp:\n\tninja -f %s\n", txtHelpNinja);

                // Now that we've added the targets to the release: or debug: line, we need to add the rule

                cEnumStr.SetNewStr(GetBuildLibs(), ';');
                while (cEnumStr.Enum())
                {
                    kfOut.printf("\n%s%s:\n", ttFindFilePortion(cEnumStr), bDebugTarget ? "D" : "");    // the rule

                    // The leading \t before the command is required or make will fail
                    kfOut.printf("\tcd %s & ninja -f $(BldScript%s)\n", (const char*) cEnumStr, bDebugTarget ? "D" : "");
                }
            }
            else
                kfOut.WriteEol(kf);
        }
        else
            kfOut.WriteEol(kf);
    }

    // If the makefile already exists, don't write to it unless something has actually changed

    if (ttFileExists("makefile"))
    {
        ttCFile kfOrg;
        if (!kfOrg.ReadFile("makefile") || strcmp(kfOrg, kfOut) != 0)
        {
            if (m_dryrun.IsEnabled())
            {
                m_dryrun.NewFile("makefile");
                m_dryrun.DisplayFileDiff(kfOrg, kfOut);
            }
            else if (kfOut.WriteFile("makefile"))
            {
                // TRANSLATORS: Don't change the filename "makefile"
                puts(TRANSLATE("makefile updated"));
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
        if (kfOut.WriteFile("makefile"))
        {
            // TRANSLATORS: Don't change the filename "makefile"
            puts(TRANSLATE("makefile created"));
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
