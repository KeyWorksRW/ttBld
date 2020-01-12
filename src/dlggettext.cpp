/////////////////////////////////////////////////////////////////////////////
// Name:      CDlgGetText
// Purpose:   IDDLG_XGETTEXT dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "dlggettext.h"
#include "addfuncname.h"  // CAddFuncName

#include "ttlibicons.h"

void CDlgGetText::OnBegin(void)
{
    EnableShadeBtns();
    CenterWindow();
    SetBtnIcon(DLG_ID(IDOK), IDICON_TTLIB_OK);
    SetBtnIcon(DLG_ID(IDCANCEL), IDICON_TTLIB_CANCEL);
    m_lbKeywords.Initialize(DLG_ID(IDLIST_FUNC_NAMES));
    DisplayFunctionList();
}

void CDlgGetText::OnOK(void) {}

void CDlgGetText::ProcessCurOptions(const char* pszOptions)
{
    if (ttIsEmpty(pszOptions))
    {
        // No current options, so set some defaults

        m_lstKeywords += "_";
        m_lstKeywords += "kwxGetTranslation";
        m_lstKeywords += "kwxPLURAL";

        m_bIndents = true;
        m_bNoHeaders = true;
        return;
    }
}

void CDlgGetText::OnBtnAdd()
{
    CAddFuncName dlg;
    if (dlg.DoModal(*this) == IDOK)
    {
        if (m_lbKeywords.FindString(dlg.GetFuncName()))
        {
            ttMsgBoxFmt(_("The name %kq has already been added."), MB_OK | MB_ICONWARNING, dlg.GetFuncName());
            return;
        }
        m_lbKeywords += dlg.GetFuncName();
        DisplayFunctionList();
    }
}

void CDlgGetText::DisplayFunctionList()
{
    m_lbKeywords.Reset();
    for (size_t pos = 0; m_lstKeywords.InRange(pos); ++pos)
        m_lbKeywords.Add(m_lstKeywords[pos]);
}
