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

#include <ttenumstr.h>   // ttlib::enumstr, ttEnumView -- Enumerate through substrings in a string
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
                        m_pcSrcFiles->GetSrcFileList().addfilename(MakeSrcRelative(pszFile));
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
                            m_pcSrcFiles->setOptValue(OPT::EXE_TYPE, "dll");
                        else if (ttIsSameStrI(pChild->GetData(), "StaticLibrary"))
                            m_pcSrcFiles->setOptValue(OPT::EXE_TYPE, "lib");
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
                        if (!m_pcSrcFiles->hasOptValue(OPT::TARGET_DIR64) && pChild->cChildren > 0)
                        {
                            m_pcSrcFiles->setOptValue(OPT::TARGET_DIR64, pChild->GetChildAt(0)->GetData());
                            m_pcSrcFiles->setOptValue(OPT::TARGET_DIR32, pChild->GetChildAt(0)->GetData());
                        }
                    }
                    else if (ttIsSameSubStrI(pChild->GetName(), "TargetName"))
                    {
                        if (pChild->GetAttributeAt(0)->pszValue &&
                            ttStrStrI(pChild->GetAttributeAt(0)->pszValue, "Release"))
                        {
                            if (!m_pcSrcFiles->hasOptValue(OPT::PROJECT) && pChild->cChildren > 0)
                            {
                                m_pcSrcFiles->setOptValue(OPT::PROJECT, pChild->GetChildAt(0)->GetData());
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

    if (m_pcSrcFiles->getRcName().empty())
    {
        if (m_pcSrcFiles->hasOptValue(OPT::RC_CMN))
            m_pcSrcFiles->setOptValue(OPT::RC_CMN, "");
        if (m_pcSrcFiles->hasOptValue(OPT::RC_REL))
            m_pcSrcFiles->setOptValue(OPT::RC_REL, "");
        if (m_pcSrcFiles->hasOptValue(OPT::RC_DBG))
            m_pcSrcFiles->setOptValue(OPT::RC_DBG, "");
    }

    // If Debug and Release flags are the same, then remove them and just use the common flag setting

    if (m_pcSrcFiles->hasOptValue(OPT::CFLAGS_REL) && m_pcSrcFiles->hasOptValue(OPT::CFLAGS_DBG) &&
        m_pcSrcFiles->getOptValue(OPT::CFLAGS_REL).issameas(m_pcSrcFiles->getOptValue(OPT::CFLAGS_DBG)))
    {
        m_pcSrcFiles->setOptValue(OPT::CFLAGS_CMN, m_pcSrcFiles->getOptValue(OPT::CFLAGS_REL));
        m_pcSrcFiles->setOptValue(OPT::CFLAGS_REL, "");
        m_pcSrcFiles->setOptValue(OPT::CFLAGS_DBG, "");
    }
    if (m_pcSrcFiles->hasOptValue(OPT::MIDL_REL) && m_pcSrcFiles->hasOptValue(OPT::MIDL_DBG) &&
        m_pcSrcFiles->getOptValue(OPT::MIDL_REL).issameas(m_pcSrcFiles->getOptValue(OPT::MIDL_DBG)))
    {
        m_pcSrcFiles->setOptValue(OPT::MIDL_CMN, m_pcSrcFiles->getOptValue(OPT::MIDL_REL));
        m_pcSrcFiles->setOptValue(OPT::MIDL_REL, "");
        m_pcSrcFiles->setOptValue(OPT::MIDL_DBG, "");
    }
    if (m_pcSrcFiles->hasOptValue(OPT::RC_REL) && m_pcSrcFiles->hasOptValue(OPT::RC_DBG) &&
        m_pcSrcFiles->getOptValue(OPT::RC_REL).issameas(m_pcSrcFiles->getOptValue(OPT::RC_DBG)))
    {
        m_pcSrcFiles->setOptValue(OPT::RC_CMN, m_pcSrcFiles->getOptValue(OPT::RC_REL));
        m_pcSrcFiles->setOptValue(OPT::RC_REL, "");
        m_pcSrcFiles->setOptValue(OPT::RC_DBG, "");
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
            m_pcSrcFiles->setOptValue(bDebug ? OPT::MIDL_DBG : OPT::MIDL_REL, (char*) cszFlags);
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
            m_pcSrcFiles->setOptValue(bDebug ? OPT::RC_DBG : OPT::RC_REL, (char*) cszFlags);
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
            ttlib::enumstr enumFlags(pChild->GetData(), ' ');
            ttlib::cstr CFlags;
            if (m_pcSrcFiles->hasOptValue(OPT::CFLAGS_CMN))
                CFlags = m_pcSrcFiles->getOptValue(OPT::CFLAGS_CMN);
            for (auto iter: enumFlags)
            {
                if (ttIsSameSubStrI(iter.c_str() + 1, "std:"))
                {
                    if (!CFlags.contains(iter, tt::CASE::either))
                    {
                        if (!CFlags.empty())
                            CFlags += " ";
                        CFlags += iter;
                    }
                }
            }
            if (!CFlags.empty())
                m_pcSrcFiles->setOptValue(OPT::CFLAGS_CMN, CFlags);
        }
    }
    if (!bDebug)
    {
        pFlags = pSection->FindFirstElement("FavorSizeOrSpeed");
        if (pFlags && pFlags->GetChildrenCount() > 0)
        {
            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
            if (pChild->GetData())
            {
                m_pcSrcFiles->setOptValue(OPT::OPTIMIZE,
                                          ttlib::issameas(pChild->GetData(), "size" ? "space" : "speed",
                                          tt::CASE::either));
            }
        }
    }
    if (!m_pcSrcFiles->hasOptValue(OPT::INC_DIRS))
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

                ttlib::enumstr enumPaths(cszFlags.c_str());
                ttlib::cstr Include;
                for (auto& iter: enumPaths)
                {
                    ttlib::cstr cszTmp;
                    ConvertScriptDir(iter.c_str(), cszTmp);
                    if (!Include.empty())
                        Include += ";";
                    Include += cszTmp;
                }

                m_pcSrcFiles->setOptValue(OPT::INC_DIRS, Include);
            }
        }
    }
    if (!m_pcSrcFiles->hasOptValue(OPT::PCH))
    {
        pFlags = pSection->FindFirstElement("PrecompiledHeaderFile");
        if (pFlags && pFlags->GetChildrenCount() > 0)
        {
            ttCXMLBranch* pChild = pFlags->GetChildAt(0);
            if (pChild->GetData())
                m_pcSrcFiles->setOptValue(OPT::PCH, pChild->GetData());
        }
    }
    if (!m_pcSrcFiles->hasOptValue(OPT::WARN))
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
                m_pcSrcFiles->setOptValue(OPT::WARN, pszTmp);
            }
        }
    }

#if 0
// REVIEW: [KeyWorks - 03-06-2020] Calling convention is only useful for 32-bit apps.
    pFlags = pSection->FindFirstElement("CallingConvention");
    if (pFlags && pFlags->GetChildrenCount() > 0)
    {
        ttCXMLBranch* pChild = pFlags->GetChildAt(0);
        if (pChild->GetData() && ttStrStrI(pChild->GetData(), "stdcall"))
        {
            m_pcSrcFiles->setOptValue(OPT_STDCALL, true);
        }
    }
#endif

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
            m_pcSrcFiles->setOptValue(bDebug ? OPT::CFLAGS_DBG : OPT::CFLAGS_REL, (char*) cszFlags);
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
            ttlib::enumstr enumLibs(pChild->GetData());
            ttCStr cszCurLibs;
            for (auto iter: enumLibs)
            {
                // We only add libraries that are relative to our project
                if (tt::issamesubstr(iter, ".."))
                {
                    if (cszCurLibs.IsNonEmpty())
                        cszCurLibs += ";";
                    cszCurLibs += iter.c_str();
                }
            }
            if (cszCurLibs.IsNonEmpty())
                m_pcSrcFiles->setOptValue(bDebug ? OPT::LIBS_DBG : OPT::LIBS_REL, (char*) cszCurLibs);
        }
    }
}

void CVcxRead::ConvertScriptDir(const char* pszDir, ttlib::cstr& cszResult)
{
    if (pszDir[1] == ':')
    {
        // If the path starts with a drive letter, then we can't make it relative
        cszResult = pszDir;
        return;
    }

    cszResult.assign(m_pcszConvertScript->c_str());
    cszResult.remove_filename();
    cszResult.append_filename(pszDir);

    cszResult.make_absolute();

    ttlib::cstr cwd;
    cszResult.make_relative(cwd.assignCwd());
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
    UUID uuid;
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
        AddError(_tt("Unable to create a UUID -- cannot create .vcxproj without it."));
        return false;
    }

    ttCStr cszProjVC(GetProjectName().c_str());
    cszProjVC.ChangeExtension(".vcxproj");
    if (!ttFileExists(cszProjVC))
    {
        ttCFile kf;
        kf.ReadResource(IDR_VCXPROJ_MASTER);
        while (kf.ReplaceStr("%guid%", cszGuid))
            ;
        while (kf.ReplaceStr("%%DebugExe%", GetTargetDebug().c_str()))
            ;
        while (kf.ReplaceStr("%%ReleaseExe%", GetTargetRelease().c_str()))
            ;
        while (kf.ReplaceStr("%%DebugExe64%", GetTargetDebug().c_str()))
            ;
        while (kf.ReplaceStr("%%ReleaseExe64%", GetTargetRelease().c_str()))
            ;

        for (auto& file: m_lstSrcFiles)
        {
            auto ext = file.extension();
            if (ext.empty() || std::tolower(ext[1] != 'c'))
                continue;
            kf.WriteEol(
                (" <ItemGroup>\n    <ClCompile Include=%kq />\n  </ItemGroup>" + file).c_str());
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
            std::string msg = _tt("Unable to write to ");
            msg += cszProjVC.c_str();
            AddError(msg);
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
            std::string msg = _tt("Unable to write to ");
            msg += cszProjVC.c_str();
            AddError(msg);
            return false;
        }
        else
            printf("Created %s\n", (char*) cszProjVC);
    }
    return true;
}
