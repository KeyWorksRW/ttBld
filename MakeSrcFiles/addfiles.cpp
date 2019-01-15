/////////////////////////////////////////////////////////////////////////////
// Name:		addfiles.cpp
// Purpose:
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "../ttLib/include/ttlist.h"	// ttList, ttDblList, ttStrIntList
#include "../ttLib/include/ttfile.h"	// ttFile

#include "../common/csrcfiles.h"		// CSrcFiles

void AddFiles(ttList& lstFiles)
{
	if (lstFiles.GetCount() < 1) {
		puts("You didn't specify any files to add!");
		return;
	}

	CSrcFiles cSrcFiles;
	cSrcFiles.ReadFile(".srcfiles");

	for (size_t pos = 0; pos < lstFiles.GetCount(); ++pos) {
		if (cSrcFiles.m_lstSrcFiles.Find(lstFiles[pos])) {
			printf("%s already in .srcfiles\n", lstFiles[pos]);
			cSrcFiles.m_lstSrcFiles.Remove(pos);
		}
	}

	size_t cFilesAdded = 0;

	ttFile kfIn, kfOut;
	if (!kfIn.ReadFile(".srcfiles")) {
		puts("Cannot read .srcfiles!");
		return;
	}

	while (kfIn.readline() && !tt::samesubstri(kfIn, "Files:"))
		kfOut.WriteEol(kfIn);

	if (kfOut.IsEndOfFile()) {	// means there was no Files: section
		kfOut.WriteEol("Files:");
		for (size_t pos = 0; pos < lstFiles.GetCount(); ++pos) {
			if (!tt::FileExists(lstFiles[pos]))	{
				printf("%s doesn't exist -- not added\n", lstFiles[pos]);
				continue;
			}
			kfOut.printf("  %s\n", lstFiles[pos]);
			++cFilesAdded;
		}
		if (!kfOut.WriteFile(".srcfiles")) {
			puts("Cannot write to .srcfiles!");
			return;
		}

		// Use CStr to print the count to properly add the "s" to "file" as needed. I.e., 0 files, 1 file, 2 files -- 1
		// file is singular

		ttStr cszResults;
		cszResults.printf("%u file%ks added.", cFilesAdded, cFilesAdded);
		puts(cszResults);
		return;
	}

	kfOut.WriteEol(kfIn);	// This will be the Files: line
	while (kfIn.readline()) {
		if (tt::isalpha(*(const char*)kfIn))	// Alphabetical character in first column is a new section
			break;
		else if (!tt::nextnonspace(kfIn)) {		// blank line, insert after it
			kfOut.WriteEol(kfIn);
			break;
		}
		else
			kfOut.WriteEol(kfIn);
	}

	// We are now after a blank line, at the beginning of a section, or at the end of the file

	for (size_t pos = 0; pos < lstFiles.GetCount(); ++pos) {
		if (!tt::FileExists(lstFiles[pos]))	{
			printf("%s doesn't exist -- not added\n", lstFiles[pos]);
			continue;
		}
		kfOut.printf("  %s\n", lstFiles[pos]);
		++cFilesAdded;
	}

	// Now that the file(s) have been inserted, add everything else

	while (kfIn.readline())
		kfOut.WriteEol(kfIn);

	if (!kfOut.WriteFile(".srcfiles")) {
		puts("Cannot write to .srcfiles!");
		return;
	}

	// Use ttString to print the count to properly add the "s" to "file" as needed. I.e., 0 files, 1 file, 2 files

	ttStr cszResults;
	cszResults.printf("%u file%ks added.", cFilesAdded, cFilesAdded);
	puts(cszResults);
}
