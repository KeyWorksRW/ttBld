/////////////////////////////////////////////////////////////////////////////
// Name:      CDlgVsCode
// Purpose:   IDDLG_VSCODE dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "dlgvscode.h"   // CDlgVsCode -- IDDLG_VSCODE dialog handler
#include "ttlibicons.h"  // Icons for use on 3D shaded buttons (ttShadeBtn)
#include "funcs.h"       // List of function declarations

void CDlgVsCode::OnBegin(void)
{
    EnableShadeBtns();
    CenterWindow();
    SetBtnIcon(DLG_ID(IDOK), IDICON_TTLIB_OK);
    SetBtnIcon(DLG_ID(IDCANCEL), IDICON_TTLIB_CANCEL);

    m_bMainTasks = true;
    m_bNinjaTask = true;
    ttDDX_Check(DLG_ID(IDCHECK_MAIN_COMPILER), m_bMainTasks);
    ttDDX_Check(DLG_ID(IDCHECK_NINJA_DEBUG), m_bNinjaTask);

    ttCStr csz;
#if defined(_WIN32)
    m_bMake = FindFileEnv("PATH", "mingw32-make.exe");
    if (FindFileEnv("PATH", "clang-cl.exe", &csz))
        SetCheck(DLG_ID(IDCHECK_CLANG_COMPILER));
#else
    m_bMake = false;
    if (FindFileEnv("PATH", "clang.exe", &csz))
        SetCheck(DLG_ID(IDCHECK_CLANG_COMPILER))
#endif  // _WIN32

    m_PreLaunch = PRELAUNCH_NINJA;
    SetCheck(DLG_ID(IDRADIO_PRE_NINJA));

    SetCheck(DLG_ID(IDRADIO_MAIN_DEFAULT));
}

void CDlgVsCode::OnOK(void)
{
    ttDDX_Check(IDCHECK_MAIN_COMPILER, m_bMainTasks);
    ttDDX_Check(IDCHECK_CLANG_COMPILER, m_bClangTasks);
    ttDDX_Check(IDCHECK_NINJA_DEBUG, m_bNinjaTask);

    if (GetCheck(DLG_ID(IDRADIO_PRE_NONE)))
        m_PreLaunch = PRELAUNCH_NONE;
    else if (GetCheck(DLG_ID(IDRADIO_PRE_MAIN)))
        m_PreLaunch = PRELAUNCH_NONE;
    else if (GetCheck(DLG_ID(IDRADIO_PRE_CLANG)))
        m_PreLaunch = PRELAUNCH_NONE;
    else
        m_PreLaunch = PRELAUNCH_NINJA;

    if (GetCheck(DLG_ID(IDRADIO_MAIN_DEFAULT)))
        m_DefTask = DEFTASK_MAIN;
    else if (GetCheck(DLG_ID(IDRADIO_CLANG_DEFAULT)))
        m_DefTask = DEFTASK_CLANG;
    else
        m_DefTask = DEFTASK_NINJA;
}
