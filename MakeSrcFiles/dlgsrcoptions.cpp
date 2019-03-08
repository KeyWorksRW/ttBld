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

void SetSrcFileOptions(bool bDryRun)
{
	CDlgSrcOptions dlg;
	if (bDryRun)
		dlg.EnableDryRun();

	if (dlg.DoModal() == IDOK)
		dlg.SaveChanges();
}

void CreateNewSrcFiles()
{
	if (tt::FileExists(".srcfiles")) {
		if (tt::MsgBox(".srcfiles already exists. Are you certain you want to replace it with a new one?", MB_YESNO) != IDYES)
			return;
	}

	CDlgSrcOptions dlg;
	if (dlg.DoModal() == IDOK) {
		// CWriteSrcFiles only writes out the Options: section since that's all that's needed by MakeNinja. For the -new
		// option, we first create a .srcfiles with nothing but a Files: section, and then call SaveChanges() to add the
		// Options: section.

		ttCFile kfOut;
		kfOut.WriteEol("Files:");

		dlg.AddSourcePattern("*.cpp;*.cc;*.cxx;*.c;*.rc;*.idl;*.hhp;");
		ttCStr cszPCHSrc;
		bool bSpecialFiles = false;
		if (tt::isEmpty(dlg.GetPchHeader())) {
			cszPCHSrc = dlg.GetPchHeader();
			cszPCHSrc.ChangeExtension(".cpp");
			if (!tt::FileExists(cszPCHSrc)) {
				cszPCHSrc.ChangeExtension(".c");
				if (!tt::FileExists(cszPCHSrc))
					cszPCHSrc.Delete();
			}
			if (cszPCHSrc.isNonEmpty())	{
				kfOut.printf("  %s\n", (char*) cszPCHSrc);
				bSpecialFiles = true;
			}
		}
		if (dlg.m_cszHHPName.isNonEmpty()) {
			kfOut.printf("  %s\n", (char*) dlg.m_cszHHPName);
			bSpecialFiles = true;
		}
		if (dlg.m_cszRcName.isNonEmpty()) {
			kfOut.printf("  %s\n", (char*) dlg.m_cszRcName);
			bSpecialFiles = true;
		}
		if (bSpecialFiles)
			kfOut.WriteEol();	// add a blank line

		for (size_t pos = 0; pos < dlg.m_lstSrcFiles.GetCount(); ++pos) {
			if (cszPCHSrc.isNonEmpty() && cszPCHSrc.isSameStri(dlg.m_lstSrcFiles[pos]))
				continue;	// we already added it
			else if (dlg.m_cszRcName.isNonEmpty() && dlg.m_cszRcName.isSameStri(dlg.m_lstSrcFiles[pos]))
				continue;	// we already added it
			else if (dlg.m_cszHHPName.isNonEmpty() && dlg.m_cszHHPName.isSameStri(dlg.m_lstSrcFiles[pos]))
				continue;	// we already added it

			kfOut.printf("  %s\n", dlg.m_lstSrcFiles[pos]);
		}
		if (!kfOut.WriteFile(".srcfiles")) {
			puts("Unable to write to .srcfiles!");
			return;
		}

		puts(".srcfiles created");
		dlg.SaveChanges();	// now add the Options: section
	}
}

CDlgSrcOptions::CDlgSrcOptions(const char* pszSrcDir) : ttCDlg(IDDLG_SRCFILES), CWriteSrcFiles()
{
	EnableShadeBtns();
	ReadFile();	// read in any existing .srcfiles

	if (pszSrcDir && *pszSrcDir)
		m_cszSrcDir = pszSrcDir;

	if (m_WarningLevel == 0) {
		m_WarningLevel = WARNLEVEL_DEFAULT;
	}

	if (tt::isEmpty(GetPchHeader())) {
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
	m_ShadedBtns.SetIcon(DLG_ID(IDOK), IDICON_TTLIB_OK);
	m_ShadedBtns.SetIcon(DLG_ID(IDCANCEL), IDICON_TTLIB_CANCEL);

	ttCStr cszTitle(txtSrcFilesFileName);
	cszTitle += " Options";
	SetWindowText(*this, cszTitle);

	if (GetProjectName())
		SetControlText(DLG_ID(IDEDIT_PROJ_NAME), GetProjectName());
	else {
		char szCwd[MAX_PATH];
		GetCurrentDirectory(sizeof(szCwd), szCwd);
		char* pszProject = tt::findFilePortion(szCwd);
		if (tt::isSameStri(pszProject, "src")) {
			pszProject--;
			*pszProject = 0;
			pszProject = tt::findFilePortion(szCwd);
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
	if (m_cszBuildLibs.isNonEmpty())
		SetControlText(DLG_ID(IDEDIT_LIBS_BUILD), m_cszBuildLibs);
	if (GetOption(OPT_INC_DIRS))
		SetControlText(DLG_ID(IDEDIT_INCDIRS), GetOption(OPT_INC_DIRS));
	if (GetOption(OPT_LIB_DIRS))
		SetControlText(DLG_ID(IDEDIT_LIBDIRS), GetOption(OPT_LIB_DIRS));

	if (m_exeType == EXE_CONSOLE)
		SetCheck(DLG_ID(IDRADIO_CONSOLE));
	else if (m_exeType == EXE_DLL)
		SetCheck(DLG_ID(IDRADIO_DLL));
	else if (m_exeType == EXE_LIB)
		SetCheck(DLG_ID(IDRADIO_LIB));
	else
		SetCheck(DLG_ID(IDRADIO_NORMAL));

	SetCheck(DLG_ID(IDCHECK_64BIT), GetBoolOption(OPT_64BIT));
	SetCheck(DLG_ID(IDCHECK_BITEXT), GetBoolOption(OPT_BIT_SUFFIX));
	SetCheck(DLG_ID(IDC_CHECK_PERMISSIVE), GetBoolOption(OPT_PERMISSIVE));
	SetCheck(DLG_ID(IDCHECK_STATIC_CRT), GetBoolOption(OPT_STATIC_CRT));
	SetCheck(DLG_ID(IDCHECK_MSLINKER), GetBoolOption(OPT_MS_LINKER));

	SetCheck(DLG_ID(m_bBuildForSpeed ? IDC_RADIO_SPEED : IDC_RADIO_SPACE));
	SetCheck(DLG_ID(GetBoolOption(OPT_STDCALL) ? IDC_RADIO_STDCALL :IDC_RADIO_CDECL));

	switch (m_WarningLevel) {
		case WARNLEVEL_1:
			SetCheck(DLG_ID(IDRADIO_WARN1));
			break;
		case WARNLEVEL_2:
			SetCheck(DLG_ID(IDRADIO_WARN2));
			break;
		case WARNLEVEL_3:
			SetCheck(DLG_ID(IDRADIO_WARN3));
			break;
		case WARNLEVEL_4:
		default:
			SetCheck(DLG_ID(IDRADIO_WARN4));
			break;
	}

	// Makefile: section options

	switch (m_fCreateMakefile) {
		case MAKEMAKE_NEVER:
			SetCheck(DLG_ID(IDRADIO_MF_NEVER));
			break;

		case MAKEMAKE_ALWAYS:
			SetCheck(DLG_ID(IDRADIO_MF_ALWAYS));
			break;

		case MAKEMAKE_MISSING:
		default:
			SetCheck(DLG_ID(IDRADIO_MF_MISSING));
			break;
	}
}

void CDlgSrcOptions::OnOK(void)
{
	ttCStr csz;

	csz.getWindowText(GetDlgItem(DLG_ID(IDEDIT_PROJ_NAME)));
	UpdateOption(OPT_PROJECT, (char*) csz);

	csz.getWindowText(GetDlgItem(DLG_ID(IDEDIT_CFLAGS)));
	UpdateOption(OPT_CFLAGS, (char*) csz);

	csz.getWindowText(GetDlgItem(DLG_ID(OPT_LINK_FLAGS)));
	UpdateOption(OPT_LINK_FLAGS, (char*) csz);

	csz.getWindowText(GetDlgItem(DLG_ID(IDEDIT_INCDIRS)));
	UpdateOption(OPT_INC_DIRS, (char*) csz);

	csz.getWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBDIRS)));
	UpdateOption(OPT_LIB_DIRS, (char*) csz);

	csz.getWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBS_LINK)));
	UpdateOption(OPT_LIBS, (char*) csz);

	m_cszBuildLibs.getWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBS_BUILD)));

	if (GetCheck(DLG_ID(IDRADIO_CONSOLE)))
		m_exeType = EXE_CONSOLE;
	else if (GetCheck(DLG_ID(IDRADIO_DLL)))
		m_exeType = EXE_DLL;
	else if (GetCheck(DLG_ID(IDRADIO_LIB)))
		m_exeType = EXE_LIB;
	else
		m_exeType = EXE_WINDOW;

	UpdateOption(OPT_64BIT, GetCheck(DLG_ID(IDCHECK_64BIT)));
	UpdateOption(OPT_BIT_SUFFIX, GetCheck(DLG_ID(IDCHECK_BITEXT)));
	UpdateOption(OPT_PERMISSIVE, GetCheck(DLG_ID(IDC_CHECK_PERMISSIVE)));
	UpdateOption(OPT_STDCALL, GetCheck(DLG_ID(IDC_RADIO_STDCALL)));
	UpdateOption(OPT_STATIC_CRT, GetCheck(DLG_ID(IDCHECK_STATIC_CRT)));
	UpdateOption(OPT_MS_LINKER, GetCheck(DLG_ID(IDCHECK_MSLINKER)));

	m_bBuildForSpeed = GetCheck(DLG_ID(IDC_RADIO_SPEED));

	if (GetCheck(DLG_ID(IDRADIO_WARN1)))
		m_WarningLevel = 1;
	else if (GetCheck(DLG_ID(IDRADIO_WARN2)))
		m_WarningLevel = 2;
	else if (GetCheck(DLG_ID(IDRADIO_WARN3)))
		m_WarningLevel = 3;
	else if (GetCheck(DLG_ID(IDRADIO_WARN4)))
		m_WarningLevel = 4;

	// Makefile: section options

	if (GetCheck(DLG_ID(IDRADIO_MF_NEVER)))
		m_fCreateMakefile = MAKEMAKE_NEVER;
	else if (GetCheck(DLG_ID(IDRADIO_MF_ALWAYS)))
		m_fCreateMakefile = MAKEMAKE_ALWAYS;
	else
		m_fCreateMakefile = MAKEMAKE_MISSING;
}

void CDlgSrcOptions::OnBtnChangePch()
{
	ttCFileDlg fdlg(*this);
	fdlg.SetFilter("Header Files|*.h");
	ttCStr cszCWD;
	cszCWD.getCWD();
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
