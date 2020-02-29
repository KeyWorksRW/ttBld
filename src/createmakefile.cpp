/////////////////////////////////////////////////////////////////////////////
// Name:      CreateMakeFile()
// Purpose:   Creates an MS nmake and GNU-make compatible makefile
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttTR.h>  // Function for translating strings

#include <ttfile.h>      // ttCFile
#include <ttenumstr.h>   // ttEnumStr, ttEnumView -- Enumerate through substrings in a string
#include <ttfindfile.h>  // ttCFindFile

#include "ninja.h"     // CNinja
#include "resource.h"  // IDR_MAKEFILE
#include "funcs.h"     // List of function declarations

extern const char* txtHelpNinja;

bool CNinja::CreateMakeFile(bool bAllVersion, std::string_view Dir)
{
    ttString BuildRoot(Dir);

    ttString MakeFile(BuildRoot);
    MakeFile.append_filename("makefile");

    if (MakeFile.fileExists())
        return true;  // Don't overwrite an existing makefile

    // We get here if the makefile is missing

    ttString BuildDir(BuildRoot);
    // Some repositories use a bld directory which is added to .gitignore. If it exists, we'll use this directory
    BuildDir.append_filename("bld");

    auto pProjectFile = locateProjectFile(Dir);
    if (pProjectFile->empty())
    {
        m_lstErrMessages += _tt("Cannot locate .srcfiles.yaml. Makefile not created.");
        return false;
    }

    ttCFile kf;
    if (!kf.ReadResource(bAllVersion ? IDR_MAKEFILE_ALL : IDR_MAKEFILE_SINGLE))
    {
        // TRANSLATORS: Don't change the filename "makefile"
        m_lstErrMessages +=
            _tt("ttBld.exe is corrupted -- unable to read the required resource for creating a makefile,");
        return false;
    }

    while (kf.ReplaceStr("%build%", BuildDir.c_str()))
        ;
    //    while (kf.ReplaceStr("%libbuild%", BuildDir));

    if (!bAllVersion)
        while (kf.ReplaceStr("%srcfiles%", pProjectFile->c_str()))
            ;

    while (kf.ReplaceStr("%project%", GetProjectName()))
        ;

    // Now we parse the file as if we had read it, changing or adding as needed

    ttCFile kfOut;
    while (kf.ReadLine())
    {
        if (ttIsSameSubStrI(kf, "release:") || ttIsSameSubStrI(kf, "debug:"))
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

                    // Line is "release: project" so we simply replace the first space with our additional target
                    // (which begins and ends with a space)

                    cszNewLine.ReplaceStr(" ", cszTarget);
                }
                kfOut.WriteEol(cszNewLine);

                if (!ttIsEmpty(GetHHPName()))
                    kfOut.printf("\nChmHelp:\n\tninja -f %s\n", txtHelpNinja);

                // Now that we've added the targets to the release: or debug: line, we need to add the rule

                for (size_t pos = 0; m_dlstTargetDir.InRange(pos); ++pos)
                {
                    kfOut.printf("\n%s%s:\n", m_dlstTargetDir.GetKeyAt(pos), bDebugTarget ? "D" : "");  // the rule

                    // m_dlstTargetDir contains the root directory. We use that to locate .srcfiles.yaml which is
                    // the directory we need to change to in order to build the library.
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

    if (MakeFile.fileExists())
    {
        ttCFile kfOrg;
        if (!kfOrg.ReadFile(MakeFile.c_str()) || strcmp(kfOrg, kfOut) != 0)
        {
            if (m_dryrun.IsEnabled())
            {
                m_dryrun.NewFile(MakeFile.c_str());
                m_dryrun.DisplayFileDiff(kfOrg, kfOut);
            }
            else if (kfOut.WriteFile(MakeFile.c_str()))
            {
                printf(_tt("%s updated.\n"), (char*) MakeFile.c_str());
                return true;
            }
            else
            {
                // TRANSLATORS: Don't change the filename "makefile"
                std::cout << _tt("Unable to write to makefile.") << '\n';
                return false;
            }
        }
    }
    else
    {
        if (kfOut.WriteFile(MakeFile.c_str()))
        {
            // TRANSLATORS: Don't change the filename "makefile"
            std::cout << MakeFile << _tt(" created.") << '\n';
            return true;
        }
        else
        {
            // TRANSLATORS: Don't change the filename "makefile"
            std::cout << _tt("Unable to write to makefile.") << '\n';
            return false;
        }
    }

    return true;
}
