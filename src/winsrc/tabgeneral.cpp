/////////////////////////////////////////////////////////////////////////////
// Name:      CTabGeneral
// Purpose:   IDTAB_GENERAL dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttTR.h>  // Function for translating strings

#include <ttdirdlg.h>  // ttCDirDlg

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
        SetCheck(DLG_ID(IDCHECK_64BIT));  // default to 64-bit builds
    else
    {
        SetCheck(DLG_ID(IDCHECK_32BIT));
        SetCheck(DLG_ID(IDCHECK_64BIT), m_pOpts->GetBoolOption(OPT_64BIT));
    }
}

void CTabGeneral::OnOK(void)
{
    ttCStr csz;

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_PROJ_NAME)));
    m_pOpts->UpdateOption(OPT_PROJECT, (char*) csz);

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
    dlg.SetTitle(_tt("Select 32-bit Target directory"));

    ttCStr cszDir;
    cszDir.GetWndText(GetDlgItem(DLG_ID(IDEDIT_DIR32)));
    cszDir.FullPathName();
    if (!ttDirExists(cszDir))  // SHCreateItemFromParsingName will fail if the folder doesn't already exist
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
    dlg.SetTitle(_tt("Select 64-bit target directory"));

    ttCStr cszDir;
    cszDir.GetWndText(GetDlgItem(DLG_ID(IDEDIT_DIR64)));
    cszDir.FullPathName();
    if (!ttDirExists(cszDir))  // SHCreateItemFromParsingName will fail if the folder doesn't already exist
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
    bool bx86Set = false;
    bool bx64Set = false;

    if (m_pOpts->GetOption(OPT_TARGET_DIR32))
    {
        SetControlText(DLG_ID(IDEDIT_DIR32), m_pOpts->GetOption(OPT_TARGET_DIR32));
        bx86Set = true;
    }
    if (m_pOpts->GetOption(OPT_TARGET_DIR64))
    {
        SetControlText(DLG_ID(IDEDIT_DIR64), m_pOpts->GetOption(OPT_TARGET_DIR64));
        bx64Set = true;
    }

    if (bx86Set && bx64Set)
        return;

    // Directory names don't actually need to exist, since Ninja will create them if they are missing. We just need
    // to figure out what names to use.

    ttCStr cszCWD;
    cszCWD.GetCWD();
    const char* pszCurFolder = ttFindFilePortion(cszCWD);

    bool bUseParent = (ttStrStrI(pszCurFolder, "src") || ttStrStrI(pszCurFolder, "source")) ? true : false;

    if (!bUseParent)
    {
        // If we aren't in the root of a .git repository, then look at the parent folder for the directories to
        // use. We check for a filename called .git in case this is a submodule.

        cszCWD.AppendFileName(m_pOpts->IsExeTypeLib() ? "../lib_x64" : "../bin_x64");
        if (ttDirExists(cszCWD) && !ttDirExists(".git") && !ttFileExists(".git"))
            bUseParent = true;
    }

    if (bUseParent)
    {
        if (!bx64Set)
            SetControlText(DLG_ID(IDEDIT_DIR64), m_pOpts->IsExeTypeLib() ? "../lib_x64" : "../bin_x64");

        if (!bx86Set)
            SetControlText(DLG_ID(IDEDIT_DIR32), m_pOpts->IsExeTypeLib() ? "../lib_x86" : "../bin_x86");
        return;
    }
    else
    {
        if (!bx64Set)
            SetControlText(DLG_ID(IDEDIT_DIR64), m_pOpts->IsExeTypeLib() ? "lib_x64" : "bin_x64");

        if (!bx86Set)
            SetControlText(DLG_ID(IDEDIT_DIR32), m_pOpts->IsExeTypeLib() ? "lib_x86" : "bin_x86");
        return;
    }
}
