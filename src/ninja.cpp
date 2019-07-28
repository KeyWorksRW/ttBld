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

CNinja::CNinja(bool bVsCodeDir) : CSrcFiles(bVsCodeDir),
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

bool CNinja::CreateBuildFile(GEN_TYPE gentype, bool bClang)
{
    ttCFile file;
    m_pkfOut = &file;
    m_gentype = gentype;
    m_bClang = bClang;

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

    ttCStr cszScriptFile(GetBldDir());
    cszScriptFile.AddTrailingSlash();

    switch (gentype)
    {
        case GEN_DEBUG32:
            cszOutDir     += m_bClang ? "clangDebug32"        : "msvcDebug32";
            cszScriptFile += m_bClang ? "clangBuild32D.ninja" : "msvcBuild32D.ninja";
            break;

        case GEN_DEBUG64:
            cszOutDir     += m_bClang ? "clangDebug64"        : "msvcDebug64";
            cszScriptFile += m_bClang ? "clangBuild64D.ninja" : "msvcBuild64D.ninja";
            break;

        case GEN_RELEASE32:
            cszOutDir     += m_bClang ? "clangRelease32"     : "msvcRelease32";
            cszScriptFile += m_bClang ? "clangBuild32.ninja" : "msvcBuild32.ninja";
            break;

        case GEN_RELEASE64:
        default:
            cszOutDir     += m_bClang ? "clangRelease64"     : "msvcRelease64";
            cszScriptFile += m_bClang ? "clangBuild64.ninja" : "msvcBuild64.ninja";
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

    WriteCompilerComments();

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

    WriteCompilerFlags();
    WriteCompilerDirectives();
    WriteRcDirective();
    WriteMidlDirective(gentype);
    WriteLibDirective();
    WriteLinkDirective();

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

    WriteMidlTargets();
    WriteLinkTargets(gentype);

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
        return file.WriteFile(cszScriptFile);

    ttCFile fileOrg;
    if (fileOrg.ReadFile(cszScriptFile))
    {
        if (strcmp(fileOrg, file) == 0)    // Only write the build script if something changed
            return false;
        else if (m_dryrun.IsEnabled())
        {
            m_dryrun.NewFile(cszScriptFile);
            m_dryrun.DisplayFileDiff(fileOrg, file);
            return false;   // because we didn't write anything
        }
    }

    if (!file.WriteFile(cszScriptFile))
    {
        ttCStr cszMsg;
        cszMsg.printf(GETSTRING(IDS_NINJA_CANT_WRITE), (char*) cszScriptFile);
        cszMsg += "\n";
        AddError(cszMsg);
        return false;
    }
    return true;
}

void CNinja::WriteCompilerComments()
{
    if (!m_bClang)
    {
        m_pkfOut->WriteEol("msvc_deps_prefix = Note: including file:\n");

        // Write comment section explaining the compiler flags in use

        m_pkfOut->WriteEol("# -EHsc\t// Structured exception handling");
        if (m_gentype == GEN_RELEASE32 || m_gentype == GEN_RELEASE64)
        {
            m_pkfOut->WriteEol("# -GL\t// Whole program optimization");
            m_pkfOut->WriteEol("# -GS-\t// Turn off buffer security checks");
            if (GetBoolOption(OPT_STDCALL))
                m_pkfOut->WriteEol("# -Gz\t// __stdcall calling convention");
            if (GetBoolOption(OPT_STATIC_CRT_REL))
                m_pkfOut->WriteEol("# -MT\t// Static multi-threaded library");
            else
                m_pkfOut->WriteEol("# -MD\t// DLL version of multi-threaded library");
            if (IsExeTypeLib())
                m_pkfOut->WriteEol("# -Zl\t// Don't specify default runtime library in .obj file");
            if (IsOptimizeSpeed())
                m_pkfOut->WriteEol("# -O2\t// Optimize for speed (/Og /Oi /Ot /Oy /Ob2 /Gs /GF /Gy)");
            else
                m_pkfOut->WriteEol("# -O1\t// Optimize for size (/Og /Os /Oy /Ob2 /Gs /GF /Gy)");
        }
        else  // Presumably GEN_DEBUG32 or GEN_DEBUG64
        {
            if (GetBoolOption(OPT_STATIC_CRT_DBG))
                m_pkfOut->WriteEol("# -MTd\t// Multithreaded debug dll (MSVCRTD)");
            else
                m_pkfOut->WriteEol("# -MDd\t// Multithreaded debug dll (MSVCRTD)");
            m_pkfOut->WriteEol("# -Z7\t// produces object files with full symbolic debugging information");
        }
    }
    else if (m_bClang)
    {
        m_pkfOut->WriteEol("msvc_deps_prefix = Note: including file:\n");

        // Write comment section explaining the compiler flags in use

        m_pkfOut->WriteEol("# -EHsc\t// Structured exception handling");
        if (m_gentype == GEN_RELEASE32 || m_gentype == GEN_RELEASE64)
        {
            if (GetBoolOption(OPT_STDCALL))
                m_pkfOut->WriteEol("# -Gz\t// __stdcall calling convention");
            if (GetBoolOption(OPT_STATIC_CRT_REL))
                m_pkfOut->WriteEol("# -MT\t// Static multi-threaded library");
            else
                m_pkfOut->WriteEol("# -MD\t// DLL version of multi-threaded library");
            if (IsExeTypeLib())
                m_pkfOut->WriteEol("# -Zl\t// Don't specify default runtime library in .obj file");
            if (IsOptimizeSpeed())
                m_pkfOut->WriteEol("# -O2\t// Optimize for speed (/Og /Oi /Ot /Oy /Ob2 /Gs /GF /Gy)");
            else
                m_pkfOut->WriteEol("# -O1\t// Optimize for size (/Og /Os /Oy /Ob2 /Gs /GF /Gy)");
        }
        else              // Presumably GEN_DEBUG32 or GEN_DEBUG64
        {
            if (GetBoolOption(OPT_STATIC_CRT_DBG))
                m_pkfOut->WriteEol("# -MTd\t// Multithreaded debug dll (MSVCRTD)");
            else
                m_pkfOut->WriteEol("# -MDd\t// Multithreaded debug dll (MSVCRTD)");
            m_pkfOut->WriteEol("# -Z7\t// produces object files with full symbolic debugging information");
        }
    }
    m_pkfOut->WriteEol();   // force a blank line after the options are listed
}

// The CLANG compiler we are writing for is clang-cl.exe, which means most of the compiler flags are common for both CLANG and MSVC

void CNinja::WriteCompilerFlags()
{
    // First we write the flags common to both compilers

    if (m_gentype == GEN_DEBUG32 || m_gentype == GEN_DEBUG64)
        // For MSVC compiler you can either use -Z7 or -FS -Zf -Zi. My testing of the two approaches is that -Z7 yields
        // larger object files but reduces compile/link time by about 20% (compile speed is faster because no serialized
        // writing to the PDB file). CLANG behaves the same with either -Z7 or -Zi but does not recognize -Zf.

        m_pkfOut->printf("cflags = -nologo -D_DEBUG -showIncludes -EHsc%s -W%s%s%s -Od -Z7 -GS-",
                IsExeTypeConsole() ? " -D_CONSOLE" : "",

                GetOption(OPT_WARN_LEVEL) ? GetOption(OPT_WARN_LEVEL) : "4",
                GetBoolOption(OPT_STDCALL) ? " -Gz" : "",
                IsExeTypeLib() ? " -Zl" : (GetBoolOption(OPT_STATIC_CRT_DBG) ? " -MTd" : " -MDd")
            );
    else    // Presumably GEN_RELEASE32 or GEN_RELEASE64
        m_pkfOut->printf("cflags = -nologo -DNDEBUG -showIncludes -EHsc%s -W%s%s%s%s",
                IsExeTypeConsole() ? " -D_CONSOLE" : "",

                GetOption(OPT_WARN_LEVEL) ? GetOption(OPT_WARN_LEVEL) : "4",
                GetBoolOption(OPT_STDCALL) ?    " -Gz" : "",
                IsExeTypeLib() ? " -Zl" :  (GetBoolOption(OPT_STATIC_CRT_REL) ? " -MT" : " -MD"),
                IsOptimizeSpeed() ? " -O2" : " -O1"
            );

    if (GetOption(OPT_INC_DIRS))
    {
        ttCEnumStr cEnumStr(GetOption(OPT_INC_DIRS));
        while (cEnumStr.Enum())
            m_pkfOut->printf(" -I%kq", (const char*) cEnumStr);
    }

    if (GetOption(OPT_CFLAGS_CMN))
    {
        m_pkfOut->WriteChar(' ');
        m_pkfOut->WriteStr(GetOption(OPT_CFLAGS_CMN));
    }
    if (GetOption(OPT_CFLAGS_REL) && (m_gentype == GEN_RELEASE32 || m_gentype == GEN_RELEASE64))
    {
        m_pkfOut->WriteChar(' ');
        m_pkfOut->WriteStr(GetOption(OPT_CFLAGS_REL));
    }
    if (GetOption(OPT_CFLAGS_DBG) && (m_gentype == GEN_DEBUG32 || m_gentype == GEN_DEBUG64))
    {
        m_pkfOut->WriteChar(' ');
        m_pkfOut->WriteStr(GetOption(OPT_CFLAGS_DBG));
    }

    // Now write out the compiler-specific flags

    if (!m_bClang)
    {
        if (GetBoolOption(OPT_PERMISSIVE))
            m_pkfOut->WriteStr(" -permissive-");
        if (m_gentype == GEN_RELEASE32 || m_gentype == GEN_RELEASE64)
            m_pkfOut->WriteStr(" -GL"); // whole program optimization
    }

    else if (m_bClang)
    {
        m_pkfOut->WriteStr(" -D__clang__"); // unlike the non-MSVC compatible version, clang-cl.exe (version 7) doesn't define this
        m_pkfOut->WriteStr(" -fms-compatibility-version=19");   // Version of MSVC to be compatible with
        m_pkfOut->WriteStr(m_gentype == GEN_DEBUG64 || m_gentype == GEN_RELEASE64 ? " -m64" : " -m32"); // specify the platform
        if (m_gentype == GEN_RELEASE32 || m_gentype == GEN_RELEASE64)
            m_pkfOut->WriteStr(" -flto -fwhole-program-vtables");   // whole program optimization

        if (GetOption(OPT_CLANG_CMN))
        {
            m_pkfOut->WriteChar(' ');
            m_pkfOut->WriteStr(GetOption(OPT_CLANG_CMN));
        }
        if (GetOption(OPT_CLANG_REL) && (m_gentype == GEN_RELEASE32 || m_gentype == GEN_RELEASE64))
        {
            m_pkfOut->WriteChar(' ');
            m_pkfOut->WriteStr(GetOption(OPT_CLANG_REL));
        }
        if (GetOption(OPT_CLANG_DBG) && (m_gentype == GEN_DEBUG32 || m_gentype == GEN_DEBUG64))
        {
            m_pkfOut->WriteChar(' ');
            m_pkfOut->WriteStr(GetOption(OPT_CLANG_DBG));
        }
    }

    if (GetPchHeader())
        m_pkfOut->printf(" /Fp$outdir/%s", (char*) m_cszPCH);

    m_pkfOut->WriteEol("\n");
}

void CNinja::WriteCompilerDirectives()
{
    if (!m_bClang)
    {
        if (GetPchHeader())
        {
            m_pkfOut->printf(
                    "rule compilePCH\n"
                    "  deps = msvc\n"
                    "  command = cl.exe -c $cflags -Fo$outdir/ $in -Fd$outdir/%s.pdb -Yc%s\n",
                    GetProjectName(),
                    GetPchHeader()                  // typically stdafx.h or precomp.h
                    );
            m_pkfOut->WriteEol("  description = compiling $in\n");
        }

        // Write compile directive

        if (m_gentype == GEN_DEBUG32 || m_gentype == GEN_DEBUG64)
        {
            m_pkfOut->printf(
                    "rule compile\n"
                    "  deps = msvc\n"
                    "  command = cl.exe -c $cflags -Fo$out $in -Fd$outdir/%s.pdb %s%s\n",
                    GetProjectName(),
                    GetPchHeader() ? "-Yu" : "", GetPchHeader() ? GetPchHeader() : ""
                    );
        }
        else
        {
            m_pkfOut->printf(
                    "rule compile\n"
                    "  deps = msvc\n"
                    "  command = cl.exe -c $cflags -Fo$out $in %s%s\n",
                    GetPchHeader() ? "-Yu" : "",
                    GetPchHeader() ? GetPchHeader() : ""
                    );
        }
        m_pkfOut->WriteEol("  description = compiling $in\n");
    }
    else if (m_bClang)
    {
        if (GetPchHeader())
        {
            m_pkfOut->printf(
                    "rule compilePCH\n"
                    "  deps = msvc\n"
                    "  command = clang-cl.exe -c $cflags -Fo$outdir/ $in -Fd$outdir/%s.pdb -Yc%s\n",
                    GetProjectName(),
                    GetPchHeader()                  // typically stdafx.h or precomp.h
                    );
            m_pkfOut->WriteEol("  description = compiling $in\n");
        }

        // Write compile directive

        if (m_gentype == GEN_DEBUG32 || m_gentype == GEN_DEBUG64)
        {
            m_pkfOut->printf(
                    "rule compile\n"
                    "  deps = msvc\n"   // clang-cl supports -showIncludes, same as msvc
                    "  command = clang-cl.exe -c $cflags -Fo$out $in -Fd$outdir/%s.pdb %s%s\n",
                    GetProjectName(),
                    GetPchHeader() ? "-Yu" : "", GetPchHeader() ? GetPchHeader() : ""
                    );
        }
        else
        {
            m_pkfOut->printf(
                    "rule compile\n"
                    "  deps = msvc\n"
                    "  command = clang-cl.exe -c $cflags -Fo$out $in %s%s\n",
                    GetPchHeader() ? "-Yu" : "",
                    GetPchHeader() ? GetPchHeader() : ""
                    );
        }
        m_pkfOut->WriteEol("  description = compiling $in\n");
    }
}

void CNinja::WriteLinkDirective()
{
    if (IsExeTypeLib())
        return; // lib directive should be used if the project is a library

    ttCStr cszRule("rule link\n  command = ");

    if (GetBoolOption(OPT_MS_LINKER))
        cszRule += "link.exe /nologo";
    else
        cszRule += m_bClang ? "lld-link.exe" : "link.exe /nologo";

    cszRule += " /out:$out /manifest:no";
    if (IsExeTypeDll())
        cszRule += " /dll";
    cszRule += (m_gentype == GEN_DEBUG64 || m_gentype == GEN_RELEASE64 ? " /machine:x64" : " /machine:x86");

    if (GetOption(OPT_LINK_CMN))
    {
        cszRule += " ";
        cszRule += GetOption(OPT_LINK_CMN);
    }
    if (GetOption(OPT_LINK_REL) && (m_gentype == GEN_RELEASE32 || m_gentype == GEN_RELEASE64))
    {
        cszRule += " ";
        cszRule += GetOption(OPT_LINK_REL);
    }

    if (m_gentype == GEN_DEBUG32 || m_gentype == GEN_DEBUG64)
    {
        if (GetOption(OPT_LINK_DBG))
        {
            cszRule += " ";
            cszRule += GetOption(OPT_LINK_DBG);
        }

        if (GetOption(OPT_NATVIS))
        {
            cszRule += " /natvis:";
            cszRule += GetOption(OPT_NATVIS);
        }
        cszRule +=" /debug /pdb:$outdir/";
        cszRule += GetProjectName();
        cszRule += ".pdb";
    }
    else
    {
        cszRule += " /opt:ref /opt:icf";
        if (!m_bClang)
            cszRule += " /ltcg";    // whole program optimization, MSVC only
    }
    cszRule += IsExeTypeConsole() ? " /subsystem:console" : " /subsystem:windows";

    if (GetOption(OPT_LIB_DIRS))
    {
        ttCEnumStr enumLib(GetOption(OPT_LIB_DIRS), ';');
        while (enumLib.Enum())
        {
            cszRule += " /LIBPATH:";
            cszRule += enumLib;
        }
    }

    if (GetOption(OPT_LIBS))
    {
        ttCEnumStr enumLib(GetOption(OPT_LIBS), ';');
        while (enumLib.Enum())
        {
            ttCStr cszLib;
            GetLibName(enumLib, cszLib);
            cszRule += " ";
            cszRule += cszLib;
        }
    }

    if (GetOption(OPT_LIBS_DBG) && (m_gentype == GEN_DEBUG32 || m_gentype == GEN_DEBUG64))
    {
        ttCEnumStr enumLib(GetOption(OPT_LIBS_DBG), ';');
        while (enumLib.Enum())
        {
            cszRule += " ";
            cszRule += enumLib;
        }
    }

    if (GetOption(OPT_LIBS_REL) && (m_gentype == GEN_RELEASE32 || m_gentype == GEN_RELEASE64))
    {
        ttCEnumStr enumLib(GetOption(OPT_LIBS_REL), ';');
        while (enumLib.Enum())
        {
            cszRule += " ";
            cszRule += enumLib;
        }
    }

    cszRule += " $in";
    m_pkfOut->WriteEol(cszRule);
    m_pkfOut->WriteEol("  description = linking $out\n");
}

void CNinja::WriteLibDirective()
{
    if (!m_bClang)
    {
        if (IsExeTypeLib() || GetOption(OPT_LIB_DIRS))
        {
            m_pkfOut->printf("rule lib\n  command = lib.exe /MACHINE:%s /LTCG /NOLOGO /OUT:$out $in\n",
                (m_gentype == GEN_DEBUG64 || m_gentype == GEN_RELEASE64) ? "x64" : "x86");
            m_pkfOut->WriteEol("  description = creating library $out\n");
        }
    }
    else if (m_bClang)
    {
        if (IsExeTypeLib() || GetOption(OPT_LIB_DIRS))
        {
            // MSVC -LTCG option is not supported by lld
            m_pkfOut->printf("rule lib\n  command = lld-link.exe /lib /machine:%s /out:$out $in\n",
                (m_gentype == GEN_DEBUG64 || m_gentype == GEN_RELEASE64) ? "x64" : "x86");
            m_pkfOut->WriteEol("  description = creating library $out\n");
        }
    }
}

void CNinja::WriteRcDirective()
{
    if (ttFileExists(GetRcFile()))
    {
        if (m_bClang && !GetBoolOption(OPT_MS_RC))
            m_pkfOut->WriteStr("rule rc\n  command = llvm-rc.exe -nologo");
        else
            m_pkfOut->WriteStr("rule rc\n  command = rc.exe -nologo");

        if (GetOption(OPT_RC_CMN))
        {
            m_pkfOut->WriteChar(CH_SPACE);
            m_pkfOut->WriteStr(GetOption(OPT_RC_CMN));
        }
        if (m_gentype == GEN_DEBUG32 || m_gentype == GEN_DEBUG64)
        {
            m_pkfOut->WriteStr(" -d_DEBUG");
            if (GetOption(OPT_RC_DBG))
            {
                m_pkfOut->WriteChar(CH_SPACE);
                m_pkfOut->WriteStr(GetOption(OPT_RC_DBG));
            }
        }
        else if ((m_gentype == GEN_RELEASE32 || m_gentype == GEN_RELEASE64) && GetOption(OPT_RC_REL))
        {
            m_pkfOut->WriteChar(CH_SPACE);
            m_pkfOut->WriteStr(GetOption(OPT_RC_REL));
        }
        m_pkfOut->WriteEol(" /l 0x409 -fo$out $in\n  description = resource compiler... $in\n");
    }
}

void CNinja::WriteMidlDirective(GEN_TYPE gentype)
{
    if (m_lstIdlFiles.GetCount())
    {
        if (gentype == GEN_DEBUG64 || gentype == GEN_RELEASE64)
            m_pkfOut->WriteStr("rule midl\n  command = midl.exe /nologo /x64");
        else
            m_pkfOut->WriteStr("rule midl\n  command = midl.exe /nologo /win32");

        if (GetOption(OPT_MDL_CMN))
        {
            m_pkfOut->WriteChar(CH_SPACE);
            m_pkfOut->WriteStr(GetOption(OPT_MDL_CMN));
        }
        if (m_gentype == GEN_DEBUG32 || m_gentype == GEN_DEBUG64)
        {
            // m_pkfOut->WriteStr(" -D_DEBUG");
            if (GetOption(OPT_MDL_DBG))
            {
                m_pkfOut->WriteChar(CH_SPACE);
                m_pkfOut->WriteStr(GetOption(OPT_MDL_DBG));
            }
        }
        else if ((m_gentype == GEN_RELEASE32 || m_gentype == GEN_RELEASE64) && GetOption(OPT_MDL_REL))
        {
            m_pkfOut->WriteChar(CH_SPACE);
            m_pkfOut->WriteStr(GetOption(OPT_MDL_REL));
        }
        m_pkfOut->WriteEol(" $in\n  description = midl compiler... $in\n");
    }
}

void CNinja::WriteMidlTargets()
{
    // .idl files have one input file, and two output files: a header file (.h) and a type library file (.tlb). Typically
    // the header file will be needed by one or more source files and the typelib file will be needed by the resource
    // compiler. We create the header file as a target, and a phony rule for the typelib pointing to the header file
    // target.

    for (size_t pos = 0; pos < m_lstIdlFiles.GetCount(); ++pos)
    {
        ttCStr cszTypeLib(m_lstIdlFiles[pos]);
        cszTypeLib.ChangeExtension(".tlb");
        ttCStr cszHeader(m_lstIdlFiles[pos]);
        cszHeader.ChangeExtension(".h");
        m_pkfOut->printf("build %s : midl %s\n\n", (char*) cszHeader, m_lstIdlFiles[pos]);
        m_pkfOut->printf("build %s : phony %s\n\n", (char*) cszTypeLib, (char*) cszHeader);
    }
}

void CNinja::WriteLinkTargets(GEN_TYPE gentype)
{
    // Note that bin and lib don't need to exist ahead of time as ninja will create them, however if the output is
    // supposed to be up one directory (../bin, ../lib) then the directories MUST exist ahead of time. Only way around
    // this would be to add support for an "OutPrefix: ../" option in .srcfiles.

    const char* pszTarget;
    switch (gentype)
    {
        case GEN_DEBUG32:
            pszTarget = GetTargetDebug32();
            break;
        case GEN_DEBUG64:
            pszTarget = GetTargetDebug64();
            break;
        case GEN_RELEASE32:
            pszTarget = GetTargetRelease32();
            break;
        case GEN_RELEASE64:
        default:
            pszTarget = GetTargetRelease64();
            break;
    }

    if (IsExeTypeLib())
        m_pkfOut->printf("build %s : lib", pszTarget);
    else
        m_pkfOut->printf("build %s : link", pszTarget);

    if (ttFileExists(GetRcFile()))
    {
        ttCStr cszRes(GetRcFile());
        cszRes.RemoveExtension();
        cszRes += ((m_gentype == GEN_DEBUG32 || m_gentype == GEN_DEBUG64) ?  "D.res" : ".res");
        m_pkfOut->printf(" $resout/%s", (char*) cszRes);
    }

    bool bPchSeen = false;
    for (size_t iPos = 0; iPos < getSrcCount(); iPos++)
    {
        ttCStr cszFile(ttFindFilePortion(GetSrcFileList()->GetAt(iPos)));
        if (!ttStrStrI(cszFile, ".c"))  // we don't care about any type of file that wasn't compiled into an .obj file
            continue;
        cszFile.ChangeExtension(".obj");
        if (!bPchSeen && ttIsSameStrI(cszFile, m_cszPCHObj))
            bPchSeen = true;
        m_pkfOut->printf(" $\n  $outdir/%s", (char*) cszFile);
    }

    // The precompiled object file must be linked. It may or may not show up in the list of source files. We check here
    // to make certain it does indeed get written.

    if (!bPchSeen && m_cszPCHObj.IsNonEmpty())
        m_pkfOut->printf(" $\n  $outdir/%s", (char*) m_cszPCHObj);

    switch (gentype)
    {
        case GEN_DEBUG32:
            for (size_t pos = 0; m_lstBuildLibs32D.InRange(pos); ++pos)
                m_pkfOut->printf(" $\n  %s", (char*) m_lstBuildLibs32D[pos]);
            break;

        case GEN_DEBUG64:
            for (size_t pos = 0; m_lstBuildLibs64D.InRange(pos); ++pos)
                m_pkfOut->printf(" $\n  %s", (char*) m_lstBuildLibs64D[pos]);
            break;

        case GEN_RELEASE32:
            for (size_t pos = 0; m_lstBuildLibs32R.InRange(pos); ++pos)
                m_pkfOut->printf(" $\n  %s", (char*) m_lstBuildLibs32R[pos]);
            break;

        case GEN_RELEASE64:
        default:
            for (size_t pos = 0; m_lstBuildLibs64R.InRange(pos); ++pos)
                m_pkfOut->printf(" $\n  %s", (char*) m_lstBuildLibs64R[pos]);
            break;
    }

    if ((m_gentype == GEN_DEBUG32 || m_gentype == GEN_DEBUG64) && GetOption(OPT_NATVIS))
        m_pkfOut->printf(" $\n  | %s", GetOption(OPT_NATVIS));

    m_pkfOut->WriteEol("\n");
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
            // Change to the directory that should contain a .srcfiles and read it

            if (!ttChDir(enumLib))
            {
                ttCStr cszMsg;
                cszMsg.printf(GETSTRING(IDS_LIB_DIR_FAIL), (char*) enumLib);
                cszMsg += "\n";
                AddError(cszMsg);
                continue;
            }

            // The current directory may just be the name of the library, but not necessarily where srcfiles is located.

            if (!ttFileExists(".srcfiles") && !ttFileExists(".vscode/srcfiles.yaml"))
            {
                for (;;)    // empty for loop that we break out of as soon as we find a srcfiles file to use
                {
                    // CSrcFiles will automatically read .vscode/srcfiles.yaml if there is no .srcfiles in the current
                    // directory

                    if (ttFileExists(".vscode/srcfiles.yaml"))
                        break;

                    // See if there is a src/.srcfiles or src/.vscode/srcfiles.yaml file

                    if (ttDirExists("src"))
                    {
                        ttChDir("src");
                        if (ttFileExists(".srcfiles") || ttFileExists(".vscode/srcfiles.yaml"))
                            break;
                        else
                            ttChDir("..");
                    }

                    if (ttDirExists("source"))
                    {
                        ttChDir("source");
                        if (ttFileExists(".srcfiles") || ttFileExists(".vscode/srcfiles.yaml"))
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
                        if (ttFileExists(".srcfiles") || ttFileExists(".vscode/srcfiles.yaml"))
                            break;
                        else
                            ttChDir("..");
                    }

                    // Any further directory searches should go above this -- once we get here, we can't find a .srcfiles. We go ahead an break
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
