/////////////////////////////////////////////////////////////////////////////
// Purpose:   Adds files to .srcfiles Files: section
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "tttextfile.h"  // Classes for reading and writing line-oriented files

#include "csrcfiles.h"  // CSrcFiles
#include "dryrun.h"     // CDryRun

void AddFiles(const std::vector<ttlib::cstr>& lstFiles)
{
    if (lstFiles.size() < 1)
    {
        std::cout << "You didn't specify any files to add!" << '\n';
        return;
    }

    CSrcFiles cSrcFiles;
    if (!cSrcFiles.ReadFile())
    {
        std::cout << "Cannot locate the file " << cSrcFiles.GetSrcFilesName() << '\n';
        return;
    }

    size_t cFilesAdded = 0;
    for (auto& iter: lstFiles)
    {
        if (!ttlib::has_filename(cSrcFiles.GetSrcFileList(), iter))
        {
            ttlib::add_if(cSrcFiles.GetSrcFileList(), iter);
            ++cFilesAdded;
        }
    }

    ttlib::textfile file;
    if (!file.ReadFile(cSrcFiles.GetSrcFilesName()))
    {
        std::cout << "Cannot locate read the file " << cSrcFiles.GetSrcFilesName() << '\n';
        return;
    }

    auto pos = file.FindLineContaining("Files:");
    if (pos == tt::npos)
    {
        file.emplace_back("Files:");
        for (auto& iter: lstFiles)
        {
            auto& line = file.addEmptyLine();
            line.assign("    " + iter);
        }
    }
    else
    {
        for (++pos; pos < file.size(); ++pos)
        {
            if (file[pos].empty())
            {
                if (pos < file.size())
                    ++pos;
                break;
            }
        }

        for (auto& iter: lstFiles)
        {
            auto line = file.insertEmptyLine(pos++);
            line.assign("    " + iter);
        }
    }

    if (!file.WriteFile(cSrcFiles.GetSrcFilesName()))
    {
        std::cout << "Unable to create or write to " << cSrcFiles.GetSrcFilesName() << '\n';
    }
    else
    {
        std::cout << cFilesAdded << "files added." << '\n';
    }
    return;
}
