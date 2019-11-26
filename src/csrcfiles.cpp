/////////////////////////////////////////////////////////////////////////////
// Name:      CSrcFiles
// Purpose:   Class for reading/writing .srcfiles.yaml (master file used by ttBld.exe to generate build scripts)
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttfindfile.h>  // ttCFindFile
#include <ttenumstr.h>   // ttCEnumStr
#include <ttmem.h>       // ttCMem, ttCTMem
#include <ttcwd.h>       // ttCCwd

#if !defined(NDEBUG)  // Starts debug section.
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
    ,
    // make all ttCList classes use the same sub-heap
    m_lstSrcFiles(m_ttHeap)
    , m_lstLibFiles(m_ttHeap)
    , m_lstIdlFiles(m_ttHeap)
    , m_lstErrors(m_ttHeap)
    , m_lstLibAddSrcFiles(m_ttHeap)
    , m_lstSrcIncluded(m_ttHeap)
{
    m_bRead = false;

    m_lstSrcFiles.SetFlags(ttCList::FLG_URL_STRINGS);
    m_lstLibFiles.SetFlags(ttCList::FLG_URL_STRINGS);
    m_lstErrors.SetFlags(ttCList::FLG_IGNORE_CASE);

    m_RequiredMajor = 1;
    m_RequiredMinor = 0;
    m_RequiredSub = 0;

    if (pszNinjaDir)
        m_cszBldDir = pszNinjaDir;
    else
        m_cszBldDir = txtDefBuildDir;
}

bool CSrcFiles::ReadFile(const char* pszFile)
{
    if (!pszFile)
    {
        pszFile = LocateSrcFiles();
        if (!pszFile)
        {
            ttCStr csz;
            csz.printf(_("Cannot locate the file %s"), ".srcfiles.yaml");
            m_lstErrors += csz;
            return false;  // if we still can't find it, bail
        }
    }

    m_cszSrcFilePath = pszFile;

    if (m_cszBldDir.IsEmpty())
    {
        // pszFile might now point to a sub-directory, so we need to create the .ninja directory under that
        m_cszBldDir = pszFile;
        *(ttFindFilePortion(m_cszBldDir)) = 0;  // remove the file portion
        m_cszBldDir.AppendFileName(txtDefBuildDir);
    }

    ttCFile kfSrcFiles;
    if (!kfSrcFiles.ReadFile(m_cszSrcFilePath))
    {
        ttCStr csz;
        csz.printf(_("Cannot open \"%s\"."), (char*) m_cszSrcFilePath);
        m_lstErrors += csz;

        return false;
    }

    m_bRead = true;

    char*       pszLine;
    SRC_SECTION section = SECTION_UNKNOWN;

    while (kfSrcFiles.ReadLine(&pszLine))
    {
        char* pszBegin = ttFindNonSpace(pszLine);  // ignore any leading spaces
        if (ttIsEmpty(pszBegin) || pszBegin[0] == '#' ||
            (pszBegin[0] == '-' && pszBegin[1] == '-' && pszBegin[2] == '-'))  // ignore empty, comment or divider lines
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
        ttCStr cszProj;
        cszProj.GetCWD();
        char* pszProj = ttFindFilePortion(cszProj);
        if (pszProj && !ttIsSameStrI(pszProj, "src"))
            UpdateOption(OPT_PROJECT, pszProj);
        else
        {
            if (pszProj > cszProj.GetPtr())
                --pszProj;
            *pszProj = 0;
            pszProj = ttFindFilePortion(cszProj);
            if (pszProj)
                UpdateOption(OPT_PROJECT, pszProj);
        }
    }

    // If no Files: were specified, then we still won't have any files to build. Default to every type of C++ source
    // file in the current directory.

    if (m_lstSrcFiles.GetCount() < 1)
    {
#if defined(_WIN32)
        AddSourcePattern("*.cpp;*.cc;*.cxx;*.rc;*.idl;*.hhp");
#else
        AddSourcePattern("*.cpp;*.cc;*.cxx");
#endif
    }

    return true;
}

void CSrcFiles::ProcessOption(char* pszLine)
{
    ttCStr cszName, cszVal, cszComment;

    if (!GetOptionParts(pszLine, cszName, cszVal, cszComment))
        return;

    if (ttIsSameStrI(cszName, "BuildLibs"))
    {
        while (cszVal.ReplaceStr(".lib", ""))
            ;  // we want the target name, not the library filename

        ttCStr     cszCleaned;
        ttCEnumStr cEnumStr(cszVal, ';');
        while (cEnumStr.Enum())
        {
            ttCStr cszLib(cEnumStr);
            cszLib.TrimRight();
            char* pszLast = cszLib.FindLastSlash();
            if (pszLast && !pszLast[1])
                *pszLast = 0;
            if (cszLib.StrLen() > 0)  // don't add an empty string
            {
                if (cszCleaned.IsNonEmpty())
                    cszCleaned += ";";
                cszCleaned += cszLib;
            }
        }
        if (cszCleaned.IsEmpty())
            return;  // ignore it if it's just a blank line
        cszVal = cszCleaned;
    }

    OPT_INDEX opt = UpdateReadOption(cszName, cszVal, cszComment);
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

    // If you need to support reading old options, add the code here to convert them into the new options. You will also
    // need to add code in CWriteSrcFiles::WriteUpdates to prevent writing the line out again.

    if (ttIsSameStrI(cszName, "TargetDirs"))  // first target is 32-bit directory, second is 64-bit directory
    {
        ttCEnumStr cenum(cszVal, ';');
        if (cenum.Enum())
            UpdateOption(OPT_TARGET_DIR32, (char*) cenum);
        if (cenum.Enum())
            UpdateOption(OPT_TARGET_DIR64, (char*) cenum);
        return;
    }
    else if (ttIsSameStrI(cszName, "LinkFlags"))
        UpdateOption(OPT_LINK_CMN, (char*) cszVal);

    ttCStr csz;
    csz.printf(_("%s is an unknown option"), (char*) cszName);
    m_lstErrors += csz;
}

void CSrcFiles::AddCompilerFlag(const char* pszFlag)
{
    if (!GetOption(OPT_CFLAGS_CMN))
        UpdateOption(OPT_CFLAGS_CMN, pszFlag);
    // else append the flag if it hasn't already been added
    else if (!ttStrStrI(GetOption(OPT_CFLAGS_CMN), pszFlag))
    {
        ttCStr csz(GetOption(OPT_CFLAGS_CMN));
        csz += " ";
        csz += pszFlag;
        UpdateOption(OPT_CFLAGS_CMN, (char*) csz);
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
    // default name of "tmplib". The name is also to indicate that this is a temporary library -- it's not designed to
    // be linked to outside of the scope of the current project.

    if (m_cszLibName.IsEmpty())
        m_cszLibName = "tmplib";

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
            ttCStr cszErrMsg;
            cszErrMsg.printf("Cannot locate the file %s.", (char*) pszLibFile);
            m_lstErrors += cszErrMsg;
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

    m_lstSrcFiles += pszFile;
    if (!ttFileExists(pszFile))
    {
        ttCStr cszErrMsg;
#if !defined(NDEBUG)  // Starts debug section.
        cszErrMsg.GetCWD();
#endif
        if (ttIsNonEmpty(m_cszReportPath))
            cszErrMsg.printf("%s: Unable to locate the file %s.", GetReportFilename(), pszFile);
        else
            cszErrMsg.printf(_("Cannot locate the file %s."), (char*) pszFile);
        AddError(cszErrMsg);
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
        m_cszRcName = pszFile;
        return;
    }

    pszExt = ttStrStrI(pszFile, ".hhp");
    if (pszExt)  // ignore .rc2, .resources, etc.
    {
        m_cszHHPName = pszFile;
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

    ttCCwd cszCWD;

    ttCStr cszFullPath(pszFile);
    cszFullPath.FullPathName();

    CSrcFiles cIncSrcFiles;
    cIncSrcFiles.SetReportingFile(cszFullPath);

    {
        ttCStr cszNewDir(cszFullPath);
        char*  pszFilePortion = ttFindFilePortion(cszNewDir);
        if (pszFilePortion)
            *pszFilePortion = 0;
        ttChDir(cszNewDir);
    }

    if (!cIncSrcFiles.ReadFile(cszFullPath))
    {
        ttCStr cszMsg;
        cszMsg.printf(_("%s: unable to locate the file %s."), GetReportFilename(), pszFile);
        m_lstErrors += cszMsg;
        return;
    }

    ttCTMem<char*> szPath(1024);
    ttStrCpy(szPath, 1024, cszFullPath);
    char* pszFilePortion = ttFindFilePortion(szPath);

    ttCStr cszRelative;

    for (size_t pos = 0;
         pos < (bFileSection ? cIncSrcFiles.m_lstSrcFiles.GetCount() : cIncSrcFiles.m_lstLibFiles.GetCount()); ++pos)
    {
        ttStrCpy(pszFilePortion, bFileSection ? cIncSrcFiles.m_lstSrcFiles[pos] : cIncSrcFiles.m_lstLibFiles[pos]);
        ttConvertToRelative(cszCWD, szPath, cszRelative);
        size_t posAdd;
        posAdd = m_lstSrcIncluded.Add(cszRelative);
        lstAddSrcFiles.Add(pszFile, posAdd);
        if (bFileSection)
            m_lstSrcFiles += cszRelative;
        else
            m_lstLibFiles += cszRelative;
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
                    m_lstSrcFiles += ff;
                }
                else if (ttIsSameStrI(psz, ".rc"))
                {
                    m_lstSrcFiles += ff;
                    if (m_cszRcName.IsEmpty())
                        m_cszRcName = ff;
                }
                else if (ttIsSameStrI(psz, ".hhp"))
                {
                    m_lstSrcFiles += ff;
                    if (m_cszHHPName.IsEmpty())
                        m_cszHHPName = ff;
                }
                else if (ttIsSameStrI(psz, ".idl"))
                {
                    m_lstSrcFiles += ff;
                    m_lstIdlFiles += ff;
                }
            }
            if (!ff.NextFile())
                break;
        }
    }
}

// .srcfiles is a YAML file, so the value of the option may be within a single or double quote. That means we can't just
// search for '#' to find the comment, we must first step over any opening/closing quote.

bool CSrcFiles::GetOptionParts(char* pszLine, ttCStr& cszName, ttCStr& cszVal, ttCStr& cszComment)
{
    char* pszVal = strpbrk(pszLine, ":=");
    ttASSERT_MSG(pszVal, "Invalid Option -- missing ':' or '=' character");
    if (!pszVal)
    {
        ttCStr cszTmp;
        cszTmp.printf(_("Option name is missing a ':' (%s)"), pszLine);
        AddError(cszTmp);
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
        m_cszPchCpp = GetOption(OPT_PCH_CPP);
        return m_cszPchCpp;
    }

    if (!GetPchHeader())
        return nullptr;

    m_cszPchCpp = GetPchHeader();
    m_cszPchCpp.ChangeExtension(".cpp");
    if (ttFileExists(m_cszPchCpp))
        return m_cszPchCpp;

    // Check for other possible extensions

    m_cszPchCpp.ChangeExtension(".cc");
    if (ttFileExists(m_cszPchCpp))
        return m_cszPchCpp;

    m_cszPchCpp.ChangeExtension(".cxx");
    if (ttFileExists(m_cszPchCpp))
        return m_cszPchCpp;

    m_cszPchCpp.ChangeExtension(".cpp");  // file doesn't exist, we'll generate a warning about it later
    return m_cszPchCpp;
}

const char* CSrcFiles::GetBuildScriptDir()
{
    if (m_cszBldDir.IsNonEmpty())
        return m_cszBldDir;

    if (m_cszSrcFilePath.IsEmpty())
    {
        m_cszBldDir = txtDefBuildDir;
        return m_cszBldDir;
    }

    return m_cszBldDir;
}

// Don't add the 'D' to the end of DLL's -- it is perfectly viable for a release app to use a debug dll and that won't
// work if the filename has changed. Under MSVC Linker, it will also generate a LNK4070 error if the dll name is
// specified in the matching .def file. The downside, of course, is that without the 'D' only the size of the dll
// indicates if it is a debug or release version.

const char* CSrcFiles::GetTargetDebug32()
{
    if (m_cszTargetDebug32.IsNonEmpty())
        return m_cszTargetDebug32;

    if (!GetDir32())
        return nullptr;

    m_cszTargetDebug32 = GetDir32();

#if 1
    // Issue # 211 (https://github.com/KeyWorksRW/ttBld/issues/211) is to remove platform suffix, making it the
    // caller's responsibility to set different output directory names if needed.

    // We conditionalize the code in case there's a compelling reason later to either append 32/64 or _x86/_x64.

    bool bAddPlatformSuffix = false;

#else
    // If 64-bit and 32-bit target builds are enabled and the target directories are identical, then we need to add a
    // suffix to the target filename to differentiate between the two builds.

    bool bAddPlatformSuffix =
        (GetBoolOption(OPT_64BIT) && GetBoolOption(OPT_32BIT) && ttIsSameStr(GetDir64(), GetDir32())) ? true : false;
#endif

    if (IsExeTypeLib())
    {
        m_cszTargetDebug32.AppendFileName(GetProjectName());
        m_cszTargetDebug32 += (bAddPlatformSuffix || GetBoolOption(OPT_32BIT_SUFFIX)) ? "_x86D.lib" : "D.lib";
    }
    else if (IsExeTypeDll())
    {
        m_cszTargetDebug32.AppendFileName(GetProjectName());
        m_cszTargetDebug32 += (bAddPlatformSuffix || GetBoolOption(OPT_32BIT_SUFFIX)) ? "_x86.dll" : ".dll";
        if (ttStrStrI(GetOption(OPT_EXE_TYPE), "ocx"))
            m_cszTargetDebug32.ReplaceStr(".dll", ".ocx");
    }
    else
    {
        m_cszTargetDebug32.AppendFileName(GetProjectName());
        m_cszTargetDebug32 += (bAddPlatformSuffix || GetBoolOption(OPT_32BIT_SUFFIX)) ? "_x86D.exe" : "D.exe";
    }
    return m_cszTargetDebug32;
}

const char* CSrcFiles::GetTargetRelease32()
{
    if (m_cszTargetRelease32.IsNonEmpty())
        return m_cszTargetRelease32;

    if (!GetDir32())
        return nullptr;

    m_cszTargetRelease32 = GetDir32();

#if 1
    // Issue # 211 (https://github.com/KeyWorksRW/ttBld/issues/211) is to remove platform suffix, making it the
    // caller's responsibility to set different output directory names if needed.

    // We conditionalize the code in case there's a compelling reason later to either append 32/64 or _x86/_x64.

    bool bAddPlatformSuffix = false;

#else

    // If 64-bit and 32-bit target builds are enabled and the target directories are identical, then we need to add a
    // suffix to the target filename to differentiate between the two builds.

    bool bAddPlatformSuffix =
        (GetBoolOption(OPT_64BIT) && GetBoolOption(OPT_32BIT) && ttIsSameStr(GetDir64(), GetDir32())) ? true : false;
#endif
    if (IsExeTypeLib())
    {
        m_cszTargetRelease32.AppendFileName(GetProjectName());
        m_cszTargetRelease32 += (bAddPlatformSuffix || GetBoolOption(OPT_32BIT_SUFFIX)) ? "_x86.lib" : ".lib";
    }
    else if (IsExeTypeDll())
    {
        m_cszTargetRelease32.AppendFileName(GetProjectName());
        m_cszTargetRelease32 += (bAddPlatformSuffix || GetBoolOption(OPT_32BIT_SUFFIX)) ? "_x86.dll" : ".dll";
        if (ttStrStrI(GetOption(OPT_EXE_TYPE), "ocx"))
            m_cszTargetRelease32.ReplaceStr(".dll", ".ocx");
    }
    else
    {
        m_cszTargetRelease32.AppendFileName(GetProjectName());
        m_cszTargetRelease32 += (bAddPlatformSuffix || GetBoolOption(OPT_32BIT_SUFFIX)) ? "_x86.exe" : ".exe";
    }
    return m_cszTargetRelease32;
}

const char* CSrcFiles::GetTargetDebug64()
{
    if (m_cszTargetDebug64.IsNonEmpty())
        return m_cszTargetDebug64;

    ttASSERT_MSG(GetDir64(), "64-bit target must be set in constructor after reading .srcfiles.yaml");
    m_cszTargetDebug64 = GetDir64();

#if 1
    // Issue # 211 (https://github.com/KeyWorksRW/ttBld/issues/211) is to remove platform suffix, making it the
    // caller's responsibility to set different output directory names if needed.

    // We conditionalize the code in case there's a compelling reason later to either append 32/64 or _x86/_x64.

    bool bAddPlatformSuffix = false;

#else

    // If 64-bit and 32-bit target builds are enabled and the target directories are identical, then we need to add a
    // suffix to the target filename to differentiate between the two builds.

    bool bAddPlatformSuffix =
        (GetBoolOption(OPT_64BIT) && GetBoolOption(OPT_32BIT) && ttIsSameStr(GetDir64(), GetDir32())) ? true : false;
#endif

    if (IsExeTypeLib())
    {
        m_cszTargetDebug64.AppendFileName(GetProjectName());
        m_cszTargetDebug64 += (bAddPlatformSuffix || GetBoolOption(OPT_64BIT_SUFFIX)) ? "_x64D.lib" : "D.lib";
    }
    else if (IsExeTypeDll())
    {
        m_cszTargetDebug64.AppendFileName(GetProjectName());
        m_cszTargetDebug64 += (bAddPlatformSuffix || GetBoolOption(OPT_64BIT_SUFFIX)) ? "_x64.dll" : ".dll";
        if (ttStrStrI(GetOption(OPT_EXE_TYPE), "ocx"))
            m_cszTargetDebug64.ReplaceStr(".dll", ".ocx");
    }
    else
    {
        m_cszTargetDebug64.AppendFileName(GetProjectName());
        m_cszTargetDebug64 += (bAddPlatformSuffix || GetBoolOption(OPT_64BIT_SUFFIX)) ? "_x64D.exe" : "D.exe";
    }
    return m_cszTargetDebug64;
}

const char* CSrcFiles::GetTargetRelease64()
{
    if (m_cszTargetRelease64.IsNonEmpty())
        return m_cszTargetRelease64;

    ttASSERT_MSG(GetDir64(), "64-bit target must be set in constructor after reading .srcfiles.yaml");
    m_cszTargetRelease64 = GetDir64();

#if 1
    // Issue # 211 (https://github.com/KeyWorksRW/ttBld/issues/211) is to remove platform suffix, making it the
    // caller's responsibility to set different output directory names if needed.

    // We conditionalize the code in case there's a compelling reason later to either append 32/64 or _x86/_x64.

    bool bAddPlatformSuffix = false;

#else

    // If 64-bit and 32-bit target builds are enabled and the target directories are identical, then we need to add a
    // suffix to the target filename to differentiate between the two builds.

    bool bAddPlatformSuffix =
        (GetBoolOption(OPT_64BIT) && GetBoolOption(OPT_32BIT) && ttIsSameStr(GetDir64(), GetDir32())) ? true : false;
#endif

    if (IsExeTypeLib())
    {
        m_cszTargetRelease64.AppendFileName(GetProjectName());
        m_cszTargetRelease64 += (bAddPlatformSuffix || GetBoolOption(OPT_64BIT_SUFFIX)) ? "_x64.lib" : ".lib";
    }
    else if (IsExeTypeDll())
    {
        m_cszTargetRelease64.AppendFileName(GetProjectName());
        m_cszTargetRelease64 += (bAddPlatformSuffix || GetBoolOption(OPT_64BIT_SUFFIX)) ? "_x64.dll" : ".dll";
        if (ttStrStrI(GetOption(OPT_EXE_TYPE), "ocx"))
            m_cszTargetRelease64.ReplaceStr(".dll", ".ocx");
    }
    else
    {
        m_cszTargetRelease64.AppendFileName(GetProjectName());
        m_cszTargetRelease64 += (bAddPlatformSuffix || GetBoolOption(OPT_64BIT_SUFFIX)) ? "_x64.exe" : ".exe";
    }
    return m_cszTargetRelease64;
}

#if !defined(NDEBUG)  // Starts debug section.
void CSrcFiles::AddError(const char* pszErrMsg)
{
    ttCStr cszMsg(pszErrMsg);
    cszMsg += "\n";
    wxLogDebug((char*) cszMsg);
    m_lstErrors += pszErrMsg;
}
#endif
