/////////////////////////////////////////////////////////////////////////////
// Purpose:   Functions for working with .git
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "tttextfile.h"  // textfile -- Classes for reading and writing line-oriented files

// If .gitignore is found, gitIgnorePath will be updated to point to it
bool gitIsFileIgnored(ttlib::cstr& gitIgnorePath, std::string_view filename)
{
    if (gitIgnorePath.empty())
        gitIgnorePath = ".gitignore";
    if (!gitIgnorePath.file_exists())
    {
        gitIgnorePath = "../.gitignore";
        if (!gitIgnorePath.file_exists())
        {
            gitIgnorePath = "../../.gitignore";
            if (!gitIgnorePath.file_exists())
            {
                gitIgnorePath.clear();
                return false;
            }
        }
    }

    ttlib::viewfile file;
    if (file.ReadFile(gitIgnorePath))
    {
        return (file.FindLineContaining(filename) != tt::npos);
    }
    return false;
}

// If .git/info/exclude is found, GitExclude will be updated to point to it
bool gitIsExcluded(ttlib::cstr& GitExclude, std::string_view filename)
{
    if (GitExclude.empty())
    {
        if (ttlib::dir_exists(".git"))
            GitExclude = ".git/info/exclude";
        else if (ttlib::dir_exists("../.git"))
            GitExclude = "../.git/info/exclude";
        else if (ttlib::dir_exists("../../.git"))
            GitExclude = "../../.git/info/exclude";
        else
            return false;
    }

    if (!GitExclude.file_exists())
    {
        return false;
    }

    ttlib::viewfile file;
    if (file.ReadFile(GitExclude))
    {
        return (file.FindLineContaining(filename) != tt::npos);
    }
    return false;
}

// This will work with either .gitignore or exclude
bool gitAddtoIgnore(ttlib::cstr& GitIgnore, std::string_view filename)
{
    ttlib::textfile file;
    if (!file.ReadFile(GitIgnore))
        return false;

    for (size_t line = 0; line < file.size(); ++line)
    {
        if (file[line][0] == '#')
            continue;
        file.insertLine(line, filename);
        return file.WriteFile(GitIgnore);
    }

    file.emplace_back(filename);
    return file.WriteFile(GitIgnore);
}

// clang-format off
static const char* atxtIgnoreList[]
{
    "makefile",
    ".srcfiles.yaml",
    "bld/",
    ".vscode/",
};
// clang-format on

bool gitIgnoreAll(ttlib::cstr& GitExclude)
{
    if (ttlib::dir_exists(".git"))
        GitExclude = ".git/info/exclude";
    else if (ttlib::dir_exists("../.git"))
        GitExclude = "../.git/info/exclude";
    else if (ttlib::dir_exists("../../.git"))
        GitExclude = "../../.git/info/exclude";
    else
        return false;

    ttlib::textfile file;
    if (!file.ReadFile(GitExclude))
        return false;

    for (auto name: atxtIgnoreList)
    {
        // If makefile or bld/ already exist, then assume they are already part of the project, so don't
        // ignore them.

        if (ttlib::is_sameas(name, "makefile") && ttlib::file_exists("makefile"))
            continue;
        if (ttlib::is_sameas(name, "bld/") && ttlib::dir_exists("bld"))
            continue;

        bool doInsert = true;
        for (auto& line: file)
        {
            if (line.is_sameas(name))
            {
                doInsert = false;
                break;
            }
        }
        if (doInsert)
            file.emplace_back(name);
    }

    return file.WriteFile(GitExclude);
}
