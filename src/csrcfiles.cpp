/////////////////////////////////////////////////////////////////////////////
// Name:      CSrcFiles
// Purpose:   Class for reading/writing .srcfiles.yaml (master file used by ttBld.exe to generate build scripts)
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <sstream>

#include <ttcwd.h>       // Class for storing and optionally restoring the current directory
#include <ttmultistr.h>  // multistr -- Breaks a single string into multiple strings
#include <tttextfile.h>  // Classes for reading and writing line-oriented files
#include <ttwinff.h>     // Wrapper around Windows FindFile

#include "csrcfiles.h"  // CSrcFiles

CSrcFiles::CSrcFiles() {}

CSrcFiles::CSrcFiles(std::string_view NinjaDir)
{
    m_bldFolder = NinjaDir;
}

bool CSrcFiles::ReadFile(std::string_view filename)
{
    if (filename.empty())
    {
        auto project = locateProjectFile();
        if (project.empty())
        {
            ttlib::cwd cwd;
            AddError(_tt("Cannot locate .srcfiles.yaml starting in ") + cwd);
            return false;  // if we still can't find it, bail
        }
        m_srcfilename = std::move(project);
    }
    else
        m_srcfilename = filename;

    if (m_bldFolder.empty())
    {
        // pszFile might now point to a sub-directory, so we need to create the .ninja directory under that
        m_bldFolder = m_srcfilename;
        m_bldFolder.replace_filename(txtDefBuildDir);
    }

    ttlib::viewfile SrcFile;
    if (!SrcFile.ReadFile(m_srcfilename))
    {
        AddError(_tt("Cannot open ") + m_srcfilename);
        return false;
    }

    m_bRead = true;

    InitOptions();

    m_section = SECTION_UNKNOWN;

    for (auto& line: SrcFile)
    {
        // Note that we are only looking for leading characters that would appear in a .srcfiles.yaml file, not all
        // of the special characters allowed in the full YAML specification.
        if (line.empty() || line[0] == '#' || line[0] == '-')
            continue;

        // If the line doesn't begin with whitespace, then they only thing we look at is whether it is a section
        // name
        if (ttlib::is_alpha(line[0]))
        {
            if (ttlib::is_sameprefix(line, "Files:", tt::CASE::either) || ttlib::is_sameprefix(line, "[FILES]", tt::CASE::either))
            {
                m_section = SECTION_FILES;
            }
            else if (ttlib::is_sameprefix(line, "DebugFiles:", tt::CASE::either) ||
                     ttlib::is_sameprefix(line, "[DEBUG FILES]", tt::CASE::either))
            {
                m_section = SECTION_DEBUG_FILES;
            }
            else if (ttlib::is_sameprefix(line, "GZIP:", tt::CASE::either) || ttlib::is_sameprefix(line, "[GZIP]", tt::CASE::either))
            {
                m_section = SECTION_GZIP;
            }
            else if (ttlib::is_sameprefix(line, "Options:", tt::CASE::either) || ttlib::is_sameprefix(line, "[OPTIONS]"))
            {
                m_section = SECTION_OPTIONS;
            }
            else
            {
                m_section = SECTION_UNKNOWN;
            }
            continue;
        }

        auto begin = ttlib::find_nonspace(line);
        if (begin.empty())
            continue;

        if (begin[0] == '#')
        {
            // If an older version of ttBld marked the option as unrecognized, but this current version of ttBld does
            // recognize it, then go ahead and process it as if it was a normal option. Note that this means that if the
            // Options dialog is reading this, it will display the option as if it was created normally. If it writes
            // the file, the comment line will be replaced with the actual option.

            if (m_section == SECTION_OPTIONS && ttlib::is_sameprefix(begin, "# unrecognized option --"))
            {
                ttlib::cstr option_line = ttlib::stepover(begin.substr(begin.find("--")));
                auto pos = option_line.find_oneof(":=");
                if (pos == ttlib::cstr::npos)
                    continue;
                ttlib::cstr name(option_line.substr(0, pos));
                auto option = FindOption(name);
                if (option != OPT::LAST)
                {
                    ProcessOption(option_line);
                }
            }

            continue;
        }

        switch (m_section)
        {
            case SECTION_FILES:
                ProcessFile(begin);
                break;

            case SECTION_DEBUG_FILES:
                ProcessDebugFile(begin);
                break;

            case SECTION_GZIP:
                ProcessGzipLine(begin);
                break;

            case SECTION_OPTIONS:
                ProcessOption(begin);
                break;

            case SECTION_UNKNOWN:
            default:
                break;  // ignore it since we don't know what it's supposed to be used for
        }
    }

    // Everything has been processed, if options were not specified that are needed, make some default assumptions

    if (getOptValue(OPT::PROJECT).empty())
    {
        ttlib::cstr projectname;
        projectname.assignCwd();
        if (ttlib::is_sameprefix(projectname.filename(), "src", tt::CASE::either))
        {
            projectname.replace_filename("");
            // remove trailing slash
            projectname.pop_back();
        }
        std::string name(projectname.filename());
        setOptValue(OPT::PROJECT, name);
    }

    // If no Files: were specified, then we still won't have any files to build. Default to every type of C++
    // source file in the current directory.

    if (!m_lstSrcFiles.size())
    {
#if defined(_WIN32)
        AddSourcePattern("*.cpp;*.cc;*.cxx;*.rc;*.idl");
#else
        AddSourcePattern("*.cpp;*.cc;*.cxx");
#endif
    }

    return true;
}

void CSrcFiles::ProcessOption(std::string_view yamlLine)
{
    ttlib::cstr name;
    ttlib::cstr value;
    ttlib::cstr comment;

    ttlib::cstr line = ttlib::find_nonspace(yamlLine);
    auto pos = line.find_oneof(":=");
    if (pos == ttlib::cstr::npos)
    {
        AddError(_tt("Invalid Option -- missing ':' or '=' character"));
        return;
    }

    name.assign(line.substr(0, pos));

    // Ignore or change obsolete options

    if (name.is_sameprefix("64Bit") || name.is_sameprefix("b64_suffix") || name.is_sameprefix("b32_suffix"))
    {
        return;
    }

    pos = line.stepover(pos);
    if (pos == ttlib::cstr::npos)
    {
        std::stringstream msg;
        msg << _tt("The option ") << name << _tt(" does not have a value");
        AddError(msg.str());
        return;
    }

    if (line[pos] == '"')
    {
        auto posNext = value.ExtractSubString(line, pos);
        if (posNext == ttlib::cstr::npos)
        {
            std::stringstream msg;
            msg << _tt("The value for ") << name << _tt(" has an opening quote, but no closing quote.");
            AddError(msg.str());
            value.assign(line.substr(pos));
            posNext = pos;
        }
        posNext = line.find('#', posNext + 1);
        if (posNext != ttlib::cstr::npos)
        {
            // Technically we should check the preceeeding character and determine if it is a backslash. Shouldn't
            // ever occur in .srcfiles.yanml files, but it is allowed in the YAML spec.
            comment.assign(line.substr(line.find_nonspace(posNext + 1)));
        }
    }
    else
    {
        auto posComment = line.find('#', pos);
        if (posComment != ttlib::cstr::npos)
        {
            // Technically we should check the preceeeding character and determine if it is a backslash. Shouldn't
            // ever occur in .srcfiles.yanml files, but it is allowed in the YAML spec.
            comment.assign(line.substr(line.find_nonspace(posComment + 1)));
            value.assign(line.substr(pos, posComment - pos));
            value.trim();
        }
        else
        {
            value.assign(line.substr(pos));
            value.trim();
        }
    }

    auto option = FindOption(name);
    if (option == OPT::LAST)
    {
        std::stringstream msg;
        msg << name << _tt(" is an unrecognized option and will be ignored.");
        AddError(msg.str());
        return;
    }

    m_Options[option].value = value;
    m_Options[option].comment = comment;
}

void CSrcFiles::AddCompilerFlag(std::string_view flag)
{
    if (m_Options[OPT::CFLAGS_CMN].value.contains(flag))
        return;
    if (!m_Options[OPT::CFLAGS_CMN].value.empty())
        m_Options[OPT::CFLAGS_CMN].value += " ";
    m_Options[OPT::CFLAGS_CMN].value += flag;
}

void CSrcFiles::ProcessFile(std::string_view line)
{
    if (ttlib::is_sameprefix(line, ".include", tt::CASE::either))
    {
        ttlib::cstr filename = ttlib::stepover(line);
        if (filename[0] == '\"')
            filename.ExtractSubString(filename);
        else
        {
            // Remove any comment
            filename.erase_from('#');
        }
        if (!filename.empty())
            ProcessIncludeDirective(filename);
        return;
    }

    ttlib::cstr filename = line;
    filename.erase_from('#');

    if (filename.find('*') != tt::npos || filename.find('?') != tt::npos)
    {
        AddSourcePattern(filename);
        return;
    }

    if (auto name = m_lstSrcFiles.addfilename(filename); !name.file_exists())
    {
        AddError(_tt("Unable to locate the file ") + name);
    }

    if (filename.has_extension(".idl"))
    {
        m_lstIdlFiles += filename.c_str();
    }

    // ignore .rc2, .resources, etc.
    else if (filename.has_extension(".rc") && filename.extension().length() < sizeof(".rc"))
    {
        m_RCname = filename;
    }
    else if (filename.has_extension(".hhp"))
    {
        m_HPPname = filename;
    }
}

void CSrcFiles::ProcessDebugFile(std::string_view line)
{
    ttlib::cstr filename = line;
    filename.erase_from('#');

    // Unlike the regular Files: section, the DebugFiles: section does not support .include, .idl, .rc, or .hhp files

    if (filename.find('*') != tt::npos || filename.find('?') != tt::npos)
    {
        AddSourcePattern(filename);
        return;
    }

    if (auto name = m_lstDebugFiles.addfilename(filename); !name.file_exists())
    {
        AddError(_tt("Unable to locate the file ") + name);
    }
}

/*
    Lines are expected to be of the form:

        source.src_ext: header.hdr_ext  # optional comment

        *.src_ext: *.hdr_ext

        *.src_ext: header.hdr_ext

    In the above case all files matching *.src_ext will be converted into arrays in header.hdr_ext

    The reason for putting the source file first is because it is valid to have multiple source files with a single
    destination.
*/
void CSrcFiles::ProcessGzipLine(std::string_view line)
{
    ttlib::multistr pair(line, ':');
    if (pair.size() < 2)
    {
        AddError(_tt("Expected \"header: source\" but ':' not found seperating the two"));
        return;
    }

    pair[0].trim(tt::TRIM::both);
    pair[1].erase_from('#');
    pair[1].trim(tt::TRIM::both);

    m_gzip_files[pair[0]] = pair[1];
}

// Process a ".include" directive
void CSrcFiles::ProcessIncludeDirective(std::string_view file, ttlib::cstr root)
{
    if (ttlib::is_sameprefix(file, ".include"))
    {
        ttlib::cstr filename = ttlib::stepover(file);
        filename.erase_from('#');
        if (!filename.empty())
        {
            ProcessIncludeDirective(filename);
        }
        return;
    }

    ttlib::cwd cwd(true);
    if (root.empty())
        root = cwd;

    ttlib::cstr FullPath(file);
    FullPath.make_absolute();

    CSrcFiles cIncSrcFiles;
    cIncSrcFiles.SetReportingFile(FullPath);
    ttlib::cstr incRoot = FullPath;
    incRoot.remove_filename();

    try
    {
        ttlib::cstr NewDir(FullPath);
        auto filename = NewDir.filename();
        if (!filename.empty())
            fs::current_path(filename.c_str());
    }
    catch (const std::exception& e)
    {
        AddError(_tt("An exception occurred while reading ") + FullPath + ": " + e.what());
        return;
    }

    if (!cIncSrcFiles.ReadFile(FullPath))
    {
        AddError(_tt("Unable to locate the file ") + FullPath);
        return;
    }

    for (auto& incFile: cIncSrcFiles.m_lstSrcFiles)
    {
        ttlib::cstr filename = incFile;
        filename.make_relative(incRoot);
        filename.make_absolute();
        filename.make_relative(root);
        m_lstSrcFiles.addfilename(filename);
    }
}

void CSrcFiles::AddSourcePattern(std::string_view FilePattern)
{
    if (FilePattern.empty())
        return;

    ttlib::multistr enumPattern(FilePattern, ';');
    for (auto& pattern: enumPattern)
    {
        ttlib::winff ff(pattern);
        while (ff.isvalid())
        {
            auto& name = ff.getcstr();
            if (name.has_extension(".c") || name.has_extension(".cpp") || name.has_extension(".cxx"))
            {
                if (m_section == SECTION_DEBUG_FILES)
                    m_lstDebugFiles.addfilename(name);
                else
                    m_lstSrcFiles.addfilename(name);
            }
            else if (name.has_extension(".rc"))
            {
                if (m_section != SECTION_DEBUG_FILES)
                {
                    m_lstSrcFiles.addfilename(name);
                    m_RCname = name;
                }
            }
            else if (name.has_extension(".hhp"))
            {
                if (m_section != SECTION_DEBUG_FILES)
                {
                    m_HPPname = name;
                }
            }
            else if (name.has_extension(".idl"))
            {
                if (m_section != SECTION_DEBUG_FILES)
                {
                    m_lstSrcFiles.addfilename(name);
                    m_lstIdlFiles.addfilename(name);
                }
            }

            if (!ff.next())
                break;
        }
    }
}

const ttlib::cstr& CSrcFiles::GetPchCpp()
{
    if (hasOptValue(OPT::PCH_CPP) && !isOptValue(OPT::PCH_CPP, "none"))
    {
        m_pchCPPname = getOptValue(OPT::PCH_CPP);
        return m_pchCPPname;
    }

    if (!hasOptValue(OPT::PCH))
        return m_pchCPPname;

    m_pchCPPname = getOptValue(OPT::PCH);
    m_pchCPPname.replace_extension(".cpp");
    if (m_pchCPPname.file_exists())
        return m_pchCPPname;

    // Check for other possible extensions

    m_pchCPPname.replace_extension(".cc");
    if (m_pchCPPname.file_exists())
        return m_pchCPPname;

    m_pchCPPname.replace_extension(".cxx");
    if (m_pchCPPname.file_exists())
        return m_pchCPPname;

    m_pchCPPname.replace_extension(".cpp");  // file doesn't exist, we'll generate a warning about it later
    return m_pchCPPname;
}

const ttlib::cstr& CSrcFiles::GetBuildScriptDir()
{
    if (!m_bldFolder.empty())
        return m_bldFolder;

    if (m_srcfilename.empty())
    {
        m_bldFolder = txtDefBuildDir;
        return m_bldFolder;
    }

    return m_bldFolder;
}

const std::string& CSrcFiles::GetTargetDir()
{
    if (!m_strTargetDir.empty())
        return m_strTargetDir;

    if (hasOptValue(OPT::TARGET_DIR))
    {
        m_strTargetDir = getOptValue(OPT::TARGET_DIR);
        return m_strTargetDir;
    }

    ttlib::cstr dir(IsExeTypeLib() ? "../lib" : "../bin");

    // If it's not 32-bit code then just use lib or bin as the target dir if not specified.
    if (hasOptValue(OPT::TARGET_DIR64) || !isOptTrue(OPT::BIT32))
    {
        if (hasOptValue(OPT::TARGET_DIR64))
        {
            m_strTargetDir = getOptValue(OPT::TARGET_DIR64);
            return m_strTargetDir;
        }
        else if (hasOptValue(OPT::TARGET_DIR))
        {
            m_strTargetDir = getOptValue(OPT::TARGET_DIR);
            return m_strTargetDir;
        }

        ttlib::cstr cwd;
        cwd.assignCwd();
        bool isSrcDir = ttlib::is_sameas(cwd.filename(), "src", tt::CASE::either) ? true : false;
        if (!isSrcDir)
        {
            cwd.append_filename(IsExeTypeLib() ? "../lib" : "../bin");
            if (cwd.dir_exists())
                isSrcDir = true;
        }
        if (isSrcDir)
        {
            dir = (IsExeTypeLib() ? "../lib" : "../bin");
        }
        else
        {
            dir = (IsExeTypeLib() ? "lib" : "bin");
        }
    }

    // For 32-bit code, check to see if there is a 32, x86, or _x86 suffix to the standard bin and lib directories.
    // If it exists and the user didn't tell us where to put it, then use that directory.
    else
    {
        if (hasOptValue(OPT::TARGET_DIR32))
        {
            m_strTargetDir = getOptValue(OPT::TARGET_DIR32);
            return m_strTargetDir;
        }
        else if (hasOptValue(OPT::TARGET_DIR))
        {
            m_strTargetDir = getOptValue(OPT::TARGET_DIR);
            return m_strTargetDir;
        }

        ttlib::cstr cwd;
        cwd.assignCwd();
        bool isSrcDir = ttlib::is_sameas(cwd.filename(), "src", tt::CASE::either) ? true : false;
        if (!isSrcDir)
        {
            cwd.append_filename(IsExeTypeLib() ? "../lib" : "../bin");
            if (cwd.dir_exists())
                isSrcDir = true;
        }

        // For 32-bit

        if (isSrcDir)
            dir = (IsExeTypeLib() ? "../lib" : "../bin");
        else
            dir = (IsExeTypeLib() ? "lib" : "bin");

        ttlib::cstr tmp(dir);
        tmp += "32";
        if (tmp.dir_exists())
            dir = tmp;
        else
        {
            tmp = (dir + "x86");
            if (tmp.dir_exists())
                dir = tmp;
            else
            {
                tmp = (dir + "_x86");
                if (tmp.dir_exists())
                    dir = tmp;
            }
        }
    }

    m_strTargetDir = dir;
    return m_strTargetDir;
}

const ttlib::cstr& CSrcFiles::GetTargetRelease()
{
    if (!m_relTarget.empty())
        return m_relTarget;

    m_relTarget = GetTargetDir();
    m_relTarget.append_filename(GetProjectName());

    if (IsExeTypeLib())
        m_relTarget += ".lib";
    else if (IsExeTypeDll())
        m_relTarget += (getOptValue(OPT::EXE_TYPE).contains("ocx", tt::CASE::either) ? ".ocx" : ".dll");
    else
        m_relTarget += ".exe";
    return m_relTarget;
}

const ttlib::cstr& CSrcFiles::GetTargetDebug()
{
    if (!m_dbgTarget.empty())
        return m_dbgTarget;

    m_dbgTarget = GetTargetDir();
    m_dbgTarget.append_filename(GetProjectName());

    // Never automatically add a 'D' to a dll.
    if (!IsExeTypeDll())
        m_dbgTarget += "D";

    if (IsExeTypeLib())
        m_dbgTarget += ".lib";
    else if (IsExeTypeDll())
        m_dbgTarget += (getOptValue(OPT::EXE_TYPE).contains("ocx", tt::CASE::either) ? ".ocx" : ".dll");
    else
        m_dbgTarget += ".exe";
    return m_dbgTarget;
}

#if defined(WIN32)
    #include <ttdebug.h>

void CSrcFiles::AddError(std::string_view err)
{
    m_lstErrMessages.append(err);
    ttFAIL_MSG(err.data());
}

#else

void CSrcFiles::AddError(std::string_view err)
{
    m_lstErrMessages.append(err);
}

#endif

static const char* aProjectLocations[] {
    // clang-format off
    "src/",
    "source/",
    "bld/",
    "build/",
    ".private/",
    // clang-format on
};

ttlib::cstr locateProjectFile(std::string_view StartDir)
{
    // A project built with wxWidgets is cross-platform, and as such it takes precedence over all other variations
    constexpr const char* txtWidgetsName { ".srcfiles.wx.yaml" };

#if defined(_WIN32)
    constexpr const char* txtPlatformName { ".srcfiles.win.yaml" };
#elif defined(__APPLE__)
    constexpr const char* txtPlatformName { ".srcfiles.mac.yaml" };
#else
    constexpr const char* txtPlatformName { ".srcfiles.unix.yaml" };
#endif  // _WIN32

#if !defined(NDEBUG)  // Starts debug section.
    // Just so you can tell what the current directory is in a debugger.
    ttlib::cwd cwd;
#endif

    ttlib::cstr path;
    if (!StartDir.empty())
    {
        path.assign(StartDir);
        path.addtrailingslash();

        path.append_filename(txtWidgetsName);
        if (path.file_exists())
            return path;

        path.replace_filename(txtPlatformName);
        if (path.file_exists())
            return path;

        path.replace_filename(".srcfiles.yaml");
        if (path.file_exists())
            return path;

        // Search common sub directories for the file.
        for (auto iter: aProjectLocations)
        {
            path.assign(StartDir);
            path.addtrailingslash();
            path.append_filename(iter);

            path.append_filename(txtWidgetsName);
            if (path.file_exists())
                return path;

            path.replace_filename(txtPlatformName);
            if (path.file_exists())
                return path;

            path.replace_filename(".srcfiles.yaml");
            if (path.file_exists())
                return path;
        }
    }
    else
    {
        // A wxWidgets version takes precedence
        path.assign(txtWidgetsName);
        if (path.file_exists())
            return path;

        // Otherwise look for a platform-specific version
        path.assign(txtPlatformName);
        if (path.file_exists())
            return path;

        // If all else fails, use a "normal" version.
        path.assign(".srcfiles.yaml");
        if (path.file_exists())
            return path;

        for (auto iter: aProjectLocations)
        {
            path.assign(iter);

            path.append_filename(txtWidgetsName);
            if (path.file_exists())
                return path;

            path.replace_filename(txtPlatformName);
            if (path.file_exists())
                return path;

            path.replace_filename(".srcfiles.yaml");
            if (path.file_exists())
                return path;
        }
    }

    path.clear();
    return path;
}
