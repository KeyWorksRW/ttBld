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
class ttString;
class CSrcFiles;

bool gitIgnoreAll(ttCStr& cszGitExclude);
bool gitAddtoIgnore(ttCStr& cszGitIgnore, const char* pszFile);

bool gitIsExcluded(ttCStr& cszGitExclude, const char* pszFile);
bool gitIsFileIgnored(ttCStr& cszGitIgnore, const char* pszFile);

void AddFiles(ttCList& lstFiles, bool bDryRun);

// Successful return will have filled in ProjectFile with the path to the projectfile that
// was located.
bool ChangeOptions(std::string& ProjectFile, bool bDryRun = false);

bool ConvertBuildScript(const char* pszBldFile);

// Search PATH, LIB, or INCLUDE (or variants)
bool FindFileEnv(const char* pexEnv, const char* pszFile, ttCStr* pcszPath = nullptr);
int  MakeNinja(int argc, char* argv[]);
void ParseDefines(ttCList& lst, const char* pszDefines);
bool isSystemHeaderFile(const char* pszHeaderFile);

// If pcszStartDir is used, it will be set to the path to the file or dir/file where .srcfiles.yaml was found
const char* LocateSrcFiles(ttCStr* pcszStartDir = nullptr);

// Attempts to locate .srcfiles.yaml
// std::unique_ptr<ttString> locateProjectFile(std::string_view StartDir = ttEmptyString);

// Try to locate .srcfiles.yaml and return a pointer to it's location if found.
//
// If pPath is specified, it will be assigned the path (including the .srcfiles.yaml
// filename)
const char* FindProjectFile(ttString* pPath = nullptr);

// Returns 0 - no errors, 1 - file already exists, 2 - other error
size_t CreateCodeLiteProject(const char* pszSrcFiles = nullptr, ttCList* plstResults = nullptr);

// Creates .json files for Visual Studio
bool CreateVsJson(const char* pszSrcFiles, std::vector<std::string>& results);

// Following functions are for use in setting up a build system for VS Code

// Returns true unless unable to write to a file
bool CreateVsCodeProject(const char* pszSrcFiles = nullptr, ttCList* plstResults = nullptr);
bool Yamalize();

#if defined(_WIN32)

// Determines if Visual Studio Code is installed
bool IsVsCodeAvail();

void CreateCodeCmd(const char* pszFile);
bool CreateMSVCEnvCmd(const char* pszDstFile, bool bDef64 = false);
bool FindCurMsvcPath(ttCStr& cszPath);
bool FindVsCode(ttCStr& cszPath);
// Returns true if we are running on an x64 processor
bool IsHost64();
bool JunctionToReal(const char* pszDir, ttCStr& cszDir);

#endif
