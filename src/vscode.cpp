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

static void AddTask(ttCFile& fileOut, const char* pszLabel, const char* pszGroup, const char* pszCommand, const char* pszProblem);

static const char* txtProperties =
    "{\n"
    "    \"configurations\": [\n"
    "        {\n"
    "            \"name\": \"Default\",\n"
    "             \"defines\": [\n"
    "             ],\n"
    "             \"cStandard\": \"c11\",\n"
    "             \"cppStandard\": \"c++17\",\n"
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

static const char* txtDefTaskName = _XGET("Build %s (debug) using %s");

bool CreateVsCodeProps(CSrcFiles& cSrcFiles, ttCList* plstResults);
bool CreateVsCodeLaunch(CSrcFiles& cSrcFiles, ttCList* plstResults);
bool CreateVsCodeTasks(CSrcFiles& cSrcFiles, ttCList* plstResults);

bool UpdateVsCodeProps(CSrcFiles& cSrcFiles, ttCList* plstResults);

bool CreateVsCodeProject(const char* pszSrcFiles, ttCList* plstResults)      // returns true unless unable to write to a file
{
    if (!ttDirExists(".vscode"))
    {
        if (!ttCreateDir(".vscode"))
        {
            ttMsgBox(TRANSLATE("Unable to create the required .vscode directory."));
            return false;
        }
    }

    CSrcFiles cSrcFiles;
    if (!cSrcFiles.ReadFile(pszSrcFiles))
    {
        if (plstResults)
            *plstResults += TRANSLATE("Cannot locate a .srcfiles.yaml file need to configure .vscode/*.json files.");
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
    ttCFile kf, kfOut;

    kf.ReadStrFile(txtProperties);
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
        // Note that we can't use IDS_FILE_CREATED since that string adds \n to the end.
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

    // REVIEW: [KeyWorks - 7/30/2019] If we ever add the ability to set a default compiler for makefile, then should
    // change this. For now we'll default to MSVC on Windows.

#ifdef _WIN32
    cszTarget.printf(TRANSLATE(txtDefTaskName), // "Build %s (debug) using %s"
        ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
        cSrcFiles.GetTargetDebug64() : cSrcFiles.GetTargetDebug32()),
        "MSVC");
    kf.ReplaceStr("%bld%", cszTarget);
#else
    // [KeyWorks - 7/30/2019] This is a placeholder -- it needs to be set depending on whether clang.exe is actually
    // available. Use GCC if not.

    cszTarget.printf(TRANSLATE(txtDefTaskName), // "Build %s (debug) using %s"
        ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
        cSrcFiles.GetTargetDebug64() : cSrcFiles.GetTargetDebug32())
        "CLANG");
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


// https://vscode-docs.readthedocs.io/en/latest/editor/tasks/

// For clang-cl.exe we definitely need to provide a problem matcher since the $mscompiler one doesn't work. We will
// probably need to add it for our regular builds as well if the makefile is not in the same location as the current
// directory. Reason is that $mscompile assumes an absolute path, but all of the compilers are outputing a relative path.

bool CreateVsCodeTasks(CSrcFiles& cSrcFiles, ttCList* plstResults)
{
    ttCFile kfTask, kfSubTask, kfOut;

    ttCStr cszMakeFileOption("");
    {
        char* pszFilePortion = ttFindFilePortion(cSrcFiles.GetSrcFiles());
        if (pszFilePortion != cSrcFiles.GetSrcFiles())
        {
            cszMakeFileOption += "-f ";
            cszMakeFileOption += cSrcFiles.GetSrcFiles();
            pszFilePortion = ttFindFilePortion(cszMakeFileOption);
            *pszFilePortion = 0;
            cszMakeFileOption.AppendFileName("makefile");
        }
    }

    // Read the array of lines, write them into kf so it looks like it was read from a file

    kfTask.ReadStrFile(txtTasks);

    while (kfTask.ReadLine())
    {
        if (ttIsSameSubStrI(ttFindNonSpace(kfTask), "\042tasks"))
        {
            kfOut.WriteEol(kfTask);

            // First set the default task. This is the only one where the property "isDefault" is set

            kfSubTask.ReadStrFile(txtDefaultTask);
            ttCStr cszLabel, cszMakeCommand;

#if defined(_WIN32)

            // Build MSVC debug

            cszLabel.printf(TRANSLATE(txtDefTaskName), // "Build %s (debug) using %s"
                ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
                cSrcFiles.GetTargetDebug64() : cSrcFiles.GetTargetDebug32()),
                "MSVC");

            kfSubTask.ReplaceStr("%label%", cszLabel);
            kfSubTask.ReplaceStr("%group%", "build");

            cszMakeCommand.printf("nmake.exe -nologo debug %s", (char*) cszMakeFileOption);
            kfSubTask.ReplaceStr("%command%", cszMakeCommand);
            kfSubTask.ReplaceStr("%problem%", "\042$msCompile\042");

            while (kfSubTask.ReadLine())
                kfOut.WriteEol(kfSubTask);
#else
            bool bClangAvail = FindFileEnv("PATH", "clang.exe");

            cszLabel.printf(TRANSLATE(txtDefTaskName), // "Build %s (debug) using %s"
                ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
                cSrcFiles.GetTargetDebug64() : cSrcFiles.GetTargetDebug32())
                bClangAvail ? "CLANG" : "GCC");

            kfSubTask.ReplaceStr("%label%", cszLabel);
            kfSubTask.ReplaceStr("%group%", "build");

            cszMakeCommand.printf("make debug cmplr=clang %s", (char*) cszMakeFileOption);
            kfSubTask.ReplaceStr("%command%", cszMakeCommand);
            if (!bClangAvail)
                kfSubTask.ReplaceStr("cmplr=clang", "cmplr=gcc");

            kfSubTask.ReplaceStr("%problem%", "\042$gcc\042");

            while (kfSubTask.ReadLine())
                kfOut.WriteEol(kfSubTask);
#endif

            // To prevent writing the same code multiple times, we write it once assuming nmake and MSVC compiler. If we're not on Windows, then
            // before we write the file, we replace nmake.exe with make.exe, and MSVC with either CLANG or GCC.

            // Build MSVC release

            cszLabel.printf("Build %s (release) using MSVC", ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
                cSrcFiles.GetTargetRelease64() : cSrcFiles.GetTargetRelease32()));
            cszMakeCommand.printf("nmake.exe -nologo release %s", (char*) cszMakeFileOption);
            AddTask(kfOut, cszLabel, "build", cszMakeCommand, "\042$msCompile\042");

            // Rebuild MSVC release

            cszLabel.printf("Rebuild %s (release) using MSVC", ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
                cSrcFiles.GetTargetRelease64() : cSrcFiles.GetTargetRelease32()));
            cszMakeCommand.printf("nmake.exe -nologo clean release %s", (char*) cszMakeFileOption);
            AddTask(kfOut, cszLabel, "build", cszMakeCommand, "\042$msCompile\042");

            // Test: View MSVC build commands

            cszMakeCommand.printf("nmake.exe -nologo commandsD %s", (char*) cszMakeFileOption);
            AddTask(kfOut, "View MSVC debug build commands", "test", cszMakeCommand, "");

            // Test View debug targets

            cszMakeCommand.printf("nmake.exe -nologo targetsD %s", (char*) cszMakeFileOption);
            AddTask(kfOut, "View MSVC debug targets", "test", cszMakeCommand, "");

            // Test View dependencies

            cszMakeCommand.printf("nmake.exe -nologo deps %s", (char*) cszMakeFileOption);
            AddTask(kfOut, "View debug dependencies", "test", cszMakeCommand, "");

            // Test Generate messages.po

            cszMakeCommand.printf("nmake.exe -nologo locale %s", (char*) cszMakeFileOption);
            AddTask(kfOut, "Generate messages.pos", "test", cszMakeCommand, "");

#if defined(_WIN32)
            // If we're on Windows, then we also look to see if either the clang or gcc compiler is available. If so,
            // we add build targets for them (we only choose one, with clang being the first choice)/

            if (FindFileEnv("PATH", "mingw32-make.exe"))
            {
                if (FindFileEnv("PATH", "clang.exe"))
                {
                    cszLabel.printf("Build %s (debug) using CLANG", ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
                        cSrcFiles.GetTargetRelease64() : cSrcFiles.GetTargetRelease32()));
                    cszMakeCommand.printf("mingw32-make.exe debug %s", (char*) cszMakeFileOption);
                    AddTask(kfOut, cszLabel, "build", cszMakeCommand, "\042$gcc\042");

                    cszLabel.printf("Build %s (release) using CLANG", ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
                        cSrcFiles.GetTargetRelease64() : cSrcFiles.GetTargetRelease32()));
                    cszMakeCommand.printf("mingw32-make.exe release %s", (char*) cszMakeFileOption);
                    AddTask(kfOut, cszLabel, "build", cszMakeCommand, "\042$gcc\042");

                    cszLabel.printf("Rebuild %s (release) using CLANG", ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
                        cSrcFiles.GetTargetRelease64() : cSrcFiles.GetTargetRelease32()));
                    cszMakeCommand.printf("mingw32-make.exe clean release %s", (char*) cszMakeFileOption);
                    AddTask(kfOut, cszLabel, "build", cszMakeCommand, "\042$gcc\042");
                }
                else if (FindFileEnv("PATH", "gcc.exe"))
                {
                    cszLabel.printf("Build %s (debug) using GCC", ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
                        cSrcFiles.GetTargetRelease64() : cSrcFiles.GetTargetRelease32()));
                    cszMakeCommand.printf("mingw32-make.exe cmplr=gcc debug %s", (char*) cszMakeFileOption);
                    AddTask(kfOut, cszLabel, "build", cszMakeCommand, "\042$gcc\042");

                    cszLabel.printf("Build %s (release) using GCC", ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
                        cSrcFiles.GetTargetRelease64() : cSrcFiles.GetTargetRelease32()));
                    cszMakeCommand.printf("mingw32-make.exe cmplr=gcc release %s", (char*) cszMakeFileOption);
                    AddTask(kfOut, cszLabel, "build", cszMakeCommand, "\042$gcc\042");

                    cszLabel.printf("Rebuild %s (release) using GCC", ttFindFilePortion(cSrcFiles.GetBoolOption(OPT_64BIT) ?
                        cSrcFiles.GetTargetRelease64() : cSrcFiles.GetTargetRelease32()));
                    cszMakeCommand.printf("mingw32-make.exe cmplr=gcc clean release %s", (char*) cszMakeFileOption);
                    AddTask(kfOut, cszLabel, "build", cszMakeCommand, "\042$gcc\042");
                }
            }

#endif

#if !defined(_WIN32)
            if (FindFileEnv("PATH", "clang.exe"))
            {
                while (kfOut.ReplaceStr("nmake.exe -nologo", "make.exe cmplr=clang"));
                while (kfOut.ReplaceStr("MSVC", "CLANG"));
            }
            else
            {
                while (kfOut.ReplaceStr("nmake.exe -nologo", "make.exe cmplr=gcc"));
                while (kfOut.ReplaceStr("MSVC", "GCC"));
            }
            while (kfOut.ReplaceStr("$msCompile", "$gcc"));
#endif

        }
        else
            kfOut.WriteEol(kfTask);
    }

    if (!kfOut.WriteFile(".vscode/tasks.json"))
    {
        if (plstResults)
        {
            ttCStr cszMsg;
            cszMsg.printf(GETSTRING(IDS_NINJA_CANT_WRITE), ".vscode/tasks.json");
            *plstResults += (char*) cszMsg;
        }
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

static void AddTask(ttCFile& fileOut, const char* pszLabel, const char* pszGroup, const char* pszCommand, const char* pszProblem)
{
    ttCFile fileTask;
    fileTask.Delete();
    fileTask.ReadStrFile(txtSubTasks);

    fileTask.ReplaceStr("%label%", pszLabel);
    fileTask.ReplaceStr("%group%", pszGroup);
    fileTask.ReplaceStr("%command%", pszCommand);
    fileTask.ReplaceStr("%problem%", pszProblem);

    while (fileTask.ReadLine())
        fileOut.WriteEol(fileTask);
}
