/////////////////////////////////////////////////////////////////////////////
// Name:		CTabGeneral
// Purpose:		IDTAB_GENERAL dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "dlgoptions.h"

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
}

