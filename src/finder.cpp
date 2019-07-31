/////////////////////////////////////////////////////////////////////////////
// Name:      finder.cpp
// Purpose:   Functions for finding executables
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#if defined(_WIN32)

#include <ttreg.h>      // ttCRegistry
#include <ttstr.h>      // ttCStr
#include <ttfindfile.h> // ttCFindFile
#include <ttenumstr.h>  // ttCEnumStr

/*
    The path to the MSVC compiler changes every time a new version is downloaded, no matter how minor a change that
    version may be. At the time this code is being written, those updates occur as often as once a week. That forces you
    to either hand-edit your PATH every week or so, or to run one of the MS batch files (which stopped working on Windows
    7 with the 16.2.0 release).

    Unlike the compiler and related tools, the path to devenv.exe is stored in the registry, so using that path we can deduce the
    current path to the compiler, linker, along with the library and include files.
*/

bool FindCurMsvcPath(ttCStr& cszPath)
{
    ttCRegistry reg;
    if (reg.Open(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\devenv.exe", false))
    {
        char szPath[MAX_PATH];
        if (reg.ReadString("", szPath, sizeof(szPath)))
        {
            cszPath.GetQuotedString(szPath);
            char* psz = ttStrStrI(cszPath, "Common");
            if (psz)
            {
                *psz = 0;
                cszPath += "VC\\Tools\\MSVC\\*.*";

                ttCFindFile ff(cszPath);
                if (ff.IsValid()) {
                    do {
                        if (ff.IsDir() && ttIsValidFileChar(ff, 0))
                        {
                            psz = ttStrChr(cszPath, '*');
                            *psz = 0;
                            cszPath += (const char*) ff;
                            return true;
                        }
                    } while(ff.NextFile());
                }
            }
        }
    }

    return false;
}

bool FindVsCode(ttCStr& cszPath)
{
    ttCRegistry reg;
    if (reg.Open(HKEY_CLASSES_ROOT, "Applications\\Code.exe\\shell\\open\\command", false))
    {
        char szPath[MAX_PATH];
        if (reg.ReadString("", szPath, sizeof(szPath)))
        {
            cszPath.GetQuotedString(szPath);
            char* pszExe = ttFindFilePortion(cszPath);
            if (pszExe)
                *pszExe = 0;    // remove the filename
            return true;
        }
    }
    return false;
}

bool IsHost64()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (si.dwProcessorType == PROCESSOR_AMD_X8664 || si.dwProcessorType == PROCESSOR_INTEL_IA64);
}

#endif

bool FindFileEnv(const char* pszEnv, const char* pszFile, ttCStr* pcszPath)
{
    ttASSERT_MSG(pszEnv, "NULL pointer!");
    ttASSERT_MSG(pszFile, "NULL pointer!");

    ttCStr cszEnv, cszPath;
    if (!pcszPath)
        pcszPath = &cszPath;
    size_t cbEnv = 0;
    if (getenv_s(&cbEnv, nullptr, 0, pszEnv) == 0 && cbEnv > 0)
    {
        cszEnv.ReSize(cbEnv + 1);
        if (getenv_s(&cbEnv, cszEnv.GetPtr(), cbEnv, pszEnv) == 0)
        {
            ttCEnumStr enumLib(cszEnv, ';');
            while (enumLib.Enum())
            {
                *pcszPath = enumLib;
                pcszPath->AppendFileName(pszFile);
                if (ttFileExists(*pcszPath))
                    return true;
            }
        }
    }
    return false;
}

static const char* aSrcFilesLocations[] =
{
    ".srcfiles.yaml",           // this MUST be the first file
    ".vscode/srcfiles.yaml",
    ".private/.srcfiles.yaml",
    "build/.srcfiles.yaml",
    "bld/.srcfiles.yaml",

    // the following is here for backwards compatability
    ".srcfiles",

    nullptr
};

const char* LocateSrcFiles(ttCStr* pcszStartDir)
{
    if (pcszStartDir)
    {
        ttCStr cszPath;
        for (size_t pos = 0; aSrcFilesLocations[pos]; ++pos)
        {
            cszPath = *pcszStartDir;
            cszPath.AppendFileName(aSrcFilesLocations[pos]);
            if (ttFileExists(cszPath))
            {
                *pcszStartDir = (const char*) cszPath;
                return *pcszStartDir;
            }
        }
    }
    else
    {
        for (size_t pos = 0; aSrcFilesLocations[pos]; ++pos)
        {
            if (ttFileExists(aSrcFilesLocations[pos]))
                return aSrcFilesLocations[pos];
        }
    }

    return nullptr;
}
