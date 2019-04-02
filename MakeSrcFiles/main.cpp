/////////////////////////////////////////////////////////////////////////////
// Name:		main.cpp
// Purpose:		Entry point and Usage information
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <iostream>

#include <ttlist.h> 					// ttCList, ttDblList, ttCStrIntList
#include <ttfindfile.h>					// ttFindFile

#include "version.txt"	// Version (txtVersion) and Copyright (txtCopyRight) information

#include "funcs.h"
#include "convertdlg.h"	// CConvertDlg

void DisplayUsage()
{
	puts(txtVersion);
	puts(txtCopyRight);

	puts("\nMakeSrcFiles [option] [files]");
	puts("\twith no arguments, displays a dialog allowing you to modify .srcfiles in current directory");
	puts("\t-add filename(s) <-- will add the filename(s) to the Files: section of the current .srcfiles file");
	puts("\t-convert filename <-- will convert the build script (file.vcxproj, file.project, or file.cbp) into a .srcfiles file in current directory");
	puts("\t-new <-- creates a new .srcfiles file (unless one already exists)");
}

int main(int argc, char* argv[])
{
	tt::InitCaller(txtVersion);
	bool bDryRun = false;

	for (int argpos = 1; argpos < argc && (*argv[argpos] == '-' || *argv[argpos] == '/'); ++argpos) {
		if (argv[argpos][1] == '?') {
			DisplayUsage();
			return 1;
		}

		// The dryrun option isn't shown in Usage because it's really just for debugging -- i.e., you can make some
		// changes to the code then run MakeNinja -dryrun to make certain it is doing what you expect. It's not under
		// _DEBUG so you can test retail release.
		else if (tt::isSameStri(argv[argpos] + 1, "dryrun"))
			bDryRun = true;
		else if (tt::isSameStri(argv[argpos] + 1, "convert")) {
			CConvertDlg dlg;
			if (dlg.DoModal(NULL) == IDOK) {
			}
#if 0
			if (ConvertBuildScript(argpos + 1 > argc ? nullptr : argv[argpos + 1])) {
				// SetSrcFileOptions();
				return 1;
			}
			else
				return 0;
#endif
		}
		else if (tt::isSameStri(argv[argpos] + 1, "add")) {
			ttCList lstFiles;
			for (++argpos; argpos < argc; ++argpos) {
				lstFiles += argv[argpos];
			}
			AddFiles(lstFiles, bDryRun);
			return 1;
		}
		else if (tt::isSameStri(argv[argpos] + 1, "new")) {
			CreateNewSrcFiles();
			return 1;
		}
		else {
			printf("%s is an unknown option\n", argv[argpos]);
			return 0;
		}
	}
	if (tt::FileExists(".srcfiles")) {
		SetSrcFileOptions(bDryRun);
		return 1;
	}
	// if we get here, there's no .srcfiles and the user didn't tell us what to do. Start by looking for a build script
	// to convert. If that doesn't exist than create a new .srcfiles file.

	else {
		ttCFindFile ff("*.vcxproj");
		if (ff.isValid())
			return ConvertBuildScript(ff);
		else if (ff.NewPattern("*.vcproj"))
			return ConvertBuildScript(ff);
		else if (ff.NewPattern("*.project"))
			return ConvertBuildScript(ff);
		else if (ff.NewPattern("*.cbp"))
			return ConvertBuildScript(ff);
		else {
			CreateNewSrcFiles();
			return 1;
		}
	}
}
