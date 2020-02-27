/////////////////////////////////////////////////////////////////////////////
// Name:      cmplrMsvc.cpp
// Purpose:   Creates .ninja scripts for MSVC and CLANG-CL compilers
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttlibspace.h>
#include <ttenumstr.h>  // ttEnumStr, ttEnumView -- Enumerate through substrings in a string
#include <ttcstr.h>
#include <ttcview.h>

#include "ninja.h"  // CNinja

#if defined(_WIN32)  // MSVC and CLANG-CL are only available on Windows

void CNinja::msvcWriteCompilerComments(CMPLR_TYPE cmplr)
{
    m_ninjafile.push_back("msvc_deps_prefix = Note: including file:");
    m_ninjafile.addblankline();
    m_ninjafile.push_back("# -EHsc\t// Structured exception handling");

    // Write comment section explaining the compiler flags in use
    if (cmplr == CMPLR_MSVC && m_gentype == GEN_RELEASE)
    {
        // These are only supported by the MSVC compiler
        m_ninjafile.push_back("# -GL\t  // Whole program optimization");
    }

    if (m_gentype == GEN_RELEASE)
    {
        if (GetBoolOption(OPT_STDCALL))
            m_ninjafile.push_back("# -Gz\t// __stdcall calling convention");
        if (IsStaticCrtRel())
            m_ninjafile.push_back("# -MT\t// Static CRT multi-threaded library");
        else
            m_ninjafile.push_back("# -MD\t// Dynamic CRT multi-threaded library");
        if (IsExeTypeLib())
            m_ninjafile.push_back("# -Zl\t// Don't specify default runtime library in .obj file");
        if (IsOptimizeSpeed())
            m_ninjafile.push_back("# -O2\t// Optimize for speed (/Og /Oi /Ot /Oy /Ob2 /Gs /GF /Gy)");
        else
            m_ninjafile.push_back("# -O1\t// Optimize for size (/Og /Os /Oy /Ob2 /Gs /GF /Gy)");

        m_ninjafile.push_back("# -FC\t  // Full path to source code file in diagnostics");
    }
    else
    {
        if (IsStaticCrtDbg())
            m_ninjafile.push_back("# -MD\t// Multithreaded static CRT");
        else
            m_ninjafile.push_back("# -MDd\t// Multithreaded debug dll (MSVCRTD)");
        m_ninjafile.push_back("# -Z7\t  // Produces object files with full symbolic debugging information");
        m_ninjafile.push_back("# -FC\t  // Full path to source code file in diagnostics");
    }

    m_ninjafile.addblankline();  // force a blank line after the options are listed
}

// The CLANG compiler we are writing for is clang-cl.exe, which means most of the compiler flags are common for
// both CLANG and MSVC

void CNinja::msvcWriteCompilerFlags(CMPLR_TYPE cmplr)
{
    auto& tmpline = m_ninjafile.GetTempLine();
    // First we write the flags common to both compilers

    if (m_gentype == GEN_DEBUG)
    {
        // For MSVC compiler you can either use -Z7 or -FS -Zf -Zi. My testing of the two approaches is that -Z7
        // yields larger object files but reduces compile/link time by about 20% (compile speed is faster because
        // no serialized writing to the PDB file). CLANG behaves the same with either -Z7 or -Zi but does not
        // recognize -Zf.

        // clang-format off
        tmpline.Format("cflags = -nologo -D_DEBUG -showIncludes -EHsc%s -W%s%s%s -Od -Z7",
                         IsExeTypeConsole() ? " -D_CONSOLE" : "",
                         GetOption(OPT_WARN_LEVEL) ? GetOption(OPT_WARN_LEVEL) : "4",
                         GetBoolOption(OPT_STDCALL) ? " -Gz" : "",
                        IsStaticCrtDbg() ? " -MD" : " -MDd");
        // clang-format on
    }
    else
    {
        // clang-format off
        tmpline.Format(
            "cflags = -nologo -DNDEBUG -showIncludes -EHsc%s -W%s%s%s%s", IsExeTypeConsole() ? " -D_CONSOLE" : "",
            GetOption(OPT_WARN_LEVEL) ? GetOption(OPT_WARN_LEVEL) : "4",
            GetBoolOption(OPT_STDCALL) ? " -Gz" : "",
            IsStaticCrtRel() ? " -MT" : " -MD",
            IsOptimizeSpeed() ? " -O2" : " -O1");
        // clang-format on
    }

    tmpline += " -FC";

    if (!getOptValue(Opt::CFLAGS_CMN).empty())
    {
        tmpline += (" " + getOptValue(Opt::CFLAGS_CMN));
    }

    {
        ttlib::cstr env;
        if (env.assignEnvVar("CFLAGS"))
        {
            tmpline += " " + env;
        }
    }
    if (m_gentype == GEN_RELEASE)
    {
        if (!getOptValue(Opt::CFLAGS_REL).empty())
        {
            tmpline += (" " + getOptValue(Opt::CFLAGS_REL));
        }
        ttlib::cstr env;
        if (env.assignEnvVar("CFLAGSR"))
        {
            tmpline += " " + env;
        }
    }
    else if (m_gentype == GEN_DEBUG)
    {
        if (!getOptValue(Opt::CFLAGS_DBG).empty())
        {
            tmpline += (" " + getOptValue(Opt::CFLAGS_DBG));
        }
        ttlib::cstr env;
        if (env.assignEnvVar("CFLAGSD"))
        {
            tmpline += " " + env;
        }
    }

    // Now write out the compiler-specific flags

    if (cmplr == CMPLR_MSVC)
    {
        if (m_gentype == GEN_RELEASE)
            tmpline += " -GL";
    }

    else
    {
        // unlike the non-MSVC compatible version, clang-cl.exe (version 7) doesn't define this
        tmpline += " -D__clang__";
        tmpline += " -fms-compatibility-version=19";
        tmpline += getOptBoolean(Opt::BIT32) ? " -m32" : " -m64";

        if (m_gentype == GEN_RELEASE)
        {
            tmpline += " -flto -fwhole-program-vtables";
            if (!getOptValue(Opt::CLANG_REL).empty())
                tmpline += (" " + getOptValue(Opt::CLANG_REL));
        }

        if (!getOptValue(Opt::CLANG_CMN).empty())
        {
            tmpline += (" " + getOptValue(Opt::CLANG_CMN));
        }

        if (m_gentype == GEN_DEBUG && !getOptValue(Opt::CLANG_DBG).empty())
        {
            tmpline += (" " + getOptValue(Opt::CLANG_DBG));
        }
    }

    if (!getOptValue(Opt::INC_DIRS).empty())
    {
        ttEnumStr IncDirs(getOptValue(Opt::INC_DIRS));
        for (auto dir : IncDirs)
        {
            ttlib::cstr tmp;
            tmpline += tmp.Format(" -I%ks", dir.c_str());
        }
    }

    if (GetPchHeader())
    {
        ttlib::cstr tmp;
        tmpline += tmp.Format(" /Fp$outdir/%s", m_pchHdrName.c_str());
    }
    m_ninjafile.WriteTempLine();
    m_ninjafile.addblankline();
}

void CNinja::msvcWriteCompilerDirectives(CMPLR_TYPE cmplr)
{
    ttlib::cview compiler = (cmplr == CMPLR_MSVC ? "cl.exe" : "clang-cl.exe");
    auto& tmpline = m_ninjafile.GetTempLine();

    if (GetPchHeader())
    {
        m_ninjafile.push_back("rule compilePCH");
        m_ninjafile.push_back("  deps = msvc");
        tmpline.Format("  command = %s -c $cflags -Fo$outdir/ $in -Fd$outdir/%s.pdb -Yc%s", compiler.c_str(),
                       GetProjectName(),
                       GetPchHeader()  // typically stdafx.h or precomp.h
        );
        m_ninjafile.WriteTempLine();
        m_ninjafile.push_back("  description = compiling $in");
        m_ninjafile.addblankline();
    }

    m_ninjafile.push_back("rule compile");
    m_ninjafile.push_back("  deps = msvc");

    ttlib::cstr addyu;
    if (GetPchHeader())
    {
        addyu = " -Yu";
        addyu += GetPchHeader();
    }

    if (m_gentype == GEN_DEBUG)
    {
        tmpline.Format("  command = %s -c $cflags -Fo$out $in -Fd$outdir/%s.pdb", compiler.c_str(),
                       GetProjectName());
    }
    else
    {
        tmpline.Format("  command = %s -c $cflags -Fo$out $in", compiler.c_str());
    }
    if (!addyu.empty())
        tmpline += addyu;
    m_ninjafile.WriteTempLine();

    m_ninjafile.WriteTempLine("  description = compiling $in");
    m_ninjafile.addblankline();
}

void CNinja::msvcWriteLinkDirective(CMPLR_TYPE cmplr)
{
    if (IsExeTypeLib())
        return;  // lib directive should be used if the project is a library

    m_ninjafile.push_back("rule link");
    auto& tmpline = m_ninjafile.GetTempLine();
    tmpline = "  command = ";

    if (getOptBoolean(Opt::MS_LINKER))
        tmpline += "link.exe /nologo";
    else
        tmpline += (cmplr == CMPLR_MSVC ? "link.exe -nologo" : "lld-link.exe");

    tmpline += " /out:$out /manifest:no";
    if (IsExeTypeDll())
        tmpline += " /dll";
    tmpline += (getOptBoolean(Opt::BIT32) ? " /machine:x86" : " /machine:x64");

    if (!getOptValue(Opt::LINK_CMN).empty())
        tmpline += (" " + getOptValue(Opt::LINK_CMN));

    if (m_gentype == GEN_RELEASE && !getOptValue(Opt::LINK_REL).empty())
        tmpline += (" " + getOptValue(Opt::LINK_REL));

    if (m_gentype == GEN_DEBUG)
    {
        if (!getOptValue(Opt::LINK_DBG).empty())
            tmpline += (" " + getOptValue(Opt::LINK_DBG));

        if (!getOptValue(Opt::NATVIS).empty())
            tmpline += (" /natvis:" + getOptValue(Opt::NATVIS));

        tmpline += " /debug /pdb:$outdir/";
        tmpline += GetProjectName();
        tmpline += ".pdb";
    }
    else
    {
        tmpline += " /opt:ref /opt:icf";
        if (cmplr == CMPLR_MSVC)
            tmpline += " /ltcg";  // whole program optimization, MSVC only
    }
    tmpline += IsExeTypeConsole() ? " /subsystem:console" : " /subsystem:windows";

    if (!getOptValue(Opt::LIB_DIRS).empty())
    {
        ttEnumView enumLib(getOptValue(Opt::LIB_DIRS), ';');
        for (auto iter : enumLib)
        {
            tmpline += " /LIBPATH:";
            tmpline += iter;
        }
    }

    if (!getOptValue(Opt::LIBS_CMN).empty())
    {
        ttEnumView enumLib(getOptValue(Opt::LIBS_CMN), ';');
        for (auto iter : enumLib)
        {
            tmpline += " ";
            tmpline += iter;
        }
    }

    if (m_gentype == GEN_DEBUG && !getOptValue(Opt::LIBS_DBG).empty())
    {
        ttEnumView enumLib(getOptValue(Opt::LIBS_DBG), ';');
        for (auto iter : enumLib)
        {
            tmpline += " ";
            tmpline += iter;
        }
    }

    if (m_gentype == GEN_RELEASE && !getOptValue(Opt::LIBS_REL).empty())
    {
        ttEnumView enumLib(getOptValue(Opt::LIBS_REL), ';');
        for (auto iter : enumLib)
        {
            tmpline += " ";
            tmpline += iter;
        }
    }

    m_ninjafile.WriteTempLine(" $in");
    m_ninjafile.push_back("  description = linking $out");
    m_ninjafile.addblankline();
}

void CNinja::msvcWriteLibDirective(CMPLR_TYPE cmplr)
{
    if (!IsExeTypeLib())
        return;

    auto& tmpline = m_ninjafile.GetTempLine();

    m_ninjafile.push_back("rule lib");
    if (cmplr == CMPLR_MSVC)
    {
        tmpline.Format("  command = lib.exe /MACHINE:%s /LTCG /NOLOGO /OUT:$out $in",
                       (GetBoolOption(OPT_32BIT)) ? "x86" : "x64");
        m_ninjafile.WriteTempLine();
    }
    else
    {
        // MSVC -LTCG option is not supported by lld
        tmpline.Format("  command = lld-link.exe /lib /machine:%s /out:$out $in",
                       (GetBoolOption(OPT_32BIT)) ? "x86" : "x64");
        m_ninjafile.WriteTempLine();
    }
    m_ninjafile.push_back("  description = creating library $out");
    m_ninjafile.addblankline();
}

void CNinja::msvcWriteRcDirective(CMPLR_TYPE cmplr)
{
    if (!GetRcFile().fileExists())
        return;

    auto& tmpline = m_ninjafile.GetTempLine();
    m_ninjafile.push_back("rule rc");

    if (cmplr == CMPLR_CLANG && !getOptBoolean(Opt::MS_RC))
        tmpline = "  command = llvm-rc.exe -nologo";
    else
        tmpline = "  command = rc.exe -nologo";

    if (!getOptValue(Opt::INC_DIRS).empty())
    {
        ttEnumStr enumDirs(getOptValue(Opt::INC_DIRS));
        for (auto iter : enumDirs)
        {
            ttlib::cstr tmp;
            if (iter.contains(" "))
                tmp.Format(" -I%ks", iter.c_str());
            else
                tmp.Format(" -I%s", iter.c_str());
            tmpline += tmp;
        }
    }

    if (!getOptValue(Opt::RC_CMN).empty())
        tmpline += (" " + getOptValue(Opt::RC_CMN));

    if (m_gentype == GEN_DEBUG)
    {
        tmpline += " -d_DEBUG";
        if (!getOptValue(Opt::RC_DBG).empty())
            tmpline += (" " + getOptValue(Opt::RC_DBG));
    }
    else if (m_gentype == GEN_RELEASE && !getOptValue(Opt::RC_REL).empty())
    {
        tmpline += (" " + getOptValue(Opt::RC_REL));
    }
    m_ninjafile.WriteTempLine(" /l 0x409 -fo$out $in");
    m_ninjafile.push_back("  description = resource compiler... $in");
    m_ninjafile.addblankline();
}

void CNinja::msvcWriteMidlDirective(CMPLR_TYPE /* cmplr */)
{
    if (!m_lstIdlFiles.size())
        return;

    auto& tmpline = m_ninjafile.GetTempLine();
    m_ninjafile.push_back("rule midl");

    if (getOptBoolean(Opt::BIT32))
        tmpline = "  command = midl.exe /nologo /win32";
    else
        tmpline = "  command = midl.exe /nologo /x64";

    if (!getOptValue(Opt::INC_DIRS).empty())
    {
        ttEnumStr enumDirs(getOptValue(Opt::INC_DIRS));
        for (auto iter : enumDirs)
        {
            ttlib::cstr tmp;
            if (iter.contains(" "))
                tmp.Format(" -I%ks", iter.c_str());
            else
                tmp.Format(" -I%s", iter.c_str());
            tmpline += tmp;
        }
    }

    if (!getOptValue(Opt::MIDL_CMN).empty())
        tmpline += (" " + getOptValue(Opt::MIDL_CMN));

    if (m_gentype == GEN_DEBUG && !getOptValue(Opt::MIDL_DBG).empty())
        tmpline += (" " + getOptValue(Opt::MIDL_DBG));
    else if (m_gentype == GEN_RELEASE && !getOptValue(Opt::MIDL_DBG).empty())
        tmpline += (" " + getOptValue(Opt::MIDL_DBG));

    m_ninjafile.WriteTempLine(" $in");
    m_ninjafile.push_back("  description = midl compiler... $in");
    m_ninjafile.addblankline();
}

void CNinja::msvcWriteMidlTargets(CMPLR_TYPE /* cmplr */)
{
    // .idl files have one input file, and two output files: a header file (.h) and a type library file (.tlb).
    // Typically the header file will be needed by one or more source files and the typelib file will be needed by
    // the resource compiler. We create the header file as a target, and a phony rule for the typelib pointing to
    // the header file target.

    for (size_t pos = 0; pos < m_lstIdlFiles.size(); ++pos)
    {
        ttlib::cstr TypeLib(m_lstIdlFiles[pos]);
        TypeLib.replace_extension(".tlb");
        ttlib::cstr Header(m_lstIdlFiles[pos]);
        Header.replace_extension(".h");
        m_ninjafile.GetTempLine().Format("build %s : midl %s", Header.c_str(), m_lstIdlFiles[pos].c_str());
        m_ninjafile.WriteTempLine();
        m_ninjafile.addblankline();

        m_ninjafile.GetTempLine().Format("build %s : phony %s", TypeLib.c_str(), Header.c_str());
        m_ninjafile.WriteTempLine();
        m_ninjafile.addblankline();
    }
}

void CNinja::msvcWriteLinkTargets(CMPLR_TYPE /* cmplr */)
{
    // Note that bin and lib don't need to exist ahead of time as ninja will create them, however if the output is
    // supposed to be up one directory (../bin, ../lib) then the directories MUST exist ahead of time. Only way
    // around this would be to add support for an "OutPrefix: ../" option in .srcfiles.yaml.

    auto& tmpline = m_ninjafile.GetTempLine();

    std::stringstream line;
    // "build file : cmd"
    tmpline.Format("build %s : ", m_gentype == GEN_DEBUG ? GetTargetDebug() : GetTargetRelease());

    if (IsExeTypeLib())
        tmpline += "lib";
    else
        tmpline += "link";

    if (GetRcFile().fileExists())
    {
        ttlib::cstr name(GetRcFile());
        name.replace_extension("");
        tmpline += (" $resout/" + name);
        tmpline += m_gentype == GEN_DEBUG ? "D.res" : ".res";
    }

    bool bPchSeen = false;

    for (auto file : GetSrcFileList())
    {
        auto ext = file.extension();
        if (ext.empty() || std::tolower(ext[1] != 'c'))
            continue;
        ttlib::cstr objFile(file.filename());
        objFile.replace_extension(".obj");
        if (!bPchSeen && objFile.issameas(m_pchHdrNameObj, ttlib::CASE::utf8))
            bPchSeen = true;
        tmpline += " $";
        m_ninjafile.WriteTempLine();
        tmpline = ("  $outdir/" + objFile);
    }

    // The precompiled object file must be linked. It may or may not show up in the list of source files. We check
    // here to make certain it does indeed get written.

    if (!bPchSeen && !m_pchHdrNameObj.empty())
    {
        m_ninjafile.WriteTempLine(" $");
        tmpline = ("  $outdir/" + m_pchHdrNameObj);
    }

    switch (m_gentype)
    {
        case GEN_DEBUG:
            for (size_t pos = 0; m_lstBldLibsD.InRange(pos); ++pos)
            {
                m_ninjafile.WriteTempLine(" $");
                tmpline = "  ";
                tmpline += (const char*) m_lstBldLibsD[pos];
            }
            break;

        case GEN_RELEASE:
        default:
            for (size_t pos = 0; m_lstBldLibsR.InRange(pos); ++pos)
            {
                m_ninjafile.WriteTempLine(" $");
                tmpline = "  ";
                tmpline += (const char*) m_lstBldLibsR[pos];
            }
            break;
    }

    if (m_gentype == GEN_DEBUG && !getOptValue(Opt::NATVIS).empty())
    {
        m_ninjafile.WriteTempLine(" $");
        tmpline = ("  | " + getOptValue(Opt::NATVIS));
    }
    m_ninjafile.WriteTempLine();
}

#endif  // defined(_WIN32)
