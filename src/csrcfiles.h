/////////////////////////////////////////////////////////////////////////////
// Purpose:   Class for reading/writing .srcfiles.yaml (master file used by ttBld.exe to generate build scripts)
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

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
        failure,  // internal failure

    };
}  // namespace bld

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

    bool isOptValue(OPT::value option, std::string_view value) const
    {
        return (getOptValue(option).is_sameas(value, tt::CASE::either));
    }

    bool isOptTrue(OPT::value option) const
    {
        assert(option < OPT::LAST);
        return ttlib::is_sameas(m_Options[option].value, "true", tt::CASE::either);
    }

    bool hasOptValue(OPT::value option) const noexcept { return (!getOptValue(option).empty()); }

    const ttlib::cstr& getOptValue(OPT::value option) const noexcept
    {
        assert(option < OPT::LAST);
        return m_Options[option].value;
    }

    void setOptValue(OPT::value option, std::string_view value);
    void setBoolOptValue(OPT::value option, bool value = true);

    const std::string& getOptComment(OPT::value option) const noexcept
    {
        assert(option < OPT::LAST);
        return m_Options[option].comment;
    }

    void setOptComment(OPT::value option, std::string_view value)
    {
        assert(option < OPT::LAST);
        m_Options[option].comment = value;
    }

    void SetRequired(OPT::value option, bool isRequired = true)
    {
        assert(option < OPT::LAST);
        m_Options[option].isRequired = isRequired;
    }

    const std::string& GetTargetDir();
    const ttlib::cstr& GetTargetRelease();
    const ttlib::cstr& GetTargetDebug();
    const ttlib::cstr& GetTargetRelease32();
    const ttlib::cstr& GetTargetDebug32();
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

    void AddSourcePattern(std::string_view FilePattern);

    const ttlib::cstr& GetProjectName() { return m_Options[OPT::PROJECT].value; }
    const ttlib::cstr& GetPchCpp();
    bool HasPch() { return (hasOptValue(OPT::PCH) && !isOptValue(OPT::PCH, "none")); }

    void SetRcName(std::string_view name) { m_RCname = name; }
    const ttlib::cstr& getRcName() { return m_RCname; }

    // Gets name/location of srcfiles (normally .srcfiles.yaml)
    const ttlib::cstr& GetSrcFilesName() { return m_srcfilename; };

    // Ninja's builddir should be set to this directory
    const ttlib::cstr& GetBldDir() { return m_bldFolder; }

    int GetMajorRequired() { return m_RequiredMajor; }
    int GetMinorRequired() { return m_RequiredMinor; }
    int GetSubRequired() { return m_RequiredSub; }

    const auto& getErrorMsgs() { return m_lstErrMessages; }

    auto& GetSrcFileList() { return m_lstSrcFiles; }
    auto& GetDebugFileList() { return m_lstDebugFiles; }

    void SetReportingFile(std::string_view filename) { m_ReportPath = filename; }

    void AddError(std::string_view err);

    void InitOptions();

    bool isOptionRequired(OPT::value option) const
    {
        assert(option < OPT::LAST);
        return m_Options[option].isRequired;
    }

    std::string getOptionName(OPT::value option) const
    {
        assert(option < OPT::LAST);
        std::string name;
        name.assign(m_Options[option].OriginalName);
        return name;
    }

    bool hasOptionChanged(OPT::value option) const
    {
        assert(option < OPT::LAST);

        if (m_Options[option].value.empty() ||
            (m_Options[option].OriginalValue && m_Options[option].value.is_sameas(m_Options[option].OriginalValue)))
        {
            return false;
        }
        return true;
    }

protected:
    void ParseOption(std::string_view yamlLine);

    void ProcessFile(std::string_view line);
    void ProcessDebugFile(std::string_view line);
    void ProcessOption(std::string_view line);

    void ProcessIncludeDirective(std::string_view file, ttlib::cstr root = std::string {});

    void AddCompilerFlag(std::string_view flag);

    const ttlib::cstr& GetReportFilename() { return m_ReportPath; }

protected:
    void ProcessGzipLine(std::string_view line);
    void ProcessXpmLine(std::string_view line);
    void ProcessPngLine(std::string_view line);

    ttlib::cstr m_LIBname;  // Name and location of any additional library to build (used by Lib: section)
    ttlib::cstr m_RCname;   // Resource file to build (if any)
    ttlib::cstr m_HPPname;  // HTML Help project file

    std::vector<ttlib::cstr> m_lstSrcFiles;    // List of all source files except DEBUG build files
    std::vector<ttlib::cstr> m_lstIdlFiles;    // List of any idl files to compile with midl compiler
    std::vector<ttlib::cstr> m_lstDebugFiles;  // List of all source files for DEBUG builds only

    std::map<ttlib::cstr, std::string> m_gzip_files;  // Map of header/source filename pairs
    std::map<ttlib::cstr, ttlib::cstr> m_xpm_files;   // Map of src/dst filename pairs for xpm conversion
    std::map<ttlib::cstr, ttlib::cstr> m_png_files;   // Map of src/dst filename pairs for png conversion

    std::vector<ttlib::cstr> m_lstIncludeSrcFiles;

    ttlib::cstr m_pchCPPname;

    OPT::value FindOption(const std::string_view name) const;

private:
    friend CWriteSrcFiles;

    std::vector<ttlib::cstr> m_lstErrMessages;  // List of any errors that occurred during processing

    std::vector<CURRENT> m_Options { OPT::LAST + 1 };

    ttlib::cstr m_srcfilename;
    ttlib::cstr m_ReportPath;  // Path to use when reporting a problem.
    ttlib::cstr m_bldFolder;   // This is where we write the .ninja files, and is ninja's builddir.

    ttlib::cstr m_relTarget;
    ttlib::cstr m_dbgTarget;
    ttlib::cstr m_relTarget32;
    ttlib::cstr m_dbgTarget32;

    std::string m_strTargetDir;

    enum SRC_SECTION : size_t
    {
        SECTION_UNKNOWN,
        SECTION_OPTIONS,
        SECTION_FILES,
        SECTION_DEBUG_FILES,
        SECTION_GZIP,
        SECTION_XPM,
        SECTION_PNG,
    };
    SRC_SECTION m_section { SECTION_UNKNOWN };

    int m_RequiredMajor { 1 };  // These three get filled in to the minimum ttBld version required to process.
    int m_RequiredMinor { 4 };
    int m_RequiredSub { 0 };

    bool m_bRead { false };        // File has been read and processed.
    bool m_Initialized { false };  // true if InitOptions has been called
};
