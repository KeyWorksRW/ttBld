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

#include "convertdlg.h"  // CConvertDlg
#include "funcs.h"       // List of function declarations
#include "ninja.h"       // CNinja
#include "vcxproj.h"     // CVcxWrite

#if defined(TESTING)
    #include "dlgvscode.h"  // CDlgVsCode -- IDDLG_VSCODE dialog handler
#endif

void AddFiles(const ttlib::cstrVector& lstFiles);

// clang-format off
#if defined(wxWIDGETS)
    wxIMPLEMENT_APP_CONSOLE(CMainApp);

    int oldMain(int argc, char** argv);

    bool CMainApp::OnInit()
    {
        SetAppDisplayName(txtAppname);
        SetVendorName("KeyWorks");
        SetVendorDisplayName("KeyWorks Software");

        return true;
    }

    int CMainApp::OnRun()
    {
        return oldMain(argc, argv);
    }
#endif
// clang-format on

void Usage()
{
    std::cout << '\n' << txtVersion << '\n' << txtCopyright << '\n';

    std::cout
        << _tt("\nttBld [options] -- parses .srcfiles.yaml and produces ninja build scripts\n")
        << _tt("    -dir [directory] -- uses specified directory to create/maintain .srcfiles.yaml and "
               "build/*.ninja\n")
        << _tt("    -add [file(s)] -- Adds file(s) to .srcfiles")
        << _tt("    -options   -- displays a dialog allowing you to change options in .srcfiles.yaml\n")
        << _tt("    -codecmd   -- creates code32.cmd and code64.cmd in same directory as code.cmd\n")
        << _tt("    -new       -- displays a dialog allowing you to create a new .srcfiles.yaml file\n")
        << _tt("    -vs        -- creates files used to build and debug a project using Visual Studio\n")
        << _tt("    -vscode    -- creates or updates files used to build and debug a project using VS Code\n");

    // Currently non-finished commands
    std::cout
        << "\n  Unfinished commands:\n"
        << _tt("    -all          -- equivalent to -allninja and -vscode\n")
        << _tt("    -alld         -- deletes makefile and .vscode/c_cpp_preferences.json before running -all\n")
        << _tt("    -allninja     -- creates/updates all .ninja scripts, creeates makefile (if missing)\n")
        << _tt("    -convert file -- Converts build script file (.e.g., file.vcxproj) to .srcfiles.yaml\n");

    // Currently hidden or non-finished commands

    // puts("\t-add [file(s)] -- Adds file(s) to .srcfiles");
    // puts(_tt("    -dryrun     -- displays what would have happened, but doesn't change anything"));
    // puts(_tt("    -force      -- create .ninja file even if it hasn't changed"));
    // puts(_tt("    -msvcenv32  -- creates MSVCenv.cmd file in same location as code.cmd"));
    // puts(_tt("    -msvcenv64  -- create .ninja file even if it hasn't changed"));

    // puts(_tt("    -help       -- displays usage information"));
    // puts(_tt("    -?          -- displays usage information"));

    // Following commands are no longer supported:

    // puts("\nIDE workspace options:");
    // puts(_tt("    -vcxproj    -- creates or updates files needed to build project using MS Visual Studio"));
    // puts(_tt("    -codelite   -- creates or updates files needed to build project using CodeLite"));
    // puts(_tt("    -codeblocks -- creates or updates files needed to build project using CodeBlocks"));

#if defined(TESTING)
    std::cout << "    -tvdlg    -- tests the CDlgVsCode dialog\n";
#endif
    // -bwt -- sets break on warning to true
    // -bwf -- sets break on warning to false
}

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

enum  // actions that can be run in addition to normal single command actions
{
    // clang-format off

    ACT_DRYRUN   = 1 << 0,  // This is a dry run -- don't actually output anything.
    ACT_VS       = 1 << 1,
    ACT_VSCODE   = 1 << 2,  // Generate .vscode directory and .json files.
    ACT_DIR      = 1 << 3,
    ACT_MAKEFILE = 1 << 4,
    ACT_ALLNINJA = 1 << 5,  // Creates/updates all .ninja scripts, creeates makefile (if missing).
    ACT_NEW      = 1 << 6,  // Displays a dialog for creating a new .srcfiles.yaml file.
    ACT_FORCE    = 1 << 7,  // Creates .ninja file even if it hasn't changed.
    ACT_OPTIONS  = 1 << 8,  // Displays a dialog for changing options in .srcfiles.yaml
    ACT_CONVERT  = 1 << 9,  // Converts the specified build script file to .srcfiles.yaml
    ACT_ALL      = 1 << 10,  // Mostly equivalent to ACT_ALLNINJA + ACT_VSCODE
    ACT_ALLD     = 1 << 11,  // Same as ACT_ALL but forces output.

    // clang-format on
};

#if defined(wxWIDGETS)

int oldMain(int argc, char* argv[])

#else

int main(int argc, char* argv[])

#endif
{
    UPDATE_TYPE upType = UPDATE_NORMAL;
    size_t Action = 0;

    ttlib::cstr RootDir;      // this will be set if (Action & ACT_DIR) is set
    ttlib::cstr SrcFilePath;  // location of srcfiles.yaml

// Change 0 to 1 to confirm that our locating functions are actually working as expected
#if 0 && !defined(NDEBUG) && defined(_WIN32)
    {
        ttCStr cszTest;
        if (FindCurMsvcPath(cszTest))
            puts((char*) cszTest);
        else
            puts("Cannot locate MSVC path");

        if (FindVsCode(cszTest))
            puts((char*) cszTest);
        else
            puts("Cannot locate VSCode.exe");

        if (FindFileEnv("PATH", "mingw32-make.exe", cszTest))
            puts((char*) cszTest);
        else
            puts("Cannot locate mingw32-make.exe");
    }
#endif

    auto pszEnv = std::getenv("TTBLD");
    if (pszEnv)
    {
        auto pszArg = ttFindNonSpace(pszEnv);
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

    for (int argpos = 1; argpos < argc && (*argv[argpos] == '-' || *argv[argpos] == '/'); ++argpos)
    {
        // Only one of these commands can be used -- after it is processes, ttBld will exit
        if (argv[argpos][1] == '?' || ttlib::issameprefix(argv[argpos] + 1, "help", tt::CASE::either))
        {
            Usage();
            return 1;
        }
        else if (ttlib::issameas(argv[argpos] + 1, "add", tt::CASE::either))
        {
            ttlib::cstrVector lstFiles;
            for (++argpos; argpos < argc; ++argpos)
                lstFiles += argv[argpos];
            AddFiles(lstFiles);
            return 1;
        }
        else if (ttlib::issameas(argv[argpos] + 1, "convert", tt::CASE::either))
        {
            if (++argpos < argc)
                SrcFilePath = argv[argpos++];
            Action |= ACT_CONVERT;
        }
        else if (ttIsSameSubStrI(argv[argpos] + 1, "codecmd"))  // used in case it was set in environment
        {
            CreateCodeCmd("code32.cmd");
            CreateCodeCmd("code64.cmd");
            return 1;
        }
        else if (ttlib::issameprefix(argv[argpos] + 1, "msvcenv64", tt::CASE::either))
        {
            // used in case it was set in environment
            if (argpos + 1 < argc)
            {
                CreateMSVCEnvCmd(argv[argpos + 1], true);
                return 1;
            }
            return 0;
        }
        else if (ttlib::issameprefix(argv[argpos] + 1, "msvcenv32", tt::CASE::either))
        {
            // used in case it was set in environment
            if (argpos + 1 < argc)
            {
                CreateMSVCEnvCmd(argv[argpos + 1], false);
                return 1;
            }
            return 0;
        }
#if defined(TESTING) && defined(_DEBUG)

        else if (ttlib::issameprefix(argv[argpos] + 1, "tvdlg", tt::CASE::either))
        {
            // used in case it was set in environment
            CDlgVsCode dlg;
            if (dlg.DoModal(NULL) != IDOK)
                return 1;

            fs::remove(".vscode/launch.json");
            fs::remove(".vscode/tasks.json");

            // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
            std::vector<std::string> Results;
            CreateVsCodeProject(SrcFilePath, &Results);
            for (auto& iter: Results)
                std::cout << iter << '\n';
        }
#endif

#if !defined(NDEBUG) && defined(wxWidgets)  // Starts debug section.
        else if (ttlib::issameprefix(argv[argpos] + 1, "bwt", tt::CASE::either))
        {
            wxConfig config("ttBld");
            config.SetPath("/Settings");
            config.Write("BreakOnWarning", true);
        }
        else if (ttIsSameSubStrI(argv[argpos] + 1, "bwf"))
        {
            wxConfig config("ttBld");
            config.SetPath("/Settings");
            config.Write("BreakOnWarning", false);
        }
#endif

        // The following commands can be combined

        else if (ttlib::issameprefix(argv[argpos] + 1, "allninja", tt::CASE::either))
            Action |= ACT_ALLNINJA;
        else if (ttlib::issameprefix(argv[argpos] + 1, "alld", tt::CASE::either))
            Action |= ACT_ALLD;
        else if (ttlib::issameprefix(argv[argpos] + 1, "all", tt::CASE::either))
            Action |= ACT_ALL;

        else if (ttlib::issameprefix(argv[argpos] + 1, "dry", tt::CASE::either))
            Action |= ACT_DRYRUN;
        else if (ttlib::issameas(argv[argpos] + 1, "new", tt::CASE::either))
            Action |= ACT_NEW;
        else if (ttlib::issameprefix(argv[argpos] + 1, "opt", tt::CASE::either))
            Action |= ACT_OPTIONS;
        else if (ttlib::issameprefix(argv[argpos] + 1, "vscode", tt::CASE::either))  // check this before "vs"
            Action |= ACT_VSCODE;
        else if (ttlib::issameprefix(argv[argpos] + 1, "vs", tt::CASE::either))
            Action |= ACT_VS;
        else if (ttlib::issameprefix(argv[argpos] + 1, "force", tt::CASE::either))
            Action |= ACT_FORCE;
        // -dir base_directory (used to specify directory for .srcfiles.yaml, makefile, and build directory)
        else if (ttlib::issameprefix(argv[argpos] + 1, "dir", tt::CASE::either))
        {
            ++argpos;
            if (argpos > argc || (*argv[argpos] == '-' || *argv[argpos] == '/'))
            {
                ttConsoleColor clr(ttConsoleColor::LIGHTRED);
                std::cout << _tt("-dir must be followed by the directory to use.") << '\n';
                return 1;
            }
            RootDir = argv[argpos];
            Action |= ACT_DIR;
            SrcFilePath = RootDir;
            SrcFilePath.append_filename("srcfiles.yaml");
            continue;
        }

        // The following commands are called from a makefile to update one .ninja script and immediately exit

        else if (ttlib::issameas(argv[argpos] + 1, "umsvc", tt::CASE::either))
            upType = UPDATE_MSVC;
        else if (ttlib::issameas(argv[argpos] + 1, "umsvc_x86", tt::CASE::either))
            upType = UPDATE_MSVC32;
        else if (ttlib::issameas(argv[argpos] + 1, "uclang", tt::CASE::either))
            upType = UPDATE_CLANG_CL;
        else if (ttlib::issameas(argv[argpos] + 1, "uclang_x86", tt::CASE::either))
            upType = UPDATE_CLANG_CL32;
        else if (ttlib::issameas(argv[argpos] + 1, "umsvcD", tt::CASE::either))
            upType = UPDATE_MSVCD;
        else if (ttlib::issameas(argv[argpos] + 1, "umsvc_x86D", tt::CASE::either))
            upType = UPDATE_MSVC32D;
        else if (ttlib::issameas(argv[argpos] + 1, "uclangD", tt::CASE::either))
            upType = UPDATE_CLANG_CLD;
        else if (ttlib::issameas(argv[argpos] + 1, "uclang_x86D", tt::CASE::either))
            upType = UPDATE_CLANG_CL32D;
    }

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
            std::vector<std::string> Results;
            CreateVsCodeProject(SrcFilePath, Results);
            for (auto iter: Results)
                std::cout << iter << '\n';
        }
        if (dlg.isGitIgnoreAll())
        {
            ttlib::cstr GitExclude;
            if (gitIgnoreAll(GitExclude))
            {
                std::cout << _tt("Added directories and filenames to ignore to ") << GitExclude << '\n';
            }
        }
    }
    else if (Action & ACT_CONVERT)
    {
        // -dir option will have set SrcFilePath, if option not specified, default to current directory
        CConvertDlg dlg;
        dlg.SetConvertScritpt(SrcFilePath.c_str());
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
            std::vector<std::string> Results;
            CreateVsCodeProject(SrcFilePath, Results);
            for (auto iter: Results)
                std::cout << iter << '\n';
        }
        if (dlg.isGitIgnoreAll())
        {
            ttlib::cstr GitExclude;
            if (gitIgnoreAll(GitExclude))
                std::cout << _tt("Added directories and filenames to ignore to ") << GitExclude << '\n';
        }
    }

    // At this point we must locate a .srcfiles.yaml file. This may have been set by either -dir or -new. If not,
    // we need to locate it.

    if (!SrcFilePath.empty())
    {
        SrcFilePath.assign(locateProjectFile(RootDir));
        if (SrcFilePath.empty())
        {
            ttConsoleColor clr(ttConsoleColor::LIGHTRED);
            std::cout << _tt("ttBld was unable to locate a .srcfiles.yaml file -- either use the -new option, "
                             "or set the location with -dir.")
                      << '\n';
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

    if (Action & ACT_VSCODE || Action & ACT_ALL || Action & ACT_ALLD)
    {
        if (Action & ACT_ALLD)
            std::filesystem::remove(".vscode/c_cpp_properties.json");
        // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
        std::vector<std::string> Results;
        CreateVsCodeProject(SrcFilePath, Results);
        for (auto msg: Results)
            std::cout << msg << '\n';
    }

    if (Action & ACT_VS)
    {
        // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
        std::vector<std::string> results;
        CreateVsJson(SrcFilePath.c_str(), results);
        for (auto msg: results)
            std::cout << msg << '\n';
    }

    {
        CNinja cNinja;
        if (!cNinja.IsValidVersion())
        {
            if (ttlib::MsgBox(_tt("This version of ttBld is too old -- create ninja scripts anyway?"),
                              MB_YESNO | MB_ICONWARNING) != IDYES)
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
            ttConsoleColor clr(ttConsoleColor::LIGHTRED);
            for (auto iter: cNinja.getErrorMsgs())
            {
                std::cout << iter << '\n';
            }
            std::cout << "\n\n";
        }

        if (countNinjas > 0)
        {
            std::cout << "Created " << countNinjas << " .ninja files" << '\n';
        }
        else
            std::cout << _tt("All ninja scripts are up to date.") << '\n';
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
                    std::cout << cNinja.GetScriptFile() << _tt(" updated.\n");
                break;

            case UPDATE_MSVC32:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_MSVC))
                    std::cout << cNinja.GetScriptFile() << _tt(" updated.\n");
                break;

            case UPDATE_CLANG_CL:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_CLANG))
                    std::cout << cNinja.GetScriptFile() << _tt(" updated.\n");
                break;

            case UPDATE_CLANG_CL32:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_CLANG))
                    std::cout << cNinja.GetScriptFile() << _tt(" updated.\n");
                break;

            case UPDATE_MSVCD:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_MSVC))
                    std::cout << cNinja.GetScriptFile() << _tt(" updated.\n");
                break;

            case UPDATE_MSVC32D:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_MSVC))
                    std::cout << cNinja.GetScriptFile() << _tt(" updated.\n");
                break;

            case UPDATE_CLANG_CLD:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_CLANG))
                    std::cout << cNinja.GetScriptFile() << _tt(" updated.\n");
                break;

            case UPDATE_CLANG_CL32D:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_CLANG))
                    std::cout << cNinja.GetScriptFile() << _tt(" updated.\n");
                break;

            default:
                break;
        }
    }

    else
    {
        ttConsoleColor clr(ttConsoleColor::LIGHTRED);
        std::cout << _tt(
            "This version of ttBld is too old to properly create ninja scripts from your current srcfiles.");
    }
}
