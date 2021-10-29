/////////////////////////////////////////////////////////////////////////////
// Purpose:   Class for converting a Visual Studio .DSP file to .srcfiles.yaml
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "ttcwd.h"       // cwd -- Class for storing and optionally restoring the current directory
#include "ttmultistr.h"  // multistr -- Breaks a single string into multiple strings
#include "tttextfile.h"  // textfile -- Classes for reading and writing line-oriented files

#include "convert.h"  // CConvert, CVcxWrite
#include "uifuncs.h"  // Miscellaneous functions for displaying UI

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
        appMsgBox("Cannot open " + srcFile);
        return bld::RESULT::read_failed;
    }

    bool inReleaseSection = false;
    auto Group = GROUP_UNKNOWN;

    for (auto& line: fileIn)
    {
        if (line.view_nonspace().empty() || line.is_sameprefix("!MESSAGE"))
            continue;

        if (line.is_sameprefix("CFG", tt::CASE::either))
        {
            auto pos = line.find('=');
            if (pos != tt::npos)
            {
                pos = line.find_nonspace(pos + 1);
                auto end = line.find_space(pos);
                std::string project(line.c_str() + pos, end - pos);
                m_srcfiles.setOptValue(OPT::PROJECT, project);
            }
        }
        else if (line.is_sameprefix("CFG", tt::CASE::either) && line.contains("$(CFG)"))
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
            else if (line.contains("TARGTYPE"))
            {
                if (line.contains("Dynamic-Link Library"))
                    m_srcfiles.setOptValue(OPT::EXE_TYPE, "dll");
            }
            else if (line.contains("# ADD CPP /Yc"))
            {
                if (auto pos = line.find('\"'); ttlib::is_found(pos))
                {
                    ttlib::cstr header;
                    header.AssignSubString(line.subview(pos));
                    if (header.size())
                        m_srcfiles.setOptValue(OPT::PCH, header);

                    // The /Yc flag will be added after the src file that is used to create the precompiled header. At the
                    // very least, we need to remove it from the files list since it will be processed differently. Adding it
                    // as a PCH_CPP may not be necessary if it has the same base name as the precompiled header, but it
                    // doesn't hurt to add it even if they do have the same name.

                    auto& src_files = m_srcfiles.GetSrcFileList();
                    m_srcfiles.setOptValue(OPT::PCH_CPP, src_files.back());
                    src_files.pop_back();
                }
            }
            else if (line.contains("# ADD LINK32"))
            {
                auto pos = line.find("/out:");
                if (ttlib::is_found(pos))
                {
                    if (line.subview().contains(".ocx"))
                        m_srcfiles.setOptValue(OPT::EXE_TYPE, "ocx");
                }
                pos = line.find("/map:");
                if (ttlib::is_found(pos))
                {
                    pos += 5;
                    // Visual Studio normally places all filenames in quotes, so we rely on that initial quote to tell us
                    // where the filename begins and ends.
                    if (line[pos] == '"')
                    {
                        ttlib::cstr filename;
                        filename.AssignSubString(line.subview(pos));
                        ttlib::cstr cur_flags;
                        if (m_srcfiles.hasOptValue(OPT::LINK_REL))
                            cur_flags = m_srcfiles.getOptValue(OPT::LINK_REL);
                        if (!cur_flags.contains("/map:"))
                        {
                            if (cur_flags.size())
                                cur_flags << ' ';
                            cur_flags << "/map:" << '"' << filename << '"';
                            m_srcfiles.setOptValue(OPT::LINK_REL, cur_flags);
                        }
                    }
                }
            }
            else if (line.contains("ADD BASE CPP") || line.contains("ADD CPP"))
            {
                // Since this is a really old project, we ignore any compiler flags -- we just grab the defines
                ttlib::cstr NewFlags;
                ttlib::cstr CurFlags;
                if (m_srcfiles.hasOptValue(OPT::CFLAGS_CMN))
                    CurFlags = m_srcfiles.getOptValue(OPT::CFLAGS_CMN);
                if (inReleaseSection && m_srcfiles.hasOptValue(OPT::CFLAGS_REL))
                {
                    if (CurFlags.size())
                    {
                        CurFlags << ' ';
                    }
                    CurFlags << m_srcfiles.getOptValue(OPT::CFLAGS_REL);
                }
                else if (!inReleaseSection && m_srcfiles.hasOptValue(OPT::CFLAGS_DBG))
                {
                    if (CurFlags.size())
                    {
                        CurFlags << ' ';
                    }
                    CurFlags << m_srcfiles.getOptValue(OPT::CFLAGS_DBG);
                }

                auto pos = line.find("/D");
                if (pos != tt::npos)
                {
                    do
                    {
                        ttlib::cstr def;
                        pos = def.ExtractSubString(line, line.find_space(pos));
                        if (!def.is_sameprefix("NDEBUG") && !def.is_sameprefix("_DEBUG") && !def.is_sameprefix("_WIN32"))
                        {
                            ttlib::cstr Flag("-D" + def);
                            // If we don't already have the flag, then add it
                            if (!Flag.contains(CurFlags))
                            {
                                if (NewFlags.size())
                                {
                                    NewFlags << ' ';
                                }
                                NewFlags << Flag;
                            }
                        }
                        pos = line.find("/D", pos);
                    } while (pos != tt::npos);
                }
                CurFlags.clear();
                if (!NewFlags.empty())
                {
                    if (inReleaseSection && m_srcfiles.hasOptValue(OPT::CFLAGS_REL))
                    {
                        CurFlags << m_srcfiles.getOptValue(OPT::CFLAGS_REL) << ' ';
                    }
                    CurFlags << NewFlags;
                    m_srcfiles.setOptValue(OPT::CFLAGS_REL, CurFlags);
                }
            }
            else if (line.contains("ADD BASE MTL") || line.contains("ADD MTL"))
            {
                ttlib::cstr NewFlags;
                ttlib::cstr CurFlags;
                if (m_srcfiles.hasOptValue(OPT::MIDL_CMN))
                    CurFlags = m_srcfiles.getOptValue(OPT::MIDL_CMN);

                if (line.contains("/mktyplib203", tt::CASE::either) && !CurFlags.contains("/mktyplib203"))
                {
                    if (NewFlags.size())
                    {
                        NewFlags << ' ';
                    }
                    NewFlags << "/mktyplib203";
                }
                if (line.contains("/win32", tt::CASE::either) && !CurFlags.contains("/win32"))
                {
                    if (NewFlags.size())
                    {
                        NewFlags << ' ';
                    }
                    NewFlags << "/win32";
                }

                auto pos = line.find("/D");
                if (pos != tt::npos)
                {
                    do
                    {
                        ttlib::cstr def;
                        pos = def.ExtractSubString(line, line.find_space(pos));
                        if (!def.is_sameprefix("NDEBUG") && !def.is_sameprefix("_DEBUG") && !def.is_sameprefix("_WIN32"))
                        {
                            ttlib::cstr Flag("-D" + def);
                            // If we don't already have the flag, then add it
                            if (!Flag.contains(CurFlags))
                            {
                                if (NewFlags.size())
                                {
                                    NewFlags << ' ';
                                }
                                NewFlags << Flag;
                            }
                        }
                        pos = line.find("/D", pos);
                    } while (pos != tt::npos);
                }

                CurFlags.clear();
                if (!NewFlags.empty())
                {
                    if (m_srcfiles.hasOptValue(OPT::MIDL_CMN))
                    {
                        CurFlags << m_srcfiles.getOptValue(OPT::MIDL_CMN) << ' ';
                    }
                    CurFlags << NewFlags;
                    m_srcfiles.setOptValue(OPT::MIDL_CMN, CurFlags);
                }
            }
        }
        else if (Group == GROUP_SRC && line.is_sameprefix("SOURCE"))
        {
            auto pos = line.find('=');
            if (pos != tt::npos)
            {
                pos = line.find_nonspace(pos + 1);
                ttlib::cstr filename(line.c_str() + pos);
                MakeNameRelative(filename);
                if (filename.extension().is_sameas(".def", tt::CASE::either))
                {
                    ttlib::cstr cur_flags;
                    if (m_srcfiles.hasOptValue(OPT::LINK_REL))
                        cur_flags = m_srcfiles.getOptValue(OPT::LINK_REL);
                    if (!cur_flags.contains("/def:"))
                    {
                        if (cur_flags.size())
                            cur_flags << ' ';
                        cur_flags << "/def:" << '"' << filename << '"';
                        m_srcfiles.setOptValue(OPT::LINK_REL, cur_flags);
                    }
                }
                else
                {
                    ttlib::add_if(m_srcfiles.GetSrcFileList(), filename);
                }
            }
        }
    }

    return (m_CreateSrcFiles ? m_srcfiles.WriteNew(dstFile) : bld::RESULT::success);
}
