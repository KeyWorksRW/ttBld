/////////////////////////////////////////////////////////////////////////////
// Name:      gitfuncs.cpp
// Purpose:   Functions for working with .git
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttfile.h>      // ttCFile -- class for reading and writing files, strings, data, etc.
#include <ttlinefile.h>  // ttCLineFile -- Line-oriented file class
#include <ttstr.h>       // ttCStr -- SBCS string class.
#include <tttextfile.h>  // textfile -- Classes for reading and writing line-oriented files

// If .gitignore is found, gitIgnorePath will be updated to point to it
bool gitIsFileIgnored(ttlib::cstr& gitIgnorePath, std::string_view filename)
{
    if (gitIgnorePath.empty())
        gitIgnorePath = ".gitignore";
    if (!gitIgnorePath.fileExists())
    {
        gitIgnorePath = "../.gitignore";
        if (!gitIgnorePath.fileExists())
        {
            gitIgnorePath = "../../.gitignore";
            if (!gitIgnorePath.fileExists())
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
        if (ttlib::dirExists(".git"))
            GitExclude = ".git/info/exclude";
        else if (ttlib::dirExists("../.git"))
            GitExclude = "../.git/info/exclude";
        else if (ttlib::dirExists("../../.git"))
            GitExclude = "../../.git/info/exclude";
        else
            return false;
    }

    if (!GitExclude.fileExists())
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

bool gitIsFileIgnored(ttCStr& cszGitIgnore, const char* pszFile)
{
    if (cszGitIgnore.IsEmpty())
        cszGitIgnore = ".gitignore";
    if (!ttFileExists(cszGitIgnore))
    {
        cszGitIgnore = "../.gitignore";
        if (!ttFileExists(cszGitIgnore))
        {
            cszGitIgnore = "../../.gitignore";
            if (!ttFileExists(cszGitIgnore))
            {
                cszGitIgnore.Delete();
                return false;
            }
        }
    }

    ttCFile file;
    if (file.ReadFile(cszGitIgnore))
    {
        while (file.ReadLine())
        {
            if (ttIsSameStrI(file, pszFile))
                return true;
        }
    }
    cszGitIgnore.Delete();
    return false;
}

// If .git/info/exclude is found, cszGitExclude will be updated to point to it

bool gitIsExcluded(ttCStr& cszGitExclude, const char* pszFile)
{
    ttASSERT_MSG(pszFile, "NULL pointer!");
    if (!pszFile)
        return false;

    if (cszGitExclude.IsEmpty())
    {
        if (ttlib::dirExists(".git"))
            cszGitExclude = ".git/info/exclude";
        else if (ttlib::dirExists("../.git"))
            cszGitExclude = "../.git/info/exclude";
        else if (ttlib::dirExists("../../.git"))
            cszGitExclude = "../../.git/info/exclude";
        else
            return false;
    }

    if (!ttFileExists(cszGitExclude))
    {
        cszGitExclude.Delete();
        return false;
    }

    ttCFile file;
    if (file.ReadFile(cszGitExclude))
    {
        while (file.ReadLine())
        {
            if (ttIsSameStrI(file, pszFile))
                return true;
        }
    }
    cszGitExclude.Delete();
    return false;
}

// This will work with either .gitignore or exclude

bool gitAddtoIgnore(ttCStr& cszGitIgnore, const char* pszFile)
{
    ttCLineFile file;
    if (!file.ReadFile(cszGitIgnore))
        return false;

    int line;
    for (line = 0; line < file.GetMaxLine(); ++line)
    {
        if (file[line][0] == '#')
            continue;
        file.InsertLine(line, pszFile);
        return file.WriteFile();
    }

    file.AddLine(pszFile);
    return file.WriteFile();
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
    if (ttlib::dirExists(".git"))
        GitExclude = ".git/info/exclude";
    else if (ttlib::dirExists("../.git"))
        GitExclude = "../.git/info/exclude";
    else if (ttlib::dirExists("../../.git"))
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

        if (ttlib::issameas(name, "makefile") && ttlib::fileExists("makefile"))
            continue;
        if (ttlib::issameas(name, "bld/") && ttlib::dirExists("bld"))
            continue;

        bool doInsert = true;
        for (auto& line: file)
        {
            if (line.issameas(name))
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
