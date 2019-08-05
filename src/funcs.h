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

void AddFiles(ttCList& lstFiles, bool bDryRun);
bool ChangeOptions(ttCStr* pcszSrcFiles, bool bDryRun = false);
bool ConvertBuildScript(const char* pszBldFile);
bool FindFileEnv(const char* pexEnv, const char* pszFile, ttCStr* pcszPath = nullptr);     // Search PATH, LIB, or INCLUDE (or variants)
int  MakeNinja(int argc, char* argv[]);
void ParseDefines(ttCList& lst, const char* pszDefines);
bool isSystemHeaderFile(const char* pszHeaderFile);

const char* LocateSrcFiles(ttCStr* pcszStartDir = nullptr);   // If pcszStartDir is used, it will be set to the path to the file or dir/file where .srcfiles.yaml was found

size_t CreateCodeLiteProject(const char* pszSrcFiles = nullptr, ttCList* plstResults = nullptr);    // returns 0 - no errors, 1 - file already exists, 2 - other error

bool CreateVsJson(const char* pszSrcFiles = nullptr, ttCList* plstResults = nullptr);   // creates .json files for Visual Studio

// Following functions are for use in setting up a build system for VS Code

bool CreateVsCodeProject(const char* pszSrcFiles = nullptr, ttCList* plstResults = nullptr);      // returns true unless unable to write to a file
bool Yamalize();

#if defined(_WIN32)

void CreateCodeCmd(const char* pszFile);
bool CreateMSVCEnvCmd(const char* pszDstFile, bool bDef64 = false);
bool FindCurMsvcPath(ttCStr& cszPath);
bool FindVsCode(ttCStr& cszPath);
bool IsHost64();    // returns true if we are running on an x64 processor

#endif
