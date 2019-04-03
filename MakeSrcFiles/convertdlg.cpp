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
#include <ttxml.h>		// ttCXMLBranch, ttCParseXML

#include "convertdlg.h"
#include "ttlibicons.h"
#include "../common/strtable.h" 	// String resource IDs

#ifdef _MSC_VER
	#pragma warning(disable: 6031)	// Return value ignored: '_chdir'.
#endif // _MSC_VER

static const char* atxtSrcTypes[] = {
	"*.cpp",
	"*.cc",
	"*.cxx",
	"*.c",

	nullptr
};

static void AddCodeLiteFiles(ttCXMLBranch* pParent, CWriteSrcFiles& cSrcFiles);
static bool isValidSrcFile(const char* pszFile);

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

	if (m_cszConvertScript.isNonEmpty()) {
		const char* pszExt = tt::findLastChar(m_cszConvertScript, '.');
		if (pszExt) {
			bool bResult = false;
			if (tt::isSameStri(pszExt, ".project"))
				bResult = ConvertCodeLite();
			// TODO: [randalphwa - 4/2/2019] add other project conversions here
			if (!bResult)
				CancelEnd();
			return;
		}
	}
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

bool CConvertDlg::ConvertCodeLite()
{
	if (!tt::FileExists(m_cszConvertScript)) {
		tt::MsgBoxFmt(IDS_CS_CANNOT_OPEN, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}

	CWriteSrcFiles cSrcFiles;
	ttCParseXML xml;

	HRESULT hr = xml.ParseXmlFile(m_cszConvertScript);
	if (hr == S_OK)	{
		ttCXMLBranch* pProject = xml.GetRootBranch()->FindFirstElement("CodeLite_Project");
		if (!pProject)	{
			tt::MsgBoxFmt(IDS_CS_INVALID_PROJECT, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
			return false;
		}
		const char* pszProject = pProject->GetAttribute("Name");
		if (pszProject && *pszProject)
			cSrcFiles.UpdateOption(OPT_PROJECT, pszProject);

		for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
			ttCXMLBranch* pItem = pProject->GetChildAt(item);
			if (tt::isSameStri(pItem->GetName(), "VirtualDirectory"))
				AddCodeLiteFiles(pItem, cSrcFiles);
			else if (tt::isSameStri(pItem->GetName(), "Settings")) {
				const char* pszType = pItem->GetAttribute("Type");
				if (tt::isSameStri(pszType, "Dynamic Library"))
					cSrcFiles.UpdateOption(OPT_EXE_TYPE, "dll");
				else if (tt::isSameStri(pszType, "Static Library"))
					cSrcFiles.UpdateOption(OPT_EXE_TYPE, "lib");
			}
		}
		m_cszDirOutput.AppendFileName(".srcfiles");
		if (cSrcFiles.WriteNew(m_cszDirOutput))	{
			tt::MsgBoxFmt(IDS_CS_SRCFILES_CREATED, MB_OK, (char*) tt::findFilePortion(m_cszConvertScript));
			return true;
		}
		else {
			tt::MsgBoxFmt(IDS_CS_CANT_WRITE, MB_OK | MB_ICONWARNING, (char*) m_cszDirOutput);
			return false;
		}
	}
	else {
		tt::MsgBoxFmt(IDS_CS_PARSE_ERROR, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}
}

static void AddCodeLiteFiles(ttCXMLBranch* pParent, CWriteSrcFiles& cSrcFiles)
{
	for (size_t child = 0; child < pParent->GetChildrenCount(); child++) {
		ttCXMLBranch* pFile = pParent->GetChildAt(child);
		if (tt::isSameStri(pFile->GetName(), "File")) {
			if (isValidSrcFile(pFile->GetAttribute("Name")))
				cSrcFiles.m_lstSrcFiles += pFile->GetAttribute("Name");
		}
		// CodeLite nests resources in a sub <VirtualDirectory> tag
		else if (tt::isSameStri(pFile->GetName(), "VirtualDirectory")) {
			AddCodeLiteFiles(pFile, cSrcFiles);
		}
	}
}

static bool isValidSrcFile(const char* pszFile)
{
	if (!pszFile)
		return false;

	char* psz = tt::findLastChar(pszFile, '.');
	if (psz) {
		if (	tt::isSameStri(psz, ".cpp") ||
				tt::isSameStri(psz, ".cc") ||
				tt::isSameStri(psz, ".cxx") ||
				tt::isSameStri(psz, ".c") ||
				tt::isSameStri(psz, ".rc"))
			return true;
	}
	return false;
}
