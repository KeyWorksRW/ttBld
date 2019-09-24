/////////////////////////////////////////////////////////////////////////////
// Name:      CAddFuncName
// Purpose:   IDDLG_FUNC_NAME dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#if (defined(_WIN64) || defined(_WIN32)) && !defined(IDDLG_FUNC_NAME)
    #include "resource.h"
#endif

#include <ttdlg.h>  // ttCDlg, ttCComboBox, ttCListBox, ttCListView

class CAddFuncName : public ttCDlg
{
public:
    CAddFuncName()
        : ttCDlg(IDDLG_FUNC_NAME)
    {
    }

    // Public functions

    const char* GetFuncName() { return m_cszFunctionName; }

protected:
    BEGIN_TTCMD_MAP()
        TTCASE_CTRL(IDEDIT_FUNCTION_NAME, OnEditFunctionName)
    END_TTMSG_MAP()

    // Message handlers

    void OnEditFunctionName(int NotifyCode);
    void OnBegin(void);
    void OnOK(void);

private:
    // Class members

    ttCStr m_cszFunctionName;
};
