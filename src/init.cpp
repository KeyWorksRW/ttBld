/////////////////////////////////////////////////////////////////////////////
// Name:		init.cpp
// Purpose:		initialization
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <iostream>
#include <direct.h>		// Functions for directory handling and creation

#ifdef _MSC_VER
	#pragma warning(disable: 6031)	// Return value ignored: '_chdir'.
#endif // _MSC_VER

#include "convertdlg.h" 	// CConvertDlg
#include "ninja.h"			// CNinja
#include "vcxproj.h"		// CVcxProj

#include "version.txt"	// Version (txtVersion) and Copyright (txtCopyRight) information

void Usage()
{
	puts(txtVersion);
	puts(txtCopyRight);

	puts("\nMakeNinja [options] -- parses .srcfiles and produces ninja build scripts\n");
	// puts("\t-add [file(s)] -- Adds file(s) to .srcfiles");
	puts("\t-dryrun -- displays what would have happened, but doesn't change anything");
	puts("\t-force -- writes new ninja build scripts even if nothing changed");
	puts("\t-new -- displays a dialog allowing you to create a .srcfiles file");
	puts("\t-noprivate -- do not look for .private/.srcfiles");
	puts("\t-options -- displays a dialog allowing you to change options in a .srcfiles file");
}

int main(int argc, char* argv[])
{
	tt::InitCaller(txtVersion);
	bool bDryRun = false;
	bool bForce = false;
	bool bReadPrivate = true;	// read .private/.srcfiles in addition to .srcfiles

	for (int argpos = 1; argpos < argc && (*argv[argpos] == '-' || *argv[argpos] == '/'); ++argpos) {
		if (argv[argpos][1] == '?') {
			Usage();
			return 1;
		}
		else if (tt::IsSameSubStrI(argv[argpos] + 1, "dry"))
			bDryRun = true;
		else if (tt::IsSameStrI(argv[argpos] + 1, "new")) {
			CConvertDlg dlg;
			if (!dlg.CreateSrcFiles())
				return 0;	// means .srcfiles wasn't created, so nothing further that we can do

			ttCStr csz(dlg.GetDirOutput());
			char* pszTmp = csz.FindStrI(".srcfiles");
			if (pszTmp)
				*pszTmp = 0;
			_chdir(csz);
			if (!ChangeOptions(bDryRun))
				return 1;
		}
		else if (tt::IsSameStrI(argv[argpos] + 1, "add")) {
			ttCList lstFiles;
			for (++argpos; argpos < argc; ++argpos) {
				lstFiles += argv[argpos];
			}
			AddFiles(lstFiles, bDryRun);
			return 1;
		}
		else if (tt::IsSameSubStrI(argv[argpos] + 1, "opt")) {
			if (!ChangeOptions(bDryRun))
				return 1;
		}
		else if (tt::IsSameSubStrI(argv[argpos] + 1, "nop")) {
			bReadPrivate = false;
		}
		else if (tt::IsSameSubStrI(argv[argpos] + 1, "force")) {
			bForce = false;
		}
	}

	if (!tt::FileExists(".srcfiles")) {
		CConvertDlg dlg;
		if (!dlg.CreateSrcFiles())
			return 0;	// means .srcfiles wasn't created, so nothing further that we can do
		ttCStr csz(dlg.GetDirOutput());
		char* pszTmp = csz.FindStrI(".srcfiles");
		if (pszTmp)
			*pszTmp = 0;
		_chdir(csz);
		if (!ChangeOptions(bDryRun))
			return 1;
	}

	{
		CNinja cNinja;
		if (bForce)
			cNinja.ForceOutput();
		if (bDryRun)
			cNinja.EnableDryRun();

		cNinja.CreateMakeFile();	// this will create/update it if .srcfiles has a Makefile: section

		ttASSERT_MSG(cNinja.GetBoolOption(OPT_64BIT) || cNinja.GetBoolOption(OPT_32BIT), "At least one platform build should have been set in CBldMaster (CNinja) constructor")

		int countNinjas = 0;
		if (cNinja.GetOption(OPT_64BIT)) {
			if (cNinja.IsCompilerMSVC())	{
				if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG64, false))
					countNinjas++;
				if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE64, false))
					countNinjas++;
			}
			if (cNinja.IsCompilerClang())	{
				if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG64, true))
					countNinjas++;
				if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE64, true))
					countNinjas++;
			}
		}
		if (cNinja.GetOption(OPT_32BIT)) {
			if (cNinja.IsCompilerMSVC())	{
				if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG32, false))
					countNinjas++;
				if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE32, false))
					countNinjas++;
			}
			if (cNinja.IsCompilerClang())	{
				if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG32, true))
					countNinjas++;
				if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE32, true))
					countNinjas++;
			}
		}

		if (tt::IsNonEmpty(cNinja.GetHHPName()))
			cNinja.CreateHelpFile();

		// Display any errors that occurred during processing

		if (cNinja.GetErrorCount()) {
			for (size_t pos = 0; pos < cNinja.GetErrorCount(); pos++) {
				puts(cNinja.GetError(pos));
			}
			puts("");
		}

		if (countNinjas > 0)
			printf("Created %d .ninja files\n", countNinjas);
		else
			puts("All ninja scripts are up to date.");

		if (cNinja.IsCodeLiteIDE())
			CreateCodeLiteProject();

		if (cNinja.IsVisualStudioIDE())	{
			CVcxProj vcxProj;
			vcxProj.CreateBuildFile();
		}
	}

	return 0;
}
