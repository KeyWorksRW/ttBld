/////////////////////////////////////////////////////////////////////////////
// Name:		makeninja.cpp
// Purpose:		ninja build script generator
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2018 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

// Given a .srcfiles file in the current directory, this tool creates Ninja build script files.

#include "precomp.h"

#include "bldmaster.h"	// CBldMaster
#include "ninja.h"		// CNinja

int MakeNinja(int argc, char* argv[])
{
	bool bReadPrivate = true;	// read .private/.srcfiles in addition to .srcfiles

	if (argc > 1) {
		for (int argpos = 1; argpos < argc; argpos++) {
			if (argv[argpos][0] == '-' || argv[argpos][0] == '/') {
				if (tolower(argv[argpos][1]) == '?') {
					puts(txtVersion);
					puts(txtCopyRight);
					puts("\nUsage: MakeNinja [-np]");
					puts("\tprocesses .srcfiles in the current directory, creates ninja scripts in build/ subdirectory");
					puts("\t-np ignore any .private/.srcfiles (default is this file overrides anything specified in the master .srcfiles)");
					return 1;
				}
				else if (IsSameSubString(argv[argpos] + 1, "np")) {
					bReadPrivate = false;
				}
				else {
					printf("%s is an unknown option.\n", argv[argpos]);
					return 1;
				}
			}
		}
	}

	CNinja cNinja;
	cNinja.CreateMakeFile();	// this will create/update it if .srcfiles has a Makefile: section

	int countNinjas = 0;
	if (!cNinja.is64BitBuild() || cNinja.is64BitSuffix()) {	// if not 64-bit only
		if (cNinja.isCompilerMSVC())	{
			if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CSrcFiles::COMPILER_MSVC))
				countNinjas++;
			if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CSrcFiles::COMPILER_MSVC))
				countNinjas++;
		}
		if (cNinja.isCompilerClang())	{
			if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CSrcFiles::COMPILER_CLANG))
				countNinjas++;
			if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CSrcFiles::COMPILER_CLANG))
				countNinjas++;
		}
	}
	if (cNinja.is64BitBuild()) {
		if (cNinja.isCompilerMSVC())	{
			if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG64, CSrcFiles::COMPILER_MSVC))
				countNinjas++;
			if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE64, CSrcFiles::COMPILER_MSVC))
				countNinjas++;
		}
		if (cNinja.isCompilerClang())	{
			if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG64, CSrcFiles::COMPILER_CLANG))
				countNinjas++;
			if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE64, CSrcFiles::COMPILER_CLANG))
				countNinjas++;
		}
	}

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

	if (cNinja.isCodeLiteIDE()) {
		CreateCodeLiteProject();
	}

	return 0;
}

bool isSystemHeaderFile(const char* pszHeaderFile)
{
	CTMem<char*> pszEnvironment(4096);
	GetEnvironmentVariable("INCLUDE", pszEnvironment, 4094);	// leave room for appending a ';' character and NULL
	kstrcat(pszEnvironment, ";");
	char* pszSemi = kstrchr(pszEnvironment, ';');
	char* pszPath = pszEnvironment;
	while (pszSemi) {
		*pszSemi = 0;
		CStr cszFile(pszPath);
		cszFile.AddTrailingSlash();
		cszFile += pszHeaderFile;
		if (FileExists(cszFile))
			return true;

		pszPath = pszSemi + 1;
		pszSemi = kstrchr(pszPath, ';');
	}
	return false;
}
