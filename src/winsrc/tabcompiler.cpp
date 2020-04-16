/////////////////////////////////////////////////////////////////////////////
// Name:      CTabCompiler
// Purpose:   IDTAB_COMPILER dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttcwd.h>       // cwd -- Class for storing and optionally restoring the current directory
#include <ttdirdlg.h>    // DirDlg -- Class for displaying a dialog to select a directory
#include <ttopenfile.h>  // openfile -- Wrapper around Windows GetOpenFileName() API

#include "dlgoptions.h"

void CTabCompiler::OnBegin(void)
{
    CHECK_DLG_ID(IDC_RADIO_SPEED);
    CHECK_DLG_ID(IDC_RADIO_SPACE);
    CHECK_DLG_ID(IDEDIT_COMMON);
    CHECK_DLG_ID(IDEDIT_DEBUG);
    CHECK_DLG_ID(IDEDIT_INCDIRS);
    CHECK_DLG_ID(IDEDIT_PCH);
    CHECK_DLG_ID(IDEDIT_PCH_CPP);
    CHECK_DLG_ID(IDEDIT_RELEASE);
    CHECK_DLG_ID(IDRADIO_WARN1);
    CHECK_DLG_ID(IDRADIO_WARN2);
    CHECK_DLG_ID(IDRADIO_WARN3);
    CHECK_DLG_ID(IDRADIO_WARN4);

    EnableShadeBtns();
    SetCheck(m_pOpts->IsOptimizeSpeed() ? IDC_RADIO_SPEED : IDC_RADIO_SPACE);

    auto warnLevel = ttlib::atoi(m_pOpts->getOptValue(OPT::WARN));

    switch (warnLevel)
    {
        case 1:
            SetCheck(IDRADIO_WARN1);
            break;
        case 2:
            SetCheck(IDRADIO_WARN2);
            break;
        case 3:
            SetCheck(IDRADIO_WARN3);
            break;
        case 4:
        default:
            SetCheck(IDRADIO_WARN4);
            break;
    }

    if (m_pOpts->hasOptValue(OPT::PCH))
        SetControlText(IDEDIT_PCH, m_pOpts->getOptValue(OPT::PCH));
    if (m_pOpts->hasOptValue(OPT::PCH_CPP))
        SetControlText(IDEDIT_PCH_CPP, m_pOpts->getOptValue(OPT::PCH_CPP));
    if (m_pOpts->hasOptValue(OPT::INC_DIRS))
        SetControlText(IDEDIT_INCDIRS, m_pOpts->getOptValue(OPT::INC_DIRS));

    if (m_pOpts->hasOptValue(OPT::CFLAGS_CMN))
    {
        SetControlText(IDEDIT_COMMON, m_pOpts->getOptValue(OPT::CFLAGS_CMN));
        if (m_pOpts->getOptValue(OPT::CFLAGS_CMN).contains("-Zc:__cplusplus"))
            DisableControl(IDBTN_CPLUSPLUS);
        if (m_pOpts->getOptValue(OPT::CFLAGS_CMN).contains("-std:c"))
            DisableControl(IDBTN_STD);
    }
    if (m_pOpts->hasOptValue(OPT::CFLAGS_REL))
        SetControlText(IDEDIT_RELEASE, m_pOpts->getOptValue(OPT::CFLAGS_REL));
    if (m_pOpts->hasOptValue(OPT::CFLAGS_DBG))
        SetControlText(IDEDIT_DEBUG, m_pOpts->getOptValue(OPT::CFLAGS_DBG));
}

void CTabCompiler::OnOK(void)
{
    ttlib::cstr csz;

    csz.GetWndText(gethwnd(IDEDIT_COMMON));
    m_pOpts->setOptValue(OPT::CFLAGS_CMN, csz);

    csz.GetWndText(gethwnd(IDEDIT_RELEASE));
    m_pOpts->setOptValue(OPT::CFLAGS_REL, csz);

    csz.GetWndText(gethwnd(IDEDIT_DEBUG));
    m_pOpts->setOptValue(OPT::CFLAGS_DBG, csz);

    csz.GetWndText(gethwnd(IDEDIT_INCDIRS));
    m_pOpts->setOptValue(OPT::INC_DIRS, csz);

    csz.GetWndText(gethwnd(IDEDIT_PCH));
    if (csz.empty())
        m_pOpts->setOptValue(OPT::PCH, "none");
    else
        m_pOpts->setOptValue(OPT::PCH, csz);

    // We let WriteSrcFiles handle the case where the base names of PCH and PCH_CPP are identical

    csz.GetWndText(gethwnd(IDEDIT_PCH_CPP));
    if (csz.empty())
        m_pOpts->setOptValue(OPT::PCH_CPP, "none");
    else
        m_pOpts->setOptValue(OPT::PCH_CPP, csz);

    if (GetCheck(IDC_RADIO_SPEED))
        m_pOpts->setOptValue(OPT::OPTIMIZE, "speed");
    else
        m_pOpts->setOptValue(OPT::OPTIMIZE, "space");

    if (GetCheck(IDRADIO_WARN1))
        m_pOpts->setOptValue(OPT::WARN, "1");
    else if (GetCheck(IDRADIO_WARN2))
        m_pOpts->setOptValue(OPT::WARN, "2");
    else if (GetCheck(IDRADIO_WARN3))
        m_pOpts->setOptValue(OPT::WARN, "3");
    else if (GetCheck(IDRADIO_WARN4))
        m_pOpts->setOptValue(OPT::WARN, "4");
}

void CTabCompiler::OnBtnChangePch()
{
    ttlib::openfile fdlg(*this);
    fdlg.SetFilter("Header Files|*.h;*.hh;*.hpp;*.hxx||");
    ttlib::cwd cwd;

    fdlg.RestoreDirectory();
    if (fdlg.GetOpenName())
    {
        ttlib::cstr cszOrg, cszCpp;
        cszOrg.GetWndText(gethwnd(IDEDIT_PCH));
        cszCpp.GetWndText(gethwnd(IDEDIT_PCH_CPP));

        ttlib::cstr cszRelPath(fdlg.filename());
        cszRelPath.make_relative(cwd);
        SetControlText(IDEDIT_PCH, cszRelPath);

        if (cszCpp.fileExists())  // if the current source file exists, assume that's what the user wants
            return;

        // The default behaviour is to use the same base name for the C++ source file as the precompiled header
        // file. So, if they are the same, then change the source file name to match (provided the file actually
        // exists).

        cszOrg.remove_extension();
        cszCpp.remove_extension();

        if (cszOrg.issameprefix(cszCpp, tt::CASE::either))
        {
            cszCpp = cszRelPath;
            cszCpp.replace_extension(".cpp");
            if (cszCpp.fileExists())
            {
                SetControlText(IDEDIT_PCH_CPP, cszCpp);
                return;
            }
            cszCpp.replace_extension(".cxx");
            if (cszCpp.fileExists())
            {
                SetControlText(IDEDIT_PCH_CPP, cszCpp);
                return;
            }
            cszCpp.replace_extension(".cc");
            if (cszCpp.fileExists())
            {
                SetControlText(IDEDIT_PCH_CPP, cszCpp);
                return;
            }

            ttlib::MsgBox(
                cszRelPath +
                _tt(" does not have a matching C++ source file -- precompiled header will fail without it!"));
        }
    }
}

void CTabCompiler::OnBtnPchCpp()
{
    ttlib::openfile fdlg(*this);
    fdlg.SetFilter("C++ Files|*.cpp;*.cc;*.cxx||");
    fdlg.RestoreDirectory();
    if (fdlg.GetOpenName())
    {
        ttlib::cwd cwd;
        ttlib::cstr cszRelPath(fdlg.filename());
        cszRelPath.make_relative(cwd);
        SetControlText(IDEDIT_PCH_CPP, cszRelPath);
    }
}

void CTabCompiler::OnBtnCplusplus()
{
    ttlib::cstr csz;

    csz.GetWndText(gethwnd(IDEDIT_COMMON));
    if (!csz.contains("-Zc:__cplusplus", tt::CASE::either))
    {
        if (!csz.empty())
            csz += " ";
        csz += "-Zc:__cplusplus";
        SetControlText(IDEDIT_COMMON, csz);
        DisableControl(IDBTN_CPLUSPLUS);
    }
}

void CTabCompiler::OnBtnStd()
{
    ttlib::cstr csz;

    csz.GetWndText(gethwnd(IDEDIT_COMMON));
    if (!csz.contains("-std:c", tt::CASE::either))
    {
        if (!csz.empty())
            csz += " ";
        csz += "-std:c++17";
        SetControlText(IDEDIT_COMMON, csz);
        DisableControl(IDBTN_STD);
    }
}

#if 0
// REVIEW: [KeyWorks - 09-25-2019] This will lock up the app. The problem is that the parent dialog is being launched
// from a Modeless dialog with a message loop purely to handle the dialog. Apparently the lack of a standard message
// loop is causing the problem. What will happen is you will select a directory and click ok, and it will simply show
// the same dialog box again. This time when you click ok, the application crashes. Note that dlg.GetFolderName() never
// returns.

void CTabCompiler::OnBtnAddInclude()
{
    ttCDirDlg dlg;
    ttlib::cwd cwd;

    dlg.SetStartingDir(cszCWD);
    if (dlg.GetFolderName((HWND) *m_pOpts))
    {
        ttlib::cstr csz;
        csz.GetWndText(gethwnd(IDEDIT_INCDIRS));
        csz += ";";
        csz += (char*) dlg.GetFolderName();
        SetControlText(IDEDIT_INCDIRS, csz);
    }
}

#endif
