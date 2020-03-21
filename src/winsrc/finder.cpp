/////////////////////////////////////////////////////////////////////////////
// Name:      finder.cpp
// Purpose:   Functions for finding executables
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttcstr.h>     // cstr -- Classes for handling zero-terminated char strings.
#include <ttenumstr.h>  // enumstr -- Enumerate through substrings in a string
#include <ttreg.h>      // ttCRegistry
#include <ttwinff.h>    // winff -- Wrapper around Windows FindFile

/*
    The path to the MSVC compiler changes every time a new version is downloaded, no matter how minor a change that
    version may be. At the time this code is being written, those updates occur as often as once a week. That
    forces you to either hand-edit your PATH every week or so, or to run one of the MS batch files (which stopped
    working on Windows 7 with the 16.2.0 release).

    Unlike the compiler and related tools, the path to devenv.exe is stored in the registry, so using that path we
    can deduce the current path to the compiler, linker, along with the library and include files.
*/

bool FindCurMsvcPath(ttlib::cstr& Result)
{
    ttCRegistry reg;
    if (reg.Open(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\devenv.exe", false))
    {
        char szPath[MAX_PATH];
        if (reg.ReadString("", szPath, sizeof(szPath)))
        {
            Result.ExtractSubString(szPath);
            auto pos = Result.locate("Common", 0, tt::CASE::either);
            if (pos != tt::npos)
            {
                Result.erase(pos);
                Result += "VC\\Tools\\MSVC\\*.*";

                ttlib::winff ff(Result);
                if (ff.isvalid())
                {
                    do
                    {
                        if (ff.isdir())
                        {
                            Result.replace_filename(ff.getcstr());
                            return true;
                        }
                    } while (ff.next());
                }
            }
        }
    }

    return false;
}

bool FindVsCode(ttlib::cstr& Result)
{
    ttCRegistry reg;
    if (reg.Open(HKEY_CLASSES_ROOT, "Applications\\Code.exe\\shell\\open\\command", false))
    {
        char szPath[MAX_PATH];
        if (reg.ReadString("", szPath, sizeof(szPath)))
        {
            Result.ExtractSubString(szPath);
            Result.remove_filename();
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

bool FindFileEnv(ttlib::cview Env, std::string_view filename, ttlib::cstr& pathResult)
{
    auto pszEnv = std::getenv(Env);
    if (pszEnv)
    {
        ttlib::enumstr enumstr(pszEnv);
        for (auto& str: enumstr)
        {
            str.append_filename(filename);
            if (str.fileExists())
            {
                pathResult = str;
                pathResult.backslashestoforward();
                return true;
            }
        }
    }
    return false;
}

// Specifies various locations to look for a .srcfiles.yaml
static const char* aSrcFilesLocations[] = {
    // clang-format off
    ".srcfiles.yaml",  // this MUST be the first file
    "src/.srcfiles.yaml",
    "source/.srcfiles.yaml",
    ".private/.srcfiles.yaml",
    "bld/.srcfiles.yaml",
    "build/.srcfiles.yaml",

    // the following is here for backwards compatability
    ".srcfiles",

    nullptr
    // clang-format on
};

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

const char* FindProjectFile(ttlib::cstr* pStartDir)
{
    if (pStartDir)
    {
        ttlib::cstr path;
        for (size_t pos = 0; aSrcFilesLocations[pos]; ++pos)
        {
            path = *pStartDir;
            path.append_filename(aSrcFilesLocations[pos]);
            if (path.fileExists())
            {
                *pStartDir = path;
                return pStartDir->c_str();
            }
        }
    }
    else
    {
        for (size_t pos = 0; aSrcFilesLocations[pos]; ++pos)
        {
            if (ttlib::fileExists(aSrcFilesLocations[pos]))
                return aSrcFilesLocations[pos];
        }
    }

    return nullptr;
}
