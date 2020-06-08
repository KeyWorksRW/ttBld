/////////////////////////////////////////////////////////////////////////////
// Name:      gencmdfiles.cpp
// Purpose:   Generates MSVCenv.cmd and Code.cmd files
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <iostream>

#include <ttmultistr.h>  // multistr -- Breaks a single string into multiple strings
#include <tttextfile.h>  // textfile -- Classes for reading and writing line-oriented files

#include "funcs.h"     // List of function declarations

#if defined(_WIN32)  // no reason to use the batch files on non-Windows platforms

void CreateCodeCmd(const char* pszFile)
{
    ttASSERT_MSG(pszFile, "NULL pointer!");

    ttlib::cstr Root;
    if (!FindVsCode(Root))
    {
        puts("Unable to locate the VS Code directory.");
        return;
    }

    ttlib::cstr Path { Root };
    Path.append_filename("bin/MSVCenv.cmd");
    Path.backslashestoforward();

    ttlib::textfile file;
    file.emplace_back("@echo off");

    const char* pszType = ttlib::contains(pszFile, "64") ? "64" : "32";
    file.addEmptyLine().Format("ttBld.exe -msvcenv%s %ks", pszType, Path.c_str());
    file.addEmptyLine().Format("call %ks", Path.c_str());
    file.addEmptyLine();

    file.emplace_back("setlocal");
    file.emplace_back("set VSCODE_DEV=");
    file.emplace_back("set ELECTRON_RUN_AS_NODE=1");

    Path = Root;
    Path.append_filename("code.exe");
    Path.backslashestoforward();
    ttlib::cstr Cli { Root };
    Cli.append_filename("resources/app/out/cli.js");
    Cli.backslashestoforward();
    file.addEmptyLine().Format("%ks %ks %%*", Path.c_str(), Cli.c_str());
    file.emplace_back("endlocal");

    Path = Root;
    Path.append_filename("bin/");
    Path.append_filename(pszFile);
    Path.backslashestoforward();

    if (file.WriteFile(Path))
    {
        std::cout << Path << _tt(strIdCreatedSuffix) << '\n';
    }
    else
    {
        // It's possible to just move the entire VSCode directory and it will continue to work fine. In that case,
        // the registry will still be pointing to the old location, but code.cmd may be located in the PATH, so we
        // can look for that and use that directory if the registry location is wrong.

        ttlib::cstr NewPath;
        if (FindFileEnv("PATH", "code.cmd", NewPath))
        {
            NewPath.replace_filename(pszFile);
            if (file.WriteFile(NewPath))
            {
                std::cout << NewPath << _tt(strIdCreatedSuffix) << '\n';
                return;
            }
        }

        std::cerr << _tt(strIdCantWrite) << Path << '\n';
    }
}

#endif  // !defined(_WIN32)

static void AddToList(const char* pszEnv, ttlib::cstrVector& lstPaths)
{
    ttlib::cstr Env;
    Env.assignEnvVar(pszEnv);

    if (Env.length())
    {
        ttlib::multistr enumLib(Env);
        for (auto& iter: enumLib)
        {
            iter.backslashestoforward();

            lstPaths.addfilename(iter);
        }
    }
}

bool CreateMSVCEnvCmd(const char* pszDstFile, bool bDef64)
{
    ttlib::cstr MSVC;
    if (!FindCurMsvcPath(MSVC))
        return false;  // probably means MSVC isn't installed
    MSVC.backslashestoforward();

    bool bHost64 = IsHost64();  // figure out what processor we have to determine what compiler host to use

    ttlib::cstrVector lstLib;
    ttlib::cstrVector lstLib32;
    ttlib::cstrVector lstPath;
    ttlib::cstrVector lstPath32;
    ttlib::cstrVector lstInc;

    // Add the PATH to the toolchain first so that it becomes the first directory searched

    ttlib::cstr Path;

    Path = MSVC;
    Path.append_filename("bin/");
    Path.append_filename(bHost64 ? "Hostx64/" : "Hostx86/");
    Path.append_filename(bDef64 ? "x64" : "x86");
    lstPath.addfilename(Path);

    if (bDef64)
    {
        Path = MSVC;
        Path.append_filename("bin/");
        Path.append_filename(bHost64 ? "Hostx64/" : "Hostx86/");
        Path.append_filename("x86");
        lstPath32.addfilename(Path);

        // The main reason for switching to Hostx64/x86 is to swap compilers. Not all parts of the toolchain are
        // duplicated there, so we need to add a path to the rest of the toolchain.

        if (bHost64)
        {
            Path = MSVC;
            Path.append_filename("bin/Hostx64/x64");
            lstPath32.addfilename(Path);
        }
    }

    // Gather up all the current paths into lists for each environment variable

    AddToList("LIB", lstLib);
    AddToList("LIB32", lstLib32);
    AddToList("PATH", lstPath);
    AddToList("PATH32", lstPath32);
    AddToList("INCLUDE", lstInc);

    if (bDef64 && lstLib32.empty())
    {
        // LIB32 hasn't been set, so let's copy LIB to LIB32, converting any x64 to x86
        for (auto& iter: lstLib)
        {
            auto& name = lstLib32.emplace_back(iter);
            name.Replace("x64", "x86");
        }
    }
    if (bDef64 && lstPath32.empty())
    {
        // PATH32 hasn't been set, so let's copy PATH to PATH32, converting any x64 to x86
        for (auto& iter: lstPath)
        {
            auto& name = lstPath32.emplace_back(iter);
            name.Replace("x64", "x86");
        }
    }

    Path = MSVC;
    Path.append_filename(bDef64 ? "lib/x64" : "lib/x86");
    lstLib.addfilename(Path);

    if (bDef64)
    {
        Path = MSVC;
        Path.append_filename("lib/x86");
        lstLib32.addfilename(Path);
    }

    Path = MSVC;
    Path.append_filename("include");
    lstInc.addfilename(Path);

    ttlib::textfile file;
    file.emplace_back("@echo off");
    file.emplace_back("@REM This file is automatically created by ttBld. Any changes you make will be lost if ttBld "
                      "creates it again.");

    Path.clear();
    for (auto& iter: lstPath)
    {
        Path.append(iter + ";");
    }
    file.emplace_back("set PATH=" + Path);

    if (bDef64)
    {
        Path.clear();
        for (auto& iter: lstPath32)
        {
            Path.append(iter + ";");
        }
        file.emplace_back("set PATH32=" + Path);
    }

    Path.clear();
    for (auto& iter: lstLib)
    {
        Path.append(iter + ";");
    }
    file.emplace_back("set LIB=" + Path);

    if (bDef64)
    {
        Path.clear();
        for (auto& iter: lstLib32)
        {
            Path.append(iter + ";");
        }
        file.emplace_back("set LIB32=" + Path);
    }

    // Now add the include environment the only one that doesn't have a 32-bit and 64-bit version

    Path.clear();
    for (auto& iter: lstInc)
    {
        Path.append(iter + ";");
    }
    file.emplace_back("set INCLUDE=" + Path);

    // Don't write the file unless something has actually changed

    ttlib::viewfile fileOrg;
    if (fileOrg.ReadFile(pszDstFile))
    {
        if (file.issameas(fileOrg))
            return true;  // nothing changed
    }

    return file.WriteFile(pszDstFile);
}
