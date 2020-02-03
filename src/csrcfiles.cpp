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

#include <ttfindfile.h>  // ttCFindFile
#include <ttenumstr.h>   // ttCEnumStr
#include <ttmem.h>       // ttCMem, ttCTMem

#include <ttstring.h>  // ttString, ttCwd, ttStrVector

#if !defined(NDEBUG)  // Starts debug section.
    #include <wx/config.h>
    #include <wx/log.h>
#endif

#include "csrcfiles.h"  // CSrcFiles
#include "funcs.h"      // List of function declarations

const char* txtSrcFilesFileName = ".srcfiles.yaml";
const char* txtDefBuildDir = "bld";

typedef enum
{
    SECTION_UNKNOWN,
    SECTION_OPTIONS,
    SECTION_FILES,
    SECTION_LIB,
} SRC_SECTION;

CSrcFiles::CSrcFiles(const char* pszNinjaDir)
    : m_ttHeap(true)
    // make all ttCList classes use the same sub-heap
    , m_lstLibAddSrcFiles(m_ttHeap)
    , m_lstSrcIncluded(m_ttHeap)
{
    m_bRead = false;

    m_RequiredMajor = 1;
    m_RequiredMinor = 0;
    m_RequiredSub = 0;

    if (pszNinjaDir)
        m_bldFolder = pszNinjaDir;
    else
        m_bldFolder = txtDefBuildDir;

#if !defined(NDEBUG)  // Starts debug section.
    wxConfig config("ttBld");
    config.SetPath("/Settings");
    config.Read("BreakOnWarning", &m_bBreakOnWarning);
#endif
}

bool CSrcFiles::ReadFile(std::string_view filename)
{
    if (filename.empty())
    {
        auto pszFile = LocateSrcFiles();
        if (!pszFile)
        {
            m_lstErrMessages.append(_tt("Cannot locate .srcfiles.yaml"));
            BREAKONWARNING;
            return false;  // if we still can't find it, bail
        }
        m_srcfilename = pszFile;
    }
    else
        m_srcfilename = filename;

    if (m_bldFolder.empty())
    {
        // pszFile might now point to a sub-directory, so we need to create the .ninja directory under that
        m_bldFolder = m_srcfilename;
        m_bldFolder.replace_filename(txtDefBuildDir);
    }

    ttCFile kfSrcFiles;
    if (!kfSrcFiles.ReadFile(m_srcfilename.c_str()))
    {
        std::string msg = _tt("Cannot open ");
        msg += m_srcfilename;
        m_lstErrMessages.append(msg);
        BREAKONWARNING;
        return false;
    }

    m_bRead = true;

    InitOptions();

    char*       pszLine;
    SRC_SECTION section = SECTION_UNKNOWN;

    while (kfSrcFiles.ReadLine(&pszLine))
    {
        char* pszBegin = ttFindNonSpace(pszLine);  // ignore any leading spaces
        if (ttIsEmpty(pszBegin) || pszBegin[0] == '#' ||
            (pszBegin[0] == '-' && pszBegin[1] == '-' &&
             pszBegin[2] == '-'))  // ignore empty, comment or divider lines
        {
            continue;
        }

        if (ttIsSameSubStrI(pszBegin, "%YAML"))  // not required, but possible a YAML editor could add this
        {
            continue;
        }

        if (ttIsAlpha(*pszLine))  // sections always begin with an alphabetical character
        {
            if (ttIsSameSubStrI(pszBegin, "Files:") || ttIsSameSubStrI(pszBegin, "[FILES]"))
                section = SECTION_FILES;
            else if (ttIsSameSubStrI(pszBegin, "Options:") || ttIsSameSubStrI(pszBegin, "[OPTIONS]"))
                section = SECTION_OPTIONS;
            else if (ttIsSameSubStrI(pszBegin, "Lib:"))
                section = SECTION_LIB;
            else
                section = SECTION_UNKNOWN;
            continue;
        }

        switch (section)
        {
            case SECTION_FILES:
                ProcessFile(pszBegin);
                break;

            case SECTION_LIB:
                ProcessLibSection(pszBegin);
                break;

            case SECTION_OPTIONS:
                ProcessOption(pszBegin);
                break;

            case SECTION_UNKNOWN:
            default:
                break;  // ignore it since we don't know what it's supposed to be used for
        }
    }

    // Everything has been processed, if options were not specified that are needed, make some default assumptions

    if (ttIsEmpty(GetProjectName()))
    {
        ttString projectname;
        projectname.assignCwd();
        if (tt::issamestri(projectname.filename(), "src"))
        {
            projectname.replace_filename("");
            // remove trailing slash
            projectname.erase(projectname.length() - 1, 1);
        }
        std::string name(projectname.filename());
        UpdateOption(OPT_PROJECT, name.c_str());
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

void CSrcFiles::ParseOption(std::string_view yamlLine)
{
    ttString name;
    ttString value;
    ttString comment;

    ttString line = tt::findnonspace(yamlLine);
    auto     pos = line.findoneof(":=");
    if (pos == ttString::npos)
    {
        AddError(_tt("Invalid Option -- missing ':' or '=' character"));
        return;
    }

    name.assign(line.substr(0, pos));

    // Ignore or change obsolete options

    if (name.issamestri("64Bit") || name.issamestri("b64_suffix") || name.issamestri("b32_suffix"))
    {
        return;
    }

    pos = line.stepover(pos);
    if (pos == ttString::npos)
    {
        std::stringstream msg;
        msg << _tt("The option ") << name << _tt(" does not have a value");
        AddError(msg.str());
        return;
    }

    if (line[pos] == '"')
    {
        auto posNext = value.ExtractSubString(line, pos);
        if (posNext == ttString::npos)
        {
            std::stringstream msg;
            msg << _tt("The value for ") << name << _tt(" has an opening quote, but no closing quote.");
            AddError(msg.str());
            value.assign(line.substr(pos));
            posNext = pos;
        }
        posNext = line.find('#', posNext + 1);
        if (posNext != ttString::npos)
        {
            // Technically we should check the preceeeding character and determine if it is a backslash. Shouldn't
            // ever occur in .srcfiles.yanml files, but it is allowed in the YAML spec.
            comment.assign(line.substr(line.findnonspace(posNext + 1)));
        }
    }
    else
    {
        auto posComment = line.find('#', pos);
        if (posComment != ttString::npos)
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

    auto& option = FindOption(name);
    if (option.optionID == Opt::LAST)
    {
        std::stringstream msg;
        msg << name << _tt(" is an unrecognized option and will be ignored.");
        AddError(msg.str());
        return;
    }

    option.value = value;
    option.comment = comment;
}

void CSrcFiles::ProcessOption(char* pszLine)
{
    ParseOption(pszLine);

    ttCStr cszName, cszVal, cszComment;

    if (!GetOptionParts(pszLine, cszName, cszVal, cszComment))
        return;

    ttString name(cszName);
    ttString value(cszVal);
    ttString comment(cszComment);

    // Ignore or change obsolete options

    if (name.issamestri("64Bit") || name.issamestri("b64_suffix") || name.issamestri("b32_suffix"))
    {
        return;
    }

    if (name.issamestri("TargetDir64"))
        name = "TargetDir";

    else if (name.issamestri("LibDirs64"))
        name = "LibDirs";

    OPT_INDEX opt = UpdateReadOption(name.c_str(), value.c_str(), comment.c_str());
    if (opt < OPT_OVERFLOW)
    {
        const OPT_VERSION* pVer = GetOptionMinVersion(opt);
        if (pVer)
        {
            if (pVer->major > m_RequiredMajor)
                m_RequiredMajor = pVer->major;
            if (pVer->minor > m_RequiredMinor)
                m_RequiredMinor = pVer->minor;
            if (pVer->sub > m_RequiredSub)
                m_RequiredSub = pVer->sub;
        }
        return;
    }

    // If you need to support reading old options, add the code here to convert them into the new options. You will
    // also need to add code in CWriteSrcFiles::WriteUpdates to prevent writing the line out again.

    if (name.issamestri("LinkFlags"))
    {
        UpdateOption(OPT_LINK_CMN, value.c_str());
        SetOptionValue(Opt::LINK_CMN, value);
    }

    ttString msg(name);
    msg += _tt(" is an unknown option");
    m_lstErrMessages.append(msg);
#if !defined(NDEBUG)  // Starts debug section.
    if (m_bBreakOnWarning)
        wxTrap();
#endif
}

void CSrcFiles::AddCompilerFlag(const char* pszFlag)
{
    if (!GetOption(OPT_CFLAGS_CMN))
        UpdateOption(OPT_CFLAGS_CMN, pszFlag);
    // else append the flag if it hasn't already been added
    else if (!ttStrStrI(GetOption(OPT_CFLAGS_CMN), pszFlag))
    {
        std::stringstream flag;
        flag << GetOption(OPT_CFLAGS_CMN) << ' ' << pszFlag;
        UpdateOption(OPT_CFLAGS_CMN, flag.str().c_str());
    }
}

#if 0
// REVIEW: [KeyWorks - 8/7/2019] doesn't appear to be used
void CSrcFiles::AddLibrary(const char* pszName)
{
    if (!ttStrStrI(GetOption(OPT_LIBS), pszName))
    {
        ttCStr csz(GetOption(OPT_LIBS));
        if (csz.IsNonEmpty())
            csz += " ";
        csz += pszName;
        UpdateOption(OPT_LIBS, (char*) csz);
    }
}
#endif

void CSrcFiles::ProcessLibSection(char* pszLibFile)
{
    // The library is built in the $libout directory, so we don't need to worry about a name conflict -- hence the
    // default name of "tmplib". The name is also to indicate that this is a temporary library -- it's not designed
    // to be linked to outside of the scope of the current project.

    if (m_LIBname.empty())
        m_LIBname = "tmplib";

    if (ttStrStrI(pszLibFile, ".lib"))  // this was used before we created a default name
        return;
    else if (ttIsSameSubStrI(pszLibFile, ".include"))
    {
        char* pszIncFile = ttFindNonSpace(ttFindSpace(pszLibFile));
        ProcessInclude(pszIncFile, m_lstLibAddSrcFiles, false);
    }
    else
    {
        m_lstLibFiles += pszLibFile;
        if (!ttFileExists(pszLibFile))
        {
            ttString str(_tt("Cannot locate the file "));
            str += pszLibFile;
            m_lstErrMessages.append(str);
#if !defined(NDEBUG)  // Starts debug section.
            if (m_bBreakOnWarning)
                wxTrap();
#endif
        }
    }
}

void CSrcFiles::ProcessFile(char* pszFile)
{
    if (ttIsSameSubStrI(pszFile, ".include"))
    {
        const char* pszIncFile = ttFindNonSpace(ttFindSpace(pszFile));
        if (pszIncFile)
        {
            ttCStr cszFile(pszIncFile);
            char*  pszTmp = ttStrChr(cszFile, '#');
            if (pszTmp)
            {
                *pszTmp = 0;
                cszFile.TrimRight();
            }
            ProcessInclude(cszFile, m_lstAddSrcFiles, true);
        }
        return;
    }

    char* pszComment = ttStrChr(pszFile, '#');
    if (pszComment)
    {
        *pszComment = 0;
        ttTrimRight(pszFile);
    }

    if (ttStrChr(pszFile, '*') || ttStrChr(pszFile, '?'))
    {
        AddSourcePattern(pszFile);
        return;
    }

    if (m_lstSrcFiles.addfilename(pszFile))
    {
        if (!m_lstSrcFiles.back().fileExists())
        {
            ttString msg(_tt("Unable to locate the file "));
            msg += pszFile;
            AddError(msg.c_str());
        }
    }

    char* pszExt = ttStrStrI(pszFile, ".idl");
    if (pszExt)
    {
        m_lstIdlFiles += pszFile;
        return;
    }

    pszExt = ttStrStrI(pszFile, ".rc");
    if (pszExt && !pszExt[3])  // ignore .rc2, .resources, etc.
    {
        m_RCname = pszFile;
        return;
    }

    pszExt = ttStrStrI(pszFile, ".hhp");
    if (pszExt)  // ignore .rc2, .resources, etc.
    {
        m_HPPname = pszFile;
        return;
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
            char*  pszTmp = ttStrChr(cszFile, '#');
            if (pszTmp)
            {
                *pszTmp = 0;
                cszFile.TrimRight();
            }
            ProcessInclude(cszFile, m_lstLibAddSrcFiles, false);
        }
        return;
    }

    ttCwd cwd;

    ttCStr cszFullPath(pszFile);
    cszFullPath.FullPathName();

    CSrcFiles cIncSrcFiles;
    cIncSrcFiles.SetReportingFile(cszFullPath);

    try
    {
        ttCStr cszNewDir(cszFullPath);
        char*  pszFilePortion = ttFindFilePortion(cszNewDir);
        if (pszFilePortion)
            *pszFilePortion = 0;
        fs::current_path(cszNewDir.c_str());
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    if (!cIncSrcFiles.ReadFile(cszFullPath.c_str()))
    {
        ttString str(_tt("Unable to locate the file "));
        str += cszFullPath;
        m_lstErrMessages.append(str);
#if !defined(NDEBUG)  // Starts debug section.
        if (m_bBreakOnWarning)
            wxTrap();
#endif
        return;
    }

    ttCTMem<char*> szPath(1024);
    ttStrCpy(szPath, 1024, cszFullPath);
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

void CSrcFiles::AddSourcePattern(const char* pszFilePattern)
{
    if (!pszFilePattern || !*pszFilePattern)
        return;

    ttCStr      cszPattern(pszFilePattern);
    ttCEnumStr  enumstr(cszPattern, ';');
    const char* pszPattern;

    while (enumstr.Enum(&pszPattern))
    {
        ttCFindFile ff(pszPattern);
        while (ff.IsValid())
        {
            char* psz = ttStrChrR(ff, '.');
            if (psz)
            {
                if (ttIsSameStrI(psz, ".c") || ttIsSameStrI(psz, ".cpp") || ttIsSameStrI(psz, ".cc") ||
                    ttIsSameStrI(psz, ".cxx"))
                {
                    m_lstSrcFiles += ff.GetFileName();
                }
                else if (ttIsSameStrI(psz, ".rc"))
                {
                    m_lstSrcFiles += ff.GetFileName();
                    if (m_RCname.empty())
                        m_RCname = ff.GetFileName();
                }
                else if (ttIsSameStrI(psz, ".hhp"))
                {
                    m_lstSrcFiles += ff.GetFileName();
                    if (m_HPPname.empty())
                        m_HPPname = ff.GetFileName();
                }
                else if (ttIsSameStrI(psz, ".idl"))
                {
                    m_lstSrcFiles += ff.GetFileName();
                    m_lstIdlFiles += ff.GetFileName();
                }
            }
            if (!ff.NextFile())
                break;
        }
    }
}

// .srcfiles is a YAML file, so the value of the option may be within a single or double quote. That means we can't
// just search for '#' to find the comment, we must first step over any opening/closing quote.

bool CSrcFiles::GetOptionParts(char* pszLine, ttCStr& cszName, ttCStr& cszVal, ttCStr& cszComment)
{
    char* pszVal = strpbrk(pszLine, ":=");
    if (!pszVal)
    {
        AddError(_tt("Invalid Option -- missing ':' or '=' character"));
        return false;
    }
    *pszVal = 0;
    cszName = pszLine;
    pszVal = ttFindNonSpace(pszVal + 1);

    if (*pszVal == CH_QUOTE || *pszVal == CH_SQUOTE || *pszVal == CH_START_QUOTE)
    {
        cszVal.GetQuotedString(pszVal);
        pszVal += (cszVal.StrLen() + 2);
        char* pszComment = ttStrChr(pszVal + cszVal.StrLen() + 2, '#');
        if (pszComment)
        {
            pszComment = ttStepOver(pszComment);
            ttTrimRight(pszComment);  // remove any trailing whitespace
            cszComment = pszComment;
        }
        else
        {
            cszComment.Delete();
        }
    }
    else  // non-quoted option
    {
        char* pszComment = ttStrChr(pszVal, '#');
        if (pszComment)
        {
            *pszComment = 0;
            pszComment = ttStepOver(pszComment);
            ttTrimRight(pszComment);  // remove any trailing whitespace
            cszComment = pszComment;
        }
        else
        {
            cszComment.Delete();
        }
        ttTrimRight(pszVal);  // remove any trailing whitespace
        cszVal = pszVal;
    }

    return true;
}

const char* CSrcFiles::GetPchHeader()
{
    const char* pszPch = GetOption(OPT_PCH);
    if (pszPch && ttIsSameStrI(pszPch, "none"))
        return nullptr;
    return pszPch;
}

const char* CSrcFiles::GetPchCpp()
{
    const char* pszPch = GetOption(OPT_PCH_CPP);
    if (pszPch && !ttIsSameStrI(pszPch, "none"))
    {
        m_pchCPPname = GetOption(OPT_PCH_CPP);
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

    if (GetOption(OPT_TARGET_DIR))
    {
        m_strTargetDir = GetOption(OPT_TARGET_DIR);
        return m_strTargetDir.c_str();
    }

    ttCStr cszDir(IsExeTypeLib() ? "../lib" : "../bin");

    // If it's not 32-bit code then just use lib or bin as the target dir if not specified.
    if (GetOption(OPT_TARGET_DIR64) || !GetBoolOption(OPT_32BIT))
    {
        if (GetOption(OPT_TARGET_DIR64))
        {
            m_strTargetDir = GetOption(OPT_TARGET_DIR64);
            return m_strTargetDir.c_str();
        }
        else if (GetOption(OPT_TARGET_DIR))
        {
            m_strTargetDir = GetOption(OPT_TARGET_DIR);
            return m_strTargetDir.c_str();
        }

        ttCStr cszCWD;
        cszCWD.GetCWD();
        bool bSrcDir = ttStrStrI(ttFindFilePortion(cszCWD), "src") ? true : false;
        if (!bSrcDir)
        {
            cszCWD.AppendFileName(IsExeTypeLib() ? "../lib" : "../bin");
            if (ttDirExists(cszCWD))
                bSrcDir = true;
        }
        if (bSrcDir)
        {
            cszDir = (IsExeTypeLib() ? "../lib" : "../bin");
        }
        else
        {
            cszDir = (IsExeTypeLib() ? "lib" : "bin");
        }
    }

    // For 32-bit code, check to see if there is a 32, x86, or _x86 suffix to the standard bin and lib directories.
    // If it exists and the user didn't tell us where to put it, then use that directory.
    else
    {
        if (GetOption(OPT_TARGET_DIR32))
        {
            m_strTargetDir = GetOption(OPT_TARGET_DIR32);
            return m_strTargetDir.c_str();
        }
        else if (GetOption(OPT_TARGET_DIR))
        {
            m_strTargetDir = GetOption(OPT_TARGET_DIR);
            return m_strTargetDir.c_str();
        }

        ttCStr cszCWD;
        cszCWD.GetCWD();
        bool bSrcDir = ttStrStrI(ttFindFilePortion(cszCWD), "src") ? true : false;
        if (!bSrcDir)
        {
            cszCWD.AppendFileName(IsExeTypeLib() ? "../lib" : "../bin");
            if (ttDirExists(cszCWD))
                bSrcDir = true;
        }

        // For 32-bit w

        if (bSrcDir)
        {
            cszDir = (IsExeTypeLib() ? "../lib" : "../bin");
            ttCStr cszTmp(cszDir);
            cszTmp += "32";
            if (ttDirExists(cszTmp))
                cszDir = cszTmp;
            else
            {
                char* pszTmp = strstr(cszTmp, "32");
                *pszTmp = 0;
                cszTmp += "x86";
                if (ttDirExists(cszTmp))
                    cszDir = cszTmp;
                else
                {
                    pszTmp = strstr(cszTmp, "x86");
                    *pszTmp = 0;
                    cszTmp += "_x86";
                    if (ttDirExists(cszTmp))
                        cszDir = cszTmp;
                }
            }
        }
        else
        {
            cszDir = (IsExeTypeLib() ? "lib" : "bin");
            ttCStr cszTmp(cszDir);
            cszTmp += "32";
            if (ttDirExists(cszTmp))
                cszDir = cszTmp;
            else
            {
                char* pszTmp = strstr(cszTmp, "32");
                *pszTmp = 0;
                cszTmp += "x86";
                if (ttDirExists(cszTmp))  // if there is a ../lib32 or ../bin32, then use that
                    cszDir = cszTmp;
                else
                {
                    pszTmp = strstr(cszTmp, "x86");
                    *pszTmp = 0;
                    cszTmp += "_x86";
                    if (ttDirExists(cszTmp))
                        cszDir = cszTmp;
                }
            }
        }
    }

    m_strTargetDir = static_cast<const char*>(cszDir);
    return m_strTargetDir.c_str();
}

const char* CSrcFiles::GetTargetRelease()
{
    if (!m_relTargetFolder.empty())
        return m_relTargetFolder.c_str();

    m_relTargetFolder = GetTargetDir();
    m_relTargetFolder.append_filename(GetProjectName());

    if (IsExeTypeLib())
        m_relTargetFolder += ".lib";
    else if (IsExeTypeDll())
        m_relTargetFolder += (ttStrStrI(GetOption(OPT_EXE_TYPE), "ocx") ? ".ocx" : ".dll");
    else
        m_relTargetFolder += ".exe";
    return m_relTargetFolder.c_str();
}

const char* CSrcFiles::GetTargetDebug()
{
    if (!m_dbgTargetFolder.empty())
        return m_dbgTargetFolder.c_str();

    m_dbgTargetFolder = GetTargetDir();
    m_dbgTargetFolder.append_filename(GetProjectName());

    // Never automatically add a 'D' to a dll.
    if (!IsExeTypeDll())
        m_dbgTargetFolder += "D";

    if (IsExeTypeLib())
        m_dbgTargetFolder += ".lib";
    else if (IsExeTypeDll())
        m_dbgTargetFolder += (ttStrStrI(GetOption(OPT_EXE_TYPE), "ocx") ? ".ocx" : ".dll");
    else
        m_dbgTargetFolder += ".exe";
    return m_dbgTargetFolder.c_str();
}

#if !defined(NDEBUG)  // Starts debug section.

void CSrcFiles::AddError(std::string_view err)
{
    ttString msg(err);
    msg += "\n";
    if (m_lstErrMessages.append(msg))
    {
        wxLogDebug(msg.c_str());
        if (m_bBreakOnWarning)
            ttFAIL_MSG(msg.c_str());
    }
}

#endif
