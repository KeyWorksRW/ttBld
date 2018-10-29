/////////////////////////////////////////////////////////////////////////////
// Name:		CDlgSrcOptions
// Purpose:		Class for displaying a dialog allowing for modification of .srcfiles
// Author:		Ralph Walden
// Copyright:	Copyright (c) 1998-2018 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#include "../ttLib/include/cstr.h"		// CStr
#include "../ttLib/include/filedlg.h"	// CFileDlg
#include "../ttLib/include/findfile.h"	// CTTFindFile

#include "dlgsrcoptions.h"

void SetSrcFileOptions()
{
	CDlgSrcOptions dlg;
	if (dlg.DoModal() == IDOK)
		dlg.SaveChanges();
}

CDlgSrcOptions::CDlgSrcOptions(const char* pszSrcDir) : CTTDlg(IDDLG_SRCFILES), CWriteSrcFiles()
{
	ReadFile();	// read in any existing .srcfiles

	if (pszSrcDir && *pszSrcDir)
		m_cszSrcDir = pszSrcDir;

	if (m_WarningLevel == 0) {
		m_WarningLevel = 4;
	}

	if (m_cszPCHheader.IsEmpty()) {
		if (FileExists("stdafx.h"))	{
			m_cszPCHheader = "stdafx.h";
		}
		else if (FileExists("pch.h")) {
			m_cszPCHheader = "pch.h";
		}
		else if (FileExists("pch.hh")) {
			m_cszPCHheader = "pch.hh";
		}
		else if (FileExists("pch.hpp")) {
			m_cszPCHheader = "pch.hpp";
		}
		else if (FileExists("pch.hxx")) {
			m_cszPCHheader = "pch.hxx";
		}
		else if (FileExists("precomp.h")) {
			m_cszPCHheader = "precomp.h";
		}
	}

	if (m_lstSrcFiles.GetCount() < 1) {
		CStr cszPattern;
		if (m_cszSrcDir.IsNonEmpty()) {
			cszPattern = m_cszSrcDir;
			cszPattern.AppendFileName("*.c*");
		}
		else
			 cszPattern = "*.c*";

		CTTFindFile ff(cszPattern);
		if (ff.isValid()) {
			do {
				if (m_cszSrcDir.IsNonEmpty()) {
					CStr cszFile(m_cszSrcDir);
					cszFile.AppendFileName(ff);
					m_lstSrcFiles += (char*) cszFile;
				}
				else
					m_lstSrcFiles += (const char*) ff;
			} while(ff.NextFile());
		}

		if (m_cszSrcDir.IsNonEmpty()) {
			cszPattern = m_cszSrcDir;
			cszPattern.AppendFileName("*.rc");
		}
		else
			 cszPattern = "*.rc";

		if (ff.NewPattern(cszPattern)) {
			do {
				if (m_cszSrcDir.IsNonEmpty()) {
					CStr cszFile(m_cszSrcDir);
					cszFile.AppendFileName(ff);
					m_lstSrcFiles += (char*) cszFile;
				}
				else
					m_lstSrcFiles += (const char*) ff;
			} while(ff.NextFile());
		}
	}

	m_fFileListChanged = false;
}

void CDlgSrcOptions::SaveChanges()
{
	if (FileExists(txtSrcFilesFileName)) {
		if (WriteUpdates())
			puts(".srcfiles updated");
	}
	else {
		if (WriteUpdates())
			puts(".srcfiles created");
	}
}

void CDlgSrcOptions::OnBegin(void)
{
	CStr cszTitle(txtSrcFilesFileName);
	cszTitle += " Options";
	SetWindowText(*this, cszTitle);

	m_lb.Initialize(*this, DLG_ID(IDLIST_FILES));
	for (size_t pos = 0; pos < m_lstSrcFiles.GetCount(); pos++) {
		m_lb.Add(m_lstSrcFiles[pos]);
	}

	if (GetProjectName())
		SetControlText(DLG_ID(IDEDIT_PROJ_NAME), GetProjectName());
	else {
		char szCwd[MAX_PATH];
		GetCurrentDirectory(sizeof(szCwd), szCwd);
		char* pszProject = FindFilePortion(szCwd);
		if (IsSameString(pszProject, "src")) {
			pszProject--;
			*pszProject = 0;
			pszProject = FindFilePortion(szCwd);
		}
		SetControlText(DLG_ID(IDEDIT_PROJ_NAME), pszProject);
	}

	if (m_cszPCHheader.IsNonEmpty())
		SetControlText(DLG_ID(IDEDIT_PCH), m_cszPCHheader);
	if (m_cszCFlags.IsNonEmpty())
		SetControlText(DLG_ID(IDEDIT_CFLAGS), m_cszCFlags);
	if (m_cszLinkFlags.IsNonEmpty())
		SetControlText(DLG_ID(IDEDIT_LINK_FLAGS), m_cszLinkFlags);
	if (m_cszLibs.IsNonEmpty())
		SetControlText(DLG_ID(IDEDIT_LIBS), m_cszLibs);
	if (m_cszBuildLibs.IsNonEmpty())
		SetControlText(DLG_ID(IDEDIT_LIBS_BUILD), m_cszBuildLibs);
	if (m_cszIncDirs.IsNonEmpty())
		SetControlText(DLG_ID(IDEDIT_INCDIRS), m_cszIncDirs);
	if (m_cszLibDirs.IsNonEmpty())
		SetControlText(DLG_ID(IDEDIT_LIBDIRS), m_cszLibDirs);

	if (m_exeType == EXE_CONSOLE)
		SetCheck(DLG_ID(IDRADIO_CONSOLE));
	else if (m_exeType == EXE_DLL)
		SetCheck(DLG_ID(IDRADIO_DLL));
	else if (m_exeType == EXE_LIB)
		SetCheck(DLG_ID(IDRADIO_LIB));
	else
		SetCheck(DLG_ID(IDRADIO_NORMAL));

	if (m_b64bit)
		SetCheck(DLG_ID(IDCHECK_64BIT));
	if (m_bBitSuffix)
		SetCheck(DLG_ID(IDCHECK_BITEXT));
	if (m_bStaticCrt)
		SetCheck(DLG_ID(IDCHECK_STATIC_CRT));
	if (m_bBuildForSpeed)
		SetCheck(DLG_ID(IDC_RADIO_SPEED));
	else
		SetCheck(DLG_ID(IDC_RADIO_SPACE));
	if (m_bPermissive)
		SetCheck(DLG_ID(IDC_CHECK_PERMISSIVE));
	if (m_bStdcall)
		SetCheck(DLG_ID(IDC_RADIO_STDCALL));
	if (m_bUseMsvcLinker)
		SetCheck(DLG_ID(IDCHECK_MSLINKER));

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

	// IDE: section options

	if (m_IDE & IDE_VS)
		SetCheck(DLG_ID(IDCHECK_VCXPROJ));
	if (m_IDE & IDE_CODELITE)
		SetCheck(DLG_ID(IDCHECK_CODELITE));
	if (m_IDE & IDE_CODEBLOCK)
		SetCheck(DLG_ID(IDCHECK_CODEBLOCKS));

	// Compilers: section options

	if (m_CompilerType & COMPILER_MSVC)
		SetCheck(DLG_ID(IDCHECK_MSVC));
	if (m_CompilerType & COMPILER_CLANG || m_CompilerType == COMPILER_DEFAULT)
		SetCheck(DLG_ID(IDCHECK_CLANG));

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

	if (m_MakeFileCompiler == COMPILER_CLANG)
		SetCheck(DLG_ID(IDRADIO_MF_CLANG));
	else
		SetCheck(DLG_ID(IDRADIO_MF_MSVC));

	if (isSameString(m_cszDefaultTarget, "release"))
		SetCheck(DLG_ID(IDRADIO_MF_RELEASE));
	else
		SetCheck(DLG_ID(IDRADIO_MF_DEBUG));
}

void CDlgSrcOptions::OnEnd(void)
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

	m_b64bit = GetCheck(DLG_ID(IDCHECK_64BIT));
	m_bBitSuffix = GetCheck(DLG_ID(IDCHECK_BITEXT));
	m_bStaticCrt = GetCheck(DLG_ID(IDCHECK_STATIC_CRT));
	m_bBuildForSpeed = GetCheck(DLG_ID(IDC_RADIO_SPEED));
	m_bPermissive = GetCheck(DLG_ID(IDC_CHECK_PERMISSIVE));
	m_bStdcall = GetCheck(DLG_ID(IDC_RADIO_STDCALL));
	m_bUseMsvcLinker = GetCheck(DLG_ID(IDCHECK_MSLINKER));

	if (GetCheck(DLG_ID(IDRADIO_WARN1)))
		m_WarningLevel = 1;
	else if (GetCheck(DLG_ID(IDRADIO_WARN2)))
		m_WarningLevel = 2;
	else if (GetCheck(DLG_ID(IDRADIO_WARN3)))
		m_WarningLevel = 3;
	else if (GetCheck(DLG_ID(IDRADIO_WARN4)))
		m_WarningLevel = 4;

	if (m_fFileListChanged) {
		auto cbItems = m_lb.GetCount();
		if (cbItems > 0) {
			if ((size_t) cbItems > m_lstSrcFiles.GetCount()) {
				m_lstSrcFiles.RemoveAll();
				for (auto pos = cbItems - 1; pos >= 0; pos--) {		// This assumes that whatever type m_lb.GetCount() uses, it will be signed
					CStr cszFile;
					m_lb.GetText(&cszFile, (int) pos);
					m_lstSrcFiles.Add(cszFile);
				}
			}
		}
	}

	// Find out which build scripts to generate

	m_IDE = IDE_NONE;
	if (GetCheck(DLG_ID(IDCHECK_VCXPROJ)))
		m_IDE |= IDE_VS;
	if (GetCheck(DLG_ID(IDCHECK_CODELITE)))
		m_IDE |= IDE_CODELITE;
	if (GetCheck(DLG_ID(IDCHECK_CODEBLOCKS)))
		m_IDE |= IDE_CODEBLOCK;

	// Find out which compiler to generate a script for

	m_CompilerType = 0;
	if (GetCheck(DLG_ID(IDCHECK_MSVC)))
		m_CompilerType |= COMPILER_MSVC;
	if (GetCheck(DLG_ID(IDCHECK_CLANG)))
		m_CompilerType |= COMPILER_CLANG;

	// Makefile: section options

	if (GetCheck(DLG_ID(IDRADIO_MF_NEVER)))
		m_fCreateMakefile = MAKEMAKE_NEVER;
	else if (GetCheck(DLG_ID(IDRADIO_MF_ALWAYS)))
		m_fCreateMakefile = MAKEMAKE_ALWAYS;
	else
		m_fCreateMakefile = MAKEMAKE_MISSING;

	m_MakeFileCompiler = GetCheck(DLG_ID(IDRADIO_MF_CLANG)) ? COMPILER_CLANG : COMPILER_MSVC;
	m_cszDefaultTarget = GetCheck(DLG_ID(IDRADIO_MF_RELEASE)) ? "release" : "debug";
}

void CDlgSrcOptions::OnBtnChangePch()
{
	CFileDlg fdlg(*this);
	fdlg.SetFilter("Header Files|*.h");
	CStr cszCWD;
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
		CStr cszFile(pszFileName);
		cszFile.ChangeExtension(".cpp");
		if (FileExists(cszFile)) {
			SetControlText(DLG_ID(IDEDIT_PCH), pszFileName);
			return;
		}
		cszFile.ChangeExtension(".cxx");
		if (FileExists(cszFile)) {
			SetControlText(DLG_ID(IDEDIT_PCH), pszFileName);
			return;
		}
		cszFile.ChangeExtension(".ccc");
		if (FileExists(cszFile)) {
			SetControlText(DLG_ID(IDEDIT_PCH), pszFileName);
			return;
		}
		cszFile.Delete();
		cszFile.printf("%s does not have a matching C++ source file -- precompiled header will fail without it!", (char*) pszFileName);
		MsgBox(cszFile);
	}
}

void CDlgSrcOptions::OnBtnAddFile()
{
	CFileDlg fdlg(*this);
	fdlg.SetFilter("Source Files|*.c;*.cc;*.cxx;*.cpp;*.rc;*.idl");
	CStr cszCWD;
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
		if (m_lb.FindString(pszFileName) == LB_ERR) {
			m_lb.Add(pszFileName);
			m_fFileListChanged = true;
		}
	}
}

void CDlgSrcOptions::OnBtnREmove()
{
	auto sel = m_lb.GetCurSel();
	if (sel == LB_ERR) {
		MsgBox("You need to select a file to delete first!");
		return;
	}

	// Need to confirm if attempting to delete the source file used to build a precompiled header

	CStr cszFile;
	cszFile.GetListBoxText(GetDlgItem(DLG_ID(IDLIST_FILES)));
	CStr cszPCH;
	cszPCH.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_PCH)));
	if (cszPCH.IsNonEmpty()) {
		char* pszExtension = kstrchrR(cszPCH, '.');
		if (pszExtension) {
			cszFile.ChangeExtension(pszExtension);
			if (IsSameString(cszFile, cszPCH)) {
				if (MsgBox("Removing this file will prevent Precompiled Headers. Continue?", MB_YESNO | MB_ICONWARNING) == IDYES)
					SetControlText(DLG_ID(IDEDIT_PCH), "");
				else
					return;
			}
		}
	}

	m_lb.DeleteString(sel);
	m_fFileListChanged = true;
}

