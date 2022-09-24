/////////////////////////////////////////////////////////////////////////////
// Purpose:   Dialog for setting options to create tasks.json and launch.json
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include <string_view>

#include <tttextfile_wx.h>  // textfile -- Classes for reading and writing line-oriented files

#include "vscodedlg.h"  // auto-generated: vscodedlgBase.h and vscodedlgBase.cpp

#include "csrcfiles.h"  // CSrcFiles
#include "funcs.h"      // List of function declarations
#include "uifuncs.h"    // Miscellaneous functions for displaying UI

void AddMsvcTask(ttlib::textfile& fileOut, std::string_view Label, std::string_view Group, std::string_view Command);
void AddClangTask(ttlib::textfile& fileOut, std::string_view Label, std::string_view Group, std::string_view Command);

VsCodeDlg::VsCodeDlg(wxWindow* parent) : VsCodeDlgBase(parent) {}

void VsCodeDlg::OnInit(wxInitDialogEvent& event)
{
    m_hasMakefileTask = true;
    m_hasNinjaTask = true;

    ttlib::cstr tmp;
#if defined(_WIN32)
    m_taskMSVCBuild = true;
    m_hasMingwMake = FindFileEnv("PATH", "mingw32-make.exe", tmp);
    if (FindFileEnv("PATH", "clang-cl.exe", tmp))
        m_taskCLANGBuild = true;
#else
    m_hasMingwMake = false;
    if (FindFileEnv("PATH", "clang.exe", tmp))
        m_taskCLANGBuild = true;
#endif  // _WIN32

    event.Skip();  // transfer all validator data to their windows and update UI
}

constexpr auto txtLaunch = R"===(
{
   // Use IntelliSense to learn about possible attributes.
   // Hover to view descriptions of existing attributes.
   // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
   "version": "0.2.0",
   "configurations": [
      {
         "name": "Debug %proj%",
         "type": "cppvsdbg",
         "request": "launch",
         "cwd" : "${workspaceRoot}",
         "program": "${workspaceRoot}/%targetD%",
         "args": [ "" ],
         "logging": {
            "moduleLoad": false,
         },
         "visualizerFile": "${workspaceRoot}/%natvis%",
         "stopAtEntry": true,    // whether to stop or not at program entry point
         "preLaunchTask": "%bld%"
      }
   ]
};
)===";

bool VsCodeDlg::CreateVsCodeLaunch(CSrcFiles& cSrcFiles, std::vector<ttlib::cstr>& Results)
{
    if (cSrcFiles.IsExeTypeLib() || cSrcFiles.IsExeTypeDll())
        return true;  // nothing that we know how to launch if this is a library or dynamic link library

    ttlib::cstr Launch = ttlib::find_nonspace(txtLaunch);

    Launch.Replace("%proj%", cSrcFiles.GetProjectName());
#if defined(_WIN32)
    if (m_preLaunchMSVC)
        Launch.Replace("%bld%", "Build Debug MSVC");
    else if (m_preLaunchCLANG)
        Launch.Replace("%bld%", "Build Debug CLANG");
    else if (m_preLaunchNinja)
        Launch.Replace("%bld%", "Ninja Debug Build");
    else
        Launch.Replace("%bld%", "");
#else   // not defined(_WIN32)
    if (m_preLaunchMSVC)
        Launch.Replace("%bld%", "Build Debug GCC");
    else if (m_preLaunchCLANG)
        Launch.Replace("%bld%", "Build Debug CLANG");
    else if (m_preLaunchNinja)
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
        appMsgBox(ttlib::cstr("Unable to create or write to ") + ".vscode/launch.json");
        return false;
    }
    else
    {
        Results.emplace_back("Created .vscode/launch.json");
    }

    return true;
}

constexpr auto txtTasks = R"===(
{
    // See https://go.microsoft.com/fwlink/?LinkId=733558
    // for the documentation about the tasks.json format
    "version": "2.0.0\
    "tasks": [
    ]
}
)===";

constexpr auto txtDefaultGroup = R"===(
"group": {
    "kind": "build",
    "isDefault": true
},
)===";

constexpr auto txtDefaultTask = R"===(
{
    "label": "%label%",
    "type": "shell",
    "command": "%command%",
    "group": {
        "kind": "%group%",
        "isDefault": true
    },
    "problemMatcher": [ %problem% ]
},
)===";

constexpr auto txtNormalGroup = "            \"group\": \"build\",\n";

bool VsCodeDlg::CreateVsCodeTasks(CSrcFiles& cSrcFiles, std::vector<ttlib::cstr>& Results)
{
    ttlib::textfile out;

    ttlib::textfile tasks;
    tasks.ReadString(ttlib::find_nonspace(txtTasks));

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
            AddMsvcTask(out, "Build Debug MSVC", (m_defTaskMSVC) ? txtDefaultGroup : txtNormalGroup, MakeCommand);
#else
            MakeCommand = "make debug cmplr=gcc " + MakeFileOption;
            AddClangTask(out, "Build Debug GCC", (m_defTaskMSVC) ? txtDefaultGroup : txtNormalGroup, MakeCommand);
#endif

            // To prevent writing the same code multiple times, we write it once assuming nmake and MSVC
            // compiler. If we're not on Windows, then before we write the file, we replace nmake.exe with
            // make.exe, and MSVC with either CLANG or GCC.

            // Build MSVC release

            ttlib::cstr Label;
            Label << "Build " << cSrcFiles.GetTargetRelease().filename() << " (release) using MSVC";

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
            AddClangTask(out, "Build Debug CLANG", (m_defTaskCLANG) ? txtDefaultGroup : txtNormalGroup, MakeCommand);

            MakeCommand =
                (m_hasMingwMake ? "mingw32-make.exe release " : "nmake -nologo release cmplr=clang ") + MakeFileOption;
            AddClangTask(out, "Build Release CLANG", txtNormalGroup, MakeCommand);

            MakeCommand = (m_hasMingwMake ? "mingw32-make.exe clean release " : "nmake -nologo clean release cmplr=clang ") +
                          MakeFileOption;
            AddClangTask(out, "Rebuild Release CLANG", txtNormalGroup, MakeCommand);
            if (m_hasNinjaTask && !m_hasMakefileTask)
                AddClangTask(out, "Ninja Debug Build", (m_defTaskNinja) ? txtDefaultGroup : txtNormalGroup,
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

    // At this point the task assume a bld/ directory -- if that needs to change, then we can simply iterate through the out
    // vector and replace "-f bld/" with whatever the directory should be.

    if (!out.WriteFile(".vscode/tasks.json"))
    {
        Results.emplace_back(ttlib::cstr("Unable to create or write to ") + ".vscode/tasks.json");
        return false;
    }
    else
    {
        Results.emplace_back("Created .vscode/tasks.json");
    }

    return true;
}
