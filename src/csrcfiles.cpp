/////////////////////////////////////////////////////////////////////////////
// Name:      CSrcFiles
// Purpose:   Class for reading/writing .srcfiles.yaml (master file used by ttBld.exe to generate build scripts)
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <sstream>

#include <ttTR.h>  // Function for translating strings

#include <ttenumstr.h>  // ttCEnumStr
#include <ttmem.h>      // ttCMem, ttCTMem

#include <ttcwd.h>       // Class for storing and optionally restoring the current directory
#include <tttextfile.h>  // Classes for reading and writing line-oriented files
#include <ttwinff.h>     // Wrapper around Windows FindFile

#include "csrcfiles.h"  // CSrcFiles

const char* txtSrcFilesFileName = ".srcfiles.yaml";
const char* txtDefBuildDir = "bld";

typedef enum
{
    SECTION_UNKNOWN,
    SECTION_OPTIONS,
    SECTION_FILES,
} SRC_SECTION;

// make all ttCList classes use the same sub-heap
CSrcFiles::CSrcFiles() : m_ttHeap(true), m_lstLibAddSrcFiles(m_ttHeap), m_lstSrcIncluded(m_ttHeap) {}

CSrcFiles::CSrcFiles(std::string_view NinjaDir) :
    m_ttHeap(true), m_lstLibAddSrcFiles(m_ttHeap), m_lstSrcIncluded(m_ttHeap)
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
            std::stringstream msg;
            msg << "Cannot locate .srcfiles.yaml starting in " << cwd;
            m_lstErrMessages.append(msg.str());
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
        std::string msg = (_tt("Cannot open ") + m_srcfilename);
        m_lstErrMessages.append(msg);
        return false;
    }

    m_bRead = true;

    InitOptions();

    SRC_SECTION section = SECTION_UNKNOWN;

    for (auto line: SrcFile)
    {
        // Note that we are only looking for leading characters that would appear in a .srcfiles.yaml file, not all
        // of the special characters allowed in the full YAML specification.
        if (line.empty() || line[0] == '#' || line[0] == '-')
            continue;

        // If the line doesn't begin with whitespace, then they only thing we look at is whether it is a section
        // name
        if (ttlib::isalpha(line[0]))
        {
            if (ttlib::issameprefix(line, "Files:", ttlib::CASE::either) ||
                ttlib::issameprefix(line, "[FILES]", ttlib::CASE::either))
            {
                section = SECTION_FILES;
            }
            else if (ttlib::issameprefix(line, "Options:", ttlib::CASE::either) ||
                     ttlib::issameprefix(line, "[OPTIONS]"))
            {
                section = SECTION_OPTIONS;
            }
            else
            {
                section = SECTION_UNKNOWN;
            }
            continue;
        }

        auto begin = ttlib::findnonspace(line);

        switch (section)
        {
            case SECTION_FILES:
                ProcessFile(begin);
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
        if (ttlib::issameprefix(projectname.filename(), "src", ttlib::CASE::either))
        {
            projectname.replace_filename("");
            // remove trailing slash
            projectname.erase(projectname.length() - 1, 1);
        }
        std::string name(projectname.filename());
        setOptValue(OPT::PROJECT, name);
    }

    // If no Files: were specified, then we still won't have any files to build. Default to every type of C++
    // source file in the current directory.

    if (!m_lstSrcFiles.size())
    {
#if defined(_WIN32)
        AddSourcePattern("*.cpp;*.cc;*.cxx;*.rc;*.idl;*.hhp");
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

    ttlib::cstr line = tt::findnonspace(yamlLine);
    auto pos = line.findoneof(":=");
    if (pos == ttlib::cstr::npos)
    {
        AddError(_tt("Invalid Option -- missing ':' or '=' character"));
        return;
    }

    name.assign(line.substr(0, pos));

    // Ignore or change obsolete options

    if (name.issameprefix("64Bit") || name.issameprefix("b64_suffix") || name.issameprefix("b32_suffix"))
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
            comment.assign(line.substr(line.findnonspace(posNext + 1)));
        }
    }
    else
    {
        auto posComment = line.find('#', pos);
        if (posComment != ttlib::cstr::npos)
        {
            // Technically we should check the preceeeding character and determine if it is a backslash. Shouldn't
            // ever occur in .srcfiles.yanml files, but it is allowed in the YAML spec.
            comment.assign(line.substr(line.findnonspace(posComment + 1)));
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
    auto& option = getOption(OPT::CFLAGS_CMN);
    if (option.value.contains(flag))
        return;
    if (!option.value.empty())
        option.value += " ";
    option.value += flag;
}

void CSrcFiles::ProcessFile(std::string_view line)
{
    if (ttlib::issameprefix(line, ".include", ttlib::CASE::either))
    {
        ttlib::cstr filename = ttlib::stepover(line);
        if (filename[0] == '\"')
            filename.ExtractSubString(filename);
        else
        {
            // If the filename isn't in quotes, then remove any trailing comment
            auto pos = filename.find('#');
            if (pos != ttlib::npos)
            {
                filename.erase(pos);
                filename.trim();
            }
        }
        if (!filename.empty())
            ProcessInclude(filename.c_str(), m_lstAddSrcFiles, true);
        return;
    }

    ttlib::cstr filename = line;
    auto pos = filename.find('#');
    if (pos != ttlib::npos)
    {
        filename.erase(pos);
        filename.trim();
    }

    if (filename.find('*') || filename.find('?'))
    {
        AddSourcePattern(filename.c_str());
        return;
    }

    if (m_lstSrcFiles.addfilename(filename))
    {
        if (!m_lstSrcFiles.back().fileExists())
        {
            ttlib::cstr msg(_tt("Unable to locate the file ") + filename);
            AddError(msg);
        }
    }

    if (filename.hasExtension(".idl"))
    {
        m_lstIdlFiles += filename.c_str();
    }

    // ignore .rc2, .resources, etc.
    else if (filename.hasExtension(".rc") && filename.extension().length() < 3)
    {
        m_RCname = filename;
    }

    else if (filename.hasExtension(".hhp"))
    {
        m_HPPname = filename;
    }
}

void CSrcFiles::ProcessInclude(const char* pszFile, ttCStrIntList& lstAddSrcFiles, bool bFileSection)
{
    if (ttIsSameSubStrI(pszFile, ".include"))
    {
        const char* pszIncFile = ttFindNonSpace(ttFindSpace(pszFile));
        if (pszIncFile)
        {
            ttCStr cszFile(pszIncFile);
            char* pszTmp = ttStrChr(cszFile, '#');
            if (pszTmp)
            {
                *pszTmp = 0;
                cszFile.TrimRight();
            }
            ProcessInclude(cszFile, m_lstLibAddSrcFiles, false);
        }
        return;
    }

    ttlib::cwd cwd(true);

    ttlib::cstr FullPath(pszFile);
    FullPath.make_absolute();

    CSrcFiles cIncSrcFiles;
    cIncSrcFiles.SetReportingFile(FullPath);

    try
    {
        ttlib::cstr NewDir(FullPath);
        auto filename = NewDir.filename();
        if (!filename.empty())
            fs::current_path(filename.c_str());
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    if (!cIncSrcFiles.ReadFile(FullPath))
    {
        ttlib::cstr str(_tt("Unable to locate the file ") + FullPath);
        m_lstErrMessages.append(str);
        return;
    }

    ttCTMem<char*> szPath(1024);
    ttStrCpy(szPath, 1024, FullPath.c_str());
    char* pszFilePortion = ttFindFilePortion(szPath);

    ttCStr cszRelative;

    for (size_t pos = 0;
         pos < (bFileSection ? cIncSrcFiles.m_lstSrcFiles.size() : cIncSrcFiles.m_lstLibFiles.size()); ++pos)
    {
        ttStrCpy(pszFilePortion,
                 bFileSection ? cIncSrcFiles.m_lstSrcFiles[pos].c_str() : cIncSrcFiles.m_lstLibFiles[pos].c_str());
        ttConvertToRelative(cwd.c_str(), szPath, cszRelative);
        size_t posAdd;
        posAdd = m_lstSrcIncluded.Add(cszRelative);
        lstAddSrcFiles.Add(pszFile, posAdd);
        if (bFileSection)
            m_lstSrcFiles.addfilename(cszRelative.c_str());
        else
            m_lstLibFiles.addfilename(cszRelative.c_str());
    }
}

void CSrcFiles::AddSourcePattern(std::string_view FilePattern)
{
    if (FilePattern.empty())
        return;

    ttEnumStr enumPattern(FilePattern, ';');
    for (auto pattern: enumPattern)
    {
        ttlib::winff ff(pattern);
        while (ff.isvalid())
        {
            auto name = ff.GetFileName();
            if (name.hasExtension(".c") || name.hasExtension(".cpp") || name.hasExtension(".cxx"))
            {
                m_lstSrcFiles += name;
            }
            else if (name.hasExtension(".rc"))
            {
                m_lstSrcFiles += name;
                m_RCname = name;
            }
            else if (name.hasExtension(".hhp"))
            {
                m_lstSrcFiles += name;
                m_HPPname = name;
            }
            else if (name.hasExtension(".idl"))
            {
                m_lstSrcFiles += name;
                m_lstIdlFiles += name;
            }

            if (!ff.next())
                break;
        }
    }
}

const char* CSrcFiles::GetPchHeader() const
{
    return hasOptValue(OPT::PCH) ? getOptValue(OPT::PCH).c_str() : nullptr;
}

const char* CSrcFiles::GetPchCpp()
{
    if (hasOptValue(OPT::PCH_CPP) && !isOptValue(OPT::PCH_CPP, "none"))
    {
        m_pchCPPname = getOptValue(OPT::PCH_CPP);
        return m_pchCPPname.c_str();
    }

    if (!GetPchHeader())
        return nullptr;

    m_pchCPPname = GetPchHeader();
    m_pchCPPname.replace_extension(".cpp");
    if (m_pchCPPname.fileExists())
        return m_pchCPPname.c_str();

    // Check for other possible extensions

    m_pchCPPname.replace_extension(".cc");
    if (m_pchCPPname.fileExists())
        return m_pchCPPname.c_str();

    m_pchCPPname.replace_extension(".cxx");
    if (m_pchCPPname.fileExists())
        return m_pchCPPname.c_str();

    m_pchCPPname.replace_extension(".cpp");  // file doesn't exist, we'll generate a warning about it later
    return m_pchCPPname.c_str();
}

const char* CSrcFiles::GetBuildScriptDir()
{
    if (!m_bldFolder.empty())
        return m_bldFolder.c_str();

    if (m_srcfilename.empty())
    {
        m_bldFolder = txtDefBuildDir;
        return m_bldFolder.c_str();
    }

    return m_bldFolder.c_str();
}

const char* CSrcFiles::GetTargetDir()
{
    if (!m_strTargetDir.empty())
        return m_strTargetDir.c_str();

    if (hasOptValue(OPT::TARGET_DIR))
    {
        m_strTargetDir = getOptValue(OPT::TARGET_DIR);
        return m_strTargetDir.c_str();
    }

    ttlib::cstr dir(IsExeTypeLib() ? "../lib" : "../bin");

    // If it's not 32-bit code then just use lib or bin as the target dir if not specified.
    if (hasOptValue(OPT::TARGET_DIR64) || !isOptTrue(OPT::BIT32))
    {
        if (hasOptValue(OPT::TARGET_DIR64))
        {
            m_strTargetDir = getOptValue(OPT::TARGET_DIR64);
            return m_strTargetDir.c_str();
        }
        else if (hasOptValue(OPT::TARGET_DIR))
        {
            m_strTargetDir = getOptValue(OPT::TARGET_DIR);
            return m_strTargetDir.c_str();
        }

        ttlib::cstr cwd;
        cwd.assignCwd();
        bool isSrcDir = ttlib::issameas(cwd.filename(), "src", ttlib::CASE::either) ? true : false;
        if (!isSrcDir)
        {
            cwd.append_filename(IsExeTypeLib() ? "../lib" : "../bin");
            if (cwd.dirExists())
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
            return m_strTargetDir.c_str();
        }
        else if (hasOptValue(OPT::TARGET_DIR))
        {
            m_strTargetDir = getOptValue(OPT::TARGET_DIR);
            return m_strTargetDir.c_str();
        }

        ttlib::cstr cwd;
        cwd.assignCwd();
        bool isSrcDir = ttlib::issameas(cwd.filename(), "src", ttlib::CASE::either) ? true : false;
        if (!isSrcDir)
        {
            cwd.append_filename(IsExeTypeLib() ? "../lib" : "../bin");
            if (cwd.dirExists())
                isSrcDir = true;
        }

        // For 32-bit

        if (isSrcDir)
            dir = (IsExeTypeLib() ? "../lib" : "../bin");
        else
            dir = (IsExeTypeLib() ? "lib" : "bin");

        ttlib::cstr tmp(dir);
        tmp += "32";
        if (tmp.dirExists())
            dir = tmp;
        else
        {
            tmp = (dir + "x86");
            if (tmp.dirExists())
                dir = tmp;
            else
            {
                tmp = (dir + "_x86");
                if (tmp.dirExists())
                    dir = tmp;
            }
        }
    }

    m_strTargetDir = dir;
    return m_strTargetDir.c_str();
}

const char* CSrcFiles::GetTargetRelease()
{
    if (!m_relTarget.empty())
        return m_relTarget.c_str();

    m_relTarget = GetTargetDir();
    m_relTarget.append_filename(GetProjectName());

    if (IsExeTypeLib())
        m_relTarget += ".lib";
    else if (IsExeTypeDll())
        m_relTarget += (getOptValue(OPT::EXE_TYPE).contains("ocx", ttlib::CASE::either) ? ".ocx" : ".dll");
    else
        m_relTarget += ".exe";
    return m_relTarget.c_str();
}

const char* CSrcFiles::GetTargetDebug()
{
    if (!m_dbgTarget.empty())
        return m_dbgTarget.c_str();

    m_dbgTarget = GetTargetDir();
    m_dbgTarget.append_filename(GetProjectName());

    // Never automatically add a 'D' to a dll.
    if (!IsExeTypeDll())
        m_dbgTarget += "D";

    if (IsExeTypeLib())
        m_dbgTarget += ".lib";
    else if (IsExeTypeDll())
        m_dbgTarget += (getOptValue(OPT::EXE_TYPE).contains("ocx", ttlib::CASE::either) ? ".ocx" : ".dll");
    else
        m_dbgTarget += ".exe";
    return m_dbgTarget.c_str();
}

#if !defined(NDEBUG)  // Starts debug section.

void CSrcFiles::AddError(std::string_view err)
{
    ttlib::cstr msg(err);
    msg += "\n";
    m_lstErrMessages.append(msg);
}

#endif

static const char* aProjectLocations[] = {
    // clang-format off
    ".srcfiles.yaml",  // this MUST be the first file
    "src/.srcfiles.yaml",
    "source/.srcfiles.yaml",
    ".private/.srcfiles.yaml",
    "bld/.srcfiles.yaml",
    "build/.srcfiles.yaml",
    // clang-format on
};

ttlib::cstr locateProjectFile(std::string_view StartDir)
{
#if !defined(NDEBUG)  // Starts debug section.
    ttlib::cwd cwd;
#endif
    ttlib::cstr path;
    if (!StartDir.empty())
    {
        for (auto iter: aProjectLocations)
        {
            path.assign(StartDir);
            path.append_filename(iter);
            if (path.fileExists())
            {
                return path;
            }
        }
    }
    else
    {
        for (auto iter: aProjectLocations)
        {
            path.assign(iter);
            if (path.fileExists())
            {
                return path;
            }
        }
    }
    path.clear();
    return path;
}
