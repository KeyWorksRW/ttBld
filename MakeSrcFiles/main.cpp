/////////////////////////////////////////////////////////////////////////////
// Name:		main.cpp
// Purpose:		Entry point and Usage information
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <iostream>
#include <direct.h>		// Functions for directory handling and creation

#ifdef _MSC_VER
	#pragma warning(disable: 6031)	// Return value ignored: '_chdir'.
#endif // _MSC_VER

#include <ttlist.h> 					// ttCList, ttDblList, ttCStrIntList
#include <ttfindfile.h>					// ttFindFile

#include "version.txt"	// Version (txtVersion) and Copyright (txtCopyRight) information

#include "funcs.h"
#include "convertdlg.h"	// CConvertDlg

void DisplayUsage()
{
	puts(txtVersion);
	puts(txtCopyRight);

	puts("\nMakeSrcFiles [option]");
	puts("\twith no arguments, displays a dialog allowing you to modify .srcfiles in current directory");
	puts("\t-add filename(s) <-- will add the filename(s) to the Files: section of the current .srcfiles file");
	puts("\t-new <-- displays a dialog allowing you to create a .srcfiles file");
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

		else if (tt::isSameStri(argv[argpos] + 1, "new")) {
			CConvertDlg dlg;
			if (dlg.CreateSrcFiles()) {
				ttCStr csz(dlg.GetDirOutput());
				char* pszTmp = csz.findStri(".srcfiles");
				if (pszTmp)
					*pszTmp = 0;
				_chdir(csz);
				if (tt::FileExists(".srcfiles")) {
					SetSrcFileOptions(bDryRun);
					return 1;
				}
			}
			else
				return 0;
		}
		else if (tt::isSameStri(argv[argpos] + 1, "add")) {
			ttCList lstFiles;
			for (++argpos; argpos < argc; ++argpos) {
				lstFiles += argv[argpos];
			}
			AddFiles(lstFiles, bDryRun);
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
}
