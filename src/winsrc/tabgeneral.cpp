/////////////////////////////////////////////////////////////////////////////
// Name:      CTabGeneral
// Purpose:   IDTAB_GENERAL dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttcwd.h>     // cwd -- Class for storing and optionally restoring the current directory
#include <ttdirdlg.h>  // ttlib::DirDlg

#include "dlgoptions.h"

void CTabGeneral::OnBegin(void)
{
    CHECK_DLG_ID(IDEDIT_PROJ_NAME);
    CHECK_DLG_ID(IDEDIT_TARGET);
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
        if (cwd.filename().issameas("src", tt::CASE::either) || cwd.filename().issameas("source", tt::CASE::either))
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

    if (m_pOpts->hasOptValue(OPT::TARGET_DIR))
    {
        SetControlText(IDEDIT_TARGET, m_pOpts->getOptValue(OPT::TARGET_DIR32));
    }
}

void CTabGeneral::OnOK(void)
{
    ttlib::cstr csz;

    auto text = GetControlText(IDEDIT_PROJ_NAME);
    m_pOpts->setOptValue(OPT::PROJECT, text);
    GetControlText(IDEDIT_TARGET, text);
    m_pOpts->setOptValue(OPT::TARGET_DIR, text);

    if (GetCheck(IDRADIO_CONSOLE))
        m_pOpts->setOptValue(OPT::EXE_TYPE, "console");
    else if (GetCheck(IDRADIO_DLL))
        m_pOpts->setOptValue(OPT::EXE_TYPE, "dll");
    else if (GetCheck(IDRADIO_LIB))
        m_pOpts->setOptValue(OPT::EXE_TYPE, "lib");
    else
        m_pOpts->setOptValue(OPT::EXE_TYPE, "window");
}

void CTabGeneral::OnBtnDir()
{
    ttlib::DirDlg dlg;
    dlg.SetTitle(_tt(strIdTitleTargetDir));

    ttlib::cwd cwd(true);
    dlg.SetStartingDir(cwd);
    if (dlg.GetFolderName())
    {
        dlg.make_absolute();
        dlg.make_relative(cwd);
        SetControlText(IDEDIT_TARGET, dlg);
    }
}

void CTabGeneral::OnCheckLib()
{
    m_pOpts->setOptValue(OPT::EXE_TYPE, "lib");
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
    }
}
