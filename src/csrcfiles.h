/////////////////////////////////////////////////////////////////////////////
// Name:      CSrcFiles
// Purpose:   Class for reading/writing .srcfiles.yaml (master file used by ttBld.exe to generate build scripts)
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

#include <ttlist.h>   // ttCList, ttCDblList, ttCStrIntList
#include <ttstr.h>    // ttStr, ttCWD
#include <ttfile.h>   // ttCFile
#include <ttarray.h>  // ttCArray
#include <ttmap.h>    // ttCMap

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
    void        AddFile(const char* pszFile) { m_lstSrcFiles += pszFile; }

    // If pszFile is NULL, CSrcFiles will attempt to locate the file (see LocateSrcFiles()).
    bool ReadFile(const char* pszFile = nullptr);

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

    // Gets name/location of srcfiles (normally .srcfiles.yaml)
    const char* GetSrcFiles() { return m_cszSrcFilePath; };
    // Ninja's builddir should be set to this directory
    const char* GetBldDir() { return m_cszBldDir; }

    int GetMajorRequired() { return m_RequiredMajor; }
    int GetMinorRequired() { return m_RequiredMinor; }
    int GetSubRequired() { return m_RequiredSub; }

    ttCList* GetSrcFilesList() { return &m_lstSrcFiles; }

    void SetReportingFile(const char* pszFile) { m_cszReportPath = pszFile; }

#if !defined(NDEBUG)  // Starts debug section.
    void AddError(const char* pszErrMsg);
    #define BREAKONWARNING         \
        {                          \
            if (m_bBreakOnWarning) \
                wxTrap();           \
        }
#else
    void AddError(const char* pszErrMsg) { m_lstErrors += pszErrMsg; }
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

    const char* GetReportFilename() { return ttIsNonEmpty(m_cszReportPath) ? m_cszReportPath : m_cszSrcFilePath; }

public:
    // Class members (note that these are NOT marked protected or private -- too many callers need to access individual
    // members)

    // REVIEW: [randalphwa - 7/6/2019] Having these public: is a bad design. We should replace them with Get/Set
    // functions

    ttCStr m_cszLibName;  // Name and location of any additional library to build (used by Lib: section)
    ttCStr m_cszRcName;   // Resource file to build (if any)
    ttCStr m_cszHHPName;  // HTML Help project file

    ttCHeap m_ttHeap;  // All the ttCList files will be attatched to this heap

    ttCList m_lstSrcFiles;  // List of all source files
    ttCList m_lstLibFiles;  // List of any files used to build additional library
    ttCList m_lstIdlFiles;  // List of any idl files to compile with midl compiler

    ttCList m_lstErrors;  // List of any errors that occurred during processing

    ttCStrIntList m_lstAddSrcFiles;     // Additional .srcfiles.yaml to read into Files: section
    ttCStrIntList m_lstLibAddSrcFiles;  // Additional .srcfiles.yaml to read into Lib: section
    ttCList       m_lstSrcIncluded;  // The names of all files included by all ".include path/.srcfiles.yaml" directives

    ttCStr m_cszPchHdr;
    ttCStr m_cszPchCpp;

private:
    // Class members

    ttCStr m_cszSrcFilePath;
    ttCStr m_cszReportPath;  // Path to use when reporting a problem.
    ttCStr m_cszBldDir;      // This is where we write the .ninja files, and is ninja's builddir.

    ttCStr m_cszTargetRelease;
    ttCStr m_cszTargetDebug;

    std::string m_strTargetDir;

    int m_RequiredMajor;  // These three get filled in to the minimum ttBld version required to process.
    int m_RequiredMinor;
    int m_RequiredSub;

    bool m_bRead;  // File has been read and processed.
    bool m_bBreakOnWarning;  // Used in debug builds to call wxTrap().
};
