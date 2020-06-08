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

    fdlg.RestoreDirectory();
    if (fdlg.GetOpenName())
    {
        ttlib::cwd cwd;
        fdlg.filename().make_relative(cwd);
        fdlg.filename().backslashestoforward();
        SetControlText(IDEDIT_PCH, fdlg.filename());

        auto pch_cpp = GetControlText(IDEDIT_PCH_CPP);
        if (pch_cpp.size())
            return;
        pch_cpp = fdlg.filename();
        if (pch_cpp.replace_extension(".cpp"); pch_cpp.fileExists())
            SetControlText(IDEDIT_PCH_CPP, pch_cpp);
        else if (pch_cpp.replace_extension(".ccc"); pch_cpp.fileExists())
            SetControlText(IDEDIT_PCH_CPP, pch_cpp);
        else if (pch_cpp.replace_extension(".cxx"); pch_cpp.fileExists())
            SetControlText(IDEDIT_PCH_CPP, pch_cpp);
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
        fdlg.filename().make_relative(cwd);
        fdlg.filename().backslashestoforward();
        SetControlText(IDEDIT_PCH_CPP, fdlg.filename());
    }
}

void CTabCompiler::OnBtnAddInclude()
{
    ttlib::DirDlg dlg;
    dlg.SetTitle(_tt(stdIdTitleInclude));

    ttlib::cwd cwd(true);
    dlg.SetStartingDir(cwd);
    if (dlg.GetFolderName())
    {
        dlg.make_absolute();
        dlg.make_relative(cwd);
        dlg.backslashestoforward();

        if (auto olddirs = GetControlText(IDEDIT_INCDIRS); !olddirs.contains(dlg))
        {
            if (olddirs.size() && olddirs.back() != ';')
                olddirs += ';';
            olddirs += dlg;
            SetControlText(IDEDIT_INCDIRS, olddirs);
        }
    }
}
