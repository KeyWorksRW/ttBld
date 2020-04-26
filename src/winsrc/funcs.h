/////////////////////////////////////////////////////////////////////////////
// Name:      funcs.h
// Purpose:   List of function declarations
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ttcvector.h>  // cstrVector -- Vector of ttlib::cstr strings

class cstr;
class CSrcFiles;

bool gitIgnoreAll(ttlib::cstr& GitExclude);
bool gitIsFileIgnored(ttlib::cstr& gitIgnorePath, std::string_view filename);
bool gitIsExcluded(ttlib::cstr& GitExclude, std::string_view filename);
bool gitAddtoIgnore(ttlib::cstr& GitIgnore, std::string_view filename);

// Successful return will have filled in ProjectFile with the path to the projectfile that
// was located.
bool ChangeOptions(std::string& ProjectFile);

bool ConvertBuildScript(const char* pszBldFile);

// Search PATH, LIB, or INCLUDE (or variants)
bool FindFileEnv(ttlib::cview Env, std::string_view filename, ttlib::cstr& pathResult);

void ParseDefines(ttlib::cstrVector& Results, std::string_view Defines);

// Try to locate .srcfiles.yaml and return a pointer to it's location if found.
//
// If pPath is specified, it will be assigned the path (including the .srcfiles.yaml
// filename)
const char* FindProjectFile(ttlib::cstr* pPath = nullptr);

// Creates .json files for Visual Studio
bool CreateVsJson(const char* pszSrcFiles, std::vector<std::string>& results);

// Following functions are for use in setting up a build system for VS Code

// Returns true unless unable to write to a file
std::vector<std::string> CreateVsCodeProject(std::string_view SrcFilename);
bool Yamalize();

#if defined(_WIN32)

// Determines if Visual Studio Code is installed
bool IsVsCodeAvail();

void CreateCodeCmd(const char* pszFile);
bool CreateMSVCEnvCmd(const char* pszDstFile, bool bDef64 = false);
bool FindCurMsvcPath(ttlib::cstr& Result);
bool FindVsCode(ttlib::cstr& Result);
// Returns true if we are running on an x64 processor
bool IsHost64();
bool JunctionToReal(const std::string& Dir, ttlib::cstr& Result);

#endif
