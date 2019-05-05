/////////////////////////////////////////////////////////////////////////////
// Name:		CTabCompiler
// Purpose:		IDTAB_COMPILER dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttfiledlg.h>	// ttCFileDlg

#include "dlgoptions.h"

void CTabCompiler::OnBegin(void)
{
	EnableShadeBtns();
	SetCheck(DLG_ID(m_pOpts->IsOptimizeSpeed() ? IDC_RADIO_SPEED : IDC_RADIO_SPACE));
	SetCheck(DLG_ID(m_pOpts->GetBoolOption(OPT_STDCALL) ? IDC_RADIO_STDCALL :IDC_RADIO_CDECL));

	ptrdiff_t warnLevel = m_pOpts->GetOption(OPT_WARN_LEVEL) ? tt::Atoi(m_pOpts->GetOption(OPT_WARN_LEVEL)) : 4;

	switch (warnLevel) {
		case 1:
			SetCheck(DLG_ID(IDRADIO_WARN1));
			break;
		case 2:
			SetCheck(DLG_ID(IDRADIO_WARN2));
			break;
		case 3:
			SetCheck(DLG_ID(IDRADIO_WARN3));
			break;
		case 4:
		default:
			SetCheck(DLG_ID(IDRADIO_WARN4));
			break;
	}

	if (m_pOpts->GetPchHeader())
		SetControlText(DLG_ID(IDEDIT_PCH), m_pOpts->GetPchHeader());
	if (m_pOpts->GetOption(OPT_INC_DIRS))
		SetControlText(DLG_ID(IDEDIT_INCDIRS), m_pOpts->GetOption(OPT_INC_DIRS));

	if (m_pOpts->GetOption(OPT_CFLAGS_CMN))
		SetControlText(DLG_ID(IDEDIT_COMMON), m_pOpts->GetOption(OPT_CFLAGS_CMN));
	if (m_pOpts->GetOption(OPT_CFLAGS_REL))
		SetControlText(DLG_ID(IDEDIT_RELEASE), m_pOpts->GetOption(OPT_CFLAGS_REL));
	if (m_pOpts->GetOption(OPT_CFLAGS_DBG))
		SetControlText(DLG_ID(IDEDIT_DEBUG), m_pOpts->GetOption(OPT_CFLAGS_DBG));

	SetCheck(DLG_ID(IDC_CHECK_PERMISSIVE), m_pOpts->GetBoolOption(OPT_PERMISSIVE));
}

void CTabCompiler::OnOK(void)
{
	ttCStr csz;

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_COMMON)));
	m_pOpts->UpdateOption(OPT_CFLAGS_CMN, (char*) csz);

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_RELEASE)));
	m_pOpts->UpdateOption(OPT_CFLAGS_REL, (char*) csz);

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_DEBUG)));
	m_pOpts->UpdateOption(OPT_CFLAGS_DBG, (char*) csz);

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_INCDIRS)));
	m_pOpts->UpdateOption(OPT_INC_DIRS, (char*) csz);

	if (GetCheck(DLG_ID(IDC_RADIO_SPEED)))
		m_pOpts->UpdateOption(OPT_OPTIMIZE, "speed");
	else
		m_pOpts->UpdateOption(OPT_OPTIMIZE, "space");

	if (GetCheck(DLG_ID(IDRADIO_WARN1)))
		m_pOpts->UpdateOption(OPT_WARN_LEVEL, "1");
	else if (GetCheck(DLG_ID(IDRADIO_WARN2)))
		m_pOpts->UpdateOption(OPT_WARN_LEVEL, "2");
	else if (GetCheck(DLG_ID(IDRADIO_WARN3)))
		m_pOpts->UpdateOption(OPT_WARN_LEVEL, "3");
	else if (GetCheck(DLG_ID(IDRADIO_WARN4)))
		m_pOpts->UpdateOption(OPT_WARN_LEVEL, "4");

	m_pOpts->UpdateOption(OPT_PERMISSIVE, GetCheck(DLG_ID(IDC_CHECK_PERMISSIVE)));
	m_pOpts->UpdateOption(OPT_STDCALL, GetCheck(DLG_ID(IDC_RADIO_STDCALL)));
}

void CTabCompiler::OnBtnChangePch()
{
	ttCFileDlg fdlg(*this);
	fdlg.SetFilter("Header Files|*.h");
	ttCStr cszCWD;
	cszCWD.GetCWD();
	cszCWD.AddTrailingSlash();
	fdlg.SetInitialDir(cszCWD);
	if (fdlg.GetOpenFileName()) {
		const char* pszFileName = fdlg.GetFileName();
		const char* pszCWD = cszCWD;
		while (tolower(*pszFileName) == tolower(*pszCWD)) {
			pszFileName++;
			pszCWD++;
		}
		ttCStr cszFile(pszFileName);
		cszFile.ChangeExtension(".cpp");
		if (tt::FileExists(cszFile)) {
			SetControlText(DLG_ID(IDEDIT_PCH), pszFileName);
			return;
		}
		cszFile.ChangeExtension(".cxx");
		if (tt::FileExists(cszFile)) {
			SetControlText(DLG_ID(IDEDIT_PCH), pszFileName);
			return;
		}
		cszFile.ChangeExtension(".ccc");
		if (tt::FileExists(cszFile)) {
			SetControlText(DLG_ID(IDEDIT_PCH), pszFileName);
			return;
		}
		cszFile.Delete();
		cszFile.printf("%s does not have a matching C++ source file -- precompiled header will fail without it!", (char*) pszFileName);
		tt::MsgBox(cszFile);
	}
}
