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
#include "strtable.h"				// String resource IDs
#include "strtable.h"				// String resource IDs
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

static const char* atxtProjects[] = {
	"*.filters",
	"*.vcproj",
	"*.project",	// CodeLite
	"*.cbp",		// CodeBlocks

	nullptr
};


bool CConvertDlg::CreateSrcFiles()
{
	if (tt::FileExists(".srcfiles")) {
		if (tt::MsgBox(IDS_SRCFILES_EXISTS, MB_YESNO) != IDYES)
			return false;

		// If we converted this before, grab the name of the project we converted from

		ttCFile file;
		if (file.ReadFile(".srcfiles")) {
			if (file.ReadLine()) {
				if (tt::IsSameSubStrI(file, "# Converted from")) {
					char* psz = tt::FindNonSpace((char*) file + sizeof("# Converted from"));
					m_cszConvertScript = psz;
				}
			}
		}
	}

	if (DoModal(NULL) == IDOK) {
		tt::MsgBoxFmt(IDS_SRCFILES_CREATED, MB_OK, (char*) tt::FindFilePortion(m_cszConvertScript));
		return true;
	}
	else
		return false;
}

void CConvertDlg::OnBegin(void)
{
	CenterWindow(true);
	EnableShadeBtns();
	SetBtnIcon(DLG_ID(IDOK), IDICON_TTLIB_OK);
	SetBtnIcon(DLG_ID(IDCANCEL), IDICON_TTLIB_CANCEL);

	ttCStr csz;
	csz.GetCWD();
	SetControlText(DLG_ID(IDEDIT_OUT_DIR), csz);
	SetControlText(DLG_ID(IDEDIT_IN_DIR), csz);

	m_comboScripts.Initialize(*this, DLG_ID(IDCOMBO_SCRIPTS));
	ttCFindFile ff(atxtProjects[0]);

	// If we converted once before, then default to that script

	if (m_cszConvertScript.IsNonEmpty())
		m_comboScripts.Add(m_cszConvertScript);
	else {
		for (size_t pos = 1; !ff.IsValid() && atxtProjects[pos]; ++pos)
			ff.NewPattern(atxtProjects[pos]);
		if (ff.IsValid())
			m_comboScripts.Add((char*) ff);

		if (m_comboScripts.GetCount() < 1) {	// no scripts found, let's check sub directories
			ff.NewPattern("*.*");
			do {
				if (ff.IsDir()) {
					if (!tt::IsValidFileChar(ff, 0))	// this will skip over . and ..
						continue;
					ttCStr cszDir((char*) ff);
					cszDir.AppendFileName(atxtProjects[0]);
					ttCFindFile ffFilter(cszDir);
					for (size_t pos = 1; !ffFilter.IsValid() && atxtProjects[pos]; ++pos)	{
						cszDir = ff;
						cszDir.AppendFileName(atxtProjects[pos]);
						ffFilter.NewPattern(cszDir);
					}

					if (ffFilter.IsValid()) {
						cszDir = ff;
						cszDir.AppendFileName(ffFilter);
						m_comboScripts.Add(cszDir);
					}
				}
			} while(ff.NextFile());
		}
	}
	if (m_comboScripts.GetCount() > 0) {
		m_comboScripts.SetCurSel();
		SetCheck(DLG_ID(IDRADIO_CONVERT));
	}
	else {
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
		if (cFilesFound)
			SetCheck(DLG_ID(IDRADIO_FILES));
	}

}

void CConvertDlg::OnBtnLocateScript()
{
	ttCFileDlg dlg(*this);
	dlg.SetFilter("Project Files|*.filters;*.vcproj;*.project;*.cbp");
	dlg.UseCurrentDirectory();
	if (dlg.GetOpenFileName()) {
		auto item = m_comboScripts.Add(dlg.GetFileName());
		m_comboScripts.SetCurSel(item);
		UnCheck(DLG_ID(IDRADIO_FILES));
		SetCheck(DLG_ID(IDRADIO_CONVERT));
		// TODO: [randalphwa - 4/2/2019] Need to decide how to handle IDEDIT_IN_DIR since this may no longer be correct
	}
}

void CConvertDlg::OnBtnChangeOut()	// change the directory to write .srcfiles to
{
	ttCDirDlg dlg;
	ttCStr cszCWD;
	cszCWD.GetCWD();
	dlg.SetStartingDir(cszCWD);
	if (dlg.GetFolderName(*this)) {
		ttCStr cszSrcFiles(dlg);
		cszSrcFiles.AppendFileName(".srcfiles");
		if (tt::FileExists(cszSrcFiles)) {
			if (tt::MsgBox(IDS_SRCFILES_EXISTS, MB_YESNO) != IDYES)
				return;
		}
		SetControlText(DLG_ID(IDEDIT_OUT_DIR), dlg);
	}
}

void CConvertDlg::OnBtnChangeIn()
{
	ttCDirDlg dlg;
	ttCStr cszCWD;
	cszCWD.GetCWD();
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
		UnCheck(DLG_ID(IDRADIO_CONVERT));
		SetCheck(DLG_ID(IDRADIO_FILES));
	}
}

void CConvertDlg::OnOK(void)
{
	m_cszDirOutput.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_OUT_DIR)));
	m_cszDirSrcFiles.GetWindowText(GetDlgItem(DLG_ID(IDEDIT_IN_DIR)));
	if (GetCheck(DLG_ID(IDRADIO_CONVERT)))
		m_cszConvertScript.GetWindowText(GetDlgItem(DLG_ID(IDCOMBO_SCRIPTS)));
	else
		m_cszConvertScript.Delete();

	if (m_cszConvertScript.IsNonEmpty()) {
		if (!tt::FileExists(m_cszConvertScript)) {
			tt::MsgBoxFmt(IDS_CS_CANNOT_OPEN, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
			CancelEnd();
			return;
		}

		const char* pszExt = tt::FindLastChar(m_cszConvertScript, '.');
		if (pszExt) {
			HRESULT hr = m_xml.ParseXmlFile(m_cszConvertScript);
			if (hr != S_OK) {
				tt::MsgBoxFmt(IDS_PARSE_ERROR, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
				CancelEnd();
			}
			m_cszDirOutput.AppendFileName(".srcfiles");

			bool bResult = false;
			if (tt::IsSameStrI(pszExt, ".project"))
				bResult = ConvertCodeLite();
			else if (tt::IsSameStrI(pszExt, ".cdb"))
				bResult = ConvertCodeBlocks();
			else if (tt::IsSameStrI(pszExt, ".filters"))
				bResult = ConvertVcxProj();
			else if (tt::IsSameStrI(pszExt, ".vcproj"))
				bResult = ConvertVcProj();

			if (bResult) {
				ttCStr cszHdr;
				cszHdr.printf("# Converted from %s", (char*) m_cszConvertScript);

				if (tt::IsEmpty(m_cSrcFiles.GetOption(OPT_PROJECT))) {
					ttCStr cszProject(m_cszDirOutput);
					char* pszFilePortion = tt::FindFilePortion(cszProject);
					if (pszFilePortion)
						*pszFilePortion = 0;
					m_cSrcFiles.UpdateOption(OPT_PROJECT, tt::FindFilePortion(cszProject));
				}

				if (!m_cSrcFiles.WriteNew(m_cszDirOutput, cszHdr)) {
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
		tt::MsgBoxFmt(IDS_INVALID_PROJECT, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}

	const char* pszProject = pProject->GetAttribute("Name");
	if (pszProject && *pszProject)
		m_cSrcFiles.UpdateOption(OPT_PROJECT, pszProject);

	for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
		ttCXMLBranch* pItem = pProject->GetChildAt(item);
		if (tt::IsSameStrI(pItem->GetName(), "VirtualDirectory"))
			AddCodeLiteFiles(pItem);
		else if (tt::IsSameStrI(pItem->GetName(), "Settings")) {
			const char* pszType = pItem->GetAttribute("Type");
			if (tt::IsSameStrI(pszType, "Dynamic Library"))
				m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "dll");
			else if (tt::IsSameStrI(pszType, "Static Library"))
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
		tt::MsgBoxFmt(IDS_MISSING_PROJECT, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}

	for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
		ttCXMLBranch* pItem = pProject->GetChildAt(item);
		if (tt::IsSameStrI(pItem->GetName(), "Option")) {
			if (pItem->GetAttribute("title"))
				m_cSrcFiles.UpdateOption(OPT_PROJECT, pItem->GetAttribute("title"));
		}
		else if (tt::IsSameStrI(pItem->GetName(), "Unit")) {
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
		tt::MsgBoxFmt(IDS_MISSING_PROJECT, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}

	for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
		ttCXMLBranch* pItem = pProject->GetChildAt(item);
		if (tt::IsSameStrI(pItem->GetName(), "ItemGroup")) {
			for (size_t cmd = 0; cmd < pItem->GetChildrenCount(); cmd++) {
				ttCXMLBranch* pCmd = pItem->GetChildAt(cmd);
				if (tt::IsSameStrI(pCmd->GetName(), "ClCompile") || tt::IsSameStrI(pCmd->GetName(), "ResourceCompile")) {
					const char* pszFile = pCmd->GetAttribute("Include");
					if (pszFile && *pszFile)
						m_cSrcFiles.m_lstSrcFiles += MakeSrcRelative(pszFile);
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
		tt::MsgBoxFmt(IDS_MISSING_VSP, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}

	if (pProject->GetAttribute("Name"))
		m_cSrcFiles.UpdateOption(OPT_PROJECT, pProject->GetAttribute("Name"));

	ttCXMLBranch* pFiles = pProject->FindFirstElement("Files");
	if (!pFiles)	{
		tt::MsgBoxFmt(IDS_MISSING_FILES, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}
	ttCXMLBranch* pFilter = pFiles->FindFirstElement("Filter");
	if (!pFilter) {
		tt::MsgBoxFmt(IDS_MISSING_FILTER, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}
	for (size_t item = 0; item < pFilter->GetChildrenCount(); item++) {
		ttCXMLBranch* pItem = pFilter->GetChildAt(item);
		if (tt::IsSameStrI(pItem->GetName(), "File")) {
			if (isValidSrcFile(pItem->GetAttribute("RelativePath")))
				m_cSrcFiles.m_lstSrcFiles += MakeSrcRelative(pItem->GetAttribute("RelativePath"));
		}
	}
	pFilter = pFiles->FindFirstAttribute("Name", "Resource Files");
	if (pFilter) {
		for (size_t item = 0; item < pFilter->GetChildrenCount(); item++) {
			ttCXMLBranch* pItem = pFilter->GetChildAt(item);
			if (tt::IsSameStrI(pItem->GetName(), "File")) {
				if (isValidSrcFile(pItem->GetAttribute("RelativePath")))
					m_cSrcFiles.m_lstSrcFiles += MakeSrcRelative(pItem->GetAttribute("RelativePath"));
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
			char* pszFile = tt::FindFilePortion(cszOutDir);
			if (pszFile)
				*pszFile = 0;
			m_cSrcFiles.UpdateOption(OPT_TARGET_DIR32, (char*) cszOutDir);
		}
		do {
			if (tt::FindStrI(pConfiguration->GetAttribute("Name"), "Release"))
				break;
			pConfiguration = pConfigurations->FindNextElement("Configuration");
		} while(pConfiguration);

		ttCXMLBranch* pRelease = pConfiguration ? pConfiguration->FindFirstAttribute("Name") : nullptr;

		if (pRelease) {
			const char* pszOption = pRelease->GetAttribute("FavorSizeOrSpeed");
			if (pszOption && tt::IsSameSubStrI(pszOption, "1"))
				m_cSrcFiles.UpdateOption(OPT_OPTIMIZE, "speed");

			pszOption = pRelease->GetAttribute("WarningLevel");
			if (pszOption && !tt::IsSameSubStrI(pszOption, "4"))
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
					if (tt::IsSameStrI(enumFlags, "NDEBUG"))
						continue;	// we already added this
					if (tt::IsSameStrI(enumFlags, "_CONSOLE")) {	// the define is already in use, but make certain exeType matches
						m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "console");
						continue;
					}
					if (tt::IsSameStrI(enumFlags, "_USRDLL")) {	// the define is already in use, but make certain exeType matches
						m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "dll");
						continue;	// do we need to add this?
					}
					if (tt::IsSameSubStrI(enumFlags, "$("))	// Visual Studio specific, ignore it
						continue;


					if (cszCFlags.IsNonEmpty())
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
		if (tt::IsSameStrI(pFile->GetName(), "File")) {
			if (isValidSrcFile(pFile->GetAttribute("Name")))
				m_cSrcFiles.m_lstSrcFiles += MakeSrcRelative(pFile->GetAttribute("Name"));
		}
		// CodeLite nests resources in a sub <VirtualDirectory> tag
		else if (tt::IsSameStrI(pFile->GetName(), "VirtualDirectory")) {
			AddCodeLiteFiles(pFile);
		}
	}
}

bool CConvertDlg::isValidSrcFile(const char* pszFile) const
{
	if (!pszFile)
		return false;

	char* psz = tt::FindLastChar(pszFile, '.');
	if (psz) {
		if (	tt::IsSameStrI(psz, ".cpp") ||
				tt::IsSameStrI(psz, ".cc") ||
				tt::IsSameStrI(psz, ".cxx") ||
				tt::IsSameStrI(psz, ".c") ||
				tt::IsSameSubStrI(psz, ".rc"))
			return true;
	}
	return false;
}

void CConvertDlg::CreateNewSrcFiles()
{
	char szPath[MAX_PATH];
	tt::StrCopy(szPath, m_cszDirSrcFiles);
	tt::AddTrailingSlash(szPath);
	size_t cbRoot = tt::StrByteLen(szPath);

	ttCStr cszRelPath;

	for (size_t pos = 0; atxtSrcTypes[pos]; ++pos) {
		tt::StrCopy(szPath + cbRoot, atxtSrcTypes[pos]);
		ttCFindFile ff(szPath);
		if (ff.IsValid()) {
			do {
				tt::StrCopy(szPath + cbRoot, sizeof(szPath) - cbRoot, ff);
				tt::ConvertToRelative(m_cszDirOutput, szPath, cszRelPath);
				m_cSrcFiles.m_lstSrcFiles += cszRelPath;
			} while(ff.NextFile());
		}
	}
}

// This function first converts the file relative to the location of the build script, and then relative to the location of .srcfiles

char* CConvertDlg::MakeSrcRelative(const char* pszFile)
{
	ttASSERT(m_cszConvertScript.IsNonEmpty());

	if (m_cszScriptRoot.IsEmpty()) {
		m_cszScriptRoot = (char*) m_cszConvertScript;
		char* pszFilePortion = tt::FindFilePortion(m_cszScriptRoot);
		ttASSERT_MSG(pszFilePortion, "No filename in m_cszScriptRoot--things will go badly without it.");
		if (pszFilePortion)
			*pszFilePortion = 0;
	}

	if (m_cszOutRoot.IsEmpty()) {
		m_cszOutRoot = (char*) m_cszDirOutput;
		char* pszFilePortion = m_cszOutRoot.FindStrI(".srcfiles");
		if (pszFilePortion)
			*pszFilePortion = 0;
	}

	ttCStr cszTmp;
	tt::ConvertToRelative(m_cszScriptRoot, pszFile, cszTmp);
	tt::ConvertToRelative(m_cszOutRoot, cszTmp, m_cszRelative);
	return m_cszRelative;
}
