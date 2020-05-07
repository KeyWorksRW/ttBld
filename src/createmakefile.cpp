/////////////////////////////////////////////////////////////////////////////
// Name:      CreateMakeFile()
// Purpose:   Creates an MS nmake and GNU-make compatible makefile
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttcstr.h>      // Classes for handling zero-terminated char strings.
#include <ttmultistr.h>  // multistr -- Breaks a single string into multiple strings

#include "ninja.h"     // CNinja
#include "resource.h"  // IDR_MAKEFILE
#include "strtable.h"  // String resource IDs

extern const char* txtHelpNinja;

bool CNinja::CreateMakeFile(bool isAllVersion, std::string_view Dir)
{
    ttlib::cstr BuildRoot(Dir);

    ttlib::cstr MakeFile;

    if (hasOptValue(OPT::MAKE_DIR))
    {
        MakeFile = getOptValue(OPT::MAKE_DIR);
        MakeFile.backslashestoforward();
        if (MakeFile.back() == '/')
            MakeFile.pop_back();
        if (GetBldDir().issameas(MakeFile, tt::CASE::either))
        {
            if (!GetBldDir().dirExists())
            {
                if (!fs::create_directory(GetBldDir().c_str()))
                {
                    AddError(_tt(IDS_CANT_CREATE) + GetBldDir());
                    return false;
                }
            }
        }
        else {
            if (!MakeFile.dirExists())
            {
                AddError(_tt(IDS_MISSING_MAKEFILE_DIR) + MakeFile());
            }
        }
    }
    else
    {
        MakeFile = BuildRoot;
    }

    MakeFile.append_filename("makefile");

    if (!hasOptValue(OPT::MAKE_DIR) && MakeFile.fileExists())
        return true;  // Don't overwrite an existing makefile

    // We get here if the makefile is missing or makedir: option was specified

    ttlib::cstr BuildDir(BuildRoot);
    // Some repositories use a bld directory which is added to .gitignore. If it exists, we'll use this directory
    BuildDir.append_filename("bld");

    auto ProjectFile = locateProjectFile(Dir);
    if (ProjectFile.empty())
    {
        AddError(_tt(IDS_CANT_LOCATE_SRCFILES) + " " + _tt(IDS_MAKEFILE_NOT_CREATED));
        return false;
    }

    ttlib::textfile file;
    // Put resText in a block so that it gets deleted after we've read it.
    {
        auto resText = ttlib::LoadTextResource(isAllVersion ? IDR_MAKEFILE_ALL : IDR_MAKEFILE_SINGLE);
        if (resText.empty())
        {
            AddError(_tt(IDS_APP_CORRUPTED) + " " + _tt(IDS_MAKEFILE_NOT_CREATED));
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

    if (hasOptValue(OPT::MAKE_DIR))
    {
        auto& line = file.insertLine(0, "# WARNING: This file is auto-generated by ");
        line += txtVersion;
        file.insertLine(1, "# Changes you make will be lost if it is auto-generated again!");
        file.insertEmptyLine(2);
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
                std::cout << MakeFile << _tt(IDS_UPDATED) << '\n';
                return true;
            }
            else
            {
                std::cout << _tt(IDS_CANT_CREATE) << MakeFile << '\n';
                return false;
            }
        }
    }
    else
    {
        if (file.WriteFile(MakeFile))
        {
            std::cout << MakeFile << _tt(IDS_UPDATED) << '\n';
            return true;
        }
        else
        {
            std::cout << _tt(IDS_CANT_CREATE) << MakeFile << '\n';
            return false;
        }
    }

    return true;
}
