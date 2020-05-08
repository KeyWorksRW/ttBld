/////////////////////////////////////////////////////////////////////////////
// Name:      mainapp.cpp
// Purpose:   entry point, global strings, library pragmas
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#if defined(wxWIDGETS)
    #define wxMSVC_VERSION_ABI_COMPAT
    #include <msvc/wx/setup.h>  // This will add #pragmas for the wxWidgets libraries

    #include <wx/cmdline.h>
    #include <wx/config.h>
    #include <wx/init.h>
    #include <wx/wxcrtvararg.h>
#endif

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")

#include "mainapp.h"  // CMainApp -- Main application class

#include <cstdlib>
#include <direct.h>  // Functions for directory handling and creation
#include <iostream>

#include <ttconsole.h>  // ttConsoleColor
#include <ttcvector.h>  // Vector of ttlib::cstr strings
#include <ttcwd.h>      // cwd -- Class for storing and optionally restoring the current directory
#include <ttparser.h>   // cmd -- Command line parser

#include "convert.h"     // CConvert
#include "convertdlg.h"  // CConvertDlg
#include "funcs.h"       // List of function declarations
#include "ninja.h"       // CNinja
#include "strtable.h"    // String resource IDs
#include "writevcx.h"    // CVcxWrite -- Create a Visual Studio project file

#if defined(TESTING)
    #include "dlgvscode.h"  // CDlgVsCode -- IDDLG_VSCODE dialog handler
#endif

void AddFiles(const ttlib::cstrVector& lstFiles);

enum UPDATE_TYPE
{
    UPDATE_NORMAL,

    UPDATE_MSVCD,
    UPDATE_MSVC32D,
    UPDATE_CLANG_CLD,
    UPDATE_CLANG_CL32D,
    UPDATE_CLANGD,
    UPDATE_CLANG32D,
    UPDATE_GCCD,
    UPDATE_GCC32D,

    UPDATE_MSVC,
    UPDATE_MSVC32,
    UPDATE_CLANG_CL,
    UPDATE_CLANG_CL32,
    UPDATE_CLANG,
    UPDATE_CLANG32,
    UPDATE_GCC,
    UPDATE_GCC32,

};

void MakeFileCaller(UPDATE_TYPE upType, const char* pszRootDir);

int main(int /* argc */, char** /* argv */)
{
    ttlib::cmd cmd;

    cmd.addHelpOption("h|help", _tt(IDS_DISPLAY_HELP_MSG));

    cmd.addOption("codecmd", _tt(IDS_CODECMD_HELP_MSG));

    cmd.addOption("dir", _tt(IDS_DIR_HELP_MSG), ttlib::cmd::needsarg);

    cmd.addOption("options", _tt(IDS_OPTIONS_HELP_MSG));
    cmd.addOption("vscode", _tt(IDS_VSCODE_HELP_MSG));
    cmd.addOption("force", _tt(IDS_FORCE_HELP_MSG));

#if defined(_WIN32)
    // REVIEW: [KeyWorks - 04-30-2020] What's the difference between these two?
    cmd.addOption("vs", _tt("adds or updates .json files in the .vs/ directory"));
    cmd.addOption("vcxproj", _tt(IDS_VCXPROJ_HELP_MSG));
#endif
    cmd.addHiddenOption("add");

    cmd.addHiddenOption("msvcenv64", ttlib::cmd::needsarg);
    cmd.addHiddenOption("msvcenv32", ttlib::cmd::needsarg);
    cmd.addHiddenOption("dryrun");

    cmd.addHiddenOption("umsvc");
    cmd.addHiddenOption("umsvc_x86");
    cmd.addHiddenOption("uclang");
    cmd.addHiddenOption("uclang_x86");
    cmd.addHiddenOption("umsvcD");
    cmd.addHiddenOption("umsvc_x86D");
    cmd.addHiddenOption("uclangD");
    cmd.addHiddenOption("uclang_x86D");

#if defined(TESTING) && !defined(NDEBUG)
    cmd.addHiddenOption("tvdlg", ttlib::cmd::needsarg);
#endif

    cmd.parse();

    if (cmd.isHelpRequested())
    {
        std::cout << txtVersion << '\n' << txtCopyRight << "\n\n";
        std::cout << _tt("ttBld.exe [options] [path]") << '\n';

        for (auto& iter: cmd.getUsage())
        {
            std::cout << iter << '\n';
        }
        return 0;
    }

    UPDATE_TYPE upType = UPDATE_NORMAL;

    // this will only be initialized if the user specifies a -dir option
    ttlib::cstr RootDir;

    // This will be initialized by the user if they specifies a name.yaml file on the command
    // line. Otherwise, a search is made to find the most likely .srcfiles.yaml (or it's
    // platform-variations) to use and this will be initialized to point to it.
    ttlib::cstr projectFile;

// Change 0 to 1 to confirm that our locating functions are actually working as expected
#if 0 && !defined(NDEBUG) && defined(_WIN32)
    {
        ttlib::cstr str;
        if (FindCurMsvcPath(str))
            std::cout << str;
        else
            std::cout << "Cannot locate MSVC path";

        if (FindVsCode(str))
            std::cout << str;
        else
            std::cout << "Cannot locate VSCode.exe";

        if (FindFileEnv("PATH", "mingw32-make.exe", str))
            std::cout << str;
        else
            std::cout << "Cannot locate mingw32-make.exe";
    }
#endif

    if (cmd.isOption("codecmd"))
    {
        CreateCodeCmd("code32.cmd");
        CreateCodeCmd("code64.cmd");
        return 1;
    }

    if (cmd.isOption("msvcenv64"))
    {
        CreateMSVCEnvCmd(cmd.getOption("msvcenv64").value_or("msvcenv64.cmd").c_str(), true);
        return 1;
    }

    if (cmd.isOption("msvcenv32"))
    {
        CreateMSVCEnvCmd(cmd.getOption("msvcenv32").value_or("msvcenv32.cmd").c_str(), false);
        return 1;
    }

    if (cmd.isOption("dir"))
    {
        RootDir.assign(cmd.getOption("dir").value_or("bld"));
    }

    // Allow the user to specify a path to the project file to use.
    if (cmd.getExtras().size())
    {
        if (cmd.getExtras().at(0).hasExtension(".yaml"))
            projectFile = cmd.getExtras().at(0);
    }

#if defined(TESTING) && !defined(NDEBUG)
    if (cmd.isOption("msvcenv32"))
    {
        CDlgVsCode dlg;
        if (dlg.DoModal(NULL) != IDOK)
            return 1;

        fs::remove(".vscode/launch.json");
        fs::remove(".vscode/tasks.json");

        // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
        auto results = CreateVsCodeProject(projectFile);
        for (auto& iter: results)
            std::cout << iter << '\n';
    }
#endif

    // The following commands are called from a makefile to update one .ninja script and immediately exit

    if (cmd.isOption("umsvc"))
        upType = UPDATE_MSVC;
    else if (cmd.isOption("umsvc_x86"))
        upType = UPDATE_MSVC32;
    else if (cmd.isOption("uclang"))
        upType = UPDATE_CLANG_CL;
    else if (cmd.isOption("uclang_x86"))
        upType = UPDATE_CLANG_CL32;
    else if (cmd.isOption("umsvcD"))
        upType = UPDATE_MSVCD;
    else if (cmd.isOption("umumsvc_x86Dsvc"))
        upType = UPDATE_MSVC32D;
    else if (cmd.isOption("uclangD"))
        upType = UPDATE_CLANG_CLD;
    else if (cmd.isOption("uclang_x86D"))
        upType = UPDATE_CLANG_CL32D;

    // If we are being called from a makefile then this is the only option we will now process. Note that we ignore
    // -dryrun even though we are generating a .ninja script. We do process the -dir option.

    if (upType != UPDATE_NORMAL)
    {
        MakeFileCaller(upType, RootDir.c_str());
        return 0;
    }

    // If a project file gets created, the options and vscode have already been set, and this flag will have been set to true.
    bool projectCreated = false;

    // Unless the user specified this on the command line, locate a version, or create on if none can be found.
    if (projectFile.empty())
    {
        projectFile.assign(locateProjectFile(RootDir));
        if (projectFile.empty())
        {
            if (!MakeNewProject(projectFile))
            {
                ttlib::concolor clr(ttlib::concolor::LIGHTRED);
                std::cout << _tt(IDS_CANT_FIND_SRCFILES) << '\n';
                return 1;
            }
            projectCreated = true;
        }
    }

#if 0
    // BUGBUG: [KeyWorks - 05-08-2020] Can't add a filename without also specifying the project file to add it to (since
    // the user might have specified the one they want)
    if (cmd.isOption("add"))
    {
        AddFiles(cmd.getExtras());
    }
#endif

    if (!projectCreated && cmd.isOption("options"))
    {
        if (!ChangeOptions(projectFile))
            return 1;
    }

    if (!projectCreated && cmd.isOption("vscode"))
    {
        if (cmd.isOption("force"))
            std::filesystem::remove(".vscode/c_cpp_properties.json");
        // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
        auto results = CreateVsCodeProject(projectFile);
        for (auto& iter: results)
            std::cout << iter << '\n';
    }

#if defined(_WIN32)
    if (cmd.isOption("vcxproj"))
    {
        CVcxWrite vcx;
        return (vcx.CreateBuildFile() ? 0 : 1);
    }
    else if (cmd.isOption("vs"))
    {
        // Create .vs/ and any of the three .json files that are missing, and update c_cpp_properties.json
        std::vector<std::string> results;
        CreateVsJson(projectFile.c_str(), results);
        for (auto msg: results)
            std::cout << msg << '\n';
    }
#endif  // _WIN32

    // It's possible that the user specified a different location and even a different name for the project file to use.
    // If the project file is in a different directory, then we change to that directory now before continuing. The
    // destructor will restore the original directory.

    ttlib::cwd cwd(true);
    {
        ttlib::cstr path(projectFile);
        path.remove_filename();
        if (path.size())
        {
            ttlib::ChangeDir(path);
            projectFile.erase(0, projectFile.find_filename());
        }
    }

    CNinja cNinja(projectFile);
    if (!cNinja.IsValidVersion())
    {
        std::cerr << _tt(IDS_OLD_TTBLD) << '\n';
        return 1;
    }

    if (cmd.isOption("force"))  // force write ignores any request for dryrun
        cNinja.ForceWrite();
    else if (cmd.isOption("dryrun"))
        cNinja.EnableDryRun();

    int countNinjas = 0;
#if defined(_WIN32)
    if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_MSVC))
        countNinjas++;
    if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_MSVC))
        countNinjas++;
#endif
    if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_CLANG))
        countNinjas++;
    if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_CLANG))
        countNinjas++;

#if 0
        if (ttIsNonEmpty(cNinja.GetHHPName()))
            cNinja.CreateHelpFile();
#endif

    // Display any errors that occurred during processing

    if (cNinja.getErrorMsgs().size())
    {
        ttlib::concolor clr(ttlib::concolor::LIGHTRED);
        for (auto iter: cNinja.getErrorMsgs())
        {
            std::cout << iter << '\n';
        }
        std::cout << "\n\n";
    }

    if (countNinjas > 0)
    {
        std::cout << _tt(IDS_CREATED) << countNinjas << " .ninja" << _tt(IDS_FILES) << '\n';
    }
    else
        std::cout << _tt(IDS_UP_TO_DATE) << '\n';

    return 0;
}

void MakeFileCaller(UPDATE_TYPE upType, const char* pszRootDir)
{
    CNinja cNinja(pszRootDir);
    cNinja.ForceWrite();

    if (cNinja.IsValidVersion())
    {
        switch (upType)
        {
            case UPDATE_MSVC:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_MSVC))
                    std::cout << cNinja.GetScriptFile() << _tt(IDS_UPDATED) << '\n';
                break;

            case UPDATE_MSVC32:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_MSVC))
                    std::cout << cNinja.GetScriptFile() << _tt(IDS_UPDATED) << '\n';
                break;

            case UPDATE_CLANG_CL:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_CLANG))
                    std::cout << cNinja.GetScriptFile() << _tt(IDS_UPDATED) << '\n';
                break;

            case UPDATE_CLANG_CL32:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_CLANG))
                    std::cout << cNinja.GetScriptFile() << _tt(IDS_UPDATED) << '\n';
                break;

            case UPDATE_MSVCD:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_MSVC))
                    std::cout << cNinja.GetScriptFile() << _tt(IDS_UPDATED) << '\n';
                break;

            case UPDATE_MSVC32D:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_MSVC))
                    std::cout << cNinja.GetScriptFile() << _tt(IDS_UPDATED) << '\n';
                break;

            case UPDATE_CLANG_CLD:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_CLANG))
                    std::cout << cNinja.GetScriptFile() << _tt(IDS_UPDATED) << '\n';
                break;

            case UPDATE_CLANG_CL32D:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_CLANG))
                    std::cout << cNinja.GetScriptFile() << _tt(IDS_UPDATED) << '\n';
                break;

            default:
                break;
        }
    }

    else
    {
        ttlib::concolor clr(ttlib::concolor::LIGHTRED);
        std::cout << _tt(IDS_OLD_TTBLD);
    }
}
