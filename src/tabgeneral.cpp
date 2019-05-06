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

	ttCStr cszDir64, cszDir32;

	ttCStr cszCWD;
	cszCWD.GetCWD();
	bool bSrcDir = ttstristr(tt::FindFilePortion(cszCWD), "src") ? true : false;
	if (!bSrcDir) {
		cszCWD.AppendFileName(m_pOpts->IsExeTypeLib() ? "../lib" : "../bin");
		if (tt::DirExists(cszCWD))
			bSrcDir = true;
	}

	if (bSrcDir) {
		cszDir64 = m_pOpts->IsExeTypeLib() ? "../lib" : "../bin";
		ttCStr cszTmp(cszDir64);
		cszTmp += "64";
		if (tt::DirExists(cszTmp)) {		// if there is a ../lib64 or ../bin64, then use that
			cszDir64 = cszTmp;
			cszDir32 = m_pOpts->IsExeTypeLib() ? "../lib" : "../bin";
			cszTmp = cszDir32;
			cszTmp += "32";
			if (tt::DirExists(cszTmp))
				cszDir32 = cszTmp;
		}
		else
			cszDir32 = m_pOpts->IsExeTypeLib() ? "../lib32" : "../bin32";
	}
	else {
		cszDir64 = m_pOpts->IsExeTypeLib() ? "lib" : "bin";
		ttCStr cszTmp(cszDir64);
		cszTmp += "64";
		if (tt::DirExists(cszTmp)) {		// if there is a lib64 or bin64, then use that
			cszDir64 = cszTmp;
			cszDir32 = m_pOpts->IsExeTypeLib() ? "lib" : "bin";
			cszTmp = cszDir32;
			cszTmp += "32";
			if (tt::DirExists(cszTmp))
				cszDir32 = cszTmp;
		}
		else
			cszDir32 = m_pOpts->IsExeTypeLib() ? "lib32" : "bin32";
	}

	if (m_pOpts->GetOption(OPT_TARGET_DIR32))
		SetControlText(DLG_ID(IDEDIT_DIR32), m_pOpts->GetOption(OPT_TARGET_DIR32));
	else
		SetControlText(DLG_ID(IDEDIT_DIR32), cszDir32);

	if (m_pOpts->GetOption(OPT_TARGET_DIR64))
		SetControlText(DLG_ID(IDEDIT_DIR64), m_pOpts->GetOption(OPT_TARGET_DIR64));
	else
		SetControlText(DLG_ID(IDEDIT_DIR64), cszDir64);

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