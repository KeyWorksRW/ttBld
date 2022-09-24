/////////////////////////////////////////////////////////////////////////////
// Name:      funcs.h
// Purpose:   List of function declarations
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

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
bool FindFileEnv(const std::string& Env, std::string_view filename, ttlib::cstr& pathResult);

void ParseDefines(std::vector<ttlib::cstr>& Results, std::string_view Defines);

// Creates .json files for Visual Studio
bool CreateVsJson(const char* pszSrcFiles, std::vector<std::string>& results);

// Following functions are for use in setting up a build system for VS Code

// Returns true unless unable to write to a file
std::vector<ttlib::cstr> CreateVsCodeProject(std::string_view SrcFilename);
bool Yamalize();

#if defined(_WIN32)

// Determines if Visual Studio Code is installed
bool IsVsCodeAvail();

void CreateCodeCmd(const char* pszFile);
bool CreateMSVCEnvCmd(const char* pszDstFile, bool bDef64 = false);
bool FindCurMsvcPath(ttlib::cstr& Result);
bool FindVsCode(ttlib::cstr& Result);

// Called when ttBld is run and there is no project, or -new is specified.
bool MakeNewProject(ttlib::cstr& projectFile);

// Returns true if we are running on an x64 processor
bool IsHost64();

#endif
