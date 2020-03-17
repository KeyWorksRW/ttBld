/////////////////////////////////////////////////////////////////////////////
// Name:      CTabGeneral
// Purpose:   IDTAB_GENERAL dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttTR.h>  // Function for translating strings

#include <ttdirdlg.h>  // ttCDirDlg

#include "dlgoptions.h"

void CTabGeneral::OnBegin(void)
{
    CHECK_DLG_ID(IDEDIT_PROJ_NAME);
    CHECK_DLG_ID(IDRADIO_CONSOLE);
    CHECK_DLG_ID(IDRADIO_DLL);
    CHECK_DLG_ID(IDRADIO_LIB);
    CHECK_DLG_ID(IDRADIO_NORMAL);

    if (m_pOpts->hasOptValue(OPT::PROJECT))
        SetControlText(IDEDIT_PROJ_NAME, m_pOpts->GetProjectName());
    else
    {
        // If project name not specified, use the current directory name as the project name.
        ttlib::cstr cwd;
        cwd.assignCwd();

        // If current directory is src or source, then the project name should be the parent directory name.
        if (cwd.filename().issameas("src", tt::CASE::either) ||
            cwd.filename().issameas("source", tt::CASE::either))
            cwd.remove_filename();

        SetControlText(IDEDIT_PROJ_NAME, cwd.filename());
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
    ttlib::cstr csz;

    csz.GetWndText(gethwnd(IDEDIT_PROJ_NAME));
    m_pOpts->setOptValue(OPT::PROJECT, csz);

    if (GetCheck(IDRADIO_CONSOLE))
        m_pOpts->setOptValue(OPT::EXE_TYPE, "console");
    else if (GetCheck(IDRADIO_DLL))
        m_pOpts->setOptValue(OPT::EXE_TYPE, "dll");
    else if (GetCheck(IDRADIO_LIB))
        m_pOpts->setOptValue(OPT::EXE_TYPE, "lib");
    else
        m_pOpts->setOptValue(OPT::EXE_TYPE, "window");

    m_pOpts->setBoolOptValue(OPT::BIT32, GetCheck(IDCHECK_32BIT));
    if (GetCheck(IDCHECK_32BIT))
    {
        csz.GetWndText(gethwnd(IDEDIT_DIR32));
        if (!csz.empty())
            m_pOpts->setOptValue(OPT::TARGET_DIR32, csz);
    }

    m_pOpts->setBoolOptValue(OPT::BIT64, GetCheck(IDCHECK_64BIT));
    if (GetCheck(IDCHECK_64BIT))
    {
        csz.GetWndText(gethwnd(IDEDIT_DIR64));
        if (!csz.empty())
            m_pOpts->setOptValue(OPT::TARGET_DIR64, csz);
    }
}

void CTabGeneral::OnBtnDir32()
{
// REVIEW: [KeyWorks - 03-17-2020] On hold -- currently this blows up, probably because we aren't a proper
// Windows application.
#if 0
    ttCDirDlg dlg;
    dlg.SetTitle(_tt("Select 32-bit Target directory"));

    ttlib::cstr cszDir;
    cszDir.GetWndText(gethwnd(IDEDIT_DIR32));
    cszDir.make_absolute();
    if (!cszDir.dirExists())  // SHCreateItemFromParsingName will fail if the folder doesn't already exist
        cszDir.assignCwd();

    dlg.SetStartingDir(cszDir);
    if (dlg.GetFolderName(*this))
    {
        ttCStr cszCWD;
        cszCWD.GetCWD();
        ttConvertToRelative(cszCWD, dlg, cszDir);
        SetControlText(IDEDIT_DIR32, cszDir);
    }
#endif
}

void CTabGeneral::OnBtnDir64()
{
// REVIEW: [KeyWorks - 03-17-2020] On hold -- currently this blows up, probably because we aren't a proper
// Windows application.
#if 0
    ttCDirDlg dlg;
    dlg.SetTitle(_tt("Select 64-bit target directory"));

    ttCStr cszDir;
    cszDir.GetWndText(gethwnd(IDEDIT_DIR64));
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
#endif
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
    bool is_x86 = false;
    bool is_x64 = false;

    if (m_pOpts->hasOptValue(OPT::TARGET_DIR32))
    {
        SetControlText(IDEDIT_DIR32, m_pOpts->getOptValue(OPT::TARGET_DIR32));
        is_x86 = true;
    }
    if (m_pOpts->hasOptValue(OPT::TARGET_DIR64))
    {
        SetControlText(IDEDIT_DIR64, m_pOpts->getOptValue(OPT::TARGET_DIR64));
        is_x64 = true;
    }

    if (is_x86 && is_x64)
        return;

    // Directory names don't actually need to exist, since Ninja will create them if they are missing. We just need
    // to figure out what names to use.

    ttlib::cstr cwd;
    cwd.assignCwd();

    bool UseParent =
        (cwd.filename().issameas("src", tt::CASE::either) || cwd.filename().issameas("src", tt::CASE::either));

    if (!UseParent)
    {
        // If we aren't in the root of a .git repository, then look at the parent folder for the directories to
        // use. We check for a filename called .git in case this is a submodule.

        cwd.append_filename(m_pOpts->IsExeTypeLib() ? "../lib_x64" : "../bin_x64");
        if (cwd.dirExists() && !ttlib::dirExists(".git") && !ttlib::dirExists(".git"))
            UseParent = true;
    }

    if (UseParent)
    {
        if (!is_x64)
            SetControlText(IDEDIT_DIR64, m_pOpts->IsExeTypeLib() ? "../lib_x64" : "../bin_x64");

        if (!is_x86)
            SetControlText(IDEDIT_DIR32, m_pOpts->IsExeTypeLib() ? "../lib_x86" : "../bin_x86");
        return;
    }
    else
    {
        if (!is_x64)
            SetControlText(IDEDIT_DIR64, m_pOpts->IsExeTypeLib() ? "lib_x64" : "bin_x64");

        if (!is_x86)
            SetControlText(IDEDIT_DIR32, m_pOpts->IsExeTypeLib() ? "lib_x86" : "bin_x86");
        return;
    }
}
