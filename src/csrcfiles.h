/////////////////////////////////////////////////////////////////////////////
// Name:      CSrcFiles
// Purpose:   Class for reading/writing .srcfiles.yaml (master file used by ttBld.exe to generate build scripts)
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <ttlibwin.h>

#include <ttcstr.h>       // Classes for handling zero-terminated char strings.
#include <ttcvector.h>    // Vector of ttlib::cstr strings
#include <ttnamespace.h>  // Contains the tt namespace functions common to all tt libraries

#include <ttarray.h>  // ttCArray
#include <ttfile.h>   // ttCFile
#include <ttlist.h>   // ttCList, ttCDblList, ttCStrIntList
#include <ttmap.h>    // ttCMap

#include <ttstr.h>  // ttlib::cstr, ttStrlist -- String and vector classes with some additional functionality

#include "options.h"  // OPT:: enumerated Options

extern const char* txtSrcFilesFileName;

// Attempts to locate .srcfiles.yaml
ttlib::cstr locateProjectFile(std::string_view StartDir = ttEmptyString);

// Class for reading/writing .srcfiles.yaml (master file used by ttBld.exe to generate build scripts)
class CSrcFiles : public OPT
{
public:
    CSrcFiles();
    // NinjaDir is the directory to create .ninja scripts in
    CSrcFiles(std::string_view NinjaDir);

    // Public functions

    bool isOptValue(size_t option, std::string_view value) const
    {
        return (getOptValue(option).issameas(value, ttlib::CASE::either));
    }

    bool isOptTrue(size_t index) const
    {
        assert(index < OPT::LAST);
        return ttlib::issameas(m_Options[index].value, "true", ttlib::CASE::either);
    }

    bool hasOptValue(size_t option) const
    {
        return (!getOptValue(option).empty());
    }

    const ttlib::cstr& getOptValue(size_t index) const noexcept
    {
        assert(index < OPT::LAST);
        return m_Options[index].value;
    }

    void setOptValue(size_t index, std::string_view value);

    const std::string& getOptComment(size_t index) const noexcept
    {
        assert(index < OPT::LAST);
        return m_Options[index].comment;
    }

    void setOptComment(size_t index, std::string_view value)
    {
        assert(index < OPT::LAST);
        m_Options[index].comment = value;
    }

    const char* GetTargetDir();
    const char* GetTargetRelease();
    const char* GetTargetDebug();

    const char* GetBuildScriptDir();

    // If filename is not specified, CSrcFiles will attempt to locate the file.
    bool ReadFile(std::string_view filename = std::string_view {});

    bool IsProcessed() { return m_bRead; }

    bool IsExeTypeConsole() const { return (isOptValue(OPT::EXE_TYPE, "console")); }
    bool IsExeTypeDll() const { return (isOptValue(OPT::EXE_TYPE, "dll") || isOptValue(OPT::EXE_TYPE, "ocx")); }
    bool IsExeTypeLib() const { return (isOptValue(OPT::EXE_TYPE, "lib")); }
    bool IsExeTypeWindow() const { return (isOptValue(OPT::EXE_TYPE, "window")); }
    bool IsStaticCrtRel() const { return (isOptValue(OPT::CRT_REL, "static")); }
    bool IsStaticCrtDbg() const { return (isOptValue(OPT::CRT_DBG, "static")); }
    bool IsOptimizeSpeed() const { return (isOptValue(OPT::OPTIMIZE, "speed")); }

    // const char* GetBuildLibs() { return GetOption(OPT_BUILD_LIBS); }
    // const char* GetXgetFlags() { return GetOption(OPT_XGET_FLAGS); }

    bool AddFile(std::string_view filename) { return m_lstSrcFiles.addfilename(filename); }
    void AddSourcePattern(std::string_view FilePattern);

    // These are just for convenience--it's fine to call GetOption directly

    const ttlib::cstr& GetProjectName() { return m_Options[OPT::PROJECT].value; }
    const char* GetPchHeader() const;
    // Source file to compile the precompiled header
    const char* GetPchCpp();

    void SetRcName(std::string_view name) { m_RCname = name; }
    const ttlib::cstr& getRcName() { return m_RCname; }

    // Gets name/location of srcfiles (normally .srcfiles.yaml)
    const ttlib::cstr& GetSrcFilesName() { return m_srcfilename; };

    // Ninja's builddir should be set to this directory
    const ttlib::cstr& GetBldDir() { return m_bldFolder; }

    int GetMajorRequired() { return m_RequiredMajor; }
    int GetMinorRequired() { return m_RequiredMinor; }
    int GetSubRequired() { return m_RequiredSub; }

    ttlib::cstrVector& GetSrcFilesList() { return m_lstSrcFiles; }

    void SetReportingFile(std::string_view filename) { m_ReportPath = filename; }

    void AddError(const std::stringstream& msg) { AddError(msg.str()); };
#if !defined(NDEBUG)  // Starts debug section.
    void AddError(std::string_view err);
#else
    void AddError(std::string_view err) { m_lstErrMessages.append(err); }
#endif

protected:
    // Protected functions

    void ParseOption(std::string_view yamlLine);

    void ProcessFile(std::string_view line);
    void ProcessOption(std::string_view line);

    void ProcessInclude(const char* pszFile, ttCStrIntList& lstAddSrcFiles, bool bFileSection);
    void ProcessTarget(char* pszLine);

    void AddCompilerFlag(std::string_view flag);
    //    void AddLibrary(const char* pszName);     // REVIEW: [KeyWorks - 8/7/2019] doesn't appear to be used

    const ttlib::cstr& GetReportFilename() { return m_ReportPath; }

    void InitOptions();

    CURRENT& getOption(size_t index)
    {
        assert(index < LAST);
        return m_Options[index];
    }

protected:
    ttlib::cstr m_LIBname;  // Name and location of any additional library to build (used by Lib: section)
    ttlib::cstr m_RCname;   // Resource file to build (if any)
    ttlib::cstr m_HPPname;  // HTML Help project file

    ttCHeap m_ttHeap;  // All the ttCList files will be attatched to this heap

    ttlib::cstrVector m_lstSrcFiles;  // List of all source files
    ttlib::cstrVector m_lstLibFiles;  // List of any files used to build additional library
    ttlib::cstrVector m_lstIdlFiles;  // List of any idl files to compile with midl compiler

    ttlib::cstrVector m_lstErrMessages;  // List of any errors that occurred during processing

    ttCStrIntList m_lstAddSrcFiles;     // Additional .srcfiles.yaml to read into Files: section
    ttCStrIntList m_lstLibAddSrcFiles;  // Additional .srcfiles.yaml to read into Lib: section
    ttCList m_lstSrcIncluded;  // The names of all files included by all ".include path/.srcfiles.yaml" directives

    ttlib::cstr m_pchCPPname;

    size_t FindOption(const std::string_view name) const;

private:
    // Class members

    std::vector<CURRENT> m_Options { OPT::LAST + 1 };

    ttlib::cstr m_srcfilename;
    ttlib::cstr m_ReportPath;  // Path to use when reporting a problem.
    ttlib::cstr m_bldFolder;   // This is where we write the .ninja files, and is ninja's builddir.

    ttlib::cstr m_relTarget;
    ttlib::cstr m_dbgTarget;

    std::string m_strTargetDir;

    int m_RequiredMajor { 1 };  // These three get filled in to the minimum ttBld version required to process.
    int m_RequiredMinor { 0 };
    int m_RequiredSub { 0 };

    bool m_bRead { false };  // File has been read and processed.
};
