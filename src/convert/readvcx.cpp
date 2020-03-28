/////////////////////////////////////////////////////////////////////////////
// Name:      CConvert
// Purpose:   Class for converting a Visual Studio Build file to .srcfiles.yaml
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttcwd.h>      // cwd -- Class for storing and optionally restoring the current directory
#include <ttenumstr.h>  // ttlib::enumstr, ttEnumView -- Enumerate through substrings in a string

#include "convert.h"  // CConvert, CVcxWrite

bld::RESULT CConvert::ConvertVcx(const std::string& srcFile, std::string_view dstFile)
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

    std::wstring str16;
    ttlib::utf8to16(srcFile, str16);
    // auto result = m_xmldoc.load_file(str16.c_str());
    auto result = m_xmldoc.load_file(srcFile.c_str());

    m_writefile.InitOptions();

    if (!result)
    {
        ttlib::cstr msg;
        ttlib::MsgBox(msg.Format(_tt("Unable to read %s.\n\n%s"), m_srcFile.c_str(), result.description()));
        return bld::RESULT::read_failed;
    }

    auto files = m_xmldoc.select_nodes("/Project/ItemGroup/ClCompile[@Include]");

    for (size_t pos = 0; pos < files.size(); ++pos)
    {
        auto name = files[pos].node().first_attribute().value();
        if (name)
        {
            // The filename will be relative to the location of the xml file, so first we need to make it relative
            // to that. Since the .srcfiles may be in a different location, we then need to make the file relative
            // to that.

            ttlib::cstr filename(name);
            MakeNameRelative(filename);
            m_writefile.GetSrcFileList().addfilename(filename);
        }
    }

    // All Debug| sections are shared, so only need to process one of them.
    bool DebugProcessed = false;  // Same as Debug|, only one gets processed
    bool ReleaseProcessed = false;

    auto configs = m_xmldoc.select_nodes("/Project/ItemDefinitionGroup[@Condition]");
    for (size_t pos = 0; pos < configs.size(); ++pos)
    {
        auto condition = configs[pos].node().first_attribute().value();
        if (!condition)
            continue;  // theoretically impossible
        if (ttlib::contains(condition, "Debug|"))
        {
            if (DebugProcessed)
                continue;
            ProcessVcxDebug(configs[pos].node());
            DebugProcessed = true;
        }
        else if (ttlib::contains(condition, "Release|"))
        {
            if (ReleaseProcessed)
                continue;
            ProcessVcxRelease(configs[pos].node());
            ReleaseProcessed = true;
        }
    }

    // If debug and release libraries are identical, switch them to shared libraries
    if (m_writefile.hasOptValue(OPT::LIBS_DBG) && m_writefile.hasOptValue(OPT::LIBS_REL) &&
        m_writefile.getOptValue(OPT::LIBS_DBG) == m_writefile.getOptValue(OPT::LIBS_REL))
    {
        m_writefile.setOptValue(OPT::LIBS_CMN, m_writefile.getOptValue(OPT::LIBS_DBG));
        m_writefile.setOptValue(OPT::LIBS_DBG, {});
        m_writefile.setOptValue(OPT::LIBS_REL, {});
    }

    return m_writefile.WriteNew(dstFile);
}

void CConvert::MakeNameRelative(ttlib::cstr& filename)
{
    filename.make_absolute();
    filename.make_relative(m_srcDir);
    filename.make_absolute();
    filename.make_relative(m_dstDir);
    filename.backslashestoforward();
}

void CConvert::ProcessVcxDebug(pugi::xml_node node)
{
    auto compile = node.child("ClCompile");
    if (compile)
    {
        auto val = compile.child("PrecompiledHeader").first_child().value();
        if (val && !ttlib::issameprefix(val, "NotUsing", tt::CASE::either))
            m_writefile.setOptValue(OPT::PCH, val);

        val = compile.child("WarningLevel").first_child().value();
        if (val)
        {
            while (*val && !ttlib::isdigit(*val))
                ++val;
            m_writefile.setOptValue(OPT::WARN, val);
        }

        val = compile.child("AdditionalIncludeDirectories").first_child().value();
        if (val)
        {
            ttlib::cstr incs(val);
            incs.Replace(";%(AdditionalIncludeDirectories)", "");

            ttlib::enumstr enumPaths(incs);
            ttlib::cstr Includes;
            for (auto& filename: enumPaths)
            {
                MakeNameRelative(filename);
                if (!Includes.empty())
                    Includes += ";";
                Includes += filename;
            }
            if (!Includes.empty())
                m_writefile.setOptValue(OPT::INC_DIRS, Includes);
        }

        val = compile.child("PreprocessorDefinitions").first_child().value();
        if (val)
        {
            ttlib::cstr Flags("-D");
            Flags += val;
            Flags.Replace("_DEBUG;", "");
            Flags.Replace("WIN32;", "");
            Flags.Replace("WINDOWS;", "");
            Flags.Replace(";%(PreprocessorDefinitions)", "");
            Flags.Replace(";", " -D", true);
            m_writefile.setOptValue(OPT::CFLAGS_DBG, Flags);
        }
    }

    auto link = node.child("Link");
    if (link)
    {
        auto val = link.child("SubSystem").first_child().value();
        if (val)
        {
            if (ttlib::issameprefix(val, "Console", tt::CASE::either))
                m_writefile.setOptValue(OPT::EXE_TYPE, "console");
        }

        val = link.child("AdditionalDependencies").first_child().value();
        if (val)
        {
            ttlib::cstr libs(val);
            libs.Replace(";%(AdditionalDependencies)", "");

            if (!libs.empty())
                m_writefile.setOptValue(OPT::LIBS_DBG, libs);
        }

        val = link.child("AdditionalOptions").first_child().value();
        if (val)
        {
            ttlib::cstr options(val);
            options.Replace("%(AdditionalOptions)", "");
            if (!options.empty())
                m_writefile.setOptValue(OPT::LINK_DBG, options);
        }
    }
}

void CConvert::ProcessVcxRelease(pugi::xml_node node)
{
    auto compile = node.child("ClCompile");
    if (compile)
    {
        auto val = compile.child("FavorSizeOrSpeed").first_child().value();
        if (val && ttlib::issameprefix(val, "Speed", tt::CASE::either))
            m_writefile.setOptValue(OPT::OPTIMIZE, "speed");

        val = compile.child("PreprocessorDefinitions").first_child().value();
        if (val)
        {
            ttlib::cstr Flags("-D");
            Flags += val;
            Flags.Replace("NDEBUG;", "");
            Flags.Replace("WIN32;", "");
            Flags.Replace("WINDOWS;", "");
            Flags.Replace(";%(PreprocessorDefinitions)", "");
            Flags.Replace(";", " -D", true);
            m_writefile.setOptValue(OPT::CFLAGS_DBG, Flags);
        }
    }

    auto link = node.child("Link");
    if (link)
    {
        auto val = link.child("SubSystem").first_child().value();
        if (val)
        {
            if (ttlib::issameprefix(val, "Console", tt::CASE::either))
                m_writefile.setOptValue(OPT::EXE_TYPE, "console");
        }

        val = link.child("AdditionalDependencies").first_child().value();
        if (val)
        {
            ttlib::cstr libs(val);
            libs.Replace(";%(AdditionalDependencies)", "");

            if (!libs.empty())
                m_writefile.setOptValue(OPT::LIBS_REL, libs);
        }

        val = link.child("AdditionalOptions").first_child().value();
        if (val)
        {
            ttlib::cstr options(val);
            options.Replace("%(AdditionalOptions)", "");
            if (!options.empty())
                m_writefile.setOptValue(OPT::LINK_DBG, options);
        }
    }
}
