/////////////////////////////////////////////////////////////////////////////
// Name:      funcs.h
// Purpose:   List of function declarations
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

class ttCList;  // forward definition
class ttCStr;
class CSrcFiles;

void ParseDefines(ttCList& lst, const char* pszDefines);
void AddFiles(ttCList& lstFiles, bool bDryRun);
bool ChangeOptions(ttCStr* pcszSrcFiles, bool bDryRun);
bool ConvertBuildScript(const char* pszBldFile);
int  MakeNinja(int argc, char* argv[]);
bool isSystemHeaderFile(const char* pszHeaderFile);

size_t CreateCodeLiteProject();    // returns 0 - no errors, 1 - file already exists, 2 - other error

// Following functions are for use in setting up a build system for VS Code

bool CreateVsCodeProject(ttCList* plstResults = nullptr);      // returns true unless unable to write to a file
bool CreateMSVCEnvCmd();
bool Yamalize();
