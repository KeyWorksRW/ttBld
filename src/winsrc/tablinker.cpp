/////////////////////////////////////////////////////////////////////////////
// Name:      CTabLinker
// Purpose:   IDTAB_LINKER dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttTR.h>  // Function for translating strings

#include <ttfiledlg.h>  // ttCFileDlg
#include <ttenumstr.h>  // ttlib::enumstr, ttEnumView -- Enumerate through substrings in a string
#include <ttdirdlg.h>   // ttCDirDlg

#include "dlgoptions.h"
void CTabLinker::OnBegin(void)
{
    EnableShadeBtns();

    SetCheck(DLG_ID(IDCHECK_STATIC_CRT_REL), m_pOpts->IsStaticCrtRel());
    SetCheck(DLG_ID(IDCHECK_STATIC_CRT_DBG), m_pOpts->IsStaticCrtDbg());

    //    if (m_pOpts->getOptValue(OPT_LIBS))
    //        SetControlText(DLG_ID(IDEDIT_LIBS_LINK), m_pOpts->getOptValue(OPT_LIBS));
    //    if (m_pOpts->getOptValue(OPT::BUILD_LIBS))
    //        SetControlText(DLG_ID(IDEDIT_LIBS_BUILD), m_pOpts->getOptValue(OPT::BUILD_LIBS));
    //    if (m_pOpts->getOptValue(OPT::LIB_DIRS))
    //        SetControlText(DLG_ID(IDEDIT_LIBDIRS), m_pOpts->getOptValue(OPT::LIB_DIRS));

    if (m_pOpts->hasOptValue(OPT::LINK_CMN))
        setControlText(DLG_ID(IDEDIT_COMMON), m_pOpts->getOptValue(OPT::LINK_CMN));
    if (m_pOpts->hasOptValue(OPT::LINK_REL))
        setControlText(DLG_ID(IDEDIT_RELEASE), m_pOpts->getOptValue(OPT::LINK_REL));
    if (m_pOpts->hasOptValue(OPT::LINK_DBG))
        setControlText(DLG_ID(IDEDIT_DEBUG), m_pOpts->getOptValue(OPT::LINK_DBG));

    if (m_pOpts->hasOptValue(OPT::NATVIS))
        setControlText(DLG_ID(IDEDIT_NATVIS), m_pOpts->getOptValue(OPT::NATVIS));
}

void CTabLinker::OnOK(void)
{
    ttCStr csz;

    csz.GetWndText(GetDlgItem(IDEDIT_COMMON));
    m_pOpts->setOptValue(OPT::LINK_CMN, (char*) csz);

    csz.GetWndText(GetDlgItem(IDEDIT_RELEASE));
    m_pOpts->setOptValue(OPT::LINK_REL, (char*) csz);

    csz.GetWndText(GetDlgItem(IDEDIT_DEBUG));
    m_pOpts->setOptValue(OPT::LINK_DBG, (char*) csz);

    //    csz.GetWndText(GetDlgItem(IDEDIT_LIBDIRS));
    //    m_pOpts->setOptValue(OPT::LIB_DIRS, (char*) csz);

    //    csz.GetWndText(GetDlgItem(IDEDIT_LIBS_LINK));
    //    m_pOpts->setOptValue(OPT_LIBS, (char*) csz);

    //    csz.GetWndText(GetDlgItem(IDEDIT_LIBS_BUILD));
    //    m_pOpts->setOptValue(OPT::BUILD_LIBS, (char*) csz);

    m_pOpts->setOptValue(OPT::CRT_REL, GetCheck(IDCHECK_STATIC_CRT_REL) ? "static" : "dll");
    m_pOpts->setOptValue(OPT::CRT_DBG, GetCheck(IDCHECK_STATIC_CRT_DBG) ? "static" : "dll");

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_NATVIS)));
    m_pOpts->setOptValue(OPT::NATVIS, (char*) csz);
}

void CTabLinker::OnBtnChange()
{
    ttCFileDlg fdlg(*this);
    fdlg.SetFilter("Natvis Files|*.natvis");
    fdlg.UseCurrentDirectory();
    fdlg.RestoreDirectory();
    if (fdlg.GetOpenName())
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
    dlg.SetTitle(_tt("Library directory"));  // we're repurposing an existing dialog, so change the title
    if (dlg.GetFolderName(*this))
    {
        ttCStr cszTmp, cszLibDir, cszCurLibDirs;
        cszTmp.GetCWD();
        ttConvertToRelative(cszTmp, dlg, cszLibDir);
        cszCurLibDirs.GetWndText(GetDlgItem(DLG_ID(IDEDIT_LIBDIRS)));

        if (cszCurLibDirs.IsNonEmpty())
        {
            ttlib::enumstr enumPaths(cszCurLibDirs.c_str());
            for (auto iter : enumPaths)
            {
                if (ttIsSamePath(iter.c_str(), cszLibDir))
                {
                    ttMsgBoxFmt(_tt("You've already added the directory %s"), MB_OK | MB_ICONWARNING,
                                (char*) cszLibDir);
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
    cszCurLibDirs.GetWndText(GetDlgItem(DLG_ID(IDEDIT_LIBDIRS)));

    if (cszCurLibDirs.IsNonEmpty() &&
        !ttStrChr(cszCurLibDirs, ';'))  // set initial directory if one and only one path
        fdlg.SetInitialDir(cszCurLibDirs);

    fdlg.RestoreDirectory();
    if (fdlg.GetOpenName())
    {
        ttCStr cszCWD, cszFile, cszCurLibs;
        cszCWD.GetCWD();
        ttConvertToRelative(cszCWD, fdlg, cszFile);
        cszCurLibDirs.GetWndText(GetDlgItem(DLG_ID(IDEDIT_LIBS_LINK)));
        if (cszCurLibDirs.IsNonEmpty())
            cszCurLibDirs += ";";
        cszCurLibDirs += ttFindFilePortion(cszFile);

        SetControlText(DLG_ID(IDEDIT_LIBS_LINK), cszCurLibDirs);
    }
}
