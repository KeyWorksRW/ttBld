/////////////////////////////////////////////////////////////////////////////
// Name:      CVcxWrite
// Purpose:   Class for creating/maintaining .vcxproj file for use by the msbuild build tool (or VS IDE)
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#ifdef _WINDOWS_
    #include <Rpc.h>
    #pragma comment(lib, "Rpcrt4.lib")
#endif

#include <ttTR.h>  // Function for translating strings

#include <ttenumstr.h>   // ttCEnumStr -- Enumerate through substrings in a string
#include <ttfile.h>      // ttCFile
#include <ttfindfile.h>  // ttCFindFile

#include "resource.h"
#include "vcxproj.h"  // CVcxRead, CVcxWrite

CVcxRead::CVcxRead(ttCParseXML* pxml, CWriteSrcFiles* pcSrcFiles, ttCStr* pcszConvertScript)
{
    m_pxml = pxml;
    m_pcSrcFiles = pcSrcFiles;
    m_pcszConvertScript = pcszConvertScript;
}

bool CVcxRead::ConvertVcxProj()
{
    ttCXMLBranch* pProject = m_pxml->GetRootBranch()->FindFirstElement("Project");
    if (!pProject)
    {
        ttMsgBoxFmt(_tt("Cannot locate <Project> in %s"), MB_OK | MB_ICONWARNING, (char*) *m_pcszConvertScript);
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
                        m_pcSrcFiles->GetSrcFilesList().addfile(MakeSrcRelative(pszFile));
                }
            }
        }
        else if (ttIsSameStrI(pItem->GetName(), "PropertyGroup"))
        {
            if (!bTypeSeen)
            {
                ttCXMLBranch* pFlags = pItem->FindFirstElement("ConfigurationType");
                if (pFlags && pFlags->GetChildrenCount() > 0)
                {
                    ttCXMLBranch* pChild = pFlags->GetChildAt(0);
                    if (pChild->GetData())
                    {
                        bTypeSeen = true;
                        if (ttIsSameStrI(pChild->GetData(), "DynamicLibrary"))
                            m_pcSrcFiles->UpdateOption(OPT_EXE_TYPE, "dll");
                        else if (ttIsSameStrI(pChild->GetData(), "StaticLibrary"))
                            m_pcSrcFiles->UpdateOption(OPT_EXE_TYPE, "lib");
                        // TODO: [randalphwa - 5/9/2019] What are the options for console and gui?
                        continue;  // We don't care about any other settings in this group
                    }
                }
            }
            if (pItem->cAttributes == 0 && pItem->cChildren > 0)
            {
                for (size_t child = 0; child < pItem->cChildren; ++child)
                {
                    // Visual Studio lets you specify different directories and target names for debug and release
                    // builds. ttBld only supports a single target name and directory and then modifies that based
                    // on Debug versus Release builds. Since the two methods aren't really compatible, we only use
                    // the release target name, and whichever output directory we encounter first.

                    ttCXMLBranch* pChild = pItem->GetChildAt(child);
                    if (ttIsSameSubStrI(pChild->GetName(), "OutDir"))
                    {
                        if (!m_pcSrcFiles->GetOption(OPT_TARGET_DIR64) && pChild->cChildren > 0)
                        {
                            m_pcSrcFiles->UpdateOption(OPT_TARGET_DIR64, pChild->GetChildAt(0)->GetData());
                            m_pcSrcFiles->UpdateOption(OPT_TARGET_DIR32, pChild->GetChildAt(0)->GetData());
                        }
                    }
                    else if (ttIsSameSubStrI(pChild->GetName(), "TargetName"))
                    {
                        if (pChild->GetAttributeAt(0)->pszValue &&
                            ttStrStrI(pChild->GetAttributeAt(0)->pszValue, "Release"))
                        {
                            if (!m_pcSrcFiles->GetOption(OPT_PROJECT) && pChild->cChildren > 0)
                            {
                                m_pcSrcFiles->UpdateOption(OPT_PROJECT, pChild->GetChildAt(0)->GetData());
                            }
                        }
                    }
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
                        ProcessMidl(pCmd, true);
                    }
                    else if (ttIsSameStrI(pCmd->GetName(), "ResourceCompile"))
                    {
                        ProcessRC(pCmd, true);
                    }
                    else if (ttIsSameStrI(pCmd->GetName(), "Link"))
                    {
                        ProcessLink(pCmd, true);
                    }
                    else if (ttIsSameStrI(pCmd->GetName(), "ClCompile"))
                    {
                        ProcessCompiler(pCmd, true);
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
                        ProcessMidl(pCmd, false);
                    }
                    else if (ttIsSameStrI(pCmd->GetName(), "ResourceCompile"))
                    {
                        ProcessRC(pCmd, false);
                    }
                    else if (ttIsSameStrI(pCmd->GetName(), "Link"))
                    {
                        ProcessLink(pCmd, false);
                    }
                    else if (ttIsSameStrI(pCmd->GetName(), "ClCompile"))
                    {
                        ProcessCompiler(pCmd, false);
                    }
                }
            }
        }
    }

    // The project file will have specified resouce compiler flags even if there isn't a resource file. If there is
    // no resource file, then we remove those flags here.

    if (m_pcSrcFiles->GetRcName().empty())
    {
        if (m_pcSrcFiles->GetOption(OPT_RC_CMN))
            m_pcSrcFiles->UpdateOption(OPT_RC_CMN, "");
        if (m_pcSrcFiles->GetOption(OPT_RC_REL))
            m_pcSrcFiles->UpdateOption(OPT_RC_REL, "");
        if (m_pcSrcFiles->GetOption(OPT_RC_DBG))
            m_pcSrcFiles->UpdateOption(OPT_RC_DBG, "");
    }

    // If Debug and Release flags are the same, then remove them and just use the common flag setting

    if (m_pcSrcFiles->GetOption(OPT_CFLAGS_REL) && m_pcSrcFiles->GetOption(OPT_CFLAGS_DBG) &&
        ttIsSameStrI(m_pcSrcFiles->GetOption(OPT_CFLAGS_REL), m_pcSrcFiles->GetOption(OPT_CFLAGS_DBG)))
    {
        m_pcSrcFiles->UpdateOption(OPT_CFLAGS_CMN, m_pcSrcFiles->GetOption(OPT_CFLAGS_REL));
        m_pcSrcFiles->UpdateOption(OPT_CFLAGS_REL, "");
        m_pcSrcFiles->UpdateOption(OPT_CFLAGS_DBG, "");
    }
    if (m_pcSrcFiles->GetOption(OPT_MDL_REL) && m_pcSrcFiles->GetOption(OPT_MDL_DBG) &&
        ttIsSameStrI(m_pcSrcFiles->GetOption(OPT_MDL_REL), m_pcSrcFiles->GetOption(OPT_MDL_DBG)))
    {
        m_pcSrcFiles->UpdateOption(OPT_MDL_CMN, m_pcSrcFiles->GetOption(OPT_MDL_REL));
        m_pcSrcFiles->UpdateOption(OPT_MDL_REL, "");
        m_pcSrcFiles->UpdateOption(OPT_MDL_DBG, "");
    }
    if (m_pcSrcFiles->GetOption(OPT_RC_REL) && m_pcSrcFiles->GetOption(OPT_RC_DBG) &&
        ttIsSameStrI(m_pcSrcFiles->GetOption(OPT_RC_REL), m_pcSrcFiles->GetOption(OPT_RC_DBG)))
    {
        m_pcSrcFiles->UpdateOption(OPT_RC_CMN, m_pcSrcFiles->GetOption(OPT_RC_REL));
        m_pcSrcFiles->UpdateOption(OPT_RC_REL, "");
        m_pcSrcFiles->UpdateOption(OPT_RC_DBG, "");
    }

    return true;
}

void CVcxRead::ProcessMidl(ttCXMLBranch* pSection, bool bDebug)
{
    ttCXMLBranch* pFlags = pSection->FindFirstElement("PreprocessorDefinitions");
    if (pFlags && pFlags->GetChildrenCount() > 0)
    {
        ttCXMLBranch* pChild = pFlags->GetChildAt(0);
        if (pChild->GetData())
        {
            ttCStr cszFlags("-D");
            cszFlags += pChild->GetData();
            cszFlags.ReplaceStr("_DEBUG;", "");
            cszFlags.ReplaceStr("NDEBUG;", "");
            cszFlags.ReplaceStr(";%(PreprocessorDefinitions)", "");
            while (cszFlags.ReplaceStr(";", " -D"))
                ;
            m_pcSrcFiles->UpdateOption(bDebug ? OPT_MDL_DBG : OPT_MDL_REL, (char*) cszFlags);
        }
    }
}

void CVcxRead::ProcessRC(ttCXMLBranch* pSection, bool bDebug)
{
    ttCXMLBranch* pFlags = pSection->FindFirstElement("PreprocessorDefinitions");
    if (pFlags && pFlags->GetChildrenCount() > 0)
    {
        ttCXMLBranch* pChild = pFlags->GetChildAt(0);
        if (pChild->GetData())
        {
            ttCStr cszFlags("-D");
            cszFlags += pChild->GetData();
            cszFlags.ReplaceStr("_DEBUG;", "");
            cszFlags.ReplaceStr("NDEBUG;", "");
            cszFlags.ReplaceStr(";%(PreprocessorDefinitions)", "");
            while (cszFlags.ReplaceStr(";", " -D"))
                ;
            m_pcSrcFiles->UpdateOption(bDebug ? OPT_RC_DBG : OPT_RC_REL, (char*) cszFlags);
        }
    }
}

void CVcxRead::ProcessCompiler(ttCXMLBranch* pSection, bool bDebug)
{
    ttCXMLBranch* pFlags;

    pFlags = pSection->FindFirstElement("AdditionalOptions");
    if (pFlags && pFlags->GetChildrenCount() > 0)
    {
        ttCXMLBranch* pChild = pFlags->GetChildAt(0);
        if (pChild->GetData())
        {
            ttCEnumStr enumFlags(pChild->GetData(), CH_SPACE);
            ttCStr     cszCFlags;
            if (m_pcSrcFiles->GetOption(OPT_CFLAGS_CMN))
                cszCFlags = m_pcSrcFiles->GetOption(OPT_CFLAGS_CMN);
            while (enumFlags.Enum())
            {
                if (ttIsSameSubStrI(enumFlags + 1, "std:"))
                {
                    if (cszCFlags.IsEmpty() || !ttStrStrI(cszCFlags, enumFlags))
                    {
                        if (cszCFlags.IsNonEmpty())
                            cszCFlags += " ";
                        cszCFlags += enumFlags;
                    }
                }
            }
            if (cszCFlags.IsNonEmpty())
                m_pcSrcFiles->UpdateOption(OPT_CFLAGS_CMN, (char*) cszCFlags);
        }
    }
    if (!bDebug)
    {
        pFlags = pSection->FindFirstElement("FavorSizeOrSpeed");
        if (pFlags && pFlags->GetChildrenCount() > 0)
        {
            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
            if (pChild->GetData())
                m_pcSrcFiles->UpdateOption(OPT_OPTIMIZE,
                                           ttIsSameSubStrI(pChild->GetData(), "size") ? "space" : "speed");
        }
    }
    if (!m_pcSrcFiles->GetOption(OPT_INC_DIRS))
    {
        pFlags = pSection->FindFirstElement("AdditionalIncludeDirectories");
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

                // Paths will be relative to the location of the script file. We need to make them
                // relative to .srcfiles.yaml.

                ttCEnumStr cszPaths(cszFlags);
                ttCStr     cszInc, cszTmp;

                while (cszPaths.Enum())
                {
                    ConvertScriptDir(cszPaths, cszTmp);
                    if (cszInc.IsNonEmpty())
                        cszInc += ";";
                    cszInc += cszTmp;
                }

                m_pcSrcFiles->UpdateOption(OPT_INC_DIRS, (char*) cszInc);
            }
        }
    }
    if (!m_pcSrcFiles->GetOption(OPT_PCH))
    {
        pFlags = pSection->FindFirstElement("PrecompiledHeaderFile");
        if (pFlags && pFlags->GetChildrenCount() > 0)
        {
            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
            if (pChild->GetData())
                m_pcSrcFiles->UpdateOption(OPT_PCH, pChild->GetData());
        }
    }
    if (!m_pcSrcFiles->GetOption(OPT_WARN_LEVEL))
    {
        pFlags = pSection->FindFirstElement("WarningLevel");
        if (pFlags && pFlags->GetChildrenCount() > 0)
        {
            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
            if (pChild->GetData())
            {
                const char* pszTmp = pChild->GetData();
                while (*pszTmp && !ttIsDigit(*pszTmp))
                    ++pszTmp;
                m_pcSrcFiles->UpdateOption(OPT_WARN_LEVEL, pszTmp);
            }
        }
    }
    pFlags = pSection->FindFirstElement("CallingConvention");
    if (pFlags && pFlags->GetChildrenCount() > 0)
    {
        ttCXMLBranch* pChild = pFlags->GetChildAt(0);
        if (pChild->GetData() && ttStrStrI(pChild->GetData(), "stdcall"))
        {
            m_pcSrcFiles->UpdateOption(OPT_STDCALL, true);
        }
    }
    pFlags = pSection->FindFirstElement("PreprocessorDefinitions");
    if (pFlags && pFlags->GetChildrenCount() > 0)
    {
        ttCXMLBranch* pChild = pFlags->GetChildAt(0);
        if (pChild->GetData())
        {
            ttCStr cszFlags("-D");
            cszFlags += pChild->GetData();
            cszFlags.ReplaceStr("_DEBUG;", "");
            cszFlags.ReplaceStr("NDEBUG;", "");
            cszFlags.ReplaceStr(";%(PreprocessorDefinitions)", "");
            while (cszFlags.ReplaceStr(";", " -D"))
                ;
            m_pcSrcFiles->UpdateOption(bDebug ? OPT_CFLAGS_DBG : OPT_CFLAGS_REL, (char*) cszFlags);
        }
    }
}

void CVcxRead::ProcessLink(ttCXMLBranch* pSection, bool bDebug)
{
    ttCXMLBranch* pFlags = pSection->FindFirstElement("AdditionalDependencies");
    if (pFlags && pFlags->GetChildrenCount() > 0)
    {
        ttCXMLBranch* pChild = pFlags->GetChildAt(0);
        if (pChild->GetData())
        {
            ttCEnumStr enumLibs(pChild->GetData());
            ttCStr     cszCurLibs;
            while (enumLibs.Enum())
            {
                // We only add libraries that are relative to our project
                if (ttIsSameSubStr(enumLibs, ".."))
                {
                    if (cszCurLibs.IsNonEmpty())
                        cszCurLibs += ";";
                    cszCurLibs += enumLibs;
                }
            }
            if (cszCurLibs.IsNonEmpty())
                m_pcSrcFiles->UpdateOption(bDebug ? OPT_LIBS_DBG : OPT_LIBS_REL, (char*) cszCurLibs);
        }
    }
}

void CVcxRead::ConvertScriptDir(const char* pszDir, ttCStr& cszResult)
{
    if (pszDir[1] == ':')
    {
        // If the path starts with a drive letter, then we can't make it relative
        cszResult = pszDir;
        return;
    }

    ttCStr cszScript(*m_pcszConvertScript);
    char*  pszFilePortion = ttFindFilePortion(cszScript);
    ttASSERT(pszFilePortion);
    *pszFilePortion = 0;
    cszScript.AppendFileName(pszDir);

    cszScript.FullPathName();

    ttCStr cszCWD;
    cszCWD.GetCWD();

    // ttConvertToRelative is expecting a filename, so let's make one up.

    cszScript.AppendFileName("pch.h");
    ttConvertToRelative(cszCWD, cszScript, cszResult);
    pszFilePortion = ttFindFilePortion(cszResult);
    if (pszFilePortion)
    {
        if (pszFilePortion > cszResult.GetPtr() && pszFilePortion[-1] == '/')
            --pszFilePortion;  // backup so that we remove the trailing slash
        *pszFilePortion = 0;
    }
}

// This function first converts the file relative to the location of the build script, and then relative to the
// location of .srcfiles

char* CVcxRead::MakeSrcRelative(const char* pszFile)
{
    if (m_cszScriptRoot.IsEmpty())
    {
        m_cszScriptRoot = (char*) *m_pcszConvertScript;
        m_cszScriptRoot.FullPathName();
        char* pszFilePortion = ttFindFilePortion(m_cszScriptRoot);
        ttASSERT_MSG(pszFilePortion, "No filename in m_cszScriptRoot--things will go badly without it.");
        if (pszFilePortion)
            *pszFilePortion = 0;

        // For GetFullPathName() to work properly on a file inside the script, we need to be in the same directory
        // as the script file

        ttChDir(m_cszScriptRoot);
    }

    if (m_cszOutRoot.IsEmpty())
    {
        m_cszOutRoot.GetCWD();
        m_cszOutRoot.FullPathName();
    }

    ttCStr cszFile(pszFile);
    cszFile.FullPathName();

    ttConvertToRelative(m_cszOutRoot, cszFile, m_cszRelative);
    return m_cszRelative;
}

static bool CreateGuid(ttCStr& cszGuid)
{
    UUID       uuid;
    RPC_STATUS ret_val = ::UuidCreate(&uuid);

    if (ret_val == RPC_S_OK)
    {
        RPC_CSTR pszUuid = nullptr;
        if (::UuidToStringA(&uuid, &pszUuid) == RPC_S_OK && pszUuid)
        {
            cszGuid = (char*) pszUuid;
            ::RpcStringFreeA(&pszUuid);
        }
    }
    return cszGuid.IsNonEmpty();
}

bool CVcxWrite::CreateBuildFile()
{
#ifndef _WINDOWS_
    // Currently we only support creating VisualStudio projects on Windows. To get this to work on another
    // platform, a replacement would be needed for creating a GUID, and the templates we store in the .rc file
    // would need to be added in a different way (perhaps including them directly into the source code instead of
    // the resource).

    return false;
#endif  // _WINDOWS_

    ttCStr cszGuid;
    if (!CreateGuid(cszGuid))
    {
        AddError("Unable to create a UUID -- cannot create .vcxproj without it.");
        return false;
    }

    ttCStr cszProjVC(GetProjectName());
    cszProjVC.ChangeExtension(".vcxproj");
    if (!ttFileExists(cszProjVC))
    {
        ttCFile kf;
        kf.ReadResource(IDR_VCXPROJ_MASTER);
        while (kf.ReplaceStr("%guid%", cszGuid))
            ;
        while (kf.ReplaceStr("%%DebugExe%", GetTargetDebug()))
            ;
        while (kf.ReplaceStr("%%ReleaseExe%", GetTargetRelease()))
            ;
        while (kf.ReplaceStr("%%DebugExe64%", GetTargetDebug()))
            ;
        while (kf.ReplaceStr("%%ReleaseExe64%", GetTargetRelease()))
            ;

        for (auto file : m_lstSrcFiles)
        {
            auto ext = file.extension();
            if (ext.empty() || std::tolower(ext[1] != 'c'))
                continue;
            kf.WriteEol(
                wxString::Format(" <ItemGroup>\n    <ClCompile Include=%kq />\n  </ItemGroup>", file.c_str()));
        }

        ttCStr cszSrcFile;
        if (!m_RCname.empty())
        {
            cszSrcFile.printf(" <ItemGroup>\n    <ResourceCompile Include=%kq />\n  </ItemGroup>",
                              m_RCname.c_str());
            kf.WriteEol(cszSrcFile);
        }

        ttCFindFile ff("*.h");  // add all header files in current directory
        if (ff.IsValid())
        {
            do
            {
                cszSrcFile.printf(" <ItemGroup>\n    <ClInclude Include=%kq />\n  </ItemGroup>", (const char*) ff);
                kf.WriteEol(cszSrcFile);
            } while (ff.NextFile());
        }

        kf.WriteEol("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />");
        kf.WriteEol("  <ImportGroup Label=\"ExtensionTargets\">");
        kf.WriteEol("  </ImportGroup>");
        kf.WriteEol("</Project>");

        if (!kf.WriteFile(cszProjVC))
        {
            ttCStr cszMsg;
            cszMsg.printf("Unable to write to %s", (char*) cszProjVC);
            AddError(cszMsg);
            return false;
        }
        else
            printf("Created %s\n", (char*) cszProjVC);

        kf.Delete();
        kf.ReadResource(IDR_VCXPROJ_FILTERS);
        CreateGuid(cszGuid);  // it already succeeded once if we got here, so we don't check for error again
        while (kf.ReplaceStr("%guidSrc%", cszGuid))
            ;
        CreateGuid(cszGuid);
        while (kf.ReplaceStr("%guidHdr%", cszGuid))
            ;
        CreateGuid(cszGuid);
        while (kf.ReplaceStr("%guidResource%", cszGuid))
            ;

        kf.WriteEol("  <ItemGroup>");

        for (size_t pos = 0; pos < m_lstSrcFiles.size(); ++pos)
        {
            if (ttStrStrI(m_lstSrcFiles[pos].c_str(), ".c"))  // only add C/C++ files
            {
                cszSrcFile.printf(
                    "    <ClCompile Include=%kq>\n      <Filter>Source Files</Filter>\n    </ClCompile>",
                    m_lstSrcFiles[pos].c_str());
                kf.WriteEol(cszSrcFile);
            }
        }
        kf.WriteEol("  </ItemGroup>");
        kf.WriteEol("</Project>");
        cszProjVC += ".filters";
        if (!kf.WriteFile(cszProjVC))
        {
            ttCStr cszMsg;
            cszMsg.printf("Unable to write to %s", (char*) cszProjVC);
            AddError(cszMsg);
            return false;
        }
        else
            printf("Created %s\n", (char*) cszProjVC);
    }
    return true;
}
