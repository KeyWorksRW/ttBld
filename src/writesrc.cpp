/////////////////////////////////////////////////////////////////////////////
// Name:      CWriteSrcFiles
// Purpose:   Writes a new or update srcfiles.yaml file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   MIT License (see %lic_name%)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <array>

#include <tttextfile.h>  // textfile -- Classes for reading and writing line-oriented files

#include "writesrc.h"  // CWriteSrcFiles

CWriteSrcFiles::CWriteSrcFiles() {}

// extern const std::array<OPT::ORIGINAL, OPT::LAST + 1> DefaultOptions;

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
    InitOptions();

    if (!filename.empty())
        m_outFilename = filename;
    if (!comment.empty())
        m_fileComment = comment;

    ttlib::textfile out;

    ttlib::cstr str;
    str.Format(txtNinjaVerFormat, GetMajorRequired(), GetMinorRequired(), GetSubRequired());
    out.emplace_back(str);
    out.addEmptyLine();

    if (!m_fileComment.empty())
    {
        auto pos = m_fileComment.findnonspace();
        if (pos != tt::npos && m_fileComment[pos] != '#')
            m_fileComment.insert(0, "# ");
        out.emplace_back(m_fileComment);
        out.addEmptyLine();
    }

    out.emplace_back("Options:");

    // REVIEW: [KeyWorks - 03-21-2020] These column numbers are placeholders. At some point we need to calculate
    // them.

    size_t colValue = 18;
    size_t colComment = 29;

    // We use the DefaultOptions to determine the order to write the options.

    for (const auto& defOption: DefaultOptions)
    {
        if (defOption.optionID == OPT::LAST)
            break;
        auto& option = m_Options[defOption.optionID];
        if (!option.isRequired)
        {
            // If the option isn't required, then we only output it if it has changed from the original default
            // value.
            if (option.value.empty() || (option.OriginalValue && option.value.issameas(option.OriginalValue)))
                continue;
        }

        auto& line = out.addEmptyLine();

        line.Format("    %s: ", option.OriginalName);
        while (line.length() < colValue)
            line.push_back(' ');
        line.append(option.value);

        if (!option.comment.empty())
        {
            while (line.length() < colComment)
                line.push_back(' ');
            line.append(" # " + option.comment);
        }
    }

    // Now add some blank lines to make the file a bit easier to read.

    auto posOpt = out.FindLineContaining("CFlags_cmn:");
    if (posOpt != tt::npos)
        out.insertEmptyLine(posOpt);

    posOpt = out.FindLineContaining("TargetDir:");
    if (posOpt != tt::npos)
        out.insertEmptyLine(posOpt);

    posOpt = out.FindLineContaining("Natvis:");
    if (posOpt != tt::npos)
        out.insertEmptyLine(posOpt);

    posOpt = out.FindLineContaining("IncDirs:");
    if (posOpt != tt::npos)
        out.insertEmptyLine(posOpt);

    out.addEmptyLine();
    out.emplace_back("Files:");

    if (m_lstSrcFiles.size())
        std::sort(m_lstSrcFiles.begin(), m_lstSrcFiles.end());

    if (!m_RCname.empty())
    {
        // If there is a .rc file, then add it first and remove from srcfiles list so it doesn't get added twice.
        auto posFile = m_lstSrcFiles.find(0, m_RCname);
        if (posFile != tt::npos)
            m_lstSrcFiles.erase(m_lstSrcFiles.begin() + posFile);
        out.emplace_back("    " + m_RCname);
        out.addEmptyLine();
    }

    for (auto& iter: m_lstSrcFiles)
    {
        out.emplace_back("    " + iter);
    }

    return (out.WriteFile(filename) ? bld::success : bld::write_failed);
}
