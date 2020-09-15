/////////////////////////////////////////////////////////////////////////////
// Name:      CAddFuncName
// Purpose:   IDDLG_FUNC_NAME dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#if (defined(_WIN64) || defined(_WIN32)) && !defined(IDDLG_FUNC_NAME)
    #include "resource.h"
#endif

#include <ttwindlg.h>  // dlg -- Class for displaying a dialog

class CAddFuncName : public ttlib::dlg
{
public:
    CAddFuncName()
        : ttlib::dlg(IDDLG_FUNC_NAME)
    {
    }

    // Public functions

    const ttlib::cstr& GetFuncName() { return m_FunctionName; }

protected:
    BEGIN_TTCMD_MAP()
        TTCASE_CTRL(IDEDIT_FUNCTION_NAME, OnEditFunctionName)
    END_TTMSG_MAP()

    // Message handlers

    void OnEditFunctionName(int NotifyCode);
    void OnBegin(void) override;
    void OnOK(void) override;

private:
    // Class members

    ttlib::cstr m_FunctionName;
};
