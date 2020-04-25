/////////////////////////////////////////////////////////////////////////////
// Name:      mainapp.cpp
// Purpose:   Entry point
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#ifdef _MSC_VER
    #if defined(wxMSVC_VERSION_AUTO)
        #define wxMSVC_VERSION_ABI_COMPAT
        #include <msvc/wx/setup.h>  // This will add #pragmas for the wxWidgets libraries
    #endif
    #if defined(_WIN32)
        #pragma comment(lib, "kernel32.lib")
        #pragma comment(lib, "user32.lib")
        #pragma comment(lib, "gdi32.lib")
        #pragma comment(lib, "comctl32.lib")
        #pragma comment(lib, "comdlg32.lib")
        #pragma comment(lib, "shell32.lib")

        #pragma comment(lib, "rpcrt4.lib")
        #pragma comment(lib, "advapi32.lib")
    #endif
#endif

#include <iostream>

#include <ttconsole.h>
#include <ttcstr.h>

#include "ninja.h"     // CNinja
#include "strtable.h"  // String resource IDs

void Usage()
{
    std::cout << '\n' << txtVersion << '\n' << txtCopyright << '\n';

    std::cout << _tt("\nttBld [options] -- parses .srcfiles.yaml and produces ninja build scripts\n");

    // Currently non-finished commands
    std::cout << "\n  Unfinished commands:\n"
              << _tt("    -all          -- equivalent to -allninja and -vscode\n")
              << _tt("    -alld         -- deletes makefile and .vscode/c_cpp_preferences.json before running -all\n")
              << _tt("    -allninja     -- creates/updates all .ninja scripts, creeates makefile (if missing)\n");
}

enum UPDATE_TYPE : size_t
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

enum : size_t  // actions that can be run in addition to normal single command actions
{
    // clang-format off

    ACT_ALLNINJA = 1 << 0,  // Creates/updates all .ninja scripts, creeates makefile (if missing).
    ACT_FORCE    = 1 << 1,  // Creates .ninja file even if it hasn't changed.
    ACT_ALL      = 1 << 2,  // Mostly equivalent to ACT_ALLNINJA + ACT_VSCODE
    ACT_ALLD     = 1 << 3,  // Same as ACT_ALL but forces output.
    ACT_DIR      = 1 << 4,

    // clang-format on
};

void MakeFileCaller(UPDATE_TYPE upType, const char* pszRootDir);

int main(int argc, char** argv)
{
    UPDATE_TYPE upType = UPDATE_NORMAL;
    size_t Action = 0;

    ttlib::cstr RootDir;      // this will be set if (Action & ACT_DIR) is set
    ttlib::cstr SrcFilePath;  // location of srcfiles.yaml

    for (int argpos = 1; argpos < argc && (*argv[argpos] == '-' || *argv[argpos] == '/'); ++argpos)
    {
        // Only one of these commands can be used -- after it is processes, ttBld will exit
        if (argv[argpos][1] == '?' || ttlib::issameprefix(argv[argpos] + 1, "help", tt::CASE::either))
        {
            Usage();
            return 1;
        }
        else if (ttlib::issameprefix(argv[argpos] + 1, "allninja", tt::CASE::either))  // -allninja
            Action |= ACT_ALLNINJA;
        else if (ttlib::issameprefix(argv[argpos] + 1, "alld", tt::CASE::either))  // -alld
            Action |= ACT_ALLD;
        else if (ttlib::issameprefix(argv[argpos] + 1, "all", tt::CASE::either))  // -all
            Action |= ACT_ALL;

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

    // At this point we must locate a .srcfiles.yaml file. This may have been set by either -dir or -new. If not,
    // we need to locate it.

    if (SrcFilePath.empty())
    {
        auto path = locateProjectFile(RootDir);
        if (path.empty())
        {
            ttConsoleColor clr(ttConsoleColor::LIGHTRED);
            std::cout << _tt(IDS_CANT_FIND_SRCFILES) << '\n';
            return 1;
        }
        SrcFilePath = std::move(path);
        SrcFilePath.remove_filename();
    }

    CNinja cNinja(SrcFilePath);
    if (!cNinja.IsValidVersion())
    {
        std::cout << _tt(IDS_OLD_TTBLD) << '\n';
        return 1;
    }

    if (Action & ACT_FORCE)  // force write ignores any request for dryrun
        cNinja.ForceWrite();

    if (Action & ACT_ALLD)
    {
        fs::remove("makefile");
        cNinja.CreateMakeFile(true, RootDir);
    }

    if (Action & ACT_ALL)
    {
        cNinja.CreateMakeFile(true, RootDir);
    }

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
    if (!cNinja.GetHHPName().empty())
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
                    std::cout << cNinja.GetScriptFile() << _tt(IDS_UPDATED) << '\n');
                break;

            case UPDATE_CLANG_CL:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_CLANG))
                    std::cout << cNinja.GetScriptFile() << _tt(IDS_UPDATED) << '\n');
                break;

            case UPDATE_CLANG_CL32:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_CLANG))
                    std::cout << cNinja.GetScriptFile() << _tt(IDS_UPDATED) << '\n');
                break;

            case UPDATE_MSVCD:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_MSVC))
                    std::cout << cNinja.GetScriptFile() << _tt(IDS_UPDATED) << '\n');
                break;

            case UPDATE_MSVC32D:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_MSVC))
                    std::cout << cNinja.GetScriptFile() << _tt(IDS_UPDATED) << '\n');
                break;

            case UPDATE_CLANG_CLD:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_CLANG))
                    std::cout << cNinja.GetScriptFile() << _tt(IDS_UPDATED) << '\n');
                break;

            case UPDATE_CLANG_CL32D:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_CLANG))
                    std::cout << cNinja.GetScriptFile() << _tt(IDS_UPDATED) << '\n');
                break;

            default:
                break;
        }
    }
}
