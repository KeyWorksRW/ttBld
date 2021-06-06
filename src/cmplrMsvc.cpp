/////////////////////////////////////////////////////////////////////////////
// Purpose:   Creates .ninja scripts for MSVC and CLANG-CL compilers
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "ttmultistr.h"  // multistr -- Breaks a single string into multiple strings

#include "ninja.h"  // CNinja

void CNinja::msvcWriteCompilerComments(CMPLR_TYPE cmplr)
{
    m_ninjafile.emplace_back("msvc_deps_prefix = Note: including file:");
    m_ninjafile.addEmptyLine();
    m_ninjafile.emplace_back("# -EHsc\t// Structured exception handling");

    // Write comment section explaining the compiler flags in use
    if (cmplr == CMPLR_MSVC && (m_gentype == GEN_RELEASE || m_gentype == GEN_RELEASE32))
    {
        // These are only supported by the MSVC compiler
        m_ninjafile.emplace_back("# -GL\t  // Whole program optimization");
    }

    if (m_gentype == GEN_RELEASE || m_gentype == GEN_RELEASE32)
    {
        if (IsStaticCrtRel())
            m_ninjafile.emplace_back("# -MT\t// Static CRT multi-threaded library");
        else
            m_ninjafile.emplace_back("# -MD\t// Dynamic CRT multi-threaded library");
        if (IsExeTypeLib())
            m_ninjafile.emplace_back("# -Zl\t// Don't specify default runtime library in .obj file");
        if (IsOptimizeSpeed())
            m_ninjafile.emplace_back("# -O2\t// Optimize for speed (/Og /Oi /Ot /Oy /Ob2 /Gs /GF /Gy)");
        else
            m_ninjafile.emplace_back("# -O1\t// Optimize for size (/Og /Os /Oy /Ob2 /Gs /GF /Gy)");

        m_ninjafile.emplace_back("# -FC\t  // Full path to source code file in diagnostics");
    }
    else
    {
        if (IsStaticCrtDbg())
            m_ninjafile.emplace_back("# -MD\t// Multithreaded static CRT");
        else
            m_ninjafile.emplace_back("# -MDd\t// Multithreaded debug dll (MSVCRTD)");
        m_ninjafile.emplace_back("# -Z7\t  // Produces object files with full symbolic debugging information");
        m_ninjafile.emplace_back("# -FC\t  // Full path to source code file in diagnostics");
    }

    m_ninjafile.addEmptyLine();  // force a blank line after the options are listed
}

// The CLANG compiler we are writing for is clang-cl.exe, which means most of the compiler flags are common for both CLANG
// and MSVC

void CNinja::msvcWriteCompilerFlags(CMPLR_TYPE cmplr)
{
    auto& line = m_ninjafile.addEmptyLine();
    line << "cflags = -nologo -showIncludes -EHsc -W" << getOptValue(OPT::WARN);

    // First we write the flags common to both compilers

    if (m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG32)
    {
        // For MSVC compiler you can either use -Z7 or -FS -Zf -Zi. My testing of the two approaches is that -Z7 yields
        // larger object files but reduces compile/link time by about 20% (compile speed is faster because no serialized
        // writing to the PDB file). CLANG behaves the same with either -Z7 or -Zi but does not recognize -Zf.

        line << " -Od -Z7";

        line << (IsStaticCrtDbg() ? " -MTd" : " -MDd");

        line << " -D_DEBUG";
    }
    else
    {
        line << (IsOptimizeSpeed() ? " -O2" : " -O1");

        line << (IsStaticCrtRel() ? " -MT" : " -MD");

        line << " -DNDEBUG";
    }

    if (IsExeTypeConsole())
        line << " -D_CONSOLE";

    line << " -FC";

    if (hasOptValue(OPT::CFLAGS_CMN))
    {
        line << ' ' << getOptValue(OPT::CFLAGS_CMN);
    }

    {
        ttlib::cstr env;
        if (env.assignEnvVar("CFLAGS"))
        {
            line << ' ' << env;
        }
    }

    if (m_gentype == GEN_RELEASE || m_gentype == GEN_RELEASE32)
    {
        if (hasOptValue(OPT::CFLAGS_REL))
        {
            line << ' ' << getOptValue(OPT::CFLAGS_REL);
        }

        ttlib::cstr env;
        if (env.assignEnvVar("CFLAGSR"))
        {
            line << ' ' << env;
        }
    }
    else if (m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG32)
    {
        if (hasOptValue(OPT::CFLAGS_DBG))
        {
            line << ' ' << getOptValue(OPT::CFLAGS_DBG);
        }

        ttlib::cstr env;
        if (env.assignEnvVar("CFLAGSD"))
        {
            line << ' ' << env;
        }
    }

    // Now write out the compiler-specific flags

    if (cmplr == CMPLR_MSVC)
    {
        if (m_gentype == GEN_RELEASE || m_gentype == GEN_RELEASE32)
        {
            line << " -GL";

            if (hasOptValue(OPT::MSVC_REL))
            {
                line << ' ' << getOptValue(OPT::MSVC_REL);
            }
        }

        if (hasOptValue(OPT::MSVC_CMN))
        {
            line << ' ' << getOptValue(OPT::MSVC_CMN);
        }

        if (m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG32)
        {
            if (hasOptValue(OPT::MSVC_DBG))
            {
                line << ' ' << getOptValue(OPT::MSVC_DBG);
            }
        }
    }

    else
    {
        // unlike the non-MSVC compatible version, clang-cl.exe doesn't define this
        line << " -D__clang__";
        line << " -fms-compatibility-version=19";
        if (m_gentype == GEN_DEBUG || m_gentype == GEN_RELEASE)
            line << " -m64";
        else
            line << " -m32";

        if (m_gentype == GEN_RELEASE || m_gentype == GEN_RELEASE32)
        {
            line << " -flto -fwhole-program-vtables";
            if (hasOptValue(OPT::CLANG_REL))
                line << ' ' << getOptValue(OPT::CLANG_REL);
        }

        if (hasOptValue(OPT::CLANG_CMN))
        {
            line << ' ' << getOptValue(OPT::CLANG_CMN);
        }

        if ((m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG32) && hasOptValue(OPT::CLANG_DBG))
        {
            line << ' ' << getOptValue(OPT::CLANG_DBG);
        }
    }

    if (hasOptValue(OPT::INC_DIRS))
    {
        ttlib::multistr IncDirs(getOptValue(OPT::INC_DIRS));
        for (auto dir: IncDirs)
        {
            // If the directory name contains a space, then place it in quotes
            if (ttlib::is_found(dir.find(' ')))
                line << " -I\"" << dir << '\"';
            else
                line << " -I" << dir;
        }
    }

    if (HasPch())
    {
        ttlib::cstr tmp;
        line << " /Fp$outdir/" << m_pchHdrName;
    }
    m_ninjafile.addEmptyLine();
}

void CNinja::msvcWriteCompilerDirectives(CMPLR_TYPE cmplr)
{
    ttlib::cview compiler = (cmplr == CMPLR_MSVC ? "cl.exe" : "clang-cl.exe");

    if (HasPch())
    {
        m_ninjafile.emplace_back("rule compilePCH");
        m_ninjafile.emplace_back("  deps = msvc");
        m_ninjafile.addEmptyLine().Format("  command = %s -c $cflags -Fo$outdir/ $in -Fd$outdir/%s.pdb -Yc%s",
                                          compiler.c_str(), GetProjectName().c_str(), getOptValue(OPT::PCH).c_str());
        m_ninjafile.emplace_back("  description = compiling $in");
        m_ninjafile.addEmptyLine();
    }

    m_ninjafile.emplace_back("rule compile");
    m_ninjafile.emplace_back("  deps = msvc");

    auto& line = m_ninjafile.addEmptyLine();

    if (m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG32)
    {
        line << "  command = " << compiler << " -c $cflags -Fo$out $in -Fd$outdir/" << GetProjectName() << ".pdb";
    }
    else
    {
        line << "  command = " << compiler << " -c $cflags -Fo$out $in";
    }

    if (HasPch())
        line << " -Yu" << getOptValue(OPT::PCH);

    m_ninjafile.emplace_back("  description = compiling $in");
    m_ninjafile.addEmptyLine();
}

void CNinja::msvcWriteLinkDirective(CMPLR_TYPE cmplr)
{
    if (IsExeTypeLib())
        return;  // lib directive should be used if the project is a library

    m_ninjafile.emplace_back("rule link");

    auto& line = m_ninjafile.addEmptyLine();
    line = "  command = ";

    if (isOptTrue(OPT::MS_LINKER))
        line << "link.exe /nologo";
    else
        line << (cmplr == CMPLR_MSVC ? "link.exe -nologo" : "lld-link.exe");

    line << " /out:$out /manifest:no";
    if (IsExeTypeDll())
        line << " /dll";

    if (m_gentype == GEN_DEBUG || m_gentype == GEN_RELEASE)
        line << " /machine:x64";
    else
        line << " /machine:x86";

    if (hasOptValue(OPT::LINK_CMN))
        line << ' ' << getOptValue(OPT::LINK_CMN);

    if ((m_gentype == GEN_RELEASE || m_gentype == GEN_RELEASE32) && hasOptValue(OPT::LINK_REL))
        line << ' ' << getOptValue(OPT::LINK_REL);

    if (m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG32)
    {
        if (hasOptValue(OPT::LINK_DBG))
            line << ' ' << getOptValue(OPT::LINK_DBG);

        if (hasOptValue(OPT::NATVIS))
            line << " /natvis:" << getOptValue(OPT::NATVIS);

        line << " /debug /pdb:$outdir/" << GetProjectName() << ".pdb";
    }
    else
    {
        line << " /opt:ref /opt:icf";
        if (cmplr == CMPLR_MSVC)
            line << " /ltcg";  // whole program optimization, MSVC only
    }
    line << (IsExeTypeConsole() ? " /subsystem:console" : " /subsystem:windows");

    if (hasOptValue(OPT::LIB_DIRS))
    {
        ttlib::multiview enumLib(getOptValue(OPT::LIB_DIRS), ';');
        for (auto iter: enumLib)
        {
            line << " /LIBPATH:" << iter;
        }
    }

    if (hasOptValue(OPT::LIBS_CMN))
    {
        ttlib::multiview enumLib(getOptValue(OPT::LIBS_CMN), ';');
        for (auto iter: enumLib)
        {
            line << ' ' << iter;
        }
    }

    if ((m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG32) && hasOptValue(OPT::LIBS_DBG))
    {
        ttlib::multiview enumLib(getOptValue(OPT::LIBS_DBG), ';');
        for (auto iter: enumLib)
        {
            line << ' ' << iter;
        }
    }

    if ((m_gentype == GEN_RELEASE || m_gentype == GEN_RELEASE32) && hasOptValue(OPT::LIBS_REL))
    {
        ttlib::multiview enumLib(getOptValue(OPT::LIBS_REL), ';');
        for (auto iter: enumLib)
        {
            line << ' ' << iter;
        }
    }
    line << " $in";

    m_ninjafile.emplace_back("  description = linking $out");
    m_ninjafile.addEmptyLine();
}

void CNinja::msvcWriteLibDirective(CMPLR_TYPE cmplr)
{
    if (!IsExeTypeLib())
        return;

    m_ninjafile.emplace_back("rule lib");
    auto& line = m_ninjafile.addEmptyLine();
    if (cmplr == CMPLR_MSVC)
    {
        if (m_gentype == GEN_DEBUG || m_gentype == GEN_RELEASE)
            line << "  command = lib.exe /MACHINE:x64 /LTCG /NOLOGO /OUT:$out $in";
        else
            line << "  command = lib.exe /MACHINE:x86 /LTCG /NOLOGO /OUT:$out $in";
    }
    else
    {
        if (m_gentype == GEN_DEBUG || m_gentype == GEN_RELEASE)
            // MSVC -LTCG option is not supported by lld
            line << "  command = lld-link.exe /lib /machine:x64 /out:$out $in";
        else
            line << "  command = lld-link.exe /lib /machine:x86 /out:$out $in";
    }
    m_ninjafile.emplace_back("  description = creating library $out");
    m_ninjafile.addEmptyLine();
}

void CNinja::msvcWriteRcDirective(CMPLR_TYPE cmplr)
{
    if (!GetRcFile().file_exists())
        return;

    m_ninjafile.emplace_back("rule rc");
    auto& line = m_ninjafile.addEmptyLine();

    if (cmplr == CMPLR_CLANG && !isOptTrue(OPT::MS_RC))
        line = "  command = llvm-rc.exe -nologo";
    else
        line = "  command = rc.exe -nologo";

    if (hasOptValue(OPT::INC_DIRS))
    {
        ttlib::multistr enumDirs(getOptValue(OPT::INC_DIRS));
        for (auto iter: enumDirs)
        {
            if (iter.contains(" "))
                line << " -I\"" << iter << '\"';
            else
                line << " -I" << iter;
        }
    }

    if (hasOptValue(OPT::RC_CMN))
        line << ' ' << getOptValue(OPT::RC_CMN);

    if (m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG32)
    {
        line << " -d_DEBUG";
        if (hasOptValue(OPT::RC_DBG))
            line << ' ' << getOptValue(OPT::RC_DBG);
    }
    else if ((m_gentype == GEN_RELEASE || m_gentype == GEN_RELEASE32) && hasOptValue(OPT::RC_REL))
    {
        line << ' ' << getOptValue(OPT::RC_REL);
    }
    line << " /l 0x409 -fo$out $in";
    m_ninjafile.emplace_back("  description = resource compiler... $in");
    m_ninjafile.addEmptyLine();
}

void CNinja::msvcWriteMidlDirective(CMPLR_TYPE /* cmplr */)
{
    if (!m_lstIdlFiles.size())
        return;

    m_ninjafile.emplace_back("rule midl");
    auto& line = m_ninjafile.addEmptyLine();

    if (isOptTrue(OPT::BIT32))
        line = "  command = midl.exe /nologo /win32";
    else
        line = "  command = midl.exe /nologo /x64";

    if (hasOptValue(OPT::INC_DIRS))
    {
        ttlib::multistr enumDirs(getOptValue(OPT::INC_DIRS));
        for (auto iter: enumDirs)
        {
            if (iter.contains(" "))
                line << " -I\"" << iter << '\"';
            else
                line << " -I" << iter;
        }
    }

    if (hasOptValue(OPT::MIDL_CMN))
        line << ' ' << getOptValue(OPT::MIDL_CMN);

    if ((m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG32) && hasOptValue(OPT::MIDL_DBG))
        line << ' ' << getOptValue(OPT::MIDL_DBG);
    else if ((m_gentype == GEN_RELEASE || m_gentype == GEN_RELEASE32) && hasOptValue(OPT::MIDL_DBG))
        line << ' ' << getOptValue(OPT::MIDL_DBG);

    line << " $in";
    m_ninjafile.emplace_back("  description = midl compiler... $in");
    m_ninjafile.addEmptyLine();
}

void CNinja::msvcWriteMidlTargets(CMPLR_TYPE /* cmplr */)
{
    // .idl files have one input file, and two output files: a header file (.h) and a type library file (.tlb). Typically the
    // header file will be needed by one or more source files and the typelib file will be needed by the resource compiler.
    // We create the header file as a target, and a phony rule for the typelib pointing to the header file target.

    for (size_t pos = 0; pos < m_lstIdlFiles.size(); ++pos)
    {
        ttlib::cstr TypeLib(m_lstIdlFiles[pos]);
        TypeLib.replace_extension(".tlb");
        ttlib::cstr Header(m_lstIdlFiles[pos]);
        Header.replace_extension(".h");
        m_ninjafile.addEmptyLine().Format("build %s : midl %s", Header.c_str(), m_lstIdlFiles[pos].c_str());
        m_ninjafile.addEmptyLine();

        m_ninjafile.addEmptyLine().Format("build %s : phony %s", TypeLib.c_str(), Header.c_str());
        m_ninjafile.addEmptyLine();
    }
}

void CNinja::msvcWriteLinkTargets(CMPLR_TYPE /* cmplr */)
{
    // Note that bin and lib don't need to exist ahead of time as ninja will create them, however if the output is supposed
    // to be up one directory (../bin, ../lib) then the directories MUST exist ahead of time. Only way around this would be
    // to add support for an "OutPrefix: ../" option in .srcfiles.yaml.

    m_ninjafile.addEmptyLine();

    // "build file : cmd"
    lastline() << "build ";
    if (m_gentype == GEN_DEBUG)
        lastline() << GetTargetDebug();
    else if (m_gentype == GEN_DEBUG32)
        lastline() << GetTargetDebug32();
    else if (m_gentype == GEN_RELEASE)
        lastline() << GetTargetRelease();
    else
        lastline() << GetTargetRelease32();
    lastline() << " : ";

    if (IsExeTypeLib())
        lastline() << "lib";
    else
        lastline() << "link";

    if (GetRcFile().file_exists())
    {
        ttlib::cstr name(GetRcFile());
        name.replace_extension("");
        lastline() << " $resout/" << name;
        lastline() << ((m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG32) ? "D.res" : ".res");
    }

    bool bPchSeen = false;

    for (auto file: m_lstSrcFiles)
    {
        auto ext = file.extension();
        if (ext.empty() || std::tolower(ext[1] != 'c'))
            continue;
        ttlib::cstr objFile(file.filename());
        objFile.replace_extension(".obj");
        if (!bPchSeen && objFile.is_sameas(m_pchHdrNameObj, tt::CASE::utf8))
            bPchSeen = true;
        lastline() << " $";
        m_ninjafile.emplace_back("  $outdir/" + objFile);
    }

    if (m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG32)
    {
        for (auto file: m_lstDebugFiles)
        {
            auto ext = file.extension();
            if (ext.empty() || std::tolower(ext[1] != 'c'))
                continue;
            ttlib::cstr objFile(file.filename());
            objFile.replace_extension(".obj");
            if (!bPchSeen && objFile.is_sameas(m_pchHdrNameObj, tt::CASE::utf8))
                bPchSeen = true;
            lastline() << " $";
            m_ninjafile.emplace_back("  $outdir/" + objFile);
        }
    }

    // The precompiled object file must be linked. It may or may not show up in the list of source files. We check here to
    // make certain it does indeed get written.

    if (!bPchSeen && !m_pchHdrNameObj.empty())
    {
        lastline() << " $";
        m_ninjafile.emplace_back("  $outdir/" + m_pchHdrNameObj);
    }

    switch (m_gentype)
    {
        case GEN_DEBUG:
            for (auto& dir: m_bldLibs)
            {
                lastline() << " $";
                m_ninjafile.emplace_back("  " + dir.libPathDbg);
            }
            break;

        case GEN_DEBUG32:
            for (auto& dir: m_bldLibs32)
            {
                lastline() << " $";
                m_ninjafile.emplace_back("  " + dir.libPathDbg);
            }
            break;

        case GEN_RELEASE32:
            for (auto& dir: m_bldLibs32)
            {
                lastline() << " $";
                m_ninjafile.emplace_back("  " + dir.libPathRel);
            }
            break;

        case GEN_RELEASE:
        default:
            for (auto& dir: m_bldLibs)
            {
                lastline() << " $";
                m_ninjafile.emplace_back("  " + dir.libPathRel);
            }
            break;
    }

    if ((m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG32) && hasOptValue(OPT::NATVIS))
    {
        lastline() << " $";
        m_ninjafile.emplace_back("  | " + getOptValue(OPT::NATVIS));
    }
}
