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
		if (dlg.m_cszPCHheader.isNonEmpty()) {
			cszPCHSrc = dlg.m_cszPCHheader;
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

	if (m_cszPCHheader.isEmpty()) {
		if (tt::FileExists("stdafx.h"))
			m_cszPCHheader = "stdafx.h";
		else if (tt::FileExists("pch.h"))
			m_cszPCHheader = "pch.h";
		else if (tt::FileExists("precomp.h"))
			m_cszPCHheader = "precomp.h";

		else if (tt::FileExists("pch.hh"))
			m_cszPCHheader = "pch.hh";
		else if (tt::FileExists("pch.hpp"))
			m_cszPCHheader = "pch.hpp";
		else if (tt::FileExists("pch.hxx"))
			m_cszPCHheader = "pch.hxx";
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

	if (m_cszPCHheader.isNonEmpty())
		SetControlText(DLG_ID(IDEDIT_PCH), m_cszPCHheader);
	if (m_cszCFlags.isNonEmpty())
		SetControlText(DLG_ID(IDEDIT_CFLAGS), m_cszCFlags);
	if (m_cszLinkFlags.isNonEmpty())
		SetControlText(DLG_ID(IDEDIT_LINK_FLAGS), m_cszLinkFlags);
	if (m_cszLibs.isNonEmpty())
		SetControlText(DLG_ID(IDEDIT_LIBS), m_cszLibs);
	if (m_cszBuildLibs.isNonEmpty())
		SetControlText(DLG_ID(IDEDIT_LIBS_BUILD), m_cszBuildLibs);
	if (m_cszIncDirs.isNonEmpty())
		SetControlText(DLG_ID(IDEDIT_INCDIRS), m_cszIncDirs);
	if (m_cszLibDirs.isNonEmpty())
		SetControlText(DLG_ID(IDEDIT_LIBDIRS), m_cszLibDirs);

	if (m_exeType == EXE_CONSOLE)
		SetCheck(DLG_ID(IDRADIO_CONSOLE));
	else if (m_exeType == EXE_DLL)
		SetCheck(DLG_ID(IDRADIO_DLL));
	else if (m_exeType == EXE_LIB)
		SetCheck(DLG_ID(IDRADIO_LIB));
	else
		SetCheck(DLG_ID(IDRADIO_NORMAL));

	ttDDX_Check(DLG_ID(IDCHECK_64BIT), m_b64bit);
	ttDDX_Check(DLG_ID(IDCHECK_BITEXT), m_bBitSuffix);
	ttDDX_Check(DLG_ID(IDCHECK_STATIC_CRT), m_bStaticCrt);
	ttDDX_Check(DLG_ID(IDC_CHECK_PERMISSIVE), m_bPermissive);
	ttDDX_Check(DLG_ID(IDCHECK_MSLINKER), m_bUseMsvcLinker);

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
	ttCStr csz;
	csz.getWindowText(GetDlgItem(DLG_ID(IDEDIT_PROJ_NAME)));
	UpdateOption(OPT_PROJECT, csz);

	m_cszCFlags.getWindowText(GetDlgItem(DLG_ID(IDEDIT_CFLAGS)));
	m_cszLinkFlags.getWindowText(GetDlgItem(DLG_ID(IDEDIT_LINK_FLAGS)));
	m_cszLibs.getWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBS_LINK)));
	m_cszBuildLibs.getWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBS_BUILD)));
	m_cszIncDirs.getWindowText(GetDlgItem(DLG_ID(IDEDIT_INCDIRS)));
	m_cszLibDirs.getWindowText(GetDlgItem(DLG_ID(IDEDIT_LIBDIRS)));

	if (GetCheck(DLG_ID(IDRADIO_CONSOLE)))
		m_exeType = EXE_CONSOLE;
	else if (GetCheck(DLG_ID(IDRADIO_DLL)))
		m_exeType = EXE_DLL;
	else if (GetCheck(DLG_ID(IDRADIO_LIB)))
		m_exeType = EXE_LIB;
	else
		m_exeType = EXE_WINDOW;

	ttDDX_Check(DLG_ID(IDCHECK_64BIT), m_b64bit);
	ttDDX_Check(DLG_ID(IDCHECK_BITEXT), m_bBitSuffix);
	ttDDX_Check(DLG_ID(IDCHECK_STATIC_CRT), m_bStaticCrt);
	ttDDX_Check(DLG_ID(IDC_CHECK_PERMISSIVE), m_bPermissive);
	ttDDX_Check(DLG_ID(IDCHECK_MSLINKER), m_bUseMsvcLinker);

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
