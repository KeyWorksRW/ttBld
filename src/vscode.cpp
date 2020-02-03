/////////////////////////////////////////////////////////////////////////////
// Name:      vscode.cpp
// Purpose:   Creates/updates .vscode files
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttTR.h>        // Function for translating strings
#include <tttextfile.h>  // ttTextFile, ttViewFile -- Similar to wxTextFile, but uses UTF8 strings

#include <ttenumstr.h>   // ttCEnumStr
#include <ttlinefile.h>  // ttCLineFile -- Line-oriented file class
#include <ttlist.h>      // ttCList
#include <ttpath.h>      // Contains functions for working with filesystem::path and filesystem::directory

#include "csrcfiles.h"  // CSrcFiles
#include "funcs.h"      // List of function declarations
#include "resource.h"
#include "dlgvscode.h"  // CDlgVsCode -- IDDLG_VSCODE dialog handler

#if 0
static void AddTask(ttCFile& fileOut, const char* pszLabel, const char* pszGroup, const char* pszCommand,
                    const char* pszProblem);
#endif

static void AddMsvcTask(ttCFile& fileOut, const char* pszLabel, const char* pszGroup, const char* pszCommand);
static void AddClangTask(ttCFile& fileOut, const char* pszLabel, const char* pszGroup, const char* pszCommand);

static const char* txtProperties =

    "{\n"
    "    \"configurations\": [\n"
    "        {\n"
    "            \"name\": \"Default\",\n"
    "             \"cStandard\": \"c11\",\n"
    "             \"cppStandard\": \"c++17\",\n"
    "             \"defines\": [\n"
    "             ],\n"
    "             \"includePath\": [\n"
    "             ]\n"
    "         }\n"
    "     ],\n"
    "     \"version\": 4\n"
    "}\n";

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
    "}\n";

static const char* txtTasks =

    "{\n"
    "    // See https://go.microsoft.com/fwlink/?LinkId=733558\n"
    "    // for the documentation about the tasks.json format\n"
    "    \"version\": \"2.0.0\",\n"
    "    \"tasks\": [\n"
    "    ]\n"
    "}\n";

#if 0
static const char* txtSubTasks =

    "        {\n"
    "            \"label\": \"%label%\",\n"
    "            \"type\": \"shell\",\n"
    "            \"command\": \"%command%\",\n"
    "            \"group\": \"%group%\",\n"
    "            \"problemMatcher\": [ %problem% ]\n"
    "        },\n";
#endif

static const char* txtMsvcSubTasks =

    "        {\n"
    "            \"label\": \"%label%\",\n"
    "            \"type\": \"shell\",\n"
    "            \"options\": {\n"
    "                \"cwd\": \"${workspaceFolder}\"\n"
    "            },\n"
    "            \"command\": \"%command%\",\n"
    "%group%"
    "            \"problemMatcher\": {\n"
    "                \"base\": \"$msCompile\",\n"
    "                \"fileLocation\": [\n"
    "                    \"autoDetect\",\n"
    "                    \"${workspaceFolder}\"\n"
    "                ]\n"
    "            }\n"
    "        },\n";

static const char* txtClangSubTasks =
    "        {\n"
    "            \"label\": \"%label%\",\n"
    "            \"type\": \"shell\",\n"
    "            \"options\": {\n"
    "                \"cwd\": \"${workspaceFolder}\"\n"
    "            },\n"
    "            \"command\": \"%command%\",\n"
    "%group%"
    "            \"problemMatcher\": {\n"
    "                \"owner\": \"cpp\",\n"
    "                \"fileLocation\": [\n"
    "                    \"autoDetect\",\n"
    "                    \"${workspaceFolder}\"\n"
    "                ],\n"
    "                \"pattern\": {\n"
    "                    \"regexp\": "
    "\"^(.*)\\\\((\\\\d+),(\\\\d+)\\\\):\\\\s+(note|warning|error):\\\\s+(.*)$\",\n"
    "                    \"file\": 1,\n"
    "                    \"line\": 2,\n"
    "                    \"column\": 3,\n"
    "                    \"severity\": 4,\n"
    "                    \"message\": 5\n"
    "                }\n"
    "            }\n"
    "        },\n";

static const char* txtNormalGroup =

    "            \"group\": \"build\",\n";

static const char* txtDefaultGroup =

    "            \"group\": {\n"
    "                \"kind\": \"build\",\n"
    "                \"isDefault\": true\n"
    "            },\n";

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
    "        },\n";

bool CreateVsCodeProps(CSrcFiles& cSrcFiles, ttCList* plstResults);

bool UpdateVsCodeProps(CSrcFiles& cSrcFiles, ttCList* plstResults);

// Returns true unless unable to write to a file
bool CreateVsCodeProject(const char* pszSrcFiles, ttCList* plstResults)
{
    if (!tt::dirExists(".vscode"))
    {
        if (!ttCreateDir(".vscode"))
        {
            wxMessageBox(_tt("Unable to create the required .vscode directory."));
            return false;
        }
        ttCStr cszIgnore;
        if (!gitIsFileIgnored(cszIgnore, ".vscode/") && !gitIsExcluded(cszIgnore, ".vscode/"))
        {
            if (tt::dirExists(".git"))
                cszIgnore = ".git/info/exclude";
            else if (tt::dirExists("../.git"))
                cszIgnore = "../.git/info/exclude";
            else if (tt::dirExists("../../.git"))
                cszIgnore = "../../.git/info/exclude";

            if (cszIgnore.IsNonEmpty() &&
                ttMsgBoxFmt(
                    _tt("The directory .vscode is not being ignored by git. Would you like it to be added to %s?"),
                    MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING, (char*) cszIgnore) == IDYES)
            {
                if (gitAddtoIgnore(cszIgnore, ".vscode/") && plstResults)
                {
                    std::stringstream msg;
                    msg << _tt(".vscode/ added to ") << cszIgnore.c_str() << '\n';
                    *plstResults += msg.str().c_str();
                }
            }
        }
    }

    CSrcFiles cSrcFiles;
    if (!cSrcFiles.ReadFile(pszSrcFiles))
    {
        if (plstResults)
            *plstResults += _tt("Cannot locate a .srcfiles.yaml file need to configure .vscode/*.json files.");
        return false;
    }

    if (tt::fileExists(".vscode/c_cpp_properties.json"))
    {
        if (!UpdateVsCodeProps(cSrcFiles, plstResults))
            return false;
    }
    else
    {
        if (!CreateVsCodeProps(cSrcFiles, plstResults))
            return false;
    }

    if (!tt::fileExists(".vscode/launch.json") || !tt::fileExists(".vscode/tasks.json"))
    {
        CDlgVsCode dlg;
        if (dlg.DoModal(NULL) != IDOK)
            return false;

        if (!tt::fileExists(".vscode/launch.json"))
        {
            if (!dlg.CreateVsCodeLaunch(cSrcFiles, plstResults))
                return false;
        }

        if (!tt::fileExists(".vscode/tasks.json"))
        {
            if (!dlg.CreateVsCodeTasks(cSrcFiles, plstResults))
                return false;
        }
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
            lstDefines.Sort();
            for (size_t pos = 0; lstDefines.InRange(pos); ++pos)
                kfOut.printf("                %kq,\n", lstDefines[pos]);

                // we always define _DEBUG. Under Windows, we always define _WIN32.

#if defined(_WIN32)
            kfOut.WriteEol("                \042_WIN32\042,");
#endif
            kfOut.WriteEol("                \042_DEBUG\042");
            continue;
        }

        else if (ttIsSameSubStrI(ttFindNonSpace(kf), "\042includePath"))
        {
            kfOut.WriteEol(kf);
            while (kf.ReadLine())  // find the end of the current list of includes
            {
                if (!ttIsSameSubStr(ttFindNonSpace(kf), "]"))
                    kfOut.WriteEol(kf);
                else
                    break;
            }

            if (cSrcFiles.GetOption(OPT_INC_DIRS))
            {
                ttCEnumStr enumInc(cSrcFiles.GetOption(OPT_INC_DIRS));
                while (enumInc.Enum())
                {
                    ttCStr cszInclude(enumInc);
                    cszInclude.FullPathName();
#if defined(_WIN32)
                    ttCStr cszIncDir;
                    JunctionToReal(cszInclude, cszIncDir);
                    ttBackslashToForwardslash(cszIncDir);
                    kfOut.printf("                %kq,\n", (const char*) cszIncDir);
#else
                    ttBackslashToForwardslash(cszInclude);
                    kfOut.printf("                %kq,\n", (const char*) cszInclude);
#endif
                }
            }
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
        wxString msg(_("Unable to create or write to "));
        msg += ".vscode/c_cpp_properties.json";
        wxMessageBox(msg);
        return false;
    }
    else
    {
        if (plstResults)
            *plstResults += _tt("Created .vscode/c_cpp_properties.json");
    }

    return true;
}

bool CDlgVsCode::CreateVsCodeLaunch(CSrcFiles& cSrcFiles, ttCList* plstResults)
{
    if (cSrcFiles.IsExeTypeLib() || cSrcFiles.IsExeTypeDll())
        return true;  // nothing that we know how to launch if this is a library or dynamic link library

    ttCFile kf;

    // Read the array of lines, write them into kf so it looks like it was read from a file

    kf.ReadStrFile(txtLaunch);

    kf.ReplaceStr("%proj%", cSrcFiles.GetProjectName());

    ttCStr cszTarget;

#if defined(_WIN32)
    if (m_PreLaunch == PRELAUNCH_MAIN)
        kf.ReplaceStr("%bld%", "Build Debug MSVC");
    else if (m_PreLaunch == PRELAUNCH_CLANG)
        kf.ReplaceStr("%bld%", "Build Debug CLANG");
    else if (m_PreLaunch == PRELAUNCH_NINJA)
        kf.ReplaceStr("%bld%", "Ninja Debug Build");
    else
        kf.ReplaceStr("%bld%", "");
#else   // not defined(_WIN32)
    if (m_PreLaunch == PRELAUNCH_MAIN)
        kf.ReplaceStr("%bld%", "Build Debug GCC");
    else if (m_PreLaunch == PRELAUNCH_CLANG)
        kf.ReplaceStr("%bld%", "Build Debug CLANG");
    else if (m_PreLaunch == PRELAUNCH_NINJA)
        kf.ReplaceStr("%bld%", "Ninja Debug Build");
    else
        kf.ReplaceStr("%bld%", "");
#endif  // defined(_WIN32)

    kf.ReplaceStr("%targetD%", cSrcFiles.GetTargetDebug());

    // REVIEW: [randalphwa - 7/19/2019] Will it work to have a non-existant default.natvis file or will VS Code
    // complain? An alternative would be to insert/remove the entire line
    ttCStr cszPath(cSrcFiles.GetOption(OPT_NATVIS) ? cSrcFiles.GetOption(OPT_NATVIS) : "default.natvis");
    ttBackslashToForwardslash(cszPath);
    kf.ReplaceStr("%natvis%", cszPath);

    if (!kf.WriteFile(".vscode/launch.json"))
    {
        wxString msg(_("Unable to create or write to "));
        msg += ".vscode/launch.json";
        wxMessageBox(msg);
        return false;
    }
    else
    {
        if (plstResults)
            *plstResults += _tt("Created .vscode/launch.json");
    }

    return true;
}

bool CDlgVsCode::CreateVsCodeTasks(CSrcFiles& cSrcFiles, ttCList* plstResults)
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
            if (m_bMainTasks)
            {
#if defined(_WIN32)

                // Build MSVC debug

                cszMakeCommand.printf("nmake.exe -nologo debug %s", (char*) cszMakeFileOption);
                AddMsvcTask(kfOut, "Build Debug MSVC",
                            (m_DefTask == DEFTASK_MAIN) ? txtDefaultGroup : txtNormalGroup, cszMakeCommand);

#else
                cszMakeCommand.printf("make debug cmplr=gcc %s", (char*) cszMakeFileOption);
                AddClangTask(kfOut, "Build Debug GCC",
                             (m_DefTask == DEFTASK_MAIN) ? txtDefaultGroup : txtNormalGroup, cszMakeCommand);
#endif

                // To prevent writing the same code multiple times, we write it once assuming nmake and MSVC
                // compiler. If we're not on Windows, then before we write the file, we replace nmake.exe with
                // make.exe, and MSVC with either CLANG or GCC.

                // Build MSVC release

                cszLabel.printf("Build %s (release) using MSVC", ttFindFilePortion(cSrcFiles.GetTargetRelease()));

                cszMakeCommand.printf("nmake.exe -nologo release %s", (char*) cszMakeFileOption);
                AddMsvcTask(kfOut, "Build Release MSVC", txtNormalGroup, cszMakeCommand);

                // Rebuild MSVC release

                cszMakeCommand.printf("nmake.exe -nologo clean release %s", (char*) cszMakeFileOption);
                AddMsvcTask(kfOut, "Rebuild Release MSVC", txtNormalGroup, cszMakeCommand);
                if (m_bNinjaTask)
                    AddMsvcTask(kfOut, "Ninja Debug Build", txtNormalGroup, "ninja -f bld/msvc_dbg.ninja");
            }

            // REVIEW: [KeyWorks - 10-05-2019] Disabled until we have a method to determine if this task actually
            // applies to the current project.

#if 0
            if (FindFileEnv("PATH", "xgettext.exe"))
            {
                cszMakeCommand.printf("nmake.exe -nologo locale %s", (char*) cszMakeFileOption);
                AddTask(kfOut, "Build messages.pos", "test", cszMakeCommand, "");
            }
#endif

#if defined(_WIN32)
            // If we're on Windows, then we also look to see if either the clang-cl compiler is available. If so,
            // we add build targets for it

            if (m_bClangTasks)
            {
                cszMakeCommand.printf(m_bMake ? "mingw32-make.exe debug %s" : "nmake debug cmplr=clang %s",
                                      (char*) cszMakeFileOption);
                AddClangTask(kfOut, "Build Debug CLANG",
                             (m_DefTask == DEFTASK_CLANG) ? txtDefaultGroup : txtNormalGroup, cszMakeCommand);

                cszMakeCommand.printf(m_bMake ? "mingw32-make.exe release %s" : "nmake release cmplr=clang %s",
                                      (char*) cszMakeFileOption);
                AddClangTask(kfOut, "Build Release CLANG", txtNormalGroup, cszMakeCommand);

                cszMakeCommand.printf(m_bMake ? "mingw32-make.exe clean release %s" :
                                                "nmake clean release cmplr=clang %s",
                                      (char*) cszMakeFileOption);
                AddClangTask(kfOut, "Rebuild Release CLANG", txtNormalGroup, cszMakeCommand);
                if (m_bNinjaTask && !m_bMainTasks)
                    AddClangTask(kfOut, "Ninja Debug Build",
                                 (m_DefTask == DEFTASK_NINJA) ? txtDefaultGroup : txtNormalGroup,
                                 "ninja -f bld/clang_dbg.ninja");
            }
#endif

#if !defined(_WIN32)
            {
                while (kfOut.ReplaceStr("msvcBuild", "gccBuild"))
                    ;
                while (kfOut.ReplaceStr("clangBuild", "llvmBuild"))
                    ;
                while (kfOut.ReplaceStr("nmake.exe -nologo", "make cmplr=gcc"))
                    ;
                while (kfOut.ReplaceStr("mingw32-make.exe", "make"))
                    ;
                while (kfOut.ReplaceStr("MSVC", "GCC"))
                    ;
                while (kfOut.ReplaceStr("CLANG", "LLVM"))
                    ;
            }
            while (kfOut.ReplaceStr("$msCompile", "$gcc"))
                ;
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
            cszMsg.printf(_tt("Unable to create or write to %s"), ".vscode/tasks.json");
            *plstResults += (char*) cszMsg;
        }
        return false;
    }
    else
    {
        if (plstResults)
            *plstResults += _tt("Created .vscode/tasks.json");
    }

    return true;
}

bool UpdateVsCodeProps(CSrcFiles& cSrcFiles, ttCList* plstResults)
{
    ttTextFile file;
    if (!file.ReadFile(".vscode/c_cpp_properties.json"))
    {
        std::stringstream msg;
        msg << _tt("Cannot open ") << ".vscode/c_cpp_properties.json";
        *plstResults += msg.str().c_str();
        return false;
    }

    // Gather all of our include directories into a list

    ttStrVector Includes;

    if (!cSrcFiles.getOptValue(Opt::INC_DIRS).empty())
    {
        ttEnumStr IncludeDirs(cSrcFiles.getOptValue(Opt::INC_DIRS));
        for (auto iter : IncludeDirs)
        {
            // If it's not already a relative path, make it relative
            if (iter.at(0) != '.')
            {
                iter.make_relative(".");
            }
#if defined(_WIN32)

            iter.backslashestoforward();
#endif
            std::string workspace("${workspaceRoot}/");
            workspace += iter;
            Includes.addfilename(workspace);
        }
    }

#if 0
    // REVIEW: [KeyWorks - 02-03-2020] Because MSVC frequently changes the path to the header files,
    // adding this ensures that VS Code has access to the current header files. That works great for a local
    // copy of c_cpp_properties but wreaks havoc if it's under Source control management (e.g., git).
    #if defined(_WIN32)
    ttCStr cszMSVC;
    if (FindCurMsvcPath(cszMSVC))
    {
        cszMSVC.AppendFileName("include");
        ttBackslashToForwardslash(cszMSVC);  // so we don't have to escape all the backslashes
        Includes.append(cszMSVC.c_str());
    }
    #endif
#endif

    // Gather all the common and debug defines specified in CFlags: and CFlagsD:

    ttStrVector Defines;

    ttCList lstDefines;
    if (cSrcFiles.GetOption(OPT_CFLAGS_CMN))
        ParseDefines(lstDefines, cSrcFiles.GetOption(OPT_CFLAGS_CMN));
    if (cSrcFiles.GetOption(OPT_CFLAGS_DBG))
        ParseDefines(lstDefines, cSrcFiles.GetOption(OPT_CFLAGS_DBG));

    // Also add environment flags

    ttCStr cszEnv;
    if (cszEnv.GetEnv("CFLAGS"))
        ParseDefines(lstDefines, cszEnv);
    if (cszEnv.GetEnv("CFLAGSD"))
        ParseDefines(lstDefines, cszEnv);

    // REVIEW: [KeyWorks - 02-02-2020] Hack Alert!!! This only exists until lstDefines gets replaced, which
    // requires an updated version of ParseDefines().

    for (size_t hack = 0; hack < lstDefines.GetCount(); ++hack)
        Defines.append(lstDefines[hack]);

    // BUGBUG: [KeyWorks - 7/27/2019] This is a short-term hack until we have hooked up proper routines for
    // reading/writing .json files. While .json files are typically written out as line-oriented files, there's no
    // requirement for each element to be on it's own line, which is what we require if we're reading line by line.
    // So, if we're reading a file we generated that the user didn't touch, we're fine. But if the user decides to
    // edit the file, we're likely to either lose their changes, or break entirely -- until we have a json class in
    // place.

    size_t line = 0;
    for (; line < file.size(); ++line)
    {
        // Normally defines and includePath place each argument on its own line, but it doesn't have to be done
        // that way. They could all be on a single line, or they could be on multiple lines interspersed with
        // comment lines, blank lines, etc. For now, we'll assume it wasn't hand-edited...

        if (tt::contains(file[line], "\"defines\""))
        {
            if (file[line].find(']') != tt::npos)
                continue;  // all on one line, we don't process it
            auto insertpos = line + 1;

            for (++line; line < file.size();)
            {
                if (file[line].find(']') != tt::npos)
                    break;
                auto start = file[line].findnonspace();
                if (start == tt::npos)
                    continue;
                ttString tmp;
                tmp.ExtractSubString(file[line], start);
                Defines.append(tmp);  // Only gets added if it doesn't already exist
                file.erase(file.begin() + line, file.begin() + line + 1);
            }
            if (Defines.size())
            {
                size_t defpos = 0;
                for (; defpos < Defines.size() - 1; ++defpos)
                {
                    std::stringstream str;
                    str << "                \"" << Defines[defpos] << "\",";
                    file.insert(file.begin() + insertpos, str.str().c_str());
                    ++insertpos;
                }

                // Write the last one without a trailing comma
                std::stringstream str;
                str << "                \"" << Defines[defpos] << "\"";
                file.insert(file.begin() + insertpos, str.str().c_str());
                ++insertpos;
            }
            line = insertpos;
            continue;
        }

        if (tt::contains(file[line], "\"includePath\""))
        {
            if (file[line].find(']') != tt::npos)
                continue;  // all on one line, we don't process it
            auto insertpos = line + 1;

            for (++line; line < file.size();)
            {
                if (file[line].find(']') != tt::npos)
                    break;
                auto start = file[line].findnonspace();
                if (start == tt::npos)
                    continue;
                ttString path;
                path.ExtractSubString(file[line], start);
                if (!tt::contains(path, "${workspaceRoot}") && !tt::contains(path, "${default}"))
                {
                    // If it's not already a relative path, make it relative
                    if (path.at(0) != '.')
                    {
                        path.make_relative(".");
                    }
#if defined(_WIN32)
                    path.backslashestoforward();
#endif
                    if (!tt::contains(path, ":"))
                    {
                        std::string workspace("${workspaceRoot}/");
                        workspace += path;
                        Includes.addfilename(workspace);
                    }
                    else
                    {
                        Includes.addfilename(path);
                    }
                }
                else
                {
                    Includes.addfilename(path);
                }
                file.erase(file.begin() + line, file.begin() + line + 1);
            }

            if (Includes.size())
            {
                size_t defpos = 0;
                for (; defpos < Includes.size() - 1; ++defpos)
                {
                    // ':' is checked in case a drive letter is specified
                    if (!tt::contains(Includes[defpos], "${workspaceRoot}") &&
                        !tt::contains(Includes[defpos], "${default}") && !tt::contains(Includes[defpos], ":"))
                    {
                        ttString tmp(Includes[defpos]);
                        Includes[defpos] = "${workspaceRoot}/";
                        Includes[defpos] += tmp;
                    }

                    std::stringstream str;
                    str << "                \"" << Includes[defpos] << "\",";
                    file.insert(file.begin() + insertpos, str.str().c_str());
                    ++insertpos;
                }

                // Write the last one without a trailing comma
                std::stringstream str;
                str << "                \"" << Includes[defpos] << "\"";
                file.insert(file.begin() + insertpos, str.str().c_str());
                ++insertpos;
            }
            line = insertpos;
            continue;
        }
    }
    ttViewFile orgfile;
    if (!orgfile.ReadFile(".vscode/c_cpp_properties.json"))
    {
        std::stringstream msg;
        msg << _tt("Cannot open ") << ".vscode/c_cpp_properties.json";
        *plstResults += msg.str().c_str();
        return false;
    }

    bool Modified = false;
    if (file.size() != orgfile.size())
        Modified = true;
    else
    {
        for (line = 0; line < file.size(); ++line)
        {
            if (!file[line].issamestr(orgfile[line]))
            {
                Modified = true;
                break;
            }
        }
    }

    if (!Modified)
    {
        *plstResults += "c_cpp_properties.json is up to date";
        return false;
    }

    if (!file.WriteFile(".vscode/c_cpp_properties.json"))
    {
        std::stringstream msg;
        msg << _tt("Unable to create or write to ") << ".vscode/c_cpp_properties.json";
        *plstResults += msg.str().c_str();
        return false;
    }

    *plstResults += _tt("Updated .vscode/c_cpp_properties.json");

    return true;
}

// Given a string, finds any definitions and stores them in the provided list. Primarily used
// to parse a CFlags: option string
void ParseDefines(ttCList& lst, const char* pszDefines)
{
    const char* pszStart = pszDefines;
    while (*pszStart)
    {
        if (*pszStart == '-' || *pszStart == CH_FORWARDSLASH)
        {
            ++pszStart;
            if (*pszStart != 'D')
            {  // note that this option is case sensitive
                pszStart = ttStepOver(pszStart);
                continue;  // it's not a define
            }
            ttCStr cszTmp;
            cszTmp.GetString(pszStart, *pszStart, CH_SPACE);
            lst += cszTmp;
            pszStart += ttStrLen(cszTmp);
        }
        else
        {
            pszStart = ttStepOver(pszStart);
        }
    }
}

// REVIEW: [KeyWorks - 10-05-2019] Don't remove this! We'll need it once we support conditional task
// generation for messages.po

#if 0
static void AddTask(ttCFile& fileOut, const char* pszLabel, const char* pszGroup, const char* pszCommand,
                    const char* pszProblem)
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
#endif

// AddMsvcTask uses $msCompile for the problemMatcher but changes it to use a relative path
// instead of the default absolute path
static void AddMsvcTask(ttCFile& fileOut, const char* pszLabel, const char* pszGroup, const char* pszCommand)
{
    ttCFile fileTask;
    fileTask.Delete();
    fileTask.ReadStrFile(txtMsvcSubTasks);

    fileTask.ReplaceStr("%label%", pszLabel);
    fileTask.ReplaceStr("%group%", pszGroup);
    fileTask.ReplaceStr("%command%", pszCommand);

    while (fileTask.ReadLine())
        fileOut.WriteEol(fileTask);
}

// AddClangTask provides it's own problemMatcher -- because $gcc didn't work
static void AddClangTask(ttCFile& fileOut, const char* pszLabel, const char* pszGroup, const char* pszCommand)
{
    ttCFile fileTask;
    fileTask.Delete();
    fileTask.ReadStrFile(txtClangSubTasks);

    fileTask.ReplaceStr("%label%", pszLabel);
    fileTask.ReplaceStr("%group%", pszGroup);
    fileTask.ReplaceStr("%command%", pszCommand);

    while (fileTask.ReadLine())
        fileOut.WriteEol(fileTask);
}

#if defined(_WIN32)
// This will get the full path to the directory, using the target path if it is a junction
bool JunctionToReal(const char* pszDir, ttCStr& cszDir)
{
    ttASSERT_MSG(pszDir, "NULL pointer!");

    auto hFile = CreateFileA(pszDir, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        cszDir = pszDir;
        return false;
    }
    char szPath[MAX_PATH];
    auto dwRet = GetFinalPathNameByHandleA(hFile, szPath, sizeof(szPath), VOLUME_NAME_DOS);
    if (dwRet < sizeof(szPath))
    {
        char* pszStart;
        for (pszStart = szPath; *pszStart && !ttIsAlpha(*pszStart); ++pszStart)
            ;

        cszDir = pszStart;
    }

    CloseHandle(hFile);
    return true;
}
#endif  // _WIN32
