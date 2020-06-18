/////////////////////////////////////////////////////////////////////////////
// Name:      vscode.cpp
// Purpose:   Creates/updates .vscode files
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttmultistr.h>  // multistr -- Breaks a single string into multiple strings
#include <tttextfile.h>  // ttTextFile, ttViewFile -- Similar to wxTextFile, but uses UTF8 strings

#include "csrcfiles.h"  // CSrcFiles
#include "dlgvscode.h"  // CDlgVsCode -- IDDLG_VSCODE dialog handler
#include "funcs.h"      // List of function declarations
#include "resource.h"

// If the text is read as an array, then it's stored as char*[], has a trailing nullptr, and no \n characters.

// By contrast, if the text is to be read as a string, then each "line" must end with a \n character. These string
// typically have %...% portions which are placeholders that get replaced with strings determined durint runtime.

static const auto txtProperties = {

    "{",
    "    \"configurations\": [",
    "        {",
    "            \"name\": \"Default\",",
    "             \"cStandard\": \"c11\",",
    "             \"cppStandard\": \"c++11\",",
    "             \"defines\": [",
    "             ],",
    "             \"includePath\": [",
    "             ]",
    "         }",
    "     ],",
    "     \"version\": 4",
    "}",

};

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

static const auto txtTasks = {

    "{",
    "    // See https://go.microsoft.com/fwlink/?LinkId=733558",
    "    // for the documentation about the tasks.json format",
    "    \"version\": \"2.0.0\",",
    "    \"tasks\": [",
    "    ]",
    "}",

};

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

static const char* txtClangSubTasks = "        {\n"
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

bool CreateVsCodeProps(CSrcFiles& cSrcFiles, ttlib::cstrVector& Results);
bool UpdateVsCodeProps(CSrcFiles& cSrcFiles, ttlib::cstrVector& Results);

#if 0
static void AddTask(ttlib::textfile& fileOut, const char* pszLabel, const char* pszGroup, const char* pszCommand,
                    const char* pszProblem);
#endif

static void AddMsvcTask(ttlib::textfile& fileOut, std::string_view Label, std::string_view Group, std::string_view Command);
static void AddClangTask(ttlib::textfile& fileOut, std::string_view Label, std::string_view Group, std::string_view Command);

ttlib::cstrVector CreateVsCodeProject(std::string_view projectFile)
{
    ttlib::cstrVector results;

    if (!ttlib::dirExists(".vscode"))
    {
        if (!fs::create_directory(".vscode"))
        {
            results += _tt(strIdCantCreateVsCodeDir);
            return results;
        }

        // In most cases, the .vscode/ directory should be ignored since it can contain settings that are specific to
        // the user. If it's not currently being ignored, then ask the user if they want to add the directory

        if (ttlib::cstr gitIgnore; !gitIsFileIgnored(gitIgnore, ".vscode/") && !gitIsExcluded(gitIgnore, ".vscode/"))
        {
            if (ttlib::dirExists(".git"))
                gitIgnore = ".git/info/exclude";
            else if (ttlib::dirExists("../.git"))
                gitIgnore = "../.git/info/exclude";
            else if (ttlib::dirExists("../../.git"))
                gitIgnore = "../../.git/info/exclude";

            if (!gitIgnore.empty() &&
                ttlib::MsgBox(_tt(strIdQueryIgnore) + gitIgnore + " ?", MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING) == IDYES)
            {
                if (gitAddtoIgnore(gitIgnore, ".vscode/"))
                {
                    results.emplace_back(_tt(strIdVscodeIgnored) + gitIgnore);
                }
            }
        }
    }

    CSrcFiles cSrcFiles;
    if (!cSrcFiles.ReadFile(projectFile))
    {
        results += _tt(strIdCantConfigureJson);
        return results;
    }

    if (ttlib::fileExists(".vscode/c_cpp_properties.json"))
    {
        if (!UpdateVsCodeProps(cSrcFiles, results))
            return results;
    }
    else
    {
        if (!CreateVsCodeProps(cSrcFiles, results))
            return results;
    }

    if (!ttlib::fileExists(".vscode/launch.json") || !ttlib::fileExists(".vscode/tasks.json"))
    {
        CDlgVsCode dlg;
        if (dlg.DoModal(NULL) != IDOK)
            return results;

        if (!ttlib::fileExists(".vscode/launch.json"))
        {
            if (!dlg.CreateVsCodeLaunch(cSrcFiles, results))
                return results;
        }

        if (!ttlib::fileExists(".vscode/tasks.json"))
        {
            if (!dlg.CreateVsCodeTasks(cSrcFiles, results))
                return results;
        }
    }

    return results;
}

bool CreateVsCodeProps(CSrcFiles& cSrcFiles, ttlib::cstrVector& Results)
{
    ttlib::textfile out;
    ttlib::textfile propFile;
    propFile.Read(txtProperties);

    for (auto propLine = propFile.begin(); propLine != propFile.end(); ++propLine)
    {
        if (propLine->viewnonspace().issameprefix("\"defines", tt::CASE::either))
        {
            out.emplace_back(*propLine);
            ttlib::cstrVector Defines;
            if (cSrcFiles.hasOptValue(OPT::CFLAGS_CMN))
                ParseDefines(Defines, cSrcFiles.getOptValue(OPT::CFLAGS_CMN));
            if (cSrcFiles.hasOptValue(OPT::CFLAGS_DBG))
                ParseDefines(Defines, cSrcFiles.getOptValue(OPT::CFLAGS_DBG));
            std::sort(Defines.begin(), Defines.end());
            for (auto iter: Defines)
            {
                auto& line = out.addEmptyLine();
                // add the defines in quotes, one per line (%ks puts the string in quotes)
                line.Format("                %ks,", iter.c_str());
            }
            // we always define _DEBUG. Under Windows, we always define _WIN32.

#if defined(_WIN32)
            out.emplace_back("                \"_WIN32\",");
#endif
            out.emplace_back("                \"_DEBUG\"");
            continue;
        }

        else if (propLine->viewnonspace().issameprefix("\"includePath", tt::CASE::either))
        {
            out.emplace_back(*propLine);
            while (++propLine != propFile.end())  // find the end of the current list of includes
            {
                if (propLine->find(']') == tt::npos)
                    out.emplace_back(*propLine);
                else
                    break;
            }

            if (cSrcFiles.hasOptValue(OPT::INC_DIRS))
            {
                ttlib::cstr projectDir = cSrcFiles.GetSrcFilesName();
                projectDir.make_absolute();
                projectDir.remove_filename();

                ttlib::multistr enumInc(cSrcFiles.getOptValue(OPT::INC_DIRS));
                for (auto& iter: enumInc)
                {
                    ttlib::cstr IncName(iter);
                    IncName.make_absolute();
                    IncName.make_relative(projectDir);
                    IncName.backslashestoforward();

                    auto& line = out.addEmptyLine();
                    line.Format("                \"${workspaceRoot}/%s\",", IncName.c_str());
                }
            }
            // we always add the default include path
            out.emplace_back("                \042${default}\042");
            out.emplace_back(*propLine);
            continue;
        }
        else if (propLine->viewnonspace().issameprefix("\"cppStandard", tt::CASE::either))
        {
            // The default is c++11, but check to see if a specific version was specified in Cflags
            if (cSrcFiles.hasOptValue(OPT::CFLAGS_CMN) && cSrcFiles.getOptValue(OPT::CFLAGS_CMN).contains("std:c++"))
            {
                auto option = cSrcFiles.getOptValue(OPT::CFLAGS_CMN).subview();
                option.remove_prefix(option.locate("std:c++") + 3);
                ttlib::cstr cppstandard;
                cppstandard.AssignSubString(option, ':', ' ');
                auto& line = out.emplace_back(*propLine);
                line.Replace("c++11", cppstandard);
            }
        }
        else
        {
            out.emplace_back(*propLine);
        }
    }

    if (!out.WriteFile(".vscode/c_cpp_properties.json"))
    {
        ttlib::MsgBox(_ttc(strIdCantWrite) + ".vscode/c_cpp_properties.json");
        return false;
    }
    else
    {
        Results += (_ttc(strIdCreated) + ".vscode/c_cpp_properties.json");
    }

    return true;
}

bool CDlgVsCode::CreateVsCodeLaunch(CSrcFiles& cSrcFiles, ttlib::cstrVector& Results)
{
    if (cSrcFiles.IsExeTypeLib() || cSrcFiles.IsExeTypeDll())
        return true;  // nothing that we know how to launch if this is a library or dynamic link library

    ttlib::cstr Launch = { txtLaunch };

    Launch.Replace("%proj%", cSrcFiles.GetProjectName());
#if defined(_WIN32)
    if (m_PreLaunch == PRELAUNCH_MAIN)
        Launch.Replace("%bld%", "Build Debug MSVC");
    else if (m_PreLaunch == PRELAUNCH_CLANG)
        Launch.Replace("%bld%", "Build Debug CLANG");
    else if (m_PreLaunch == PRELAUNCH_NINJA)
        Launch.Replace("%bld%", "Ninja Debug Build");
    else
        Launch.Replace("%bld%", "");
#else   // not defined(_WIN32)
    if (m_PreLaunch == PRELAUNCH_MAIN)
        Launch.Replace("%bld%", "Build Debug GCC");
    else if (m_PreLaunch == PRELAUNCH_CLANG)
        Launch.Replace("%bld%", "Build Debug CLANG");
    else if (m_PreLaunch == PRELAUNCH_NINJA)
        Launch.Replace("%bld%", "Ninja Debug Build");
    else
        Launch.Replace("%bld%", "");
#endif  // defined(_WIN32)

    Launch.Replace("%targetD%", cSrcFiles.GetTargetDebug());

    // REVIEW: [randalphwa - 7/19/2019] Will it work to have a non-existant default.natvis file or will VS Code
    // complain? An alternative would be to insert/remove the entire line
    ttlib::cstr Path(cSrcFiles.hasOptValue(OPT::NATVIS) ? cSrcFiles.getOptValue(OPT::NATVIS) : "default.natvis");
    Path.backslashestoforward();
    Launch.Replace("%natvis%", Path);

    ttlib::textfile file;
    file.ReadString(Launch);

    if (!file.WriteFile(".vscode/launch.json"))
    {
        ttlib::MsgBox(_ttc(strIdCantWrite) + ".vscode/launch.json");
        return false;
    }
    else
    {
        Results.emplace_back(_ttc(strIdCreated) + ".vscode/launch.json");
    }

    return true;
}

bool CDlgVsCode::CreateVsCodeTasks(CSrcFiles& cSrcFiles, ttlib::cstrVector& Results)
{
    ttlib::textfile out;

    ttlib::textfile tasks;
    tasks.Read(txtTasks);

    // All of our tasks assumbe a bld/ directory since that's where ttBld will be creating the ninja files and a makefile
    ttlib::cstr MakeFileOption("-f bld/makefile");

    // Read the array of lines, write them into kf so it looks like it was read from a file

    for (auto& taskLine: tasks)
    {
        if (taskLine.find("\042tasks") == tt::npos)
        {
            out.emplace_back(taskLine);
            continue;
        }

        out.emplace_back(taskLine);

        // First set the default task. This is the only one where the property "isDefault" is set

        ttlib::textfile SubTask;
        SubTask.ReadString(txtDefaultTask);
        ttlib::cstr MakeCommand;
        if (m_hasMakefileTask)
        {
#if defined(_WIN32)
            // Build MSVC debug

            MakeCommand = "nmake.exe -nologo debug " + MakeFileOption;
            AddMsvcTask(out, "Build Debug MSVC", (m_DefTask == DEFTASK_MAIN) ? txtDefaultGroup : txtNormalGroup, MakeCommand);
#else
            MakeCommand = "make debug cmplr=gcc " + MakeFileOption;
            AddClangTask(out, "Build Debug GCC", (m_DefTask == DEFTASK_MAIN) ? txtDefaultGroup : txtNormalGroup, MakeCommand);
#endif

            // To prevent writing the same code multiple times, we write it once assuming nmake and MSVC
            // compiler. If we're not on Windows, then before we write the file, we replace nmake.exe with
            // make.exe, and MSVC with either CLANG or GCC.

            // Build MSVC release

            ttlib::cstr Label;
            Label.Format("Build %s (release) using MSVC", cSrcFiles.GetTargetRelease().filename().c_str());

            MakeCommand = "nmake.exe -nologo release " + MakeFileOption;
            AddMsvcTask(out, "Build Release MSVC", txtNormalGroup, MakeCommand);

            // Rebuild MSVC release

            MakeCommand = "nmake.exe -nologo clean release " + MakeFileOption;
            AddMsvcTask(out, "Rebuild Release MSVC", txtNormalGroup, MakeCommand);
            if (m_hasNinjaTask)
                AddMsvcTask(out, "Ninja Debug Build", txtNormalGroup, "ninja -f bld/msvc_dbg.ninja");
        }

#if defined(_WIN32)
        // If we're on Windows, then we also look to see if either the clang-cl compiler is available. If so,
        // we add build targets for it

        if (m_hasClangTask)
        {
            MakeCommand = (m_hasMingwMake ? "mingw32-make.exe debug " : "nmake -nologo debug cmplr=clang ") + MakeFileOption;
            AddClangTask(out, "Build Debug CLANG", (m_DefTask == DEFTASK_CLANG) ? txtDefaultGroup : txtNormalGroup, MakeCommand);

            MakeCommand = (m_hasMingwMake ? "mingw32-make.exe release " : "nmake -nologo release cmplr=clang ") + MakeFileOption;
            AddClangTask(out, "Build Release CLANG", txtNormalGroup, MakeCommand);

            MakeCommand =
                (m_hasMingwMake ? "mingw32-make.exe clean release " : "nmake -nologo clean release cmplr=clang ") + MakeFileOption;
            AddClangTask(out, "Rebuild Release CLANG", txtNormalGroup, MakeCommand);
            if (m_hasNinjaTask && !m_hasMakefileTask)
                AddClangTask(out, "Ninja Debug Build", (m_DefTask == DEFTASK_NINJA) ? txtDefaultGroup : txtNormalGroup,
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

    // At this point the task assume a bld/ directory -- if that needs to change, then we can simply iterate through the out vector and
    // replace "-f bld/" with whatever the directory should be.

    if (!out.WriteFile(".vscode/tasks.json"))
    {
        Results.emplace_back(_ttc(strIdCantWrite) + ".vscode/tasks.json");
        return false;
    }
    else
    {
        Results.emplace_back(_ttc(strIdCreated) + ".vscode/tasks.json");
    }

    return true;
}

bool UpdateVsCodeProps(CSrcFiles& cSrcFiles, ttlib::cstrVector& Results)
{
    ttlib::textfile file;
    if (!file.ReadFile(".vscode/c_cpp_properties.json"))
    {
        Results.emplace_back(_ttc(strIdCantOpen) + ".vscode/c_cpp_properties.json");
        return false;
    }

    // Gather all of our include directories into a list

    ttlib::cstrVector Includes;

    if (!cSrcFiles.getOptValue(OPT::INC_DIRS).empty())
    {
        ttlib::multistr IncludeDirs(cSrcFiles.getOptValue(OPT::INC_DIRS));
        for (auto& iter: IncludeDirs)
        {
            // If it's not already a relative path, make it relative
            if (iter.at(0) != '.')
            {
                iter.make_relative(".");
            }
#if defined(_WIN32)

            iter.backslashestoforward();
#endif
            // Remove trailing backslash just to make all paths look the same
            if (iter.back() == '/')
                iter.pop_back();
            Includes.addfilename("${workspaceRoot}/" + iter);
        }
    }

#if 0
    // REVIEW: [KeyWorks - 02-03-2020] Because MSVC frequently changes the path to the header files,
    // adding this ensures that VS Code has access to the current header files. That works great for a local
    // copy of c_cpp_properties but wreaks havoc if it's under Source control management (e.g., git).
    #if defined(_WIN32)
    ttlib::cstr cszMSVC;
    if (FindCurMsvcPath(cszMSVC))
    {
        cszMSVC.append_filename("include");
        cszMSVC.backslashtoforwardslash();  // so we don't have to escape all the backslashes
        Includes.append(cszMSVC);
    }
    #endif
#endif

    // Gather all the common and debug defines specified in CFlags: and CFlagsD:

    ttlib::cstrVector Defines;

    if (cSrcFiles.hasOptValue(OPT::CFLAGS_CMN))
        ParseDefines(Defines, cSrcFiles.getOptValue(OPT::CFLAGS_CMN));
    if (cSrcFiles.hasOptValue(OPT::CFLAGS_DBG))
        ParseDefines(Defines, cSrcFiles.getOptValue(OPT::CFLAGS_DBG));

    // Also add environment flags

    ttlib::cstr Env;
    if (Env.assignEnvVar("CFLAGS"))
        ParseDefines(Defines, Env);
    if (Env.assignEnvVar("CFLAGSD"))
        ParseDefines(Defines, Env);

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

        if (ttlib::contains(file[line], "\"defines\""))
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
                ttlib::cstr tmp;
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

        if (file[line].contains("\"includePath\""))
        {
            if (file[line].contains("]"))
                continue;  // all on one line, we don't process it
            auto insertpos = line + 1;

            for (++line; line < file.size();)
            {
                if (file[line].contains("]"))
                    break;
                auto start = file[line].findnonspace();
                if (start == tt::npos)
                    continue;
                ttlib::cstr path;
                path.ExtractSubString(file[line], start);
                if (!path.contains("${workspaceRoot}") && !path.contains("${default}"))
                {
                    // If it's not already a relative path, make it relative
                    if (path.at(0) != '.')
                    {
                        path.make_relative(".");
                    }
#if defined(_WIN32)
                    path.backslashestoforward();
#endif
                    if (!path.contains(":"))
                    {
                        Includes.addfilename("${workspaceRoot}/" + path);
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
                    if (!Includes[defpos].contains("${workspaceRoot}") && !Includes[defpos].contains("${default}") &&
                        !Includes[defpos].contains(":"))
                    {
                        Includes[defpos].insert(0, "${workspaceRoot}/");
                    }

                    file.insert(file.begin() + insertpos, "                \"" + Includes[defpos] + "\",");
                    ++insertpos;
                }

                // Write the last one without a trailing comma
                file.insert(file.begin() + insertpos, "                \"" + Includes[defpos] + "\"");
                ++insertpos;
            }
            line = insertpos;
            continue;
        }
    }
    ttlib::viewfile orgfile;
    if (!orgfile.ReadFile(".vscode/c_cpp_properties.json"))
    {
        Results.emplace_back(_ttc(strIdCantOpen) + ".vscode/c_cpp_properties.json");
        return false;
    }

    bool Modified = false;
    if (file.size() != orgfile.size())
        Modified = true;
    else
    {
        for (line = 0; line < file.size(); ++line)
        {
            if (!file[line].issameas(orgfile[line]))
            {
                Modified = true;
                break;
            }
        }
    }

    if (!Modified)
    {
        Results.emplace_back("c_cpp_properties.json" + _ttc(strIdCurrent));
        return true;
    }

    if (!file.WriteFile(".vscode/c_cpp_properties.json"))
    {
        Results.emplace_back(_ttc(strIdCantWrite) + ".vscode/c_cpp_properties.json");
        return false;
    }

    Results.emplace_back(".vscode/c_cpp_properties.json" + _ttc(strIdUpdated));

    return true;
}

// Given a string, finds any definitions and stores them in the provided list. Primarily used
// to parse a CFlags: option string
void ParseDefines(ttlib::cstrVector& Results, std::string_view Defines)
{
    if (Defines.empty())
        return;

    if (ttlib::issameprefix(Defines, "-D") || ttlib::issameprefix(Defines, "/D"))
    {
        Defines.remove_prefix(1);
        auto& def = Results.emplace_back();
        def.AssignSubString(Defines, 'D', ' ');
    }

    for (auto pos = Defines.find(" -"); ttlib::isFound(pos); pos = Defines.find(" -"))
    {
        Defines.remove_prefix(pos + 2);
        if (std::toupper(Defines[0]) == 'D')
        {
            auto& def = Results.emplace_back();
            def.AssignSubString(Defines, 'D', ' ');
        }
    }

    // Windows command lines often use / instead of - for switches, so we check for those as well.

    for (auto pos = Defines.find(" /"); ttlib::isFound(pos); pos = Defines.find(" /"))
    {
        Defines.remove_prefix(pos + 2);
        if (std::toupper(Defines[0]) == 'D')
        {
            auto& def = Results.emplace_back();
            def.AssignSubString(Defines, 'D', ' ');
        }
    }
}

// REVIEW: [KeyWorks - 10-05-2019] Don't remove this! We'll need it once we support conditional task
// generation for messages.po

#if 0
static void AddTask(ttlib::textfile& fileOut, const char* pszLabel, const char* pszGroup, const char* pszCommand,
                    const char* pszProblem)
{
    ttlib::textfile fileTask;
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
static void AddMsvcTask(ttlib::textfile& fileOut, std::string_view Label, std::string_view Group, std::string_view Command)
{
    ttlib::cstr SubTask(txtMsvcSubTasks);
    SubTask.Replace("%label%", Label);
    SubTask.Replace("%group%", Group);
    SubTask.Replace("%command%", Command);

    ttlib::textfile fileTask;
    fileTask.ReadString(SubTask);
    for (auto& iter: fileTask)
        fileOut.emplace_back(iter);
}

// AddClangTask provides it's own problemMatcher -- because $gcc didn't work
static void AddClangTask(ttlib::textfile& fileOut, std::string_view Label, std::string_view Group, std::string_view Command)
{
    ttlib::cstr SubTask(txtClangSubTasks);
    SubTask.Replace("%label%", Label);
    SubTask.Replace("%group%", Group);
    SubTask.Replace("%command%", Command);

    ttlib::textfile fileTask;
    fileTask.ReadString(SubTask);
    for (auto& iter: fileTask)
        fileOut.emplace_back(iter);
}
