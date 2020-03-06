/////////////////////////////////////////////////////////////////////////////
// Name:      CDlgVsCode
// Purpose:   IDDLG_VSCODE dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDDLG_VSCODE
    #include "resource.h"
#endif

#include <ttdlg.h>  // ttCDlg, ttCComboBox, ttCListBox, ttCListView

// forward definitions

class ttCList;
class CSrcFiles;

class CDlgVsCode : public ttCDlg
{
public:
    CDlgVsCode()
        : ttCDlg(IDDLG_VSCODE)
    {
    }

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

    bool CreateVsCodeLaunch(CSrcFiles& cSrcFiles, ttCList* plstResults);
    bool CreateVsCodeTasks(CSrcFiles& cSrcFiles, ttCList* plstResults);

protected:
    // Message handlers

    BEGIN_TTCMD_MAP()
        TTCASE_CMD(IDCHECK_NINJA_DEBUG, OnCheckNinjaDebug)
    END_TTMSG_MAP()

    void OnCheckNinjaDebug();
    void OnBegin(void);
    void OnOK(void);

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
