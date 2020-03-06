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
    if (m_pOpts->GetOption(OPT_RC_CMN))
        SetControlText(DLG_ID(IDEDIT_COMMON), m_pOpts->GetOption(OPT_RC_CMN));
    if (m_pOpts->GetOption(OPT_RC_REL))
        SetControlText(DLG_ID(IDEDIT_RELEASE), m_pOpts->GetOption(OPT_RC_REL));
    if (m_pOpts->GetOption(OPT_RC_DBG))
        SetControlText(DLG_ID(IDEDIT_DEBUG), m_pOpts->GetOption(OPT_RC_DBG));

    if (m_pOpts->GetOption(OPT_MDL_CMN))
        SetControlText(DLG_ID(IDEDIT_COMMON_MIDL), m_pOpts->GetOption(OPT_MDL_CMN));
    if (m_pOpts->GetOption(OPT_MDL_REL))
        SetControlText(DLG_ID(IDEDIT_RELEASE_MIDL), m_pOpts->GetOption(OPT_MDL_REL));
    if (m_pOpts->GetOption(OPT_MDL_DBG))
        SetControlText(DLG_ID(IDEDIT_DEBUG_MIDL), m_pOpts->GetOption(OPT_MDL_DBG));
}

void CTabRcMidl::OnOK(void)
{
    ttCStr csz;

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_COMMON)));
    m_pOpts->UpdateOption(OPT_RC_CMN, (char*) csz);

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_RELEASE)));
    m_pOpts->UpdateOption(OPT_RC_REL, (char*) csz);

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_DEBUG)));
    m_pOpts->UpdateOption(OPT_RC_DBG, (char*) csz);

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_COMMON_MIDL)));
    m_pOpts->UpdateOption(OPT_MDL_CMN, (char*) csz);

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_RELEASE_MIDL)));
    m_pOpts->UpdateOption(OPT_MDL_REL, (char*) csz);

    csz.GetWndText(GetDlgItem(DLG_ID(IDEDIT_DEBUG_MIDL)));
    m_pOpts->UpdateOption(OPT_MDL_DBG, (char*) csz);
}
