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
	if (m_pOpts->GetOption(OPT_CLANG_CMN))
		SetControlText(DLG_ID(IDEDIT_COMMON), m_pOpts->GetOption(OPT_CLANG_CMN));
	if (m_pOpts->GetOption(OPT_CLANG_REL))
		SetControlText(DLG_ID(IDEDIT_RELEASE), m_pOpts->GetOption(OPT_CLANG_REL));
	if (m_pOpts->GetOption(OPT_CLANG_DBG))
		SetControlText(DLG_ID(IDEDIT_DEBUG), m_pOpts->GetOption(OPT_CLANG_DBG));

	if (!m_pOpts->IsCompilerClang()) {
		UnCheck(DLG_ID(IDCHECK_NO_MSVC));
		DisableControl(DLG_ID(IDCHECK_MSLINKER));
		DisableControl(DLG_ID(IDEDIT_CFLAGS));
	}
	else {
		SetCheck(DLG_ID(IDCHECK_CLANG));
		if (!m_pOpts->IsCompilerMSVC())
			SetCheck(DLG_ID(IDCHECK_NO_MSVC));
		SetCheck(DLG_ID(IDCHECK_MSLINKER), m_pOpts->GetBoolOption(OPT_MS_LINKER));
	}
}

void CTabCLang::OnOK(void)
{
	ttCStr csz;

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_COMMON)));
	m_pOpts->UpdateOption(OPT_CLANG_CMN, (char*) csz);

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_RELEASE)));
	m_pOpts->UpdateOption(OPT_CLANG_REL, (char*) csz);

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_DEBUG)));
	m_pOpts->UpdateOption(OPT_CLANG_DBG, (char*) csz);

	if (!GetCheck(DLG_ID(IDCHECK_CLANG))) {
		m_pOpts->UpdateOption(OPT_COMPILERS, "MSVC");
		// We leave the rest of the CLANG options in case the user enables CLANG at a later time
	}
	else {
		if (GetCheck(DLG_ID(IDCHECK_NO_MSVC)))
			m_pOpts->UpdateOption(OPT_COMPILERS, "CLANG");
		m_pOpts->UpdateOption(OPT_MS_LINKER, GetCheck(DLG_ID(IDCHECK_MSLINKER)));
	}
}

