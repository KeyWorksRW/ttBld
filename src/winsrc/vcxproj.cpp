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
#include <ttfindfile.h>  // ttCFindFile
#include <ttwinff.h>     // winff -- Wrapper around Windows FindFile

#include "resource.h"
#include "vcxproj.h"  // CVcxRead, CVcxWrite

CVcxRead::CVcxRead(ttCParseXML* pxml, CWriteSrcFiles* pcSrcFiles, std::string_view ConvertScript)
{
    m_pxml = pxml;
    m_pcSrcFiles = pcSrcFiles;
    m_ConvertScript = ConvertScript;
}

bool CVcxRead::ConvertVcxProj()
{
    ttCXMLBranch* pProject = m_pxml->GetRootBranch()->FindFirstElement("Project");
    if (!pProject)
    {
        ttlib::MsgBox(_tt("Cannot locate <Project> in ") + m_ConvertScript);
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
            ttlib::cstr cszFlags("-D");
            cszFlags += pChild->GetData();
            cszFlags.Replace("_DEBUG;", "");
            cszFlags.Replace("NDEBUG;", "");
            cszFlags.Replace(";%(PreprocessorDefinitions)", "");
            cszFlags.Replace(";", " -D", true);
            m_pcSrcFiles->setOptValue(bDebug ? OPT::MIDL_DBG : OPT::MIDL_REL, cszFlags);
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
            ttlib::cstr cszFlags("-D");
            cszFlags += pChild->GetData();
            cszFlags.Replace("_DEBUG;", "");
            cszFlags.Replace("NDEBUG;", "");
            cszFlags.Replace(";%(PreprocessorDefinitions)", "");
            cszFlags.Replace(";", " -D", true);
            m_pcSrcFiles->setOptValue(bDebug ? OPT::RC_DBG : OPT::RC_REL, cszFlags);
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
                m_pcSrcFiles->setBoolOptValue(
                    OPT::OPTIMIZE,
                    ttlib::issameas(pChild->GetData(), "size" ? "space" : "speed", tt::CASE::either));
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
                ttlib::cstr cszFlags(ttlib::issameprefix(pChild->GetData(), "$(OutDir", tt::CASE::either) &&
                                             pszFirstSemi ?
                                         pszFirstSemi :
                                         pChild->GetData());
                cszFlags.Replace(";%(AdditionalIncludeDirectories)", "");

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
            ttlib::cstr cszFlags("-D");
            cszFlags += pChild->GetData();
            cszFlags.Replace("_DEBUG;", "");
            cszFlags.Replace("NDEBUG;", "");
            cszFlags.Replace(";%(PreprocessorDefinitions)", "");
            cszFlags.Replace(";", " -D", true);
            m_pcSrcFiles->setOptValue(bDebug ? OPT::CFLAGS_DBG : OPT::CFLAGS_REL, cszFlags);
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
            ttlib::cstr cszCurLibs;
            for (auto iter: enumLibs)
            {
                // We only add libraries that are relative to our project
                if (tt::issamesubstr(iter, ".."))
                {
                    if (!cszCurLibs.empty())
                        cszCurLibs += ";";
                    cszCurLibs += iter.c_str();
                }
            }
            if (!cszCurLibs.empty())
                m_pcSrcFiles->setOptValue(bDebug ? OPT::LIBS_DBG : OPT::LIBS_REL, cszCurLibs);
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

    cszResult.assign(m_ConvertScript);
    cszResult.remove_filename();
    cszResult.append_filename(pszDir);

    cszResult.make_absolute();

    ttlib::cstr cwd;
    cszResult.make_relative(cwd.assignCwd());
}

// This function first converts the file relative to the location of the build script, and then relative to the
// location of .srcfiles

const ttlib::cstr& CVcxRead::MakeSrcRelative(const char* pszFile)
{
    if (m_cszScriptRoot.empty())
    {
        m_cszScriptRoot = m_ConvertScript;
        m_cszScriptRoot.make_absolute();
        m_cszScriptRoot.remove_filename();

        // For GetFullPathName() to work properly on a file inside the script, we need to be in the same directory
        // as the script file

        ttlib::ChangeDir(m_cszScriptRoot);
    }

    if (m_cszOutRoot.empty())
    {
        m_cszOutRoot.assignCwd();
        m_cszOutRoot.make_absolute();
    }

    m_cszRelative = pszFile;
    m_cszRelative.make_absolute();
    m_cszRelative.make_relative(m_cszRelative);
    return m_cszRelative;
}

static bool CreateGuid(ttlib::cstr& Result)
{
    UUID uuid;
    RPC_STATUS ret_val = ::UuidCreate(&uuid);

    if (ret_val == RPC_S_OK)
    {
        RPC_CSTR pszUuid = nullptr;
        if (::UuidToStringA(&uuid, &pszUuid) == RPC_S_OK && pszUuid)
        {
            Result = (char*) pszUuid;
            ::RpcStringFreeA(&pszUuid);
        }
    }
    return !Result.empty();
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

    ttlib::cstr cszGuid;
    if (!CreateGuid(cszGuid))
    {
        AddError(_tt("Unable to create a UUID -- cannot create .vcxproj without it."));
        return false;
    }

    ttlib::cstr cszProjVC(GetProjectName());
    cszProjVC.replace_extension(".vcxproj");
    if (!cszProjVC.fileExists())
    {
        ttlib::cstr master = std::move(ttlib::LoadTextResource(IDR_VCXPROJ_MASTER));

        master.Replace("%guid%", cszGuid, true);
        master.Replace("%%DebugExe%", GetTargetDebug(), true);
        master.Replace("%%ReleaseExe%", GetTargetRelease(), true);
        master.Replace("%%DebugExe64%", GetTargetDebug(), true);
        master.Replace("%%ReleaseExe64%", GetTargetRelease(), true);

        ttlib::textfile out;
        out.ReadString(master);

        for (auto& file: m_lstSrcFiles)
        {
            auto ext = file.extension();
            if (ext.empty() || std::tolower(ext[1] != 'c'))
                continue;
            out.emplace_back(" <ItemGroup>");
            out.addEmptyLine().Format("    <ClCompile Include=%ks />", file.c_str());
            out.emplace_back(" </ItemGroup>");
        }

        ttlib::cstr cszSrcFile;
        if (!m_RCname.empty())
        {
            out.emplace_back(" <ItemGroup>");
            out.addEmptyLine().Format("    <ResourceCompile Include=%ks />", m_RCname.c_str());
            out.emplace_back(" </ItemGroup>");
        }

        ttlib::winff ff("*.h");  // add all header files in current directory
        if (ff.isvalid())
        {
            do
            {
                out.emplace_back(" <ItemGroup>");
                out.addEmptyLine().Format("    <ClInclude Include=%ks />", ff.c_str());
                out.emplace_back(" </ItemGroup>");
            } while (ff.next());
        }

        out.emplace_back("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />");
        out.emplace_back("  <ImportGroup Label=\"ExtensionTargets\">");
        out.emplace_back("  </ImportGroup>");
        out.emplace_back("</Project>");

        if (!out.WriteFile(cszProjVC))
        {
            std::string msg = _tt("Unable to write to ");
            msg += cszProjVC.c_str();
            AddError(msg);
            return false;
        }
        else
            std::cout << _tt("Created ") << cszProjVC << '\n';

        master = ttlib::LoadTextResource(IDR_VCXPROJ_FILTERS);

        CreateGuid(cszGuid);  // it already succeeded once if we got here, so we don't check for error again
        master.Replace("%guidSrc%", cszGuid, true);
        CreateGuid(cszGuid);
        master.Replace("%guidHdr%", cszGuid, true);
        CreateGuid(cszGuid);
        master.Replace("%guidResource%", cszGuid, true);

        out.clear();
        out.ReadString(master);

        out.emplace_back("  <ItemGroup>");

        for (size_t pos = 0; pos < m_lstSrcFiles.size(); ++pos)
        {
            if (m_lstSrcFiles[pos].contains(".c"))  // only add C/C++ files
            {
                out.addEmptyLine().Format("    <ClCompile Include=%ks>", m_lstSrcFiles[pos].c_str());
                out.emplace_back("      <Filter>Source Files</Filter>");
                out.emplace_back("    </ClCompile>");
                out.emplace_back(cszSrcFile);
            }
        }
        out.emplace_back("  </ItemGroup>");
        out.emplace_back("</Project>");
        cszProjVC += ".filters";
        if (!out.WriteFile(cszProjVC))
        {
            std::string msg = _tt("Unable to write to ");
            msg += cszProjVC.c_str();
            AddError(msg);
            return false;
        }
        else
            std::cout << _tt("Created ") << cszProjVC << '\n';
    }
    return true;
}
