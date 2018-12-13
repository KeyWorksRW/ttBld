/////////////////////////////////////////////////////////////////////////////
// Name:		conver.cpp
// Purpose:		Converts various build scripts into a .srcfiles file
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2018 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#include "../ttLib/include/keyxml.h"	// CKeyXmlBranch
#include "../ttLib/include/findfile.h"	// CTTFindFile
#include "../ttLib/include/enumstr.h"	// CEnumStr

#include "../common/writesrcfiles.h"	// CWriteSrcFiles

bool ConvertCodeBlocks(const char* pszBldFile);
bool ConvertCodeLite(const char* pszBldFile);
bool ConvertVcProj(const char* pszBldFile);
bool ConvertVcxProj(const char* pszBldFile);
bool ConvertVcxProj(const char* pszBldFile);

static void AddCodeLiteFiles(CKeyXmlBranch* pParent, CWriteSrcFiles& cSrcFiles);
static bool isValidSrcFile(const char* pszFile);

bool ConvertBuildScript(const char* pszBldFile)
{
#ifndef _DEBUG
	if (FileExists(".srcfiles")) {
		puts(".srcfiles already exists!");
		return false;
	}
#endif

	if (!pszBldFile || !*pszBldFile) {
		CTTFindFile ff("*.vcxproj");
		if (ff.isValid())
			return ConvertVcxProj(ff);
		else if (ff.NewPattern("*.vcproj"))
			return ConvertVcProj(ff);
		else if (ff.NewPattern("*.project"))
			return ConvertCodeLite(ff);
		else if (ff.NewPattern("*.cbp"))
			return ConvertCodeBlocks(ff);

		puts("Could not locate a supported build script to convert.");
		return false;
	}

	const char* pszExt = kstrchrR(pszBldFile, '.');
	if (!pszExt) {
		CStr cszBuild(pszBldFile);
		cszBuild.ChangeExtension(".vcxproj");
		if (FileExists(cszBuild))
			return ConvertVcxProj(cszBuild);
		cszBuild.ChangeExtension(".vcproj");
		if (FileExists(cszBuild))
			return ConvertVcProj(cszBuild);
		cszBuild.ChangeExtension(".project");
		if (FileExists(cszBuild))
			return ConvertCodeLite(cszBuild);
		cszBuild.ChangeExtension(".cbp");
		if (FileExists(cszBuild))
			return ConvertCodeBlocks(cszBuild);

		puts("Could not locate a supported build script to convert.");
		return false;
	}

	if (IsSameString(pszExt, ".project"))
		return ConvertCodeLite(pszBldFile);
	else if (IsSameString(pszExt, ".cbp"))
		return ConvertCodeBlocks(pszBldFile);
	else if (IsSameString(pszExt, ".vcproj"))
		return ConvertVcProj(pszBldFile);
	else if (IsSameString(pszExt, ".vcxproj"))
		return ConvertVcxProj(pszBldFile);
	else {
		printf("%s is an unrecognized project type!", pszBldFile);
		return false;
	}
}

bool ConvertCodeLite(const char* pszBldFile)
{
	if (!FileExists(pszBldFile)) {
		printf("Cannot open %s!\n", pszBldFile);
		return false;
	}

	CWriteSrcFiles cSrcFiles;
	CKeyXML xml;

	HRESULT hr = xml.ParseXmlFile(pszBldFile);
	if (hr == S_OK)	{
		CKeyXmlBranch* pProject = xml.GetRootBranch()->FindFirstElement("CodeLite_Project");
		if (!pProject)	{
			CStr cszMsg;
			puts(cszMsg.printf("Cannot locate <CodeLite_Project> in %s", pszBldFile));
			return false;
		}
		const char* pszProject = pProject->GetAttribute("Name");
		if (pszProject && *pszProject)
			cSrcFiles.m_cszProjectName = pszProject;

		for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
			CKeyXmlBranch* pItem = pProject->GetChildAt(item);
			if (IsSameString(pItem->GetName(), "VirtualDirectory"))
				AddCodeLiteFiles(pItem, cSrcFiles);
			else if (IsSameString(pItem->GetName(), "Settings")) {
				const char* pszType = pItem->GetAttribute("Type");
				if (IsSameString(pszType, "Dynamic Library"))
					cSrcFiles.m_exeType = CSrcFiles::EXE_DLL;
				else if (IsSameString(pszType, "Static Library"))
					cSrcFiles.m_exeType = CSrcFiles::EXE_LIB;
			}
		}
		if (cSrcFiles.WriteNew())
			printf(".srcfiles created using %s as the original\n", pszBldFile);
		return true;
	}
	else
		return false;
}

bool ConvertCodeBlocks(const char* pszBldFile)
{
	if (!FileExists(pszBldFile)) {
		printf("Cannot open %s!\n", pszBldFile);
		return false;
	}

	CWriteSrcFiles cSrcFiles;
	CKeyXML xml;

	HRESULT hr = xml.ParseXmlFile(pszBldFile);
	if (hr == S_OK)	{
		CKeyXmlBranch* pProject = xml.GetRootBranch()->FindFirstElement("Project");
		if (!pProject)	{
			CStr cszMsg;
			puts(cszMsg.printf("Cannot locate <Project> in %s", pszBldFile));
			return false;
		}
		for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
			CKeyXmlBranch* pItem = pProject->GetChildAt(item);
			if (IsSameString(pItem->GetName(), "Option")) {
				if (pItem->GetAttribute("title"))
					cSrcFiles.m_cszProjectName = pItem->GetAttribute("title");
			}
			else if (IsSameString(pItem->GetName(), "Unit")) {
				if (isValidSrcFile(pItem->GetAttribute("filename")))
					cSrcFiles.m_lstSrcFiles += pItem->GetAttribute("filename");
			}
		}

		if (cSrcFiles.WriteNew())
			printf(".srcfiles created using %s as the original\n", pszBldFile);

		return true;
	}
	else
		return false;

	return true;
}

bool ConvertVcxProj(const char* pszBldFile)
{
	if (!FileExists(pszBldFile)) {
		printf("Cannot open %s!\n", pszBldFile);
		return false;
	}

	CStr cszFilters(pszBldFile);
	if (!IsSameString(cszFilters.FindExt(), ".filters"))
		cszFilters += ".filters";

	CWriteSrcFiles cSrcFiles;
	CKeyXML xml;

	HRESULT hr = xml.ParseXmlFile(cszFilters);
	if (hr == S_OK)	{
		CKeyXmlBranch* pProject = xml.GetRootBranch()->FindFirstElement("Project");
		if (!pProject)	{
			CStr cszMsg;
			puts(cszMsg.printf("Cannot locate <Project> in %s", (char*) cszFilters));
			return false;
		}
		for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
			CKeyXmlBranch* pItem = pProject->GetChildAt(item);
			if (IsSameString(pItem->GetName(), "ItemGroup")) {
				for (size_t cmd = 0; cmd < pItem->GetChildrenCount(); cmd++) {
					CKeyXmlBranch* pCmd = pItem->GetChildAt(cmd);
					if (IsSameString(pCmd->GetName(), "ClCompile") || IsSameString(pCmd->GetName(), "ResourceCompile")) {
						const char* pszFile = pCmd->GetAttribute("Include");
						if (pszFile && *pszFile)
							cSrcFiles.m_lstSrcFiles += pszFile;
					}
				}
			}
		}
		if (cSrcFiles.WriteNew())
			printf(".srcfiles created using %s as the original\n", pszBldFile);
		return true;
	}
	else
		return false;
}

bool ConvertVcProj(const char* pszBldFile)
{
	if (!FileExists(pszBldFile)) {
		printf("Cannot open %s!\n", pszBldFile);
		return false;
	}

	CWriteSrcFiles cSrcFiles;
	CKeyXML xml;

	HRESULT hr = xml.ParseXmlFile(pszBldFile);
	if (hr == S_OK)	{
		CKeyXmlBranch* pProject = xml.GetRootBranch()->FindFirstElement("VisualStudioProject");
		if (!pProject)	{
			CStr cszMsg;
			puts(cszMsg.printf("Cannot locate <VisualStudioProject> in %s", pszBldFile));
			return false;
		}

		if (pProject->GetAttribute("Name"))
			cSrcFiles.m_cszProjectName = pProject->GetAttribute("Name");

		CKeyXmlBranch* pFiles = pProject->FindFirstElement("Files");
		if (!pFiles)	{
			CStr cszMsg;
			puts(cszMsg.printf("Cannot locate <pFiles> in %s", pszBldFile));
			return false;
		}
		CKeyXmlBranch* pFilter = pFiles->FindFirstElement("Filter");
		if (!pFilter) {
			CStr cszMsg;
			puts(cszMsg.printf("Cannot locate <Filter> in %s", pszBldFile));
			return false;
		}
		for (size_t item = 0; item < pFilter->GetChildrenCount(); item++) {
			CKeyXmlBranch* pItem = pFilter->GetChildAt(item);
			if (IsSameString(pItem->GetName(), "File")) {
				if (isValidSrcFile(pItem->GetAttribute("RelativePath")))
					cSrcFiles.m_lstSrcFiles += pItem->GetAttribute("RelativePath");
			}
		}
		pFilter = pFiles->FindFirstAttribute("Name", "Resource Files");
		if (pFilter) {
			for (size_t item = 0; item < pFilter->GetChildrenCount(); item++) {
				CKeyXmlBranch* pItem = pFilter->GetChildAt(item);
				if (IsSameString(pItem->GetName(), "File")) {
					if (isValidSrcFile(pItem->GetAttribute("RelativePath")))
						cSrcFiles.m_lstSrcFiles += pItem->GetAttribute("RelativePath");
				}
			}
		}

		CKeyXmlBranch* pConfigurations = pProject->FindFirstElement("Configurations");
		CKeyXmlBranch* pConfiguration = pConfigurations ? pConfigurations->FindFirstElement("Configuration") : nullptr;
		if (pConfiguration) {
			// REVIEW: [randalphwa - 12/12/2018] Unusual, but possible to have 64-bit projects in an old .vcproj file,
			// but we should look for it and set the output directory to match 32 and 64 bit builds.

			CKeyXmlBranch* pOption = pConfiguration->FindFirstAttribute("OutputFile");
			if (pOption) {
				CStr cszOutDir(pOption->GetAttribute("OutputFile"));	// will typically be something like: "../bin/$(ProjectName).exe"
				char* pszFile = FindFilePortion(cszOutDir);
				if (pszFile)
					*pszFile = 0;
				cSrcFiles.m_cszTarget32 = (const char*) cszOutDir;
			}
			do {
				if (kstristr(pConfiguration->GetAttribute("Name"), "Release"))
					break;
				pConfiguration = pConfigurations->FindNextElement("Configuration");
			} while(pConfiguration);

			CKeyXmlBranch* pRelease = pConfiguration ? pConfiguration->FindFirstAttribute("Name") : nullptr;

			if (pRelease) {
				const char* pszOption = pRelease->GetAttribute("FavorSizeOrSpeed");
				if (pszOption && isSameSubString(pszOption, "1"))
					cSrcFiles.m_bBuildForSpeed = true;

				pszOption = pRelease->GetAttribute("WarningLevel");
				if (pszOption && !isSameSubString(pszOption, "4"))
					cSrcFiles.m_WarningLevel = Atoi(pszOption);

				pszOption = pRelease->GetAttribute("AdditionalIncludeDirectories");
				if (pszOption)
					cSrcFiles.m_cszIncDirs += pszOption;

				pszOption = pRelease->GetAttribute("PreprocessorDefinitions");
				if (pszOption) {
					CEnumStr enumFlags(pszOption);
					while (enumFlags.Enum()) {
						if (isSameString(enumFlags, "NDEBUG"))
							continue;	// we already add this
						if (isSameString(enumFlags, "_CONSOLE")) {	// the define is already in use, but make certain exeType matches
							cSrcFiles.m_exeType = CSrcFiles::EXE_CONSOLE;
							continue;	// we already add this
						}
						if (isSameString(enumFlags, "_USRDLL")) {	// the define is already in use, but make certain exeType matches
							cSrcFiles.m_exeType = CSrcFiles::EXE_DLL;
							continue;	// do we need to add this?
						}
						if (isSameSubString(enumFlags, "$("))	// Visual Studio specific, ignore it
							continue;
						if (cSrcFiles.m_cszCFlags.IsNonEmpty())
							cSrcFiles.m_cszCFlags += " ";
						cSrcFiles.m_cszCFlags += "-D";
						cSrcFiles.m_cszCFlags += enumFlags;
					}
				}
			}
		}

		if (cSrcFiles.WriteNew())
			printf(".srcfiles created using %s as the original\n", pszBldFile);

		return true;
	}
	else
		return false;
}

static void AddCodeLiteFiles(CKeyXmlBranch* pParent, CWriteSrcFiles& cSrcFiles)
{
	for (size_t child = 0; child < pParent->GetChildrenCount(); child++) {
		CKeyXmlBranch* pFile = pParent->GetChildAt(child);
		if (IsSameString(pFile->GetName(), "File")) {
			if (isValidSrcFile(pFile->GetAttribute("Name")))
				cSrcFiles.m_lstSrcFiles += pFile->GetAttribute("Name");
		}
		// CodeLite nests resources in a sub <VirtualDirectory> tag
		else if (IsSameString(pFile->GetName(), "VirtualDirectory")) {
			AddCodeLiteFiles(pFile, cSrcFiles);
		}
	}
}

static bool isValidSrcFile(const char* pszFile)
{
	if (!pszFile)
		return false;

	char* psz = kstrchrR(pszFile, '.');
	if (psz) {
		if (	IsSameString(psz, ".cpp") ||
				IsSameString(psz, ".cc") ||
				IsSameString(psz, ".cxx") ||
				IsSameString(psz, ".c") ||
				IsSameString(psz, ".rc"))
			return true;
	}
	return false;
}
