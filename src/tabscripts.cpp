/////////////////////////////////////////////////////////////////////////////
// Name:		CTabScripts
// Purpose:		IDTAB_SCRIPTS dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "dlgoptions.h"

void CTabScripts::OnBegin(void)
{
	if (m_pOpts->IsMakeNever())
		SetCheck(DLG_ID(IDRADIO_MF_NEVER));

	else if (m_pOpts->IsMakeAlways())
		SetCheck(DLG_ID(IDRADIO_MF_ALWAYS));
	else
		SetCheck(DLG_ID(IDRADIO_MF_MISSING));
}

void CTabScripts::OnOK(void)
{
	if (GetCheck(DLG_ID(IDRADIO_MF_NEVER)))
		m_pOpts->UpdateOption(OPT_MAKEFILE, "never");
	else if (GetCheck(DLG_ID(IDRADIO_MF_ALWAYS)))
		m_pOpts->UpdateOption(OPT_MAKEFILE, "always");
	else
		m_pOpts->UpdateOption(OPT_MAKEFILE, "missing");
}

