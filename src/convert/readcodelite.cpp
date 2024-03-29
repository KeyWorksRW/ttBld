///////////////////////////////////////////////////////////////////////////////////////////
// Purpose:   Class for converting a CodeLite .project file to .srcfiles.yaml
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "ttcwd.h"          // cwd -- Class for storing and optionally restoring the current directory
#include <ttmultistr_wx.h>  // multistr -- Breaks a single string into multiple strings

#include "convert.h"  // CConvert, CVcxWrite
#include "uifuncs.h"  // Miscellaneous functions for displaying UI

bld::RESULT CConvert::ConvertCodeLite(const std::string& srcFile, std::string_view dstFile)
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
        appMsgBox("Cannot open " + m_srcFile + "\n\n" + result.description());
        return bld::RESULT::read_failed;
    }

    auto Directories = m_xmldoc.select_nodes("/CodeLite_Project/VirtualDirectory/[@Name]");

    for (size_t pos = 0; pos < Directories.size(); ++pos)
    {
        if (Directories[pos].node().name() == "src")
        {
            auto file = Directories[pos].node().first_child();
            do
            {
                ttlib::cstr filename = file.attribute("Name").value();
                MakeNameRelative(filename);
                ttlib::add_if(m_srcfiles.GetSrcFileList(), filename);
                file = file.next_sibling();
            } while (file);
        }
    }

    auto root = m_xmldoc.child("CodeLite_Project");
    auto settings = root.child("Settings");
    if (settings)
    {
        if (settings.attribute("Type").value() == "Dynamic Library")
            m_srcfiles.setOptValue(OPT::EXE_TYPE, "dll");
        else if (settings.attribute("Type").value() == "Static Library")
            m_srcfiles.setOptValue(OPT::EXE_TYPE, "lib");

        auto Definitions = m_xmldoc.select_nodes("/CodeLite_Project/Settings/GlobalSettings/Compiler/Preprocessor[@Value]");
        ttlib::cstr defs;
        for (size_t pos = 0; pos < Definitions.size(); ++pos)
        {
            defs += "-D" + Definitions[pos].attribute().as_std_str() + " ";
        }
        defs.trim();
        m_srcfiles.setOptValue(OPT::CFLAGS_CMN, defs);

        auto Includes = m_xmldoc.select_nodes("/CodeLite_Project/Settings/Configuration/Compiler/IncludePath[@Value]");

        ttlib::cstr incs;
        for (size_t pos = 0; pos < Includes.size(); ++pos)
        {
            if (!incs.empty())
                incs += ";";
            incs += Includes[pos].attribute().value();
        }
    }

    return (m_CreateSrcFiles ? m_srcfiles.WriteNew(dstFile) : bld::RESULT::success);
}
