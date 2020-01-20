/////////////////////////////////////////////////////////////////////////////
// Name:      CSrcFiles
// Purpose:   Class for reading/writing .srcfiles.yaml (master file used by ttBld.exe to generate build scripts)
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>
#include <iostream>

#include <ttlist.h>   // ttCList, ttCDblList, ttCStrIntList
#include <ttfile.h>   // ttCFile
#include <ttarray.h>  // ttCArray
#include <ttmap.h>    // ttCMap

#include <ttstring.h>  // ttString, ttStrlist -- String and vector classes with some additional functionality

#include "options.h"  // CSrcOptions

using namespace sfopt;  // OPT_ options are used extensively, hence using the namespace in the header file

extern const char* txtSrcFilesFileName;

// Class for reading/writing .srcfiles.yaml (master file used by ttBld.exe to generate build scripts)
class CSrcFiles : public CSrcOptions
{
public:
    // If specified, pszNinjaDir is the directory to create .ninja scripts in
    CSrcFiles(const char* pszNinjaDir = nullptr);

    // Public functions
    const char* GetTargetDir();
    const char* GetTargetRelease();
    const char* GetTargetDebug();

    const char* GetBuildScriptDir();
    bool        AddFile(const char* pszFile) { return m_lstSrcFiles.addfile(pszFile); }

    // If pszFile is NULL, CSrcFiles will attempt to locate the file (see LocateSrcFiles()).
    bool ReadFile(std::string_view filename = "");

    bool IsProcessed() { return m_bRead; }

    bool IsExeTypeConsole() { return (ttStrStrI(GetOption(OPT_EXE_TYPE), "console")); }  // this is the default
    bool IsExeTypeDll()
    {
        return (ttStrStrI(GetOption(OPT_EXE_TYPE), "dll") || ttStrStrI(GetOption(OPT_EXE_TYPE), "ocx"));
    }
    bool IsExeTypeLib() { return (ttStrStrI(GetOption(OPT_EXE_TYPE), "lib")); }
    bool IsExeTypeWindow() { return (ttStrStrI(GetOption(OPT_EXE_TYPE), "window")); }

    bool IsStaticCrtRel() { return (ttStrStrI(GetOption(OPT_STATIC_CRT_REL), "static")); }
    bool IsStaticCrtDbg() { return (ttStrStrI(GetOption(OPT_STATIC_CRT_DBG), "static")); }

    bool IsOptimizeSpeed() { return (ttStrStrI(GetOption(OPT_OPTIMIZE), "speed")); }

    const char* GetBuildLibs() { return GetOption(OPT_BUILD_LIBS); }
    const char* GetXgetFlags() { return GetOption(OPT_XGET_FLAGS); }

    void AddSourcePattern(const char* pszFilePattern);

    // These are just for convenience--it's fine to call GetOption directly

    const char* GetProjectName() { return GetOption(OPT_PROJECT); }
    const char* GetPchHeader();
    // Source file to compile the precompiled header
    const char* GetPchCpp();

    std::string_view GetRcName() { return m_RCname; }

    // Gets name/location of srcfiles (normally .srcfiles.yaml)
    const char* GetSrcFiles() { return m_srcfilename.c_str(); };
    // Ninja's builddir should be set to this directory
    const char* GetBldDir() { return m_bldFolder.c_str(); }

    int GetMajorRequired() { return m_RequiredMajor; }
    int GetMinorRequired() { return m_RequiredMinor; }
    int GetSubRequired() { return m_RequiredSub; }

    ttStrVector& GetSrcFilesList() { return m_lstSrcFiles; }

    void SetReportingFile(const char* pszFile) { m_ReportPath = pszFile; }

#if !defined(NDEBUG)  // Starts debug section.
    void AddError(std::string_view err);
    #define BREAKONWARNING                                  \
        {                                                   \
            if (m_bBreakOnWarning && wxIsDebuggerRunning()) \
                wxTrap();                                   \
        }
#else
    void AddError(std::string_view err) { m_lstErrMessages.append(err); }
    #define BREAKONWARNING
#endif

protected:
    // Protected functions

    void ProcessFile(char* pszFile);
    void ProcessInclude(const char* pszFile, ttCStrIntList& lstAddSrcFiles, bool bFileSection);
    void ProcessLibSection(char* pszLibFile);
    void ProcessOption(char* pszLine);
    void ProcessTarget(char* pszLine);

    bool GetOptionParts(char* pszLine, ttCStr& cszName, ttCStr& cszVal, ttCStr& cszComment);

    void AddCompilerFlag(const char* pszFlag);
    //    void AddLibrary(const char* pszName);     // REVIEW: [KeyWorks - 8/7/2019] doesn't appear to be used

    const char* GetReportFilename() { return (m_ReportPath.empty()) ? "" : m_ReportPath.c_str(); }

protected:
    // Class members (note that these are NOT marked protected or private -- too many callers need to access
    // individual members)

    // REVIEW: [randalphwa - 7/6/2019] Having these public: is a bad design. We should replace them with Get/Set
    // functions

    ttString m_LIBname;  // Name and location of any additional library to build (used by Lib: section)
    ttString m_RCname;   // Resource file to build (if any)
    ttString m_HPPname;  // HTML Help project file

    ttCHeap m_ttHeap;  // All the ttCList files will be attatched to this heap

    ttStrVector m_lstSrcFiles;  // List of all source files
    ttStrVector m_lstLibFiles;  // List of any files used to build additional library
    ttStrVector m_lstIdlFiles;  // List of any idl files to compile with midl compiler

    ttStrVector m_lstErrMessages;  // List of any errors that occurred during processing

    ttCStrIntList m_lstAddSrcFiles;     // Additional .srcfiles.yaml to read into Files: section
    ttCStrIntList m_lstLibAddSrcFiles;  // Additional .srcfiles.yaml to read into Lib: section
    ttCList m_lstSrcIncluded;  // The names of all files included by all ".include path/.srcfiles.yaml" directives

    ttString m_pchCPPname;

private:
    // Class members

    ttString m_srcfilename;
    ttString m_ReportPath;  // Path to use when reporting a problem.
    ttString m_bldFolder;   // This is where we write the .ninja files, and is ninja's builddir.

    ttString m_relTargetFolder;
    ttString m_dbgTargetFolder;

    std::string m_strTargetDir;

    int m_RequiredMajor;  // These three get filled in to the minimum ttBld version required to process.
    int m_RequiredMinor;
    int m_RequiredSub;

    bool m_bRead;            // File has been read and processed.
    bool m_bBreakOnWarning;  // Used in debug builds to call wxTrap().
};
