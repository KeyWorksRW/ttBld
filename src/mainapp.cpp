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

#include "ninja.h"  // CNinja

void Usage()
{
    std::cout << '\n' << txtVersion << '\n' << txtCopyright << '\n';

    std::cout << _tt("\nttBld [options] -- parses .srcfiles.yaml and produces ninja build scripts\n");

    // Currently non-finished commands
    std::cout
        << "\n  Unfinished commands:\n"
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
    size_t      Action = 0;

    ttString RootDir;      // this will be set if (Action & ACT_DIR) is set
    ttString SrcFilePath;  // location of srcfiles.yaml

    for (int argpos = 1; argpos < argc && (*argv[argpos] == '-' || *argv[argpos] == '/'); ++argpos)
    {
        // Only one of these commands can be used -- after it is processes, ttBld will exit
        if (argv[argpos][1] == '?' || ttIsSameSubStrI(argv[argpos] + 1, "help"))
        {
            Usage();
            return 1;
        }
        else if (ttIsSameSubStrI(argv[argpos] + 1, "allninja"))  // -allninja
            Action |= ACT_ALLNINJA;
        else if (ttIsSameSubStrI(argv[argpos] + 1, "alld"))  // -alld
            Action |= ACT_ALLD;
        else if (ttIsSameSubStrI(argv[argpos] + 1, "all"))  // -all
            Action |= ACT_ALL;

        // -dir base_directory (used to specify directory for .srcfiles.yaml, makefile, and build directory)
        else if (ttIsSameSubStrI(argv[argpos] + 1, "dir"))
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
        MakeFileCaller(upType, RootDir.c_str());
        return 0;
    }

    // At this point we must locate a .srcfiles.yaml file. This may have been set by either -dir or -new. If not,
    // we need to locate it.

    if (SrcFilePath.empty())
    {
        auto pPath = locateProjectFile(RootDir);
        if (pPath->empty())
        {
            ttConsoleColor clr(ttConsoleColor::LIGHTRED);
            std::cout << _tt("ttBld was unable to locate a .srcfiles.yaml file -- either use the -new option, "
                             "or set the location with -dir.")
                      << '\n';
            return 1;
        }
        SrcFilePath = *pPath;
    }

    CNinja cNinja;
    if (!cNinja.IsValidVersion())
    {
        if (ttMsgBox(_tt("This version of ttBld is too old -- create ninja scripts anyway?"),
                     MB_YESNO | MB_ICONWARNING) != IDYES)
            return 1;
    }

    if (Action & ACT_FORCE)  // force write ignores any request for dryrun
        cNinja.ForceWrite();

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
        {
            std::cout << cNinja.GetError(pos) << '\n';
        }
        std::cout << "\n\n";
    }

    if (countNinjas > 0)
    {
        std::cout << "Created " << countNinjas << " .ninja files" << '\n';
    }
    else
        std::cout << _tt("All ninja scripts are up to date.") << '\n';

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
}
