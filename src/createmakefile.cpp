/////////////////////////////////////////////////////////////////////////////
// Purpose:   Creates an MS nmake and GNU-make compatible makefile
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <filesystem>

#include "ttmultistr.h"  // multistr -- Breaks a single string into multiple strings

#include "ninja.h"  // CNinja

const char* res_makefile =
#include "res/makefile"
    ;

const char* res_makefile_ttbld =
#include "res/makefile_ttbld"
    ;

extern const char* txtHelpNinja;

bool CNinja::CreateMakeFile(MAKE_TYPE type)
{
    ttlib::cstr MakeFile(type == MAKE_TYPE::normal ? "makefile" : "bld/makefile");

    if (type == MAKE_TYPE::normal && MakeFile.file_exists())
    {
        AddError(_tt(strIdMakefileExists));
        return true;  // Don't overwrite an existing makefile
    }

    ttlib::textfile file;
    // Put resText in a block so that it gets deleted after we've read it.
    {
        ttlib::cstr resText;
        if (type == MAKE_TYPE::normal)
            resText = ttlib::find_nonspace(res_makefile);
        else
            resText = ttlib::find_nonspace(res_makefile_ttbld);

        resText.Replace("%build%", GetBldDir(), tt::REPLACE::all);
        resText.Replace("%project%", GetProjectName(), tt::REPLACE::all);

        if (type == MAKE_TYPE::autogen)
        {
            resText.Replace("%srcfiles%", GetSrcFilesName(), tt::REPLACE::all);
        }

        file.ReadString(resText);
    }

    // Now we parse the file as if we had read it, changing or adding as needed

    ttlib::textfile outFile;

    for (size_t pos = 0; pos < file.size(); ++pos)
    {
        auto& line = file[pos];
        if (line.is_sameprefix("release:"))
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
        else if (line.is_sameprefix("debug:"))
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

    if (type == MAKE_TYPE::autogen)
    {
        auto& line = file.insertLine(0, "# WARNING: This file is auto-generated by ");
        line += txtVersion;
        file.insertLine(1, "# Changes you make will be lost if it is auto-generated again!");
        file.insertEmptyLine(2);
    }

    // If the makefile already exists, don't write to it unless something has actually changed

    if (MakeFile.file_exists())
    {
        ttlib::viewfile oldMakefile;
        if (!oldMakefile.ReadFile(MakeFile) || !file.is_sameas(oldMakefile))
        {
            if (m_dryrun.IsEnabled())
            {
                m_dryrun.NewFile(MakeFile);
                m_dryrun.DisplayFileDiff(oldMakefile, file);
            }
            else if (file.WriteFile(MakeFile))
            {
                std::cout << MakeFile << _tt(strIdUpdated) << '\n';
                return true;
            }
            else
            {
                std::cout << _tt(strIdCantWrite) << MakeFile << '\n';
                return false;
            }
        }
    }
    else
    {
        ttlib::cstr path(MakeFile);
        path.remove_filename();
        if (path.size() && !path.dir_exists())
        {
            if (!std::filesystem::create_directory(path.wx_str()))
            {
                std::cout << _tt(strIdCantWrite) << MakeFile << '\n';
                return false;
            }
        }

        if (file.WriteFile(MakeFile))
        {
            std::cout << _tt(strIdCreated) << MakeFile << '\n';
            return true;
        }
        else
        {
            std::cout << _tt(strIdCantWrite) << MakeFile << '\n';
            return false;
        }
    }

    return true;
}
