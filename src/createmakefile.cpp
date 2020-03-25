/////////////////////////////////////////////////////////////////////////////
// Name:      CreateMakeFile()
// Purpose:   Creates an MS nmake and GNU-make compatible makefile
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttcstr.h>     // Classes for handling zero-terminated char strings.
#include <ttenumstr.h>  // ttlib::enumstr, ttEnumView -- Enumerate through substrings in a string

#include "ninja.h"     // CNinja
#include "resource.h"  // IDR_MAKEFILE

extern const char* txtHelpNinja;

bool CNinja::CreateMakeFile(bool isAllVersion, std::string_view Dir)
{
    ttlib::cstr BuildRoot(Dir);

    ttlib::cstr MakeFile(BuildRoot);
    MakeFile.append_filename("makefile");

    if (MakeFile.fileExists())
        return true;  // Don't overwrite an existing makefile

    // We get here if the makefile is missing

    ttlib::cstr BuildDir(BuildRoot);
    // Some repositories use a bld directory which is added to .gitignore. If it exists, we'll use this directory
    BuildDir.append_filename("bld");

    auto ProjectFile = locateProjectFile(Dir);
    if (ProjectFile.empty())
    {
        AddError(_tt("Cannot locate .srcfiles.yaml. Makefile not created."));
        return false;
    }

    ttlib::textfile file;
    // Put resText in a block so that it gets deleted after we've read it.
    {
        auto resText = ttlib::LoadTextResource(isAllVersion ? IDR_MAKEFILE_ALL : IDR_MAKEFILE_SINGLE);
        if (resText.empty())
        {
            // TRANSLATORS: Don't change the filename "makefile"
            AddError(
                _tt("ttBld.exe is corrupted -- unable to read the required resource for creating a makefile,"));
            return false;
        }

        file.ReadString(resText);
    }

    size_t pos;

    while (pos = file.FindLineContaining("%build%"), pos != tt::npos)
    {
        file[pos].Replace("%build%", BuildDir, true);
    }

    if (!isAllVersion)
    {
        while (pos = file.FindLineContaining("%srcfiles%"), pos != tt::npos)
        {
            file[pos].Replace("%srcfiles%", ProjectFile, true);
        }
    }

    while (pos = file.FindLineContaining("%project%"), pos != tt::npos)
    {
        file[pos].Replace("%project%", GetProjectName(), true);
    }

    // Now we parse the file as if we had read it, changing or adding as needed

    ttlib::textfile outFile;

    for (pos = 0; pos < file.size(); ++pos)
    {
        auto& line = file[pos];
        if (line.issameprefix("release:"))
        {
            if (!GetHHPName().empty())
            {
                line.Replace(" ", " ChmHelp ");
            }

            // add all build libs to the target list
            for (auto& bldLib: m_bldLibs)
            {
                line.Replace(" ", (" " + bldLib.shortname + " "));
            }

            // Now that the target list is updated, add specific build commands to match the targets we added.
            ++pos;
            assert(pos < file.size());

            if (!GetHHPName().empty())
            {
                file.insertEmptyLine(pos++);
                file.insertEmptyLine(pos++) = "ChmHelp:";
                file.insertEmptyLine(pos++) = "\t ninja -f bld/ChmHelp.ninja";
            }

            for (auto& bldLib: m_bldLibs)
            {
                file.insertEmptyLine(pos++);
                file.insertEmptyLine(pos++) = bldLib.shortname + ":";
                file.insertEmptyLine(pos++) = "\tcd " + bldLib.srcDir + " & ninja -f $(BldScript)";
            }
        }
        else if (line.issameprefix("debug:"))
        {
            // add all build libs to the target list
            for (auto& bldLib: m_bldLibs)
            {
                line.Replace(" ", (" " + bldLib.shortname + "D "));
            }

            // Now that the target list is updated, add specific build commands to match the targets we added.
            ++pos;
            assert(pos < file.size());

            for (auto& bldLib: m_bldLibs)
            {
                file.insertEmptyLine(pos++);
                file.insertEmptyLine(pos++) = bldLib.shortname + "D:";
                file.insertEmptyLine(pos++) = "\tcd " + bldLib.srcDir + " & ninja -f $(BldScriptD)";
            }
        }
    }

    // REVIEW: [KeyWorks - 02-29-2020] Hack Alert! We still haven't worked out a good solution for when
    // the makefile will require ttBld.exe to be used. For now, if "-alld" was specified on the command line,
    // then we add the code to force using ttBld.exe (the advantage of requiring ttBld is that ninja scripts get
    // automatically updated whenever .srcfiles.yaml changes)
    if (isAllVersion)
    {
        file.addEmptyLine();
        file.emplace_back("########## Creates or updates ninja file any time .srcfiles.yaml changes ##########");

        file.addEmptyLine();
        file.emplace_back("$(BldScript): .srcfiles.yaml");
        file.emplace_back("\tttBld.exe -u$(cmplr)");

        file.addEmptyLine();
        file.emplace_back("$(BldScriptD): .srcfiles.yaml");
        file.emplace_back("\tttBld.exe -u$(cmplr)D");
    }

    // If the makefile already exists, don't write to it unless something has actually changed

    if (MakeFile.fileExists())
    {
        ttlib::viewfile oldMakefile;
        if (!oldMakefile.ReadFile(MakeFile) || !file.issameas(oldMakefile))
        {
            if (m_dryrun.IsEnabled())
            {
                m_dryrun.NewFile(MakeFile);
                m_dryrun.DisplayFileDiff(oldMakefile, file);
            }
            else if (file.WriteFile(MakeFile))
            {
                std::cout << MakeFile << _tt(" updated") << '\n';
                return true;
            }
            else
            {
                // TRANSLATORS: Don't change the filename "makefile"
                std::cout << _tt("Unable to write to") << MakeFile << '\n';
                return false;
            }
        }
    }
    else
    {
        if (file.WriteFile(MakeFile))
        {
            std::cout << MakeFile << _tt(" updated") << '\n';
            return true;
        }
        else
        {
            // TRANSLATORS: Don't change the filename "makefile"
            std::cout << _tt("Unable to write to") << MakeFile << '\n';
            return false;
        }
    }

    return true;
}
