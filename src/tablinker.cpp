/////////////////////////////////////////////////////////////////////////////
// Name:      CTabLinker
// Purpose:   IDTAB_LINKER dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttfiledlg.h>  // ttCFileDlg
#include <ttenumstr.h>  // ttCEnumStr
#include <ttdirdlg.h>   // ttCDirDlg

#include "dlgoptions.h"
#include "strtable.h"

void CTabLinker::OnBegin(void)
{
    EnableShadeBtns();

    SetCheck(DLG_ID(IDCHECK_STATIC_CRT), m_pOpts->GetBoolOption(OPT_STATIC_CRT));

    if (m_pOpts->GetOption(OPT_LIBS))
        SetControlText(DLG_ID(IDEDIT_LIBS_LINK), m_pOpts->GetOption(OPT_LIBS));
    if (m_pOpts->GetOption(OPT_BUILD_LIBS))
        SetControlText(DLG_ID(IDEDIT_LIBS_BUILD), m_pOpts->GetOption(OPT_BUILD_LIBS));
    if (m_pOpts->GetOption(OPT_LIB_DIRS))
        SetControlText(DLG_ID(IDEDIT_LIBDIRS), m_pOpts->GetOption(OPT_LIB_DIRS));

    if (m_pOpts->GetOption(OPT_LINK_CMN))
        SetControlText(DLG_ID(IDEDIT_COMMON), m_pOpts->GetOption(OPT_LINK_CMN));
    if (m_pOpts->GetOption(OPT_LINK_REL))
        SetControlText(DLG_ID(IDEDIT_RELEASE), m_pOpts->GetOption(OPT_LINK_REL));
    if (m_pOpts->GetOption(OPT_LINK_DBG))
        SetControlText(DLG_ID(IDEDIT_DEBUG), m_pOpts->GetOption(OPT_LINK_DBG));

    if (m_pOpts->GetOption(OPT_NATVIS))
        SetControlText(DLG_ID(IDEDIT_NATVIS), m_pOpts->GetOption(OPT_NATVIS));
}

void CTabLinker::OnOK(void)
{
    ttCStr csz;

    csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_COMMON)));
    m_pOpts->UpdateOption(OPT_LINK_CMN, (char*) csz);

    csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_RELEASE)));
    m_pOpts->UpdateOption(OPT_LINK_REL, (char*) csz);

    csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_DEBUG)));
    m_pOpts->UpdateOption(OPT_LINK_DBG, (char*) csz);

    csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBDIRS)));
    m_pOpts->UpdateOption(OPT_LIB_DIRS, (char*) csz);

    csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBS_LINK)));
    m_pOpts->UpdateOption(OPT_LIBS, (char*) csz);

    csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBS_BUILD)));
    m_pOpts->UpdateOption(OPT_BUILD_LIBS, (char*) csz);

    m_pOpts->UpdateOption(OPT_STATIC_CRT, GetCheck(DLG_ID(IDCHECK_STATIC_CRT)));

    csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_NATVIS)));
    m_pOpts->UpdateOption(OPT_NATVIS, (char*) csz);
}

void CTabLinker::OnBtnChange()
{
    ttCFileDlg fdlg(*this);
    fdlg.SetFilter("Natvis Files|*.natvis");
    fdlg.UseCurrentDirectory();
    fdlg.RestoreDirectory();
    if (fdlg.GetOpenFileName())
    {
        ttCStr cszCWD, cszFile;
        cszCWD.GetCWD();
        ttConvertToRelative(cszCWD, fdlg, cszFile);
        SetControlText(DLG_ID(IDEDIT_NATVIS), cszFile);
    }
}

void CTabLinker::OnBtnLibDir()
{
    ttCDirDlg dlg;
    dlg.SetTitle(ttGetResString(IDS_LIBDIR));
    if (dlg.GetFolderName(*this))
    {
        ttCStr cszTmp, cszLibDir, cszCurLibDirs;
        cszTmp.GetCWD();
        ttConvertToRelative(cszTmp, dlg, cszLibDir);
        cszCurLibDirs.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBDIRS)));

        if (cszCurLibDirs.IsNonEmpty())
        {
            ttCEnumStr eLib(cszCurLibDirs);
            while (eLib.Enum())
            {
                if (ttIsSamePath(eLib, cszLibDir))
                {
                    ttMsgBoxFmt(ttGetResString(IDS_ALREADY_ADDED_DIR), MB_OK | MB_ICONWARNING, (char*) cszLibDir);
                    return;
                }
            }
            cszCurLibDirs += ";";
            cszCurLibDirs += cszLibDir;
            SetControlText(DLG_ID(IDEDIT_LIBDIRS), cszCurLibDirs);
        }
        cszCurLibDirs += cszLibDir;
        SetControlText(DLG_ID(IDEDIT_LIBDIRS), cszCurLibDirs);
    }
}

void CTabLinker::OnBtnAddLib()
{
    ttCFileDlg fdlg(*this);
    fdlg.SetFilter("Library Files|*.lib");

    ttCStr cszCurLibDirs;
    cszCurLibDirs.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBDIRS)));

    if (cszCurLibDirs.IsNonEmpty() && !ttStrChr(cszCurLibDirs, ';'))    // set initial directory if one and only one path
        fdlg.SetInitialDir(cszCurLibDirs);

    fdlg.RestoreDirectory();
    if (fdlg.GetOpenFileName())
    {
        ttCStr cszCWD, cszFile, cszCurLibs;
        cszCWD.GetCWD();
        ttConvertToRelative(cszCWD, fdlg, cszFile);
        cszCurLibDirs.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBS_LINK)));
        if (cszCurLibDirs.IsNonEmpty())
            cszCurLibDirs += ";";
        cszCurLibDirs += ttFindFilePortion(cszFile);

        SetControlText(DLG_ID(IDEDIT_LIBS_LINK), cszCurLibDirs);
    }
}
