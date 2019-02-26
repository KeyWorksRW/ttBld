/////////////////////////////////////////////////////////////////////////////
// Name:		init.cpp
// Purpose:		initialization
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "version.txt"	// Version (txtVersion) and Copyright (txtCopyRight) information

void Usage()
{
	puts(txtVersion);
	puts(txtCopyRight);

	puts("\nMakeNinja [options] -- parses .srcfiles and produces ninja build scripts\n");
	// puts("\t-add [file(s)] -- Adds file(s) to .srcfiles");
	puts("\t-dryrun -- displays what would have happened, but doesn't change anything");
	puts("\t-force -- writes new ninja build scripts even if nothing changed");
	puts("\t-noprivate -- do not look for .private/.srcfiles");
}

int main(int argc, char* argv[])
{
	tt::InitCaller(txtVersion);

	if (!tt::FileExists(".srcfiles")) {
		puts("Cannot run without .srcfiles in current folder. Run MakeSrcFiles.exe to create it.");
		return 1;
	}

	if (argc > 1 && (tt::isSameStri(argv[1], "-?") || tt::isSameStri(argv[1], "/?"))) {
		Usage();
		return 1;
	}

	return MakeNinja(argc, argv);
}
