/////////////////////////////////////////////////////////////////////////////
// Name:      CConvert
// Purpose:   Class for converting a Visual Studio .DSP file to .srcfiles.yaml
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttcwd.h>       // cwd -- Class for storing and optionally restoring the current directory
#include <ttenumstr.h>   // ttlib::enumstr, ttEnumView -- Enumerate through substrings in a string
#include <tttextfile.h>  // textfile -- Classes for reading and writing line-oriented files

#include "convert.h"  // CConvert, CVcxWrite

enum
{
    GROUP_UNKNOWN,
    GROUP_SRC,
    GROUP_HEADER,
    GROUP_RESOURCE,
};

bld::RESULT CConvert::ConvertDsp(const std::string& srcFile, std::string_view dstFile)
{
    ttlib::cwd cwd;
    m_srcFile.assign(srcFile);
    m_dstFile.assign(dstFile);
    m_srcFile.make_relative(cwd);
    m_dstFile.make_relative(cwd);

#if defined(_WIN32)
    m_srcFile.backslashestoforward();
    m_dstFile.backslashestoforward();
#endif  // _WIN32

    m_srcDir = srcFile;
    m_srcDir.make_absolute();
    m_srcDir.remove_filename();

    m_dstDir = dstFile;
    m_dstDir.make_absolute();
    m_dstDir.remove_filename();

    ttlib::textfile fileIn;
    if (!fileIn.ReadFile(srcFile))
    {
        ttlib::MsgBox(_tt("Cannot open ") + srcFile);
        return bld::RESULT::read_failed;
    }

    bool inReleaseSection = false;
    auto Group = GROUP_UNKNOWN;

    for (auto& line: fileIn)
    {
        if (line.viewnonspace().empty())
            continue;

        if (line.issameprefix("CFG", tt::CASE::either))
        {
            auto pos = line.find('=');
            if (pos != tt::npos)
            {
                pos = line.findnonspace(pos + 1);
                auto end = line.findspace(pos);
                std::string project(line.c_str() + pos, end - pos);
                m_writefile.setOptValue(OPT::PROJECT, project);
            }
        }
        else if (line.issameprefix("CFG", tt::CASE::either) && line.contains("$(CFG)"))
        {
            inReleaseSection = line.contains(" Release");
        }
        else if (line[0] == '#')
        {
            if (line.contains("Begin Group"))
            {
                if (line.contains("Source Files"))
                    Group = GROUP_SRC;
                else if (line.contains("Header Files"))
                    Group = GROUP_HEADER;
                else if (line.contains("Resource Files"))
                    Group = GROUP_RESOURCE;
                else
                    Group = GROUP_UNKNOWN;
            }
            else if (line.contains("ADD BASE CPP") || line.contains("ADD CPP"))
            {
                // Since this is a really old project, we ignore any compiler flags -- we just grab the defines
                ttlib::cstr NewFlags;
                ttlib::cstr CurFlags;
                if (m_writefile.hasOptValue(OPT::CFLAGS_CMN))
                    CurFlags = m_writefile.getOptValue(OPT::CFLAGS_CMN);
                if (inReleaseSection && m_writefile.hasOptValue(OPT::CFLAGS_REL))
                {
                    if (!CurFlags.empty())
                        CurFlags += " ";
                    CurFlags += m_writefile.getOptValue(OPT::CFLAGS_REL);
                }
                else if (!inReleaseSection && m_writefile.hasOptValue(OPT::CFLAGS_DBG))
                {
                    if (!CurFlags.empty())
                    {
                        CurFlags += " ";
                        CurFlags += m_writefile.getOptValue(OPT::CFLAGS_DBG);
                    }
                }

                auto pos = line.find("/D");
                if (pos != tt::npos)
                {
                    do
                    {
                        ttlib::cstr def;
                        pos = def.ExtractSubString(line, line.findspace(pos));
                        if (!def.issameprefix("NDEBUG") && !def.issameprefix("_DEBUG") &&
                            !def.issameprefix("_WIN32"))
                        {
                            ttlib::cstr Flag("-D" + def);
                            // If we don't already have the flag, then add it
                            if (!Flag.contains(CurFlags))
                            {
                                if (!NewFlags.empty())
                                    NewFlags += " ";
                                NewFlags += Flag.c_str();
                            }
                        }
                        pos = line.find("/D", pos);
                    } while (pos != tt::npos);
                }
                CurFlags.clear();
                if (!NewFlags.empty())
                {
                    if (inReleaseSection && m_writefile.hasOptValue(OPT::CFLAGS_REL))
                    {
                        CurFlags = m_writefile.getOptValue(OPT::CFLAGS_REL);
                        CurFlags += " ";
                    }
                    CurFlags += NewFlags.c_str();
                    m_writefile.setOptValue(OPT::CFLAGS_REL, CurFlags);
                }
            }
        }
        else if (Group == GROUP_SRC && line.issameas("SOURCE"))
        {
            auto pos = line.find('=');
            if (pos != tt::npos)
            {
                pos = line.findnonspace(pos + 1);
                ttlib::cstr filename(line.c_str() + pos);
                MakeNameRelative(filename);
                m_writefile.GetSrcFileList().append(filename);
            }
        }
    }

    return m_writefile.WriteNew(dstFile);
}
