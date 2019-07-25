/////////////////////////////////////////////////////////////////////////////
// Name:      CAddFuncName
// Purpose:   IDDLG_FUNC_NAME dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "addfuncname.h"

#include "ttlibicons.h"

void CAddFuncName::OnBegin(void)
{
    EnableShadeBtns();
    CenterWindow();
    SetBtnIcon(DLG_ID(IDOK), IDICON_TTLIB_OK);
    SetBtnIcon(DLG_ID(IDCANCEL), IDICON_TTLIB_CANCEL);
    DisableControl(IDOK);   // don't enable until we have some text in it
}

void CAddFuncName::OnOK(void)
{
    m_cszFunctionName.GetWndText(GetDlgItem(DLG_ID(IDEDIT_FUNCTION_NAME)));
}

void CAddFuncName::OnEditFunctionName(int /* NotifyCode */)
{
    EnableControl(IDOK, GetControlTextLength(DLG_ID(IDEDIT_FUNCTION_NAME)) > 0);
}
