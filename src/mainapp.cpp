/////////////////////////////////////////////////////////////////////////////
// Name:      mainapp.cpp
// Purpose:   entry point, global strings, library pragmas
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

const char* txtVersion   = "MakeNinja 1.1.0.8295";
const char* txtCopyRight = "Copyright (c) 2002-2019 KeyWorks Software";

#include <iostream>
#include <direct.h>     // Functions for directory handling and creation

// _LINK_ commands combined with wxLibs.h is used to specify what wxWidgets libraries you want to link to. It is not
// tracked so that you can change these to whatever you want. If you copy the file from ../wxMSW/wxLibs.h you can then
// modify it to whatever you want (for MSW builds you may not need to change it at all).

#define _LINK_WX_BASE 1
#define _LINK_WX_CORE 1

#include "wxLibs.h"     // adds pragmas telling the linker which libraries to link to

#include "convertdlg.h" // CConvertDlg
#include "ninja.h"      // CNinja
#include "vcxproj.h"    // CVcxProj

void Usage()
{
    puts("");
    puts(txtVersion);
    puts(txtCopyRight);

    puts(TRANSLATE("\nMakeNinja [options] -- parses .srcfiles and produces ninja build scripts\n"));
    puts(TRANSLATE("    -options   -- displays a dialog allowing you to change options in a .srcfiles file"));
    puts(TRANSLATE("    -dryrun    -- displays what would have happened, but doesn't change anything"));
    puts(TRANSLATE("    -new       -- displays a dialog allowing you to create a new .srcfiles file"));
    puts(TRANSLATE("    -vscode    -- creates or updates files needed to build project using VS Code"));

    puts("\nIDE workspace options:");
    puts(TRANSLATE("    -codelite   -- creates or updates files needed to build project using CodeLite"));
    puts(TRANSLATE("    -codeblocks -- creates or updates files needed to build project using CodeBlocks"));
    puts(TRANSLATE("    -vcxproj    -- creates or updates files needed to build project using MS Visual Studio"));

    // disabled until we decide if we really want to support it
    // puts("\t-add [file(s)] -- Adds file(s) to .srcfiles");
}

int main(int argc, char* argv[])
{
    ttInitCaller(txtVersion);
    bool bDryRun = false;
    bool bVsCodeDir = false;

    const char* pszEnv = getenv("MAKENINJA");
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
                    bVsCodeDir = true;

            }
            pszArg = ttStepOver(pszArg);
        }
    }

    ttCStr cszSrcFilePath(bVsCodeDir ? ".vscode/srcfiles.yaml" :  ".srcfiles");   // ChangeOptions will set this to the .srcfiles that got modified

    for (int argpos = 1; argpos < argc && (*argv[argpos] == '-' || *argv[argpos] == '/'); ++argpos)
    {
        if (argv[argpos][1] == '?' || ttIsSameSubStrI(argv[argpos] + 1, "help"))
        {
            Usage();
            return 1;
        }
        else if (ttIsSameSubStrI(argv[argpos] + 1, "dry"))  // -dryryn
            bDryRun = true;
        else if (ttIsSameStrI(argv[argpos] + 1, "new"))
        {
            CConvertDlg dlg;

            if (argpos + 1 < argc)
            {
                ++argpos;
                if (!dlg.doConversion(argv[argpos]))
                {
                    ttMsgBoxFmt(TRANSLATE("Conversion of %s failed!"), MB_OK | MB_ICONWARNING, dlg.GetConvertScript());
                    if (!dlg.CreateSrcFiles())
                        return 0;   // means .srcfiles wasn't created, so nothing further that we can do
                }
            }
            else if (!dlg.CreateSrcFiles())
                return 0;   // means .srcfiles wasn't created, so nothing further that we can do

            ttCStr csz(dlg.GetDirOutput());
            char* pszTmp = csz.FindStrI(".srcfiles");
            if (pszTmp)
                *pszTmp = 0;
            ttChDir(csz);
            if (!ChangeOptions(&cszSrcFilePath, bDryRun))
                return 1;
        }
        else if (ttIsSameStrI(argv[argpos] + 1, "add"))
        {
            ttCList lstFiles;
            for (++argpos; argpos < argc; ++argpos)
                lstFiles += argv[argpos];
            AddFiles(lstFiles, bDryRun);
            return 1;
        }
        else if (ttIsSameSubStrI(argv[argpos] + 1, "opt"))    // -options
        {
            if (!ChangeOptions(&cszSrcFilePath, bDryRun))
                return 1;
        }
        else if (ttIsSameSubStrI(argv[argpos] + 1, "vscode"))
        {
            cszSrcFilePath = ".vscode/srcfiles.yaml";
            bVsCodeDir = true;
        }
        else if (ttIsSameSubStrI(argv[argpos] + 1, "novscode"))  // used in case it was set in environment
        {
            cszSrcFilePath = ".srcfiles";
            bVsCodeDir = false;
        }
    }

    if (bVsCodeDir)
    {
        ttCList lstResults;
        CreateVsCodeProject(&lstResults);
        for (size_t pos = 0; lstResults.InRange(pos); ++pos)
            puts(lstResults[pos]);
    }

    if (!ttFileExists(cszSrcFilePath))
    {
        CConvertDlg dlg;
        if (!dlg.CreateSrcFiles())
            return 0;   // means .srcfiles wasn't created, so nothing further that we can do
        ttCStr csz(dlg.GetDirOutput());
        char* pszTmp = csz.FindStrI(".srcfiles");
        if (pszTmp)
            *pszTmp = 0;
        ttChDir(csz);
        if (!ChangeOptions(&cszSrcFilePath, bDryRun))
            return 1;
    }

    {
        CNinja cNinja(bVsCodeDir);
        if (!cNinja.IsValidVersion())
        {
            if (ttMsgBox(TRANSLATE("This version of MakeNinja is too old -- create ninja scripts anyway?"), MB_YESNO | MB_ICONWARNING) != IDYES)
                return 1;
        }

        if (bDryRun)
            cNinja.EnableDryRun();

        cNinja.CreateMakeFile();    // this will create/update it if .srcfiles has a Makefile: section

        ttASSERT_MSG(cNinja.GetBoolOption(OPT_64BIT) || cNinja.GetBoolOption(OPT_32BIT), "At least one platform build should have been set in CNinja constructor")

        int countNinjas = 0;
        if (cNinja.GetBoolOption(OPT_64BIT))
        {
            if (cNinja.IsCompilerMSVC())
            {
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG64, false))
                    countNinjas++;
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE64, false))
                    countNinjas++;
            }
            if (cNinja.IsCompilerClang())
            {
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG64, true))
                    countNinjas++;
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE64, true))
                    countNinjas++;
            }
        }
        if (cNinja.GetBoolOption(OPT_32BIT))
        {
            if (cNinja.IsCompilerMSVC())
            {
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG32, false))
                    countNinjas++;
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE32, false))
                    countNinjas++;
            }
            if (cNinja.IsCompilerClang())
            {
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG32, true))
                    countNinjas++;
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE32, true))
                    countNinjas++;
            }
        }

        if (ttIsNonEmpty(cNinja.GetHHPName()))
            cNinja.CreateHelpFile();

        // Display any errors that occurred during processing

        if (cNinja.GetErrorCount())
        {
            for (size_t pos = 0; pos < cNinja.GetErrorCount(); pos++)
                puts(cNinja.GetError(pos));
            puts("");
        }

        if (countNinjas > 0)
            printf(TRANSLATE("Created %d .ninja files\n"), countNinjas);
        else
            puts(GETSTRING(IDS_NINJA_UP_TO_DATE));
    }

    return 0;
}
