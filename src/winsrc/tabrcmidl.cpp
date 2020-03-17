/////////////////////////////////////////////////////////////////////////////
// Name:      CTabRcMidl
// Purpose:   IDTAB_RCMIDL dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "dlgoptions.h"

#include "ttlibicons.h"

void CTabRcMidl::OnBegin(void)
{
    if (m_pOpts->hasOptValue(OPT::RC_CMN))
        setControlText(DLG_ID(IDEDIT_COMMON), m_pOpts->getOptValue(OPT::RC_CMN));
    if (m_pOpts->hasOptValue(OPT::RC_REL))
        setControlText(DLG_ID(IDEDIT_RELEASE), m_pOpts->getOptValue(OPT::RC_REL));
    if (m_pOpts->hasOptValue(OPT::RC_DBG))
        setControlText(DLG_ID(IDEDIT_DEBUG), m_pOpts->getOptValue(OPT::RC_DBG));

    if (m_pOpts->hasOptValue(OPT::MIDL_CMN))
        setControlText(DLG_ID(IDEDIT_COMMON_MIDL), m_pOpts->getOptValue(OPT::MIDL_CMN));
    if (m_pOpts->hasOptValue(OPT::MIDL_REL))
        setControlText(DLG_ID(IDEDIT_RELEASE_MIDL), m_pOpts->getOptValue(OPT::MIDL_REL));
    if (m_pOpts->hasOptValue(OPT::MIDL_DBG))
        setControlText(DLG_ID(IDEDIT_DEBUG_MIDL), m_pOpts->getOptValue(OPT::MIDL_DBG));
}

void CTabRcMidl::OnOK(void)
{
    ttlib::cstr text;

    ttlib::GetWndText(GetDlgItem(IDEDIT_COMMON), text);
    m_pOpts->setOptValue(OPT::RC_CMN, text);

    ttlib::GetWndText(GetDlgItem(DLG_ID(IDEDIT_RELEASE)));
    m_pOpts->setOptValue(OPT::RC_REL, text);

    ttlib::GetWndText(GetDlgItem(DLG_ID(IDEDIT_DEBUG)));
    m_pOpts->setOptValue(OPT::RC_DBG, text);

    ttlib::GetWndText(GetDlgItem(DLG_ID(IDEDIT_COMMON_MIDL)));
    m_pOpts->setOptValue(OPT::MIDL_CMN, text);

    ttlib::GetWndText(GetDlgItem(DLG_ID(IDEDIT_RELEASE_MIDL)));
    m_pOpts->setOptValue(OPT::MIDL_REL, text);

    ttlib::GetWndText(GetDlgItem(DLG_ID(IDEDIT_DEBUG_MIDL)));
    m_pOpts->setOptValue(OPT::MIDL_DBG, text);
}
