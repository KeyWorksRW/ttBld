/////////////////////////////////////////////////////////////////////////////
// Name:      gencmdfiles.cpp
// Purpose:   Generates MSVCenv.cmd and Code.cmd files
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttfile.h>    // ttCFile
#include <ttlist.h>    // ttCList
#include <ttenumstr.h> // ttCEnumStr


#if defined(_WIN32)    // no reason to use the batch files on non-Windows platforms

void CreateCodeCmd(const char* pszFile)
{
    ttASSERT_MSG(pszFile, "NULL pointer!");

    ttCStr cszRoot, cszPath;
    if (!FindVsCode(cszRoot))
    {
        puts("Unable to locate the VS Code directory.");
        return;
    }

    cszPath = cszRoot;
    cszPath.AppendFileName("bin/");

    cszPath += "MSVCenv.cmd";
    ttBackslashToForwardslash(cszPath);

    ttCFile file;
    file.WriteEol("@echo off\n");
    const char* pszType = ttStrStr(pszFile, "64") ? "64" : "32";
    file.printf("makeninja.exe -msvcenv%s %kq\n", pszType, (char*) cszPath);
    file.printf("call %kq\n\n", (char*) cszPath);

    file.WriteEol(
            "setlocal\n"
            "set VSCODE_DEV=\n"
            "set ELECTRON_RUN_AS_NODE=1"
    );

    cszPath = cszRoot;
    cszPath.AppendFileName("code.exe");
    ttBackslashToForwardslash(cszPath);
    ttCStr cszCli(cszRoot);
    cszCli.AppendFileName("resources/app/out/cli.js");
    ttBackslashToForwardslash(cszCli);
    file.printf("%kq %kq %%*\n", (char*) cszPath, (char*) cszCli);
    file.WriteEol("endlocal");

    cszPath = cszRoot;
    cszPath.AppendFileName("bin/");
    cszPath.AppendFileName(pszFile);
    ttBackslashToForwardslash(cszPath);

    if (file.WriteFile(cszPath))
        printf(GETSTRING(IDS_NINJA_CREATED), (char*) cszPath);
}

#endif    // !defined(_WIN32)

static void AddToList(const char* pszEnv, ttCList& lstPaths)
{
    ttCStr cszEnv, cszPath;
    size_t cbEnv = 0;
    if (getenv_s(&cbEnv, nullptr, 0, pszEnv) == 0 && cbEnv > 0)
    {
        cszEnv.ReSize(cbEnv + 1);
        if (getenv_s(&cbEnv, cszEnv.GetPtr(), cbEnv, pszEnv) == 0)
        {
            ttCEnumStr enumLib(cszEnv, ';');
            while (enumLib.Enum())
            {
                cszPath = enumLib;
                ttForwardslashToBackslash(cszPath); // just to be certain that they are consistent
                lstPaths += cszPath;
            }
        }
    }
}

bool CreateMSVCEnvCmd(const char* pszDstFile, bool bDef64)
{
#if !defined(_WIN32)
    return false;   // MSVC compiler and environment is only available on Windows

#else    // Windows-only code below

    ttCStr cszMSVC;
    if (!FindCurMsvcPath(cszMSVC))
        return false;           // probably means MSVC isn't installed

    bool bHost64 = IsHost64();     // figure out what processor we have to determine what compiler host to use

    ttCList lstLib;
    ttCList lstLib32((HANDLE) lstLib), lstPath((HANDLE) lstLib), lstPath32((HANDLE) lstLib);    // share the sub-heap

    lstLib.SetFlags(ttCList::FLG_URL_STRINGS);  // ignore case, forward and backslash considered the same
    lstLib32.SetFlags(ttCList::FLG_URL_STRINGS);
    lstPath.SetFlags(ttCList::FLG_URL_STRINGS);
    lstPath32.SetFlags(ttCList::FLG_URL_STRINGS);

    // Add the PATH to the toolchain first so that it becomes the first directory searched

    ttCStr cszPath;

    cszPath = cszMSVC;
    cszPath.AppendFileName("bin/");
    cszPath.AppendFileName(bHost64 ? "Hostx64/" : "Hostx86/");
    cszPath.AppendFileName(bDef64 ? "x64" : "x86");
    ttForwardslashToBackslash(cszPath); // just to be certain that they are consistent
    lstPath += cszPath;

    if (bDef64)
    {
        cszPath = cszMSVC;
        cszPath.AppendFileName("bin/");
        cszPath.AppendFileName(bHost64 ? "Hostx64/" : "Hostx86/");
        cszPath.AppendFileName("x86");
        ttForwardslashToBackslash(cszPath); // just to be certain that they are consistent
        lstPath32 += cszPath;
    }

    // Gather up all the current paths into lists for each environment variable

    AddToList("LIB", lstLib);
    AddToList("LIB32", lstLib32);
    AddToList("PATH", lstPath);
    AddToList("PATH32", lstPath32);

    if (bDef64 && lstLib32.IsEmpty())
    {
        // LIB32 hasn't been set, so let's copy LIB to LIB32, converting any x64 to x86
        for (size_t pos = 0; lstLib.InRange(pos); ++pos)
        {
            cszPath = lstLib[pos];
            while (cszPath.ReplaceStr("x64", "x86"));
            lstLib32 += cszPath;
        }
    }
    if (bDef64 && lstPath32.IsEmpty())
    {
        // LIB32 hasn't been set, so let's copy LIB to LIB32, converting any x64 to x86
        for (size_t pos = 0; lstPath.InRange(pos); ++pos)
        {
            cszPath = lstPath[pos];
            while (cszPath.ReplaceStr("x64", "x86"));
            lstPath32 += cszPath;
        }
    }

    cszPath = cszMSVC;
    cszPath.AppendFileName(bDef64 ? "lib/x64" : "lib/x86");
    ttForwardslashToBackslash(cszPath); // just to be certain that they are consistent
    lstLib += cszPath;

    if (bDef64)
    {
        cszPath = cszMSVC;
        cszPath.AppendFileName("lib/x86");
        ttForwardslashToBackslash(cszPath); // just to be certain that they are consistent
        lstLib32 += cszPath;
    }

    ttCFile file;
    file.WriteEol("@echo off\n");
    file.WriteEol("@REM This file is automatically created by MakeNinja. Any changes you make will be lost if MakeNinja creates it again.\n");

    cszPath.Delete();
    for (size_t pos = 0; lstPath.InRange(pos); ++pos)
    {
        cszPath += lstPath[pos];
        cszPath += ";";
    }
    file.WriteStr("set PATH=");
    file.WriteEol(cszPath);

    if (bDef64)
    {
        cszPath.Delete();
        for (size_t pos = 0; lstPath32.InRange(pos); ++pos)
        {
            cszPath += lstPath32[pos];
            cszPath += ";";
        }
        file.WriteStr("set PATH32=");
        file.WriteEol(cszPath);
    }

    cszPath.Delete();
    for (size_t pos = 0; lstLib.InRange(pos); ++pos)
    {
        cszPath += lstLib[pos];
        cszPath += ";";
    }
    file.WriteStr("set LIB=");
    file.WriteEol(cszPath);

    if (bDef64)
    {
        cszPath.Delete();
        for (size_t pos = 0; lstLib32.InRange(pos); ++pos)
        {
            cszPath += lstLib32[pos];
            cszPath += ";";
        }
        file.WriteStr("set LIB32=");
        file.WriteEol(cszPath);
    }

    return file.WriteFile(pszDstFile);
#endif    // end Windows-only code
}
