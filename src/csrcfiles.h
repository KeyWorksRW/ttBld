/////////////////////////////////////////////////////////////////////////////
// Name:      CSrcFiles
// Purpose:   Class for reading/writing .SrcFile (master file used by makemake.exe to generate build scripts)
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __CSRCFILES_H__
#define __CSRCFILES_H__

#include <ttlist.h>         // ttCList, ttCDblList, ttCStrIntList
#include <ttstr.h>          // ttStr, ttCWD
#include <ttfile.h>         // ttCFile
#include <ttarray.h>        // ttCArray
#include <ttmap.h>          // ttCMap

#include "options.h"        // CSrcOptions

using namespace sfopt;      // OPT_ options are used extensively, hence using the namespace in the header file

extern const char* txtSrcFilesFileName;

class CSrcFiles : public CSrcOptions
{
public:
    CSrcFiles(bool bVsCodeDir = false);     // bVsCodeDir should only be set to true to skip looking for .srcfiles.yaml in current directory

    // Public functions

    const char* GetBuildScriptDir();
    void AddFile(const char* pszFile) { m_lstSrcFiles += pszFile; }
    bool ReadFile(const char* pszFile = nullptr);

    bool IsProcessed() { return m_bRead; }

    bool IsExeTypeConsole()  { return (ttStrStrI(GetOption(OPT_EXE_TYPE), "console")); }    // this is the default
    bool IsExeTypeDll()      { return (ttStrStrI(GetOption(OPT_EXE_TYPE), "dll") || ttStrStrI(GetOption(OPT_EXE_TYPE), "ocx")); }
    bool IsExeTypeLib()      { return (ttStrStrI(GetOption(OPT_EXE_TYPE), "lib")); }
    bool IsExeTypeWindow()   { return (ttStrStrI(GetOption(OPT_EXE_TYPE), "window")); }

    bool IsStaticCrtRel()   { return (ttStrStrI(GetOption(OPT_STATIC_CRT_REL), "static")); }
    bool IsStaticCrtDbg()   { return (ttStrStrI(GetOption(OPT_STATIC_CRT_DBG), "static")); }

    bool IsOptimizeSpeed()   { return (ttStrStrI(GetOption(OPT_OPTIMIZE), "speed")); }

    const char* GetDir32()  { return GetOption(OPT_TARGET_DIR32); } // 32-bit target directory
    const char* GetDir64()  { return GetOption(OPT_TARGET_DIR64); } // 64-bit target directory

    const char* GetTargetDebug32();
    const char* GetTargetDebug64();
    const char* GetTargetRelease32();
    const char* GetTargetRelease64();

    const char* GetBuildLibs() { return GetOption(OPT_BUILD_LIBS); }
    const char* GetXgetFlags() { return GetOption(OPT_XGET_FLAGS); }

    void AddSourcePattern(const char* pszFilePattern);

    // These are just for convenience--it's fine to call GetOption directly

    const char* GetProjectName() { return GetOption(OPT_PROJECT); }
    const char* GetPchHeader();
    const char* GetPchCpp();    // source file to compile the precompiled header
    const char* GetSrcFiles() { return m_cszSrcFilePath; };  // get location of .srcfiles.yaml

    const char* GetBldDir() { return m_cszBldDir;  }        // ninja's builddir should be set to this directory

    bool IsVsCodeDir() { return m_bVsCodeDir; }                    // true means we are reading/writing files to .vscode

    int GetMajorRequired() { return m_RequiredMajor; }
    int GetMinorRequired() { return m_RequiredMinor; }
    int GetSubRequired() { return m_RequiredSub; }

    ttCList* GetSrcFilesList() { return &m_lstSrcFiles; }

protected:
    // Protected functions

    void ProcessFile(char* pszFile);
    void ProcessInclude(const char* pszFile, ttCStrIntList& lstAddSrcFiles, bool bFileSection);
    void ProcessLibSection(char* pszLibFile);
    void ProcessOption(char* pszLine);
    void ProcessTarget(char* pszLine);

    bool GetOptionParts(char* pszLine, ttCStr& cszName, ttCStr& cszVal, ttCStr& cszComment);

    void AddCompilerFlag(const char* pszFlag);
    void AddLibrary(const char* pszName);

    // Class members (note that these are NOT marked protected or private -- too many callers need to access individual members)

public:
    // REVIEW: [randalphwa - 7/6/2019] Having these public: is a bad design. We should replace them with Get/Set functions

    ttCStr m_cszLibName;        // name and location of any additional library to build (used by Lib: section)
    ttCStr m_cszRcName;         // resource file to build (if any)
    ttCStr m_cszHHPName;        // HTML Help project file

    ttCHeap m_ttHeap;           // all the ttCList files will be attatched to this heap

    ttCList m_lstSrcFiles;      // list of all source files
    ttCList m_lstLibFiles;      // list of any files used to build additional library
    ttCList m_lstIdlFiles;      // list of any idl files to compile with midl compiler

    ttCList m_lstErrors;        // list of any errors that occurred during processing

    ttCStrIntList m_lstAddSrcFiles;     // additional .srcfiles.yaml to read into Files: section
    ttCStrIntList m_lstLibAddSrcFiles;  // additional .srcfiles.yaml to read into Lib: section
    ttCList       m_lstSrcIncluded;     // the names of all files included by all ".include path/.srcfiles.yaml" directives

    ttCStr  m_cszPchHdr;
    ttCStr  m_cszPchCpp;
    ttCStr  m_cszSrcFilePath;

private:
    // Class members

    ttCStr m_cszTargetDebug32;
    ttCStr m_cszTargetDebug64;
    ttCStr m_cszTargetRelease32;
    ttCStr m_cszTargetRelease64;

    ttCStr  m_cszBldDir;    // this is where we write the .ninja files, and is ninja's builddir

    int m_RequiredMajor;    // these three get filled in to the minimum MakeNinja version required to process
    int m_RequiredMinor;
    int m_RequiredSub;

    bool m_bRead;           // file has been read and processed
    bool m_bVsCodeDir;      // means we reading/writing to files in the .vscode directory
};

#endif  // __CSRCFILES_H__
