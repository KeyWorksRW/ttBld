/////////////////////////////////////////////////////////////////////////////
// Name:		conver.cpp
// Purpose:		Converts various build scripts into a .srcfiles file
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttxml.h>			// ttCXMLBranch, ttCParseXML
#include <ttfindfile.h> 	// ttCFindFile
#include <ttenumstr.h>		// ttCEnumStr

#include "../common/writesrcfiles.h"	// CWriteSrcFiles

bool ConvertCodeBlocks(const char* pszBldFile);
bool ConvertCodeLite(const char* pszBldFile);
bool ConvertVcProj(const char* pszBldFile);
bool ConvertVcxProj(const char* pszBldFile);
bool ConvertVcxProj(const char* pszBldFile);

static void AddCodeLiteFiles(ttCXMLBranch* pParent, CWriteSrcFiles& cSrcFiles);
static bool isValidSrcFile(const char* pszFile);

bool ConvertBuildScript(const char* pszBldFile)
{
#ifndef _DEBUG
	if (tt::FileExists(".srcfiles")) {
		puts(".srcfiles already exists!");
		return false;
	}
#endif

	if (!pszBldFile || !*pszBldFile) {
		ttCFindFile ff("*.vcxproj");
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

	const char* pszExt = tt::findLastChar(pszBldFile, '.');
	if (!pszExt) {
		ttCStr cszBuild(pszBldFile);
		cszBuild.ChangeExtension(".vcxproj");
		if (tt::FileExists(cszBuild))
			return ConvertVcxProj(cszBuild);
		cszBuild.ChangeExtension(".vcproj");
		if (tt::FileExists(cszBuild))
			return ConvertVcProj(cszBuild);
		cszBuild.ChangeExtension(".project");
		if (tt::FileExists(cszBuild))
			return ConvertCodeLite(cszBuild);
		cszBuild.ChangeExtension(".cbp");
		if (tt::FileExists(cszBuild))
			return ConvertCodeBlocks(cszBuild);

		puts("Could not locate a supported build script to convert.");
		return false;
	}

	if (tt::isSameStri(pszExt, ".project"))
		return ConvertCodeLite(pszBldFile);
	else if (tt::isSameStri(pszExt, ".cbp"))
		return ConvertCodeBlocks(pszBldFile);
	else if (tt::isSameStri(pszExt, ".vcproj"))
		return ConvertVcProj(pszBldFile);
	else if (tt::isSameStri(pszExt, ".vcxproj"))
		return ConvertVcxProj(pszBldFile);
	else {
		printf("%s is an unrecognized project type!", pszBldFile);
		return false;
	}
}

bool ConvertCodeLite(const char* pszBldFile)
{
	if (!tt::FileExists(pszBldFile)) {
		printf("Cannot open %s!\n", pszBldFile);
		return false;
	}

	CWriteSrcFiles cSrcFiles;
	ttCParseXML xml;

	HRESULT hr = xml.ParseXmlFile(pszBldFile);
	if (hr == S_OK)	{
		ttCXMLBranch* pProject = xml.GetRootBranch()->FindFirstElement("CodeLite_Project");
		if (!pProject)	{
			ttCStr cszMsg;
			puts(cszMsg.printf("Cannot locate <CodeLite_Project> in %s", pszBldFile));
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
					cSrcFiles.m_exeType = CSrcFiles::EXE_DLL;
				else if (tt::isSameStri(pszType, "Static Library"))
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
	if (!tt::FileExists(pszBldFile)) {
		printf("Cannot open %s!\n", pszBldFile);
		return false;
	}

	CWriteSrcFiles cSrcFiles;
	ttCParseXML xml;

	HRESULT hr = xml.ParseXmlFile(pszBldFile);
	if (hr == S_OK)	{
		ttCXMLBranch* pProject = xml.GetRootBranch()->FindFirstElement("Project");
		if (!pProject)	{
			ttCStr cszMsg;
			puts(cszMsg.printf("Cannot locate <Project> in %s", pszBldFile));
			return false;
		}
		for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
			ttCXMLBranch* pItem = pProject->GetChildAt(item);
			if (tt::isSameStri(pItem->GetName(), "Option")) {
				if (pItem->GetAttribute("title"))
					cSrcFiles.UpdateOption(OPT_PROJECT, pItem->GetAttribute("title"));
			}
			else if (tt::isSameStri(pItem->GetName(), "Unit")) {
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
	if (!tt::FileExists(pszBldFile)) {
		printf("Cannot open %s!\n", pszBldFile);
		return false;
	}

	ttCStr cszFilters(pszBldFile);
	if (!tt::isSameStri(cszFilters.findExt(), ".filters"))
		cszFilters += ".filters";

	CWriteSrcFiles cSrcFiles;
	ttCParseXML xml;

	HRESULT hr = xml.ParseXmlFile(cszFilters);
	if (hr == S_OK)	{
		ttCXMLBranch* pProject = xml.GetRootBranch()->FindFirstElement("Project");
		if (!pProject)	{
			ttCStr cszMsg;
			puts(cszMsg.printf("Cannot locate <Project> in %s", (char*) cszFilters));
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
	if (!tt::FileExists(pszBldFile)) {
		printf("Cannot open %s!\n", pszBldFile);
		return false;
	}

	CWriteSrcFiles cSrcFiles;
	ttCParseXML xml;

	HRESULT hr = xml.ParseXmlFile(pszBldFile);
	if (hr == S_OK)	{
		ttCXMLBranch* pProject = xml.GetRootBranch()->FindFirstElement("VisualStudioProject");
		if (!pProject)	{
			ttCStr cszMsg;
			puts(cszMsg.printf("Cannot locate <VisualStudioProject> in %s", pszBldFile));
			return false;
		}

		if (pProject->GetAttribute("Name"))
			cSrcFiles.UpdateOption(OPT_PROJECT, pProject->GetAttribute("Name"));

		ttCXMLBranch* pFiles = pProject->FindFirstElement("Files");
		if (!pFiles)	{
			ttCStr cszMsg;
			puts(cszMsg.printf("Cannot locate <pFiles> in %s", pszBldFile));
			return false;
		}
		ttCXMLBranch* pFilter = pFiles->FindFirstElement("Filter");
		if (!pFilter) {
			ttCStr cszMsg;
			puts(cszMsg.printf("Cannot locate <Filter> in %s", pszBldFile));
			return false;
		}
		for (size_t item = 0; item < pFilter->GetChildrenCount(); item++) {
			ttCXMLBranch* pItem = pFilter->GetChildAt(item);
			if (tt::isSameStri(pItem->GetName(), "File")) {
				if (isValidSrcFile(pItem->GetAttribute("RelativePath")))
					cSrcFiles.m_lstSrcFiles += pItem->GetAttribute("RelativePath");
			}
		}
		pFilter = pFiles->FindFirstAttribute("Name", "Resource Files");
		if (pFilter) {
			for (size_t item = 0; item < pFilter->GetChildrenCount(); item++) {
				ttCXMLBranch* pItem = pFilter->GetChildAt(item);
				if (tt::isSameStri(pItem->GetName(), "File")) {
					if (isValidSrcFile(pItem->GetAttribute("RelativePath")))
						cSrcFiles.m_lstSrcFiles += pItem->GetAttribute("RelativePath");
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
				cSrcFiles.m_cszTarget32 = (const char*) cszOutDir;
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
					cSrcFiles.m_bBuildForSpeed = true;

				pszOption = pRelease->GetAttribute("WarningLevel");
				if (pszOption && !tt::isSameSubStri(pszOption, "4"))
					cSrcFiles.m_WarningLevel = tt::Atoi(pszOption);

				pszOption = pRelease->GetAttribute("AdditionalIncludeDirectories");
				if (pszOption)
					cSrcFiles.m_cszIncDirs += pszOption;

				pszOption = pRelease->GetAttribute("PreprocessorDefinitions");
				if (pszOption) {
					ttCEnumStr enumFlags(pszOption);
					while (enumFlags.Enum()) {
						if (tt::isSameStri(enumFlags, "NDEBUG"))
							continue;	// we already add this
						if (tt::isSameStri(enumFlags, "_CONSOLE")) {	// the define is already in use, but make certain exeType matches
							cSrcFiles.m_exeType = CSrcFiles::EXE_CONSOLE;
							continue;	// we already add this
						}
						if (tt::isSameStri(enumFlags, "_USRDLL")) {	// the define is already in use, but make certain exeType matches
							cSrcFiles.m_exeType = CSrcFiles::EXE_DLL;
							continue;	// do we need to add this?
						}
						if (tt::isSameSubStri(enumFlags, "$("))	// Visual Studio specific, ignore it
							continue;
						if (cSrcFiles.m_cszCFlags.isNonEmpty())
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
