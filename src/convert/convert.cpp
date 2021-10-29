/////////////////////////////////////////////////////////////////////////////
// Purpose:   Various conversion methods
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

// Assumes that everything except a .DSP file is an xml file and has been parsed into m_xml

#include <filesystem>

#include "ttmultistr.h"  // multistr -- Breaks a single string into multiple strings

#include "convert.h"

#include "../ui/convertdlg.h"  // ConvertDlg -- Dialog specifying what to convert into a .srcfiles.yaml file
#include "funcs.h"             // List of function declarations
#include "ui/optionsdlg.h"     // OptionsDlg -- Dialog for setting all .srcfile options
#include "uifuncs.h"           // Miscellaneous functions for displaying UI

bool MakeNewProject(ttlib::cstr& projectFile)
{
    ConvertDlg dlg(projectFile);
    if (dlg.ShowModal() != wxID_OK)
        return false;

    projectFile = dlg.GetOutSrcFiles();

    OptionsDlg dlgOptions(projectFile);
    if (dlgOptions.ShowModal() == wxID_OK)
        dlgOptions.SaveChanges();

    if (dlg.isCreateVsCode())
    {
        // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
        auto results = CreateVsCodeProject(projectFile);
        for (auto& iter: results)
            std::cout << iter << '\n';
    }

    if (dlg.isAddToGitExclude())
    {
        ttlib::cstr GitExclude;
        if (gitIgnoreAll(GitExclude))
        {
            std::cout << "Added directories and filenames to ignore to " << GitExclude << '\n';
        }
    }

    return true;
}

bld::RESULT CConvert::ConvertSrcfiles(const std::string& srcFile, std::string_view dstFile)
{
    CSrcFiles srcOrg;
    if (!srcOrg.ReadFile(srcFile.c_str()))
    {
        appMsgBox("Cannot open " + srcFile);
        return bld::RESULT::read_failed;
    }

    // First we copy all the options. Then we tweak those that will need to be changed to use a different path.

    m_srcfiles.setOptValue(OPT::PROJECT, srcOrg.getOptValue(OPT::PROJECT));
    m_srcfiles.setOptValue(OPT::EXE_TYPE, srcOrg.getOptValue(OPT::EXE_TYPE));
    m_srcfiles.setOptValue(OPT::PCH, srcOrg.getOptValue(OPT::PCH));

    m_srcfiles.setOptValue(OPT::OPTIMIZE, srcOrg.getOptValue(OPT::OPTIMIZE));
    m_srcfiles.SetRequired(OPT::OPTIMIZE);
    m_srcfiles.setOptValue(OPT::WARN, srcOrg.getOptValue(OPT::WARN));
    m_srcfiles.SetRequired(OPT::WARN);

    m_srcfiles.setOptValue(OPT::CFLAGS_CMN, srcOrg.getOptValue(OPT::CFLAGS_CMN));
    m_srcfiles.setOptValue(OPT::CFLAGS_REL, srcOrg.getOptValue(OPT::CFLAGS_REL));
    m_srcfiles.setOptValue(OPT::CFLAGS_DBG, srcOrg.getOptValue(OPT::CFLAGS_DBG));

    m_srcfiles.setOptValue(OPT::MSVC_CMN, srcOrg.getOptValue(OPT::MSVC_CMN));
    m_srcfiles.setOptValue(OPT::MSVC_REL, srcOrg.getOptValue(OPT::MSVC_REL));
    m_srcfiles.setOptValue(OPT::MSVC_DBG, srcOrg.getOptValue(OPT::MSVC_DBG));

    m_srcfiles.setOptValue(OPT::CLANG_CMN, srcOrg.getOptValue(OPT::CLANG_CMN));
    m_srcfiles.setOptValue(OPT::CLANG_REL, srcOrg.getOptValue(OPT::CLANG_REL));
    m_srcfiles.setOptValue(OPT::CLANG_DBG, srcOrg.getOptValue(OPT::CLANG_DBG));

    m_srcfiles.setOptValue(OPT::LINK_CMN, srcOrg.getOptValue(OPT::LINK_CMN));
    m_srcfiles.setOptValue(OPT::LINK_REL, srcOrg.getOptValue(OPT::LINK_REL));
    m_srcfiles.setOptValue(OPT::LINK_DBG, srcOrg.getOptValue(OPT::LINK_DBG));

#if defined(_WIN32)
    m_srcfiles.setOptValue(OPT::NATVIS, srcOrg.getOptValue(OPT::NATVIS));

    m_srcfiles.setOptValue(OPT::RC_CMN, srcOrg.getOptValue(OPT::RC_CMN));
    m_srcfiles.setOptValue(OPT::RC_REL, srcOrg.getOptValue(OPT::RC_REL));
    m_srcfiles.setOptValue(OPT::RC_DBG, srcOrg.getOptValue(OPT::RC_DBG));

    m_srcfiles.setOptValue(OPT::MIDL_CMN, srcOrg.getOptValue(OPT::MIDL_CMN));
    m_srcfiles.setOptValue(OPT::MIDL_REL, srcOrg.getOptValue(OPT::MIDL_REL));
    m_srcfiles.setOptValue(OPT::MIDL_DBG, srcOrg.getOptValue(OPT::MIDL_DBG));

    m_srcfiles.setOptValue(OPT::MS_LINKER, srcOrg.getOptValue(OPT::MS_LINKER));

#endif  // defined(_WIN32)

    m_srcfiles.setOptValue(OPT::CRT_REL, srcOrg.getOptValue(OPT::CRT_REL));
    m_srcfiles.setOptValue(OPT::CRT_DBG, srcOrg.getOptValue(OPT::CRT_DBG));

    m_srcfiles.setOptValue(OPT::BIT64, srcOrg.getOptValue(OPT::BIT64));
    m_srcfiles.setOptValue(OPT::TARGET_DIR64, srcOrg.getOptValue(OPT::TARGET_DIR64));
    m_srcfiles.setOptValue(OPT::BIT32, srcOrg.getOptValue(OPT::BIT32));
    m_srcfiles.setOptValue(OPT::TARGET_DIR32, srcOrg.getOptValue(OPT::TARGET_DIR32));

    m_srcfiles.setOptValue(OPT::LIBS_CMN, srcOrg.getOptValue(OPT::LIBS_CMN));
    m_srcfiles.setOptValue(OPT::LIBS_REL, srcOrg.getOptValue(OPT::LIBS_REL));
    m_srcfiles.setOptValue(OPT::LIBS_DBG, srcOrg.getOptValue(OPT::LIBS_DBG));

    m_srcfiles.setOptValue(OPT::XGET_FLAGS, srcOrg.getOptValue(OPT::XGET_FLAGS));

    ttlib::cstr Root(srcFile);
    Root.remove_filename();

    ttlib::cstr cszRelative;
    ttlib::cstr cszCurCwd;
    cszCurCwd.assignCwd();
    ttlib::ChangeDir(Root);

    if (srcOrg.hasOptValue(OPT::PCH))
    {
        ttlib::cstr cszPch(srcOrg.getOptValue(OPT::PCH_CPP));
        if (cszPch.empty() || cszPch.is_sameas("none"))
        {
            cszPch = srcOrg.getOptValue(OPT::PCH);
            cszPch.replace_extension(".cpp");
            if (!cszPch.file_exists())
            {
                cszPch.replace_extension(".cc");
                if (!cszPch.file_exists())
                {
                    cszPch.replace_extension(".cxx");
                    if (!cszPch.file_exists())
                        cszPch.replace_extension(".cpp");  // File can't be found, switch back to original
                }
            }
            cszPch.make_absolute();
        }
        else
        {
            cszPch = Root;
            cszPch.append(srcOrg.getOptValue(OPT::PCH_CPP));
        }

        cszPch.make_relative(cszCurCwd);
        cszPch.backslashestoforward();
        m_srcfiles.setOptValue(OPT::PCH_CPP, cszPch);
    }

    cszRelative = srcFile.c_str();
    cszRelative.make_relative(cszCurCwd);

    ttlib::add_if(m_srcfiles.GetSrcFileList(), ".include " + cszRelative);

    ttlib::cstr IncDirs(cszRelative);
    IncDirs.remove_filename();

    if (srcOrg.hasOptValue(OPT::INC_DIRS))
    {
        ttlib::multistr enumPaths(srcOrg.getOptValue(OPT::INC_DIRS));
        std::filesystem::current_path(Root.wx_str());
        for (auto& iter: enumPaths)
        {
            cszRelative.assign(iter);
            cszRelative.make_absolute();
            cszRelative.make_relative(cszCurCwd);
            IncDirs << ';' << cszRelative;
        }

        cszRelative += srcOrg.getOptValue(OPT::INC_DIRS);
    }
    m_srcfiles.setOptValue(OPT::INC_DIRS, IncDirs);

#if defined(_WIN32)
    if (srcOrg.hasOptValue(OPT::NATVIS))
    {
        cszRelative = srcOrg.getOptValue(OPT::NATVIS);
        cszRelative.make_absolute();
        cszRelative.make_relative(cszCurCwd);
        m_srcfiles.setOptValue(OPT::NATVIS, cszRelative);
    }
#endif  // _WIN32

    if (srcOrg.hasOptValue(OPT::TARGET_DIR))
    {
        cszRelative = srcOrg.getOptValue(OPT::TARGET_DIR);
        cszRelative.make_absolute();
        cszRelative.make_relative(cszCurCwd);
        m_srcfiles.setOptValue(OPT::TARGET_DIR, cszRelative);
    }

    if (srcOrg.hasOptValue(OPT::TARGET_DIR32))
    {
        cszRelative = srcOrg.getOptValue(OPT::TARGET_DIR32);
        cszRelative.make_absolute();
        cszRelative.make_relative(cszCurCwd);
        m_srcfiles.setOptValue(OPT::TARGET_DIR32, cszRelative);
    }

    if (srcOrg.hasOptValue(OPT::LIB_DIRS))
    {
        cszRelative = srcOrg.getOptValue(OPT::LIB_DIRS);
        cszRelative.make_absolute();
        cszRelative.make_relative(cszCurCwd);
        m_srcfiles.setOptValue(OPT::LIB_DIRS, cszRelative);
    }
    if (srcOrg.hasOptValue(OPT::LIB_DIRS32))
    {
        cszRelative = srcOrg.getOptValue(OPT::LIB_DIRS32);
        cszRelative.make_absolute();
        cszRelative.make_relative(cszCurCwd);
        m_srcfiles.setOptValue(OPT::LIB_DIRS32, cszRelative);
    }

    if (srcOrg.hasOptValue(OPT::BUILD_LIBS))
    {
        cszRelative = srcOrg.getOptValue(OPT::BUILD_LIBS);
        cszRelative.make_absolute();
        cszRelative.make_relative(cszCurCwd);
        m_srcfiles.setOptValue(OPT::BUILD_LIBS, cszRelative);
    }

    return m_srcfiles.WriteNew(dstFile);
}
