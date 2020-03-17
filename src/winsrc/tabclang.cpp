/////////////////////////////////////////////////////////////////////////////
// Name:      CTabCLang
// Purpose:   IDTAB_CLANG dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "dlgoptions.h"
#include "tabclang.h"

void CTabCLang::OnBegin(void)
{
    CHECK_DLG_ID(IDCHECK_MSLINKER);
    CHECK_DLG_ID(IDCHECK_MSRC);
    CHECK_DLG_ID(IDEDIT_COMMON);
    CHECK_DLG_ID(IDEDIT_DEBUG);
    CHECK_DLG_ID(IDEDIT_RELEASE);

    if (m_pOpts->hasOptValue(OPT::CLANG_CMN))
        SetControlText(IDEDIT_COMMON, m_pOpts->getOptValue(OPT::CLANG_CMN));
    if (m_pOpts->hasOptValue(OPT::CLANG_REL))
        SetControlText(IDEDIT_RELEASE, m_pOpts->getOptValue(OPT::CLANG_REL));
    if (m_pOpts->hasOptValue(OPT::CLANG_DBG))
        SetControlText(IDEDIT_DEBUG, m_pOpts->getOptValue(OPT::CLANG_DBG));
    if (m_pOpts->isOptTrue(OPT::MS_LINKER))
        SetCheck(IDCHECK_MSLINKER);
    if (m_pOpts->isOptTrue(OPT::MS_RC))
        SetCheck(IDCHECK_MSRC);
}

void CTabCLang::OnOK(void)
{
    ttlib::cstr csz;

    csz.GetWndText(gethwnd(IDEDIT_COMMON));
    m_pOpts->setOptValue(OPT::CLANG_CMN, csz);

    csz.GetWndText(gethwnd(IDEDIT_RELEASE));
    m_pOpts->setOptValue(OPT::CLANG_REL, csz);

    csz.GetWndText(gethwnd(IDEDIT_DEBUG));
    m_pOpts->setOptValue(OPT::CLANG_DBG, csz);

    m_pOpts->setBoolOptValue(OPT::MS_LINKER, GetCheck(IDCHECK_MSLINKER));
}
