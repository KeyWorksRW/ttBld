/////////////////////////////////////////////////////////////////////////////
// Name:      convert.cpp
// Purpose:   Various conversion methods
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

// Assumes that everything except a .DSP file is an xml file and has been parsed into m_xml

#include "pch.h"

#include <ttenumstr.h>  // ttCEnumStr -- Enumerate through substrings in a string
#include <ttfile.h>     // ttCFile -- class for reading and writing files, strings, data, etc.

#include "convertdlg.h"  // CConvertDlg

bool CConvertDlg::ConvertCodeBlocks()
{
    if (!ttFileExists(m_cszConvertScript))
    {
        ttMsgBoxFmt(GETSTRING(IDS_NINJA_CANNOT_OPEN), MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
        return false;
    }

    ttCXMLBranch* pProject = m_xml.GetRootBranch()->FindFirstElement("Project");
    if (!pProject)
    {
        ttMsgBoxFmt(GETSTRING(IDS_NINJA_MISSING_PROJECT), MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
        return false;
    }

    for (size_t item = 0; item < pProject->GetChildrenCount(); item++)
    {
        ttCXMLBranch* pItem = pProject->GetChildAt(item);
        if (ttIsSameStrI(pItem->GetName(), "Option"))
        {
            if (pItem->GetAttribute("title"))
                m_cSrcFiles.UpdateOption(OPT_PROJECT, pItem->GetAttribute("title"));
        }
        else if (ttIsSameStrI(pItem->GetName(), "Unit"))
        {
            if (isValidSrcFile(pItem->GetAttribute("filename")))
                m_cSrcFiles.m_lstSrcFiles += MakeSrcRelative(pItem->GetAttribute("filename"));
        }
    }

    return true;
}

bool CConvertDlg::ConvertCodeLite()
{
    ttCXMLBranch* pProject = m_xml.GetRootBranch()->FindFirstElement("CodeLite_Project");
    if (!pProject)
    {
        ttMsgBoxFmt(GETSTRING(IDS_NINJA_INVALID_PROJECT), MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
        return false;
    }

    const char* pszProject = pProject->GetAttribute("Name");
    if (pszProject && *pszProject)
        m_cSrcFiles.UpdateOption(OPT_PROJECT, pszProject);

    for (size_t item = 0; item < pProject->GetChildrenCount(); item++)
    {
        ttCXMLBranch* pItem = pProject->GetChildAt(item);
        if (ttIsSameStrI(pItem->GetName(), "VirtualDirectory"))
            AddCodeLiteFiles(pItem);
        else if (ttIsSameStrI(pItem->GetName(), "Settings"))
        {
            const char* pszType = pItem->GetAttribute("Type");
            if (ttIsSameStrI(pszType, "Dynamic Library"))
                m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "dll");
            else if (ttIsSameStrI(pszType, "Static Library"))
                m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "lib");
        }
    }
    return true;
}

bool CConvertDlg::ConvertVcProj()
{
    ttCXMLBranch* pProject = m_xml.GetRootBranch()->FindFirstElement("VisualStudioProject");
    if (!pProject)
    {
        ttMsgBoxFmt(GETSTRING(IDS_NINJA_MISSING_VSP), MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
        return false;
    }

    if (pProject->GetAttribute("Name"))
        m_cSrcFiles.UpdateOption(OPT_PROJECT, pProject->GetAttribute("Name"));

    ttCXMLBranch* pFiles = pProject->FindFirstElement("Files");
    if (!pFiles)
    {
        ttMsgBoxFmt(GETSTRING(IDS_NINJA_MISSING_FILES), MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
        return false;
    }
    for (size_t iFilter = 0; iFilter < pFiles->GetChildrenCount(); ++iFilter)
    {
        ttCXMLBranch* pFilter = pFiles->GetChildAt(iFilter);
        if (ttIsSameStrI(pFilter->GetName(), "Filter"))
        {
            for (size_t item = 0; item < pFilter->GetChildrenCount(); item++)
            {
                ttCXMLBranch* pItem = pFilter->GetChildAt(item);
                if (ttIsSameStrI(pItem->GetName(), "File"))
                {
                    if (isValidSrcFile(pItem->GetAttribute("RelativePath")))
                        m_cSrcFiles.m_lstSrcFiles += MakeSrcRelative(pItem->GetAttribute("RelativePath"));
                }
            }
        }
    }

    ttCXMLBranch* pFilter = pFiles->FindFirstAttribute("Name", "Resource Files");
    if (pFilter)
    {
        for (size_t item = 0; item < pFilter->GetChildrenCount(); item++)
        {
            ttCXMLBranch* pItem = pFilter->GetChildAt(item);
            if (ttIsSameStrI(pItem->GetName(), "File"))
            {
                if (isValidSrcFile(pItem->GetAttribute("RelativePath")))
                    m_cSrcFiles.m_lstSrcFiles += MakeSrcRelative(pItem->GetAttribute("RelativePath"));
            }
        }
    }

    ttCXMLBranch* pConfigurations = pProject->FindFirstElement("Configurations");
    ttCXMLBranch* pConfiguration = pConfigurations ? pConfigurations->FindFirstElement("Configuration") : nullptr;
    if (pConfiguration)
    {
        // REVIEW: [randalphwa - 12/12/2018] Unusual, but possible to have 64-bit projects in an old .vcproj file,
        // but we should look for it and set the output directory to match 32 and 64 bit builds.

        ttCXMLBranch* pOption = pConfiguration->FindFirstAttribute("OutputFile");
        if (pOption)
        {
            ttCStr cszOutDir(
                pOption->GetAttribute("OutputFile"));  // will typically be something like: "../bin/$(ProjectName).exe"
            char* pszFile = ttFindFilePortion(cszOutDir);
            if (pszFile)
                *pszFile = 0;
            m_cSrcFiles.UpdateOption(OPT_TARGET_DIR32, (char*) cszOutDir);
        }
        do
        {
            if (ttStrStrI(pConfiguration->GetAttribute("Name"), "Release"))
                break;
            pConfiguration = pConfigurations->FindNextElement("Configuration");
        } while (pConfiguration);

        ttCXMLBranch* pRelease = pConfiguration ? pConfiguration->FindFirstAttribute("Name") : nullptr;

        if (pRelease)
        {
            const char* pszOption = pRelease->GetAttribute("FavorSizeOrSpeed");
            if (pszOption && ttIsSameSubStrI(pszOption, "1"))
                m_cSrcFiles.UpdateOption(OPT_OPTIMIZE, "speed");

            pszOption = pRelease->GetAttribute("WarningLevel");
            if (pszOption && !ttIsSameSubStrI(pszOption, "4"))
                m_cSrcFiles.UpdateOption(OPT_WARN_LEVEL, pszOption);

            pszOption = pRelease->GetAttribute("AdditionalIncludeDirectories");
            if (pszOption)
            {
                ttCStr csz(m_cSrcFiles.GetOption(OPT_INC_DIRS));
                csz += pszOption;
                m_cSrcFiles.UpdateOption(OPT_INC_DIRS, (char*) csz);
            }

            pszOption = pRelease->GetAttribute("PreprocessorDefinitions");
            if (pszOption)
            {
                ttCEnumStr enumFlags(pszOption);
                ttCStr     cszCFlags;
                while (enumFlags.Enum())
                {
                    if (ttIsSameStrI(enumFlags, "NDEBUG"))
                        continue;  // we already added this
                    if (ttIsSameStrI(enumFlags,
                                     "_CONSOLE"))  // the define is already in use, but make certain exeType matches
                    {
                        m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "console");
                        continue;
                    }
                    if (ttIsSameStrI(enumFlags,
                                     "_USRDLL"))  // the define is already in use, but make certain exeType matches
                    {
                        m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "dll");
                        continue;  // do we need to add this?
                    }
                    if (ttIsSameSubStrI(enumFlags, "$("))  // Visual Studio specific, ignore it
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

bool CConvertDlg::ConvertVcxProj()
{
    ttCXMLBranch* pProject = m_xml.GetRootBranch()->FindFirstElement("Project");
    if (!pProject)
    {
        ttMsgBoxFmt(GETSTRING(IDS_NINJA_MISSING_PROJECT), MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
        return false;
    }

    bool bDebugFlagsSeen = false;
    bool bRelFlagsSeen = false;
    bool bTypeSeen = false;

    for (size_t item = 0; item < pProject->GetChildrenCount(); item++)
    {
        ttCXMLBranch* pItem = pProject->GetChildAt(item);
        if (ttIsSameStrI(pItem->GetName(), "ItemGroup"))
        {
            for (size_t cmd = 0; cmd < pItem->GetChildrenCount(); cmd++)
            {
                ttCXMLBranch* pCmd = pItem->GetChildAt(cmd);
                if (ttIsSameStrI(pCmd->GetName(), "ClCompile") || ttIsSameStrI(pCmd->GetName(), "ResourceCompile"))
                {
                    const char* pszFile = pCmd->GetAttribute("Include");
                    if (pszFile && *pszFile)
                        m_cSrcFiles.m_lstSrcFiles += MakeSrcRelative(pszFile);
                }
            }
        }
        else if (ttIsSameStrI(pItem->GetName(), "PropertyGroup"))
        {
            if (bTypeSeen)
                continue;
            ttCXMLBranch* pFlags = pItem->FindFirstElement("ConfigurationType");
            if (pFlags && pFlags->GetChildrenCount() > 0)
            {
                ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                if (pChild->GetData())
                {
                    bTypeSeen = true;
                    if (ttIsSameStrI(pChild->GetData(), "DynamicLibrary"))
                        m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "dll");
                    else if (ttIsSameStrI(pChild->GetData(), "StaticLibrary"))
                        m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, "lib");
                    // TODO: [randalphwa - 5/9/2019] What are the options for console and gui?
                }
            }
        }
        else if (ttIsSameStrI(pItem->GetName(), "ItemDefinitionGroup"))
        {
            const char* pszCondition = pItem->GetAttribute("Condition");
            if (!bDebugFlagsSeen && pszCondition &&
                (ttStrStrI(pszCondition, "Debug|Win32") || ttStrStrI(pszCondition, "Debug|x64")))
            {
                bDebugFlagsSeen = true;
                for (size_t cmd = 0; cmd < pItem->GetChildrenCount(); cmd++)
                {
                    ttCXMLBranch* pCmd = pItem->GetChildAt(cmd);
                    if (ttIsSameStrI(pCmd->GetName(), "Midl"))
                    {
                        ttCXMLBranch* pFlags = pCmd->FindFirstElement("PreprocessorDefinitions");
                        if (pFlags && pFlags->GetChildrenCount() > 0)
                        {
                            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                            if (pChild->GetData())
                            {
                                ttCStr cszFlags("-D");
                                cszFlags += pChild->GetData();
                                cszFlags.ReplaceStr("_DEBUG;", "");
                                cszFlags.ReplaceStr(";%(PreprocessorDefinitions)", "");
                                while (cszFlags.ReplaceStr(";", " -D"))
                                    ;
                                m_cSrcFiles.UpdateOption(OPT_MDL_DBG, (char*) cszFlags);
                            }
                        }
                    }
                    else if (ttIsSameStrI(pCmd->GetName(), "ResourceCompile"))
                    {
                        ttCXMLBranch* pFlags = pCmd->FindFirstElement("PreprocessorDefinitions");
                        if (pFlags && pFlags->GetChildrenCount() > 0)
                        {
                            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                            if (pChild->GetData())
                            {
                                ttCStr cszFlags("-D");
                                cszFlags += pChild->GetData();
                                cszFlags.ReplaceStr("_DEBUG;", "");
                                cszFlags.ReplaceStr(";%(PreprocessorDefinitions)", "");
                                while (cszFlags.ReplaceStr(";", " -D"))
                                    ;
                                m_cSrcFiles.UpdateOption(OPT_RC_DBG, (char*) cszFlags);
                            }
                        }
                    }
                    else if (ttIsSameStrI(pCmd->GetName(), "ClCompile"))
                    {
                        ttCXMLBranch* pFlags = pCmd->FindFirstElement("FavorSizeOrSpeed");
                        if (pFlags && pFlags->GetChildrenCount() > 0)
                        {
                            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                            if (pChild->GetData())
                                m_cSrcFiles.UpdateOption(
                                    OPT_OPTIMIZE, ttIsSameSubStrI(pChild->GetData(), "size") ? "space" : "speed");
                        }
                        pFlags = pCmd->FindFirstElement("AdditionalIncludeDirectories");
                        if (pFlags && pFlags->GetChildrenCount() > 0)
                        {
                            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                            if (pChild->GetData())
                            {
                                const char* pszFirstSemi = ttStrChr(pChild->GetData(), ';');
                                if (pszFirstSemi)
                                    ++pszFirstSemi;
                                ttCStr cszFlags(ttIsSameSubStrI(pChild->GetData(), "$(OutDir") && pszFirstSemi ?
                                                    pszFirstSemi :
                                                    pChild->GetData());
                                cszFlags.ReplaceStr(";%(AdditionalIncludeDirectories)", "");
                                m_cSrcFiles.UpdateOption(OPT_INC_DIRS, (char*) cszFlags);
                            }
                        }
                        pFlags = pCmd->FindFirstElement("PrecompiledHeaderFile");
                        if (pFlags && pFlags->GetChildrenCount() > 0)
                        {
                            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                            if (pChild->GetData())
                                m_cSrcFiles.UpdateOption(OPT_PCH, pChild->GetData());
                        }
                        pFlags = pCmd->FindFirstElement("WarningLevel");
                        if (pFlags && pFlags->GetChildrenCount() > 0)
                        {
                            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                            if (pChild->GetData())
                            {
                                const char* pszTmp = pChild->GetData();
                                while (*pszTmp && !ttIsDigit(*pszTmp))
                                    ++pszTmp;
                                m_cSrcFiles.UpdateOption(OPT_WARN_LEVEL, pszTmp);
                            }
                        }
                        pFlags = pCmd->FindFirstElement("CallingConvention");
                        if (pFlags && pFlags->GetChildrenCount() > 0)
                        {
                            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                            if (pChild->GetData() && ttStrStrI(pChild->GetData(), "stdcall"))
                            {
                                m_cSrcFiles.UpdateOption(OPT_STDCALL, true);
                            }
                        }
                        pFlags = pCmd->FindFirstElement("PreprocessorDefinitions");
                        if (pFlags && pFlags->GetChildrenCount() > 0)
                        {
                            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                            if (pChild->GetData())
                            {
                                ttCStr cszFlags("-D");
                                cszFlags += pChild->GetData();
                                cszFlags.ReplaceStr("_DEBUG;", "");
                                cszFlags.ReplaceStr(";%(PreprocessorDefinitions)", "");
                                while (cszFlags.ReplaceStr(";", " -D"))
                                    ;
                                m_cSrcFiles.UpdateOption(OPT_CFLAGS_DBG, (char*) cszFlags);
                            }
                        }
                    }
                }
            }
            else if (!bRelFlagsSeen && pszCondition &&
                     (ttStrStrI(pszCondition, "Release|Win32") || ttStrStrI(pszCondition, "Release|x64")))
            {
                bRelFlagsSeen = true;
                for (size_t cmd = 0; cmd < pItem->GetChildrenCount(); cmd++)
                {
                    ttCXMLBranch* pCmd = pItem->GetChildAt(cmd);
                    if (ttIsSameStrI(pCmd->GetName(), "Midl"))
                    {
                        ttCXMLBranch* pFlags = pCmd->FindFirstElement("PreprocessorDefinitions");
                        if (pFlags && pFlags->GetChildrenCount() > 0)
                        {
                            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                            if (pChild->GetData())
                            {
                                ttCStr cszFlags("-D");
                                cszFlags += pChild->GetData();
                                cszFlags.ReplaceStr("NDEBUG;", "");
                                cszFlags.ReplaceStr(";%(PreprocessorDefinitions)", "");
                                while (cszFlags.ReplaceStr(";", " -D"))
                                    ;
                                m_cSrcFiles.UpdateOption(OPT_MDL_REL, (char*) cszFlags);
                            }
                        }
                    }
                    else if (ttIsSameStrI(pCmd->GetName(), "ResourceCompile"))
                    {
                        ttCXMLBranch* pFlags = pCmd->FindFirstElement("PreprocessorDefinitions");
                        if (pFlags && pFlags->GetChildrenCount() > 0)
                        {
                            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                            if (pChild->GetData())
                            {
                                ttCStr cszFlags("-D");
                                cszFlags += pChild->GetData();
                                cszFlags.ReplaceStr("NDEBUG;", "");
                                cszFlags.ReplaceStr(";%(PreprocessorDefinitions)", "");
                                while (cszFlags.ReplaceStr(";", " -D"))
                                    ;
                                m_cSrcFiles.UpdateOption(OPT_RC_REL, (char*) cszFlags);
                            }
                        }
                    }
                    else if (ttIsSameStrI(pCmd->GetName(), "ClCompile"))
                    {
                        ttCXMLBranch* pFlags = pCmd->FindFirstElement("FavorSizeOrSpeed");
                        if (pFlags && pFlags->GetChildrenCount() > 0)
                        {
                            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                            if (pChild->GetData())
                                m_cSrcFiles.UpdateOption(
                                    OPT_OPTIMIZE, ttIsSameSubStrI(pChild->GetData(), "size") ? "space" : "speed");
                        }
                        pFlags = pCmd->FindFirstElement("PrecompiledHeaderFile");
                        if (pFlags && pFlags->GetChildrenCount() > 0)
                        {
                            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                            if (pChild->GetData())
                                m_cSrcFiles.UpdateOption(OPT_PCH, pChild->GetData());
                        }
                        pFlags = pCmd->FindFirstElement("WarningLevel");
                        if (pFlags && pFlags->GetChildrenCount() > 0)
                        {
                            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                            if (pChild->GetData())
                            {
                                const char* pszTmp = pChild->GetData();
                                while (*pszTmp && !ttIsDigit(*pszTmp))
                                    ++pszTmp;
                                m_cSrcFiles.UpdateOption(OPT_WARN_LEVEL, pszTmp);
                            }
                        }
                        pFlags = pCmd->FindFirstElement("CallingConvention");
                        if (pFlags && pFlags->GetChildrenCount() > 0)
                        {
                            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                            if (pChild->GetData() && ttStrStrI(pChild->GetData(), "stdcall"))
                            {
                                m_cSrcFiles.UpdateOption(OPT_STDCALL, true);
                            }
                        }
                        pFlags = pCmd->FindFirstElement("PreprocessorDefinitions");
                        if (pFlags && pFlags->GetChildrenCount() > 0)
                        {
                            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                            if (pChild->GetData())
                            {
                                ttCStr cszFlags("-D");
                                cszFlags += pChild->GetData();
                                cszFlags.ReplaceStr("NDEBUG;", "");
                                cszFlags.ReplaceStr(";%(PreprocessorDefinitions)", "");
                                while (cszFlags.ReplaceStr(";", " -D"))
                                    ;
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
        ttIsSameStrI(m_cSrcFiles.GetOption(OPT_CFLAGS_REL), m_cSrcFiles.GetOption(OPT_CFLAGS_DBG)))
    {
        m_cSrcFiles.UpdateOption(OPT_CFLAGS_CMN, m_cSrcFiles.GetOption(OPT_CFLAGS_REL));
        m_cSrcFiles.UpdateOption(OPT_CFLAGS_REL, "");
        m_cSrcFiles.UpdateOption(OPT_CFLAGS_DBG, "");
    }
    if (m_cSrcFiles.GetOption(OPT_MDL_REL) && m_cSrcFiles.GetOption(OPT_MDL_DBG) &&
        ttIsSameStrI(m_cSrcFiles.GetOption(OPT_MDL_REL), m_cSrcFiles.GetOption(OPT_MDL_DBG)))
    {
        m_cSrcFiles.UpdateOption(OPT_MDL_CMN, m_cSrcFiles.GetOption(OPT_MDL_REL));
        m_cSrcFiles.UpdateOption(OPT_MDL_REL, "");
        m_cSrcFiles.UpdateOption(OPT_MDL_DBG, "");
    }
    if (m_cSrcFiles.GetOption(OPT_RC_REL) && m_cSrcFiles.GetOption(OPT_RC_DBG) &&
        ttIsSameStrI(m_cSrcFiles.GetOption(OPT_RC_REL), m_cSrcFiles.GetOption(OPT_RC_DBG)))
    {
        m_cSrcFiles.UpdateOption(OPT_RC_CMN, m_cSrcFiles.GetOption(OPT_RC_REL));
        m_cSrcFiles.UpdateOption(OPT_RC_REL, "");
        m_cSrcFiles.UpdateOption(OPT_RC_DBG, "");
    }

    return true;
}

enum
{
    GROUP_UNKNOWN,
    GROUP_SRC,
    GROUP_HEADER,
    GROUP_RESOURCE,
};

bool CConvertDlg::ConvertDsp()
{
    ttCFile file;
    if (!file.ReadFile(m_cszConvertScript))
    {
        ttMsgBoxFmt(GETSTRING(IDS_NINJA_CANNOT_OPEN), MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
        return false;
    }

    bool bReleaseSection = false;
    auto Group = GROUP_UNKNOWN;

    while (file.ReadLine())
    {
        char* pszBegin = ttFindNonSpace(file);
        if (!*pszBegin)
            continue;

        if (ttIsSameSubStrI(pszBegin, "CFG"))
        {
            char* pszProject = ttStrChr(pszBegin, '=');
            if (!pszProject)
                continue;
            pszProject = ttFindNonSpace(pszProject + 1);
            char* pszEndProject = ttFindSpace(pszProject);
            if (pszEndProject)
                *pszEndProject = 0;
            if (ttIsEmpty(m_cSrcFiles.GetProjectName()))
                m_cSrcFiles.UpdateOption(OPT_PROJECT, pszProject);
        }

        else if (ttIsSameSubStrI(pszBegin, "!IF") && ttStrStrI(pszBegin, "$(CFG)"))
        {
            bReleaseSection = ttStrStrI(pszBegin, " Release") ? true : false;
        }

        else if (*pszBegin == '#' && ttStrStrI(pszBegin, "Begin Group"))
        {
            if (ttStrStrI(pszBegin, "Source Files"))
                Group = GROUP_SRC;
            else if (ttStrStrI(pszBegin, "Header Files"))
                Group = GROUP_HEADER;
            else if (ttStrStrI(pszBegin, "Resource Files"))
                Group = GROUP_RESOURCE;
            else
                Group = GROUP_UNKNOWN;
        }

        else if (*pszBegin == '#' && (ttStrStrI(pszBegin, "ADD BASE CPP") || ttStrStrI(pszBegin, "ADD CPP")))
        {
            // Since this is a really old project, we ignore any compiler flags -- we just grab the defines

            ttCStr cszCurFlags, cszNewFlags;
            if (m_cSrcFiles.GetOption(sfopt::OPT_CFLAGS_CMN))
                cszCurFlags = m_cSrcFiles.GetOption(sfopt::OPT_CFLAGS_CMN);
            if (bReleaseSection && m_cSrcFiles.GetOption(sfopt::OPT_CFLAGS_REL))
            {
                if (ttIsNonEmpty(cszCurFlags))
                    cszCurFlags += " ";
                cszCurFlags += m_cSrcFiles.GetOption(sfopt::OPT_CFLAGS_REL);
            }
            else if (!bReleaseSection && m_cSrcFiles.GetOption(sfopt::OPT_CFLAGS_DBG))
            {
                if (ttIsNonEmpty(cszCurFlags))
                {
                    cszCurFlags += " ";
                    cszCurFlags += m_cSrcFiles.GetOption(sfopt::OPT_CFLAGS_DBG);
                }
            }

            char* pszDef = ttStrStr(pszBegin, "/D");
            if (pszDef)
            {
                do
                {
                    pszDef = ttStrChr(pszDef, CH_QUOTE);
                    if (pszDef)
                    {
                        ttCStr csz;
                        csz.GetQuotedString(pszDef);
                        if (!ttIsSameSubStr(csz, "NDEBUG") && !ttIsSameSubStr(csz, "_DEBUG") &&
                            !ttIsSameSubStr(csz, "_WIN32"))
                        {
                            ttCStr cszFlag("-D");
                            cszFlag += csz;
                            // If we don't already have the flag, then add it
                            if (!ttStrStrI(cszCurFlags, cszFlag))
                            {
                                if (ttIsNonEmpty(cszNewFlags))
                                    cszNewFlags += " ";
                                cszNewFlags += (char*) cszFlag;
                            }
                        }
                        pszDef = ttStrStr(pszDef, "/D");  // look for the next define
                    }
                } while (pszDef);
            }

            cszCurFlags.Delete();
            if (cszNewFlags.IsNonEmpty())
            {
                if (bReleaseSection && m_cSrcFiles.GetOption(sfopt::OPT_CFLAGS_REL))
                {
                    cszCurFlags = m_cSrcFiles.GetOption(sfopt::OPT_CFLAGS_REL);
                    cszCurFlags += " ";
                }
                cszCurFlags += (char*) cszNewFlags;
                m_cSrcFiles.UpdateOption(OPT_CFLAGS_REL, (char*) cszCurFlags);
            }
        }

        else if (ttIsSameSubStrI(pszBegin, "SOURCE"))
        {
            if (Group == GROUP_SRC)
            {
                char* pszFile = ttStrChr(pszBegin, '=');
                if (!pszFile)
                    continue;
                pszFile = ttFindNonSpace(pszFile + 1);
                if (ttIsSameSubStr(pszFile, ".\\"))
                    pszFile += 2;
                *m_cSrcFiles.GetSrcFilesList() += pszFile;
            }
        }
    }
    return true;
}
