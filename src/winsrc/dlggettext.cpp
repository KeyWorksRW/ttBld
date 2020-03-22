/////////////////////////////////////////////////////////////////////////////
// Name:      CDlgGetText
// Purpose:   IDDLG_XGETTEXT dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "dlggettext.h"
#include "addfuncname.h"  // CAddFuncName

#include "ttlibicons.h"

void CDlgGetText::OnBegin(void)
{
    CHECK_DLG_ID(IDCANCEL);
    CHECK_DLG_ID(IDLIST_FUNC_NAMES);
    CHECK_DLG_ID(IDOK);

    EnableShadeBtns();
    CenterWindow();
    SetBtnIcon(IDOK, IDICON_TTLIB_OK);
    SetBtnIcon(IDCANCEL, IDICON_TTLIB_CANCEL);
    m_lbKeywords.Initialize(IDLIST_FUNC_NAMES);
    DisplayFunctionList();
}

void CDlgGetText::OnOK(void) {}

void CDlgGetText::ProcessCurOptions(const char* pszOptions)
{
    if (ttIsEmpty(pszOptions))
    {
        // No current options, so set some defaults

        m_lstKeywords.emplace_back("_");
        m_lstKeywords.emplace_back("kwxGetTranslation");
        m_lstKeywords.emplace_back("kwxPLURAL");

        m_Indents = true;
        m_NoHeaders = true;
        return;
    }
}

void CDlgGetText::OnBtnAdd()
{
    CAddFuncName dlg;
    if (dlg.DoModal(*this) == IDOK)
    {
        if (m_lbKeywords.find(dlg.GetFuncName()))
        {
            ttlib::cstr msg;
            ttlib::MsgBox(msg.Format(_tt("The name %kq has already been added."), dlg.GetFuncName().c_str()));
            return;
        }
        m_lbKeywords.append(dlg.GetFuncName());
        DisplayFunctionList();
    }
}

void CDlgGetText::DisplayFunctionList()
{
    m_lbKeywords.Reset();
    for (auto& iter: m_lstKeywords)
        m_lbKeywords.append(iter);
}
