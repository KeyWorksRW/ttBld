/////////////////////////////////////////////////////////////////////////////
// Name:      CConvertDlg
// Purpose:   IDDDLG_CONVERT dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

/*
    Source files specified in a build script are relative to the location of that build script. The .srcfiles file
    we are creating may be in an entirely different directory. So before we add a file to .srcfiles, we must first
    make it relative to the location of the build script, and then make it relative to the location of .srcfiles.
*/

#include "pch.h"

#include <ttcwd.h>       // cwd -- Class for storing and optionally restoring the current directory
#include <ttdirdlg.h>    // DirDlg -- Class for displaying a dialog to select a directory
#include <ttmultistr.h>  // multistr -- Breaks a single string into multiple strings
#include <ttopenfile.h>  // openfile -- Wrapper around Windows GetOpenFileName() API
#include <ttwinff.h>     // winff -- Wrapper around Windows FindFile

#include "funcs.h"       // List of function declarations
#include "ttlibicons.h"  // Icons for use on 3D shaded buttons (ttShadeBtn)

#include "convert.h"     // CConvert
#include "convertdlg.h"  // CConvertDlg

// clang-format off
static const char* listSrcTypes[]
{
    "*.cpp",
    "*.cc",
    "*.cxx",
    "*.c",
};

// Array of project extensions (*.vcxproj, *.project, etc.).
static const char* atxtProjects[]
{
    "*.vcxproj",       // Visual Studio
    "*.vcproj",        // Old Visual Studio
    "*.project",       // CodeLite
    "*.cbp",           // CodeBlocks
    "*.dsp",           // Very old Visual Studio project
    ".srcfiles.yaml",  // ttBld project files

     nullptr
};

// clang-format on

bool MakeNewProject(ttlib::cstr& projectFile)
{
    CConvertDlg dlg(projectFile);
    if (dlg.DoModal(NULL) != IDOK)
        return false;

    projectFile = dlg.GetOutSrcFiles();

    ChangeOptions(projectFile);

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
            std::cout << _tt(strIdIgnoredFiles) << GitExclude << '\n';
        }
    }

    return true;
}

CConvertDlg::CConvertDlg() : ttlib::dlg(IDDDLG_CONVERT)
{
    m_new_srcfiles = ".srcfiles.yaml";
}

CConvertDlg::CConvertDlg(std::string_view projectFile) : ttlib::dlg(IDDDLG_CONVERT)
{
    if (projectFile.size())
        m_new_srcfiles.assign(projectFile);
    else
        m_new_srcfiles = ".srcfiles.yaml";
}

void CConvertDlg::OnBegin(void)
{
    CHECK_DLG_ID(IDCANCEL);
    CHECK_DLG_ID(IDCHECK_IGNORE_ALL);
    CHECK_DLG_ID(IDCHECK_VSCODE);
    CHECK_DLG_ID(IDCOMBO_SCRIPTS);
    CHECK_DLG_ID(IDEDIT_IN_DIR);
    CHECK_DLG_ID(IDEDIT_OUT_DIR);
    CHECK_DLG_ID(IDOK);
    CHECK_DLG_ID(IDRADIO_CONVERT);
    CHECK_DLG_ID(IDRADIO_FILES);
    CHECK_DLG_ID(IDTXT_FILES_FOUND);

    CenterWindow(true);
    EnableShadeBtns();
    SetBtnIcon(IDOK, IDICON_TTLIB_OK);
    SetBtnIcon(IDCANCEL, IDICON_TTLIB_CANCEL);

    ttlib::cstr tmp;
    if (!ttlib::dir_exists(".vscode") && IsVsCodeAvail())
        SetCheck(IDCHECK_VSCODE);

    // These three directories are the only locations that gitIgnoreAll() will use, so if they don't exist, then there
    // isn't any way to add to the excludes list -- so just hide the control if that's the case.
    if (ttlib::dir_exists(".git") || ttlib::dir_exists("../.git") || ttlib::dir_exists("../../.git"))
        SetCheck(IDCHECK_IGNORE_ALL);
    else
        HideControl(IDCHECK_IGNORE_ALL);

    m_cwd.backslashestoforward();
    SetControlText(IDEDIT_IN_DIR, m_cwd);

    if (!m_new_srcfiles.empty())
    {
        tmp = m_new_srcfiles;
        tmp.make_relative(m_cwd);
        tmp.remove_filename();
        tmp.backslashestoforward();
    }
    if (!tmp.empty())
        SetControlText(IDEDIT_OUT_DIR, tmp);
    else
        SetControlText(IDEDIT_OUT_DIR, m_cwd);

    m_comboScripts.Initialize(*this, IDCOMBO_SCRIPTS);
    ttlib::winff ff(atxtProjects[0]);

    // If we converted once before, then default to that script

    if (!m_ConvertFile.empty())
        m_comboScripts.append(m_ConvertFile);
    else
    {
        for (size_t pos = 1; !ff.isvalid() && atxtProjects[pos]; ++pos)
            ff.newpattern(atxtProjects[pos]);
        if (ff.isvalid())
            m_comboScripts.append(ff.getcstr());

        if (m_comboScripts.GetCount() < 1)  // no scripts found, let's check sub directories
        {
            ff.newpattern("*.*");
            do
            {
                if (ff.isdir())
                {
                    ttlib::cstr dir(ff.getcstr());
                    dir.append(atxtProjects[0]);
                    ttlib::winff ffFilter(dir);
                    for (size_t pos = 1; !ffFilter.isvalid() && atxtProjects[pos]; ++pos)
                    {
                        dir = ff.getcstr();
                        dir.append(atxtProjects[pos]);
                        ffFilter.newpattern(dir);
                    }

                    if (ffFilter.isvalid())
                    {
                        dir = ff.getcstr();
                        dir.append(ffFilter);
                        m_comboScripts.append(dir);
                    }
                }
            } while (ff.next());
        }
    }

    size_t cFilesFound = 0;
    for (size_t pos = 0; listSrcTypes[pos]; ++pos)
    {
        if (ff.newpattern(listSrcTypes[pos]))
        {
            do
            {
                ++cFilesFound;
            } while (ff.next());
        }
    }
    SetControlText(IDTXT_FILES_FOUND, ttlib::cstr().Format(_tt(strIdFmtFilesLocated), cFilesFound));

    if (m_comboScripts.GetCount() > 0)
    {
        m_comboScripts.SetCurSel();
        SetCheck(IDRADIO_CONVERT);
    }
    else if (cFilesFound)
    {
        SetCheck(IDRADIO_FILES);
    }
}

void CConvertDlg::OnBtnLocateScript()
{
    ttlib::openfile dlg(*this);
    dlg.SetFilter(_ttc(strIdProjectFiles) + "|*.vcxproj;*.vcproj;*.dsp;*.project;*.cbp;.srcfiles.yaml||");
    dlg.RestoreDirectory();
    if (dlg.GetOpenName())
    {
        auto item = m_comboScripts.append(dlg.filename());
        m_comboScripts.SetCurSel(item);
        UnCheck(IDRADIO_FILES);
        SetCheck(IDRADIO_CONVERT);
        // TODO: [randalphwa - 4/2/2019] Need to decide how to handle IDEDIT_IN_DIR since this may no longer be
        // correct
    }
}

void CConvertDlg::OnBtnChangeOut()  // change the directory to write .srcfiles to
{
    ttlib::DirDlg dlg;
    ttlib::cwd cwd;
    dlg.SetStartingDir(cwd);
    if (dlg.GetFolderName(*this))
    {
        dlg.append_filename(".srcfiles.yaml");
        if (dlg.file_exists())
        {
            if (ttlib::MsgBox(_tt(strIdConfirmReplace), MB_YESNO) != IDYES)
                return;
        }
        dlg.backslashestoforward();
        SetControlText(IDEDIT_OUT_DIR, dlg);
    }
}

void CConvertDlg::OnBtnChangeIn()
{
    ttlib::DirDlg dlg;
    ttlib::cwd cwd;
    dlg.SetStartingDir(cwd);
    if (dlg.GetFolderName(*this))
    {
        SetControlText(IDEDIT_IN_DIR, dlg.c_str());
        ttlib::ChangeDir(dlg);
        ttlib::winff ff("*.cpp");
        size_t cFilesFound = 0;
        for (auto type: listSrcTypes)
        {
            if (ff.newpattern(type))
            {
                do
                {
                    ++cFilesFound;
                } while (ff.next());
            }
        }
        ttlib::ChangeDir(cwd);  // restore our directory
        ttlib::cstr tmp;
        SetControlText(IDTXT_FILES_FOUND, tmp.Format("%kn file located", cFilesFound));
        UnCheck(IDRADIO_CONVERT);
        SetCheck(IDRADIO_FILES);
    }
}

void CConvertDlg::OnOK(void)
{
    m_new_srcfiles.GetWndText(gethwnd(IDEDIT_OUT_DIR));
    m_new_srcfiles.append_filename(".srcfiles.yaml");
    m_cszDirSrcFiles.GetWndText(gethwnd(IDEDIT_IN_DIR));
    if (GetCheck(IDRADIO_CONVERT))
        m_ConvertFile.GetWndText(gethwnd(IDCOMBO_SCRIPTS));
    else
    {
        m_ConvertFile.clear();
    }

    m_CreateVscode = GetCheck(IDCHECK_VSCODE);
    m_AddToGitExclude = GetCheck(IDCHECK_IGNORE_ALL);

    if (!m_ConvertFile.empty() && !m_ConvertFile.file_exists())
    {
        ttlib::MsgBox(_tt(strIdCantOpen) + m_ConvertFile);
        CancelEnd();
        return;
    }

    if (!doConversion())
        CancelEnd();
}

bool CConvertDlg::doConversion()
{
    ttASSERT_MSG(!m_new_srcfiles.empty(), "Need to set path to .srcfiles.yaml before calling doConversion()");

    // If there is no conversion script file, then convert using files in the current directory.
    if (m_ConvertFile.empty())
    {
        m_cSrcFiles.InitOptions();

        if (!m_cSrcFiles.hasOptValue(OPT::PROJECT))
        {
            ttlib::cstr projname(m_new_srcfiles);
            projname.remove_filename();
            projname.backslashestoforward();
            if (projname.back() == '/')
                projname.pop_back();
            if (projname.has_filename("src"))
            {
                projname.remove_filename();
                if (projname.back() == '/')
                    projname.pop_back();
            }
            m_cSrcFiles.setOptValue(OPT::PROJECT, projname.filename());

            if (!m_cSrcFiles.hasOptValue(OPT::PCH) || m_cSrcFiles.getOptValue(OPT::PCH).is_sameas("none"))
            {
                if (ttlib::file_exists("stdafx.h"))
                    m_cSrcFiles.setOptValue(OPT::PCH, "stdafx.h");
                else if (ttlib::file_exists("pch.h"))
                    m_cSrcFiles.setOptValue(OPT::PCH, "pch.h");
                else if (ttlib::file_exists("precomp.h"))
                    m_cSrcFiles.setOptValue(OPT::PCH, "precomp.h");

                else if (ttlib::file_exists("pch.hh"))
                    m_cSrcFiles.setOptValue(OPT::PCH, "pch.hh");
                else if (ttlib::file_exists("pch.hpp"))
                    m_cSrcFiles.setOptValue(OPT::PCH, "pch.hpp");
                else if (ttlib::file_exists("pch.hxx"))
                    m_cSrcFiles.setOptValue(OPT::PCH, "pch.hxx");
            }
        }

#if defined(_WIN32)
        m_cSrcFiles.AddSourcePattern("*.cpp;*.cc;*.cxx;*.rc;*.idl");
#else
        m_cSrcFiles.AddSourcePattern("*.cpp;*.cc;*.cxx");
#endif

        if (m_cSrcFiles.WriteNew(m_new_srcfiles) != bld::success)
        {
            ttlib::MsgBox(_tt(strIdCantWrite) + m_new_srcfiles);
            CancelEnd();
            return false;
        }
        return true;
    }

    auto extension = m_ConvertFile.extension();
    if (!extension.empty())
    {
        bool bResult = false;
        if (extension.is_sameas(".vcxproj", tt::CASE::either))
        {
            CConvert convert;
            auto result = convert.ConvertVcx(m_ConvertFile, m_new_srcfiles);
            return (result == bld::success);
        }
        else if (extension.is_sameas(".vcproj", tt::CASE::either))
        {
            CConvert convert;
            auto result = convert.ConvertVc(m_ConvertFile, m_new_srcfiles);
            return (result == bld::success);
        }
        else if (extension.is_sameas(".dsp", tt::CASE::either))
        {
            CConvert convert;
            auto result = convert.ConvertDsp(m_ConvertFile, m_new_srcfiles);
            return (result == bld::success);
        }
        else if (extension.is_sameas(".project", tt::CASE::either))
        {
            CConvert convert;
            auto result = convert.ConvertCodeLite(m_ConvertFile, m_new_srcfiles);
            return (result == bld::success);
        }
        else if (m_ConvertFile.is_sameprefix(".srcfiles", tt::CASE::either))
        {
            CConvert convert;
            auto result = convert.ConvertSrcfiles(m_ConvertFile, m_new_srcfiles);
            return (result == bld::success);
        }

        ttlib::ChangeDir(m_cwd);  // we may have changed directories during the conversion

        if (bResult)
        {
            ttlib::cstr cszHdr, cszRelative;
            cszRelative = m_ConvertFile;
            cszRelative.make_relative(m_new_srcfiles);

            cszHdr = "# Converted from " + cszRelative;

            if (!m_cSrcFiles.hasOptValue(OPT::PROJECT))
            {
                ttlib::cstr cszProject(m_new_srcfiles);
                cszProject.remove_filename();
                m_cSrcFiles.setOptValue(OPT::PROJECT, cszProject.filename());
            }

            if (m_cSrcFiles.WriteNew(m_new_srcfiles.c_str(), cszHdr.c_str()) != bld::success)
            {
                ttlib::MsgBox(_tt(strIdCantWrite) + m_new_srcfiles);
                return false;
            }
            return true;
        }
    }
    return false;
}

void CConvertDlg::OnCheckConvert()
{
    UnCheck(IDRADIO_FILES);
}

void CConvertDlg::OnCheckFiles()
{
    UnCheck(IDRADIO_CONVERT);
}

void CConvertDlg::SetConvertScritpt(std::string_view filename)
{
    m_ConvertFile = filename;
    m_ConvertFile.make_relative(m_cwd);
    m_ConvertFile.backslashestoforward();
}
