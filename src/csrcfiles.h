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

#include <ttcstr.h>       // Classes for handling zero-terminated char strings.
#include <ttcvector.h>    // Vector of ttlib::cstr strings
#include <ttnamespace.h>  // Contains the tt namespace functions common to all tt libraries

#include "options.h"  // OPT -- Structures and enum for storing/retrieving options in a .srcfiles.yaml file

namespace bld
{
    enum RESULT : size_t
    {
        success,

        nochanges,     // nothing changed, so not written
        write_failed,  // unable to write to the file
        read_failed,   // unable to read the original file (but it does exist)

        invalid_file,
        failure,        // internal failure

    };
} // namespace bld

constexpr const char* txtSrcFilesFileName { ".srcfiles.yaml" };
constexpr const char* txtDefBuildDir { "bld" };

// Attempts to locate .srcfiles.yaml
ttlib::cstr locateProjectFile(std::string_view StartDir = std::string_view {});

class CWriteSrcFiles;  // forward definition

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
        return (getOptValue(option).issameas(value, tt::CASE::either));
    }

    bool isOptTrue(size_t index) const
    {
        assert(index < OPT::LAST);
        return ttlib::issameas(m_Options[index].value, "true", tt::CASE::either);
    }

    bool hasOptValue(size_t option) const noexcept { return (!getOptValue(option).empty()); }

    const ttlib::cstr& getOptValue(size_t index) const noexcept
    {
        assert(index < OPT::LAST);
        return m_Options[index].value;
    }

    void setOptValue(size_t index, std::string_view value);
    void setBoolOptValue(size_t index, bool value = true);

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

    void SetRequired(size_t index, bool isRequired = true)
    {
        assert(index < OPT::LAST);
        m_Options[index].isRequired = isRequired;
    }

    const std::string& GetTargetDir();
    const ttlib::cstr& GetTargetRelease();
    const ttlib::cstr& GetTargetDebug();
    const ttlib::cstr& GetBuildScriptDir();

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

    bool AddFile(std::string_view filename) { return m_lstSrcFiles.addfilename(filename); }
    void AddSourcePattern(std::string_view FilePattern);

    // These are just for convenience--it's fine to call getOptValue directly

    const ttlib::cstr& GetProjectName() { return m_Options[OPT::PROJECT].value; }
    const ttlib::cstr& GetPchCpp();

    void SetRcName(std::string_view name) { m_RCname = name; }
    const ttlib::cstr& getRcName() { return m_RCname; }

    // Gets name/location of srcfiles (normally .srcfiles.yaml)
    const ttlib::cstr& GetSrcFilesName() { return m_srcfilename; };

    // Ninja's builddir should be set to this directory
    const ttlib::cstr& GetBldDir() { return m_bldFolder; }

    int GetMajorRequired() { return m_RequiredMajor; }
    int GetMinorRequired() { return m_RequiredMinor; }
    int GetSubRequired() { return m_RequiredSub; }

    const ttlib::cstrVector& getErrorMsgs() { return m_lstErrMessages; }

    ttlib::cstrVector& GetSrcFileList() { return m_lstSrcFiles; }

    void SetReportingFile(std::string_view filename) { m_ReportPath = filename; }

    void AddError(const std::stringstream& msg) { AddError(msg.str()); };
    void AddError(std::string_view err);

protected:
    // Protected functions

    void ParseOption(std::string_view yamlLine);

    void ProcessFile(std::string_view line);
    void ProcessOption(std::string_view line);

    void ProcessIncludeDirective(std::string_view file, ttlib::cstr root = std::string {});

    void AddCompilerFlag(std::string_view flag);

    const ttlib::cstr& GetReportFilename() { return m_ReportPath; }

    void InitOptions();

protected:
    ttlib::cstr m_LIBname;  // Name and location of any additional library to build (used by Lib: section)
    ttlib::cstr m_RCname;   // Resource file to build (if any)
    ttlib::cstr m_HPPname;  // HTML Help project file

    ttlib::cstrVector m_lstSrcFiles;  // List of all source files
    ttlib::cstrVector m_lstLibFiles;  // List of any files used to build additional library
    ttlib::cstrVector m_lstIdlFiles;  // List of any idl files to compile with midl compiler

    ttlib::cstrVector m_lstIncludeSrcFiles;

    ttlib::cstr m_pchCPPname;

    size_t FindOption(const std::string_view name) const;

private:
    friend CWriteSrcFiles;

    // Class members

    ttlib::cstrVector m_lstErrMessages;  // List of any errors that occurred during processing

    std::vector<CURRENT> m_Options { OPT::LAST + 1 };

    ttlib::cstr m_srcfilename;
    ttlib::cstr m_ReportPath;  // Path to use when reporting a problem.
    ttlib::cstr m_bldFolder;   // This is where we write the .ninja files, and is ninja's builddir.

    ttlib::cstr m_relTarget;
    ttlib::cstr m_dbgTarget;

    std::string m_strTargetDir;

    int m_RequiredMajor { 1 };  // These three get filled in to the minimum ttBld version required to process.
    int m_RequiredMinor { 4 };
    int m_RequiredSub { 0 };

    bool m_bRead { false };  // File has been read and processed.
};
