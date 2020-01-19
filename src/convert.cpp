/////////////////////////////////////////////////////////////////////////////
// Name:      convert.cpp
// Purpose:   Various conversion methods
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

// Assumes that everything except a .DSP file is an xml file and has been parsed into m_xml

#include "pch.h"

#include <ttTR.h>  // Function for translating strings

#include <ttenumstr.h>  // ttCEnumStr -- Enumerate through substrings in a string
#include <ttfile.h>     // ttCFile -- class for reading and writing files, strings, data, etc.
#include <ttxml.h>      // ttCXMLBranch

#include "convertdlg.h"  // CConvertDlg
#include "vcxproj.h"     // CVcxRead

void ConvertScriptDir(const char* pszScript, const char* pszDir, ttCStr& cszResult);

bool CConvertDlg::ConvertCodeBlocks()
{
    if (!ttFileExists(m_cszConvertScript))
    {
        ttMsgBoxFmt(_tt("Cannot open \"%s\"."), MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
        return false;
    }

    ttCXMLBranch* pProject = m_xml.GetRootBranch()->FindFirstElement("Project");
    if (!pProject)
    {
        ttMsgBoxFmt(_tt("Cannot locate <Project> in %s"), MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
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
                m_cSrcFiles.GetSrcFilesList().addfile(MakeSrcRelative(pItem->GetAttribute("filename")));
        }
    }

    return true;
}

bool CConvertDlg::ConvertCodeLite()
{
    ttCXMLBranch* pProject = m_xml.GetRootBranch()->FindFirstElement("CodeLite_Project");
    if (!pProject)
    {
        ttMsgBoxFmt(_tt("Cannot locate <CodeLite_Project> in %s"), MB_OK | MB_ICONWARNING,
                    (char*) m_cszConvertScript);
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
        ttMsgBoxFmt(_tt("Cannot locate <VisualStudioProject> in %s"), MB_OK | MB_ICONWARNING,
                    (char*) m_cszConvertScript);
        return false;
    }

    if (pProject->GetAttribute("Name"))
        m_cSrcFiles.UpdateOption(OPT_PROJECT, pProject->GetAttribute("Name"));

    ttCXMLBranch* pFiles = pProject->FindFirstElement("Files");
    if (!pFiles)
    {
        ttMsgBoxFmt(_tt("Cannot locate <Files> in %s"), MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
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
                        m_cSrcFiles.GetSrcFilesList().addfile(MakeSrcRelative(pItem->GetAttribute("RelativePath")));
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
                    m_cSrcFiles.GetSrcFilesList().addfile(MakeSrcRelative(pItem->GetAttribute("RelativePath")));
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
            ttCStr cszOutDir(pOption->GetAttribute(
                "OutputFile"));  // will typically be something like: "../bin/$(ProjectName).exe"
            char*  pszFile = ttFindFilePortion(cszOutDir);
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
                ttCStr cszOrgInc(m_cSrcFiles.GetOption(OPT_INC_DIRS));

                // Paths will be relative to the location of the script file. We need to make them
                // relative to .srcfiles.yaml.

                ttCEnumStr cszPaths(pszOption);
                ttCStr     cszInc, cszTmp;

                while (cszPaths.Enum())
                {
                    ConvertScriptDir(m_cszConvertScript, cszPaths, cszTmp);
                    if (cszInc.IsNonEmpty())
                        cszInc += ";";
                    cszInc += cszTmp;
                }
                if (cszOrgInc.IsNonEmpty())
                    cszOrgInc += ";";
                cszOrgInc += cszInc;

                m_cSrcFiles.UpdateOption(OPT_INC_DIRS, (char*) cszOrgInc);
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
                    if (ttIsSameStrI(
                            enumFlags,
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
    CVcxRead vcx(&m_xml, &m_cSrcFiles, &m_cszConvertScript);
    return vcx.ConvertVcxProj();
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
        ttMsgBoxFmt(_tt("Cannot open \"%s\"."), MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
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
                m_cSrcFiles.GetSrcFilesList().append(pszFile);
            }
        }
    }
    return true;
}

bool CConvertDlg::ConvertSrcfiles()
{
    CSrcFiles srcOrg;
    if (!srcOrg.ReadFile(m_cszConvertScript))
    {
        ttMsgBoxFmt(_tt("Cannot open \"%s\"."), MB_OK | MB_ICONWARNING, (char*) m_cszConvertScript);
        return false;
    }

    // First we copy all the options. Then we tweak those that will need to be changed to use a different path.

    m_cSrcFiles.UpdateOption(OPT_PROJECT, srcOrg.GetOption(OPT_PROJECT));
    m_cSrcFiles.UpdateOption(OPT_EXE_TYPE, srcOrg.GetOption(OPT_EXE_TYPE));
    m_cSrcFiles.UpdateOption(OPT_PCH, srcOrg.GetOption(OPT_PCH));

    m_cSrcFiles.UpdateOption(OPT_OPTIMIZE, srcOrg.GetOption(OPT_OPTIMIZE));
    m_cSrcFiles.SetRequired(OPT_OPTIMIZE);
    m_cSrcFiles.UpdateOption(OPT_WARN_LEVEL, srcOrg.GetOption(OPT_WARN_LEVEL));
    m_cSrcFiles.SetRequired(OPT_WARN_LEVEL);

    m_cSrcFiles.UpdateOption(OPT_PERMISSIVE, srcOrg.GetOption(OPT_PERMISSIVE));
    m_cSrcFiles.UpdateOption(OPT_STDCALL, srcOrg.GetOption(OPT_STDCALL));

    m_cSrcFiles.UpdateOption(OPT_CFLAGS_CMN, srcOrg.GetOption(OPT_CFLAGS_CMN));
    m_cSrcFiles.UpdateOption(OPT_CFLAGS_REL, srcOrg.GetOption(OPT_CFLAGS_REL));
    m_cSrcFiles.UpdateOption(OPT_CFLAGS_DBG, srcOrg.GetOption(OPT_CFLAGS_DBG));

    m_cSrcFiles.UpdateOption(OPT_CLANG_CMN, srcOrg.GetOption(OPT_CLANG_CMN));
    m_cSrcFiles.UpdateOption(OPT_CLANG_REL, srcOrg.GetOption(OPT_CLANG_REL));
    m_cSrcFiles.UpdateOption(OPT_CLANG_DBG, srcOrg.GetOption(OPT_CLANG_DBG));

    m_cSrcFiles.UpdateOption(OPT_LINK_CMN, srcOrg.GetOption(OPT_LINK_CMN));
    m_cSrcFiles.UpdateOption(OPT_LINK_REL, srcOrg.GetOption(OPT_LINK_REL));
    m_cSrcFiles.UpdateOption(OPT_LINK_DBG, srcOrg.GetOption(OPT_LINK_DBG));

#if defined(_WIN32)
    m_cSrcFiles.UpdateOption(OPT_NATVIS, srcOrg.GetOption(OPT_NATVIS));

    m_cSrcFiles.UpdateOption(OPT_RC_CMN, srcOrg.GetOption(OPT_RC_CMN));
    m_cSrcFiles.UpdateOption(OPT_RC_REL, srcOrg.GetOption(OPT_RC_REL));
    m_cSrcFiles.UpdateOption(OPT_RC_DBG, srcOrg.GetOption(OPT_RC_DBG));

    m_cSrcFiles.UpdateOption(OPT_MDL_CMN, srcOrg.GetOption(OPT_MDL_CMN));
    m_cSrcFiles.UpdateOption(OPT_MDL_REL, srcOrg.GetOption(OPT_MDL_REL));
    m_cSrcFiles.UpdateOption(OPT_MDL_DBG, srcOrg.GetOption(OPT_MDL_DBG));

    m_cSrcFiles.UpdateOption(OPT_DEBUG_RC, srcOrg.GetOption(OPT_DEBUG_RC));

    m_cSrcFiles.UpdateOption(OPT_MS_LINKER, srcOrg.GetOption(OPT_MS_LINKER));
    m_cSrcFiles.UpdateOption(OPT_MS_RC, srcOrg.GetOption(OPT_MS_RC));
#endif  // defined(_WIN32)

    m_cSrcFiles.UpdateOption(OPT_STATIC_CRT_REL, srcOrg.GetOption(OPT_STATIC_CRT_REL));
    m_cSrcFiles.UpdateOption(OPT_STATIC_CRT_DBG, srcOrg.GetOption(OPT_STATIC_CRT_DBG));

    m_cSrcFiles.UpdateOption(OPT_64BIT, srcOrg.GetOption(OPT_64BIT));
    m_cSrcFiles.UpdateOption(OPT_TARGET_DIR64, srcOrg.GetOption(OPT_TARGET_DIR64));
    m_cSrcFiles.UpdateOption(OPT_32BIT, srcOrg.GetOption(OPT_32BIT));
    m_cSrcFiles.UpdateOption(OPT_TARGET_DIR32, srcOrg.GetOption(OPT_TARGET_DIR32));

    m_cSrcFiles.UpdateOption(OPT_LIBS_CMN, srcOrg.GetOption(OPT_LIBS_CMN));
    m_cSrcFiles.UpdateOption(OPT_LIBS_REL, srcOrg.GetOption(OPT_LIBS_REL));
    m_cSrcFiles.UpdateOption(OPT_LIBS_DBG, srcOrg.GetOption(OPT_LIBS_DBG));

    m_cSrcFiles.UpdateOption(OPT_XGET_FLAGS, srcOrg.GetOption(OPT_XGET_FLAGS));

    ttCStr cszRoot(m_cszConvertScript);
    char*  pszTmp = ttFindFilePortion(cszRoot);
    if (pszTmp)
        *pszTmp = 0;
    // pszTmp = ttFindLastSlash(cszRoot);
    // if (pszTmp)
    // *pszTmp = 0;

    ttCStr cszRelative;
    ttCStr cszCurCwd;
    cszCurCwd.GetCWD();
    ttChDir(cszRoot);

    if (srcOrg.GetOption(OPT_PCH))
    {
        ttCStr cszPch(srcOrg.GetOption(OPT_PCH_CPP));
        if (cszPch.IsEmpty() || ttIsSameStrI(cszPch, "none"))
        {
            cszPch = srcOrg.GetOption(OPT_PCH);
            cszPch.ChangeExtension(".cpp");
            if (!ttFileExists(cszPch))
            {
                cszPch.ChangeExtension(".cc");
                if (!ttFileExists(cszPch))
                {
                    cszPch.ChangeExtension(".cxx");
                    if (!ttFileExists(cszPch))
                        cszPch.ChangeExtension(".cpp");  // File can't be found, switch back to original
                }
            }
            cszPch.FullPathName();
        }
        else
        {
            cszPch = (char*) cszRoot;
            cszPch.AppendFileName(srcOrg.GetOption(OPT_PCH_CPP));
        }

        ttConvertToRelative(cszCurCwd, cszPch, cszRelative);
        m_cSrcFiles.UpdateOption(OPT_PCH_CPP, (char*) cszRelative);
    }

    ttConvertToRelative(cszCurCwd, m_cszConvertScript, cszRelative);

    ttCStr cszTmp(".include ");
    cszTmp += (char*) cszRelative;

    m_cSrcFiles.GetSrcFilesList() += cszTmp.c_str();

    pszTmp = ttFindFilePortion(cszRelative);
    if (pszTmp)
        *pszTmp = 0;
    pszTmp = ttFindLastSlash(cszRelative);
    if (pszTmp)
        *pszTmp = 0;

    ttCStr cszIncDirs(cszRelative);
    if (srcOrg.GetOption(OPT_INC_DIRS))
    {
        ttCEnumStr enumStr(srcOrg.GetOption(OPT_INC_DIRS));
        ttChDir(cszRoot);
        while (enumStr.Enum())
        {
            cszTmp = (char*) enumStr;
            cszTmp.FullPathName();
            ttConvertToRelative(cszCurCwd, cszTmp, cszRelative);
            cszIncDirs += ";";
            cszIncDirs += (char*) cszRelative;
        }

        cszRelative += srcOrg.GetOption(OPT_INC_DIRS);
    }
    m_cSrcFiles.UpdateOption(OPT_INC_DIRS, (char*) cszIncDirs);

#if defined(_WIN32)
    if (srcOrg.GetOption(OPT_NATVIS))
    {
        cszTmp = srcOrg.GetOption(OPT_NATVIS);
        cszTmp.FullPathName();

        ttConvertToRelative(cszCurCwd, cszTmp, cszRelative);
        m_cSrcFiles.UpdateOption(OPT_NATVIS, (char*) cszRelative);
    }
#endif  // _WIN32

    if (srcOrg.GetOption(OPT_TARGET_DIR))
    {
        cszTmp = srcOrg.GetOption(OPT_TARGET_DIR);
        cszTmp.FullPathName();

        ttConvertToRelative(cszCurCwd, cszTmp, cszRelative);
        m_cSrcFiles.UpdateOption(OPT_TARGET_DIR, (char*) cszRelative);
    }

    if (srcOrg.GetOption(OPT_TARGET_DIR32))
    {
        cszTmp = srcOrg.GetOption(OPT_TARGET_DIR32);
        cszTmp.FullPathName();

        ttConvertToRelative(cszCurCwd, cszTmp, cszRelative);
        m_cSrcFiles.UpdateOption(OPT_TARGET_DIR32, (char*) cszRelative);
    }

    if (srcOrg.GetOption(OPT_LIB_DIRS))
    {
        cszTmp = srcOrg.GetOption(OPT_LIB_DIRS);
        cszTmp.FullPathName();

        ttConvertToRelative(cszCurCwd, cszTmp, cszRelative);
        m_cSrcFiles.UpdateOption(OPT_LIB_DIRS, (char*) cszRelative);
    }
    if (srcOrg.GetOption(OPT_LIB_DIRS32))
    {
        cszTmp = srcOrg.GetOption(OPT_LIB_DIRS32);
        cszTmp.FullPathName();

        ttConvertToRelative(cszCurCwd, cszTmp, cszRelative);
        m_cSrcFiles.UpdateOption(OPT_LIB_DIRS32, (char*) cszRelative);
    }

    if (srcOrg.GetOption(OPT_BUILD_LIBS))
    {
        cszTmp = srcOrg.GetOption(OPT_BUILD_LIBS);
        cszTmp.FullPathName();

        ttConvertToRelative(cszCurCwd, cszTmp, cszRelative);
        m_cSrcFiles.UpdateOption(OPT_BUILD_LIBS, (char*) cszRelative);
    }

    ttChDir(cszCurCwd);  // Restore our current directory

    return true;
}

void ConvertScriptDir(const char* pszScript, const char* pszDir, ttCStr& cszResult)
{
    if (pszDir[1] == ':')
    {
        // If the path starts with a drive letter, then we can't make it relative
        cszResult = pszDir;
        return;
    }

    ttCStr cszScript(pszScript);
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
