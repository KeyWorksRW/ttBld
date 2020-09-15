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

    // Always write the version of ttBld.exe used. That way if an older version is used and an option line gets commented out, the user
    // might spot that an earlier version of ttBld is being used (if tracked by SCM, it will show up in the diff)

    out.emplace_back(txtNinjaVerFormat);
    out.addEmptyLine();

    size_t pos = 0;
    bool SeenRequiredComment = false;
    do
    {
        // Ignore any previous comment about ttBld since we've already written it.
        if (!SeenRequiredComment)
        {
            if (orgFile[pos].size() && (ttlib::is_sameprefix(orgFile[pos], "# Requires ttBld", tt::CASE::either) ||
                                        ttlib::is_sameprefix(orgFile[pos], "# Updated by ttBld", tt::CASE::either)))
            {
                ++pos;
                if (orgFile[pos].empty())
                    ++pos;
                SeenRequiredComment = true;
            }
        }

        auto& line = out.emplace_back(orgFile[pos]);
        if (line.is_sameprefix("Options:", tt::CASE::either))
            break;
    } while (++pos < orgFile.size());

    if (pos >= orgFile.size())
        return bld::invalid_file;

    std::vector<size_t> orgOptions;  // vector of options previously used

    for (++pos; pos < orgFile.size(); ++pos)
    {
        auto view = ttlib::find_nonspace(orgFile[pos]);
        if (view.empty() || view[0] == '#')
        {
            // write blank or comment lines unmodified
            out.emplace_back(orgFile[pos]);
            continue;
        }

        // the Options: section ends if a new section is encountered
        if (ttlib::is_alpha(orgFile[pos][0]) || ttlib::is_digit(orgFile[pos][0]))
            break;

        auto posColon = view.find(':');
        if (!ttlib::is_found(posColon))
        {
            posColon = view.find('=');
            if (!ttlib::is_found(posColon))
            {
                // not a comment or option, write it unmodified
                out.emplace_back(orgFile[pos]);
                continue;
            }
        }

        ttlib::cstr name { view.substr(0, posColon) };
        auto option = FindOption(name);
        if (option == OPT::LAST)
        {
            auto& line = out.addEmptyLine();
            line.assign("    # unrecognized option -- ");
            line += orgFile[pos];
            continue;
        }
        auto& line = out.addEmptyLine();
        line.assign("    " + name + ":");
        while (line.length() < 22)
            line.push_back(' ');
        line += getOptValue(option);
        while (line.length() < 30)
            line.push_back(' ');
        if (getOptComment(option).size())
            line += (" # " + getOptComment(option));

        // Keep track of every option we've written out. We'll use this outside of this loop to determine what
        // additional options to write.
        orgOptions.emplace_back(option);
    }

    // At this point, all the original options specified have been written out with possible new values. If there are
    // other options that are required or have non-default values, they must be written here.

    bool SomethingChanged = false;
    for (size_t option = 0; option < OPT::LAST; ++option)
    {
        // ignore if we've already written out the option
        if (std::find(orgOptions.begin(), orgOptions.end(), option) != orgOptions.end())
            continue;

        if (isOptionRequired(option) || hasOptionChanged(option))
        {
            SomethingChanged = true;
            auto& line = out.addEmptyLine();
            line.assign("    " + getOptionName(option) + ":");
            while (line.length() < 22)
                line.push_back(' ');
            line += getOptValue(option);
            while (line.length() < 30)
                line.push_back(' ');
            line += (" # " + getOptComment(option));
        }
    }

    if (SomethingChanged)
        out.addEmptyLine();

    // Now that the Options: section has been written, write out the rest of the file without further modifications.

    while (pos < orgFile.size())
    {
        out.emplace_back(orgFile[pos++]);
    }

    if (out.WriteFile(filename))
        return bld::success;

    return bld::write_failed;
}

bld::RESULT CWriteSrcFiles::WriteNew(std::string_view filename, std::string_view comment)
{
    if (!m_Initialized)
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
        auto pos = m_fileComment.find_nonspace();
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
            if (option.value.empty() || (option.OriginalValue && option.value.is_sameas(option.OriginalValue)))
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
