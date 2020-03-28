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

#include <ttcwd.h>      // cwd -- Class for storing and optionally restoring the current directory
#include <ttdirdlg.h>   // DirDlg -- Class for displaying a dialog to select a directory
#include <ttenumstr.h>  // ttlib::enumstr, ttEnumView -- Enumerate through substrings in a string
#include <ttfiledlg.h>  // ttCFileDlg -- Wrapper around Windows GetOpenFileName() API
#include <ttwinff.h>    // winff -- Wrapper around Windows FindFile

#include "funcs.h"       // List of function declarations
#include "ttlibicons.h"  // Icons for use on 3D shaded buttons (ttShadeBtn)

#include "convertdlg.h"  // CConvertDlg
#include "vcxproj.h"     // CVcxRead

// clang-format off
static const char* atxtSrcTypes[]
{
    "*.cpp",
    "*.cc",
    "*.cxx",
    "*.c",
     nullptr
};
// clang-format on

// Array of project extensions (*.vcxproj, *.project, etc.).
static const char* atxtProjects[] {
    // clang-format off
    "*.vcxproj",       // Visual Studio
    "*.vcproj",        // Old Visual Studio
    "*.project",       // CodeLite
    "*.cbp",           // CodeBlocks
    "*.dsp",           // Very old Visual Studio project
    ".srcfiles.yaml",  // ttBld project files

     nullptr
    // clang-format on
};

CConvertDlg::CConvertDlg(const char* pszDstSrcFiles) : ttlib::dlg(IDDDLG_CONVERT)
{
    if (pszDstSrcFiles)
        m_cszOutSrcFiles = pszDstSrcFiles;
    else
        m_cszOutSrcFiles = ".srcfiles.yaml";

    m_bCreateVsCode = false;
    m_bGitIgnore = false;

    // Process of conversion may change directories, so save the directory we should change back to
    m_cwd.assignCwd();
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
    if (!ttlib::dirExists(".vscode") && FindVsCode(tmp))
        SetCheck(IDCHECK_VSCODE);
    if (ttlib::dirExists(".git") || ttlib::dirExists("../.git") || ttlib::dirExists("../../.git"))
        SetCheck(IDCHECK_IGNORE_ALL);

    SetControlText(IDEDIT_IN_DIR, m_cwd.c_str());

    if (!m_cszOutSrcFiles.empty())
    {
        tmp = m_cszOutSrcFiles;
        tmp.make_relative(m_cwd);
        tmp.remove_filename();
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
                    if (!ttIsValidFileChar(ff, 0))  // this will skip over . and ..
                        continue;
                    ttlib::cstr cszDir(ff.getcstr());
                    cszDir.append(atxtProjects[0]);
                    ttlib::winff ffFilter(cszDir.c_str());
                    for (size_t pos = 1; !ffFilter.isvalid() && atxtProjects[pos]; ++pos)
                    {
                        cszDir = ff.getcstr();
                        cszDir.append(atxtProjects[pos]);
                        ffFilter.newpattern(cszDir.c_str());
                    }

                    if (ffFilter.isvalid())
                    {
                        cszDir = ff.getcstr();
                        cszDir.append(ffFilter);
                        m_comboScripts.append(cszDir);
                    }
                }
            } while (ff.next());
        }
    }

    size_t cFilesFound = 0;
    for (size_t pos = 0; atxtSrcTypes[pos]; ++pos)
    {
        if (ff.newpattern(atxtSrcTypes[pos]))
        {
            do
            {
                ++cFilesFound;
            } while (ff.next());
        }
    }
    tmp.Format("%kzd %s located", cFilesFound, _ttp("file", "files", cFilesFound));
    SetControlText(IDTXT_FILES_FOUND, tmp);

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
    ttCFileDlg dlg(*this);
    dlg.SetFilter("Project Files|*.vcxproj;*.vcproj;*.dsp;*.project;*.cbp;.srcfiles.yaml||");
    dlg.UseCurrentDirectory();
    dlg.RestoreDirectory();
    if (dlg.GetOpenName())
    {
        auto item = m_comboScripts.append(dlg.GetFileName());
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
        if (dlg.fileExists())
        {
            if (ttlib::MsgBox(
                    _tt(".srcfiles.yaml already exists in this directory. Are you sure you want to replace it?"),
                    MB_YESNO) != IDYES)
                return;
        }
        SetControlText(IDEDIT_OUT_DIR, dlg.c_str());
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
        for (size_t pos = 0; atxtSrcTypes[pos]; ++pos)
        {
            if (ff.newpattern(atxtSrcTypes[pos]))
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
    m_cszOutSrcFiles.GetWndText(gethwnd(IDEDIT_OUT_DIR));
    m_cszOutSrcFiles.append_filename(".srcfiles.yaml");
    m_cszDirSrcFiles.GetWndText(gethwnd(IDEDIT_IN_DIR));
    if (GetCheck(IDRADIO_CONVERT))
        m_ConvertFile.GetWndText(gethwnd(IDCOMBO_SCRIPTS));
    else
    {
        m_ConvertFile.clear();
    }

    m_bCreateVsCode = GetCheck(IDCHECK_VSCODE);
    m_bGitIgnore = GetCheck(IDCHECK_IGNORE_ALL);

    if (!m_ConvertFile.empty() && !m_ConvertFile.fileExists())
    {
        ttlib::MsgBox(_tt("Cannot open ") + m_ConvertFile);
        CancelEnd();
        return;
    }

    if (!doConversion())
        CancelEnd();
}

void CConvertDlg::AddCodeLiteFiles(ttCXMLBranch* pParent)
{
    for (size_t child = 0; child < pParent->GetChildrenCount(); child++)
    {
        ttCXMLBranch* pFile = pParent->GetChildAt(child);
        if (ttIsSameStrI(pFile->GetName(), "File"))
        {
            if (isValidSrcFile(pFile->GetAttribute("Name")))
                m_cSrcFiles.GetSrcFileList().addfilename(MakeSrcRelative(pFile->GetAttribute("Name")));
        }
        // CodeLite nests resources in a sub <VirtualDirectory> tag
        else if (ttIsSameStrI(pFile->GetName(), "VirtualDirectory"))
        {
            AddCodeLiteFiles(pFile);
        }
    }
}

bool CConvertDlg::isValidSrcFile(const char* pszFile) const
{
    if (!pszFile)
        return false;

    char* psz = ttStrChrR(pszFile, '.');
    if (psz)
    {
        if (ttIsSameStrI(psz, ".cpp") || ttIsSameStrI(psz, ".cc") || ttIsSameStrI(psz, ".cxx") ||
            ttIsSameStrI(psz, ".c") || ttIsSameSubStrI(psz, ".rc"))
            return true;
    }
    return false;
}

// This function first converts the file relative to the location of the build script, and then relative to the
// location of .srcfiles

char* CConvertDlg::MakeSrcRelative(const char* pszFile)
{
    ttASSERT(!m_ConvertFile.empty());

    if (m_cszScriptRoot.empty())
    {
        m_cszScriptRoot = m_ConvertFile;
        m_cszScriptRoot.make_absolute();
        m_cszScriptRoot.remove_filename();

        // For GetFullPathName() to work properly on a file inside the script, we need to be in the same directory
        // as the script file

        ttlib::ChangeDir(m_cszScriptRoot);
    }

    if (m_cszOutRoot.empty())
    {
        m_cszOutRoot = m_cszOutSrcFiles;
        m_cszOutRoot.make_absolute();
        if (m_cszOutRoot.hasFilename(".srcfiles.yaml"))
            m_cszOutRoot.remove_filename();
    }

    m_cszRelative = pszFile;
    m_cszRelative.make_absolute();
    m_cszRelative.make_relative(m_cszOutRoot);
    return (char*) m_cszRelative.c_str();
}

bool CConvertDlg::doConversion()
{
    ttASSERT_MSG(!m_cszOutSrcFiles.empty(), "Need to set path to .srcfiles.yaml before calling doConversion()");

    // If there is no conversion script file, then convert using files in the current directory.
    if (m_ConvertFile.empty())
    {
        m_cSrcFiles.InitOptions();

        if (!m_cSrcFiles.hasOptValue(OPT::PROJECT))
        {
            ttlib::cstr projname(m_cszOutSrcFiles);
            projname.remove_filename();
            projname.backslashestoforward();
            if (projname.back() == '/')
                projname.erase(projname.size() - 1);
            if (projname.hasFilename("src"))
            {
                projname.remove_filename();
                if (projname.back() == '/')
                    projname.erase(projname.size() - 1);
            }
            m_cSrcFiles.setOptValue(OPT::PROJECT, projname.filename());

            if (!m_cSrcFiles.hasOptValue(OPT::PCH) || m_cSrcFiles.getOptValue(OPT::PCH).issameas("none"))
            {
                if (ttlib::fileExists("stdafx.h"))
                    m_cSrcFiles.setOptValue(OPT::PCH, "stdafx.h");
                else if (ttlib::fileExists("pch.h"))
                    m_cSrcFiles.setOptValue(OPT::PCH, "pch.h");
                else if (ttlib::fileExists("precomp.h"))
                    m_cSrcFiles.setOptValue(OPT::PCH, "precomp.h");

                else if (ttlib::fileExists("pch.hh"))
                    m_cSrcFiles.setOptValue(OPT::PCH, "pch.hh");
                else if (ttlib::fileExists("pch.hpp"))
                    m_cSrcFiles.setOptValue(OPT::PCH, "pch.hpp");
                else if (ttlib::fileExists("pch.hxx"))
                    m_cSrcFiles.setOptValue(OPT::PCH, "pch.hxx");
            }
        }

#if defined(_WIN32)
        m_cSrcFiles.AddSourcePattern("*.cpp;*.cc;*.cxx;*.rc;*.idl;*.hhp");
#else
        m_cSrcFiles.AddSourcePattern("*.cpp;*.cc;*.cxx");
#endif

        if (m_cSrcFiles.WriteNew(m_cszOutSrcFiles) != bld::success)
        {
            ttlib::MsgBox(_tt("Unable to create or write to ") + m_cszOutSrcFiles);
            CancelEnd();
            return false;
        }
        return true;
    }

    auto extension = m_ConvertFile.extension();
    if (!extension.empty())
    {
        bool bResult = false;
        if (extension.issameas(".vcxproj", tt::CASE::either))
        {
            CVcxRead vcx;
            auto result = vcx.Convert(m_ConvertFile, m_cszOutSrcFiles);
            return (result == bld::success);
        }

        else if (extension.issameas(".dsp", tt::CASE::either))
            bResult = ConvertDsp();
        else if (m_ConvertFile.issameprefix(".srcfiles", tt::CASE::either))
            bResult = ConvertSrcfiles();
        else
        {
            HRESULT hr = m_xml.ParseXmlFile(m_ConvertFile.c_str());
            if (hr != S_OK)
            {
                ttlib::MsgBox(_tt("An internal error occurred attempting to parse the file ") + m_ConvertFile);
                return false;
            }

            if (extension.issameas(".project", tt::CASE::either))
                bResult = ConvertCodeLite();
            else if (extension.issameas(".cdb", tt::CASE::either))
                bResult = ConvertCodeBlocks();
            else if (extension.issameas(".vcproj", tt::CASE::either))
                bResult = ConvertVcProj();
        }

        ttlib::ChangeDir(m_cwd);  // we may have changed directories during the conversion

        if (bResult)
        {
            ttlib::cstr cszHdr, cszRelative;
            cszRelative = m_ConvertFile;
            cszRelative.make_relative(m_cszOutSrcFiles);

            cszHdr = "# Converted from " + cszRelative;

            if (!m_cSrcFiles.hasOptValue(OPT::PROJECT))
            {
                ttlib::cstr cszProject(m_cszOutSrcFiles);
                cszProject.remove_filename();
                m_cSrcFiles.setOptValue(OPT::PROJECT, cszProject.filename());
            }

            if (m_cSrcFiles.WriteNew(m_cszOutSrcFiles.c_str(), cszHdr.c_str()) != bld::success)
            {
                ttlib::MsgBox(_tt("Unable to create or write to ") + m_cszOutSrcFiles);
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
