/////////////////////////////////////////////////////////////////////////////
// Name:		CDlgSrcOptions
// Purpose:		Class for displaying a dialog allowing for modification of .srcfiles
// Author:		Ralph Walden
// Copyright:	Copyright (c) 1998-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "../ttLib/include/ttstring.h"	// ttString
#include "../ttLib/include/filedlg.h"	// ttFileDlg
#include "../ttLib/include/findfile.h"	// ttFindFile

#include "dlgsrcoptions.h"

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
		puts(".srcfiles already exists! Delete it before using -new option, or use the -overwrite option.");
		return;
	}

	CDlgSrcOptions dlg;
	if (dlg.DoModal() == IDOK) {
		// CWriteSrcFiles only writes out the Options: section since that's all that's needed by MakeNinja. For the -new
		// option, we first create a .srcfiles with nothing but a Files: section, and then call SaveChanges() to add the
		// Options: section.

		ttFile kfOut;
		kfOut.WriteEol("Files:");

		dlg.AddSourcePattern("*.cpp;*.cc;*.cxx;*.c;*.rc;*.idl;*.hhp;");
		for (size_t pos = 0; pos < dlg.m_lstSrcFiles.GetCount(); ++pos) {
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

CDlgSrcOptions::CDlgSrcOptions(const char* pszSrcDir) : ttDlg(IDDLG_SRCFILES), CWriteSrcFiles()
{
	ReadFile();	// read in any existing .srcfiles

	if (pszSrcDir && *pszSrcDir)
		m_cszSrcDir = pszSrcDir;

	if (m_WarningLevel == 0) {
		m_WarningLevel = WARNLEVEL_DEFAULT;
	}

	if (m_cszPCHheader.isempty()) {
		if (tt::FileExists("stdafx.h"))
			m_cszPCHheader = "stdafx.h";
		else if (tt::FileExists("pch.h"))
			m_cszPCHheader = "pch.h";
		else if (tt::FileExists("pch.hh"))
			m_cszPCHheader = "pch.hh";
		else if (tt::FileExists("pch.hpp"))
			m_cszPCHheader = "pch.hpp";
		else if (tt::FileExists("pch.hxx"))
			m_cszPCHheader = "pch.hxx";
		else if (tt::FileExists("pch.h"))
			m_cszPCHheader = "pch.h";
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
	ttString cszTitle(txtSrcFilesFileName);
	cszTitle += " Options";
	SetWindowText(*this, cszTitle);

	if (GetProjectName())
		SetControlText(DLG_ID(IDEDIT_PROJ_NAME), GetProjectName());
	else {
		char szCwd[MAX_PATH];
		GetCurrentDirectory(sizeof(szCwd), szCwd);
		char* pszProject = tt::fndFilename(szCwd);
		if (tt::samestri(pszProject, "src")) {
			pszProject--;
			*pszProject = 0;
			pszProject = tt::fndFilename(szCwd);
		}
		SetControlText(DLG_ID(IDEDIT_PROJ_NAME), pszProject);
	}

	if (m_cszPCHheader.isnonempty())
		SetControlText(DLG_ID(IDEDIT_PCH), m_cszPCHheader);
	if (m_cszCFlags.isnonempty())
		SetControlText(DLG_ID(IDEDIT_CFLAGS), m_cszCFlags);
	if (m_cszLinkFlags.isnonempty())
		SetControlText(DLG_ID(IDEDIT_LINK_FLAGS), m_cszLinkFlags);
	if (m_cszLibs.isnonempty())
		SetControlText(DLG_ID(IDEDIT_LIBS), m_cszLibs);
	if (m_cszBuildLibs.isnonempty())
		SetControlText(DLG_ID(IDEDIT_LIBS_BUILD), m_cszBuildLibs);
	if (m_cszIncDirs.isnonempty())
		SetControlText(DLG_ID(IDEDIT_INCDIRS), m_cszIncDirs);
	if (m_cszLibDirs.isnonempty())
		SetControlText(DLG_ID(IDEDIT_LIBDIRS), m_cszLibDirs);

	if (m_exeType == EXE_CONSOLE)
		SetCheck(DLG_ID(IDRADIO_CONSOLE));
	else if (m_exeType == EXE_DLL)
		SetCheck(DLG_ID(IDRADIO_DLL));
	else if (m_exeType == EXE_LIB)
		SetCheck(DLG_ID(IDRADIO_LIB));
	else
		SetCheck(DLG_ID(IDRADIO_NORMAL));

	KDDX_Check(DLG_ID(IDCHECK_64BIT), m_b64bit);
	KDDX_Check(DLG_ID(IDCHECK_BITEXT), m_bBitSuffix);
	KDDX_Check(DLG_ID(IDCHECK_STATIC_CRT), m_bStaticCrt);
	KDDX_Check(DLG_ID(IDC_CHECK_PERMISSIVE), m_bPermissive);
	KDDX_Check(DLG_ID(IDCHECK_MSLINKER), m_bUseMsvcLinker);

	SetCheck(DLG_ID(m_bBuildForSpeed ? IDC_RADIO_SPEED : IDC_RADIO_SPACE));
	SetCheck(DLG_ID(m_bStdcall ? IDC_RADIO_STDCALL :IDC_RADIO_CDECL));

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
	m_cszProjectName.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_PROJ_NAME)));
	m_cszCFlags.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_CFLAGS)));
	m_cszLinkFlags.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LINK_FLAGS)));
	m_cszLibs.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBS_LINK)));
	m_cszBuildLibs.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBS_BUILD)));
	m_cszIncDirs.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_INCDIRS)));
	m_cszLibDirs.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBDIRS)));

	if (GetCheck(DLG_ID(IDRADIO_CONSOLE)))
		m_exeType = EXE_CONSOLE;
	else if (GetCheck(DLG_ID(IDRADIO_DLL)))
		m_exeType = EXE_DLL;
	else if (GetCheck(DLG_ID(IDRADIO_LIB)))
		m_exeType = EXE_LIB;
	else
		m_exeType = EXE_WINDOW;

	KDDX_Check(DLG_ID(IDCHECK_64BIT), m_b64bit);
	KDDX_Check(DLG_ID(IDCHECK_BITEXT), m_bBitSuffix);
	KDDX_Check(DLG_ID(IDCHECK_STATIC_CRT), m_bStaticCrt);
	KDDX_Check(DLG_ID(IDC_CHECK_PERMISSIVE), m_bPermissive);
	KDDX_Check(DLG_ID(IDCHECK_MSLINKER), m_bUseMsvcLinker);

	m_bBuildForSpeed = GetCheck(DLG_ID(IDC_RADIO_SPEED));
	m_bStdcall = GetCheck(DLG_ID(IDC_RADIO_STDCALL));

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
	ttFileDlg fdlg(*this);
	fdlg.SetFilter("Header Files|*.h");
	ttString cszCWD;
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
		ttString cszFile(pszFileName);
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
