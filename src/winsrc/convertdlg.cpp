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

#include <ttTR.h>  // Function for translating strings

#include "convertdlg.h"  // CConvertDlg

#include <direct.h>  // Functions for directory handling and creation

#include <ttcwd.h>       // cwd -- Class for storing and optionally restoring the current directory
#include <ttdirdlg.h>    // DirDlg -- Class for displaying a dialog to select a directory
#include <ttenumstr.h>   // ttlib::enumstr, ttEnumView -- Enumerate through substrings in a string
#include <ttfiledlg.h>   // ttCFileDlg -- Wrapper around Windows GetOpenFileName() API
#include <ttfindfile.h>  // ttCFindFile
#include <ttwinff.h>     // winff -- Wrapper around Windows FindFile

#include "funcs.h"       // List of function declarations
#include "ttlibicons.h"  // Icons for use on 3D shaded buttons (ttShadeBtn)

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
    m_cszCWD.assignCwd();
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

    tmp.assignCwd();
    SetControlText(IDEDIT_IN_DIR, tmp.c_str());

    if (!m_cszOutSrcFiles.empty())
    {
        tmp = m_cszOutSrcFiles.c_str();
        tmp.make_absolute();
        tmp.remove_filename();
    }
    SetControlText(IDEDIT_OUT_DIR, tmp);

    m_comboScripts.Initialize(*this, IDCOMBO_SCRIPTS);
    ttCFindFile ff(atxtProjects[0]);

    // If we converted once before, then default to that script

    if (!m_cszConvertScript.empty())
        m_comboScripts.append(m_cszConvertScript);
    else
    {
        for (size_t pos = 1; !ff.IsValid() && atxtProjects[pos]; ++pos)
            ff.NewPattern(atxtProjects[pos]);
        if (ff.IsValid())
            m_comboScripts.append((char*) ff);

        if (m_comboScripts.GetCount() < 1)  // no scripts found, let's check sub directories
        {
            ff.NewPattern("*.*");
            do
            {
                if (ff.IsDir())
                {
                    if (!ttIsValidFileChar(ff, 0))  // this will skip over . and ..
                        continue;
                    ttlib::cstr cszDir((char*) ff);
                    cszDir.append(atxtProjects[0]);
                    ttCFindFile ffFilter(cszDir.c_str());
                    for (size_t pos = 1; !ffFilter.IsValid() && atxtProjects[pos]; ++pos)
                    {
                        cszDir = ff.GetFileName();
                        cszDir.append(atxtProjects[pos]);
                        ffFilter.NewPattern(cszDir.c_str());
                    }

                    if (ffFilter.IsValid())
                    {
                        cszDir = ff.GetFileName();
                        cszDir.append(ffFilter);
                        m_comboScripts.append(cszDir);
                    }
                }
            } while (ff.NextFile());
        }
    }

    size_t cFilesFound = 0;
    for (size_t pos = 0; atxtSrcTypes[pos]; ++pos)
    {
        if (ff.NewPattern(atxtSrcTypes[pos]))
        {
            do
            {
                ++cFilesFound;
            } while (ff.NextFile());
        }
    }
    tmp.Format("%kn file%ks located", cFilesFound, cFilesFound);
    SetControlText(IDTXT_FILES_FOUND, tmp.c_str());

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
            if (ttMsgBox(
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
        ttCFindFile ff("*.cpp");
        size_t cFilesFound = 0;
        for (size_t pos = 0; atxtSrcTypes[pos]; ++pos)
        {
            if (ff.NewPattern(atxtSrcTypes[pos]))
            {
                do
                {
                    ++cFilesFound;
                } while (ff.NextFile());
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
        m_cszConvertScript.GetWndText(gethwnd(IDCOMBO_SCRIPTS));
    else
        m_cszConvertScript.clear();

    m_bCreateVsCode = GetCheck(IDCHECK_VSCODE);
    m_bGitIgnore = GetCheck(IDCHECK_IGNORE_ALL);

    if (!m_cszConvertScript.empty() && !m_cszConvertScript.fileExists())
    {
        ttlib::MsgBox(_tt("Cannot open ") + m_cszConvertScript);
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
    ttASSERT(!m_cszConvertScript.empty());

    if (m_cszScriptRoot.empty())
    {
        m_cszScriptRoot = m_cszConvertScript;
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

bool CConvertDlg::doConversion(const char* pszInFile)
{
    ttASSERT_MSG(!m_cszOutSrcFiles.empty(), "Need to set path to .srcfiles.yaml before calling doConversion()");

    if (pszInFile)
        m_cszConvertScript = pszInFile;

    // If there is no conversion script file, then convert using files in the current directory.
    if (m_cszConvertScript.empty())
    {
        if (!m_cSrcFiles.hasOptValue(OPT::PROJECT))
        {
            ttlib::cstr cszProject(m_cszOutSrcFiles);
            cszProject.remove_filename();
            m_cSrcFiles.setOptValue(OPT::PROJECT, cszProject.filename());
        }

#if defined(_WIN32)
        m_cSrcFiles.AddSourcePattern("*.cpp;*.cc;*.cxx;*.rc;*.idl;*.hhp");
#else
        m_cSrcFiles.AddSourcePattern("*.cpp;*.cc;*.cxx");
#endif

        if (m_cSrcFiles.WriteNew(m_cszOutSrcFiles.c_str()) != bld::success)
        {
            ttlib::MsgBox(_tt("Unable to create or write to ") + m_cszOutSrcFiles);
            CancelEnd();
            return false;
        }
        return true;
    }

    const char* pszExt = ttStrChrR(m_cszConvertScript.c_str(), '.');
    if (pszExt)
    {
        bool bResult = false;
        if (ttIsSameStrI(pszExt, ".dsp"))
            bResult = ConvertDsp();
        else if (ttStrStrI(m_cszConvertScript.c_str(), ".srcfiles"))
            bResult = ConvertSrcfiles();
        else
        {
            HRESULT hr = m_xml.ParseXmlFile(m_cszConvertScript.c_str());
            if (hr != S_OK)
            {
                ttlib::MsgBox(_tt("An internal error occurred attempting to parse the file ") +
                              m_cszConvertScript);
                return false;
            }

            if (ttIsSameStrI(pszExt, ".project"))
                bResult = ConvertCodeLite();
            else if (ttIsSameStrI(pszExt, ".cdb"))
                bResult = ConvertCodeBlocks();
            else if (ttIsSameStrI(pszExt, ".vcxproj"))
                bResult = ConvertVcxProj();
            else if (ttIsSameStrI(pszExt, ".vcproj"))
                bResult = ConvertVcProj();
        }

        ttlib::ChangeDir(m_cszCWD);  // we may have changed directories during the conversion

        if (bResult)
        {
            ttlib::cstr cszHdr, cszRelative;
            cszRelative = m_cszConvertScript;
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
