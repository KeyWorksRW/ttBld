/////////////////////////////////////////////////////////////////////////////
// Name:      convert.cpp
// Purpose:   Various conversion methods
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

// Assumes that everything except a .DSP file is an xml file and has been parsed into m_xml

#include "pch.h"

#include <ttenumstr.h>  // ttlib::enumstr, ttEnumView -- Enumerate through substrings in a string
#include <ttfile.h>     // ttCFile -- class for reading and writing files, strings, data, etc.
#include <ttxml.h>      // ttCXMLBranch

#include "convertdlg.h"  // CConvertDlg
#include "convert.h"     // CVcxRead

void ConvertScriptDir(std::string_view pszScript, std::string_view pszDir, ttlib::cstr& Result);

bool CConvertDlg::ConvertCodeBlocks()
{
    if (!m_ConvertFile.fileExists())
    {
        ttlib::MsgBox(_tt("Cannot open ") + m_ConvertFile);
        return false;
    }

    ttCXMLBranch* pProject = m_xml.GetRootBranch()->FindFirstElement("Project");
    if (!pProject)
    {
        ttlib::MsgBox(_tt("Cannot locate <Project> in ") + m_ConvertFile);
        return false;
    }

    for (size_t item = 0; item < pProject->GetChildrenCount(); item++)
    {
        ttCXMLBranch* pItem = pProject->GetChildAt(item);
        if (ttlib::issameas(pItem->GetName(), "Option", tt::CASE::either))
        {
            if (pItem->GetAttribute("title"))
                m_cSrcFiles.setOptValue(OPT::PROJECT, pItem->GetAttribute("title"));
        }
        else if (ttlib::issameas(pItem->GetName(), "Unit", tt::CASE::either))
        {
            if (isValidSrcFile(pItem->GetAttribute("filename")))
                m_cSrcFiles.GetSrcFileList().addfilename(MakeSrcRelative(pItem->GetAttribute("filename")));
        }
    }

    return true;
}

bool CConvertDlg::ConvertCodeLite()
{
    ttCXMLBranch* pProject = m_xml.GetRootBranch()->FindFirstElement("CodeLite_Project");
    if (!pProject)
    {
        ttlib::MsgBox(_tt("Cannot locate <CodeLite_Project> in ") + m_ConvertFile);
        return false;
    }

    const char* pszProject = pProject->GetAttribute("Name");
    if (pszProject && *pszProject)
        m_cSrcFiles.setOptValue(OPT::PROJECT, pszProject);

    for (size_t item = 0; item < pProject->GetChildrenCount(); item++)
    {
        ttCXMLBranch* pItem = pProject->GetChildAt(item);
        if (ttlib::issameas(pItem->GetName(), "VirtualDirectory", tt::CASE::either))
            AddCodeLiteFiles(pItem);
        else if (ttlib::issameas(pItem->GetName(), "Settings", tt::CASE::either))
        {
            const char* pszType = pItem->GetAttribute("Type");
            if (ttlib::issameas(pszType, "Dynamic Library", tt::CASE::either))
                m_cSrcFiles.setOptValue(OPT::EXE_TYPE, "dll");
            else if (ttlib::issameas(pszType, "Static Library", tt::CASE::either))
                m_cSrcFiles.setOptValue(OPT::EXE_TYPE, "lib");
        }
    }
    return true;
}

bool CConvertDlg::ConvertSrcfiles()
{
    CSrcFiles srcOrg;
    if (!srcOrg.ReadFile(m_ConvertFile.c_str()))
    {
        ttlib::MsgBox(_tt("Cannot open ") + m_ConvertFile);
        return false;
    }

    // First we copy all the options. Then we tweak those that will need to be changed to use a different path.

    m_cSrcFiles.setOptValue(OPT::PROJECT, srcOrg.getOptValue(OPT::PROJECT));
    m_cSrcFiles.setOptValue(OPT::EXE_TYPE, srcOrg.getOptValue(OPT::EXE_TYPE));
    m_cSrcFiles.setOptValue(OPT::PCH, srcOrg.getOptValue(OPT::PCH));

    m_cSrcFiles.setOptValue(OPT::OPTIMIZE, srcOrg.getOptValue(OPT::OPTIMIZE));
    m_cSrcFiles.SetRequired(OPT::OPTIMIZE);
    m_cSrcFiles.setOptValue(OPT::WARN, srcOrg.getOptValue(OPT::WARN));
    m_cSrcFiles.SetRequired(OPT::WARN);

    m_cSrcFiles.setOptValue(OPT::CFLAGS_CMN, srcOrg.getOptValue(OPT::CFLAGS_CMN));
    m_cSrcFiles.setOptValue(OPT::CFLAGS_REL, srcOrg.getOptValue(OPT::CFLAGS_REL));
    m_cSrcFiles.setOptValue(OPT::CFLAGS_DBG, srcOrg.getOptValue(OPT::CFLAGS_DBG));

    m_cSrcFiles.setOptValue(OPT::CLANG_CMN, srcOrg.getOptValue(OPT::CLANG_CMN));
    m_cSrcFiles.setOptValue(OPT::CLANG_REL, srcOrg.getOptValue(OPT::CLANG_REL));
    m_cSrcFiles.setOptValue(OPT::CLANG_DBG, srcOrg.getOptValue(OPT::CLANG_DBG));

    m_cSrcFiles.setOptValue(OPT::LINK_CMN, srcOrg.getOptValue(OPT::LINK_CMN));
    m_cSrcFiles.setOptValue(OPT::LINK_REL, srcOrg.getOptValue(OPT::LINK_REL));
    m_cSrcFiles.setOptValue(OPT::LINK_DBG, srcOrg.getOptValue(OPT::LINK_DBG));

#if defined(_WIN32)
    m_cSrcFiles.setOptValue(OPT::NATVIS, srcOrg.getOptValue(OPT::NATVIS));

    m_cSrcFiles.setOptValue(OPT::RC_CMN, srcOrg.getOptValue(OPT::RC_CMN));
    m_cSrcFiles.setOptValue(OPT::RC_REL, srcOrg.getOptValue(OPT::RC_REL));
    m_cSrcFiles.setOptValue(OPT::RC_DBG, srcOrg.getOptValue(OPT::RC_DBG));

    m_cSrcFiles.setOptValue(OPT::MIDL_CMN, srcOrg.getOptValue(OPT::MIDL_CMN));
    m_cSrcFiles.setOptValue(OPT::MIDL_REL, srcOrg.getOptValue(OPT::MIDL_REL));
    m_cSrcFiles.setOptValue(OPT::MIDL_DBG, srcOrg.getOptValue(OPT::MIDL_DBG));

    m_cSrcFiles.setOptValue(OPT::MS_LINKER, srcOrg.getOptValue(OPT::MS_LINKER));

#endif  // defined(_WIN32)

    m_cSrcFiles.setOptValue(OPT::CRT_REL, srcOrg.getOptValue(OPT::CRT_REL));
    m_cSrcFiles.setOptValue(OPT::CRT_DBG, srcOrg.getOptValue(OPT::CRT_DBG));

    m_cSrcFiles.setOptValue(OPT::BIT64, srcOrg.getOptValue(OPT::BIT64));
    m_cSrcFiles.setOptValue(OPT::TARGET_DIR64, srcOrg.getOptValue(OPT::TARGET_DIR64));
    m_cSrcFiles.setOptValue(OPT::BIT32, srcOrg.getOptValue(OPT::BIT32));
    m_cSrcFiles.setOptValue(OPT::TARGET_DIR32, srcOrg.getOptValue(OPT::TARGET_DIR32));

    m_cSrcFiles.setOptValue(OPT::LIBS_CMN, srcOrg.getOptValue(OPT::LIBS_CMN));
    m_cSrcFiles.setOptValue(OPT::LIBS_REL, srcOrg.getOptValue(OPT::LIBS_REL));
    m_cSrcFiles.setOptValue(OPT::LIBS_DBG, srcOrg.getOptValue(OPT::LIBS_DBG));

    m_cSrcFiles.setOptValue(OPT::XGET_FLAGS, srcOrg.getOptValue(OPT::XGET_FLAGS));

    ttlib::cstr Root(m_ConvertFile);
    Root.remove_filename();

    ttlib::cstr cszRelative;
    ttlib::cstr cszCurCwd;
    cszCurCwd.assignCwd();
    ttlib::ChangeDir(Root);

    if (srcOrg.hasOptValue(OPT::PCH))
    {
        ttlib::cstr cszPch(srcOrg.getOptValue(OPT::PCH_CPP));
        if (cszPch.empty() || cszPch.issameas("none"))
        {
            cszPch = srcOrg.getOptValue(OPT::PCH);
            cszPch.replace_extension(".cpp");
            if (!cszPch.fileExists())
            {
                cszPch.replace_extension(".cc");
                if (!cszPch.fileExists())
                {
                    cszPch.replace_extension(".cxx");
                    if (!cszPch.fileExists())
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
        m_cSrcFiles.setOptValue(OPT::PCH_CPP, cszPch);
    }

    cszRelative = m_ConvertFile.c_str();
    cszRelative.make_relative(cszCurCwd);

    m_cSrcFiles.GetSrcFileList().addfilename(".include " + cszRelative);

    ttlib::cstr IncDirs(cszRelative);
    IncDirs.remove_filename();

    if (srcOrg.hasOptValue(OPT::INC_DIRS))
    {
        ttlib::enumstr enumPaths(srcOrg.getOptValue(OPT::INC_DIRS));
        fs::current_path(Root.c_str());
        for (auto& iter: enumPaths)
        {
            cszRelative.assign(iter);
            cszRelative.make_absolute();
            cszRelative.make_relative(cszCurCwd);
            IncDirs += (";" + cszRelative);
        }

        cszRelative += srcOrg.getOptValue(OPT::INC_DIRS);
    }
    m_cSrcFiles.setOptValue(OPT::INC_DIRS, IncDirs);

#if defined(_WIN32)
    if (srcOrg.hasOptValue(OPT::NATVIS))
    {
        cszRelative = srcOrg.getOptValue(OPT::NATVIS);
        cszRelative.make_absolute();
        cszRelative.make_relative(cszCurCwd);
        m_cSrcFiles.setOptValue(OPT::NATVIS, cszRelative);
    }
#endif  // _WIN32

    if (srcOrg.hasOptValue(OPT::TARGET_DIR))
    {
        cszRelative = srcOrg.getOptValue(OPT::TARGET_DIR);
        cszRelative.make_absolute();
        cszRelative.make_relative(cszCurCwd);
        m_cSrcFiles.setOptValue(OPT::TARGET_DIR, cszRelative);
    }

    if (srcOrg.hasOptValue(OPT::TARGET_DIR32))
    {
        cszRelative = srcOrg.getOptValue(OPT::TARGET_DIR32);
        cszRelative.make_absolute();
        cszRelative.make_relative(cszCurCwd);
        m_cSrcFiles.setOptValue(OPT::TARGET_DIR32, cszRelative);
    }

    if (srcOrg.hasOptValue(OPT::LIB_DIRS))
    {
        cszRelative = srcOrg.getOptValue(OPT::LIB_DIRS);
        cszRelative.make_absolute();
        cszRelative.make_relative(cszCurCwd);
        m_cSrcFiles.setOptValue(OPT::LIB_DIRS, cszRelative);
    }
    if (srcOrg.hasOptValue(OPT::LIB_DIRS32))
    {
        cszRelative = srcOrg.getOptValue(OPT::LIB_DIRS32);
        cszRelative.make_absolute();
        cszRelative.make_relative(cszCurCwd);
        m_cSrcFiles.setOptValue(OPT::LIB_DIRS32, cszRelative);
    }

    if (srcOrg.hasOptValue(OPT::BUILD_LIBS))
    {
        cszRelative = srcOrg.getOptValue(OPT::BUILD_LIBS);
        cszRelative.make_absolute();
        cszRelative.make_relative(cszCurCwd);
        m_cSrcFiles.setOptValue(OPT::BUILD_LIBS, cszRelative);
    }

    ttlib::ChangeDir(cszCurCwd.c_str());  // Restore our current directory

    return true;
}

void ConvertScriptDir(std::string_view Script, std::string_view Dir, ttlib::cstr& Result)
{
    if (Dir.length() > 1 && Dir.at(1) == ':')
    {
        // If the path starts with a drive letter, then we can't make it relative
        Result = Dir;
        return;
    }

    Result = Script;
    Result.remove_filename();
    Result.append(Dir);

    Result.make_absolute();

    ttlib::cwd cwd;

    Result.make_relative(cwd);
}
