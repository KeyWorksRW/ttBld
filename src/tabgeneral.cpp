/////////////////////////////////////////////////////////////////////////////
// Name:		CTabGeneral
// Purpose:		IDTAB_GENERAL dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttdirdlg.h>	// ttCDirDlg

#include "dlgoptions.h"
#include "strtable.h"

void CTabGeneral::OnBegin(void)
{
	if (m_pOpts->GetProjectName())
		SetControlText(DLG_ID(IDEDIT_PROJ_NAME), m_pOpts->GetProjectName());
	else {
		char szCwd[MAX_PATH];
		GetCurrentDirectory(sizeof(szCwd), szCwd);
		char* pszProject = tt::FindFilePortion(szCwd);

		// If cwd is "src", then use the parent directory as the project name

		if (tt::IsSameStrI(pszProject, "src")) {
			pszProject--;
			*pszProject = 0;
			pszProject = tt::FindFilePortion(szCwd);
		}
		SetControlText(DLG_ID(IDEDIT_PROJ_NAME), pszProject);
	}

	if (m_pOpts->IsExeTypeConsole())
		SetCheck(DLG_ID(IDRADIO_CONSOLE));
	else if (m_pOpts->IsExeTypeDll())
		SetCheck(DLG_ID(IDRADIO_DLL));
	else if (m_pOpts->IsExeTypeLib())
		SetCheck(DLG_ID(IDRADIO_LIB));
	else
		SetCheck(DLG_ID(IDRADIO_NORMAL));

	// Start by setting default directories

	if (m_pOpts->GetOption(OPT_TARGET_DIR32))
		SetControlText(DLG_ID(IDEDIT_DIR32), m_pOpts->GetOption(OPT_TARGET_DIR32));
	else
		SetControlText(DLG_ID(IDEDIT_DIR32), "bin32");

	if (m_pOpts->GetOption(OPT_TARGET_DIR64))
		SetControlText(DLG_ID(IDEDIT_DIR64), m_pOpts->GetOption(OPT_TARGET_DIR64));
	else
		SetControlText(DLG_ID(IDEDIT_DIR64), "bin");

	if (!m_pOpts->GetBoolOption(OPT_32BIT))
		SetCheck(DLG_ID(IDCHECK_64BIT));	// default to 64-bit builds
	else {
		SetCheck(DLG_ID(IDCHECK_32BIT));
		SetCheck(DLG_ID(IDCHECK_64BIT), m_pOpts->GetBoolOption(OPT_64BIT));
	}
}

void CTabGeneral::OnOK(void)
{
	ttCStr csz;

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_PROJ_NAME)));
	m_pOpts->UpdateOption(OPT_PROJECT, (char*) csz);

	if (GetCheck(DLG_ID(IDRADIO_CONSOLE)))
		m_pOpts->UpdateOption(OPT_EXE_TYPE, "console");
	else if (GetCheck(DLG_ID(IDRADIO_DLL)))
		m_pOpts->UpdateOption(OPT_EXE_TYPE, "dll");
	else if (GetCheck(DLG_ID(IDRADIO_LIB)))
		m_pOpts->UpdateOption(OPT_EXE_TYPE, "lib");
	else
		m_pOpts->UpdateOption(OPT_EXE_TYPE, "window");

	if (GetCheck(DLG_ID(IDCHECK_32BIT))) {
		m_pOpts->UpdateOption(OPT_32BIT, true);
		csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_DIR32)));
		if (csz.IsNonEmpty())
			m_pOpts->UpdateOption(OPT_TARGET_DIR32, (char*) csz);
	}

	if (GetCheck(DLG_ID(IDCHECK_64BIT))) {
		m_pOpts->UpdateOption(OPT_64BIT, true);
		csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_DIR64)));
		if (csz.IsNonEmpty())
			m_pOpts->UpdateOption(OPT_TARGET_DIR64, (char*) csz);
	}
}

void CTabGeneral::OnBtnDir32()
{
	ttCDirDlg dlg;
	dlg.SetTitle(tt::GetResString(IDS_32BIT_DIR));

	ttCStr cszDir;
	cszDir.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_DIR32)));
	cszDir.GetFullPathName();
	if (!tt::DirExists(cszDir))	// SHCreateItemFromParsingName will fail if the folder doesn't already exist
		cszDir.GetCWD();

	dlg.SetStartingDir(cszDir);
	if (dlg.GetFolderName(*this)) {
		ttCStr cszCWD;
		cszCWD.GetCWD();
		tt::ConvertToRelative(cszCWD, dlg, cszDir);
		SetControlText(DLG_ID(IDEDIT_DIR32), cszDir);
	}
}

void CTabGeneral::OnBtnDir64()
{
	ttCDirDlg dlg;
	dlg.SetTitle(tt::GetResString(IDS_64BIT_DIR));

	ttCStr cszDir;
	cszDir.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_DIR64)));
	cszDir.GetFullPathName();
	if (!tt::DirExists(cszDir))	// SHCreateItemFromParsingName will fail if the folder doesn't already exist
		cszDir.GetCWD();

	dlg.SetStartingDir(cszDir);
	if (dlg.GetFolderName(*this)) {
		ttCStr cszCWD;
		cszCWD.GetCWD();
		tt::ConvertToRelative(cszCWD, dlg, cszDir);
		SetControlText(DLG_ID(IDEDIT_DIR64), cszDir);
	}
}
