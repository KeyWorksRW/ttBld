/////////////////////////////////////////////////////////////////////////////
// Name:		CDlgSrcOptions
// Purpose:		Class for displaying a dialog allowing for modification of .srcfiles
// Author:		Ralph Walden
// Copyright:	Copyright (c) 1998-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttstr.h>		// ttCStr
#include <ttfiledlg.h>	// ttCFileDlg
#include <ttfindfile.h> // ttCFindFile

#include "dlgsrcoptions.h"
#include "ttlibicons.h" 				// Icons for use on 3D shaded buttons (ttShadeBtn)

bool SetSrcFileOptions(bool bDryRun)
{
	CDlgSrcOptions dlg;
	if (bDryRun)
		dlg.EnableDryRun();

	if (dlg.DoModal(NULL) == IDOK) {
		dlg.SaveChanges();
		return true;
	}
	else
		return false;
}

CDlgSrcOptions::CDlgSrcOptions(const char* pszSrcDir) : ttCDlg(IDDLG_SRCFILES), CWriteSrcFiles()
{
	ReadFile();	// read in any existing .srcfiles

	if (pszSrcDir && *pszSrcDir)
		m_cszSrcDir = pszSrcDir;

	if (tt::IsEmpty(GetPchHeader())) {
		if (tt::FileExists("stdafx.h"))
			UpdateOption(OPT_PCH, "stdafx.h");
		else if (tt::FileExists("pch.h"))
			UpdateOption(OPT_PCH, "pch.h");
		else if (tt::FileExists("precomp.h"))
			UpdateOption(OPT_PCH, "precomp.h");

		else if (tt::FileExists("pch.hh"))
			UpdateOption(OPT_PCH, "pch.hh");
		else if (tt::FileExists("pch.hpp"))
			UpdateOption(OPT_PCH, "pch.hpp");
		else if (tt::FileExists("pch.hxx"))
			UpdateOption(OPT_PCH, "pch.hxx");
	}
}

void CDlgSrcOptions::SaveChanges()
{
	if (tt::FileExists(txtSrcFilesFileName)) {
		if (WriteUpdates())
			puts(".srcfiles Options: section updated");
	}
	else {
		if (WriteUpdates())
			puts(".srcfiles created");
	}
}
void CDlgSrcOptions::OnBegin(void)
{
	EnableShadeBtns();
	CenterWindow(true);
	SetBtnIcon(DLG_ID(IDOK), IDICON_TTLIB_OK);
	SetBtnIcon(DLG_ID(IDCANCEL), IDICON_TTLIB_CANCEL);

	ttCStr cszTitle(txtSrcFilesFileName);
	cszTitle += " Options";
	SetWindowText(*this, cszTitle);

	if (GetProjectName())
		SetControlText(DLG_ID(IDEDIT_PROJ_NAME), GetProjectName());
	else {
		char szCwd[MAX_PATH];
		GetCurrentDirectory(sizeof(szCwd), szCwd);
		char* pszProject = tt::FindFilePortion(szCwd);
		if (tt::IsSameStrI(pszProject, "src")) {
			pszProject--;
			*pszProject = 0;
			pszProject = tt::FindFilePortion(szCwd);
		}
		SetControlText(DLG_ID(IDEDIT_PROJ_NAME), pszProject);
	}

	if (GetPchHeader())
		SetControlText(DLG_ID(IDEDIT_PCH), GetPchHeader());
	if (GetOption(OPT_CFLAGS))
		SetControlText(DLG_ID(IDEDIT_CFLAGS), GetOption(OPT_CFLAGS));
	if (GetOption(OPT_LINK_FLAGS))
		SetControlText(DLG_ID(IDEDIT_LINK_FLAGS), GetOption(OPT_LINK_FLAGS));
	if (GetOption(OPT_LIBS))
		SetControlText(DLG_ID(IDEDIT_LIBS), GetOption(OPT_LIBS));
	if (GetOption(OPT_BUILD_LIBS))
		SetControlText(DLG_ID(IDEDIT_LIBS_BUILD), GetOption(OPT_BUILD_LIBS));
	if (GetOption(OPT_INC_DIRS))
		SetControlText(DLG_ID(IDEDIT_INCDIRS), GetOption(OPT_INC_DIRS));
	if (GetOption(OPT_LIB_DIRS))
		SetControlText(DLG_ID(IDEDIT_LIBDIRS), GetOption(OPT_LIB_DIRS));

	if (IsExeTypeConsole())
		SetCheck(DLG_ID(IDRADIO_CONSOLE));
	else if (IsExeTypeDll())
		SetCheck(DLG_ID(IDRADIO_DLL));
	else if (IsExeTypeLib())
		SetCheck(DLG_ID(IDRADIO_LIB));
	else
		SetCheck(DLG_ID(IDRADIO_NORMAL));

	SetCheck(DLG_ID(IDCHECK_64BIT), GetBoolOption(OPT_64BIT));
	SetCheck(DLG_ID(IDCHECK_BITEXT), GetBoolOption(OPT_BIT_SUFFIX));
	SetCheck(DLG_ID(IDC_CHECK_PERMISSIVE), GetBoolOption(OPT_PERMISSIVE));
	SetCheck(DLG_ID(IDCHECK_STATIC_CRT), GetBoolOption(OPT_STATIC_CRT));
	SetCheck(DLG_ID(IDCHECK_MSLINKER), GetBoolOption(OPT_MS_LINKER));

	SetCheck(DLG_ID(IsOptimizeSpeed() ? IDC_RADIO_SPEED : IDC_RADIO_SPACE));
	SetCheck(DLG_ID(IDCHECK_MSLINKER), GetBoolOption(OPT_MS_LINKER));
	SetCheck(DLG_ID(GetBoolOption(OPT_STDCALL) ? IDC_RADIO_STDCALL :IDC_RADIO_CDECL));

	ptrdiff_t warnLevel = GetOption(OPT_WARN_LEVEL) ? tt::Atoi(GetOption(OPT_WARN_LEVEL)) : 4;

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

	// Makefile: section options

	if (IsMakeNever())
		SetCheck(DLG_ID(IDRADIO_MF_NEVER));

	else if (IsMakeAlways())
		SetCheck(DLG_ID(IDRADIO_MF_ALWAYS));
	else
		SetCheck(DLG_ID(IDRADIO_MF_MISSING));
}

void CDlgSrcOptions::OnOK(void)
{
	ttCStr csz;

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_PROJ_NAME)));
	UpdateOption(OPT_PROJECT, (char*) csz);

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_CFLAGS)));
	UpdateOption(OPT_CFLAGS, (char*) csz);

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LINK_FLAGS)));
	UpdateOption(OPT_LINK_FLAGS, (char*) csz);

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_INCDIRS)));
	UpdateOption(OPT_INC_DIRS, (char*) csz);

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBDIRS)));
	UpdateOption(OPT_LIB_DIRS, (char*) csz);

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBS_LINK)));
	UpdateOption(OPT_LIBS, (char*) csz);

	csz.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBS_BUILD)));
	UpdateOption(OPT_BUILD_LIBS, (char*) csz);

	if (GetCheck(DLG_ID(IDRADIO_CONSOLE)))
		UpdateOption(OPT_EXE_TYPE, "console");
	else if (GetCheck(DLG_ID(IDRADIO_DLL)))
		UpdateOption(OPT_EXE_TYPE, "dll");
	else if (GetCheck(DLG_ID(IDRADIO_LIB)))
		UpdateOption(OPT_EXE_TYPE, "lib");
	else
		UpdateOption(OPT_EXE_TYPE, "window");

	if (GetCheck(DLG_ID(IDC_RADIO_SPEED)))
		UpdateOption(OPT_OPTIMIZE, "speed");
	else
		UpdateOption(OPT_OPTIMIZE, "space");

	UpdateOption(OPT_64BIT, GetCheck(DLG_ID(IDCHECK_64BIT)));
	UpdateOption(OPT_BIT_SUFFIX, GetCheck(DLG_ID(IDCHECK_BITEXT)));
	UpdateOption(OPT_PERMISSIVE, GetCheck(DLG_ID(IDC_CHECK_PERMISSIVE)));
	UpdateOption(OPT_STDCALL, GetCheck(DLG_ID(IDC_RADIO_STDCALL)));
	UpdateOption(OPT_STATIC_CRT, GetCheck(DLG_ID(IDCHECK_STATIC_CRT)));
	UpdateOption(OPT_MS_LINKER, GetCheck(DLG_ID(IDCHECK_MSLINKER)));

	if (GetCheck(DLG_ID(IDRADIO_WARN1)))
		UpdateOption(OPT_WARN_LEVEL, "1");
	else if (GetCheck(DLG_ID(IDRADIO_WARN2)))
		UpdateOption(OPT_WARN_LEVEL, "2");
	else if (GetCheck(DLG_ID(IDRADIO_WARN3)))
		UpdateOption(OPT_WARN_LEVEL, "3");
	else if (GetCheck(DLG_ID(IDRADIO_WARN4)))
		UpdateOption(OPT_WARN_LEVEL, "4");

	// Makefile: section options

	if (GetCheck(DLG_ID(IDRADIO_MF_NEVER)))
		UpdateOption(OPT_MAKEFILE, "never");
	else if (GetCheck(DLG_ID(IDRADIO_MF_ALWAYS)))
		UpdateOption(OPT_MAKEFILE, "always");
	else
		UpdateOption(OPT_MAKEFILE, "missing");
}

void CDlgSrcOptions::OnBtnChangePch()
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
