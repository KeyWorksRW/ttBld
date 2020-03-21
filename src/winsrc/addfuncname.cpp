/////////////////////////////////////////////////////////////////////////////
// Name:      CAddFuncName
// Purpose:   IDDLG_FUNC_NAME dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "addfuncname.h"

#include "ttlibicons.h"

void CAddFuncName::OnBegin(void)
{
    CHECK_DLG_ID(IDCANCEL);
    CHECK_DLG_ID(IDEDIT_FUNCTION_NAME);
    CHECK_DLG_ID(IDOK);

    EnableShadeBtns();
    CenterWindow();
    SetBtnIcon(IDOK, IDICON_TTLIB_OK);
    SetBtnIcon(IDCANCEL, IDICON_TTLIB_CANCEL);
    DisableControl(IDOK);  // don't enable until we have some text in it
}

void CAddFuncName::OnOK(void)
{
    m_FunctionName.GetWndText(gethwnd(IDEDIT_FUNCTION_NAME));
}

void CAddFuncName::OnEditFunctionName(int /* NotifyCode */)
{
    EnableControl(IDOK, GetControlTextLength(IDEDIT_FUNCTION_NAME) > 0);
}
