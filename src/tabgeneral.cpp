/////////////////////////////////////////////////////////////////////////////
// Name:      CTabGeneral
// Purpose:   IDTAB_GENERAL dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttdirdlg.h>   // ttCDirDlg

#include "dlgoptions.h"

void CTabGeneral::OnBegin(void)
{
    if (m_pOpts->GetProjectName())
        SetControlText(DLG_ID(IDEDIT_PROJ_NAME), m_pOpts->GetProjectName());
    else
    {
        char szCwd[MAX_PATH];
        GetCurrentDirectoryA(sizeof(szCwd), szCwd);
        char* pszProject = ttFindFilePortion(szCwd);

        // If cwd is "src", then use the parent directory as the project name

        if (ttIsSameStrI(pszProject, "src"))
        {
            pszProject--;
            *pszProject = 0;
            pszProject = ttFindFilePortion(szCwd);
        }
        SetControlText(DLG_ID(IDEDIT_PROJ_NAME), pszProject);
    }

    if (m_pOpts->IsExeTypeConsole())
        SetCheck(DLG_ID(IDRADIO_CONSOLE));
    else if (m_pOpts->IsExeTypeDll())
        SetCheck(DLG_ID(IDRADIO_DLL));
    else if (m_pOpts->IsExeTypeLib())
        SetCheck(DLG_ID(IDRADIO_LIB));
    else
        SetCheck(DLG_ID(IDRADIO_NORMAL));

    SetTargetDirs();

    if (!m_pOpts->GetBoolOption(OPT_32BIT))
        SetCheck(DLG_ID(IDCHECK_64BIT));    // default to 64-bit builds
    else
    {
        SetCheck(DLG_ID(IDCHECK_32BIT));
        SetCheck(DLG_ID(IDCHECK_64BIT), m_pOpts->GetBoolOption(OPT_64BIT));
    }

    if (ttIsNonEmpty(m_pOpts->GetXgetFlags()))
        SetControlText(DLG_ID(IDEDIT_XGETTEXT), m_pOpts->GetXgetFlags());
}

void CTabGeneral::OnOK(void)
{
    ttCStr csz;

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_PROJ_NAME)));
    m_pOpts->UpdateOption(OPT_PROJECT, (char*) csz);

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_XGETTEXT)));
    if (csz.IsNonEmpty() || m_pOpts->GetXgetFlags())
        m_pOpts->UpdateOption(OPT_XGET_FLAGS, (char*) csz);

    if (GetCheck(DLG_ID(IDRADIO_CONSOLE)))
        m_pOpts->UpdateOption(OPT_EXE_TYPE, "console");
    else if (GetCheck(DLG_ID(IDRADIO_DLL)))
        m_pOpts->UpdateOption(OPT_EXE_TYPE, "dll");
    else if (GetCheck(DLG_ID(IDRADIO_LIB)))
        m_pOpts->UpdateOption(OPT_EXE_TYPE, "lib");
    else
        m_pOpts->UpdateOption(OPT_EXE_TYPE, "window");

    m_pOpts->UpdateOption(OPT_32BIT, GetCheck(DLG_ID(IDCHECK_32BIT)));
    if (GetCheck(DLG_ID(IDCHECK_32BIT)))
    {
        csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_DIR32)));
        if (csz.IsNonEmpty())
            m_pOpts->UpdateOption(OPT_TARGET_DIR32, (char*) csz);
    }

    m_pOpts->UpdateOption(OPT_64BIT, GetCheck(DLG_ID(IDCHECK_64BIT)));
    if (GetCheck(DLG_ID(IDCHECK_64BIT)))
    {
        csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_DIR64)));
        if (csz.IsNonEmpty())
            m_pOpts->UpdateOption(OPT_TARGET_DIR64, (char*) csz);
    }
}

void CTabGeneral::OnBtnDir32()
{
    ttCDirDlg dlg;
    dlg.SetTitle(GETSTRING(IDS_NINJA_32BIT_DIR));

    ttCStr cszDir;
    cszDir.GetWndText(GetDlgItem(DLG_ID(IDEDIT_DIR32)));
    cszDir.FullPathName();
    if (!ttDirExists(cszDir))   // SHCreateItemFromParsingName will fail if the folder doesn't already exist
        cszDir.GetCWD();

    dlg.SetStartingDir(cszDir);
    if (dlg.GetFolderName(*this))
    {
        ttCStr cszCWD;
        cszCWD.GetCWD();
        ttConvertToRelative(cszCWD, dlg, cszDir);
        SetControlText(DLG_ID(IDEDIT_DIR32), cszDir);
    }
}

void CTabGeneral::OnBtnDir64()
{
    ttCDirDlg dlg;
    dlg.SetTitle(GETSTRING(IDS_NINJA_64BIT_DIR));

    ttCStr cszDir;
    cszDir.GetWndText(GetDlgItem(DLG_ID(IDEDIT_DIR64)));
    cszDir.FullPathName();
    if (!ttDirExists(cszDir))   // SHCreateItemFromParsingName will fail if the folder doesn't already exist
        cszDir.GetCWD();

    dlg.SetStartingDir(cszDir);
    if (dlg.GetFolderName(*this))
    {
        ttCStr cszCWD;
        cszCWD.GetCWD();
        ttConvertToRelative(cszCWD, dlg, cszDir);
        SetControlText(DLG_ID(IDEDIT_DIR64), cszDir);
    }
}

void CTabGeneral::OnCheckLib()
{
    m_pOpts->UpdateOption(OPT_EXE_TYPE, "lib");
    SetTargetDirs();
}

void CTabGeneral::OnCheckExe()
{
    if (m_pOpts->IsExeTypeLib())
    {
        if (GetCheck(DLG_ID(IDRADIO_CONSOLE)))
            m_pOpts->UpdateOption(OPT_EXE_TYPE, "console");
        else if (GetCheck(DLG_ID(IDRADIO_DLL)))
            m_pOpts->UpdateOption(OPT_EXE_TYPE, "dll");
        else
            m_pOpts->UpdateOption(OPT_EXE_TYPE, "window");
        SetTargetDirs();
    }
}

void CTabGeneral::SetTargetDirs()
{
    // Start by setting default directories

    ttCStr cszDir64, cszDir32;

    ttCStr cszCWD;
    cszCWD.GetCWD();
    bool bSrcDir = ttStrStrI(ttFindFilePortion(cszCWD), "src") ? true : false;
    if (!bSrcDir)
    {
        cszCWD.AppendFileName(m_pOpts->IsExeTypeLib() ? "../lib" : "../bin");
        if (ttDirExists(cszCWD))
            bSrcDir = true;
    }

    if (bSrcDir)
    {
        cszDir64 = m_pOpts->IsExeTypeLib() ? "../lib" : "../bin";
        ttCStr cszTmp(cszDir64);
        cszTmp += "64";
        if (ttDirExists(cszTmp))      // if there is a ../lib64 or ../bin64, then use that
        {
            cszDir64 = cszTmp;
            cszDir32 = m_pOpts->IsExeTypeLib() ? "../lib" : "../bin";
            cszTmp = cszDir32;
            cszTmp += "32";
            if (ttDirExists(cszTmp))
                cszDir32 = cszTmp;
        }
        else
            cszDir32 = m_pOpts->IsExeTypeLib() ? "../lib32" : "../bin32";
    }
    else
    {
        cszDir64 = m_pOpts->IsExeTypeLib() ? "lib" : "bin";
        ttCStr cszTmp(cszDir64);
        cszTmp += "64";
        if (ttDirExists(cszTmp))      // if there is a lib64 or bin64, then use that
        {
            cszDir64 = cszTmp;
            cszDir32 = m_pOpts->IsExeTypeLib() ? "lib" : "bin";
            cszTmp = cszDir32;
            cszTmp += "32";
            if (ttDirExists(cszTmp))
                cszDir32 = cszTmp;
        }
        else
            cszDir32 = m_pOpts->IsExeTypeLib() ? "lib32" : "bin32";
    }

    if (m_pOpts->GetOption(OPT_TARGET_DIR32))
        SetControlText(DLG_ID(IDEDIT_DIR32), m_pOpts->GetOption(OPT_TARGET_DIR32));
    else
        SetControlText(DLG_ID(IDEDIT_DIR32), cszDir32);

    if (m_pOpts->GetOption(OPT_TARGET_DIR64))
        SetControlText(DLG_ID(IDEDIT_DIR64), m_pOpts->GetOption(OPT_TARGET_DIR64));
    else
        SetControlText(DLG_ID(IDEDIT_DIR64), cszDir64);
}

#include "dlggettext.h"    // CDlgGetText

void CTabGeneral::OnMore()
{
    CDlgGetText dlg;

}
