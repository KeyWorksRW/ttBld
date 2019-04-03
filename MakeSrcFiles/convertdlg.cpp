/////////////////////////////////////////////////////////////////////////////
// Name:		CConvertDlg
// Purpose:		IDDDLG_CONVERT dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

/*

	Source files specified in a build script are relative to the location of that build script. The .srcfiles file we are
	creating may be in an entirely different directory. So before we add a file to .srcfiles, we must first make it
	relative to the location of the build script, and then make it relative to the location of .srcfiles.

*/

#include "pch.h"

#include <direct.h>		// Functions for directory handling and creation

#include <ttfindfile.h>	// ttCFindFile
#include <ttfiledlg.h>	// ttCFileDlg
#include <ttdirdlg.h>	// ttCDirDlg
#include <ttenumstr.h>	// ttCEnumStr

#include "ttlibicons.h" 			// Icons for use on 3D shaded buttons (ttShadeBtn)
#include "../common/strtable.h" 	// String resource IDs
#include "dlgsrcoptions.h"			// CDlgSrcOptions

#include "convertdlg.h" 			// CConvertDlg

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

bool CConvertDlg::CreateSrcFiles()
{
	if (tt::FileExists(".srcfiles")) {
		if (tt::MsgBox(".srcfiles already exists in this directory. Click Yes to edit it, No to create a new one.", MB_YESNO) == IDYES)
			return true;
	}

	if (DoModal(NULL) == IDOK) {
		tt::MsgBoxFmt(IDS_CS_SRCFILES_CREATED, MB_OK, (char*) tt::findFilePortion(m_cszConvertScript));
		return true;
	}
	else
		return false;
}

void CConvertDlg::OnBegin(void)
{
	m_ShadedBtns.SetIcon(DLG_ID(IDOK), IDICON_TTLIB_OK);
	m_ShadedBtns.SetIcon(DLG_ID(IDCANCEL), IDICON_TTLIB_CANCEL);

	ttCStr csz;
	csz.getCWD();
	SetControlText(DLG_ID(IDEDIT_OUT_DIR), csz);
	SetControlText(DLG_ID(IDEDIT_IN_DIR), csz);

	m_comboScripts.Initialize(*this, DLG_ID(IDCOMBO_SCRIPTS));

	ttCFindFile ff("*.filters");
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

void CConvertDlg::OnBtnLocateScript()
{
	ttCFileDlg dlg(*this);
	dlg.SetFilter("Project Files|*.filters;*.vcproj;*.project;*.cbp");
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

void CConvertDlg::OnOK(void)
{
	m_cszDirOutput.getWindowText(GetDlgItem(DLG_ID(IDEDIT_OUT_DIR)));
	m_cszDirSrcFiles.getWindowText(GetDlgItem(DLG_ID(IDEDIT_IN_DIR)));
	if (GetCheck(DLG_ID(IDCHECK_SCRIPT)))
		m_cszConvertScript.getWindowText(GetDlgItem(DLG_ID(IDCOMBO_SCRIPTS)));
	else
		m_cszConvertScript.Delete();

	if (m_cszConvertScript.isNonEmpty()) {
		if (!tt::FileExists(m_cszConvertScript)) {
			tt::MsgBoxFmt(IDS_CS_CANNOT_OPEN, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
			CancelEnd();
			return;
		}

		const char* pszExt = tt::findLastChar(m_cszConvertScript, '.');
		if (pszExt) {
			HRESULT hr = m_xml.ParseXmlFile(m_cszConvertScript);
			if (hr != S_OK) {
				tt::MsgBoxFmt(IDS_CS_PARSE_ERROR, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
				CancelEnd();
			}
			m_cszDirOutput.AppendFileName(".srcfiles");

			bool bResult = false;
			if (tt::isSameStri(pszExt, ".project"))
				bResult = ConvertCodeLite();
			else if (tt::isSameStri(pszExt, ".cdb"))
				bResult = ConvertCodeBlocks();
			else if (tt::isSameStri(pszExt, ".filters"))
				bResult = ConvertVcxProj();
			else if (tt::isSameStri(pszExt, ".vcproj"))
				bResult = ConvertVcProj();

			if (bResult) {
				if (!m_cSrcFiles.WriteNew(m_cszDirOutput)) {
					tt::MsgBoxFmt(IDS_CS_CANT_WRITE, MB_OK | MB_ICONWARNING, (char*) m_cszDirOutput);
					CancelEnd();
				}
			}
			else {
				CancelEnd();
			}
			return;
		}
	}
	else {	// We get here if the user didn't specify anything to convert from.
		CreateNewSrcFiles();
		if (!m_cSrcFiles.WriteNew(m_cszDirOutput)) {
			tt::MsgBoxFmt(IDS_CS_CANT_WRITE, MB_OK | MB_ICONWARNING, (char*) m_cszDirOutput);
			CancelEnd();
		}
	}
}

bool CConvertDlg::ConvertCodeLite()
{
	ttCXMLBranch* pProject = m_xml.GetRootBranch()->FindFirstElement("CodeLite_Project");
	if (!pProject) {
		tt::MsgBoxFmt(IDS_CS_INVALID_PROJECT, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}

	const char* pszProject = pProject->GetAttribute("Name");
	if (pszProject && *pszProject)
		m_cSrcFiles.UpdateOption(OPT_PROJECT, pszProject);

	for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
		ttCXMLBranch* pItem = pProject->GetChildAt(item);
		if (tt::isSameStri(pItem->GetName(), "VirtualDirectory"))
			AddCodeLiteFiles(pItem);
		else if (tt::isSameStri(pItem->GetName(), "Settings")) {
			const char* pszType = pItem->GetAttribute("Type");
			if (tt::isSameStri(pszType, "Dynamic Library"))
				m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "dll");
			else if (tt::isSameStri(pszType, "Static Library"))
				m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "lib");
		}
	}
	return true;
}

bool CConvertDlg::ConvertCodeBlocks()
{
	if (!tt::FileExists(m_cszConvertScript)) {
		tt::MsgBoxFmt(IDS_CS_CANNOT_OPEN, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}

	ttCXMLBranch* pProject = m_xml.GetRootBranch()->FindFirstElement("Project");
	if (!pProject)	{
		tt::MsgBoxFmt(IDS_CS_MISSING_PROJECT, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}

	for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
		ttCXMLBranch* pItem = pProject->GetChildAt(item);
		if (tt::isSameStri(pItem->GetName(), "Option")) {
			if (pItem->GetAttribute("title"))
				m_cSrcFiles.UpdateOption(OPT_PROJECT, pItem->GetAttribute("title"));
		}
		else if (tt::isSameStri(pItem->GetName(), "Unit")) {
			if (isValidSrcFile(pItem->GetAttribute("filename")))
				m_cSrcFiles.m_lstSrcFiles += MakeSrcRelative(pItem->GetAttribute("filename"));
		}
	}

	return true;
}

bool CConvertDlg::ConvertVcxProj()
{
	ttCXMLBranch* pProject = m_xml.GetRootBranch()->FindFirstElement("Project");
	if (!pProject)	{
		tt::MsgBoxFmt(IDS_CS_MISSING_PROJECT, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}

	for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
		ttCXMLBranch* pItem = pProject->GetChildAt(item);
		if (tt::isSameStri(pItem->GetName(), "ItemGroup")) {
			for (size_t cmd = 0; cmd < pItem->GetChildrenCount(); cmd++) {
				ttCXMLBranch* pCmd = pItem->GetChildAt(cmd);
				if (tt::isSameStri(pCmd->GetName(), "ClCompile") || tt::isSameStri(pCmd->GetName(), "ResourceCompile")) {
					const char* pszFile = pCmd->GetAttribute("Include");
					if (pszFile && *pszFile)
						m_cSrcFiles.m_lstSrcFiles += pszFile;
				}
			}
		}
	}
	return true;
}

bool CConvertDlg::ConvertVcProj()
{
	ttCXMLBranch* pProject = m_xml.GetRootBranch()->FindFirstElement("VisualStudioProject");
	if (!pProject)	{
		tt::MsgBoxFmt(IDS_CS_MISSING_VSP, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}

	if (pProject->GetAttribute("Name"))
		m_cSrcFiles.UpdateOption(OPT_PROJECT, pProject->GetAttribute("Name"));

	ttCXMLBranch* pFiles = pProject->FindFirstElement("Files");
	if (!pFiles)	{
		tt::MsgBoxFmt(IDS_CS_MISSING_FILES, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}
	ttCXMLBranch* pFilter = pFiles->FindFirstElement("Filter");
	if (!pFilter) {
		tt::MsgBoxFmt(IDS_CS_MISSING_FILTER, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}
	for (size_t item = 0; item < pFilter->GetChildrenCount(); item++) {
		ttCXMLBranch* pItem = pFilter->GetChildAt(item);
		if (tt::isSameStri(pItem->GetName(), "File")) {
			if (isValidSrcFile(pItem->GetAttribute("RelativePath")))
				m_cSrcFiles.m_lstSrcFiles += pItem->GetAttribute("RelativePath");
		}
	}
	pFilter = pFiles->FindFirstAttribute("Name", "Resource Files");
	if (pFilter) {
		for (size_t item = 0; item < pFilter->GetChildrenCount(); item++) {
			ttCXMLBranch* pItem = pFilter->GetChildAt(item);
			if (tt::isSameStri(pItem->GetName(), "File")) {
				if (isValidSrcFile(pItem->GetAttribute("RelativePath")))
					m_cSrcFiles.m_lstSrcFiles += pItem->GetAttribute("RelativePath");
			}
		}
	}

	ttCXMLBranch* pConfigurations = pProject->FindFirstElement("Configurations");
	ttCXMLBranch* pConfiguration = pConfigurations ? pConfigurations->FindFirstElement("Configuration") : nullptr;
	if (pConfiguration) {
		// REVIEW: [randalphwa - 12/12/2018] Unusual, but possible to have 64-bit projects in an old .vcproj file,
		// but we should look for it and set the output directory to match 32 and 64 bit builds.

		ttCXMLBranch* pOption = pConfiguration->FindFirstAttribute("OutputFile");
		if (pOption) {
			ttCStr cszOutDir(pOption->GetAttribute("OutputFile"));	// will typically be something like: "../bin/$(ProjectName).exe"
			char* pszFile = tt::findFilePortion(cszOutDir);
			if (pszFile)
				*pszFile = 0;
			m_cSrcFiles.UpdateOption(OPT_TARGET_DIR32, (char*) cszOutDir);
		}
		do {
			if (tt::findStri(pConfiguration->GetAttribute("Name"), "Release"))
				break;
			pConfiguration = pConfigurations->FindNextElement("Configuration");
		} while(pConfiguration);

		ttCXMLBranch* pRelease = pConfiguration ? pConfiguration->FindFirstAttribute("Name") : nullptr;

		if (pRelease) {
			const char* pszOption = pRelease->GetAttribute("FavorSizeOrSpeed");
			if (pszOption && tt::isSameSubStri(pszOption, "1"))
				m_cSrcFiles.UpdateOption(OPT_OPTIMIZE, "speed");

			pszOption = pRelease->GetAttribute("WarningLevel");
			if (pszOption && !tt::isSameSubStri(pszOption, "4"))
				m_cSrcFiles.UpdateOption(OPT_WARN_LEVEL, pszOption);

			pszOption = pRelease->GetAttribute("AdditionalIncludeDirectories");
			if (pszOption) {
				ttCStr csz(m_cSrcFiles.GetOption(OPT_INC_DIRS));
				csz += pszOption;
				m_cSrcFiles.UpdateOption(OPT_INC_DIRS, (char*) csz);
			}

			pszOption = pRelease->GetAttribute("PreprocessorDefinitions");
			if (pszOption) {
				ttCEnumStr enumFlags(pszOption);
				ttCStr cszCFlags;
				while (enumFlags.Enum()) {
					if (tt::isSameStri(enumFlags, "NDEBUG"))
						continue;	// we already added this
					if (tt::isSameStri(enumFlags, "_CONSOLE")) {	// the define is already in use, but make certain exeType matches
						m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "console");
						continue;
					}
					if (tt::isSameStri(enumFlags, "_USRDLL")) {	// the define is already in use, but make certain exeType matches
						m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "dll");
						continue;	// do we need to add this?
					}
					if (tt::isSameSubStri(enumFlags, "$("))	// Visual Studio specific, ignore it
						continue;


					if (cszCFlags.isNonEmpty())
						cszCFlags += " ";
					cszCFlags += "-D";
					cszCFlags += enumFlags;
				}
				m_cSrcFiles.UpdateOption(OPT_CFLAGS, (char*) cszCFlags);
			}
		}
	}
	return true;
}

void CConvertDlg::AddCodeLiteFiles(ttCXMLBranch* pParent)
{
	for (size_t child = 0; child < pParent->GetChildrenCount(); child++) {
		ttCXMLBranch* pFile = pParent->GetChildAt(child);
		if (tt::isSameStri(pFile->GetName(), "File")) {
			if (isValidSrcFile(pFile->GetAttribute("Name")))
				m_cSrcFiles.m_lstSrcFiles += pFile->GetAttribute("Name");
		}
		// CodeLite nests resources in a sub <VirtualDirectory> tag
		else if (tt::isSameStri(pFile->GetName(), "VirtualDirectory")) {
			AddCodeLiteFiles(pFile);
		}
	}
}

bool CConvertDlg::isValidSrcFile(const char* pszFile) const
{
	if (!pszFile)
		return false;

	char* psz = tt::findLastChar(pszFile, '.');
	if (psz) {
		if (	tt::isSameStri(psz, ".cpp") ||
				tt::isSameStri(psz, ".cc") ||
				tt::isSameStri(psz, ".cxx") ||
				tt::isSameStri(psz, ".c") ||
				tt::isSameSubStri(psz, ".rc"))
			return true;
	}
	return false;
}

void CConvertDlg::CreateNewSrcFiles()
{
	char szPath[MAX_PATH];
	tt::strCopy(szPath, m_cszDirSrcFiles);
	tt::AddTrailingSlash(szPath);
	size_t cbRoot = tt::strByteLen(szPath);

	ttCStr cszRelPath;

	for (size_t pos = 0; atxtSrcTypes[pos]; ++pos) {
		tt::strCopy(szPath + cbRoot, atxtSrcTypes[pos]);
		ttCFindFile ff(szPath);
		if (ff.isValid()) {
			do {
				tt::strCopy(szPath + cbRoot, sizeof(szPath) - cbRoot, ff);
				tt::ConvertToRelative(m_cszDirOutput, szPath, cszRelPath);
				m_cSrcFiles.m_lstSrcFiles += cszRelPath;
			} while(ff.NextFile());
		}
	}
}

// This function first converts the file relative to the location of the build script, and then relative to the location of .srcfiles

char* CConvertDlg::MakeSrcRelative(const char* pszFile)
{
	ttASSERT(m_cszConvertScript.isNonEmpty());

	if (m_cszScriptRoot.isEmpty()) {
		m_cszScriptRoot = (char*) m_cszConvertScript;
		char* pszFilePortion = tt::findFilePortion(m_cszScriptRoot);
		ttASSERT_MSG(pszFilePortion, "No filename in m_cszScriptRoot--things will go badly without it.");
		if (pszFilePortion)
			*pszFilePortion = 0;
	}

	if (m_cszOutRoot.isEmpty()) {
		m_cszOutRoot = (char*) m_cszDirOutput;
		char* pszFilePortion = m_cszOutRoot.findStri(".srcfiles");
		if (pszFilePortion)
			*pszFilePortion = 0;
	}

	ttCStr cszTmp;
	tt::ConvertToRelative(m_cszScriptRoot, pszFile, cszTmp);
	tt::ConvertToRelative(m_cszOutRoot, cszTmp, m_cszRelative);
	return m_cszRelative;
}
