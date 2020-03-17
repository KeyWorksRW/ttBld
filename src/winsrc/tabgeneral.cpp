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
    // CHECK_DLG_ID(IDCHECK_);
    // CHECK_DLG_ID(IDEDIT_DIR);
    CHECK_DLG_ID(IDEDIT_PROJ_NAME);
    CHECK_DLG_ID(IDRADIO_CONSOLE);
    CHECK_DLG_ID(IDRADIO_DLL);
    CHECK_DLG_ID(IDRADIO_LIB);
    CHECK_DLG_ID(IDRADIO_NORMAL);

    if (m_pOpts->hasOptValue(OPT::PROJECT))
        setControlText(IDEDIT_PROJ_NAME, m_pOpts->GetProjectName());
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
        SetControlText(IDEDIT_PROJ_NAME, pszProject);
    }

    if (m_pOpts->IsExeTypeConsole())
        SetCheck(IDRADIO_CONSOLE);
    else if (m_pOpts->IsExeTypeDll())
        SetCheck(IDRADIO_DLL);
    else if (m_pOpts->IsExeTypeLib())
        SetCheck(IDRADIO_LIB);
    else
        SetCheck(IDRADIO_NORMAL);

    SetTargetDirs();

    if (!m_pOpts->isOptTrue(OPT::BIT32))
        SetCheck(IDCHECK_64BIT);  // default to 64-bit builds
    else
    {
        SetCheck(IDCHECK_32BIT);
        SetCheck(IDCHECK_64BIT, m_pOpts->isOptTrue(OPT::BIT64));
    }
}

void CTabGeneral::OnOK(void)
{
    ttCStr csz;

    csz.GetWndText(GetDlgItem(IDEDIT_PROJ_NAME));
    m_pOpts->setOptValue(OPT::PROJECT, (char*) csz);

    if (GetCheck(IDRADIO_CONSOLE))
        m_pOpts->setOptValue(OPT::EXE_TYPE, "console");
    else if (GetCheck(IDRADIO_DLL))
        m_pOpts->setOptValue(OPT::EXE_TYPE, "dll");
    else if (GetCheck(IDRADIO_LIB))
        m_pOpts->setOptValue(OPT::EXE_TYPE, "lib");
    else
        m_pOpts->setOptValue(OPT::EXE_TYPE, "window");

    m_pOpts->setOptValue(OPT::BIT32, GetCheck(IDCHECK_32BIT));
    if (GetCheck(IDCHECK_32BIT))
    {
        csz.GetWndText(GetDlgItem(IDEDIT_DIR32));
        if (csz.IsNonEmpty())
            m_pOpts->setOptValue(OPT::TARGET_DIR32, (char*) csz);
    }

    m_pOpts->setOptValue(OPT::BIT64, GetCheck(IDCHECK_64BIT));
    if (GetCheck(IDCHECK_64BIT))
    {
        csz.GetWndText(GetDlgItem(IDEDIT_DIR64));
        if (csz.IsNonEmpty())
            m_pOpts->setOptValue(OPT::TARGET_DIR64, (char*) csz);
    }
}

void CTabGeneral::OnBtnDir32()
{
    ttCDirDlg dlg;
    dlg.SetTitle(_tt("Select 32-bit Target directory"));

    ttCStr cszDir;
    cszDir.GetWndText(GetDlgItem(IDEDIT_DIR32));
    cszDir.FullPathName();
    if (!ttDirExists(cszDir))  // SHCreateItemFromParsingName will fail if the folder doesn't already exist
        cszDir.GetCWD();

    dlg.SetStartingDir(cszDir);
    if (dlg.GetFolderName(*this))
    {
        ttCStr cszCWD;
        cszCWD.GetCWD();
        ttConvertToRelative(cszCWD, dlg, cszDir);
        SetControlText(IDEDIT_DIR32, cszDir);
    }
}

void CTabGeneral::OnBtnDir64()
{
    ttCDirDlg dlg;
    dlg.SetTitle(_tt("Select 64-bit target directory"));

    ttCStr cszDir;
    cszDir.GetWndText(GetDlgItem(IDEDIT_DIR64));
    cszDir.FullPathName();
    if (!ttDirExists(cszDir))  // SHCreateItemFromParsingName will fail if the folder doesn't already exist
        cszDir.GetCWD();

    dlg.SetStartingDir(cszDir);
    if (dlg.GetFolderName(*this))
    {
        ttCStr cszCWD;
        cszCWD.GetCWD();
        ttConvertToRelative(cszCWD, dlg, cszDir);
        SetControlText(IDEDIT_DIR64, cszDir);
    }
}

void CTabGeneral::OnCheckLib()
{
    m_pOpts->setOptValue(OPT::EXE_TYPE, "lib");
    SetTargetDirs();
}

void CTabGeneral::OnCheckExe()
{
    if (m_pOpts->IsExeTypeLib())
    {
        if (GetCheck(IDRADIO_CONSOLE))
            m_pOpts->setOptValue(OPT::EXE_TYPE, "console");
        else if (GetCheck(IDRADIO_DLL))
            m_pOpts->setOptValue(OPT::EXE_TYPE, "dll");
        else
            m_pOpts->setOptValue(OPT::EXE_TYPE, "window");
        SetTargetDirs();
    }
}

void CTabGeneral::SetTargetDirs()
{
    bool bx86Set = false;
    bool bx64Set = false;

    if (m_pOpts->hasOptValue(OPT::TARGET_DIR32))
    {
        setControlText(IDEDIT_DIR32, m_pOpts->getOptValue(OPT::TARGET_DIR32));
        bx86Set = true;
    }
    if (m_pOpts->hasOptValue(OPT::TARGET_DIR64))
    {
        setControlText(IDEDIT_DIR64, m_pOpts->getOptValue(OPT::TARGET_DIR64));
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
            SetControlText(IDEDIT_DIR64, m_pOpts->IsExeTypeLib() ? "../lib_x64" : "../bin_x64");

        if (!bx86Set)
            SetControlText(IDEDIT_DIR32, m_pOpts->IsExeTypeLib() ? "../lib_x86" : "../bin_x86");
        return;
    }
    else
    {
        if (!bx64Set)
            SetControlText(IDEDIT_DIR64, m_pOpts->IsExeTypeLib() ? "lib_x64" : "bin_x64");

        if (!bx86Set)
            SetControlText(IDEDIT_DIR32, m_pOpts->IsExeTypeLib() ? "lib_x86" : "bin_x86");
        return;
    }
}
