/////////////////////////////////////////////////////////////////////////////
// Name:      vscode.cpp
// Purpose:   Creates/updates .vscode files
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <cctype>
#include <string_view>

#include <ttmultistr.h>  // multistr -- Breaks a single string into multiple strings
#include <tttextfile.h>  // ttTextFile, ttViewFile -- Similar to wxTextFile, but uses UTF8 strings

#include "csrcfiles.h"  // CSrcFiles
#include "funcs.h"      // List of function declarations
#include "vscodedlg.h"  // VsCodeDlg -- Dialog for setting options to create tasks.json and launch.json

bool CreateVsCodeProps(CSrcFiles& cSrcFiles, ttlib::cstrVector& Results);
bool UpdateVsCodeProps(CSrcFiles& cSrcFiles, ttlib::cstrVector& Results);

void AddMsvcTask(ttlib::textfile& fileOut, std::string_view Label, std::string_view Group, std::string_view Command);
void AddClangTask(ttlib::textfile& fileOut, std::string_view Label, std::string_view Group, std::string_view Command);

ttlib::cstrVector CreateVsCodeProject(std::string_view projectFile)
{
    ttlib::cstrVector results;

    if (!ttlib::dir_exists(".vscode"))
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
            if (ttlib::dir_exists(".git"))
                gitIgnore = ".git/info/exclude";
            else if (ttlib::dir_exists("../.git"))
                gitIgnore = "../.git/info/exclude";
            else if (ttlib::dir_exists("../../.git"))
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

    if (ttlib::file_exists(".vscode/c_cpp_properties.json"))
    {
        if (!UpdateVsCodeProps(cSrcFiles, results))
            return results;
    }
    else
    {
        if (!CreateVsCodeProps(cSrcFiles, results))
            return results;
    }

    if (!ttlib::file_exists(".vscode/launch.json") || !ttlib::file_exists(".vscode/tasks.json"))
    {
        VsCodeDlg dlg;
        if (dlg.ShowModal() != wxID_OK)
            return results;

        if (!ttlib::file_exists(".vscode/launch.json"))
        {
            if (!dlg.CreateVsCodeLaunch(cSrcFiles, results))
                return results;
        }

        if (!ttlib::file_exists(".vscode/tasks.json"))
        {
            if (!dlg.CreateVsCodeTasks(cSrcFiles, results))
                return results;
        }
    }

    return results;
}

constexpr auto txtProperties = R"===(
{
    "configurations": [
        {
            "name": "Default",
             "cStandard": "c11",
             "cppStandard": "c++11",
             "defines": [
             ],
             "includePath": [
             ]
         }
     ],
     "version": 4
}
)===";

bool CreateVsCodeProps(CSrcFiles& cSrcFiles, ttlib::cstrVector& Results)
{
    ttlib::textfile out;
    ttlib::textfile propFile;
    propFile.ReadString(ttlib::find_nonspace(txtProperties));

    for (auto propLine = propFile.begin(); propLine != propFile.end(); ++propLine)
    {
        if (propLine->view_nonspace().is_sameprefix("\"defines", tt::CASE::either))
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

        else if (propLine->view_nonspace().is_sameprefix("\"includePath", tt::CASE::either))
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
        else if (propLine->view_nonspace().is_sameprefix("\"cppStandard", tt::CASE::either))
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
                auto start = file[line].find_nonspace();
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
                    file.insert(file.begin() + insertpos, ttlib::cstr() << "                \"" << Defines[defpos] << "\",");
                    ++insertpos;
                }

                // Write the last one without a trailing comma
                file.insert(file.begin() + insertpos, ttlib::cstr() << "                \"" << Defines[defpos] << "\"");
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
                auto start = file[line].find_nonspace();
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
            if (!file[line].is_sameas(orgfile[line]))
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

    if (ttlib::is_sameprefix(Defines, "-D") || ttlib::is_sameprefix(Defines, "/D"))
    {
        Defines.remove_prefix(1);
        auto& def = Results.emplace_back();
        def.AssignSubString(Defines, 'D', ' ');
    }

    for (auto pos = Defines.find(" -"); ttlib::is_found(pos); pos = Defines.find(" -"))
    {
        Defines.remove_prefix(pos + 2);
        if (std::toupper(Defines[0]) == 'D')
        {
            auto& def = Results.emplace_back();
            def.AssignSubString(Defines, 'D', ' ');
        }
    }

    // Windows command lines often use / instead of - for switches, so we check for those as well.

    for (auto pos = Defines.find(" /"); ttlib::is_found(pos); pos = Defines.find(" /"))
    {
        Defines.remove_prefix(pos + 2);
        if (std::toupper(Defines[0]) == 'D')
        {
            auto& def = Results.emplace_back();
            def.AssignSubString(Defines, 'D', ' ');
        }
    }
}

constexpr auto txtMsvcSubTasks = R"===(
{
    "label": "%label%",
    "type": "shell",
    "options": {
        "cwd": "${workspaceFolder}"
    },
    "command": "%command%",
%group%"
    "problemMatcher": {
        "base": "$msCompile",
        "fileLocation": [
            "autoDetect",
            "${workspaceFolder}"
        ]
    }
},
)===";

// AddMsvcTask uses $msCompile for the problemMatcher but changes it to use a relative path
// instead of the default absolute path
void AddMsvcTask(ttlib::textfile& fileOut, std::string_view Label, std::string_view Group, std::string_view Command)
{
    ttlib::cstr SubTask = ttlib::find_nonspace(txtMsvcSubTasks);
    SubTask.Replace("%label%", Label);
    SubTask.Replace("%group%", Group);
    SubTask.Replace("%command%", Command);

    ttlib::textfile fileTask;
    fileTask.ReadString(SubTask);
    for (auto& iter: fileTask)
        fileOut.emplace_back(iter);
}

constexpr auto txtClangSubTasks = R"===(
{
    "label": "%label%",
    "type": "shell",
    "options": {
        "cwd": "${workspaceFolder}"
    },
    "command": "%command%",
%group%"
    "problemMatcher": {
        "owner": "cpp",
        "fileLocation": [
            "autoDetect",
            "${workspaceFolder}"
        ],
        "pattern": {
            "regexp": "^(.*)\\\\((\\\\d+),(\\\\d+)\\\\):\\\\s+(note|warning|error):\\\\s+(.*)$",
             "file": 1,
             "line": 2,
             "column": 3,
             "severity": 4,
             "message": 5
        }
    }
},
)===";

// AddClangTask provides it's own problemMatcher -- because $gcc didn't work
void AddClangTask(ttlib::textfile& fileOut, std::string_view Label, std::string_view Group, std::string_view Command)
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
