/////////////////////////////////////////////////////////////////////////////
// Name:		main.cpp
// Purpose:		Entry point and Usage information
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "../ttLib/include/ttlist.h"	// ttList, ttDblList, ttStrIntList
#include <iostream>

#include "version.txt"	// Version (txtVersion) and Copyright (txtCopyRight) information

#include "funcs.h"

void DisplayUsage()
{
	puts(txtVersion);
	puts(txtCopyRight);

	puts("\nMakeSrcFiles [option] [files]");
	puts("\twith no arguments, displays a dialog allowing you to modify .srcfiles in current directory");
	puts("\t-add filename(s) <-- will add the filename(s) to the Files: section of the current .srcfiles file");
	puts("\t-convert filename <-- will convert the build script (file.vcxproj, file.project, or file.cbp) into a .srcfiles file in current directory");
	puts("\t-overwrite <-- will read the current .srcfiles and completely overwrite it with any changes you make in the dialog");
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
		else if (tt::samestri(argv[argpos] + 1, "dryrun"))
			bDryRun = true;
		else if (tt::samestri(argv[argpos] + 1, "convert")) {
			if (ConvertBuildScript(argpos + 1 > argc ? nullptr : argv[argpos + 1])) {
				// SetSrcFileOptions();
				return 1;
			}
			else
				return 0;
		}
		else if (tt::samestri(argv[argpos] + 1, "add")) {
			ttList lstFiles;
			for (++argpos; argpos < argc; ++argpos) {
				lstFiles += argv[argpos];
			}
			AddFiles(lstFiles, bDryRun);
			return 1;
		}
		else if (tt::samestri(argv[argpos] + 1, "new")) {
			CreateNewSrcFiles();
			return 1;
		}
		else {
			printf("%s is an unknown option\n", argv[argpos]);
			return 0;
		}
	}
	SetSrcFileOptions(bDryRun);
	return 1;
}
