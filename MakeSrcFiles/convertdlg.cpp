/////////////////////////////////////////////////////////////////////////////
// Name:		CConvertDlg
// Purpose:		IDDDLG_CONVERT dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <direct.h>

#include <ttfindfile.h>	// ttCFindFile
#include <ttfiledlg.h>	// ttCFileDlg
#include <ttdirdlg.h>	// ttCDirDlg

#include "convertdlg.h"
#include "ttlibicons.h"

static const char* atxtSrcTypes[] = {
	"*.cpp",
	"*.cc",
	"*.cxx",
	"*.c",

	nullptr
};

void CConvertDlg::OnBegin(void)
{
	m_ShadedBtns.SetIcon(DLG_ID(IDOK), IDICON_TTLIB_OK);
	m_ShadedBtns.SetIcon(DLG_ID(IDCANCEL), IDICON_TTLIB_CANCEL);

	ttCStr csz;
	csz.getCWD();
	SetControlText(DLG_ID(IDEDIT_OUT_DIR), csz);
	SetControlText(DLG_ID(IDEDIT_IN_DIR), csz);

	m_comboScripts.Initialize(*this, DLG_ID(IDCOMBO_SCRIPTS));

	ttCFindFile ff("*.vcxproj");
	if (ff.isValid())
		m_comboScripts.Add((char*) ff);
	if (ff.NewPattern("*.vcproj"))
		m_comboScripts.Add((char*) ff);
	if (ff.NewPattern("*.project"))
		m_comboScripts.Add((char*) ff);
	if (ff.NewPattern("*.cbp"))
		m_comboScripts.Add((char*) ff);

	size_t cFilesFound = 0;
	for (size_t pos = 0; atxtSrcTypes[pos]; ++pos) {
		if (ff.NewPattern(atxtSrcTypes[pos])) {
			do {
			   ++cFilesFound;
			} while(ff.NextFile());
		}
	}
	csz.printf("%kn file%ks located", cFilesFound, cFilesFound);
	SetControlText(DLG_ID(IDTXT_FILES_FOUND), csz);
}

void CConvertDlg::OnOK(void)
{
	m_cszDirOutput.getWindowText(GetDlgItem(DLG_ID(IDEDIT_OUT_DIR)));
	m_cszDirSrcFiles.getWindowText(GetDlgItem(DLG_ID(IDEDIT_IN_DIR)));
	if (GetCheck(DLG_ID(IDCHECK_SCRIPT)))
		m_cszConvertScript.getWindowText(GetDlgItem(DLG_ID(IDCOMBO_SCRIPTS)));
	else
		m_cszConvertScript.Delete();
}

void CConvertDlg::OnBtnLocateScript()
{
	ttCFileDlg dlg(*this);
	dlg.SetFilter("Project Files|*.vcxproj;*.vcproj;*.project;*.cbp");
	dlg.UseCurrentDirectory();
	if (dlg.GetOpenFileName()) {
		auto item = m_comboScripts.Add(dlg.GetFileName());
		m_comboScripts.SetCurSel(item);
		SetCheck(DLG_ID(IDCHECK_SCRIPT));
		// TODO: [randalphwa - 4/2/2019] Need to decide how to handle IDEDIT_IN_DIR since this may no longer be correct
	}
}

void CConvertDlg::OnBtnChangeOut()	// change the directory to write .srcfiles to
{
	ttCDirDlg dlg;
	ttCStr cszCWD;
	cszCWD.getCWD();
	dlg.SetStartingDir(cszCWD);
	if (dlg.GetFolderName(*this))
		SetControlText(DLG_ID(IDEDIT_OUT_DIR), dlg);
}

void CConvertDlg::OnBtnChangeIn()
{
	ttCDirDlg dlg;
	ttCStr cszCWD;
	cszCWD.getCWD();
	dlg.SetStartingDir(cszCWD);
	if (dlg.GetFolderName(*this)) {
		SetControlText(DLG_ID(IDEDIT_IN_DIR), dlg);
		_chdir(dlg);
		ttCFindFile ff("*.cpp");
		size_t cFilesFound = 0;
		for (size_t pos = 0; atxtSrcTypes[pos]; ++pos) {
			if (ff.NewPattern(atxtSrcTypes[pos])) {
				do {
				   ++cFilesFound;
				} while(ff.NextFile());
			}
		}
		_chdir(cszCWD);		// restore our directory
		ttCStr csz;
		csz.printf("%kn file%ks located", cFilesFound, cFilesFound);
		SetControlText(DLG_ID(IDTXT_FILES_FOUND), csz);
	}
}
