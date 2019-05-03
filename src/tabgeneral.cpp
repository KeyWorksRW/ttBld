/////////////////////////////////////////////////////////////////////////////
// Name:		CTabGeneral
// Purpose:		IDTAB_GENERAL dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "taboptions.h"

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
}

void CTabGeneral::OnOK(void)
{
	ttCStr csz;

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_PROJ_NAME)));
	m_pOpts->UpdateOption(OPT_PROJECT, (char*) csz);
}

