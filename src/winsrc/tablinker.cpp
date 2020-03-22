/////////////////////////////////////////////////////////////////////////////
// Name:      CTabLinker
// Purpose:   IDTAB_LINKER dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttcwd.h>      // cwd -- Class for storing and optionally restoring the current directory
#include <ttdirdlg.h>   // ttCDirDlg
#include <ttenumstr.h>  // ttlib::enumstr, ttEnumView -- Enumerate through substrings in a string
#include <ttfiledlg.h>  // ttCFileDlg

#include "dlgoptions.h"

void CTabLinker::OnBegin(void)
{
    CHECK_DLG_ID(IDCHECK_STATIC_CRT_DBG);
    CHECK_DLG_ID(IDCHECK_STATIC_CRT_REL);
    CHECK_DLG_ID(IDEDIT_COMMON);
    CHECK_DLG_ID(IDEDIT_DEBUG);
    CHECK_DLG_ID(IDEDIT_NATVIS);
    CHECK_DLG_ID(IDEDIT_RELEASE);

    EnableShadeBtns();

    SetCheck(IDCHECK_STATIC_CRT_REL, m_pOpts->IsStaticCrtRel());
    SetCheck(IDCHECK_STATIC_CRT_DBG, m_pOpts->IsStaticCrtDbg());

    if (m_pOpts->hasOptValue(OPT::LINK_CMN))
        SetControlText(IDEDIT_COMMON, m_pOpts->getOptValue(OPT::LINK_CMN));
    if (m_pOpts->hasOptValue(OPT::LINK_REL))
        SetControlText(IDEDIT_RELEASE, m_pOpts->getOptValue(OPT::LINK_REL));
    if (m_pOpts->hasOptValue(OPT::LINK_DBG))
        SetControlText(IDEDIT_DEBUG, m_pOpts->getOptValue(OPT::LINK_DBG));

    if (m_pOpts->hasOptValue(OPT::NATVIS))
        SetControlText(IDEDIT_NATVIS, m_pOpts->getOptValue(OPT::NATVIS));
}

void CTabLinker::OnOK(void)
{
    ttlib::cstr csz;

    csz.GetWndText(gethwnd(IDEDIT_COMMON));
    m_pOpts->setOptValue(OPT::LINK_CMN, csz);

    csz.GetWndText(gethwnd(IDEDIT_RELEASE));
    m_pOpts->setOptValue(OPT::LINK_REL, csz);

    csz.GetWndText(gethwnd(IDEDIT_DEBUG));
    m_pOpts->setOptValue(OPT::LINK_DBG, csz);

    //    csz.GetWndText(gethwnd(IDEDIT_LIBDIRS));
    //    m_pOpts->setOptValue(OPT::LIB_DIRS, (char*) csz);

    //    csz.GetWndText(gethwnd(IDEDIT_LIBS_LINK));
    //    m_pOpts->setOptValue(OPT_LIBS, (char*) csz);

    //    csz.GetWndText(gethwnd(IDEDIT_LIBS_BUILD));
    //    m_pOpts->setOptValue(OPT::BUILD_LIBS, (char*) csz);

    m_pOpts->setOptValue(OPT::CRT_REL, GetCheck(IDCHECK_STATIC_CRT_REL) ? "static" : "dll");
    m_pOpts->setOptValue(OPT::CRT_DBG, GetCheck(IDCHECK_STATIC_CRT_DBG) ? "static" : "dll");

    csz.GetWndText(gethwnd(IDEDIT_NATVIS));
    m_pOpts->setOptValue(OPT::NATVIS, csz);
}

void CTabLinker::OnBtnChange()
{
    ttCFileDlg fdlg(*this);
    fdlg.SetFilter("Natvis Files|*.natvis");
    fdlg.UseCurrentDirectory();
    fdlg.RestoreDirectory();
    if (fdlg.GetOpenName())
    {
        ttlib::cstr name(fdlg.GetFileName());
        ttlib::cwd cwd;
        name.make_relative(cwd);
        SetControlText(IDEDIT_NATVIS, name);
    }
}

void CTabLinker::OnBtnLibDir()
{
    ttlib::DirDlg dlg;
    dlg.SetTitle(_tt("Library directory"));  // we're repurposing an existing dialog, so change the title
    if (dlg.GetFolderName(*this))
    {
        ttlib::cwd cwd;
        ttlib::cstr LibDir(dlg);
        LibDir.make_relative(cwd);
        LibDir.backslashestoforward();

        ttlib::cstr CurLibDirs;
        GetControlText(IDEDIT_LIBDIRS, CurLibDirs);

        if (!CurLibDirs.empty())
        {
            CurLibDirs.backslashestoforward();
            ttlib::enumstr enumPaths(CurLibDirs);
            for (auto iter: enumPaths)
            {
                if (iter.issameas(LibDir))
                {
                    ttlib::MsgBox(_tt("You've already added the directory ") + LibDir);
                    return;
                }
            }
            CurLibDirs += ";";
            CurLibDirs += LibDir;
            SetControlText(IDEDIT_LIBDIRS, CurLibDirs);
        }
        CurLibDirs.append_filename(LibDir);
        SetControlText(IDEDIT_LIBDIRS, CurLibDirs);
    }
}

void CTabLinker::OnBtnAddLib()
{
    ttCFileDlg fdlg(*this);
    fdlg.SetFilter("Library Files|*.lib");

    ttlib::cstr cszCurLibDirs;
    cszCurLibDirs.GetWndText(gethwnd(IDEDIT_LIBDIRS));

    if (!cszCurLibDirs.empty() && !cszCurLibDirs.contains(";"))  // set initial directory if one and only one path
        fdlg.SetInitialDir(cszCurLibDirs.c_str());

    fdlg.RestoreDirectory();
    if (fdlg.GetOpenName())
    {
        ttlib::cwd cwd;
        ttlib::cstr name(fdlg.GetFileName());
        name.make_relative(cwd);

        ttlib::cstr CurLibs;
        CurLibs.GetWndText(gethwnd(IDEDIT_LIBS_LINK));
        if (!CurLibs.empty())
            CurLibs += ";";
        CurLibs += name;

        SetControlText(IDEDIT_LIBS_LINK, CurLibs);
    }
}
