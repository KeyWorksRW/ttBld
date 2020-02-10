/////////////////////////////////////////////////////////////////////////////
// Name:      CTabCompiler
// Purpose:   IDTAB_COMPILER dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttfiledlg.h>  // ttCFileDlg -- Wrapper around Windows GetOpenFileName() API
#include <ttdirdlg.h>   // ttCDirDlg -- Class for displaying a dialog to select a directory

#include "dlgoptions.h"

void CTabCompiler::OnBegin(void)
{
    EnableShadeBtns();
    SetCheck(DLG_ID(m_pOpts->IsOptimizeSpeed() ? IDC_RADIO_SPEED : IDC_RADIO_SPACE));

    ptrdiff_t warnLevel = m_pOpts->GetOption(OPT_WARN_LEVEL) ? ttAtoi(m_pOpts->GetOption(OPT_WARN_LEVEL)) : 4;

    switch (warnLevel)
    {
        case 1:
            SetCheck(DLG_ID(IDRADIO_WARN1));
            break;
        case 2:
            SetCheck(DLG_ID(IDRADIO_WARN2));
            break;
        case 3:
            SetCheck(DLG_ID(IDRADIO_WARN3));
            break;
        case 4:
        default:
            SetCheck(DLG_ID(IDRADIO_WARN4));
            break;
    }

    if (m_pOpts->GetPchHeader())
        SetControlText(DLG_ID(IDEDIT_PCH), m_pOpts->GetPchHeader());
    if (m_pOpts->GetPchCpp())
        SetControlText(DLG_ID(IDEDIT_PCH_CPP), m_pOpts->GetPchCpp());
    if (m_pOpts->GetOption(OPT_INC_DIRS))
        SetControlText(DLG_ID(IDEDIT_INCDIRS), m_pOpts->GetOption(OPT_INC_DIRS));

    if (m_pOpts->GetOption(OPT_CFLAGS_CMN))
    {
        SetControlText(DLG_ID(IDEDIT_COMMON), m_pOpts->GetOption(OPT_CFLAGS_CMN));
        if (ttStrStrI(m_pOpts->GetOption(OPT_CFLAGS_CMN), "-Zc:__cplusplus"))
            DisableControl(IDBTN_CPLUSPLUS);
        if (ttStrStrI(m_pOpts->GetOption(OPT_CFLAGS_CMN), "-std:c"))
            DisableControl(IDBTN_STD);
    }
    if (m_pOpts->GetOption(OPT_CFLAGS_REL))
        SetControlText(DLG_ID(IDEDIT_RELEASE), m_pOpts->GetOption(OPT_CFLAGS_REL));
    if (m_pOpts->GetOption(OPT_CFLAGS_DBG))
        SetControlText(DLG_ID(IDEDIT_DEBUG), m_pOpts->GetOption(OPT_CFLAGS_DBG));
}

void CTabCompiler::OnOK(void)
{
    ttCStr csz;

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_COMMON)));
    m_pOpts->UpdateOption(OPT_CFLAGS_CMN, csz.IsEmpty() ? nullptr : (char*) csz);

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_RELEASE)));
    m_pOpts->UpdateOption(OPT_CFLAGS_REL, csz.IsEmpty() ? nullptr : (char*) csz);

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_DEBUG)));
    m_pOpts->UpdateOption(OPT_CFLAGS_DBG, csz.IsEmpty() ? nullptr : (char*) csz);

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_INCDIRS)));
    m_pOpts->UpdateOption(OPT_INC_DIRS, csz.IsEmpty() ? nullptr : (char*) csz);

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_PCH)));
    if (csz.IsEmpty())
        m_pOpts->UpdateOption(OPT_PCH, "none");
    else
        m_pOpts->UpdateOption(OPT_PCH, (char*) csz);

    // We let WriteSrcFiles handle the case where the base names of PCH and PCH_CPP are identical

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_PCH_CPP)));
    if (csz.IsEmpty())
        m_pOpts->UpdateOption(OPT_PCH_CPP, "none");
    else
        m_pOpts->UpdateOption(OPT_PCH_CPP, (char*) csz);

    if (GetCheck(DLG_ID(IDC_RADIO_SPEED)))
        m_pOpts->UpdateOption(OPT_OPTIMIZE, "speed");
    else
        m_pOpts->UpdateOption(OPT_OPTIMIZE, "space");

    if (GetCheck(DLG_ID(IDRADIO_WARN1)))
        m_pOpts->UpdateOption(OPT_WARN_LEVEL, "1");
    else if (GetCheck(DLG_ID(IDRADIO_WARN2)))
        m_pOpts->UpdateOption(OPT_WARN_LEVEL, "2");
    else if (GetCheck(DLG_ID(IDRADIO_WARN3)))
        m_pOpts->UpdateOption(OPT_WARN_LEVEL, "3");
    else if (GetCheck(DLG_ID(IDRADIO_WARN4)))
        m_pOpts->UpdateOption(OPT_WARN_LEVEL, "4");
}

void CTabCompiler::OnBtnChangePch()
{
    ttCFileDlg fdlg(*this);
    fdlg.SetFilter("Header Files|*.h;*.hh;*.hpp;*.hxx||");
    ttCStr cszCWD;
    cszCWD.GetCWD();
    fdlg.UseCurrentDirectory();
    fdlg.RestoreDirectory();
    if (fdlg.GetOpenName())
    {
        ttCStr cszOrg, cszCpp;
        cszOrg.GetWndText(GetDlgItem(DLG_ID(IDEDIT_PCH)));
        cszCpp.GetWndText(GetDlgItem(DLG_ID(IDEDIT_PCH_CPP)));

        ttCStr cszRelPath;
        ttConvertToRelative(cszCWD, fdlg.GetFileName(), cszRelPath);
        SetControlText(DLG_ID(IDEDIT_PCH), cszRelPath);

        if (ttFileExists(cszCpp))  // if the current source file exists, assume that's what the user wants
            return;

        // The default behaviour is to use the same base name for the C++ source file as the precompiled header
        // file. So, if they are the same, then change the source file name to match (provided the file actually
        // exists).

        cszOrg.RemoveExtension();
        cszCpp.RemoveExtension();

        if (ttIsSameStrI(cszOrg, cszCpp))
        {
            cszCpp = cszRelPath;
            cszCpp.ChangeExtension(".cpp");
            if (ttFileExists(cszCpp))
            {
                SetControlText(DLG_ID(IDEDIT_PCH_CPP), cszCpp);
                return;
            }
            cszCpp.ChangeExtension(".cxx");
            if (ttFileExists(cszCpp))
            {
                SetControlText(DLG_ID(IDEDIT_PCH_CPP), cszCpp);
                return;
            }
            cszCpp.ChangeExtension(".cc");
            if (ttFileExists(cszCpp))
            {
                SetControlText(DLG_ID(IDEDIT_PCH_CPP), cszCpp);
                return;
            }

            cszCpp.Delete();
            cszCpp.printf(
                "%s does not have a matching C++ source file -- precompiled header will fail without it!",
                (char*) cszRelPath);
            ttMsgBox(cszCpp);
        }
    }
}

void CTabCompiler::OnBtnPchCpp()
{
    ttCFileDlg fdlg(*this);
    fdlg.SetFilter("C++ Files|*.cpp;*.cc;*.cxx||");
    fdlg.UseCurrentDirectory();
    fdlg.RestoreDirectory();
    if (fdlg.GetOpenName())
    {
        ttCStr cszCWD;
        cszCWD.GetCWD();
        ttCStr cszRelPath;
        ttConvertToRelative(cszCWD, fdlg.GetFileName(), cszRelPath);
        SetControlText(DLG_ID(IDEDIT_PCH_CPP), cszRelPath);
    }
}

void CTabCompiler::OnBtnCplusplus()
{
    ttCStr csz;

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_COMMON)));
    if (!ttStrStrI(csz, "-Zc:__cplusplus"))
    {
        if (csz.IsNonEmpty())
            csz += " ";
        csz += "-Zc:__cplusplus";
        SetControlText(DLG_ID(IDEDIT_COMMON), csz);
        DisableControl(IDBTN_CPLUSPLUS);
    }
}

void CTabCompiler::OnBtnStd()
{
    ttCStr csz;

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_COMMON)));
    if (!ttStrStrI(csz, "-std:c"))
    {
        if (csz.IsNonEmpty())
            csz += " ";
        csz += "-std:c++17";
        SetControlText(DLG_ID(IDEDIT_COMMON), csz);
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
    ttCStr    cszCWD;
    cszCWD.GetCWD();

    dlg.SetStartingDir(cszCWD);
    if (dlg.GetFolderName((HWND) *m_pOpts))
    {
        ttCStr csz;
        csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_INCDIRS)));
        csz += ";";
        csz += (char*) dlg.GetFolderName();
        SetControlText(DLG_ID(IDEDIT_INCDIRS), csz);
    }
}

#endif
