/////////////////////////////////////////////////////////////////////////////
// Name:      cmplrMsvc.cpp
// Purpose:   Creates .ninja scripts for MSVC and CLANG-CL compilers
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttfile.h>     // ttCFile
#include <ttenumstr.h>  // ttCEnumStr

#include "ninja.h"  // CNinja

#if defined(_WIN32)  // MSVC and CLANG-CL are only available on Windows

void CNinja::msvcWriteCompilerComments(CMPLR_TYPE cmplr)
{
    m_pkfOut->WriteEol("msvc_deps_prefix = Note: including file:\n");
    m_pkfOut->WriteEol("# -EHsc\t// Structured exception handling");

    // Write comment section explaining the compiler flags in use
    if (cmplr == CMPLR_MSVC && m_gentype == GEN_RELEASE)
    {
        // These are only supported by the MSVC compiler
        m_pkfOut->WriteEol("# -GL\t  // Whole program optimization");
    }

    if (m_gentype == GEN_RELEASE)
    {
        if (GetBoolOption(OPT_STDCALL))
            m_pkfOut->WriteEol("# -Gz\t// __stdcall calling convention");
        if (IsStaticCrtRel())
            m_pkfOut->WriteEol("# -MT\t// Static CRT multi-threaded library");
        else
            m_pkfOut->WriteEol("# -MD\t// Dynamic CRT multi-threaded library");
        if (IsExeTypeLib())
            m_pkfOut->WriteEol("# -Zl\t// Don't specify default runtime library in .obj file");
        if (IsOptimizeSpeed())
            m_pkfOut->WriteEol("# -O2\t// Optimize for speed (/Og /Oi /Ot /Oy /Ob2 /Gs /GF /Gy)");
        else
            m_pkfOut->WriteEol("# -O1\t// Optimize for size (/Og /Os /Oy /Ob2 /Gs /GF /Gy)");

        m_pkfOut->WriteEol("# -FC\t  // Full path to source code file in diagnostics");
    }
    else
    {
        if (IsStaticCrtDbg())
            m_pkfOut->WriteEol("# -MD\t// Multithreaded static CRT");
        else
            m_pkfOut->WriteEol("# -MDd\t// Multithreaded debug dll (MSVCRTD)");
        m_pkfOut->WriteEol("# -Z7\t  // Produces object files with full symbolic debugging information");
        m_pkfOut->WriteEol("# -FC\t  // Full path to source code file in diagnostics");
    }

    m_pkfOut->WriteEol();  // force a blank line after the options are listed
}

// The CLANG compiler we are writing for is clang-cl.exe, which means most of the compiler flags are common for both
// CLANG and MSVC

void CNinja::msvcWriteCompilerFlags(CMPLR_TYPE cmplr)
{
    // First we write the flags common to both compilers

    if (m_gentype == GEN_DEBUG)
    {
        // For MSVC compiler you can either use -Z7 or -FS -Zf -Zi. My testing of the two approaches is that -Z7 yields
        // larger object files but reduces compile/link time by about 20% (compile speed is faster because no serialized
        // writing to the PDB file). CLANG behaves the same with either -Z7 or -Zi but does not recognize -Zf.

        // clang-format off
        m_pkfOut->printf("cflags = -nologo -D_DEBUG -showIncludes -EHsc%s -W%s%s%s -Od -Z7",
                         IsExeTypeConsole() ? " -D_CONSOLE" : "",
                         GetOption(OPT_WARN_LEVEL) ? GetOption(OPT_WARN_LEVEL) : "4",
                         GetBoolOption(OPT_STDCALL) ? " -Gz" : "",
                        IsStaticCrtDbg() ? " -MD" : " -MDd");
        // clang-format on
    }
    else
    {
        // clang-format off
        m_pkfOut->printf(
            "cflags = -nologo -DNDEBUG -showIncludes -EHsc%s -W%s%s%s%s", IsExeTypeConsole() ? " -D_CONSOLE" : "",
            GetOption(OPT_WARN_LEVEL) ? GetOption(OPT_WARN_LEVEL) : "4",
            GetBoolOption(OPT_STDCALL) ? " -Gz" : "",
            IsStaticCrtRel() ? " -MT" : " -MD",
            IsOptimizeSpeed() ? " -O2" : " -O1");
        // clang-format on
    }
    m_pkfOut->WriteStr(" -FC");

    if (GetOption(OPT_CFLAGS_CMN))
    {
        m_pkfOut->WriteChar(' ');
        m_pkfOut->WriteStr(GetOption(OPT_CFLAGS_CMN));
    }

    ttCStr cszEnv;
    if (cszEnv.GetEnv("CFLAGS"))
    {
        m_pkfOut->WriteChar(' ');
        m_pkfOut->WriteStr(cszEnv);
    }

    if (GetOption(OPT_CFLAGS_REL) && m_gentype == GEN_RELEASE)
    {
        m_pkfOut->WriteChar(' ');
        m_pkfOut->WriteStr(GetOption(OPT_CFLAGS_REL));
    }

    if (m_gentype == GEN_RELEASE && cszEnv.GetEnv("CFLAGSR"))
    {
        m_pkfOut->WriteChar(' ');
        m_pkfOut->WriteStr(cszEnv);
    }

    if (GetOption(OPT_CFLAGS_DBG) && m_gentype == GEN_DEBUG)
    {
        m_pkfOut->WriteChar(' ');
        m_pkfOut->WriteStr(GetOption(OPT_CFLAGS_DBG));
    }

    if (m_gentype == GEN_DEBUG && cszEnv.GetEnv("CFLAGSD"))
    {
        m_pkfOut->WriteChar(' ');
        m_pkfOut->WriteStr(cszEnv);
    }

    // Now write out the compiler-specific flags

    if (cmplr == CMPLR_MSVC)
    {
        if (GetBoolOption(OPT_PERMISSIVE))
            m_pkfOut->WriteStr(" -permissive-");
        if (m_gentype == GEN_RELEASE)
            m_pkfOut->WriteStr(" -GL");  // whole program optimization
    }

    else
    {
        m_pkfOut->WriteStr(
            " -D__clang__");  // unlike the non-MSVC compatible version, clang-cl.exe (version 7) doesn't define this
        m_pkfOut->WriteStr(" -fms-compatibility-version=19");              // Version of MSVC to be compatible with
        m_pkfOut->WriteStr(GetBoolOption(OPT_32BIT) ? " -m32" : " -m64");  // specify the platform
        if (m_gentype == GEN_RELEASE)
            m_pkfOut->WriteStr(" -flto -fwhole-program-vtables");  // whole program optimization

        if (GetOption(OPT_CLANG_CMN))
        {
            m_pkfOut->WriteChar(' ');
            m_pkfOut->WriteStr(GetOption(OPT_CLANG_CMN));
        }
        if (m_gentype == GEN_RELEASE && GetOption(OPT_CLANG_REL))
        {
            m_pkfOut->WriteChar(' ');
            m_pkfOut->WriteStr(GetOption(OPT_CLANG_REL));
        }
        if (m_gentype == GEN_DEBUG && GetOption(OPT_CLANG_DBG))
        {
            m_pkfOut->WriteChar(' ');
            m_pkfOut->WriteStr(GetOption(OPT_CLANG_DBG));
        }
    }

    if (GetOption(OPT_INC_DIRS))
    {
        ttCEnumStr cEnumStr(GetOption(OPT_INC_DIRS));
        while (cEnumStr.Enum())
            m_pkfOut->printf(" -I%kq", (const char*) cEnumStr);
    }

    if (GetPchHeader())
        m_pkfOut->printf(" /Fp$outdir/%s", (char*) m_cszPCH);

    m_pkfOut->WriteEol("\n");
}

void CNinja::msvcWriteCompilerDirectives(CMPLR_TYPE cmplr)
{
    if (cmplr == CMPLR_MSVC)
    {
        if (GetPchHeader())
        {
            m_pkfOut->printf("rule compilePCH\n"
                             "  deps = msvc\n"
                             "  command = cl.exe -c $cflags -Fo$outdir/ $in -Fd$outdir/%s.pdb -Yc%s\n",
                             GetProjectName(),
                             GetPchHeader()  // typically stdafx.h or precomp.h
            );
            m_pkfOut->WriteEol("  description = compiling $in\n");
        }

        // Write compile directive

        if (m_gentype == GEN_DEBUG)
        {
            m_pkfOut->printf("rule compile\n"
                             "  deps = msvc\n"
                             "  command = cl.exe -c $cflags -Fo$out $in -Fd$outdir/%s.pdb %s%s\n",
                             GetProjectName(), GetPchHeader() ? "-Yu" : "", GetPchHeader() ? GetPchHeader() : "");
        }
        else
        {
            m_pkfOut->printf("rule compile\n"
                             "  deps = msvc\n"
                             "  command = cl.exe -c $cflags -Fo$out $in %s%s\n",
                             GetPchHeader() ? "-Yu" : "", GetPchHeader() ? GetPchHeader() : "");
        }
    }
    else
    {
        if (GetPchHeader())
        {
            m_pkfOut->printf("rule compilePCH\n"
                             "  deps = msvc\n"
                             "  command = clang-cl.exe -c $cflags -Fo$outdir/ $in -Fd$outdir/%s.pdb -Yc%s\n",
                             GetProjectName(),
                             GetPchHeader()  // typically stdafx.h or precomp.h
            );
            m_pkfOut->WriteEol("  description = compiling $in\n");
        }

        // Write compile directive

        if (m_gentype == GEN_DEBUG)
        {
            m_pkfOut->printf("rule compile\n"
                             "  deps = msvc\n"  // clang-cl supports -showIncludes, same as msvc
                             "  command = clang-cl.exe -c $cflags -Fo$out $in -Fd$outdir/%s.pdb %s%s\n",
                             GetProjectName(), GetPchHeader() ? "-Yu" : "", GetPchHeader() ? GetPchHeader() : "");
        }
        else
        {
            m_pkfOut->printf("rule compile\n"
                             "  deps = msvc\n"
                             "  command = clang-cl.exe -c $cflags -Fo$out $in %s%s\n",
                             GetPchHeader() ? "-Yu" : "", GetPchHeader() ? GetPchHeader() : "");
        }
    }

    m_pkfOut->WriteEol("  description = compiling $in\n");
}

void CNinja::msvcWriteLinkDirective(CMPLR_TYPE cmplr)
{
    if (IsExeTypeLib())
        return;  // lib directive should be used if the project is a library

    ttCStr cszRule("rule link\n  command = ");

    if (GetBoolOption(OPT_MS_LINKER))
        cszRule += "link.exe /nologo";
    else
        cszRule += (cmplr == CMPLR_MSVC ? "link.exe -nologo" : "lld-link.exe");

    cszRule += " /out:$out /manifest:no";
    if (IsExeTypeDll())
        cszRule += " /dll";
    cszRule += (GetBoolOption(OPT_32BIT) ? " /machine:x86" : " /machine:x64");

    if (GetOption(OPT_LINK_CMN))
    {
        cszRule += " ";
        cszRule += GetOption(OPT_LINK_CMN);
    }
    if (GetOption(OPT_LINK_REL) && (m_gentype == GEN_RELEASE))
    {
        cszRule += " ";
        cszRule += GetOption(OPT_LINK_REL);
    }

    if (m_gentype == GEN_DEBUG)
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
        cszRule += " /debug /pdb:$outdir/";
        cszRule += GetProjectName();
        cszRule += ".pdb";
    }
    else
    {
        cszRule += " /opt:ref /opt:icf";
        if (cmplr == CMPLR_MSVC)
            cszRule += " /ltcg";  // whole program optimization, MSVC only
    }
    cszRule += IsExeTypeConsole() ? " /subsystem:console" : " /subsystem:windows";

    if (GetOption(OPT_LIBS_CMN))
    {
        ttCEnumStr enumLib(GetOption(OPT_LIBS_CMN), ';');
        while (enumLib.Enum())
        {
            cszRule += " ";
            cszRule += enumLib;
        }
    }

    if (GetOption(OPT_LIBS_DBG) && m_gentype == GEN_DEBUG)
    {
        ttCEnumStr enumLib(GetOption(OPT_LIBS_DBG), ';');
        while (enumLib.Enum())
        {
            cszRule += " ";
            cszRule += enumLib;
        }
    }

    if (GetOption(OPT_LIBS_REL) && (m_gentype == GEN_RELEASE))
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

void CNinja::msvcWriteLibDirective(CMPLR_TYPE cmplr)
{
    if (cmplr == CMPLR_MSVC)
    {
        if (IsExeTypeLib())
        {
            m_pkfOut->printf("rule lib\n  command = lib.exe /MACHINE:%s /LTCG /NOLOGO /OUT:$out $in\n",
                             (GetBoolOption(OPT_32BIT)) ? "x86" : "x64");
            m_pkfOut->WriteEol("  description = creating library $out\n");
        }
    }
    else
    {
        if (IsExeTypeLib())
        {
            // MSVC -LTCG option is not supported by lld
            m_pkfOut->printf("rule lib\n  command = lld-link.exe /lib /machine:%s /out:$out $in\n",
                             (GetBoolOption(OPT_32BIT)) ? "x86" : "x64");
            m_pkfOut->WriteEol("  description = creating library $out\n");
        }
    }
}

void CNinja::msvcWriteRcDirective(CMPLR_TYPE cmplr)
{
    if (ttFileExists(GetRcFile()))
    {
        if (cmplr == CMPLR_CLANG && !GetBoolOption(OPT_MS_RC))
            m_pkfOut->WriteStr("rule rc\n  command = llvm-rc.exe -nologo");
        else
            m_pkfOut->WriteStr("rule rc\n  command = rc.exe -nologo");

        if (GetOption(OPT_INC_DIRS))
        {
            ttCEnumStr cEnumStr(GetOption(OPT_INC_DIRS));
            while (cEnumStr.Enum())
                m_pkfOut->printf(" -I%kq", (const char*) cEnumStr);
        }

        if (GetOption(OPT_RC_CMN))
        {
            m_pkfOut->WriteChar(CH_SPACE);
            m_pkfOut->WriteStr(GetOption(OPT_RC_CMN));
        }
        if (m_gentype == GEN_DEBUG)
        {
            m_pkfOut->WriteStr(" -d_DEBUG");
            if (GetOption(OPT_RC_DBG))
            {
                m_pkfOut->WriteChar(CH_SPACE);
                m_pkfOut->WriteStr(GetOption(OPT_RC_DBG));
            }
        }
        else if ((m_gentype == GEN_RELEASE) && GetOption(OPT_RC_REL))
        {
            m_pkfOut->WriteChar(CH_SPACE);
            m_pkfOut->WriteStr(GetOption(OPT_RC_REL));
        }
        m_pkfOut->WriteEol(" /l 0x409 -fo$out $in\n  description = resource compiler... $in\n");
    }
}

void CNinja::msvcWriteMidlDirective(CMPLR_TYPE /* cmplr */)
{
    if (m_lstIdlFiles.GetCount())
    {
        if (GetBoolOption(OPT_32BIT))
            m_pkfOut->WriteStr("rule midl\n  command = midl.exe /nologo /win32");
        else
            m_pkfOut->WriteStr("rule midl\n  command = midl.exe /nologo /x64");

        if (GetOption(OPT_INC_DIRS))
        {
            ttCEnumStr cEnumStr(GetOption(OPT_INC_DIRS));
            while (cEnumStr.Enum())
                m_pkfOut->printf(" -I%kq", (const char*) cEnumStr);
        }

        if (GetOption(OPT_MDL_CMN))
        {
            m_pkfOut->WriteChar(CH_SPACE);
            m_pkfOut->WriteStr(GetOption(OPT_MDL_CMN));
        }
        if (m_gentype == GEN_DEBUG)
        {
            // m_pkfOut->WriteStr(" -D_DEBUG");
            if (GetOption(OPT_MDL_DBG))
            {
                m_pkfOut->WriteChar(CH_SPACE);
                m_pkfOut->WriteStr(GetOption(OPT_MDL_DBG));
            }
        }
        else if ((m_gentype == GEN_RELEASE) && GetOption(OPT_MDL_REL))
        {
            m_pkfOut->WriteChar(CH_SPACE);
            m_pkfOut->WriteStr(GetOption(OPT_MDL_REL));
        }
        m_pkfOut->WriteEol(" $in\n  description = midl compiler... $in\n");
    }
}

void CNinja::msvcWriteMidlTargets(CMPLR_TYPE /* cmplr */)
{
    // .idl files have one input file, and two output files: a header file (.h) and a type library file (.tlb).
    // Typically the header file will be needed by one or more source files and the typelib file will be needed by the
    // resource compiler. We create the header file as a target, and a phony rule for the typelib pointing to the header
    // file target.

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

void CNinja::msvcWriteLinkTargets(CMPLR_TYPE /* cmplr */)
{
    // Note that bin and lib don't need to exist ahead of time as ninja will create them, however if the output is
    // supposed to be up one directory (../bin, ../lib) then the directories MUST exist ahead of time. Only way around
    // this would be to add support for an "OutPrefix: ../" option in .srcfiles.yaml.

    const char* pszTarget = (m_gentype == GEN_DEBUG) ? GetTargetDebug() : GetTargetRelease();

    if (IsExeTypeLib())
        m_pkfOut->printf("build %s : lib", pszTarget);
    else
        m_pkfOut->printf("build %s : link", pszTarget);

    if (ttFileExists(GetRcFile()))
    {
        ttCStr cszRes(GetRcFile());
        cszRes.RemoveExtension();
        cszRes += ((m_gentype == GEN_DEBUG) ? "D.res" : ".res");
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

    if (m_gentype == GEN_DEBUG && GetOption(OPT_NATVIS))
        m_pkfOut->printf(" $\n  | %s", GetOption(OPT_NATVIS));

    m_pkfOut->WriteEol("\n");
}

#endif  // defined(_WIN32)
