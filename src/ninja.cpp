/////////////////////////////////////////////////////////////////////////////
// Name:      CNinja
// Purpose:   Class for creating/maintaining build.ninja file for use by ninja.exe build tool
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

// This will create one build?.ninja for each build target (debug32, debug64, release32, release64). Each build file will
// be placed in a build directory with a compiler prefix (build/clangBuild.ninja or build/msvcBuild.ninja). The build
// directory will also hold the dependency files that ninja creates.

#include "pch.h"

#include <ttfile.h>     // ttCFile
#include <ttenumstr.h>  // ttCEnumStr

#include "ninja.h"      // CNinja
#include "parsehhp.h"   // CParseHHP
#include "verninja.h"   // CVerMakeNinja

const char* aCppExt[] = {
    ".cpp",
    ".cxx",
    ".cc",

    nullptr
};

CNinja::CNinja(const char* pszNinjaDir) : CSrcFiles(pszNinjaDir),
    // make all ttCList classes use the same sub-heap
    m_lstBuildLibs32D(m_ttHeap), m_lstBuildLibs64D(m_ttHeap),
    m_lstBuildLibs32R(m_ttHeap), m_lstBuildLibs64R(m_ttHeap)
{
#if defined(_DEBUG)
    ttASSERT(ReadFile());
#else
    if (!ReadFile())
        return;
#endif
    m_bForceWrite = false;

    CVerMakeNinja verSrcFiles;
    m_bInvalidVersion = verSrcFiles.IsSrcFilesNewer(GetMajorRequired(), GetMinorRequired(), GetSubRequired());

    // If no platform was specified, default to 64-bit

    if (!GetBoolOption(OPT_64BIT) && !GetBoolOption(OPT_32BIT))
    {
        UpdateOption(OPT_64BIT, true);          // default to 64-bit build
        UpdateOption(OPT_64BIT_SUFFIX, false);  // no suffix
    }

    // Set default target directories if they are missing

    if (GetBoolOption(OPT_64BIT) && !GetDir64())
    {
        ttCStr cszCWD;
        cszCWD.GetCWD();
        bool bSrcDir = ttStrStrI(ttFindFilePortion(cszCWD), "src") ? true : false;
        if (!bSrcDir)
        {
            cszCWD.AppendFileName(IsExeTypeLib() ? "../lib" : "../bin");
            if (ttDirExists(cszCWD))
                bSrcDir = true;
        }
        if (bSrcDir)
        {
            ttCStr cszDir64(IsExeTypeLib() ? "../lib" : "../bin");
            ttCStr cszTmp(cszDir64);
            cszTmp += "64";
            if (ttDirExists(cszTmp))        // if there is a ../lib64 or ../bin64, then use that
                cszDir64 = cszTmp;
            UpdateOption(OPT_TARGET_DIR64, (char*) cszDir64);
        }
        else
        {
            ttCStr cszDir64(IsExeTypeLib() ? "lib" : "bin");
            ttCStr cszTmp(cszDir64);
            cszTmp += "64";
            if (ttDirExists(cszTmp))        // if there is a ../lib64 or ../bin64, then use that
                cszDir64 = cszTmp;
            UpdateOption(OPT_TARGET_DIR64, (char*) cszDir64);
        }
    }

    if (GetBoolOption(OPT_32BIT) && !GetDir32())
    {
        ttCStr cszCWD;
        cszCWD.GetCWD();
        bool bSrcDir = ttStrStrI(ttFindFilePortion(cszCWD), "src") ? true : false;
        if (!bSrcDir)
        {
            cszCWD.AppendFileName(IsExeTypeLib() ? "../lib" : "../bin");
            if (ttDirExists(cszCWD))
                bSrcDir = true;
        }
        if (bSrcDir)
        {
            ttCStr cszDir32(IsExeTypeLib() ? "../lib" : "../bin");
            ttCStr cszTmp(cszDir32);
            cszTmp += "32";
            if (ttDirExists(cszTmp))        // if there is a ../lib32 or ../bin32, then use that
                cszDir32 = cszTmp;
            UpdateOption(OPT_TARGET_DIR32, (char*) cszDir32);
        }
        else
        {
            ttCStr cszDir32(IsExeTypeLib() ? "lib" : "bin");
            ttCStr cszTmp(cszDir32);
            cszTmp += "32";
            if (ttDirExists(cszTmp))        // if there is a ../lib32 or ../bin32, then use that
                cszDir32 = cszTmp;
            UpdateOption(OPT_TARGET_DIR32, (char*) cszDir32);
        }
    }

    if (!GetProjectName())
    {
        ttCStr cszCwd;
        cszCwd.GetCWD();
        char* pszTmp = (char*) cszCwd.FindLastSlash();
        if (!pszTmp[1])     // if path ends with a slash, remove it -- we need that last directory name
            *pszTmp = 0;

        char* pszProj = ttFindFilePortion(cszCwd);
        if (ttIsSameStrI(pszProj, "src")) // Use the parent folder for the root if the current directory is "src"
        {
            pszTmp = (char*) cszCwd.FindLastSlash();
            if (pszTmp)
            {
                *pszTmp = 0;    // remove the last slash and filename, forcing the directory name above to be the "filename"
                pszProj = ttFindFilePortion(cszCwd);
            }
        }
        UpdateOption(OPT_PROJECT, pszProj);
    }

    m_lstRcDependencies.SetFlags(ttCList::FLG_URL_STRINGS);

    if (m_cszRcName.IsNonEmpty())
        FindRcDependencies(m_cszRcName);

    m_bBin64Exists = ttStrStr(GetDir64(), "64");

    ProcessBuildLibs();
}

static const char* aszCompilerPrefix[] =
{
    "msvc",
    "clang-cl",
    "clang",
    "gcc",
};

bool CNinja::CreateBuildFile(GEN_TYPE gentype, CMPLR_TYPE cmplr)
{
    ttCFile file;
    m_pkfOut = &file;
    m_gentype = gentype;

    // Note that resout goes to the same directory in all builds. The actual filename will have a 'D' appended for debug
    // builds. Currently, 32 and 64 bit builds of the resource file are identical.

    ttCStr cszResOut("resout = ");
    cszResOut += GetBldDir();
    cszResOut.AddTrailingSlash();
    cszResOut.AppendFileName("res");

    ttCStr cszBuildDir("builddir = ");
    cszBuildDir += GetBldDir();

    ttCStr cszOutDir("outdir = ");
    cszOutDir += GetBldDir();
    cszOutDir.AddTrailingSlash();

    m_cszScriptFile = GetBldDir();
    m_cszScriptFile.AddTrailingSlash();

    switch (gentype)
    {
        case GEN_DEBUG32:
            cszOutDir += aszCompilerPrefix[cmplr];
            cszOutDir += "Debug32";
            m_cszScriptFile += aszCompilerPrefix[cmplr];
            m_cszScriptFile += "Build32D.ninja";
            break;

        case GEN_DEBUG64:
            cszOutDir += aszCompilerPrefix[cmplr];
            cszOutDir += "Debug64";
            m_cszScriptFile += aszCompilerPrefix[cmplr];
            m_cszScriptFile += "Build64D.ninja";
            break;

        case GEN_RELEASE32:
            cszOutDir += aszCompilerPrefix[cmplr];
            cszOutDir += "Release32";
            m_cszScriptFile += aszCompilerPrefix[cmplr];
            m_cszScriptFile += "Build32.ninja";
            break;

        case GEN_RELEASE64:
        default:
            cszOutDir += aszCompilerPrefix[cmplr];
            cszOutDir += "Release64";
            m_cszScriptFile += aszCompilerPrefix[cmplr];
            m_cszScriptFile += "Build64.ninja";
            break;
    }

    ttCStr cszProj(GetProjectName());   // If needed, add a suffix to the project name

    // Don't add the 'D' to the end of DLL's -- it is perfectly viable for a release app to use a debug dll and that
    // won't work if the filename has changed. Under MSVC Linker, it will also generate a LNK4070 error if the dll name
    // is specified in the matching .def file. The downside, of course, is that without the 'D' only the size of the dll
    // indicates if it is a debug or release version.

    if (IsExeTypeDll())
    {
        if (GetBoolOption(OPT_64BIT_SUFFIX))
            cszProj += "64";
        else if (GetBoolOption(OPT_32BIT_SUFFIX))
            cszProj += "32";
    }
    else
    {
        if (gentype == GEN_DEBUG64 && GetBoolOption(OPT_64BIT_SUFFIX))
            cszProj += "64D";
        else if (gentype == GEN_RELEASE64 && GetBoolOption(OPT_64BIT_SUFFIX))
            cszProj += "64";
        else if (gentype == GEN_DEBUG32 && GetBoolOption(OPT_32BIT_SUFFIX))
            cszProj += "32D";
        else if (gentype == GEN_RELEASE32 && GetBoolOption(OPT_32BIT_SUFFIX))
            cszProj += "32";
    }

    file.SetUnixLF();   // WARNING!!! NINJA doesn't allow \r characters (or \t for that matter)
    file.printf("# WARNING: THIS FILE IS AUTO-GENERATED by %s. CHANGES YOU MAKE WILL BE LOST IF IT IS AUTO-GENERATED AGAIN.\n\n", txtVersion);
    file.WriteEol("ninja_required_version = 1.8\n");
    file.WriteEol(cszBuildDir);

    file.WriteEol(cszOutDir);
    file.WriteEol(cszResOut);

    file.WriteEol();

    // Figure out the filenames to use for the source and output for a precompiled header

    if (GetPchHeader())
    {
        m_cszPCH = GetProjectName();
        m_cszPCH.ChangeExtension(".pch");

        m_cszCPP_PCH = GetPchCpp();
        m_cszPCHObj = ttFindFilePortion(m_cszCPP_PCH);
        m_cszPCHObj.ChangeExtension(".obj");

        if (!ttFileExists(m_cszCPP_PCH))
        {
            ttCStr cszErrorMsg;
            cszErrorMsg.printf("No C++ source file found that matches %s -- precompiled header will not build correctly.\n", GetPchHeader());
            puts(cszErrorMsg);
        }
    }

    if (cmplr == CMPLR_MSVC || cmplr == CMPLR_CLANG_CL)
    {
        msvcWriteCompilerComments(cmplr);
        msvcWriteCompilerFlags(cmplr);
        msvcWriteCompilerDirectives(cmplr);
        msvcWriteRcDirective(cmplr);
        msvcWriteMidlDirective(cmplr);
        msvcWriteLibDirective(cmplr);
        msvcWriteLinkDirective(cmplr);
    }

    // Issue #80

    // If the project has a .idl file, then the midl compiler will create a matching header file that will be included in
    // one or more source files. If the .idl file changes, or the header file doesn't exist yet, then we need to run the
    // midl compiler before compiling any source files. If we knew ahead of time which source files included the header
    // file, then we could create a dependency. However, that would essentially require an accurate C/C++ preprocessor to
    // run on every source file which is far beyond the scope of this project. Instead, we add the dependency to the
    // precompiled header if there is one, and if not, we add the dependency to every source file. Unfortunately that
    // does mean that every time the .idl file changes, then every source file will get rebuilt whether or not a
    // particular source file actually uses the generated header file.

    if (GetPchHeader())
    {
        file.printf("build $outdir/%s: compilePCH %s", (char*) m_cszPCHObj, (char*) m_cszCPP_PCH);
        if (m_lstIdlFiles.GetCount())
        {
            file.WriteEol(" | $");
            size_t pos;
            ttCStr cszHdr;
            for (pos = 0; pos < m_lstIdlFiles.GetCount() - 1; ++pos)
            {
                cszHdr = m_lstIdlFiles[pos];
                cszHdr.ChangeExtension(".h");
                file.printf("  %s $\n", (char*) cszHdr);
            }
            cszHdr = m_lstIdlFiles[pos];
            cszHdr.ChangeExtension(".h");
            file.printf("  %s", (char*) cszHdr);    // write the last one without the trailing pipe
        }
        file.WriteEol("\n");
    }

    // Write the build rules for all source files

    for (size_t iPos = 0; iPos < GetSrcFileList()->GetCount(); iPos++)
    {
        ttCStr cszFile(ttFindFilePortion(GetSrcFileList()->GetAt(iPos)));
        if (!ttStrStrI(cszFile, ".c") || (m_cszCPP_PCH.IsNonEmpty() &&  ttIsSameStrI(cszFile, m_cszCPP_PCH)))   // we already handled resources and pre-compiled headers
            continue;   // we already handled this
        cszFile.ChangeExtension(".obj");

        if (m_cszPCHObj.IsNonEmpty())   // we add m_cszPCHObj so it appears as a dependency and gets compiled, but not linked to
            file.printf("build $outdir/%s: compile %s | $outdir/%s\n\n", (char*) cszFile, GetSrcFileList()->GetAt(iPos), (char*) m_cszPCHObj);
        else
        {
            // We get here if we don't have a precompiled header. We might have .idl files, which means we're going to
            // need to add all the midl-generated header files as dependencies to each source file. See issue #80 for details.

            file.printf("build $outdir/%s: compile %s", (char*) cszFile, GetSrcFileList()->GetAt(iPos));
            if (m_lstIdlFiles.GetCount())
            {
                file.WriteEol(" | $");
                size_t pos;
                ttCStr cszHdr;
                for (pos = 0; pos < m_lstIdlFiles.GetCount() - 1; ++pos)
                {
                    cszHdr = m_lstIdlFiles[pos];
                    cszHdr.ChangeExtension(".h");
                    file.printf("  %s $\n", (char*) cszHdr);
                }
                cszHdr = m_lstIdlFiles[pos];
                cszHdr.ChangeExtension(".h");
                file.printf("  %s", (char*) cszHdr);    // write the last one without the trailing pipe
            }
            file.WriteEol("\n");
        }
    }

    // Write the build rule for the resource compiler if an .rc file was specified as a source

    ttCStr cszRes;
    if (ttFileExists(GetRcFile()))
    {
        cszRes = GetRcFile();
        cszRes.RemoveExtension();
        cszRes += ((m_gentype == GEN_DEBUG32 || m_gentype == GEN_DEBUG64) ?  "D.res" : ".res");
        cszRes.ChangeExtension(".res");
        file.printf("build $resout/%s: rc %s", (char*) cszRes, GetRcFile());

        if (GetRcDepList()->GetCount())
            file.WriteStr(" |");
        for (size_t nPos = 0; nPos < GetRcDepList()->GetCount(); nPos++)
        {
            file.WriteStr(" $\n  ");
            file.WriteStr(GetRcDepList()->Get(nPos));
        }
        file.WriteEol("\n");
    }

    // Write the final build rules to complete the project

    if (cmplr == CMPLR_MSVC || cmplr == CMPLR_CLANG_CL)
    {
        msvcWriteMidlTargets(cmplr);
        msvcWriteLinkTargets(cmplr);
    }

    if (!ttDirExists(GetBldDir()))
    {
        if (!ttCreateDir(GetBldDir()))
        {
            ttCStr cszMsg;
            cszMsg.printf(GETSTRING(IDS_NINJA_CANT_WRITE), GetBldDir());
            AddError(cszMsg);
            return false;
        }
    }

    if (m_bForceWrite)
        return file.WriteFile(m_cszScriptFile);

    ttCFile fileOrg;
    if (fileOrg.ReadFile(m_cszScriptFile))
    {
        if (strcmp(fileOrg, file) == 0)    // Only write the build script if something changed
            return false;
        else if (m_dryrun.IsEnabled())
        {
            m_dryrun.NewFile(m_cszScriptFile);
            m_dryrun.DisplayFileDiff(fileOrg, file);
            return false;   // because we didn't write anything
        }
    }

    if (!file.WriteFile(m_cszScriptFile))
    {
        ttCStr cszMsg;
        cszMsg.printf(GETSTRING(IDS_NINJA_CANT_WRITE), (char*) m_cszScriptFile);
        cszMsg += "\n";
        AddError(cszMsg);
        return false;
    }

    return true;
}

void CNinja::AddDependentLibrary(const char* pszLib, GEN_TYPE gentype)
{
    ttCStr cszLib(pszLib);
    char* pszTmp = ttStrStrI(pszLib, ".lib");
    if (pszTmp)
        *pszTmp = 0;

    // Note that if the path to the library contains a "64" or "32" then the "64" suffix is not added. I.e., ../ttLib/lib64/ttLib will turn
    // into ttLibD.lib not ttLib64D.lib. Otherwise we always add a platform suffix (64 or 32).

    switch (gentype)
    {
        case GEN_DEBUG32:
            cszLib += !ttStrStr(cszLib, "32") ? "32D.lib" : "D.lib";
            break;
        case GEN_DEBUG64:
            cszLib += !ttStrStr(cszLib, "64") ? "64D.lib" : "D.lib";
            break;
        case GEN_RELEASE32:
            cszLib += !ttStrStr(cszLib, "32") ? "32.lib" : ".lib";
            break;
        case GEN_RELEASE64:
        default:
            cszLib += !ttStrStr(cszLib, "64") ? "64.lib" : ".lib";
            break;
    }

    m_pkfOut->WriteChar(CH_SPACE);
    m_pkfOut->WriteStr(cszLib);
}

// Some libraries will have suffixes appended to the base name to indicate platform and debug versions. If we can find such
// a version then use that.

void CNinja::GetLibName(const char* pszBaseName, ttCStr& cszLibName)
{
    ttASSERT(ttIsNonEmpty(pszBaseName));

    cszLibName = pszBaseName;
    char* pszExt = cszLibName.FindExt();
    ttCStr cszExt;

    if (pszExt && *pszExt == '.')
    {
        cszExt = pszExt;
        *pszExt = 0;
    }
    else
        cszExt = ".lib";

    switch (m_gentype)
    {
        case GEN_DEBUG32:
            cszLibName += !ttStrStr(cszLibName, "32") ? "32D" : "D";
            break;
        case GEN_DEBUG64:
            cszLibName += !ttStrStr(cszLibName, "64") ? "64D" : "D";
            break;
        case GEN_RELEASE32:
            cszLibName += !ttStrStr(cszLibName, "32") ? "32" : "";
            break;
        case GEN_RELEASE64:
        default:
            cszLibName += !ttStrStr(cszLibName, "64") ? "64" : "";
            break;
    }

    cszLibName += cszExt;

    if (GetOption(OPT_LIB_DIRS))
    {
        ttCEnumStr enumLib(GetOption(OPT_LIB_DIRS), ';');
        while (enumLib.Enum())
        {
            ttCStr cszPath(enumLib);
            cszPath.AppendFileName(cszLibName);
            if (ttFileExists(cszPath))
                return;     // we found the modified library, so return
        }
    }

    if (FindFileEnv(cszLibName, "LIB"))
        return;     // we found the modified library, so return

    // If we get here, we couldn't find the modified version

    cszLibName = pszBaseName;
    if (!ttStrChr(cszLibName, '.'))
        cszLibName += cszExt;
}

void CNinja::ProcessBuildLibs()
{
    if (GetBuildLibs())
    {
        ttCEnumStr enumLib(ttFindNonSpace(GetBuildLibs()), ';');
        ttCStr cszSaveCwd;
        cszSaveCwd.GetCWD();

        while (enumLib.Enum())
        {
            // Change to the directory that should contain a .srcfiles.yaml and read it

            if (!ttChDir(enumLib))
            {
                ttCStr cszMsg;
                cszMsg.printf(GETSTRING(IDS_LIB_DIR_FAIL), (char*) enumLib);
                cszMsg += "\n";
                AddError(cszMsg);
                continue;
            }

            // The current directory may just be the name of the library, but not necessarily where srcfiles is located.

            if (!LocateSrcFiles())
            {
                for (;;)    // empty for loop that we break out of as soon as we find a srcfiles file to use
                {
                    if (ttDirExists("src"))
                    {
                        ttChDir("src");
                        if (LocateSrcFiles())
                            break;
                        else
                            ttChDir("..");
                    }

                    if (ttDirExists("source"))
                    {
                        ttChDir("source");
                        if (LocateSrcFiles())
                            break;
                        else
                            ttChDir("..");
                    }

                    /*
                        It's unusual, but possible for there to be a sub-directory with the same name as the root
                        directory:

                        foo
                            foo -- src for foo.lib
                            bar -- src for bar.lib
                            app -- src for some related app

                    */

                    if (ttDirExists(ttFindFilePortion(enumLib)))
                    {
                        ttChDir(ttFindFilePortion(enumLib));
                        if (LocateSrcFiles())
                            break;
                        else
                            ttChDir("..");
                    }

                    // Any further directory searches should go above this -- once we get here, we can't find a .srcfiles.yaml. We go ahead an break
                    // out of the loop. cSrcFiles.ReadFile() will fail -- we'll use whatever error reporting (if any) it uses for a file that
                    // cannot be found or read.

                    break;
                }
            }

            CSrcFiles cSrcFiles;
            if (cSrcFiles.ReadFile())
            {
                ttCStr cszCurDir;
                cszCurDir.GetCWD();
                ttCStr cszRelDir;
                ttConvertToRelative(cszSaveCwd, cszCurDir, cszRelDir);
                m_dlstTargetDir.Add(cSrcFiles.GetProjectName(), cszRelDir);

                const char* pszLib;

                pszLib = cSrcFiles.GetTargetDebug32();
                if (pszLib)
                {
                    ttCStr cszLibDir;
                    cszLibDir.GetCWD();
                    cszLibDir.AppendFileName(pszLib);
                    cszLibDir.FullPathName();
                    ttCStr cszRelLib;
                    ttConvertToRelative(cszSaveCwd, cszLibDir, cszRelLib);
                    m_lstBuildLibs32D += cszRelLib;
                }

                pszLib = cSrcFiles.GetTargetDebug64();
                if (pszLib)
                {
                    ttCStr cszLibDir;
                    cszLibDir.GetCWD();
                    cszLibDir.AppendFileName(pszLib);
                    cszLibDir.FullPathName();
                    ttCStr cszRelLib;
                    ttConvertToRelative(cszSaveCwd, cszLibDir, cszRelLib);
                    m_lstBuildLibs64D += cszRelLib;
                }

                pszLib = cSrcFiles.GetTargetRelease32();
                if (pszLib)
                {
                    ttCStr cszLibDir;
                    cszLibDir.GetCWD();
                    cszLibDir.AppendFileName(pszLib);
                    cszLibDir.FullPathName();
                    ttCStr cszRelLib;
                    ttConvertToRelative(cszSaveCwd, cszLibDir, cszRelLib);
                    m_lstBuildLibs32R += cszRelLib;
                }

                pszLib = cSrcFiles.GetTargetRelease64();
                if (pszLib)
                {
                    ttCStr cszLibDir;
                    cszLibDir.GetCWD();
                    cszLibDir.AppendFileName(pszLib);
                    cszLibDir.FullPathName();
                    ttCStr cszRelLib;
                    ttConvertToRelative(cszSaveCwd, cszLibDir, cszRelLib);
                    m_lstBuildLibs64R += cszRelLib;
                }
            }

            // Change back to our original directory since library paths will be relative to our master directory, not to
            // each other

            ttChDir(cszSaveCwd);
        }
    }
}
