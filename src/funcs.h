/////////////////////////////////////////////////////////////////////////////
// Name:		funcs.h
// Purpose:		List of function declarations
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2018 KeyWorks Software (Ralph Walden)
// License:     Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

bool	ConvertBuildScript(const char* pszBldFile);
int		MakeNinja(int argc, char* argv[]);
bool	isSystemHeaderFile(const char* pszHeaderFile);

size_t CreateCodeLiteProject();	// returns 0 - no errors, 1 - file already exists, 2 - other error
