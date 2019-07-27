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
                            cszPath += ff;
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
            return true;
        }
    }
    return false;
}

bool IsHost64()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (si.dwProcessorType == PROCESSOR_ARCHITECTURE_AMD64);
}

#endif

bool FindFileEnv(const char* pszEnv, const char* pszExe, ttCStr& cszPath)
{
    ttASSERT_MSG(pszEnv, "NULL pointer!");
    ttASSERT_MSG(pszExe, "NULL pointer!");

    ttCStr cszEnv;
    size_t cbEnv = 0;
    if (getenv_s(&cbEnv, nullptr, 0, pszEnv) == 0 && cbEnv > 0)
    {
        cszEnv.ReSize(cbEnv + 1);
        if (getenv_s(&cbEnv, cszEnv.GetPtr(), cbEnv, pszEnv) == 0)
        {
            ttCEnumStr enumLib(cszEnv, ';');
            while (enumLib.Enum())
            {
                cszPath = enumLib;
                cszPath.AppendFileName(pszExe);
                if (ttFileExists(cszPath))
                    return true;
            }
        }
    }
    return false;
}
