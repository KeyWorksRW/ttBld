/////////////////////////////////////////////////////////////////////////////
// Purpose:   Class for converting a Visual Studio .vcproj file to .srcfiles.yaml
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "ttcwd.h"       // cwd -- Class for storing and optionally restoring the current directory
#include "ttmultistr.h"  // multistr -- Breaks a single string into multiple strings

#include "convert.h"  // CConvert, CVcxWrite
#include "uifuncs.h"  // Miscellaneous functions for displaying UI

bld::RESULT CConvert::ConvertVc(const std::string& srcFile, std::string_view dstFile)
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

    if (!result)
    {
        appMsgBox(_tt(strIdCantOpen) + m_srcFile + "\n\n" + result.description());
        return bld::RESULT::read_failed;
    }

    auto files = m_xmldoc.select_nodes("/VisualStudioProject/Files/Filter[@Name]");

    for (size_t pos = 0; pos < files.size(); ++pos)
    {
        auto name = files[pos].node().first_attribute().cvalue();
        if (!name.empty())
        {
            if (ttlib::is_sameprefix(name, "Source Files", tt::CASE::either))
            {
                auto node = files[pos].node();
                for (auto filename: node.child("File"))
                {
                    auto path = filename.attribute("RelativePath").cvalue();
                    if (!path.empty())
                    {
                        ttlib::cstr relative(path);
                        MakeNameRelative(relative);
                        m_writefile.GetSrcFileList().emplace_back(relative);
                    }
                }
            }
            else if (ttlib::is_sameprefix(name, "Resource Files", tt::CASE::either))
            {
                auto node = files[pos].node();
                for (auto filename: node.child("File"))
                {
                    auto path = filename.attribute("RelativePath").cvalue();
                    if (!path.empty())
                    {
                        ttlib::cstr relative(path);
                        if (relative.has_extension(".rc"))
                        {
                            MakeNameRelative(relative);
                            m_writefile.GetSrcFileList().emplace_back(relative);
                            m_writefile.SetRcName(relative);
                        }
                    }
                }
            }
        }
    }

    auto configs = m_xmldoc.select_nodes("/VisualStudioProject/Configurations/Configuration[@Name]");

    // All Debug| sections are shared, so only need to process one of them.
    bool DebugProcessed = false;
    bool ReleaseProcessed = false;  // Same as Debug|, only one gets processed

    for (size_t pos = 0; pos < configs.size(); ++pos)
    {
        auto name = configs[pos].node().first_attribute().cvalue();
        if (!name.empty())
        {
            if (ttlib::contains(name, "Debug|"))
            {
                if (DebugProcessed)
                    continue;
                ProcessVcDebug(configs[pos].node());
                DebugProcessed = true;
            }
            else if (ttlib::contains(name, "Release|"))
            {
                if (ReleaseProcessed)
                    continue;
                ProcessVcRelease(configs[pos].node());
                ReleaseProcessed = true;
                auto type = configs[pos].node().attribute("ConfigurationType").cvalue();
                if (type.is_sameas("1"))
                    m_writefile.setOptValue(OPT::EXE_TYPE, "window");
                else if (type.is_sameas("2"))
                    m_writefile.setOptValue(OPT::EXE_TYPE, "console");
                else if (type.is_sameas("4"))
                    m_writefile.setOptValue(OPT::EXE_TYPE, "dll");
            }
        }
    }

    return m_writefile.WriteNew(dstFile);
}

void CConvert::ProcessVcDebug(pugi::xml_node node)
{
    for (auto tool: node.child("Tool"))
    {
        auto name = tool.attribute("Name").cvalue();
        if (name.is_sameas("VCCLCompilerTool", tt::CASE::either))
        {
            auto val = tool.attribute("PreprocessorDefinitions").cvalue();
            if (!val.empty())
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
    }
}

void CConvert::ProcessVcRelease(pugi::xml_node node)
{
    for (auto tool: node.child("Tool"))
    {
        auto name = tool.attribute("Name").cvalue();
        if (name.is_sameas("VCCLCompilerTool", tt::CASE::either))
        {
            auto val = tool.attribute("PreprocessorDefinitions").cvalue();
            if (!val.empty())
            {
                ttlib::cstr Flags("-D");
                Flags += val;
                Flags.Replace("_DEBUG;", "");
                Flags.Replace("WIN32;", "");
                Flags.Replace("WINDOWS;", "");
                Flags.Replace(";%(PreprocessorDefinitions)", "");
                Flags.Replace(";", " -D", true);
                m_writefile.setOptValue(OPT::CFLAGS_REL, Flags);
            }

            val = tool.attribute("WarningLevel").cvalue();
            if (!val.empty())
            {
                m_writefile.setOptValue(OPT::WARN, val);
            }

            val = tool.attribute("PrecompiledHeaderFile").cvalue();
            if (!val.empty())
            {
                m_writefile.setOptValue(OPT::PCH, val);
            }

            auto incs = tool.attribute("AdditionalIncludeDirectories").as_cstr();
            if (!incs.empty())
            {
                ttlib::multistr enumPaths(incs);
                ttlib::cstr Includes;
                for (auto& filename: enumPaths)
                {
                    MakeNameRelative(filename);
                    if (Includes.size())
                        Includes << ';';
                    Includes << filename;
                }
                if (!Includes.empty())
                    m_writefile.setOptValue(OPT::INC_DIRS, Includes);
            }
        }
        else if (name.is_sameas("VCLinkerTool", tt::CASE::either))
        {
            if (auto val = tool.attribute("AdditionalDependencies").cvalue(); val.size())
            {
                m_writefile.setOptValue(OPT::LIBS_DBG, val);
            }

            auto filename = tool.attribute("OutputFile").as_cstr();
            if (filename.size() && m_writefile.GetProjectName().empty())
            {
                filename.remove_extension();
                m_writefile.setOptValue(OPT::PROJECT, filename.filename());
            }
        }
    }
}
