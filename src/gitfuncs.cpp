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

// If .gitignore is found, cszGitIgnore will be updated to point to it

bool gitIsFileIgnored(ttCStr& cszGitIgnore, const char* pszFile)
{
    ttASSERT_MSG(pszFile, "NULL pointer!");
    if (!pszFile)
        return false;

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
        if (ttDirExists(".git"))
            cszGitExclude = ".git/info/exclude";
        else if (ttDirExists("../.git"))
            cszGitExclude = "../.git/info/exclude";
        else if (ttDirExists("../../.git"))
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
static char* atxtIgnoreList[] =
{
    ".vscode/",
    "bld/",
    ".srcfiles.yaml",
    "makefile",

    nullptr
};
// clang-format on

bool gitIgnoreAll(ttCStr& cszGitExclude)
{
    if (ttDirExists(".git"))
        cszGitExclude = ".git/info/exclude";
    else if (ttDirExists("../.git"))
        cszGitExclude = "../.git/info/exclude";
    else if (ttDirExists("../../.git"))
        cszGitExclude = "../../.git/info/exclude";
    else
        return false;

    ttCLineFile file;
    if (!file.ReadFile(cszGitExclude))
        return false;

    for (size_t pos = 0; atxtIgnoreList[pos]; ++pos)
    {
        int  line;
        bool bInserted = false;
        for (line = 0; line < file.GetMaxLine(); ++line)
        {
            if (file[line][0] == '#')
                continue;
            file.InsertLine(line, atxtIgnoreList[pos]);
            bInserted = true;
            break;
        }
        if (!bInserted)
            file.AddLine(atxtIgnoreList[pos]);
    }

    return file.WriteFile();
}
