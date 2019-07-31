/////////////////////////////////////////////////////////////////////////////
// Name:      CTabCLang
// Purpose:   IDTAB_CLANG dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "dlgoptions.h"

void CTabCLang::OnBegin(void)
{
    if (m_pOpts->GetOption(OPT_CLANG_CMN))
        SetControlText(DLG_ID(IDEDIT_COMMON), m_pOpts->GetOption(OPT_CLANG_CMN));
    if (m_pOpts->GetOption(OPT_CLANG_REL))
        SetControlText(DLG_ID(IDEDIT_RELEASE), m_pOpts->GetOption(OPT_CLANG_REL));
    if (m_pOpts->GetOption(OPT_CLANG_DBG))
        SetControlText(DLG_ID(IDEDIT_DEBUG), m_pOpts->GetOption(OPT_CLANG_DBG));
}

void CTabCLang::OnOK(void)
{
    ttCStr csz;

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_COMMON)));
    m_pOpts->UpdateOption(OPT_CLANG_CMN, (char*) csz);

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_RELEASE)));
    m_pOpts->UpdateOption(OPT_CLANG_REL, (char*) csz);

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_DEBUG)));
    m_pOpts->UpdateOption(OPT_CLANG_DBG, (char*) csz);
}

