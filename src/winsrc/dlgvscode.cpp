/////////////////////////////////////////////////////////////////////////////
// Name:      CDlgVsCode
// Purpose:   IDDLG_VSCODE dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "dlgvscode.h"   // CDlgVsCode -- IDDLG_VSCODE dialog handler
#include "funcs.h"       // List of function declarations
#include "ttlibicons.h"  // Icons for use on 3D shaded buttons (ttShadeBtn)

void CDlgVsCode::OnBegin(void)
{
    CHECK_DLG_ID(IDCANCEL);
    CHECK_DLG_ID(IDCHECK_CLANG_COMPILER);
    CHECK_DLG_ID(IDCHECK_MAIN_COMPILER);
    CHECK_DLG_ID(IDCHECK_NINJA_DEBUG);
    CHECK_DLG_ID(IDOK);
    CHECK_DLG_ID(IDRADIO_CLANG_DEFAULT);
    CHECK_DLG_ID(IDRADIO_MAIN_DEFAULT);
    CHECK_DLG_ID(IDRADIO_NINJA_DEFAULT);
    CHECK_DLG_ID(IDRADIO_PRE_CLANG);
    CHECK_DLG_ID(IDRADIO_PRE_MAIN);
    CHECK_DLG_ID(IDRADIO_PRE_NINJA);
    CHECK_DLG_ID(IDRADIO_PRE_NONE);

    EnableShadeBtns();
    CenterWindow();
    SetBtnIcon(IDOK, IDICON_TTLIB_OK);
    SetBtnIcon(IDCANCEL, IDICON_TTLIB_CANCEL);

    m_bMainTasks = true;
    m_bNinjaTask = true;
    SetCheck(IDCHECK_MAIN_COMPILER, m_bMainTasks);
    SetCheck(IDCHECK_NINJA_DEBUG, m_bNinjaTask);

    ttlib::cstr tmp;
#if defined(_WIN32)
    m_bMake = FindFileEnv("PATH", "mingw32-make.exe", tmp);
    if (FindFileEnv("PATH", "clang-cl.exe", tmp))
        SetCheck(IDCHECK_CLANG_COMPILER);
#else
    m_bMake = false;
    if (FindFileEnv("PATH", "clang.exe", tmp))
        SetCheck(IDCHECK_CLANG_COMPILER)
#endif  // _WIN32

    m_PreLaunch = PRELAUNCH_NINJA;
    SetCheck(IDRADIO_PRE_NINJA);

    SetCheck(IDRADIO_MAIN_DEFAULT);
}

void CDlgVsCode::OnOK(void)
{
    m_bMainTasks = GetCheck(IDCHECK_MAIN_COMPILER);
    m_bClangTasks = GetCheck(IDCHECK_CLANG_COMPILER);
    m_bNinjaTask = GetCheck(IDCHECK_NINJA_DEBUG);

    if (GetCheck(IDRADIO_PRE_NONE))
        m_PreLaunch = PRELAUNCH_NONE;
    else if (GetCheck(IDRADIO_PRE_MAIN))
        m_PreLaunch = PRELAUNCH_MAIN;
    else if (GetCheck(IDRADIO_PRE_CLANG))
        m_PreLaunch = PRELAUNCH_CLANG;
    else
    {
        if (!m_bNinjaTask)  // Can't prelaunch Ninja task if it's not in the list of Tasks
            m_PreLaunch = PRELAUNCH_MAIN;
        else
            m_PreLaunch = PRELAUNCH_NINJA;
    }

    if (GetCheck(IDRADIO_MAIN_DEFAULT))
        m_DefTask = DEFTASK_MAIN;
    else if (GetCheck(IDRADIO_CLANG_DEFAULT))
        m_DefTask = DEFTASK_CLANG;
    else
    {
        if (!m_bNinjaTask)  // Can't default to Ninja task if it's not in the list of Tasks
            m_DefTask = DEFTASK_MAIN;
        else
            m_DefTask = DEFTASK_NINJA;
    }
}

void CDlgVsCode::OnCheckNinjaDebug()
{
    if (GetCheck(IDCHECK_NINJA_DEBUG))
    {
        ShowControl(IDRADIO_PRE_NINJA);
        ShowControl(IDRADIO_NINJA_DEFAULT);
    }
    else
    {
        HideControl(IDRADIO_PRE_NINJA);
        HideControl(IDRADIO_NINJA_DEFAULT);
    }
}
