/////////////////////////////////////////////////////////////////////////////
// Name:      rcdep.cpp
// Purpose:   Contains functions for parsing RC dependencies
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <exception>
#include <sstream>

#include <ttTR.h>        // Function for translating strings
#include <tttextfile.h>  // Classes for reading and writing line-oriented files

#include "ninja.h"  // CNinja

// clang-format off

// list of keywords that load a file
static const char* RcKeywords[]
{
    "BITMP ",
    "CURSOR ",
    "FONT ",
    "HTML ",
    "ICON ",
    "RCDATA ",
};

static const char* aRemovals[]
{
    "PRELOAD",
    "LOADONCALL",
    "FIXED",
    "MOVEABLE",
    "DISCARDABLE",
    "PURE",
    "IMPURE",
    "SHARED",
    "NONSHARED",
};

// clang-format on

// Unlike the various C compilers, the Windows resource compiler does not output any dependent filenames. We parse
// the .rc file looking for #include directives, and add any found as a dependent. If a header file is #included,
// we also parse that to see if it also includes any additional header files.
//
// Note that we only add header files in quotes -- we don't add system files (inside angle brackets) as a
// dependency.
bool CNinja::FindRcDependencies(std::string_view rcfile, std::string_view header, std::string_view relpath)
{
    std::string inFilename{ !header.empty() ? header : rcfile };
    ttlib::viewfile file;
    try
    {
        if (!file.ReadFile(inFilename))
        {
            std::string msg(_tt("Cannot open "));
            msg += inFilename;
            m_lstErrMessages.append(msg);
            return false;
        }
    }
    catch (const std::exception& e)
    {
        std::stringstream msg;
        msg << _tt("An exception occurred while reading ") << inFilename << ": " << e.what();
        m_lstErrMessages.append(msg.str());
        return false;
    }

    ttlib::cstr reldir;
    if (!relpath.empty())
        reldir = relpath;
    else
    {
        reldir = inFilename;
        reldir.remove_filename();
    }

    for (auto line : file)
    {
        line = ttlib::findnonspace(line);
        if (!line.length())
            continue;

        if (ttlib::issameprefix(line, "#include"))
        {
            line = ttlib::stepover(line);
            if (line.empty())
                continue;

            if (line[0] == '\"')
            {
                ttlib::cstr incName;
                incName.ExtractSubString(line);

                // Older versions of Visual Studio do not allow <> to be placed around header files. Since system
                // header files rarely change, and when they do they are not likely to require rebuilding the .rc
                // file, we simply ignore them.

                if (incName.issameprefix("afx") || incName.issameprefix("atl") || incName.issameprefix("winres"))
                    continue;

                ttlib::cstr root{ inFilename };
                root.remove_filename();
                incName.make_relative(root);
                incName.backslashestoforward();

                if (!incName.fileExists())
                {
                    // We can't really report this as an error unless we first check the INCLUDE environment
                    // variable as well as the IncDirs option in .srcfiles.yaml. The resource compiler is going to
                    // report the error, so there's not a huge advantage to reporting it here.
                    continue;
                }

                size_t posHdr = m_lstRcDependencies.GetPos(incName.c_str());
                bool HdrSeenBefore = (posHdr != ttlib::npos);
                if (!HdrSeenBefore)
                    m_lstRcDependencies.Add(incName.c_str());

                if (!HdrSeenBefore)
                {
                    // Search the header file for any #includes it might have -- those will also be dependents
                    FindRcDependencies(rcfile, incName, relpath);
                }
            }
        }
        else
        {
            // Check the RC keywords that can include files
            for (auto keyword : RcKeywords)
            {
                auto posKeyword = ttlib::findstr_pos(line, keyword);
                if (posKeyword != ttlib::npos)
                {
                    // Make certain the keyword starts with whitespace. The RcKeywords list includes the trailing
                    // space so we don't need to check for that.
                    if (posKeyword > 0 && !ttlib::iswhitespace(line[posKeyword - 1]))
                        continue;

                    auto filename = ttlib::stepover(ttlib::findstr(line, keyword));

                    // Old resource files may have some old 16-bit declarations before the filename.
                    for (auto skip : aRemovals)
                    {
                        if (ttlib::issameprefix(filename, skip))
                        {
                            filename = ttlib::stepover(filename);
                        }
                    }

                    // Some keywords such as FONT are also used in DIALOGs and don't actually load a file. By only
                    // looking at names within quotes and even then only processing it if the file actually exists,
                    // we avoid misinterpreting a DIALOG directive versus something that actually includes a file.
                    if (!filename.empty() && filename[0] == '\"')
                    {
                        ttlib::cstr parseName;
                        parseName.ExtractSubString(filename);
                        if (!parseName.empty() && parseName.fileExists())
                        {
                            ttlib::cstr root{ inFilename };
                            root.remove_filename();
                            parseName.make_relative(root);
                            parseName.backslashestoforward();

                            m_lstRcDependencies.Add(parseName.c_str());
                        }
                    }
                }
            }
        }
    }
    return true;
}
