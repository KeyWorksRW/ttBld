/////////////////////////////////////////////////////////////////////////////
// Purpose:   Functions for finding executables
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include <cstdlib>

#include <wx/msw/registry.h>
#include <wx/utils.h>

#include <ttmultistr_wx.h>  // multistr -- Breaks a single string into multiple strings
#include <ttstring_wx.h>    // ttString -- wxString with additional methods similar to ttlib::cstr

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
    bool found = false;

#if defined(_WIN32)
    wxRegKey reg(wxRegKey::HKLM, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\devenv.exe");
    reg.Open(wxRegKey::Read);
    if (reg.IsOpened())
    {
        ttString path;
        if (reg.QueryValue("", path))
        {
            Result.ExtractSubString(path.sub_cstr());
            auto pos = Result.locate("Common", 0, tt::CASE::either);
            if (pos != tt::npos)
            {
                Result.erase(pos);
                Result += "VC\\Tools\\MSVC\\*.*";
                ttString name = wxFindFirstFile(Result.wx_str(), wxDIR);

                if (!name.empty())
                {
                    Result.replace_filename(name.sub_cstr());
                    found = true;
                }
            }
        }
        reg.Close();
    }
#endif  // _WIN32

    return found;
}

bool FindVsCode(ttlib::cstr& Result)
{
    bool found = false;

#if defined(_WIN32)
    wxRegKey reg(wxRegKey::HKCR, "Applications\\Code.exe\\shell\\open\\command");
    reg.Open(wxRegKey::Read);
    if (reg.IsOpened())
    {
        ttString path;
        if (reg.QueryValue("", path))
        {
            Result.ExtractSubString(path.sub_cstr());
            Result.remove_filename();
            found = true;
        }
        reg.Close();
    }
#endif  // _WIN32

    return found;
}

bool IsVsCodeAvail()
{
    bool found = false;

#if defined(_WIN32)
    wxRegKey reg(wxRegKey::HKCR, "Applications\\Code.exe\\shell\\open\\command");
    reg.Open(wxRegKey::Read);
    if (reg.IsOpened())
    {
        found = true;
        reg.Close();
    }
#endif  // _WIN32

    return found;
}

bool IsHost64()
{
    return wxIsPlatform64Bit();
}

bool FindFileEnv(const std::string& Env, std::string_view filename, ttlib::cstr& pathResult)
{
    auto pszEnv = std::getenv(Env.c_str());
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
