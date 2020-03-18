/////////////////////////////////////////////////////////////////////////////
// Name:      CWriteSrcFiles
// Purpose:   Writes a new or update srcfiles.yaml file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   MIT License (see %lic_name%)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <tttextfile.h>  // textfile -- Classes for reading and writing line-oriented files

#include "writesrc.h"  // CWriteSrcFiles

CWriteSrcFiles::CWriteSrcFiles() {}

bld::RESULT CWriteSrcFiles::UpdateOptions(std::string_view filename)
{
    if (!filename.empty())
        m_outFilename = filename;

    ttlib::viewfile orgFile;
    if (!orgFile.ReadFile(filename))
    {
        return bld::read_failed;
    }

    ttlib::textfile out;
    size_t pos = 0;
    do
    {
        auto& line = out.emplace_back(orgFile[pos]);
        if (line.issameprefix("Options:", tt::CASE::either))
            break;
    } while (++pos < orgFile.size());

    if (pos >= orgFile.size())
        return bld::invalid_file;

    std::vector<size_t> orgOptions;  // vector of options previously used

    for (++pos; pos < orgFile.size(); ++pos)
    {
        auto posColon = orgFile[pos].find(':');
        if (posColon == ttlib::cstr::npos)
            posColon = orgFile[pos].find('=');
        if (posColon == ttlib::cstr::npos)
        {
            out.emplace_back(orgFile[pos]);
            continue;
        }

        auto posBegin = ttlib::findnonspace_pos(orgFile[pos]);
        ttlib::cstr name { orgFile[pos].substr(posBegin, posColon - posBegin) };
        auto option = FindOption(name);
        if (option == OPT::LAST)
        {
            // REVIEW: [KeyWorks - 03-17-2020] This option is invalid. We should do something besides write it out
            // again...
            out.emplace_back(orgFile[pos]);
            continue;
        }
        auto& line = out.addEmptyLine();
        line.assign("    " + name + ":");
        while (line.length() < 22)
            line.push_back(' ');
        line += getOptValue(option);
        while (line.length() < 30)
            line.push_back(' ');
        line += (" # " + getOptComment(option));
        orgOptions.emplace_back(option);
    }

    return bld::failure;
}

bld::RESULT CWriteSrcFiles::WriteNew(std::string_view filename, std::string_view comment)
{
    if (!filename.empty())
        m_outFilename = filename;
    if (!comment.empty())
        m_fileComment = comment;

    return bld::failure;
}
