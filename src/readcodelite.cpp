///////////////////////////////////////////////////////////////////////////////////////////
// Name:      ConvertCodeLite
// Purpose:   Class for converting a CodeLite .project file to .srcfiles.yaml
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include <string>

#include "ttcwd.h"       // cwd -- Class for storing and optionally restoring the current directory
#include "ttmultistr.h"  // multistr -- Breaks a single string into multiple strings

#include "convert.h"  // CConvert, CVcxWrite

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
        ttlib::cstr msg;
        ttlib::MsgBox(msg.Format("Unable to read %s.\n\n%s", m_srcFile.c_str(), result.description()));
        return bld::RESULT::read_failed;
    }

    auto Directories = m_xmldoc.select_nodes("/CodeLite_Project/VirtualDirectory/[@Name]");

    for (size_t pos = 0; pos < Directories.size(); ++pos)
    {
        if (Directories[pos].node().cname().is_sameas("src"))
        {
            auto file = Directories[pos].node().first_child();
            do
            {
                auto filename = file.attribute("Name").as_cstr();
                MakeNameRelative(filename);
                m_writefile.GetSrcFileList().addfilename(filename);
                file = file.next_sibling();
            } while (file);
        }
    }

    auto root = m_xmldoc.child("CodeLite_Project");
    auto settings = root.child("Settings");
    if (settings)
    {
        if (settings.attribute("Type").cvalue().is_sameas("Dynamic Library"))
            m_writefile.setOptValue(OPT::EXE_TYPE, "dll");
        else if (settings.attribute("Type").cvalue().is_sameas("Static Library"))
            m_writefile.setOptValue(OPT::EXE_TYPE, "lib");

        auto Definitions = m_xmldoc.select_nodes("/CodeLite_Project/Settings/GlobalSettings/Compiler/Preprocessor[@Value]");
        ttlib::cstr defs;
        for (size_t pos = 0; pos < Definitions.size(); ++pos)
        {
            defs += "-D" + Definitions[pos].attribute().as_cstr() + " ";
        }
        defs.trim();
        m_writefile.setOptValue(OPT::CFLAGS_CMN, defs);

        auto Includes = m_xmldoc.select_nodes("/CodeLite_Project/Settings/Configuration/Compiler/IncludePath[@Value]");

        ttlib::cstr incs;
        for (size_t pos = 0; pos < Includes.size(); ++pos)
        {
            if (!incs.empty())
                incs += ";";
            incs += Includes[pos].attribute().cvalue();
        }
    }

    return m_writefile.WriteNew(dstFile);
}
