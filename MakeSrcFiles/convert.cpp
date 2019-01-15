/////////////////////////////////////////////////////////////////////////////
// Name:		conver.cpp
// Purpose:		Converts various build scripts into a .srcfiles file
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "../ttLib/include/ttxml.h" 	// ttXMLBranch, ttXML
#include "../ttLib/include/findfile.h"	// ttFindFile
#include "../ttLib/include/enumstr.h"	// ttEnumStr

#include "../common/writesrcfiles.h"	// CWriteSrcFiles

bool ConvertCodeBlocks(const char* pszBldFile);
bool ConvertCodeLite(const char* pszBldFile);
bool ConvertVcProj(const char* pszBldFile);
bool ConvertVcxProj(const char* pszBldFile);
bool ConvertVcxProj(const char* pszBldFile);

static void AddCodeLiteFiles(ttXMLBranch* pParent, CWriteSrcFiles& cSrcFiles);
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
		ttFindFile ff("*.vcxproj");
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

	const char* pszExt = tt::findlastchr(pszBldFile, '.');
	if (!pszExt) {
		ttString cszBuild(pszBldFile);
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

	if (tt::samestri(pszExt, ".project"))
		return ConvertCodeLite(pszBldFile);
	else if (tt::samestri(pszExt, ".cbp"))
		return ConvertCodeBlocks(pszBldFile);
	else if (tt::samestri(pszExt, ".vcproj"))
		return ConvertVcProj(pszBldFile);
	else if (tt::samestri(pszExt, ".vcxproj"))
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
	ttXML xml;

	HRESULT hr = xml.ParseXmlFile(pszBldFile);
	if (hr == S_OK)	{
		ttXMLBranch* pProject = xml.GetRootBranch()->FindFirstElement("CodeLite_Project");
		if (!pProject)	{
			ttStr cszMsg;
			puts(cszMsg.printf("Cannot locate <CodeLite_Project> in %s", pszBldFile));
			return false;
		}
		const char* pszProject = pProject->GetAttribute("Name");
		if (pszProject && *pszProject)
			cSrcFiles.m_cszProjectName = pszProject;

		for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
			ttXMLBranch* pItem = pProject->GetChildAt(item);
			if (tt::samestri(pItem->GetName(), "VirtualDirectory"))
				AddCodeLiteFiles(pItem, cSrcFiles);
			else if (tt::samestri(pItem->GetName(), "Settings")) {
				const char* pszType = pItem->GetAttribute("Type");
				if (tt::samestri(pszType, "Dynamic Library"))
					cSrcFiles.m_exeType = CSrcFiles::EXE_DLL;
				else if (tt::samestri(pszType, "Static Library"))
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
	ttXML xml;

	HRESULT hr = xml.ParseXmlFile(pszBldFile);
	if (hr == S_OK)	{
		ttXMLBranch* pProject = xml.GetRootBranch()->FindFirstElement("Project");
		if (!pProject)	{
			ttStr cszMsg;
			puts(cszMsg.printf("Cannot locate <Project> in %s", pszBldFile));
			return false;
		}
		for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
			ttXMLBranch* pItem = pProject->GetChildAt(item);
			if (tt::samestri(pItem->GetName(), "Option")) {
				if (pItem->GetAttribute("title"))
					cSrcFiles.m_cszProjectName = pItem->GetAttribute("title");
			}
			else if (tt::samestri(pItem->GetName(), "Unit")) {
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

	ttString cszFilters(pszBldFile);
	if (!tt::samestri(cszFilters.FindExt(), ".filters"))
		cszFilters += ".filters";

	CWriteSrcFiles cSrcFiles;
	ttXML xml;

	HRESULT hr = xml.ParseXmlFile(cszFilters);
	if (hr == S_OK)	{
		ttXMLBranch* pProject = xml.GetRootBranch()->FindFirstElement("Project");
		if (!pProject)	{
			ttStr cszMsg;
			puts(cszMsg.printf("Cannot locate <Project> in %s", (char*) cszFilters));
			return false;
		}
		for (size_t item = 0; item < pProject->GetChildrenCount(); item++) {
			ttXMLBranch* pItem = pProject->GetChildAt(item);
			if (tt::samestri(pItem->GetName(), "ItemGroup")) {
				for (size_t cmd = 0; cmd < pItem->GetChildrenCount(); cmd++) {
					ttXMLBranch* pCmd = pItem->GetChildAt(cmd);
					if (tt::samestri(pCmd->GetName(), "ClCompile") || tt::samestri(pCmd->GetName(), "ResourceCompile")) {
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
	ttXML xml;

	HRESULT hr = xml.ParseXmlFile(pszBldFile);
	if (hr == S_OK)	{
		ttXMLBranch* pProject = xml.GetRootBranch()->FindFirstElement("VisualStudioProject");
		if (!pProject)	{
			ttStr cszMsg;
			puts(cszMsg.printf("Cannot locate <VisualStudioProject> in %s", pszBldFile));
			return false;
		}

		if (pProject->GetAttribute("Name"))
			cSrcFiles.m_cszProjectName = pProject->GetAttribute("Name");

		ttXMLBranch* pFiles = pProject->FindFirstElement("Files");
		if (!pFiles)	{
			ttStr cszMsg;
			puts(cszMsg.printf("Cannot locate <pFiles> in %s", pszBldFile));
			return false;
		}
		ttXMLBranch* pFilter = pFiles->FindFirstElement("Filter");
		if (!pFilter) {
			ttStr cszMsg;
			puts(cszMsg.printf("Cannot locate <Filter> in %s", pszBldFile));
			return false;
		}
		for (size_t item = 0; item < pFilter->GetChildrenCount(); item++) {
			ttXMLBranch* pItem = pFilter->GetChildAt(item);
			if (tt::samestri(pItem->GetName(), "File")) {
				if (isValidSrcFile(pItem->GetAttribute("RelativePath")))
					cSrcFiles.m_lstSrcFiles += pItem->GetAttribute("RelativePath");
			}
		}
		pFilter = pFiles->FindFirstAttribute("Name", "Resource Files");
		if (pFilter) {
			for (size_t item = 0; item < pFilter->GetChildrenCount(); item++) {
				ttXMLBranch* pItem = pFilter->GetChildAt(item);
				if (tt::samestri(pItem->GetName(), "File")) {
					if (isValidSrcFile(pItem->GetAttribute("RelativePath")))
						cSrcFiles.m_lstSrcFiles += pItem->GetAttribute("RelativePath");
				}
			}
		}

		ttXMLBranch* pConfigurations = pProject->FindFirstElement("Configurations");
		ttXMLBranch* pConfiguration = pConfigurations ? pConfigurations->FindFirstElement("Configuration") : nullptr;
		if (pConfiguration) {
			// REVIEW: [randalphwa - 12/12/2018] Unusual, but possible to have 64-bit projects in an old .vcproj file,
			// but we should look for it and set the output directory to match 32 and 64 bit builds.

			ttXMLBranch* pOption = pConfiguration->FindFirstAttribute("OutputFile");
			if (pOption) {
				ttString cszOutDir(pOption->GetAttribute("OutputFile"));	// will typically be something like: "../bin/$(ProjectName).exe"
				char* pszFile = tt::fndFilename(cszOutDir);
				if (pszFile)
					*pszFile = 0;
				cSrcFiles.m_cszTarget32 = (const char*) cszOutDir;
			}
			do {
				if (tt::findstri(pConfiguration->GetAttribute("Name"), "Release"))
					break;
				pConfiguration = pConfigurations->FindNextElement("Configuration");
			} while(pConfiguration);

			ttXMLBranch* pRelease = pConfiguration ? pConfiguration->FindFirstAttribute("Name") : nullptr;

			if (pRelease) {
				const char* pszOption = pRelease->GetAttribute("FavorSizeOrSpeed");
				if (pszOption && tt::samesubstri(pszOption, "1"))
					cSrcFiles.m_bBuildForSpeed = true;

				pszOption = pRelease->GetAttribute("WarningLevel");
				if (pszOption && !tt::samesubstri(pszOption, "4"))
					cSrcFiles.m_WarningLevel = tt::atoi(pszOption);

				pszOption = pRelease->GetAttribute("AdditionalIncludeDirectories");
				if (pszOption)
					cSrcFiles.m_cszIncDirs += pszOption;

				pszOption = pRelease->GetAttribute("PreprocessorDefinitions");
				if (pszOption) {
					ttEnumStr enumFlags(pszOption);
					while (enumFlags.Enum()) {
						if (tt::samestri(enumFlags, "NDEBUG"))
							continue;	// we already add this
						if (tt::samestri(enumFlags, "_CONSOLE")) {	// the define is already in use, but make certain exeType matches
							cSrcFiles.m_exeType = CSrcFiles::EXE_CONSOLE;
							continue;	// we already add this
						}
						if (tt::samestri(enumFlags, "_USRDLL")) {	// the define is already in use, but make certain exeType matches
							cSrcFiles.m_exeType = CSrcFiles::EXE_DLL;
							continue;	// do we need to add this?
						}
						if (tt::samesubstri(enumFlags, "$("))	// Visual Studio specific, ignore it
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

static void AddCodeLiteFiles(ttXMLBranch* pParent, CWriteSrcFiles& cSrcFiles)
{
	for (size_t child = 0; child < pParent->GetChildrenCount(); child++) {
		ttXMLBranch* pFile = pParent->GetChildAt(child);
		if (tt::samestri(pFile->GetName(), "File")) {
			if (isValidSrcFile(pFile->GetAttribute("Name")))
				cSrcFiles.m_lstSrcFiles += pFile->GetAttribute("Name");
		}
		// CodeLite nests resources in a sub <VirtualDirectory> tag
		else if (tt::samestri(pFile->GetName(), "VirtualDirectory")) {
			AddCodeLiteFiles(pFile, cSrcFiles);
		}
	}
}

static bool isValidSrcFile(const char* pszFile)
{
	if (!pszFile)
		return false;

	char* psz = tt::findlastchr(pszFile, '.');
	if (psz) {
		if (	tt::samestri(psz, ".cpp") ||
				tt::samestri(psz, ".cc") ||
				tt::samestri(psz, ".cxx") ||
				tt::samestri(psz, ".c") ||
				tt::samestri(psz, ".rc"))
			return true;
	}
	return false;
}
