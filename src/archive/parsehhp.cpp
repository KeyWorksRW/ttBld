/////////////////////////////////////////////////////////////////////////////
// Name:      CParseHHP
// Purpose:   Parse a .HHP file and generate a list of file dependencies
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttcvector.h>   // cstrVector -- Vector of ttlib::cstr strings
#include <ttcwd.h>       // cwd -- Class for storing and optionally restoring the current directory
#include <tttextfile.h>  // Classes for reading and writing line-oriented files

#include "ninja.h"     // CNinja
#include "parsehhp.h"  // CParseHHP
#include "strtable.h"  // String resource IDs

bool CNinja::CreateHelpFile()
{
    if (GetHHPName().empty())
        return false;

    if (!ttlib::dirExists("bld"))
    {
        if (!fs::create_directory("bld"))
        {
            AddError("Unable to create the build directory -- so no place to put the .ninja files!\n");
            return false;
        }
    }

    ttlib::viewfile hhpFile;
    if (!hhpFile.ReadFile(GetHHPName()))
    {
        std::string str(_tt(IDS_CANNOT_OPEN); str += GetHHPName(); str += "\n") AddError(str);
        return false;
    }

    ttlib::cwd cwd;

    ttlib::cstrVector dependencyList;

    auto pos = hhpFile.FindLineContaining("Contents file=");
    if (ttlib::isFound(pos))
    {
        auto filePos = hhpFile[pos].find('=') + 1;
        ttlib::cstr file;
        file.assign(hhpFile[pos].substr(filePos + 1));
        file.make_absolute();
        file.make_relative(cwd);
        file.backslashestoforward();
        dependencyList.emplace_back(file);
    }

    pos = hhpFile.FindLineContaining("Index file=");
    if (ttlib::isFound(pos))
    {
        auto filePos = hhpFile[pos].find('=') + 1;
        ttlib::cstr file;
        file.assign(hhpFile[pos].substr(filePos + 1));
        file.make_absolute();
        file.make_relative(cwd);
        file.backslashestoforward();
        dependencyList.emplace_back(file);
    }

    return false;
}
