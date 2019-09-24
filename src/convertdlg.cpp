/////////////////////////////////////////////////////////////////////////////
// Name:      CConvertDlg
// Purpose:   IDDDLG_CONVERT dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

/*
    Source files specified in a build script are relative to the location of that build script. The .srcfiles file we
    are creating may be in an entirely different directory. So before we add a file to .srcfiles, we must first make it
    relative to the location of the build script, and then make it relative to the location of .srcfiles.
*/

#include "pch.h"

#include "convertdlg.h"  // CConvertDlg

#include <direct.h>  // Functions for directory handling and creation

#include <ttdirdlg.h>    // ttCDirDlg
#include <ttenumstr.h>   // ttCEnumStr
#include <ttfiledlg.h>   // ttCFileDlg
#include <ttfindfile.h>  // ttCFindFile

#include "ttlibicons.h"  // Icons for use on 3D shaded buttons (ttShadeBtn)
#include "funcs.h"       // List of function declarations

static const char* atxtSrcTypes[] = { "*.cpp", "*.cc", "*.cxx", "*.c", nullptr };

// clang-format off
static const char* atxtProjects[] = {
     "*.vcxproj",  // Visual Studio
     "*.vcproj",   // Old Visual Studio
     "*.project",  // CodeLite
     "*.cbp",      // CodeBlocks

     nullptr
};
// clang-format on

CConvertDlg::CConvertDlg(const char* pszDstSrcFiles)
    : ttCDlg(IDDDLG_CONVERT)
{
    if (pszDstSrcFiles)
        m_cszOutSrcFiles = pszDstSrcFiles;
    else
        m_cszOutSrcFiles = ".srcfiles.yaml";

    m_bCreateVsCode = false;
    m_bGitIgnore = false;

    // Process of conversion may change directories, so save the directory we should change back to
    m_cszCWD.GetCWD();
}

void CConvertDlg::OnBegin(void)
{
    CenterWindow(true);
    EnableShadeBtns();
    SetBtnIcon(DLG_ID(IDOK), IDICON_TTLIB_OK);
    SetBtnIcon(DLG_ID(IDCANCEL), IDICON_TTLIB_CANCEL);

    ttCStr csz;
    if (!ttDirExists(".vscode") && FindVsCode(csz))
        SetCheck(DLG_ID(IDCHECK_VSCODE));
    if (ttDirExists(".git") || ttDirExists("../.git") || ttDirExists("../../.git"))
        SetCheck(DLG_ID(IDCHECK_IGNORE_ALL));

    csz.GetCWD();
    SetControlText(DLG_ID(IDEDIT_IN_DIR), csz);

    if (m_cszOutSrcFiles.IsNonEmpty())
    {
        csz = m_cszOutSrcFiles;
        csz.FullPathName();
        char* pszFilePortion = ttFindFilePortion(csz);
        if (pszFilePortion)
            *pszFilePortion = 0;
    }
    SetControlText(DLG_ID(IDEDIT_OUT_DIR), csz);

    m_comboScripts.Initialize(*this, DLG_ID(IDCOMBO_SCRIPTS));
    ttCFindFile ff(atxtProjects[0]);

    // If we converted once before, then default to that script

    if (m_cszConvertScript.IsNonEmpty())
        m_comboScripts.Add(m_cszConvertScript);
    else
    {
        for (size_t pos = 1; !ff.IsValid() && atxtProjects[pos]; ++pos)
            ff.NewPattern(atxtProjects[pos]);
        if (ff.IsValid())
            m_comboScripts.Add((char*) ff);

        if (m_comboScripts.GetCount() < 1)  // no scripts found, let's check sub directories
        {
            ff.NewPattern("*.*");
            do
            {
                if (ff.IsDir())
                {
                    if (!ttIsValidFileChar(ff, 0))  // this will skip over . and ..
                        continue;
                    ttCStr cszDir((char*) ff);
                    cszDir.AppendFileName(atxtProjects[0]);
                    ttCFindFile ffFilter(cszDir);
                    for (size_t pos = 1; !ffFilter.IsValid() && atxtProjects[pos]; ++pos)
                    {
                        cszDir = ff;
                        cszDir.AppendFileName(atxtProjects[pos]);
                        ffFilter.NewPattern(cszDir);
                    }

                    if (ffFilter.IsValid())
                    {
                        cszDir = ff;
                        cszDir.AppendFileName(ffFilter);
                        m_comboScripts.Add(cszDir);
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
    csz.printf("%kn file%ks located", cFilesFound, cFilesFound);
    SetControlText(DLG_ID(IDTXT_FILES_FOUND), csz);

    if (m_comboScripts.GetCount() > 0)
    {
        m_comboScripts.SetCurSel();
        SetCheck(DLG_ID(IDRADIO_CONVERT));
    }
    else if (cFilesFound)
    {
        SetCheck(DLG_ID(IDRADIO_FILES));
    }
}

void CConvertDlg::OnBtnLocateScript()
{
    ttCFileDlg dlg(*this);
    dlg.SetFilter("Visual Studio|*.vcxproj;*.vcproj|CodeLite|*.project|CodeBlocks|*.cbp||");
    dlg.UseCurrentDirectory();
    dlg.RestoreDirectory();
    if (dlg.GetOpenName())
    {
        auto item = m_comboScripts.Add(dlg.GetFileName());
        m_comboScripts.SetCurSel(item);
        UnCheck(DLG_ID(IDRADIO_FILES));
        SetCheck(DLG_ID(IDRADIO_CONVERT));
        // TODO: [randalphwa - 4/2/2019] Need to decide how to handle IDEDIT_IN_DIR since this may no longer be correct
    }
}

void CConvertDlg::OnBtnChangeOut()  // change the directory to write .srcfiles to
{
    ttCDirDlg dlg;
    ttCStr    cszCWD;
    cszCWD.GetCWD();
    dlg.SetStartingDir(cszCWD);
    if (dlg.GetFolderName(*this))
    {
        ttCStr cszSrcFiles(dlg);
        cszSrcFiles.AppendFileName(".srcfiles.yaml");
        if (ttFileExists(cszSrcFiles))
        {
            if (ttMsgBox(GETSTRING(IDS_NINJA_SRCFILES_EXISTS), MB_YESNO) != IDYES)
                return;
        }
        SetControlText(DLG_ID(IDEDIT_OUT_DIR), dlg);
    }
}

void CConvertDlg::OnBtnChangeIn()
{
    ttCDirDlg dlg;
    ttCStr    cszCWD;
    cszCWD.GetCWD();
    dlg.SetStartingDir(cszCWD);
    if (dlg.GetFolderName(*this))
    {
        SetControlText(DLG_ID(IDEDIT_IN_DIR), dlg);
        ttChDir(dlg);
        ttCFindFile ff("*.cpp");
        size_t      cFilesFound = 0;
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
        ttChDir(cszCWD);  // restore our directory
        ttCStr csz;
        csz.printf("%kn file%ks located", cFilesFound, cFilesFound);
        SetControlText(DLG_ID(IDTXT_FILES_FOUND), csz);
        UnCheck(DLG_ID(IDRADIO_CONVERT));
        SetCheck(DLG_ID(IDRADIO_FILES));
    }
}

void CConvertDlg::OnOK(void)
{
    m_cszOutSrcFiles.GetWndText(GetDlgItem(DLG_ID(IDEDIT_OUT_DIR)));
    m_cszOutSrcFiles.AppendFileName(".srcfiles.yaml");
    m_cszDirSrcFiles.GetWndText(GetDlgItem(DLG_ID(IDEDIT_IN_DIR)));
    if (GetCheck(DLG_ID(IDRADIO_CONVERT)))
        m_cszConvertScript.GetWndText(GetDlgItem(DLG_ID(IDCOMBO_SCRIPTS)));
    else
        m_cszConvertScript.Delete();

    m_bCreateVsCode = GetCheck(DLG_ID(IDCHECK_VSCODE));
    m_bGitIgnore = GetCheck(DLG_ID(IDCHECK_IGNORE_ALL));

    if (m_cszConvertScript.IsNonEmpty())
    {
        if (!ttFileExists(m_cszConvertScript))
        {
            ttMsgBoxFmt(GETSTRING(IDS_NINJA_CANNOT_OPEN), MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
            CancelEnd();
            return;
        }
        if (!doConversion())
        {
            CancelEnd();
            return;
        }
    }
    else  // We get here if the user didn't specify anything to convert from.
    {
        if (!m_cSrcFiles.WriteNew(m_cszOutSrcFiles))
        {
            ttMsgBoxFmt(GETSTRING(IDS_NINJA_CANT_WRITE), MB_OK | MB_ICONWARNING, (char*) m_cszOutSrcFiles);
            CancelEnd();
        }
    }
}

void CConvertDlg::AddCodeLiteFiles(ttCXMLBranch* pParent)
{
    for (size_t child = 0; child < pParent->GetChildrenCount(); child++)
    {
        ttCXMLBranch* pFile = pParent->GetChildAt(child);
        if (ttIsSameStrI(pFile->GetName(), "File"))
        {
            if (isValidSrcFile(pFile->GetAttribute("Name")))
                m_cSrcFiles.m_lstSrcFiles += MakeSrcRelative(pFile->GetAttribute("Name"));
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

// This function first converts the file relative to the location of the build script, and then relative to the location
// of .srcfiles

char* CConvertDlg::MakeSrcRelative(const char* pszFile)
{
    ttASSERT(m_cszConvertScript.IsNonEmpty());

    if (m_cszScriptRoot.IsEmpty())
    {
        m_cszScriptRoot = (char*) m_cszConvertScript;
        m_cszScriptRoot.FullPathName();
        char* pszFilePortion = ttFindFilePortion(m_cszScriptRoot);
        ttASSERT_MSG(pszFilePortion, "No filename in m_cszScriptRoot--things will go badly without it.");
        if (pszFilePortion)
            *pszFilePortion = 0;

        // For GetFullPathName() to work properly on a file inside the script, we need to be in the same directory as
        // the script file

        ttChDir(m_cszScriptRoot);
    }

    if (m_cszOutRoot.IsEmpty())
    {
        m_cszOutRoot = (char*) m_cszOutSrcFiles;
        m_cszOutRoot.FullPathName();
        char* pszFilePortion = m_cszOutRoot.FindStrI(".srcfiles.yaml");
        if (pszFilePortion)
            *pszFilePortion = 0;
    }

    ttCStr cszFile(pszFile);
    cszFile.FullPathName();

    ttConvertToRelative(m_cszOutRoot, cszFile, m_cszRelative);
    return m_cszRelative;
}

bool CConvertDlg::doConversion(const char* pszInFile)
{
    ttASSERT_MSG(m_cszOutSrcFiles.IsNonEmpty(), "Need to set path to .srcfiles.yaml before calling doConversion()");

    if (pszInFile)
        m_cszConvertScript = pszInFile;
    const char* pszExt = ttStrChrR(m_cszConvertScript, '.');
    if (pszExt)
    {
        HRESULT hr = m_xml.ParseXmlFile(m_cszConvertScript);
        if (hr != S_OK)
        {
            ttMsgBoxFmt(GETSTRING(IDS_NINJA_PARSE_ERROR), MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
            return false;
        }

        bool bResult = false;
        if (ttIsSameStrI(pszExt, ".project"))
            bResult = ConvertCodeLite();
        else if (ttIsSameStrI(pszExt, ".cdb"))
            bResult = ConvertCodeBlocks();
        else if (ttIsSameStrI(pszExt, ".vcxproj"))
            bResult = ConvertVcxProj();
        else if (ttIsSameStrI(pszExt, ".vcproj"))
            bResult = ConvertVcProj();

        ttChDir(m_cszCWD);  // we may have changed directories during the conversion

        if (bResult)
        {
            ttCStr cszHdr, cszRelative;
            ttConvertToRelative(m_cszOutSrcFiles, m_cszConvertScript, cszRelative);

            cszHdr.printf("# Converted from %s", (char*) cszRelative);

            if (ttIsEmpty(m_cSrcFiles.GetOption(OPT_PROJECT)))
            {
                ttCStr cszProject(m_cszOutSrcFiles);
                char*  pszFilePortion = ttFindFilePortion(cszProject);
                if (pszFilePortion)
                    *pszFilePortion = 0;
                m_cSrcFiles.UpdateOption(OPT_PROJECT, ttFindFilePortion(cszProject));
            }

            if (!m_cSrcFiles.WriteNew(m_cszOutSrcFiles, cszHdr))
            {
                ttMsgBoxFmt(GETSTRING(IDS_NINJA_CANT_WRITE), MB_OK | MB_ICONWARNING, (char*) m_cszOutSrcFiles);
                return false;
            }
            return true;
        }
    }
    return false;
}
