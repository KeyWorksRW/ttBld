/////////////////////////////////////////////////////////////////////////////
// Purpose:   Class for converting a Visual Studio .vcxproj file to .srcfiles.yaml
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "ttcwd.h"          // cwd -- Class for storing and optionally restoring the current directory
#include <ttmultistr_wx.h>  // multistr -- Breaks a single string into multiple strings
#include <ttstring_wx.h>    // ttString -- wxString with additional methods similar to ttlib::cstr

#include "convert.h"  // CConvert, CVcxWrite
#include "uifuncs.h"  // Miscellaneous functions for displaying UI

bld::RESULT CConvert::ConvertVcx(const std::string& srcFile, std::string_view dstFile)
{
    ttlib::cwd cwd;
    m_srcFile.assign(srcFile);

    if (!m_isConvertToCmake)
    {
        m_dstFile.assign(dstFile);
        m_dstFile.make_relative(cwd);
        m_dstFile.backslashestoforward();
    }
    m_srcFile.make_relative(cwd);

#if defined(_WIN32)
    m_srcFile.backslashestoforward();
#endif  // _WIN32

    m_srcDir = srcFile;
    m_srcDir.make_absolute();
    m_srcDir.remove_filename();

    if (m_dstDir.empty())
    {
        m_dstDir = dstFile;
        m_dstDir.make_absolute();
        m_dstDir.remove_filename();
    }

    std::wstring str16;
    ttlib::utf8to16(srcFile, str16);

    if (auto result = m_xmldoc.load_file(srcFile.c_str()); !result)
    {
        appMsgBox(ttlib::cstr() << "Cannot open " << m_srcFile << "\n\n" << result.description());
        return bld::RESULT::read_failed;
    }

    auto files = m_xmldoc.select_nodes("/Project/ItemGroup/ClCompile[@Include]");

    for (size_t pos = 0; pos < files.size(); ++pos)
    {
        ttlib::cstr filename = files[pos].node().first_attribute().as_string();
        if (filename.size())
        {
            // The filename will be relative to the location of the xml file, so first we need to make it relative to that.
            // Since the .srcfiles may be in a different location, we then need to make the file relative to that.

            MakeNameRelative(filename);
            m_srcfiles.GetSrcFileList().emplace_back(filename);
        }
    }

    // All Debug| sections are shared, so only need to process one of them.
    bool DebugProcessed = false;
    bool ReleaseProcessed = false;  // Same as Debug|, only one gets processed

    auto configs = m_xmldoc.select_nodes("/Project/ItemDefinitionGroup[@Condition]");
    for (size_t pos = 0; pos < configs.size(); ++pos)
    {
        ttlib::cstr condition = configs[pos].node().first_attribute().value();
        if (condition.empty())
            continue;  // theoretically impossible
        if (condition.contains("Debug|"))
        {
            if (DebugProcessed)
                continue;
            ProcessVcxDebug(configs[pos].node());
            DebugProcessed = true;
        }
        else if (condition.contains("Release|"))
        {
            if (ReleaseProcessed)
                continue;
            ProcessVcxRelease(configs[pos].node());
            ReleaseProcessed = true;
        }
    }

    // If debug and release libraries are identical, switch them to shared libraries
    if (m_srcfiles.hasOptValue(OPT::LIBS_DBG) && m_srcfiles.hasOptValue(OPT::LIBS_REL) &&
        m_srcfiles.getOptValue(OPT::LIBS_DBG) == m_srcfiles.getOptValue(OPT::LIBS_REL))
    {
        m_srcfiles.setOptValue(OPT::LIBS_CMN, m_srcfiles.getOptValue(OPT::LIBS_DBG));
        m_srcfiles.setOptValue(OPT::LIBS_DBG, {});
        m_srcfiles.setOptValue(OPT::LIBS_REL, {});
    }

    return (!m_isConvertToCmake ? m_srcfiles.WriteNew(m_dstFile) : bld::RESULT::success);
}

void CConvert::MakeNameRelative(ttlib::cstr& filename)
{
    if (m_dstDir.size())
    {
        ttString ws;
        ws << filename.wx_str();
        ws.make_absolute();
        ws.make_relative(m_dstDir);
        filename = ws.ToUTF8().data();
        filename.backslashestoforward();
        return;
    }

    filename.make_absolute();
    filename.make_relative(m_srcDir);
    if (m_dstDir.size())
    {
        filename.make_absolute();
        filename.make_relative(m_dstDir);
    }
    filename.backslashestoforward();
}

void CConvert::ProcessVcxDebug(pugi::xml_node node)
{
    if (auto compile = node.child("ClCompile"); compile)
    {
        auto val = compile.child("PrecompiledHeaderFile").first_child().value();
        if (val.size())
            m_srcfiles.setOptValue(OPT::PCH, val);

        val = compile.child("WarningLevel").first_child().value();
        if (val.empty())
        {
            while (val.size() && !ttlib::is_digit(val.at(0)))
                val.remove_prefix(1);
            m_srcfiles.setOptValue(OPT::WARN, val);
        }

        val = compile.child("AdditionalIncludeDirectories").first_child().value();
        if (!val.empty())
        {
            ttlib::cstr incs(val);
            incs.Replace(";%(AdditionalIncludeDirectories)", "");

            ttlib::multistr enumPaths(incs);
            ttlib::cstr Includes;
            for (auto& filename: enumPaths)
            {
                MakeNameRelative(filename);
                if (!Includes.empty())
                    Includes << ';';
                Includes << filename;
            }
            if (!Includes.empty())
                m_srcfiles.setOptValue(OPT::INC_DIRS, Includes);
        }

        val = compile.child("PreprocessorDefinitions").first_child().value();
        if (val.empty())
        {
            ttlib::cstr Flags("-D");
            Flags += val;
            Flags.Replace("_DEBUG;", "");
            Flags.Replace("WIN32;", "");
            Flags.Replace("WINDOWS;", "");
            Flags.Replace(";%(PreprocessorDefinitions)", "");
            Flags.Replace(";", " -D", true);
            m_srcfiles.setOptValue(OPT::CFLAGS_DBG, Flags);
        }
    }

    if (auto link = node.child("Link"); link)
    {
        if (ttlib::cstr val = link.child("SubSystem").first_child().value();
            val.size() && val.is_sameprefix("Console", tt::CASE::either))
        {
            m_srcfiles.setOptValue(OPT::EXE_TYPE, "console");
        }

        if (ttlib::cstr libs = link.child("AdditionalDependencies").first_child().value(); libs.size())
        {
            libs.Replace(";%(AdditionalDependencies)", "");

            if (libs.size())
                m_srcfiles.setOptValue(OPT::LIBS_DBG, libs);
        }

        if (ttlib::cstr options = link.child("AdditionalOptions").first_child().value(); options.size())
        {
            options.Replace("%(AdditionalOptions)", "");
            if (options.size())
                m_srcfiles.setOptValue(OPT::LINK_DBG, options);
        }
    }
}

void CConvert::ProcessVcxRelease(pugi::xml_node node)
{
    if (auto compile = node.child("ClCompile"); compile)
    {
        auto val = compile.child("FavorSizeOrSpeed").first_child().value();
        if (ttlib::is_sameprefix(val, "Speed", tt::CASE::either))
            m_srcfiles.setOptValue(OPT::OPTIMIZE, "speed");

        val = compile.child("PreprocessorDefinitions").first_child().value();
        if (!val.empty())
        {
            ttlib::cstr Flags("-D");
            Flags += val;
            Flags.Replace("NDEBUG;", "");
            Flags.Replace("WIN32;", "");
            Flags.Replace("WINDOWS;", "");
            Flags.Replace(";%(PreprocessorDefinitions)", "");
            Flags.Replace(";", " -D", true);
            m_srcfiles.setOptValue(OPT::CFLAGS_DBG, Flags);
        }
    }

    if (auto link = node.child("Link"); link)
    {
        if (ttlib::sview val = link.child("SubSystem").first_child().value();
            val.size() && val.is_sameprefix("Console", tt::CASE::either))
        {
            m_srcfiles.setOptValue(OPT::EXE_TYPE, "console");
        }

        if (ttlib::cstr libs = link.child("AdditionalDependencies").first_child().value(); libs.size())
        {
            libs.Replace(";%(AdditionalDependencies)", "");

            if (libs.size())
                m_srcfiles.setOptValue(OPT::LIBS_REL, libs);
        }

        if (ttlib::cstr options = link.child("AdditionalOptions").first_child().value(); options.size())
        {
            options.Replace("%(AdditionalOptions)", "");
            if (options.size())
                m_srcfiles.setOptValue(OPT::LINK_REL, options);
        }
    }
}
