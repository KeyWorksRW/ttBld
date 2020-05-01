/////////////////////////////////////////////////////////////////////////////
// Name:      CDlgVsCode
// Purpose:   IDDLG_VSCODE dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDDLG_VSCODE
    #include "resource.h"
#endif

#include <vector>

#include <ttcvector.h>  // cstrVector -- Vector of ttlib::cstr strings
#include <ttwindlg.h>   // Class for displaying a dialog

// forward definitions

class CSrcFiles;

class CDlgVsCode : public ttlib::dlg
{
public:
    CDlgVsCode() : ttlib::dlg(IDDLG_VSCODE) {}

    enum
    {
        PRELAUNCH_NONE,
        PRELAUNCH_MAIN,
        PRELAUNCH_CLANG,
        PRELAUNCH_NINJA,
    };

    enum
    {
        DEFTASK_MAIN,
        DEFTASK_CLANG,
        DEFTASK_NINJA,
    };

    // Public functions

    bool CreateVsCodeLaunch(CSrcFiles& cSrcFiles, ttlib::cstrVector& Results);
    bool CreateVsCodeTasks(CSrcFiles& cSrcFiles, ttlib::cstrVector& Results);

protected:
    // Message handlers

    BEGIN_TTCMD_MAP()
        TTCASE_CMD(IDCHECK_NINJA_DEBUG, OnCheckNinjaDebug)
    END_TTMSG_MAP()

    void OnCheckNinjaDebug();
    void OnBegin(void) override;
    void OnOK(void) override;

    // Protected functions

private:
    // Class members

    int m_PreLaunch;
    int m_DefTask;

    bool m_bMainTasks;  // MSVC on Windows, GCC otherwise
    bool m_bClangTasks;
    bool m_bNinjaTask;
    bool m_bMake;  // true if mingw32-make.exe has been located
};
