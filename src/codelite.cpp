/////////////////////////////////////////////////////////////////////////////
// Name:		codelite.cpp
// Purpose:		Creates a CodeLite project file
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "../ttLib/include/ttfile.h"	// ttFile

#include "../common/csrcfiles.h"		// CSrcFiles

#include "resource.h"

/*

	First load pre.project, change %projname% to the project name, then add:

    <File Name="%s"/>

	for every file in .srcfiles (replace %s with the actual filename). Also include the precompiled header, and .srcfiles

	Finally add

	</VirtualDirectory>

	Next, read post.project. Replace %exepath% with the full path and filename of the debug executable (to run using F5)
	Replace %cwd% with the directory of the project

*/

enum {
	CLP_CREATED,
	CLP_EXISTS,
	CLP_NO_SRCFILES,
	CLP_NO_PROJECT,
	CLP_MISSING_RES,
	CLP_CANT_WRITE,
};

size_t CreateCodeLiteProject()
{
	CSrcFiles cSrcFiles;
	if (!cSrcFiles.ReadFile()) {
		puts("Cannot create a CodeLite project file if there is no .srcfiles file!");
		return CLP_NO_SRCFILES;
	}

	if (!cSrcFiles.GetProjectName()) {
		puts("Cannot create a CodeLite project file if .srcfiles doesn't specifiy the name of the project!");
		return CLP_NO_PROJECT;
	}

	ttString cszProjFile(cSrcFiles.GetProjectName());
	cszProjFile.ChangeExtension(".project");
	if (tt::FileExists(cszProjFile)) {
		// TODO: [randalphwa - 10/8/2018] We could call a function here to update the .project file, adding any src files that
		// are in .srcfiles, but missing in .project
		return CLP_EXISTS;
	}

	ttFile kf;
	if (!kf.ReadResource(IDR_PRE_PROJECT)) {
		puts("MakeNinja.exe is corrupted -- cannot read the necessary resource");
		return CLP_MISSING_RES;
	}

	while (kf.ReplaceStr("%projname%", cSrcFiles.GetProjectName()));

	kf.WriteEol("\t\t<File Name=\042.srcfiles\042/>");
	if (cSrcFiles.m_cszPCHheader.IsNonEmpty())
		kf.printf("\t\t<File Name=\042%s\042/>\n", (char*) cSrcFiles.m_cszPCHheader);

	cSrcFiles.m_lstSrcFiles.Sort();
	cSrcFiles.m_lstSrcFiles.BeginEnum();

	const char* pszFileName;
	while (cSrcFiles.m_lstSrcFiles.Enum(&pszFileName))
		kf.printf("\t\t<File Name=\042%s\042/>\n", pszFileName);

	kf.WriteEol("\t</VirtualDirectory>");

	ttFile kfPost;
	if (!kfPost.ReadResource(IDR_POST_PROJECT)) {
		puts("MakeNinja.exe is corrupted -- cannot read the necessary resource");
		return CLP_MISSING_RES;
	}

	ttString cszCWD;
	cszCWD.GetCWD();

	while (kfPost.ReplaceStr("%cwd%", cszCWD));
	ttString cszExe(cszCWD);
	cszExe.AppendFileName("../bin/");
	cszExe += (char*) cSrcFiles.m_cszProjectName;
	cszExe += "D.exe";
	cszExe.GetFullPathName();
	tt::BackslashToForwardslash(cszExe);
	while (kfPost.ReplaceStr("%exepath%", cszExe));

	while (kfPost.readline())
		kf.WriteEol(kfPost);

	if (!kf.WriteFile(cszProjFile)) {
		printf("Unable to write to %s!\n", (char*) cszProjFile);
		return CLP_CANT_WRITE;
	}
	else {
		printf("created %s\n", (char*) cszProjFile);
	}

	return CLP_CREATED;
}
