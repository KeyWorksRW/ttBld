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

enum : size_t  // actions that can be run in addition to normal single command actions
{
    // clang-format off

    ACT_DRYRUN    = 1 << 0,  // This is a dry run -- don't actually output anything.
    ACT_VS        = 1 << 1,
    ACT_VSCODE    = 1 << 2,  // Generate .vscode directory and .json files.
    ACT_DIR       = 1 << 3,
    ACT_MAKEFILE  = 1 << 4,
    ACT_ALLNINJA  = 1 << 5,  // Creates/updates all .ninja scripts, creeates makefile (if missing).
    ACT_NEW       = 1 << 6,  // Displays a dialog for creating a new .srcfiles.yaml file.
    ACT_FORCE     = 1 << 7,  // Creates .ninja file even if it hasn't changed.
    ACT_OPTIONS   = 1 << 8,  // Displays a dialog for changing options in .srcfiles.yaml
    ACT_CONVERT   = 1 << 9,  // Converts the specified build script file to .srcfiles.yaml
    ACT_ALL       = 1 << 10,  // Mostly equivalent to ACT_ALLNINJA + ACT_VSCODE
    ACT_ALLD      = 1 << 11,  // Same as ACT_ALL but forces output.
    ACT_WRITE_VCX = 1 << 12,  // conver .srcfiles to .vcxproj

    // clang-format on
};

int main(int /* argc */, char** /* argv */)
{
    ttlib::cmd cmd;

    cmd.addHelpOption("h|help", _tt(IDS_DISPLAY_HELP_MSG));

    cmd.addOption("add", _tt(IDS_ADD_HELP_MSG));
    cmd.addOption("codecmd", _tt(IDS_CODECMD_HELP_MSG));

    cmd.addOption("dir", _tt(IDS_DIR_HELP_MSG), ttlib::cmd::shared_val | ttlib::cmd::needsarg, ACT_DIR);

    cmd.addOption("convert", _tt(IDS_CONVERT_HELP_MSG), ttlib::cmd::shared_val, ACT_CONVERT);
    cmd.addOption("dryrun", _tt(IDS_DRYRUN_HELP_MSG), ttlib::cmd::shared_val, ACT_DRYRUN);
    cmd.addOption("new", _tt(IDS_NEW_HELP_MSG), ttlib::cmd::shared_val, ACT_NEW);
    cmd.addOption("options", _tt(IDS_OPTIONS_HELP_MSG), ttlib::cmd::shared_val, ACT_OPTIONS);
    cmd.addOption("vscode", _tt(IDS_VSCODE_HELP_MSG), ttlib::cmd::shared_val, ACT_VSCODE);
    cmd.addOption("force", _tt(IDS_FORCE_HELP_MSG), ttlib::cmd::shared_val, ACT_FORCE);

#if defined(_WIN32)
    // REVIEW: [KeyWorks - 04-30-2020] What's the difference between these two?
    cmd.addOption("vs", _tt("creates files used to build and debug a project using Visual Studio"), ttlib::cmd::shared_val, ACT_VS);
    cmd.addOption("vcxproj", _tt(IDS_VCXPROJ_HELP_MSG), ttlib::cmd::shared_val, ACT_WRITE_VCX);
#endif
    cmd.addHiddenOption("msvcenv64", ttlib::cmd::needsarg);
    cmd.addHiddenOption("msvcenv32", ttlib::cmd::needsarg);

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

    size_t Action = cmd.getSharedValue();
    UPDATE_TYPE upType = UPDATE_NORMAL;

    ttlib::cstr RootDir;      // this will be set if (Action & ACT_DIR) is set
    ttlib::cstr SrcFilePath;  // location of srcfiles.yaml

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

#if 0
    // REVIEW: [KeyWorks - 04-24-2020] Need to decide if we really want to support this...
    auto pszEnv = std::getenv("TTBLD");
    if (pszEnv)
    {
        auto pszArg = ttlib::findnonspace(pszEnv);
        while (*pszArg)
        {
            if (*pszArg == '-' || *pszArg == '/')
            {
                ++pszArg;

                // Only a few or our standard arguments will actually make sense as an environment variable

                if (ttlib::issameprefix(pszArg, "vscode", tt::CASE::either))
                    Action |= ACT_VSCODE;
            }
            pszArg = ttStepOver(pszArg);
        }
    }
#endif

    if (cmd.isOption("add"))
    {
        AddFiles(cmd.getExtras());
        return 1;
    }

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

#if defined(TESTING) && !defined(NDEBUG)
    if (cmd.isOption("msvcenv32"))
    {
        CDlgVsCode dlg;
        if (dlg.DoModal(NULL) != IDOK)
            return 1;

        fs::remove(".vscode/launch.json");
        fs::remove(".vscode/tasks.json");

        // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
        auto results = CreateVsCodeProject(SrcFilePath);
        for (auto& iter: results)
            std::cout << iter << '\n';
    }
#endif

    if (cmd.isOption("dir"))
    {
        RootDir.assign(cmd.getOption("dir").value_or("bld"));
    }

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

    // The order that we process switches is important. -new comes first since we can only continue to run
    // if we have a .srcfiles.yaml file to work with.

    if (Action & ACT_NEW)
    {
        // -dir option will have set SrcFilePath, if option not specified, default to current directory
        CConvertDlg dlg(!SrcFilePath.empty() ? SrcFilePath.c_str() : ".srcfiles.yaml");
        if (dlg.DoModal(NULL) != IDOK)
            return 1;
        SrcFilePath = dlg.GetOutSrcFiles();

        // [KeyWorks - 7/30/2019] If the conversion dialog completed successfully, then the new .srcfiles.yaml has
        // been created. We call ChangeOptions() in case the user wants to tweak anything, but it's fine if the
        // user wants to cancel -- that just means they didn't want to change any options. It does NOT mean they
        // want to cancel running any other commands, such as makefile creation (which doesn't need to know what
        // options are set in .srcfiles.yaml).

        ChangeOptions(SrcFilePath);

        if (dlg.isCreateVsCode())
        {
            // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
            auto results = CreateVsCodeProject(SrcFilePath);
            for (auto& iter: results)
                std::cout << iter << '\n';
        }
        if (dlg.isGitIgnoreAll())
        {
            ttlib::cstr GitExclude;
            if (gitIgnoreAll(GitExclude))
            {
                std::cout << _tt(IDS_ADDED_IGNORE_FILES) << GitExclude << '\n';
            }
        }
    }
    else if (Action & ACT_CONVERT)
    {
        // -dir option will have set SrcFilePath, if option not specified, default to current directory
        CConvertDlg dlg;
        dlg.SetConvertScritpt(SrcFilePath);
        if (dlg.DoModal(NULL) != IDOK)
            return 1;
        SrcFilePath = dlg.GetOutSrcFiles();

        // [KeyWorks - 7/30/2019] If the conversion dialog completed successfully, then the new .srcfiles.yaml has
        // been created. We call ChangeOptions() in case the user wants to tweak anything, but it's fine if the
        // user wants to cancel -- that just means they didn't want to change any options. It does NOT mean they
        // want to cancel running any other commands, such as makefile creation (which doesn't need to know what
        // options are set in .srcfiles.yaml).

        ChangeOptions(SrcFilePath);
        if (dlg.isCreateVsCode())
        {
            // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
            auto results = CreateVsCodeProject(SrcFilePath);
            for (auto& iter: results)
                std::cout << iter << '\n';
        }
        if (dlg.isGitIgnoreAll())
        {
            ttlib::cstr GitExclude;
            if (gitIgnoreAll(GitExclude))
                std::cout << _tt(IDS_ADDED_IGNORE_FILES) << GitExclude << '\n';
        }
    }
#if defined(_WIN32)
    else if (Action & ACT_WRITE_VCX)
    {
        CVcxWrite vcx;
        return (vcx.CreateBuildFile() ? 0 : 1);
    }
#endif  // _WIN32

    // At this point we must locate a .srcfiles.yaml file. This may have been set by either -dir or -new. If not,
    // we need to locate it.

    if (!SrcFilePath.empty())
    {
        SrcFilePath.assign(locateProjectFile(RootDir));
        if (SrcFilePath.empty())
        {
            ttlib::concolor clr(ttlib::concolor::LIGHTRED);
            std::cout << _tt(IDS_CANT_FIND_SRCFILES) << '\n';
            return 1;
        }
    }

    // We now have the location of the .srcfiles.yaml file to use. If -opt was specified, then we need to let the
    // user change options before continuing.

    if (Action & ACT_OPTIONS && !(Action & ACT_NEW))
    {
        if (!ChangeOptions(SrcFilePath))
            return 1;
    }

    // At this point we have the location of .srcfiles.yaml, and we're ready to use it.

    if (Action & ACT_VSCODE)
    {
        if (Action & ACT_FORCE)
            std::filesystem::remove(".vscode/c_cpp_properties.json");
        // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
        auto results = CreateVsCodeProject(SrcFilePath);
        for (auto& iter: results)
            std::cout << iter << '\n';
    }

    if (Action & ACT_VS)
    {
        // Create .vs/ and any of the three .json files that are missing, and update c_cpp_properties.json
        std::vector<std::string> results;
        CreateVsJson(SrcFilePath.c_str(), results);
        for (auto msg: results)
            std::cout << msg << '\n';
    }

    {
        CNinja cNinja;
        if (!cNinja.IsValidVersion())
        {
            ttlib::MsgBox(_tt(IDS_OLD_TTBLD));
            return 1;
        }

        if (Action & ACT_FORCE)  // force write ignores any request for dryrun
            cNinja.ForceWrite();
        else if (Action & ACT_DRYRUN)
            cNinja.EnableDryRun();

        if (Action & ACT_ALLD)
            fs::remove("makefile");

        if (Action & ACT_ALL || Action & ACT_ALLD)
        {
            bool bAllVersion = false;
            if (cNinja.IsExeTypeLib())
                bAllVersion = true;
            cNinja.CreateMakeFile(Action & ACT_ALLNINJA, RootDir.c_str());
        }
        else
            cNinja.CreateMakeFile(Action & ACT_ALLNINJA, RootDir.c_str());

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
    }

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
