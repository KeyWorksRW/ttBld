/////////////////////////////////////////////////////////////////////////////
// Name:      finder.cpp
// Purpose:   Functions for finding executables
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttcstr.h>      // cstr -- Classes for handling zero-terminated char strings.
#include <ttmultistr.h>  // multistr -- Breaks a single string into multiple strings
#include <ttregistry.h>
#include <ttwinff.h>  // winff -- Wrapper around Windows FindFile

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
    ttlib::registry reg;
    if (reg.OpenLocal("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\devenv.exe", KEY_READ))
    {
        auto path = reg.ReadString("");
        if (!path.empty())
        {
            Result.ExtractSubString(path);
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
    ttlib::registry reg;
    if (reg.OpenClasses("Applications\\Code.exe\\shell\\open\\command"))
    {
        auto path = reg.ReadString("");
        if (!path.empty())
        {
            Result.ExtractSubString(path);
            Result.remove_filename();
            return true;
        }
    }
    return false;
}

bool IsVsCodeAvail()
{
    ttlib::registry reg;
    if (reg.OpenClasses("Applications\\Code.exe\\shell\\open\\command"))
    {
        return true;
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
        ttlib::multistr enumstr(pszEnv);
        for (auto& str: enumstr)
        {
            str.append_filename(filename);
            if (str.file_exists())
            {
                pathResult = str;
                pathResult.backslashestoforward();
                return true;
            }
        }
    }
    return false;
}
