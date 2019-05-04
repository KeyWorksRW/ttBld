/////////////////////////////////////////////////////////////////////////////
// Name:		CTabCLang
// Purpose:		IDTAB_CLANG dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "dlgoptions.h"

void CTabCLang::OnBegin(void)
{
	SetCheck(DLG_ID(IDCHECK_MSLINKER), m_pOpts->GetBoolOption(OPT_MS_LINKER));
}

void CTabCLang::OnOK(void)
{
	m_pOpts->UpdateOption(OPT_MS_LINKER, GetCheck(DLG_ID(IDCHECK_MSLINKER)));
}

