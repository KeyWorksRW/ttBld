/////////////////////////////////////////////////////////////////////////////
// Name:		makeninja.cpp
// Purpose:		ninja build script generator
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

// Given a .srcfiles file in the current directory, this tool creates Ninja build script files.

#include "pch.h"

#include <ttmem.h>		// ttCMem, ttCTMem

#include "ninja.h"		// CNinja
#include "vcxproj.h"	// CVcxProj

int MakeNinja(int argc, char* argv[])
{
	bool bReadPrivate = true;	// read .private/.srcfiles in addition to .srcfiles
	CNinja cNinja;

	if (argc > 1) {
		for (int argpos = 1; argpos < argc; argpos++) {
			if (argv[argpos][0] == '-' || argv[argpos][0] == '/') {
				if (tt::isSameSubStri(argv[argpos] + 1, "noprivate")) {
					bReadPrivate = false;
				}

				// This option isn't shown in Usage because it's really just for debugging -- i.e., you can make some
				// changes to the code then run MakeNinja -dryrun to make certain it is doing what you expect. It's not
				// under _DEBUG so you can test retail release.

				else if (tt::isSameSubStri(argv[argpos] + 1, "dryrun")) {
					cNinja.EnableDryRun();
				}
				else if (tt::isSameSubStri(argv[argpos] + 1, "force")) {
					cNinja.ForceOutput();
				}
				else {
					printf("%s is an unknown option.\n", argv[argpos]);
					return 1;
				}
			}
		}
	}

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

	if (tt::isNonEmpty(cNinja.getHHPName()))
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

	if (cNinja.isCodeLiteIDE())
		CreateCodeLiteProject();

	if (cNinja.isVisualStudioIDE())	{
		CVcxProj vcxProj;
		vcxProj.CreateBuildFile();
	}

	return 0;
}

bool isSystemHeaderFile(const char* pszHeaderFile)
{
	ttCTMem<char*> pszEnvironment(4096);
	GetEnvironmentVariable("INCLUDE", pszEnvironment, 4094);	// leave room for appending a ';' character and NULL
	tt::strCat(pszEnvironment, ";");
	char* pszSemi = tt::findChar(pszEnvironment, ';');
	char* pszPath = pszEnvironment;
	while (pszSemi) {
		*pszSemi = 0;
		ttCStr cszFile(pszPath);
		cszFile.AddTrailingSlash();
		cszFile += pszHeaderFile;
		if (tt::FileExists(cszFile))
			return true;

		pszPath = pszSemi + 1;
		pszSemi = tt::findChar(pszPath, ';');
	}
	return false;
}
