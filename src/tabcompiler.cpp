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

	ptrdiff_t warnLevel = m_pOpts->GetOption(OPT_WARN_LEVEL) ? ttAtoi(m_pOpts->GetOption(OPT_WARN_LEVEL)) : 4;

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
	if (m_pOpts->GetPchCpp())
		SetControlText(DLG_ID(IDEDIT_PCH_CPP), m_pOpts->GetPchCpp());
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

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_PCH)));
	if (csz.IsEmpty())
		m_pOpts->UpdateOption(OPT_PCH, "none");
	else
		m_pOpts->UpdateOption(OPT_PCH, (char*) csz);

	// We let WriteSrcFiles handle the case where the base names of PCH and PCH_CPP are identical

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_PCH_CPP)));
	if (csz.IsEmpty())
		m_pOpts->UpdateOption(OPT_PCH_CPP, "none");
	else
		m_pOpts->UpdateOption(OPT_PCH_CPP, (char*) csz);

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
	fdlg.SetFilter("Header Files|*.h;*.hh;*.hpp;*.hxx||");
	ttCStr cszCWD;
	cszCWD.GetCWD();
	fdlg.UseCurrentDirectory();
	fdlg.RestoreDirectory();
	if (fdlg.GetOpenFileName()) {
		ttCStr cszOrg, cszCpp;
		cszOrg.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_PCH)));
		cszCpp.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_PCH_CPP)));

		ttCStr cszRelPath;
		ttConvertToRelative(cszCWD, fdlg.GetFileName(), cszRelPath);
		SetControlText(DLG_ID(IDEDIT_PCH), cszRelPath);

		if (ttFileExists(cszCpp))	// if the current source file exists, assume that's what the user wants
			return;

		// The default behaviour is to use the same base name for the C++ source file as the precompiled header file. So, if they are the same, then
		// change the source file name to match (provided the file actually exists).

		cszOrg.RemoveExtension();
		cszCpp.RemoveExtension();

		if (ttIsSameStrI(cszOrg, cszCpp)) {
			cszCpp = cszRelPath;
			cszCpp.ChangeExtension(".cpp");
			if (ttFileExists(cszCpp)) {
				SetControlText(DLG_ID(IDEDIT_PCH_CPP), cszCpp);
				return;
			}
			cszCpp.ChangeExtension(".cxx");
			if (ttFileExists(cszCpp)) {
				SetControlText(DLG_ID(IDEDIT_PCH_CPP), cszCpp);
				return;
			}
			cszCpp.ChangeExtension(".cc");
			if (ttFileExists(cszCpp)) {
				SetControlText(DLG_ID(IDEDIT_PCH_CPP), cszCpp);
				return;
			}

			cszCpp.Delete();
			cszCpp.printf("%s does not have a matching C++ source file -- precompiled header will fail without it!", (char*) cszRelPath);
			ttMsgBox(cszCpp);
		}
	}
}

void CTabCompiler::OnBtnPchCpp()
{
	ttCFileDlg fdlg(*this);
	fdlg.SetFilter("C++ Files|*.cpp;*.cc;*.cxx||");
	ttCStr cszCWD;
	cszCWD.GetCWD();
	cszCWD.AddTrailingSlash();
	fdlg.SetInitialDir(cszCWD);
	if (fdlg.GetOpenFileName())
		SetControlText(DLG_ID(IDEDIT_PCH_CPP), fdlg.GetFileName());

}
