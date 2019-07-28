/////////////////////////////////////////////////////////////////////////////
// Name:      vscode.cpp
// Purpose:   Creates/updates .vscode files
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttmem.h>              // ttCTMem
#include <ttenumstr.h>          // ttCEnumStr
#include <ttlist.h>             // ttCList
#include <ttreg.h>              // ttCRegistry

#include "csrcfiles.h"          // CSrcFiles
#include "resource.h"

static const char* txtProperties =
    "{\n"
    "    \"configurations\": [\n"
    "        {\n"
    "            \"name\": \"Win32\",\n"
    "             \"defines\": [\n"
    "             ],\n"
    "             \"windowsSdkVersion\": \"%sdk_ver%\",\n"
    "             \"compilerPath\": \"%cl_path%\",\n"
    "             \"cStandard\": \"c11\",\n"
    "             \"cppStandard\": \"c++17\",\n"
//    "             \"intelliSenseMode\": \"msvc-x64\",\n"
    "             \"includePath\": [\n"
    "             ]\n"
    "         }\n"
    "     ],\n"
    "     \"version\": 4\n"
    "}\n"
    ;

static const char* txtLaunch =
    "{\n"
    "   // Use IntelliSense to learn about possible attributes.\n"
    "   // Hover to view descriptions of existing attributes.\n"
    "   // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387\n"
    "   \"version\": \"0.2.0\",\n"
    "   \"configurations\": [\n"
    "      {\n"
    "         \"name\": \"Debug %proj%\",\n"
    "         \"type\": \"cppvsdbg\",\n"
    "         \"request\": \"launch\",\n"
    "         \"cwd\" : \"${workspaceRoot}\",\n"
    "         \"program\": \"${workspaceRoot}/%targetD%\",\n"
    "         \"args\": [ \"\" ],\n"
    "         \"externalConsole\": true,\n"
    "         \"logging\": {\n"
    "            \"moduleLoad\": false,\n"
    "         },\n"
    "         \"visualizerFile\": \"${workspaceRoot}/%natvis%\",\n"
    "         \"stopAtEntry\": true,    // whether to stop or not at program entry point\n"
    "         \"preLaunchTask\": \"%bld%\"\n"
    "      }\n"
    "   ]\n"
    "}\n"
    ;

static const char* txtTasks =
    "{\n"
    "    // See https://go.microsoft.com/fwlink/?LinkId=733558\n"
    "    // for the documentation about the tasks.json format\n"
    "    \"version\": \"2.0.0\",\n"
    "    \"tasks\": [\n"
    "    ]\n"
    "}\n"
    ;

static const char* txtSubTasks =
    "        {\n"
    "            \"label\": \"%label%\",\n"
    "            \"type\": \"shell\",\n"
    "            \"command\": \"%command%\",\n"
    "            \"group\": \"%group%\",\n"
    "            \"problemMatcher\": [ %problem% ]\n"
    "        },\n"
    ;

static const char* txtDefaultTask =
    "        {\n"
    "            \"label\": \"%label%\",\n"
    "            \"type\": \"shell\",\n"
    "            \"command\": \"%command%\",\n"
    "            \"group\": {\n"
    "                \"kind\": \"%group%\",\n"
    "                \"isDefault\": true\n"
    "            },\n"
    "            \"problemMatcher\": [ %problem% ]\n"
    "        },\n"
    ;

bool CreateVsCodeProps(CSrcFiles& cSrcFiles, ttCList* plstResults);
bool CreateVsCodeLaunch(CSrcFiles& cSrcFiles, ttCList* plstResults);
bool CreateVsCodeTasks(CSrcFiles& cSrcFiles, ttCList* plstResults);

bool UpdateVsCodeProps(CSrcFiles& cSrcFiles, ttCList* plstResults);

bool CreateVsCodeProject(ttCList* plstResults)      // returns true unless unable to write to a file
{
    if (!ttDirExists(".vscode"))
    {
        if (!ttCreateDir(".vscode"))
        {
            ttMsgBox(TRANSLATE("Unable to create the required .vscode directory."));
            return false;
        }
    }

    if (!ttFileExists(".vscode/srcfiles.yaml"))
    {
        if (!Yamalize())
        {
            ttMsgBox(TRANSLATE("Unable to create the file .vscode/srcfiles.yaml"));
            return 1;
        }
        else if (plstResults)
            *plstResults += TRANSLATE("Created .vscode/srcfiles.yaml");
    }

    CSrcFiles cSrcFiles(true);
    if (!cSrcFiles.ReadFile())
    {
        ttCStr cszMsg;
        cszMsg.printf(GETSTRING(IDS_NINJA_CANNOT_LOCATE), (char*) cSrcFiles.m_cszSrcFilePath);
        ttMsgBox(cszMsg);
        return false;
    }

    ttCFile kf;

    if (ttFileExists(".vscode/c_cpp_properties.json"))
    {
        if (!UpdateVsCodeProps(cSrcFiles, plstResults))
            return false;
    }
    else
    {
        if (!CreateVsCodeProps(cSrcFiles, plstResults))
            return false;
    }

    if (!ttFileExists(".vscode/launch.json"))
    {
        if (!CreateVsCodeLaunch(cSrcFiles, plstResults))
            return false;
    }

    if (!ttFileExists(".vscode/tasks.json"))
    {
        if (!CreateVsCodeTasks(cSrcFiles, plstResults))
            return false;
    }

    return true;
}

bool CreateVsCodeProps(CSrcFiles& cSrcFiles, ttCList* plstResults)
{
    ttCFile kf;

    // Read the array of lines, write them into kf so it looks like it was read from a file

    kf.ReadStrFile(txtProperties);

    bool bCompilerFound = false;
    ttCStr cszPath;

    // clang.exe or gcc.exe are preferred since the PATH rarely changes and it works on all platforms

#if defined(_WIN32)
    if (FindFileEnv("PATH", "clang-cl.exe", cszPath))
    {
        kf.ReplaceStr("%cl_path%", cszPath);
        bCompilerFound = true;
    }
    else if (FindFileEnv("PATH", "clang.exe", cszPath))
#else
    if (FindFileEnv("PATH", "clang.exe", cszPath))
#endif
    {
        kf.ReplaceStr("%cl_path%", cszPath);
        bCompilerFound = true;
    }
    else if (FindFileEnv("PATH", "gcc.exe", cszPath))
    {
        kf.ReplaceStr("%cl_path%", cszPath);
        bCompilerFound = true;
    }
    if (!bCompilerFound)
    {
#if defined(_WIN32)
        ttCStr cszMSVC;
        if (FindCurMsvcPath(cszMSVC))
        {
            bool bx64 = IsHost64();
            cszMSVC.AppendFileName("bin/");
            cszMSVC.AppendFileName(bx64 ? "Hostx64/" : "Hostx86/");
            cszMSVC.AppendFileName(bx64 ? "x64/cl.exe" : "x86/cl.exe");
            ttBackslashToForwardslash(cszMSVC);

            // This gets us the correct version for now, but the next time the user updates Visual Studio, it will be
            // wrong until the user updates the file by hand, or runs MakeNinja -vscode

            kf.ReplaceStr("%cl_path%", cszMSVC);
        }
        else
#endif
        {
            kf.ReplaceStr("%cl_path%", "cl.exe");
            puts(GETSTRING(IDS_CANT_FIND_COMPILER));
        }
    }

    // The original template specifies the installed SDK version. I'm not sure why that matters, but until we can confirm that it
    // doesn't matter, we'll need to grab the version from the registry: HKEY_CLASSES_ROOT\Installer\Dependencies\Microsoft.Windows.WindowsSDK.x86.10\Version

    bool bSdkFound = false;

#if defined(_WIN32)
    ttCRegistry reg;
    if (reg.Open(HKEY_CLASSES_ROOT, "Installer\\Dependencies\\Microsoft.Windows.WindowsSDK.x86.10", false))
    {
        char szVersion[MAX_PATH];
        if (reg.ReadString("version", szVersion, sizeof(szVersion)))
        {
            kf.ReplaceStr("%sdk_ver%", szVersion);
            bSdkFound = true;
        }
    }
#endif    // defined(_WIN32)
    if (!bSdkFound)
        kf.ReplaceStr("%sdk_ver%", "10.0.17763.0");     // make a guess, probably wrong, but will also probably work fine

    ttCFile kfOut;
    while (kf.ReadLine())
    {
        if (ttIsSameSubStrI(ttFindNonSpace(kf), "\042defines"))
        {
            kfOut.WriteEol(kf);
            ttCList lstDefines;
            if (cSrcFiles.GetOption(OPT_CFLAGS_CMN))
                ParseDefines(lstDefines, cSrcFiles.GetOption(OPT_CFLAGS_CMN));
            if (cSrcFiles.GetOption(OPT_CFLAGS_DBG))
                ParseDefines(lstDefines, cSrcFiles.GetOption(OPT_CFLAGS_DBG));
            for (size_t pos = 0; lstDefines.InRange(pos); ++pos)
                kfOut.printf("                %kq,\n", lstDefines[pos]);

            // we always define _DEBUG. Under Windows, we always define _WIN32.

#if defined(_WIN32)
            kfOut.WriteEol("                \042_WIN32\042,");
#endif
            kfOut.WriteEol("                \042_DEBUG\042");
            continue;
        }

        else if (ttIsSameSubStrI(ttFindNonSpace(kf), "\042includePath") && cSrcFiles.GetOption(OPT_INC_DIRS))
        {
            kfOut.WriteEol(kf);
            while (kf.ReadLine())   // find the end of the current list of includes
            {
                if (!ttIsSameSubStr(ttFindNonSpace(kf), "]"))
                    kfOut.WriteEol(kf);
                else
                    break;
            }

            ttCEnumStr enumInc(cSrcFiles.GetOption(OPT_INC_DIRS));
            while (enumInc.Enum())
                kfOut.printf("                %kq,\n", (const char*) enumInc);

#if defined(_WIN32)
            ttCStr cszMSVC;
            if (FindCurMsvcPath(cszMSVC))
            {
                cszMSVC.AppendFileName("include");
                ttForwardslashToBackslash(cszPath); // just to be certain that they are consistent
                kfOut.printf("                %kq,\n", (const char*) cszMSVC);
            }
#endif

            // we always add the default include path
            kfOut.WriteEol("                \042${default}\042");
            kfOut.WriteEol(kf);
            continue;
        }
        else
            kfOut.WriteEol(kf);
    }

    if (!kfOut.WriteFile(".vscode/c_cpp_properties.json"))
    {
        ttMsgBoxFmt(GETSTRING(IDS_NINJA_CANT_WRITE), MB_OK | MB_ICONWARNING, ".vscode/c_cpp_properties.json");
        return false;
    }
    else
    {
        if (plstResults)
            *plstResults += TRANSLATE("Created .vscode/c_cpp_properties.json");
    }

    return true;
}

bool CreateVsCodeLaunch(CSrcFiles& cSrcFiles, ttCList* plstResults)
{
    if (cSrcFiles.IsExeTypeLib() || cSrcFiles.IsExeTypeDll())
        return true; // nothing that we know how to launch if this is a library or dynamic link library

    ttCFile kf;

    // Read the array of lines, write them into kf so it looks like it was read from a file

    kf.ReadStrFile(txtLaunch);

    kf.ReplaceStr("%proj%", cSrcFiles.GetProjectName());

    ttCStr cszTarget;
#ifdef _WIN32
    cszTarget.printf("Build %s (debug) using MSVC", ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
        cSrcFiles.GetTargetDebug64() : cSrcFiles.GetTargetDebug32()));
    kf.ReplaceStr("%bld%", cszTarget);
#else
    cszTarget.printf("Build %s (debug) using CLANG", ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
        cSrcFiles.GetTargetDebug64() : cSrcFiles.GetTargetDebug32()));
    kf.ReplaceStr("%bld%", cszTarget);
#endif

    if (cSrcFiles.GetBoolOption(OPT_64BIT))
        kf.ReplaceStr("%targetD%", cSrcFiles.GetTargetDebug64());
    else
        kf.ReplaceStr("%targetD%", cSrcFiles.GetTargetDebug32());

    // REVIEW: [randalphwa - 7/19/2019] Will it work to have a non-existant default.natvis file or will VS Code complain?
    // An alternative would be to insert/remove the entire line
    kf.ReplaceStr("%natvis%", cSrcFiles.GetOption(OPT_NATVIS) ? cSrcFiles.GetOption(OPT_NATVIS) : "default.natvis");

    if (!kf.WriteFile(".vscode/launch.json"))
    {
        ttMsgBoxFmt(GETSTRING(IDS_NINJA_CANT_WRITE), MB_OK | MB_ICONWARNING, ".vscode/launch.json");
        return false;
    }
    else
    {
        if (plstResults)
            *plstResults += TRANSLATE("Created .vscode/launch.json");
    }

    return true;
}

bool CreateVsCodeTasks(CSrcFiles& cSrcFiles, ttCList* plstResults)
{
    ttCFile kfTask, kfSubTask, kfOut;

    // Read the array of lines, write them into kf so it looks like it was read from a file

    kfTask.ReadStrFile(txtTasks);

    while (kfTask.ReadLine())
    {
        if (ttIsSameSubStrI(ttFindNonSpace(kfTask), "\042tasks"))
        {
            kfOut.WriteEol(kfTask);

            // [KeyWorks - 7/21/2019] Currently the only group names that VS Code allows are "build" and "test"

#if defined(_WIN32)   // MSVC compiler is only available on Windows

            // Build MSVC debug

            kfSubTask.ReadStrFile(txtDefaultTask);

            ttCStr cszTarget;
            cszTarget.printf("Build %s (debug) using MSVC", ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
                    cSrcFiles.GetTargetDebug64() : cSrcFiles.GetTargetDebug32()));

            kfSubTask.ReplaceStr("%label%", cszTarget);
            kfSubTask.ReplaceStr("%group%", "build");
            kfSubTask.ReplaceStr("%command%", "nmake.exe -nologo private=1 cmplr=msvc debug");
            kfSubTask.ReplaceStr("%problem%", "\042$msCompile\042");

            while (kfSubTask.ReadLine())
                kfOut.WriteEol(kfSubTask);
#endif    // defined(_WIN32)

            // Build CLANG debug

#if defined(_WIN32)
            kfSubTask.Delete();
            kfSubTask.ReadStrFile(txtSubTasks);
#else
            // On non-Windows, default to CLANG build
            kfSubTask.ReadStrFile(txtDefaultTask);
#endif

            cszTarget.printf("Build %s (debug) using CLANG", ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
                    cSrcFiles.GetTargetDebug64() : cSrcFiles.GetTargetDebug32()));

            kfSubTask.ReplaceStr("%label%", cszTarget);
            kfSubTask.ReplaceStr("%group%", "build");
            kfSubTask.ReplaceStr("%command%", "nmake.exe -nologo private=1 cmplr=clang debug");
            kfSubTask.ReplaceStr("%problem%", "\042$msCompile\042");

            while (kfSubTask.ReadLine())
                kfOut.WriteEol(kfSubTask);

#if defined(_WIN32)   // MSVC compiler is only available on Windows

            // Build MSVC release

            kfSubTask.Delete();
            kfSubTask.ReadStrFile(txtSubTasks);

            cszTarget.printf("Build %s (release) using MSVC", ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
                    cSrcFiles.GetTargetRelease64() : cSrcFiles.GetTargetRelease32()));

            kfSubTask.ReplaceStr("%label%", cszTarget);
            kfSubTask.ReplaceStr("%group%", "build");
            kfSubTask.ReplaceStr("%command%", "nmake.exe -nologo private=1 cmplr=msvc release");
            kfSubTask.ReplaceStr("%problem%", "\042$msCompile\042");

            while (kfSubTask.ReadLine())
                kfOut.WriteEol(kfSubTask);
#endif

            // Build CLANG release

            kfSubTask.Delete();
            kfSubTask.ReadStrFile(txtSubTasks);

            cszTarget.printf("Build %s (release) using CLANG", ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
                    cSrcFiles.GetTargetRelease64() : cSrcFiles.GetTargetRelease32()));

            kfSubTask.ReplaceStr("%label%", cszTarget);
            kfSubTask.ReplaceStr("%group%", "build");
            kfSubTask.ReplaceStr("%command%", "nmake.exe -nologo private=1 cmplr=clang release");
            kfSubTask.ReplaceStr("%problem%", "\042$msCompile\042");

            while (kfSubTask.ReadLine())
                kfOut.WriteEol(kfSubTask);

            // Test: Run target (debug)

            kfSubTask.Delete();
            kfSubTask.ReadStrFile(txtSubTasks);

            if (cSrcFiles.IsExeTypeWindow())
                kfSubTask.ReplaceStr("shell", "process");

            cszTarget.printf("Run %s (debug)", cSrcFiles.GetBoolOption(OPT_64BIT) ?
                    cSrcFiles.GetTargetDebug64() : cSrcFiles.GetTargetDebug32());

            kfSubTask.ReplaceStr("%label%", cszTarget);
            kfSubTask.ReplaceStr("%group%", "test");
            kfSubTask.ReplaceStr("%command%", cSrcFiles.GetBoolOption(OPT_64BIT) ?
                                                cSrcFiles.GetTargetDebug64() : cSrcFiles.GetTargetDebug32());
            kfSubTask.ReplaceStr("%problem%", "");

            while (kfSubTask.ReadLine())
                kfOut.WriteEol(kfSubTask);

#if defined(_WIN32)   // MSVC compiler is only available on Windows

            // Test: clean MSVC release targets

            kfSubTask.Delete();
            kfSubTask.ReadStrFile(txtSubTasks);

            kfSubTask.ReplaceStr("%label%", "Clean MSVC release targets");
            kfSubTask.ReplaceStr("%group%", "test");
            kfSubTask.ReplaceStr("%command%", "nmake.exe -nologo private=1 cmplr=msvc cleanR");
            kfSubTask.ReplaceStr("%problem%", "");

            while (kfSubTask.ReadLine())
                kfOut.WriteEol(kfSubTask);

#endif

            // Test: clean CLANG release targets

            kfSubTask.Delete();
            kfSubTask.ReadStrFile(txtSubTasks);

            kfSubTask.ReplaceStr("%label%", "Clean CLANG release targets");
            kfSubTask.ReplaceStr("%group%", "test");
            kfSubTask.ReplaceStr("%command%", "nmake.exe -nologo private=1 cmplr=clang cleanR");
            kfSubTask.ReplaceStr("%problem%", "");

            while (kfSubTask.ReadLine())
                kfOut.WriteEol(kfSubTask);

#if defined(_WIN32)   // MSVC compiler is only available on Windows

            // Test: View MSVC build commands

            kfSubTask.Delete();
            kfSubTask.ReadStrFile(txtSubTasks);

            kfSubTask.ReplaceStr("%label%", "View MSVC debug build commands");
            kfSubTask.ReplaceStr("%group%", "test");
            kfSubTask.ReplaceStr("%command%", "nmake.exe -nologo private=1 cmplr=msvc commandsD");
            kfSubTask.ReplaceStr("%problem%", "");

            while (kfSubTask.ReadLine())
                kfOut.WriteEol(kfSubTask);
#endif

            // Test: View CLANG build commands

            kfSubTask.Delete();
            kfSubTask.ReadStrFile(txtSubTasks);

            kfSubTask.ReplaceStr("%label%", "View CLANG debug build commands");
            kfSubTask.ReplaceStr("%group%", "test");
            kfSubTask.ReplaceStr("%command%", "nmake.exe -nologo private=1 cmplr=clang commandsD");
            kfSubTask.ReplaceStr("%problem%", "");

            while (kfSubTask.ReadLine())
                kfOut.WriteEol(kfSubTask);

            // Test View debug targets

            kfSubTask.Delete();
            kfSubTask.ReadStrFile(txtSubTasks);

            kfSubTask.ReplaceStr("%label%", "View debug targets");
            kfSubTask.ReplaceStr("%group%", "test");
            kfSubTask.ReplaceStr("%command%", "nmake.exe -nologo private=1 targetsD");
            kfSubTask.ReplaceStr("%problem%", "");

            while (kfSubTask.ReadLine())
                kfOut.WriteEol(kfSubTask);

            // Test View dependencies

            kfSubTask.Delete();
            kfSubTask.ReadStrFile(txtSubTasks);

            kfSubTask.ReplaceStr("%label%", "View debug dependencies");
            kfSubTask.ReplaceStr("%group%", "test");
            kfSubTask.ReplaceStr("%command%", "nmake.exe -nologo private=1 deps");
            kfSubTask.ReplaceStr("%problem%", "");

            while (kfSubTask.ReadLine())
                kfOut.WriteEol(kfSubTask);

            // Test Generate messages.po

            kfSubTask.Delete();
            kfSubTask.ReadStrFile(txtSubTasks);

            kfSubTask.ReplaceStr("%label%", "Generate messages.pos");
            kfSubTask.ReplaceStr("%group%", "test");
            kfSubTask.ReplaceStr("%command%", "nmake.exe -nologo private=1 locale");
            kfSubTask.ReplaceStr("%problem%", "");

            while (kfSubTask.ReadLine())
                kfOut.WriteEol(kfSubTask);
        }
        else
            kfOut.WriteEol(kfTask);
    }

#if !defined(_WIN32)
    // nmake.exe is a Windows-only app -- make.exe should be available on Unix/Mac platforms

    while (kfOut.ReplaceStr("nmake.exe -nologo", "make.exe"));
#endif

    if (!kfOut.WriteFile(".vscode/tasks.json"))
    {
        ttMsgBoxFmt(GETSTRING(IDS_NINJA_CANT_WRITE), MB_OK | MB_ICONWARNING, ".vscode/tasks.json");
        return false;
    }
    else
    {
        if (plstResults)
            *plstResults += TRANSLATE("Created .vscode/tasks.json");
    }

    return true;
}

bool UpdateVsCodeProps(CSrcFiles& cSrcFiles, ttCList* plstResults)
{
    ttCFile kfIn, kfOut;
    if (!kfIn.ReadFile(".vscode/c_cpp_properties.json"))
    {
        if (plstResults)
        {
            ttCStr cszErr;
            cszErr.printf(GETSTRING(IDS_NINJA_CANNOT_OPEN), ".vscode/tasks.json");
            *plstResults += cszErr;
            return false;
        }
    }
    kfIn.MakeCopy();    // make a copy for comparing with later

    // Gather all of our include directories into a list

    ttCEnumStr enumInc(cSrcFiles.GetOption(OPT_INC_DIRS));
    ttCList lstIncludes;
    lstIncludes.SetFlags(ttCList::FLG_IGNORE_CASE);
    while (enumInc.Enum())
        lstIncludes += enumInc;

#if defined(_WIN32)
    ttCStr cszMSVC;
    if (FindCurMsvcPath(cszMSVC))
    {
        cszMSVC.AppendFileName("include");
        ttBackslashToForwardslash(cszMSVC); // so we don't have to escape all the backslashes
        lstIncludes += cszMSVC;
    }
#endif

    // Gather all the commonad and debug defines specified in CFlags: and CFlagsD:

    ttCList lstDefines;
    if (cSrcFiles.GetOption(OPT_CFLAGS_CMN))
        ParseDefines(lstDefines, cSrcFiles.GetOption(OPT_CFLAGS_CMN));
    if (cSrcFiles.GetOption(OPT_CFLAGS_DBG))
        ParseDefines(lstDefines, cSrcFiles.GetOption(OPT_CFLAGS_DBG));

    // BUGBUG: [KeyWorks - 7/27/2019] This is a short-term hack until we have hooked up proper routines for
    // reading/writing .json files. While .json files are typically written out as line-oriented files, there's no
    // requirement for each element to be on it's own line, which is what we require if we're reading line by line. So,
    // if we're reading a file we generated that the user didn't touch, we're fine. But if the user decides to edit the
    // file, we're likely to either lose their changes, or break entirely -- until we have a json class in place.

    while (kfIn.ReadLine())
    {
        // Normally defines and includePath place each argument on its own line, but it doesn't have to be done that way. They could all be on a single line,
        // or they could be on multiple lines interspersed with comment lines, blank lines, etc. For now, we'll assume it wasn't hand-edited...

        if (ttStrStrI(kfIn, "\"defines\""))
        {
            kfOut.WriteEol(kfIn);
            if (ttStrChr(kfIn, CH_RIGHT_BRACKET))
                continue;   // all on one line, we don't process it
            while (kfIn.ReadLine())
            {
                if (ttStrChr(kfIn, CH_RIGHT_BRACKET))
                    break;
                const char* pszDef = ttStrChr(kfIn, CH_QUOTE);
                if (!pszDef)
                    continue;
                ttCStr cszDef;
                cszDef.GetQuotedString(pszDef);
                lstDefines += cszDef;   // this will only get added if it isn't a duplicate
            }

            if (lstDefines.GetCount() > 1)
            {
                for (size_t pos = 0; pos < lstDefines.GetCount() - 1; ++pos)
                    kfOut.printf("                %kq,\n", lstDefines[pos]);
            }

            // Write the last one without a comma
            if (lstDefines.GetCount() > 0)
                kfOut.printf("                %kq\n", lstDefines[lstDefines.GetCount() - 1]);

            kfOut.WriteEol  ("            ],");
            continue;
        }

        else if (ttStrStrI(kfIn, "\"includePath\""))
        {
            kfOut.WriteEol(kfIn);
            if (ttStrChr(kfIn, CH_RIGHT_BRACKET))
                continue;   // all on one line, we don't process it
            while (kfIn.ReadLine())
            {
                if (ttStrChr(kfIn, CH_RIGHT_BRACKET))
                    break;
                const char* pszDef = ttStrChr(kfIn, CH_QUOTE);
                if (!pszDef)
                    continue;
                ttCStr cszDef;
                cszDef.GetQuotedString(pszDef);
                lstIncludes += cszDef;   // this will only get added if it isn't a duplicate
            }

            if (lstIncludes.GetCount() > 1)
            {
                for (size_t pos = 0; pos < lstIncludes.GetCount() - 1; ++pos)
                    kfOut.printf("                %kq,\n", lstIncludes[pos]);
            }

            // Write the last one without a comma
            if (lstIncludes.GetCount() > 0)
                kfOut.printf("                %kq\n", lstIncludes[lstIncludes.GetCount() - 1]);

            kfOut.WriteEol  ("            ]");
            continue;
        }
        else if (ttStrStrI(kfIn, "compilerPath") && ttStrStrI(kfIn, "cl.exe"))
        {
            // MSVC can change the path frequently (sometimes once a week).
            if (FindCurMsvcPath(cszMSVC))
            {
                bool bx64 = IsHost64();
                cszMSVC.AppendFileName("bin/");
                cszMSVC.AppendFileName(bx64 ? "Hostx64/" : "Hostx86/");
                cszMSVC.AppendFileName(bx64 ? "x64/cl.exe" : "x86/cl.exe");
                ttBackslashToForwardslash(cszMSVC);

                char* pszSep = ttStrChr(kfIn, ':');
                if (pszSep)
                {
                    pszSep = ttStrChr(pszSep, CH_QUOTE);
                    if (pszSep)
                    {
                        ttCStr cszOld;
                        cszOld.GetQuotedString(pszSep);
                        ttBackslashToForwardslash(cszOld);
                        if (!ttIsSameSubStrI(cszOld, cszMSVC))
                        {
                            ttCStr cszNewLine;
                            cszNewLine.printf("             \"compilerPath\": \"%s\",", (char*) cszMSVC);
                            kfOut.WriteEol(cszNewLine);
                            continue;
                        }
                    }
                }
            }
        }

        kfOut.WriteEol(kfIn);
    }

    kfIn.RestoreCopy(); // Restore file from temporary copy
    if (strcmp(kfIn, kfOut) != 0)    // Only write if something changed
    {
        if (!kfOut.WriteFile(".vscode/c_cpp_properties.json"))
            return false;
        else if (plstResults)
            *plstResults += TRANSLATE("Updated .vscode/c_cpp_properties.json");
    }

    return true;
}

// Given a string, finds any definitions and stores them in the provided list. Primarily used to parse a CFlags: option string

void ParseDefines(ttCList& lst, const char* pszDefines)
{
    const char* pszStart = pszDefines;
    while (*pszStart)
    {
        if (*pszStart == '-' || *pszStart != CH_FORWARDSLASH)
        {
            ++pszStart;
            if (*pszStart != 'D')   // note that this option is case sensitive
                continue;   // it's not a define
            ttCStr cszTmp;
            cszTmp.GetString(pszStart, *pszStart, CH_SPACE);
            lst += cszTmp;
            pszStart += ttStrLen(cszTmp);
        }
    }
}
