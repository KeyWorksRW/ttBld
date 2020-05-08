/////////////////////////////////////////////////////////////////////////////
// Name:      CNinja
// Purpose:   Class for creating/maintaining *.ninja files for use by ninja.exe
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttcwd.h>       // Class for storing and optionally restoring the current directory
#include <ttmultistr.h>  // multistr -- Breaks a single string into multiple strings

#include "ninja.h"     // CNinja
#include "parsehhp.h"  // CParseHHP
#include "strtable.h"  // String resource IDs
#include "verninja.h"  // CVerMakeNinja

const char* aCppExt[] { ".cpp", ".cxx", ".cc", nullptr };

CNinja::CNinja(std::string_view projectFile)
{
#if !defined(NDEBUG)  // Starts debug section.
    assert(ReadFile(projectFile));
#else
    if (!ReadFile(projectFile))
        return;
#endif
    m_isWriteIfNoChange = false;

    CVerMakeNinja verSrcFiles;
    m_isInvalidVersion = verSrcFiles.IsSrcFilesNewer(GetMajorRequired(), GetMinorRequired(), GetSubRequired());

    if (GetProjectName().empty())
    {
        ttlib::cstr projname;
        projname.assignCwd();
        projname.backslashestoforward();
        if (projname.back() == '/')
            projname.pop_back();

        if (projname.hasFilename("src"))
        {
            projname.remove_filename();
            if (projname.back() == '/')
                projname.pop_back();
        }
        setOptValue(OPT::PROJECT, projname.filename());
    }

    if (!m_RCname.empty())
        FindRcDependencies(m_RCname);

    auto envBldCFlags = getenv("TTBLD_CFLAGS");
    if (envBldCFlags)
    {
        ttlib::cstr flags;
        if (hasOptValue(OPT::CFLAGS_CMN))
        {
            flags = getOptValue(OPT::CFLAGS_CMN);
            flags += " ";
        }
        flags += envBldCFlags;
        setOptValue(OPT::CFLAGS_CMN, flags);
    }

    ProcessBuildLibs();
}

static const char* aszCompilerPrefix[] {
    "msvc_",
    "clang_",
    "gcc_",
};

bool CNinja::CreateBuildFile(GEN_TYPE gentype, CMPLR_TYPE cmplr)
{
    m_ninjafile.clear();

    m_gentype = gentype;

    // Note that resout goes to the same directory in all builds. The actual filename will have a 'D' appended for
    // debug builds. Currently, 32 and 64 bit builds of the resource file are identical.

    ttlib::cstr resout("resout = ");
    resout += GetBldDir();
    resout.append_filename("res");

    ttlib::cstr builddir("builddir = ");
    builddir += GetBldDir();

    ttlib::cstr outdir("outdir = ");
    outdir += GetBldDir();
    outdir.addtrailingslash();

    m_scriptFilename = GetBldDir();
    m_scriptFilename.backslashestoforward();
    m_scriptFilename.addtrailingslash();

    switch (gentype)
    {
        case GEN_DEBUG:
            outdir += aszCompilerPrefix[cmplr];
            outdir += "Debug";
            m_scriptFilename += aszCompilerPrefix[cmplr];
            m_scriptFilename += "dbg.ninja";
            break;

        case GEN_RELEASE:
        default:
            outdir += aszCompilerPrefix[cmplr];
            outdir += "Release";
            m_scriptFilename += aszCompilerPrefix[cmplr];
            m_scriptFilename += "rel.ninja";
            break;
    }

    m_ninjafile.addEmptyLine();
    lastline() += "# WARNING: This file is auto-generated by ";
    lastline() += txtVersion;
    m_ninjafile.emplace_back("# Changes you make will be lost if it is auto-generated again!");
    m_ninjafile.addEmptyLine();

    // REVIEW: [KeyWorks - 11-19-2019] We don't write any new features, could probably state 1.0 and it would work.
    m_ninjafile.emplace_back("ninja_required_version = 1.8");
    m_ninjafile.addEmptyLine();

    m_ninjafile.emplace_back(builddir);
    m_ninjafile.emplace_back(outdir);
    m_ninjafile.emplace_back(resout);
    m_ninjafile.addEmptyLine();

    // Figure out the filenames to use for the source and output for a precompiled header

    if (hasOptValue(OPT::PCH) && !GetPchCpp().issameprefix("none"))
    {
        m_pchHdrName = GetPchCpp();
        m_pchHdrName.replace_extension(".pch");

        m_pchCppName = GetPchCpp();
        m_pchHdrNameObj.assign(m_pchCppName.filename());
        m_pchHdrNameObj.replace_extension(".obj");

        if (!m_pchCppName.fileExists())
        {
            AddError(getOptValue(OPT::PCH) + _tt(IDS_MISSING_PCH_CPP));
        }
    }

    if (cmplr == CMPLR_MSVC || cmplr == CMPLR_CLANG)
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

    // If the project has a .idl file, then the midl compiler will create a matching header file that will be
    // included in one or more source files. If the .idl file changes, or the header file doesn't exist yet, then
    // we need to run the midl compiler before compiling any source files. If we knew ahead of time which source
    // files included the header file, then we could create a dependency. However, that would essentially require
    // an accurate C/C++ preprocessor to run on every source file which is far beyond the scope of this project.
    // Instead, we add the dependency to the precompiled header if there is one, and if not, we add the dependency
    // to every source m_ninjafile. Unfortunately that does mean that every time the .idl file changes, then every
    // source file will get rebuilt whether or not a particular source file actually uses the generated header
    // m_ninjafile.

    if (hasOptValue(OPT::PCH) && !getOptValue(OPT::PCH).issameas("none"))
    {
        m_ninjafile.addEmptyLine();
        lastline().Format("build $outdir/%s: compilePCH %s", m_pchHdrNameObj.c_str(), m_pchCppName.c_str());
        if (m_lstIdlFiles.size())
        {
            lastline() += " | $";
            size_t pos;
            ttlib::cstr header;
            for (pos = 0; pos < m_lstIdlFiles.size() - 1; ++pos)
            {
                header.assign(m_lstIdlFiles[pos]);
                header.replace_extension(".h");
                m_ninjafile.addEmptyLine().Format("  %s $", header.c_str());
            }
            header.assign(m_lstIdlFiles[pos]);
            header.replace_extension(".h");
            m_ninjafile.addEmptyLine().Format("  %s", header.c_str());
        }
        m_ninjafile.addEmptyLine();
    }

    // Write the build rules for all source files

    for (auto srcFile: GetSrcFileList())
    {
        auto ext = srcFile.extension();
        if (ext.empty() || std::tolower(ext[1] != 'c'))
            continue;
        if (srcFile.issameas(m_pchCppName))
            continue;

        ttlib::cstr objFile(srcFile.filename());
        objFile.replace_extension(".obj");

        if (!m_pchHdrNameObj.empty())
        {
            // we add m_pchHdrNameObj so it appears as a dependency and gets compiled, but not linked to
            m_ninjafile.addEmptyLine().Format("build $outdir/%s: compile %s | $outdir/%s", objFile.c_str(), srcFile.c_str(),
                                              m_pchHdrNameObj.c_str());
            m_ninjafile.addEmptyLine();
        }
        else
        {
            // We get here if we don't have a precompiled header. We might have .idl files, which means we're going
            // to need to add all the midl-generated header files as dependencies to each source m_ninjafile. See
            // issue #80 for details.

            m_ninjafile.addEmptyLine();
            lastline().Format("build $outdir/%s: compile %s", objFile.c_str(), srcFile.c_str());
            if (m_lstIdlFiles.size())
            {
                lastline() += " | $";
                m_ninjafile.addEmptyLine();
                size_t pos;
                ttlib::cstr header;
                for (pos = 0; pos < m_lstIdlFiles.size() - 1; ++pos)
                {
                    header.assign(m_lstIdlFiles[pos]);
                    header.replace_extension(".h");
                    m_ninjafile.addEmptyLine().Format("  %s $", header.c_str());
                }
                header.assign(m_lstIdlFiles[pos]);
                header.replace_extension(".h");
                // write the last one without the trailing pipe
                m_ninjafile.addEmptyLine().Format("  %s", header.c_str());
            }
            m_ninjafile.addEmptyLine();
        }
    }

    // Write the build rule for the resource compiler if an .rc file was specified as a source

    if (GetRcFile().fileExists())
    {
        ttlib::cstr resource { GetRcFile() };

        resource.replace_extension("");
        resource += ((m_gentype == GEN_DEBUG) ? "D.res" : ".res");

        m_ninjafile.addEmptyLine();
        lastline().Format("build $resout/%s: rc %s", resource.c_str(), GetRcFile().c_str());

        if (m_RcDependencies.size())
        {
            lastline() += " | $";
            size_t pos = 0;
            for (; pos < m_RcDependencies.size() - 1; pos++)
            {
                m_ninjafile.emplace_back("  " + m_RcDependencies[pos] + " $");
            }
            m_ninjafile.emplace_back("  " + m_RcDependencies[pos]);
        }
        m_ninjafile.addEmptyLine();
    }

    // Write the final build rules to complete the project

    if (cmplr == CMPLR_MSVC || cmplr == CMPLR_CLANG)
    {
        msvcWriteMidlTargets(cmplr);
        msvcWriteLinkTargets(cmplr);
    }

    if (!GetBldDir().dirExists())
    {
        if (!fs::create_directory(GetBldDir().c_str()))
        {
            AddError(_tt(IDS_CANT_CREATE) + GetBldDir());
            return false;
        }
    }

    if (m_isWriteIfNoChange)
        return m_ninjafile.WriteFile(m_scriptFilename);

    ttlib::viewfile fileOrg;
    if (fileOrg.ReadFile(m_scriptFilename))
    {
        if (m_dryrun.IsEnabled())
        {
            m_dryrun.NewFile(m_scriptFilename);
            m_dryrun.DisplayFileDiff(fileOrg, m_ninjafile);
            return false;  // because we didn't write anything
        }

        if (fileOrg.issameas(m_ninjafile))
        {
            return false;  // nothing changed
        }
    }

    if (!m_ninjafile.WriteFile(m_scriptFilename))
    {
        m_ninjafile.clear();
        std::string str(_tt(IDS_ADDED_IGNORE_FILES) + m_scriptFilename + '\n');
        AddError(str);
        return false;
    }

    return true;
}

void CNinja::ProcessBuildLibs()
{
    if (!hasOptValue(OPT::BUILD_LIBS))
        return;

    ttlib::multistr enumLib(ttlib::findnonspace(getOptValue(OPT::BUILD_LIBS)), ';');
    for (auto& libPath: enumLib)
    {
        ttlib::cwd cwd(true);

        // Change to the directory that should contain a .srcfiles.yaml and read it

        if (!ttlib::ChangeDir(libPath))
        {
            AddError(_tt(IDS_LIB_SOURCE_DIR) + libPath + _tt(IDS_INVALID_BUILDLIB_SUFFIX));
            continue;
        }

        // The current directory may just be the name of the library, but not necessarily where srcfiles is
        // located.

        ttlib::cstr BuildDirectory(libPath);
        ttlib::cstr BuildFile(BuildDirectory);

        auto path = locateProjectFile();
        if (!path.empty())
        {
            ttlib::cstr dir = path;
            dir.remove_filename();
            if (!dir.empty())
            {
                ttlib::ChangeDir(dir);
                BuildDirectory.assignCwd();
            }
            else
            {
                BuildDirectory = ttlib::emptystring;
            }

            BuildFile = BuildDirectory;
            BuildFile.append_filename(path.filename());
            BuildFile.backslashestoforward();
        }
        else
        {
            // empty for loop that we break out of as soon as we find a srcfiles file to use
            for (;;)
            {
                /*
                    It's unusual, but possible for there to be a sub-directory with the same name as the root
                    directory:

                    foo
                        foo -- src for foo.lib
                        bar -- src for bar.lib
                        app -- src for some related app

                */

                if (ttlib::dirExists(libPath.filename()))
                {
                    ttlib::ChangeDir(libPath.filename());
                    path.assign(locateProjectFile(BuildDirectory));
                    if (!path.empty())
                    {
                        BuildDirectory.append_filename(libPath.filename());
                        BuildFile = BuildDirectory;
                        BuildFile.append_filename(path.filename());
                        BuildFile.backslashestoforward();

                        break;
                    }
                    else
                    {
                        // We tried changing into a directory to find the file, that didn't work so we need to
                        // back out.
                        ttlib::ChangeDir("..");
                    }
                }

                // Any further directory searches should go above this -- once we get here, we can't find a
                // .srcfiles.yaml. We go ahead and break out of the loop. cSrcFiles.ReadFile() will fail --
                // we'll use whatever error reporting (if any) it uses for a file that cannot be found or read.

                break;
            }
        }

        // We've actually changed to the directory containing the .srcfiles.yaml, so CSrcFiles doesn't actually
        // need the filename. However, if an error occurs, we need to indicate where the .srcfiles.yaml file is
        // that had the problem.

        CSrcFiles cSrcFiles;
        // At this point, we should be in the same directory as .srcfiles.yaml
        if (!cSrcFiles.ReadFile(BuildFile))
        {
            AddError(_tt(IDS_MISSING_SRCFILES_IN) + BuildFile);
            continue;
        }

        if (cSrcFiles.getErrorMsgs().size())
        {
            for (auto& err: cSrcFiles.getErrorMsgs())
            {
                AddError(BuildFile + ": " + err);
            }
        }

        assertm(!cSrcFiles.GetTargetRelease().empty(), "Must have a release library target");
        assertm(!cSrcFiles.GetTargetDebug().empty(), "Must have a debug library target");

        if (cSrcFiles.GetTargetRelease().empty() || cSrcFiles.GetTargetDebug().empty())
        {
            AddError("Invalid .srcfiles.yaml: " + BuildFile);
            continue;
        }

        auto& bldLib = m_bldLibs.emplace_back();

        bldLib.shortname = cSrcFiles.GetProjectName();

        bldLib.srcDir = BuildFile;
        bldLib.srcDir.make_relative(cwd);
        bldLib.srcDir.remove_filename();
        bldLib.srcDir.backslashestoforward();

        bldLib.libPathDbg.assignCwd();
        bldLib.libPathRel = bldLib.libPathDbg;

        bldLib.libPathDbg.append_filename(cSrcFiles.GetTargetDebug());
        bldLib.libPathDbg.make_relative(cwd);
        bldLib.libPathDbg.backslashestoforward();

        bldLib.libPathRel.append_filename(cSrcFiles.GetTargetRelease());
        bldLib.libPathRel.make_relative(cwd);
        bldLib.libPathRel.backslashestoforward();
    }
}
