/////////////////////////////////////////////////////////////////////////////
// Name:		CTabLinker
// Purpose:		IDTAB_LINKER dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "dlgoptions.h"

void CTabLinker::OnBegin(void)
{
	SetCheck(DLG_ID(IDCHECK_64BIT), m_pOpts->GetBoolOption(OPT_64BIT));
	SetCheck(DLG_ID(IDCHECK_BITEXT), m_pOpts->GetBoolOption(OPT_BIT_SUFFIX));
	SetCheck(DLG_ID(IDCHECK_STATIC_CRT), m_pOpts->GetBoolOption(OPT_STATIC_CRT));
	SetCheck(DLG_ID(IDCHECK_MSLINKER), m_pOpts->GetBoolOption(OPT_MS_LINKER));

	SetCheck(DLG_ID(IDCHECK_MSLINKER), m_pOpts->GetBoolOption(OPT_MS_LINKER));

	if (m_pOpts->GetOption(OPT_LINK_FLAGS))
		SetControlText(DLG_ID(IDEDIT_LINK_FLAGS), m_pOpts->GetOption(OPT_LINK_FLAGS));
	if (m_pOpts->GetOption(OPT_LIBS))
		SetControlText(DLG_ID(IDEDIT_LIBS), m_pOpts->GetOption(OPT_LIBS));
	if (m_pOpts->GetOption(OPT_BUILD_LIBS))
		SetControlText(DLG_ID(IDEDIT_LIBS_BUILD), m_pOpts->GetOption(OPT_BUILD_LIBS));
	if (m_pOpts->GetOption(OPT_LIB_DIRS))
		SetControlText(DLG_ID(IDEDIT_LIBDIRS), m_pOpts->GetOption(OPT_LIB_DIRS));
}

void CTabLinker::OnOK(void)
{
	ttCStr csz;

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LINK_FLAGS)));
	m_pOpts->UpdateOption(OPT_LINK_FLAGS, (char*) csz);

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBDIRS)));
	m_pOpts->UpdateOption(OPT_LIB_DIRS, (char*) csz);

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBS_LINK)));
	m_pOpts->UpdateOption(OPT_LIBS, (char*) csz);

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBS_BUILD)));
	m_pOpts->UpdateOption(OPT_BUILD_LIBS, (char*) csz);

	m_pOpts->UpdateOption(OPT_64BIT, GetCheck(DLG_ID(IDCHECK_64BIT)));
	m_pOpts->UpdateOption(OPT_BIT_SUFFIX, GetCheck(DLG_ID(IDCHECK_BITEXT)));
	m_pOpts->UpdateOption(OPT_STATIC_CRT, GetCheck(DLG_ID(IDCHECK_STATIC_CRT)));
	m_pOpts->UpdateOption(OPT_MS_LINKER, GetCheck(DLG_ID(IDCHECK_MSLINKER)));
}

