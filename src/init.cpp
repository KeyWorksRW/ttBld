/////////////////////////////////////////////////////////////////////////////
// Name:		init.cpp
// Purpose:		initialization
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2018 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#include "version.txt"	// Version (txtVersion) and Copyright (txtCopyRight) information

void Usage()
{
	puts(txtVersion);
	puts(txtCopyRight);

	puts("\nMakeNinja\n\tparses .srcfiles and produces ninja build scripts\n");
	puts("MakeNinja file(s)\n\tAdds file(s) to .srcfiles");
}

int main(int argc, char* argv[])
{
	InitCaller(NULL, NULL, txtVersion);

	if (!FileExists(".srcfiles")) {
		puts("Cannot run without .srcfiles in current folder. Run MakeSrcFiles.exe to create it.");
		return 1;
	}

	if (argc > 1 && (IsSameString(argv[1], "-?") || IsSameString(argv[1], "/?"))) {
		Usage();
		return 1;
	}

	return MakeNinja(argc, argv);
}
