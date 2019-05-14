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
	"*.vcxproj",
	"*.vcproj",
	"*.project",	// CodeLite
	"*.cbp",		// CodeBlocks

	nullptr
};

CConvertDlg::CConvertDlg() : ttCDlg(IDDDLG_CONVERT)
{
	m_cszDirOutput.GetCWD();
	m_cszCWD = m_cszDirOutput;
}

bool CConvertDlg::CreateSrcFiles()
{
	if (ttFileExists(".srcfiles")) {
		if (ttMsgBox(IDS_SRCFILES_EXISTS, MB_YESNO) != IDYES)
			return false;

		// If we converted this before, grab the name of the project we converted from

		ttCFile file;
		if (file.ReadFile(".srcfiles")) {
			if (file.ReadLine()) {
				if (ttIsSameSubStrI(file, "# Converted from")) {
					char* psz = ttFindNonSpace((char*) file + sizeof("# Converted from"));
					m_cszConvertScript = psz;
				}
			}
		}
	}

	if (DoModal(NULL) == IDOK) {
		if (m_cszConvertScript.IsNonEmpty())
			ttMsgBoxFmt(IDS_SRCFILES_CREATED, MB_OK, (char*) ttFindFilePortion(m_cszConvertScript));
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
					if (!ttIsValidFileChar(ff, 0))	// this will skip over . and ..
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
	dlg.SetFilter("Visual Studio|*.vcxproj;*.vcproj|CodeLite|*.project|CodeBlocks|*.cbp||");
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
		if (ttFileExists(cszSrcFiles)) {
			if (ttMsgBox(IDS_SRCFILES_EXISTS, MB_YESNO) != IDYES)
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
		if (!ttFileExists(m_cszConvertScript)) {
			ttMsgBoxFmt(IDS_CS_CANNOT_OPEN, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
			CancelEnd();
			return;
		}
		if (!doConversion()) {
			CancelEnd();
			return;
		}
	}
	else {	// We get here if the user didn't specify anything to convert from.
		CreateNewSrcFiles();
		ttCStr cszFile(m_cszDirOutput);
		cszFile.AppendFileName(".srcfiles");
		if (!m_cSrcFiles.WriteNew(cszFile)) {
			ttMsgBoxFmt(IDS_CS_CANT_WRITE, MB_OK | MB_ICONWARNING, (char*) cszFile);
			CancelEnd();
		}
	}
}

bool CConvertDlg::ConvertCodeLite()
{
	ttCXMLBranch* pProject = m_xml.GetRootBranch()->FindFirstElement("CodeLite_Project");
	if (!pProject) {
		ttMsgBoxFmt(IDS_INVALID_PROJECT, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}

	const char* pszProject = pProject->GetAttribute("Name");
	if (pszProject && *pszProject)
		m_cSrcFiles.UpdateOption(OPT_PROJECT, pszProject);

	for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
		ttCXMLBranch* pItem = pProject->GetChildAt(item);
		if (ttIsSameStrI(pItem->GetName(), "VirtualDirectory"))
			AddCodeLiteFiles(pItem);
		else if (ttIsSameStrI(pItem->GetName(), "Settings")) {
			const char* pszType = pItem->GetAttribute("Type");
			if (ttIsSameStrI(pszType, "Dynamic Library"))
				m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "dll");
			else if (ttIsSameStrI(pszType, "Static Library"))
				m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "lib");
		}
	}
	return true;
}

bool CConvertDlg::ConvertCodeBlocks()
{
	if (!ttFileExists(m_cszConvertScript)) {
		ttMsgBoxFmt(IDS_CS_CANNOT_OPEN, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}

	ttCXMLBranch* pProject = m_xml.GetRootBranch()->FindFirstElement("Project");
	if (!pProject)	{
		ttMsgBoxFmt(IDS_MISSING_PROJECT, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}

	for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
		ttCXMLBranch* pItem = pProject->GetChildAt(item);
		if (ttIsSameStrI(pItem->GetName(), "Option")) {
			if (pItem->GetAttribute("title"))
				m_cSrcFiles.UpdateOption(OPT_PROJECT, pItem->GetAttribute("title"));
		}
		else if (ttIsSameStrI(pItem->GetName(), "Unit")) {
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
		ttMsgBoxFmt(IDS_MISSING_PROJECT, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}

	bool bDebugFlagsSeen = false;
	bool bRelFlagsSeen = false;
	bool bTypeSeen = false;

	for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
		ttCXMLBranch* pItem = pProject->GetChildAt(item);
		if (ttIsSameStrI(pItem->GetName(), "ItemGroup")) {
			for (size_t cmd = 0; cmd < pItem->GetChildrenCount(); cmd++) {
				ttCXMLBranch* pCmd = pItem->GetChildAt(cmd);
				if (ttIsSameStrI(pCmd->GetName(), "ClCompile") || ttIsSameStrI(pCmd->GetName(), "ResourceCompile")) {
					const char* pszFile = pCmd->GetAttribute("Include");
					if (pszFile && *pszFile)
						m_cSrcFiles.m_lstSrcFiles += MakeSrcRelative(pszFile);
				}
			}
		}
		else if (ttIsSameStrI(pItem->GetName(), "PropertyGroup")) {
			if (bTypeSeen)
				continue;
			ttCXMLBranch* pFlags = pItem->FindFirstElement("ConfigurationType");
			if (pFlags && pFlags->GetChildrenCount() > 0) {
				ttCXMLBranch* pChild = pFlags->GetChildAt(0);
				if (pChild->GetData()) {
					bTypeSeen = true;
					if (ttIsSameStrI(pChild->GetData(), "DynamicLibrary"))
						m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "dll");
					else if (ttIsSameStrI(pChild->GetData(), "StaticLibrary"))
						m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "lib");
					// TODO: [randalphwa - 5/9/2019] What are the options for console and gui?
				}
			}
		}
		else if (ttIsSameStrI(pItem->GetName(), "ItemDefinitionGroup")) {
			const char* pszCondition = pItem->GetAttribute("Condition");
			if (!bDebugFlagsSeen && pszCondition && (ttStrStrI(pszCondition, "Debug|Win32") || ttStrStrI(pszCondition, "Debug|x64"))) {
				bDebugFlagsSeen = true;
				for (size_t cmd = 0; cmd < pItem->GetChildrenCount(); cmd++) {
					ttCXMLBranch* pCmd = pItem->GetChildAt(cmd);
					if (ttIsSameStrI(pCmd->GetName(), "Midl")) {
						ttCXMLBranch* pFlags = pCmd->FindFirstElement("PreprocessorDefinitions");
						if (pFlags && pFlags->GetChildrenCount() > 0) {
							ttCXMLBranch* pChild = pFlags->GetChildAt(0);
							if (pChild->GetData()) {
								ttCStr cszFlags("-D");
								cszFlags += pChild->GetData();
								cszFlags.ReplaceStr("_DEBUG;", "");
								cszFlags.ReplaceStr(";%(PreprocessorDefinitions)", "");
								while (cszFlags.ReplaceStr(";", " -D"));
								m_cSrcFiles.UpdateOption(OPT_MDL_DBG, (char*) cszFlags);
							}
						}
					}
					else if (ttIsSameStrI(pCmd->GetName(), "ResourceCompile")) {
						ttCXMLBranch* pFlags = pCmd->FindFirstElement("PreprocessorDefinitions");
						if (pFlags && pFlags->GetChildrenCount() > 0) {
							ttCXMLBranch* pChild = pFlags->GetChildAt(0);
							if (pChild->GetData()) {
								ttCStr cszFlags("-D");
								cszFlags += pChild->GetData();
								cszFlags.ReplaceStr("_DEBUG;", "");
								cszFlags.ReplaceStr(";%(PreprocessorDefinitions)", "");
								while (cszFlags.ReplaceStr(";", " -D"));
								m_cSrcFiles.UpdateOption(OPT_RC_DBG, (char*) cszFlags);
							}
						}
					}
					else if (ttIsSameStrI(pCmd->GetName(), "ClCompile")) {
						ttCXMLBranch* pFlags = pCmd->FindFirstElement("FavorSizeOrSpeed");
						if (pFlags && pFlags->GetChildrenCount() > 0) {
							ttCXMLBranch* pChild = pFlags->GetChildAt(0);
							if (pChild->GetData())
								m_cSrcFiles.UpdateOption(OPT_OPTIMIZE, ttIsSameSubStrI(pChild->GetData(), "size") ? "space" : "speed");
						}
						pFlags = pCmd->FindFirstElement("AdditionalIncludeDirectories");
						if (pFlags && pFlags->GetChildrenCount() > 0) {
							ttCXMLBranch* pChild = pFlags->GetChildAt(0);
							if (pChild->GetData()) {
								const char* pszFirstSemi = ttStrChr(pChild->GetData(), ';');
								if (pszFirstSemi)
									++pszFirstSemi;
								ttCStr cszFlags(ttIsSameSubStrI(pChild->GetData(), "$(OutDir") && pszFirstSemi ? pszFirstSemi :  pChild->GetData());
								cszFlags.ReplaceStr(";%(AdditionalIncludeDirectories)", "");
								m_cSrcFiles.UpdateOption(OPT_INC_DIRS, (char*) cszFlags);
							}
						}
						pFlags = pCmd->FindFirstElement("PrecompiledHeaderFile");
						if (pFlags && pFlags->GetChildrenCount() > 0) {
							ttCXMLBranch* pChild = pFlags->GetChildAt(0);
							if (pChild->GetData())
								m_cSrcFiles.UpdateOption(OPT_PCH, pChild->GetData());
						}
						pFlags = pCmd->FindFirstElement("WarningLevel");
						if (pFlags && pFlags->GetChildrenCount() > 0) {
							ttCXMLBranch* pChild = pFlags->GetChildAt(0);
							if (pChild->GetData()) {
								const char* pszTmp = pChild->GetData();
								while (*pszTmp && !ttIsDigit(*pszTmp))
									++pszTmp;
								m_cSrcFiles.UpdateOption(OPT_WARN_LEVEL, pszTmp);
							}
						}
						pFlags = pCmd->FindFirstElement("CallingConvention");
						if (pFlags && pFlags->GetChildrenCount() > 0) {
							ttCXMLBranch* pChild = pFlags->GetChildAt(0);
							if (pChild->GetData() && ttStrStrI(pChild->GetData(), "stdcall")) {
								m_cSrcFiles.UpdateOption(OPT_STDCALL, true);
							}
						}
						pFlags = pCmd->FindFirstElement("PreprocessorDefinitions");
						if (pFlags && pFlags->GetChildrenCount() > 0) {
							ttCXMLBranch* pChild = pFlags->GetChildAt(0);
							if (pChild->GetData()) {
								ttCStr cszFlags("-D");
								cszFlags += pChild->GetData();
								cszFlags.ReplaceStr("_DEBUG;", "");
								cszFlags.ReplaceStr(";%(PreprocessorDefinitions)", "");
								while (cszFlags.ReplaceStr(";", " -D"));
								m_cSrcFiles.UpdateOption(OPT_CFLAGS_DBG, (char*) cszFlags);
							}
						}
					}
				}
			}
			else if (!bRelFlagsSeen && pszCondition && (ttStrStrI(pszCondition, "Release|Win32") || ttStrStrI(pszCondition, "Release|x64"))) {
				bRelFlagsSeen = true;
				for (size_t cmd = 0; cmd < pItem->GetChildrenCount(); cmd++) {
					ttCXMLBranch* pCmd = pItem->GetChildAt(cmd);
					if (ttIsSameStrI(pCmd->GetName(), "Midl")) {
						ttCXMLBranch* pFlags = pCmd->FindFirstElement("PreprocessorDefinitions");
						if (pFlags && pFlags->GetChildrenCount() > 0) {
							ttCXMLBranch* pChild = pFlags->GetChildAt(0);
							if (pChild->GetData()) {
								ttCStr cszFlags("-D");
								cszFlags += pChild->GetData();
								cszFlags.ReplaceStr("NDEBUG;", "");
								cszFlags.ReplaceStr(";%(PreprocessorDefinitions)", "");
								while (cszFlags.ReplaceStr(";", " -D"));
								m_cSrcFiles.UpdateOption(OPT_MDL_REL, (char*) cszFlags);
							}
						}
					}
					else if (ttIsSameStrI(pCmd->GetName(), "ResourceCompile")) {
						ttCXMLBranch* pFlags = pCmd->FindFirstElement("PreprocessorDefinitions");
						if (pFlags && pFlags->GetChildrenCount() > 0) {
							ttCXMLBranch* pChild = pFlags->GetChildAt(0);
							if (pChild->GetData()) {
								ttCStr cszFlags("-D");
								cszFlags += pChild->GetData();
								cszFlags.ReplaceStr("NDEBUG;", "");
								cszFlags.ReplaceStr(";%(PreprocessorDefinitions)", "");
								while (cszFlags.ReplaceStr(";", " -D"));
								m_cSrcFiles.UpdateOption(OPT_RC_REL, (char*) cszFlags);
							}
						}
					}
					else if (ttIsSameStrI(pCmd->GetName(), "ClCompile")) {
						ttCXMLBranch* pFlags = pCmd->FindFirstElement("FavorSizeOrSpeed");
						if (pFlags && pFlags->GetChildrenCount() > 0) {
							ttCXMLBranch* pChild = pFlags->GetChildAt(0);
							if (pChild->GetData())
								m_cSrcFiles.UpdateOption(OPT_OPTIMIZE, ttIsSameSubStrI(pChild->GetData(), "size") ? "space" : "speed");
						}
						pFlags = pCmd->FindFirstElement("PrecompiledHeaderFile");
						if (pFlags && pFlags->GetChildrenCount() > 0) {
							ttCXMLBranch* pChild = pFlags->GetChildAt(0);
							if (pChild->GetData())
								m_cSrcFiles.UpdateOption(OPT_PCH, pChild->GetData());
						}
						pFlags = pCmd->FindFirstElement("WarningLevel");
						if (pFlags && pFlags->GetChildrenCount() > 0) {
							ttCXMLBranch* pChild = pFlags->GetChildAt(0);
							if (pChild->GetData()) {
								const char* pszTmp = pChild->GetData();
								while (*pszTmp && !ttIsDigit(*pszTmp))
									++pszTmp;
								m_cSrcFiles.UpdateOption(OPT_WARN_LEVEL, pszTmp);
							}
						}
						pFlags = pCmd->FindFirstElement("CallingConvention");
						if (pFlags && pFlags->GetChildrenCount() > 0) {
							ttCXMLBranch* pChild = pFlags->GetChildAt(0);
							if (pChild->GetData() && ttStrStrI(pChild->GetData(), "stdcall")) {
								m_cSrcFiles.UpdateOption(OPT_STDCALL, true);
							}
						}
						pFlags = pCmd->FindFirstElement("PreprocessorDefinitions");
						if (pFlags && pFlags->GetChildrenCount() > 0) {
							ttCXMLBranch* pChild = pFlags->GetChildAt(0);
							if (pChild->GetData()) {
								ttCStr cszFlags("-D");
								cszFlags += pChild->GetData();
								cszFlags.ReplaceStr("NDEBUG;", "");
								cszFlags.ReplaceStr(";%(PreprocessorDefinitions)", "");
								while (cszFlags.ReplaceStr(";", " -D"));
								m_cSrcFiles.UpdateOption(OPT_CFLAGS_REL, (char*) cszFlags);
							}
						}
					}
				}
			}
		}
	}

	// If Debug and Release flags are the same, then remove them and just use the common flag setting

	if (m_cSrcFiles.GetOption(OPT_CFLAGS_REL) && m_cSrcFiles.GetOption(OPT_CFLAGS_DBG) &&
				ttIsSameStrI(m_cSrcFiles.GetOption(OPT_CFLAGS_REL), m_cSrcFiles.GetOption(OPT_CFLAGS_DBG)))	{
		m_cSrcFiles.UpdateOption(OPT_CFLAGS_CMN, m_cSrcFiles.GetOption(OPT_CFLAGS_REL));
		m_cSrcFiles.UpdateOption(OPT_CFLAGS_REL, "");
		m_cSrcFiles.UpdateOption(OPT_CFLAGS_DBG, "");
	}
	if (m_cSrcFiles.GetOption(OPT_MDL_REL) && m_cSrcFiles.GetOption(OPT_MDL_DBG) &&
				ttIsSameStrI(m_cSrcFiles.GetOption(OPT_MDL_REL), m_cSrcFiles.GetOption(OPT_MDL_DBG)))	{
		m_cSrcFiles.UpdateOption(OPT_MDL_CMN, m_cSrcFiles.GetOption(OPT_MDL_REL));
		m_cSrcFiles.UpdateOption(OPT_MDL_REL, "");
		m_cSrcFiles.UpdateOption(OPT_MDL_DBG, "");
	}
	if (m_cSrcFiles.GetOption(OPT_RC_REL) && m_cSrcFiles.GetOption(OPT_RC_DBG) &&
				ttIsSameStrI(m_cSrcFiles.GetOption(OPT_RC_REL), m_cSrcFiles.GetOption(OPT_RC_DBG)))	{
		m_cSrcFiles.UpdateOption(OPT_RC_CMN, m_cSrcFiles.GetOption(OPT_RC_REL));
		m_cSrcFiles.UpdateOption(OPT_RC_REL, "");
		m_cSrcFiles.UpdateOption(OPT_RC_DBG, "");
	}

	return true;
}

bool CConvertDlg::ConvertVcProj()
{
	ttCXMLBranch* pProject = m_xml.GetRootBranch()->FindFirstElement("VisualStudioProject");
	if (!pProject)	{
		ttMsgBoxFmt(IDS_MISSING_VSP, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}

	if (pProject->GetAttribute("Name"))
		m_cSrcFiles.UpdateOption(OPT_PROJECT, pProject->GetAttribute("Name"));

	ttCXMLBranch* pFiles = pProject->FindFirstElement("Files");
	if (!pFiles)	{
		ttMsgBoxFmt(IDS_MISSING_FILES, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}
	ttCXMLBranch* pFilter = pFiles->FindFirstElement("Filter");
	if (!pFilter) {
		ttMsgBoxFmt(IDS_MISSING_FILTER, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
		return false;
	}
	for (size_t item = 0; item < pFilter->GetChildrenCount(); item++) {
		ttCXMLBranch* pItem = pFilter->GetChildAt(item);
		if (ttIsSameStrI(pItem->GetName(), "File")) {
			if (isValidSrcFile(pItem->GetAttribute("RelativePath")))
				m_cSrcFiles.m_lstSrcFiles += MakeSrcRelative(pItem->GetAttribute("RelativePath"));
		}
	}
	pFilter = pFiles->FindFirstAttribute("Name", "Resource Files");
	if (pFilter) {
		for (size_t item = 0; item < pFilter->GetChildrenCount(); item++) {
			ttCXMLBranch* pItem = pFilter->GetChildAt(item);
			if (ttIsSameStrI(pItem->GetName(), "File")) {
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
			char* pszFile = ttFindFilePortion(cszOutDir);
			if (pszFile)
				*pszFile = 0;
			m_cSrcFiles.UpdateOption(OPT_TARGET_DIR32, (char*) cszOutDir);
		}
		do {
			if (ttStrStrI(pConfiguration->GetAttribute("Name"), "Release"))
				break;
			pConfiguration = pConfigurations->FindNextElement("Configuration");
		} while(pConfiguration);

		ttCXMLBranch* pRelease = pConfiguration ? pConfiguration->FindFirstAttribute("Name") : nullptr;

		if (pRelease) {
			const char* pszOption = pRelease->GetAttribute("FavorSizeOrSpeed");
			if (pszOption && ttIsSameSubStrI(pszOption, "1"))
				m_cSrcFiles.UpdateOption(OPT_OPTIMIZE, "speed");

			pszOption = pRelease->GetAttribute("WarningLevel");
			if (pszOption && !ttIsSameSubStrI(pszOption, "4"))
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
					if (ttIsSameStrI(enumFlags, "NDEBUG"))
						continue;	// we already added this
					if (ttIsSameStrI(enumFlags, "_CONSOLE")) {	// the define is already in use, but make certain exeType matches
						m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "console");
						continue;
					}
					if (ttIsSameStrI(enumFlags, "_USRDLL")) {	// the define is already in use, but make certain exeType matches
						m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "dll");
						continue;	// do we need to add this?
					}
					if (ttIsSameSubStrI(enumFlags, "$("))	// Visual Studio specific, ignore it
						continue;


					if (cszCFlags.IsNonEmpty())
						cszCFlags += " ";
					cszCFlags += "-D";
					cszCFlags += enumFlags;
				}
				m_cSrcFiles.UpdateOption(OPT_CFLAGS_CMN, (char*) cszCFlags);
			}
		}
	}
	return true;
}

void CConvertDlg::AddCodeLiteFiles(ttCXMLBranch* pParent)
{
	for (size_t child = 0; child < pParent->GetChildrenCount(); child++) {
		ttCXMLBranch* pFile = pParent->GetChildAt(child);
		if (ttIsSameStrI(pFile->GetName(), "File")) {
			if (isValidSrcFile(pFile->GetAttribute("Name")))
				m_cSrcFiles.m_lstSrcFiles += MakeSrcRelative(pFile->GetAttribute("Name"));
		}
		// CodeLite nests resources in a sub <VirtualDirectory> tag
		else if (ttIsSameStrI(pFile->GetName(), "VirtualDirectory")) {
			AddCodeLiteFiles(pFile);
		}
	}
}

bool CConvertDlg::isValidSrcFile(const char* pszFile) const
{
	if (!pszFile)
		return false;

	char* psz = ttStrChrR(pszFile, '.');
	if (psz) {
		if (	ttIsSameStrI(psz, ".cpp") ||
				ttIsSameStrI(psz, ".cc") ||
				ttIsSameStrI(psz, ".cxx") ||
				ttIsSameStrI(psz, ".c") ||
				ttIsSameSubStrI(psz, ".rc"))
			return true;
	}
	return false;
}

void CConvertDlg::CreateNewSrcFiles()
{
	char szPath[MAX_PATH];
	ttStrCpy(szPath, m_cszDirSrcFiles);
	ttAddTrailingSlash(szPath);
	size_t cbRoot = ttStrLen(szPath);

	ttCStr cszRelPath;

	for (size_t pos = 0; atxtSrcTypes[pos]; ++pos) {
		ttStrCpy(szPath + cbRoot, atxtSrcTypes[pos]);
		ttCFindFile ff(szPath);
		if (ff.IsValid()) {
			do {
				ttStrCpy(szPath + cbRoot, sizeof(szPath) - cbRoot, ff);
				ttConvertToRelative(m_cszDirOutput, szPath, cszRelPath);
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
		m_cszScriptRoot.GetFullPathName();
		char* pszFilePortion = ttFindFilePortion(m_cszScriptRoot);
		ttASSERT_MSG(pszFilePortion, "No filename in m_cszScriptRoot--things will go badly without it.");
		if (pszFilePortion)
			*pszFilePortion = 0;

		// For GetFullPathName() to work properly on a file inside the script, we need to be in the same directory as the
		// script file

		_chdir(m_cszScriptRoot);
	}

	if (m_cszOutRoot.IsEmpty()) {
		m_cszOutRoot = (char*) m_cszDirOutput;
		m_cszOutRoot.GetFullPathName();
		char* pszFilePortion = m_cszOutRoot.FindStrI(".srcfiles");
		if (pszFilePortion)
			*pszFilePortion = 0;
	}

	ttCStr cszFile(pszFile);
	cszFile.GetFullPathName();

	ttConvertToRelative(m_cszOutRoot, cszFile, m_cszRelative);
	return m_cszRelative;
}

bool CConvertDlg::doConversion(const char* pszFile)
{
	if (pszFile)
		m_cszConvertScript = pszFile;
	const char* pszExt = ttStrChrR(m_cszConvertScript, '.');
	if (pszExt) {
		HRESULT hr = m_xml.ParseXmlFile(m_cszConvertScript);
		if (hr != S_OK) {
			ttMsgBoxFmt(IDS_PARSE_ERROR, MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
			return false;
		}
		m_cszDirOutput.AppendFileName(".srcfiles");

		bool bResult = false;
		if (ttIsSameStrI(pszExt, ".project"))
			bResult = ConvertCodeLite();
		else if (ttIsSameStrI(pszExt, ".cdb"))
			bResult = ConvertCodeBlocks();
		else if (ttIsSameStrI(pszExt, ".vcxproj"))
			bResult = ConvertVcxProj();
		else if (ttIsSameStrI(pszExt, ".vcproj"))
			bResult = ConvertVcProj();

		_chdir(m_cszCWD);	// we may have changed directories during the conversion

		if (bResult) {
			ttCStr cszHdr;
			cszHdr.printf("# Converted from %s", (char*) m_cszConvertScript);

			if (ttIsEmpty(m_cSrcFiles.GetOption(OPT_PROJECT))) {
				ttCStr cszProject(m_cszDirOutput);
				char* pszFilePortion = ttFindFilePortion(cszProject);
				if (pszFilePortion)
					*pszFilePortion = 0;
				m_cSrcFiles.UpdateOption(OPT_PROJECT, ttFindFilePortion(cszProject));
			}

			if (!m_cSrcFiles.WriteNew(m_cszDirOutput, cszHdr)) {
				ttMsgBoxFmt(IDS_CS_CANT_WRITE, MB_OK | MB_ICONWARNING, (char*) m_cszDirOutput);
				return false;
			}
			return true;
		}
	}
	return false;
}
