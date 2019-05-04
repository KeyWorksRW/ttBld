/////////////////////////////////////////////////////////////////////////////
// Name:		funcs.h
// Purpose:		List of function declarations
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

class ttCList;	// forward definition

void	AddFiles(ttCList& lstFiles, bool bDryRun);
bool	ChangeOptions(bool bDryRun);
bool	ConvertBuildScript(const char* pszBldFile);
size_t	CreateCodeLiteProject();	// returns 0 - no errors, 1 - file already exists, 2 - other error
int 	MakeNinja(int argc, char* argv[]);
bool	isSystemHeaderFile(const char* pszHeaderFile);
size_t	CreateCodeLiteProject();	// returns 0 - no errors, 1 - file already exists, 2 - other error
