/////////////////////////////////////////////////////////////////////////////
// Name:      mainapp.cpp
// Purpose:   entry point, global strings, library pragmas
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#ifdef _MSC_VER
    #define wxMSVC_VERSION_ABI_COMPAT
    #include <msvc/wx/setup.h>  // This will add #pragmas for the wxWidgets libraries

    #if defined(_WIN32)

        #pragma comment(lib, "kernel32.lib")
        #pragma comment(lib, "user32.lib")
        #pragma comment(lib, "gdi32.lib")
        #pragma comment(lib, "comctl32.lib")
        #pragma comment(lib, "comdlg32.lib")
        #pragma comment(lib, "shell32.lib")

        #pragma comment(lib, "rpcrt4.lib")
        #pragma comment(lib, "advapi32.lib")

        #if wxUSE_URL_NATIVE
            #pragma comment(lib, "wininet.lib")
        #endif
    #endif
#endif

#include <wx/init.h>
#include <wx/cmdline.h>
#include <wx/init.h>
#include <wx/wxcrtvararg.h>
#include <wx/config.h>

#include "mainapp.h"  // CMainApp -- Main application class

const char* txtVersion = "ttBld 1.3.0.8295";
const char* txtCopyRight = "Copyright (c) 2002-2019 KeyWorks Software";
const char* txtAppName = "ttBld";

#include <iostream>
#include <direct.h>  // Functions for directory handling and creation

#include <ttconsole.h>  // ttConsoleColor

#include "convertdlg.h"  // CConvertDlg
#include "ninja.h"       // CNinja
#include "vcxproj.h"     // CVcxWrite
#include "funcs.h"       // List of function declarations

#if defined(TESTING)
    #include "dlgvscode.h"  // CDlgVsCode -- IDDLG_VSCODE dialog handler
#endif

wxIMPLEMENT_APP_CONSOLE(CMainApp);

int oldMain(int argc, char** argv);

bool CMainApp::OnInit()
{
    SetAppDisplayName(txtAppName);
    SetVendorName("KeyWorks");
    SetVendorDisplayName("KeyWorks Software");

    return true;
}

int CMainApp::OnRun()
{
    return oldMain(argc, argv);
}

void Usage()
{
    puts("");
    puts(txtVersion);
    puts(txtCopyRight);

    // clang-format off

    puts(_("\nttBld [options] -- parses .srcfiles.yaml and produces ninja build scripts\n"));
    puts(_("    -dir [directory] -- uses specified directory to create/maintain .srcfiles.yaml and build/*.ninja\n"));

    puts(_("    -options   -- displays a dialog allowing you to change options in .srcfiles.yaml"));
#if defined(_WIN32)
    puts(_("    -codecmd   -- creates code32.cmd and code64.cmd in same directory as code.cmd"));
#endif
    puts(_("    -new       -- displays a dialog allowing you to create a new .srcfiles.yaml file"));

    puts(_("    -vs        -- creates files used to build and debug a project using Visual Studio"));
    puts(_("    -vscode    -- creates or updates files used to build and debug a project using VS Code"));

    // Currently non-finished commands

    puts("\b  Unfinished commands:\n");
    puts(_("    -all          -- equivalent to -allninja and -vscode"));
    puts(_("    -alld         -- deletes makefile and .vscode/c_cpp_preferences.json before running -all"));
    puts(_("    -allninja     -- creates/updates all .ninja scripts, creeates makefile (if missing)"));
    puts(_("    -convert file -- Converts build script file (.e.g., file.vcxproj) to .srcfiles.yaml"));

    // clang-format on

    // Currently hidden or non-finished commands

    // puts("\t-add [file(s)] -- Adds file(s) to .srcfiles");
    // puts(_("    -dryrun     -- displays what would have happened, but doesn't change anything"));
    // puts(_("    -force      -- create .ninja file even if it hasn't changed"));
    // puts(_("    -msvcenv32  -- creates MSVCenv.cmd file in same location as code.cmd"));
    // puts(_("    -msvcenv64  -- create .ninja file even if it hasn't changed"));

    // puts(_("    -help       -- displays usage information"));
    // puts(_("    -?          -- displays usage information"));

    // Following commands are no longer supported:

    // puts("\nIDE workspace options:");
    // puts(_("    -vcxproj    -- creates or updates files needed to build project using MS Visual Studio"));
    // puts(_("    -codelite   -- creates or updates files needed to build project using CodeLite"));
    // puts(_("    -codeblocks -- creates or updates files needed to build project using CodeBlocks"));

#if defined(TESTING)
    puts("    -tvdlg    -- tests the CDlgVsCode dialog");
#endif
    // -bwt -- sets break on warning to true
    // -bwf -- sets break on warning to false
}

typedef enum
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

} UPDATE_TYPE;
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

int oldMain(int argc, char* argv[])
{
    UPDATE_TYPE upType = UPDATE_NORMAL;
    size_t      Action = 0;
    ttCStr      cszRootDir;      // this will be set if (Action & ACT_DIR) is set
    ttCStr      cszSrcFilePath;  // location of srcfiles.yaml

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

    const char* pszEnv = getenv("TTBLD");
    if (pszEnv)
    {
        char* pszArg = ttFindNonSpace(pszEnv);
        while (*pszArg)
        {
            if (*pszArg == '-' || *pszArg == '/')
            {
                ++pszArg;

                // Only a few or our standard arguments will actually make sense as an environment variable

                if (ttIsSameSubStrI(pszArg, "vscode"))
                    Action |= ACT_VSCODE;
            }
            pszArg = ttStepOver(pszArg);
        }
    }

    for (int argpos = 1; argpos < argc && (*argv[argpos] == '-' || *argv[argpos] == '/'); ++argpos)
    {
        // Only one of these commands can be used -- after it is processes, ttBld will exit
        if (argv[argpos][1] == '?' || ttIsSameSubStrI(argv[argpos] + 1, "help"))
        {
            Usage();
            return 1;
        }
        else if (ttIsSameStrI(argv[argpos] + 1, "add"))
        {
            ttCList lstFiles;
            for (++argpos; argpos < argc; ++argpos)
                lstFiles += argv[argpos];
            AddFiles(lstFiles, Action & ACT_DRYRUN);
            return 1;
        }
        else if (ttIsSameStrI(argv[argpos] + 1, "convert"))
        {
            if (++argpos < argc)
                cszSrcFilePath = argv[argpos++];
            Action |= ACT_CONVERT;
        }
        else if (ttIsSameSubStrI(argv[argpos] + 1, "codecmd"))  // used in case it was set in environment
        {
            CreateCodeCmd("code32.cmd");
            CreateCodeCmd("code64.cmd");
            return 1;
        }
        else if (ttIsSameSubStrI(argv[argpos] + 1, "msvcenv64"))  // used in case it was set in environment
        {
            if (argpos + 1 < argc)
            {
                CreateMSVCEnvCmd(argv[argpos + 1], true);
                return 1;
            }
            return 0;
        }
        else if (ttIsSameSubStrI(argv[argpos] + 1, "msvcenv32"))  // used in case it was set in environment
        {
            if (argpos + 1 < argc)
            {
                CreateMSVCEnvCmd(argv[argpos + 1], false);
                return 1;
            }
            return 0;
        }
#if defined(TESTING) && defined(_DEBUG)

        else if (ttIsSameSubStrI(argv[argpos] + 1, "tvdlg"))  // used in case it was set in environment
        {
            CDlgVsCode dlg;
            if (dlg.DoModal(NULL) != IDOK)
                return 1;

            DeleteFileA(".vscode/launch.json");
            DeleteFileA(".vscode/tasks.json");

            // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
            ttCList lstResults;
            CreateVsCodeProject(cszSrcFilePath, &lstResults);
            for (size_t pos = 0; lstResults.InRange(pos); ++pos)
                puts(lstResults[pos]);
        }
#endif

#if !defined(NDEBUG)  // Starts debug section.
        else if (ttIsSameSubStrI(argv[argpos] + 1, "bwt"))
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

        else if (ttIsSameSubStrI(argv[argpos] + 1, "allninja"))  // -allninja
            Action |= ACT_ALLNINJA;
        else if (ttIsSameSubStrI(argv[argpos] + 1, "alld"))  // -alld
            Action |= ACT_ALLD;
        else if (ttIsSameSubStrI(argv[argpos] + 1, "all"))  // -all
            Action |= ACT_ALL;

        else if (ttIsSameSubStrI(argv[argpos] + 1, "dry"))  // -dryryn
            Action |= ACT_DRYRUN;
        else if (ttIsSameStrI(argv[argpos] + 1, "new"))
            Action |= ACT_NEW;
        else if (ttIsSameSubStrI(argv[argpos] + 1, "opt"))  // -options
            Action |= ACT_OPTIONS;
        else if (ttIsSameSubStrI(argv[argpos] + 1, "vscode"))  // check this before "vs"
            Action |= ACT_VSCODE;
        else if (ttIsSameSubStrI(argv[argpos] + 1, "vs"))
            Action |= ACT_VS;
        else if (ttIsSameSubStrI(argv[argpos] + 1, "force"))  // write ninja file even if it hasn't changed
            Action |= ACT_FORCE;
        // -dir base_directory (used to specify directory for .srcfiles.yaml, makefile, and build directory)
        else if (ttIsSameSubStrI(argv[argpos] + 1, "dir"))
        {
            ++argpos;
            if (argpos > argc || (*argv[argpos] == '-' || *argv[argpos] == '/'))
            {
                ttConsoleColor clr(ttConsoleColor::LIGHTRED);
                puts(_("-dir must be followed by the directory to use."));
                return 1;
            }
            cszRootDir = argv[argpos];
            Action |= ACT_DIR;
            cszSrcFilePath = cszRootDir;
            cszSrcFilePath.AppendFileName("srcfiles.yaml");
            continue;
        }

        // The following commands are called from a makefile to update one .ninja script and immediately exit

        else if (ttIsSameStrI(argv[argpos] + 1, "umsvc"))
            upType = UPDATE_MSVC;
        else if (ttIsSameStrI(argv[argpos] + 1, "umsvc_x86"))
            upType = UPDATE_MSVC32;
        else if (ttIsSameStrI(argv[argpos] + 1, "uclang"))
            upType = UPDATE_CLANG_CL;
        else if (ttIsSameStrI(argv[argpos] + 1, "uclang_x86"))
            upType = UPDATE_CLANG_CL32;
        else if (ttIsSameStrI(argv[argpos] + 1, "umsvcD"))
            upType = UPDATE_MSVCD;
        else if (ttIsSameStrI(argv[argpos] + 1, "umsvc_x86D"))
            upType = UPDATE_MSVC32D;
        else if (ttIsSameStrI(argv[argpos] + 1, "uclangD"))
            upType = UPDATE_CLANG_CLD;
        else if (ttIsSameStrI(argv[argpos] + 1, "uclang_x86D"))
            upType = UPDATE_CLANG_CL32D;
    }

    // If we are being called from a makefile then this is the only option we will now process. Note that we ignore
    // -dryrun even though we are generating a .ninja script. We do process the -dir option.

    if (upType != UPDATE_NORMAL)
    {
        MakeFileCaller(upType, cszRootDir);
        return 0;
    }

    // The order that we process switches is important. -new comes first since we can only continue to run
    // if we have a .srcfiles.yaml file to work with.

    if (Action & ACT_NEW)
    {
        // -dir option will have set cszSrcFilePath, if option not specified, default to current directory
        CConvertDlg dlg(cszSrcFilePath.IsNonEmpty() ? (char*) cszSrcFilePath : ".srcfiles.yaml");
        if (dlg.DoModal(NULL) != IDOK)
            return 1;
        cszSrcFilePath = dlg.GetOutSrcFiles();

        // [KeyWorks - 7/30/2019] If the conversion dialog completed successfully, then the new .srcfiles.yaml has
        // been created. We call ChangeOptions() in case the user wants to tweak anything, but it's fine if the
        // user wants to cancel -- that just means they didn't want to change any options. It does NOT mean they
        // want to cancel running any other commands, such as makefile creation (which doesn't need to know what
        // options are set in .srcfiles.yaml).

        ChangeOptions(&cszSrcFilePath);
        if (dlg.isCreateVsCode())
        {
            // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
            ttCList lstResults;
            CreateVsCodeProject(cszSrcFilePath, &lstResults);
            for (size_t pos = 0; lstResults.InRange(pos); ++pos)
                puts(lstResults[pos]);
        }
        if (dlg.isGitIgnoreAll())
        {
            ttCStr cszGitExclude;
            if (gitIgnoreAll(cszGitExclude))
                printf("Added directories and filenames to ignore to %s\n", (char*) cszGitExclude);
        }
    }
    else if (Action & ACT_CONVERT)
    {
        // -dir option will have set cszSrcFilePath, if option not specified, default to current directory
        CConvertDlg dlg;
        dlg.SetConvertScritpt(cszSrcFilePath);
        if (dlg.DoModal(NULL) != IDOK)
            return 1;
        cszSrcFilePath = dlg.GetOutSrcFiles();

        // [KeyWorks - 7/30/2019] If the conversion dialog completed successfully, then the new .srcfiles.yaml has
        // been created. We call ChangeOptions() in case the user wants to tweak anything, but it's fine if the
        // user wants to cancel -- that just means they didn't want to change any options. It does NOT mean they
        // want to cancel running any other commands, such as makefile creation (which doesn't need to know what
        // options are set in .srcfiles.yaml).

        ChangeOptions(&cszSrcFilePath);
        if (dlg.isCreateVsCode())
        {
            // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
            ttCList lstResults;
            CreateVsCodeProject(cszSrcFilePath, &lstResults);
            for (size_t pos = 0; lstResults.InRange(pos); ++pos)
                puts(lstResults[pos]);
        }
        if (dlg.isGitIgnoreAll())
        {
            ttCStr cszGitExclude;
            if (gitIgnoreAll(cszGitExclude))
                printf("Added directories and filenames to ignore to %s\n", (char*) cszGitExclude);
        }
    }

    // At this point we must locate a .srcfiles.yaml file. This may have been set by either -dir or -new. If not,
    // we need to locate it.

    if (cszSrcFilePath.IsEmpty())
    {
        if (cszRootDir.IsNonEmpty())
        {
            cszSrcFilePath = cszRootDir;
            if (!LocateSrcFiles(&cszSrcFilePath))
            {
                ttConsoleColor clr(ttConsoleColor::LIGHTRED);
                puts(_("ttBld was unable to locate a .srcfiles.yaml file -- either use the -new option, or set "
                       "the location with -dir."));
                return 1;
            }
        }
        else
        {
            const char* pszFile = LocateSrcFiles();
            if (pszFile)
                cszSrcFilePath = pszFile;
            else
            {
                ttConsoleColor clr(ttConsoleColor::LIGHTRED);
                puts(_("ttBld was unable to locate a .srcfiles.yaml file -- either use the -new option, or set "
                       "the location with -dir."));
                return 1;
            }
        }
    }

    // We now have the location of the .srcfiles.yaml file to use. If -opt was specified, then we need to let the
    // user change options before continuing.

    if (Action & ACT_OPTIONS && !(Action & ACT_NEW))
    {
        if (!ChangeOptions(&cszSrcFilePath, Action & ACT_DRYRUN))
            return 1;
    }

    // At this point we have the location of .srcfiles.yaml, and we're ready to use it.

    if (Action & ACT_VSCODE || Action & ACT_ALL || Action & ACT_ALLD)
    {
        if (Action & ACT_ALLD)
            wxRemoveFile(".vscode/c_cpp_properties.json");
        // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
        ttCList lstResults;
        CreateVsCodeProject(cszSrcFilePath, &lstResults);
        for (size_t pos = 0; lstResults.InRange(pos); ++pos)
            puts(lstResults[pos]);
    }

    if (Action & ACT_VS)
    {
        // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
        ttCList lstResults;
        CreateVsJson(cszSrcFilePath, &lstResults);
        for (size_t pos = 0; lstResults.InRange(pos); ++pos)
            puts(lstResults[pos]);
    }

    {
        CNinja cNinja;
        if (!cNinja.IsValidVersion())
        {
            if (ttMsgBox(_("This version of ttBld is too old -- create ninja scripts anyway?"),
                         MB_YESNO | MB_ICONWARNING) != IDYES)
                return 1;
        }

        if (Action & ACT_FORCE)  // force write ignores any request for dryrun
            cNinja.ForceWrite();
        else if (Action & ACT_DRYRUN)
            cNinja.EnableDryRun();

        if (Action & ACT_ALLD)
            wxRemoveFile("makefile");

        if (Action & ACT_ALL || Action & ACT_ALLD)
        {
            bool bAllVersion = false;
            if (cNinja.IsExeTypeLib())
                bAllVersion = true;
            cNinja.CreateMakeFile(Action & ACT_ALLNINJA, cszRootDir);
        }
        else
            cNinja.CreateMakeFile(Action & ACT_ALLNINJA, cszRootDir);

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

        if (ttIsNonEmpty(cNinja.GetHHPName()))
            cNinja.CreateHelpFile();

        // Display any errors that occurred during processing

        if (cNinja.GetErrorCount())
        {
            ttConsoleColor clr(ttConsoleColor::LIGHTRED);
            for (size_t pos = 0; pos < cNinja.GetErrorCount(); pos++)
                puts(cNinja.GetError(pos));
            puts("");
        }

        if (countNinjas > 0)
            printf(_("Created %d .ninja files\n"), countNinjas);
        else
            puts(_("All ninja scripts are up to date."));
    }

    return 0;
}

void MakeFileCaller(UPDATE_TYPE upType, const char* pszRootDir)
{
    // TODO: [KeyWorks - 7/30/2019] Need to change CNinja to accept a root dir instead of bVsCodeDir, then we can
    // pass in pszRootDir.

    CNinja cNinja(pszRootDir);
    cNinja.ForceWrite();

    if (cNinja.IsValidVersion())
    {
        switch (upType)
        {
            case UPDATE_MSVC:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_MSVC))
                    printf(_("%s updated.\n"), cNinja.GetScriptFile());
                break;

            case UPDATE_MSVC32:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_MSVC))
                    printf(_("%s updated.\n"), cNinja.GetScriptFile());
                break;

            case UPDATE_CLANG_CL:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_CLANG))
                    printf(_("%s updated.\n"), cNinja.GetScriptFile());
                break;

            case UPDATE_CLANG_CL32:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_CLANG))
                    printf(_("%s updated.\n"), cNinja.GetScriptFile());
                break;

            case UPDATE_MSVCD:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_MSVC))
                    printf(_("%s updated.\n"), cNinja.GetScriptFile());
                break;

            case UPDATE_MSVC32D:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_MSVC))
                    printf(_("%s updated.\n"), cNinja.GetScriptFile());
                break;

            case UPDATE_CLANG_CLD:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_CLANG))
                    printf(_("%s updated.\n"), cNinja.GetScriptFile());
                break;

            case UPDATE_CLANG_CL32D:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_CLANG))
                    printf(_("%s updated.\n"), cNinja.GetScriptFile());
                break;

            default:
                break;
        }
    }

    else
    {
        ttConsoleColor clr(ttConsoleColor::LIGHTRED);
        puts(_("This version of ttBld is too old to properly create ninja scripts from your current srcfiles."));
    }
}
