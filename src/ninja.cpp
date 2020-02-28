/////////////////////////////////////////////////////////////////////////////
// Name:      CNinja
// Purpose:   Class for creating/maintaining *.ninja files for use by ninja.exe
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttTR.h>       // Function for translating strings
#include <ttcwd.h>      // Class for storing and optionally restoring the current directory
#include <ttenumstr.h>  // ttCEnumStr

#include "ninja.h"     // CNinja
#include "parsehhp.h"  // CParseHHP
#include "verninja.h"  // CVerMakeNinja

const char* aCppExt[] { ".cpp", ".cxx", ".cc", nullptr };

CNinja::CNinja(std::string_view NinjaDir) : CSrcFiles(NinjaDir)
{
#if !defined(NDEBUG)  // Starts debug section.
    assert(ReadFile());
#else
    if (!ReadFile())
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
            projname.erase(projname.size() - 1);

        if (projname.hasFilename("src"))
        {
            projname.remove_filename();
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

    auto& temp = m_ninjafile.GetTempLine();
    temp += "# WARNING: This file is auto-generated by ";
    temp += txtVersion;
    m_ninjafile.WriteTempLine(".");
    m_ninjafile.push_back("# Changes you make will be lost if it is auto-generated again!");
    m_ninjafile.addblankline();

    // REVIEW: [KeyWorks - 11-19-2019] We don't write any new features, could probably state 1.0 and it would work.
    m_ninjafile.push_back("ninja_required_version = 1.8");
    m_ninjafile.addblankline();

    m_ninjafile.push_back(builddir);
    m_ninjafile.push_back(outdir);
    m_ninjafile.push_back(resout);
    m_ninjafile.addblankline();

    // Figure out the filenames to use for the source and output for a precompiled header

    if (hasOptValue(OPT::PCH))
    {
        m_pchHdrName = GetProjectName();
        m_pchHdrName.replace_extension(".pch");

        m_pchCppName = GetPchCpp();
        m_pchHdrNameObj.assign(m_pchCppName.filename());
        m_pchHdrNameObj.replace_extension(".obj");

        if (!m_pchCppName.fileExists())
        {
            ttlib::cstr msg;
            msg.Format(
                _tt("No C++ source file found that matches %s -- precompiled header will not build correctly."),
                getOptValue(OPT::PCH).c_str());
            AddError(msg);
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

    if (hasOptValue(OPT::PCH))
    {
        temp = m_ninjafile.GetTempLine();
        temp.Format("build $outdir/%s: compilePCH %s", m_pchHdrNameObj.c_str(), m_pchCppName.c_str());
        if (m_lstIdlFiles.size())
        {
            temp += " | $";
            m_ninjafile.WriteTempLine();
            size_t pos;
            ttlib::cstr header;
            for (pos = 0; pos < m_lstIdlFiles.size() - 1; ++pos)
            {
                header.assign(m_lstIdlFiles[pos]);
                header.replace_extension(".h");
                temp.Format("  %s $", header.c_str());
                m_ninjafile.WriteTempLine();
            }
            header.assign(m_lstIdlFiles[pos]);
            header.replace_extension(".h");
            temp.Format("  %s", header.c_str());
            m_ninjafile.WriteTempLine();
        }
        if (!temp.empty())
            m_ninjafile.WriteTempLine();
        m_ninjafile.addblankline();
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
            m_ninjafile.GetTempLine().Format("build $outdir/%s: compile %s | $outdir/%s", objFile.c_str(),
                                             srcFile.c_str(), m_pchHdrNameObj.c_str());
            m_ninjafile.WriteTempLine();
            m_ninjafile.addblankline();
        }
        else
        {
            // We get here if we don't have a precompiled header. We might have .idl files, which means we're going
            // to need to add all the midl-generated header files as dependencies to each source m_ninjafile. See
            // issue #80 for details.

            m_ninjafile.GetTempLine().Format("build $outdir/%s: compile %s", objFile.c_str(), srcFile.c_str());
            if (m_lstIdlFiles.size())
            {
                m_ninjafile.WriteTempLine(" | $");
                size_t pos;
                ttlib::cstr header;
                for (pos = 0; pos < m_lstIdlFiles.size() - 1; ++pos)
                {
                    header.assign(m_lstIdlFiles[pos]);
                    header.replace_extension(".h");
                    m_ninjafile.GetTempLine().Format("  %s $", header.c_str());
                    m_ninjafile.WriteTempLine();
                }
                header.assign(m_lstIdlFiles[pos]);
                header.replace_extension(".h");
                // write the last one without the trailing pipe
                m_ninjafile.GetTempLine().Format("  %s", header.c_str());
            }
            m_ninjafile.WriteTempLine();
            m_ninjafile.addblankline();
        }
    }

    // Write the build rule for the resource compiler if an .rc file was specified as a source

    if (GetRcFile().fileExists())
    {
        ttlib::cstr resource { GetRcFile() };

        resource.replace_extension("");
        resource += ((m_gentype == GEN_DEBUG) ? "D.res" : ".res");

        m_ninjafile.GetTempLine().Format("build $resout/%s: rc %s", resource.c_str(), GetRcFile().c_str());

        if (m_RcDependencies.size())
        {
            m_ninjafile.WriteTempLine(" | $");
            size_t pos = 0;
            for (; pos < m_RcDependencies.size() - 1; pos++)
            {
                temp = ("  " + m_RcDependencies[pos]);
                m_ninjafile.WriteTempLine(" $");
            }
            m_ninjafile.push_back("  " + m_RcDependencies[pos]);
        }
        m_ninjafile.addblankline();
    }

    // Write the final build rules to complete the project

    if (cmplr == CMPLR_MSVC || cmplr == CMPLR_CLANG)
    {
        msvcWriteMidlTargets(cmplr);
        msvcWriteLinkTargets(cmplr);
    }

    if (!GetBldDir().dirExists())
    {
        if (!ttCreateDir(GetBldDir().c_str()))
        {
            AddError(_tt("Unable to create or write to ") + GetBldDir());
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
        std::string str(_tt("Unable to create or write to") + m_scriptFilename + '\n');
        AddError(str);
        return false;
    }

    return true;
}

void CNinja::ProcessBuildLibs()
{
    if (hasOptValue(OPT::BUILD_LIBS))
    {
        ttEnumStr enumLib(ttlib::findnonspace(getOptValue(OPT::BUILD_LIBS)), ';');
        for (auto iter: enumLib)
        {
            ttlib::cwd cwd(true);

            // Change to the directory that should contain a .srcfiles.yaml and read it

            if (!ttlib::ChangeDir(iter))
            {
                std::stringstream msg;
                msg << _tt("The library source directory ") << iter
                    << _tt(" specified in BuildLibs: does not exist.");
                AddError(msg);
                continue;
            }

            // The current directory may just be the name of the library, but not necessarily where srcfiles is
            // located.

            ttlib::cstr BuildDirectory(iter);
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

                BuildFile = BuildDirectory;
                BuildFile.append_filename(path);
            }
            else
            {
                for (;;)  // empty for loop that we break out of as soon as we find a srcfiles file to use
                {
                    /*
                        It's unusual, but possible for there to be a sub-directory with the same name as the root
                        directory:

                        foo
                            foo -- src for foo.lib
                            bar -- src for bar.lib
                            app -- src for some related app

                    */

                    if (ttlib::dirExists(iter.filename()))
                    {
                        ttlib::ChangeDir(iter.filename());
                        path = std::move(locateProjectFile(BuildDirectory));
                        if (!path.empty())
                        {
                            BuildDirectory.append_filename(iter.filename());
                            BuildFile = BuildDirectory;
                            BuildFile.append_filename(path);
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
            cSrcFiles.SetReportingFile(BuildFile);
            // At this point, we should be in the same directory as .srcfiles.yaml
            if (cSrcFiles.ReadFile(".srcfiles.yaml"))
            {
#if 0
                // REVIEW: [KeyWorks - 02-03-2020] Could we just all CurDir.make_relative()?

                ttlib::cstr RelDir;
                RelDir.assignCwd();
                RelDir.make_relative(cwd);
                RelDir.backslashestoforward();
    #if 1
                // REVIEW: [KeyWorks - 02-26-2020] Once we're confident the result is identical, remove
                // this conditional code block
                ttlib::cstr CurDir;
                CurDir.assignCwd();
                ttCStr cszRelDir;
                ttConvertToRelative(cwd.c_str(), CurDir.c_str(), cszRelDir);
                ttASSERT_MSG(ttlib::issameas(RelDir, cszRelDir.c_str()),
                             "RelDir and cszRelDir should be identical!");
    #endif
                m_dlstTargetDir.Add(cSrcFiles.GetProjectName().c_str(), RelDir.c_str());
#endif
            }

            auto targetDir = cSrcFiles.GetTargetDebug();
            if (!targetDir.empty())
            {
                ttlib::cstr LibDir;
                LibDir.assignCwd();
                LibDir.append_filename(targetDir);
                LibDir.make_relative(cwd);
                LibDir.backslashestoforward();
                m_BldLibsDbg.addfilename(LibDir);
            }

            targetDir = cSrcFiles.GetTargetRelease();
            if (!targetDir.empty())
            {
                ttlib::cstr LibDir;
                LibDir.assignCwd();
                LibDir.append_filename(targetDir);
                LibDir.make_relative(cwd);
                LibDir.backslashestoforward();
                m_lstBldLibsRel.addfilename(LibDir);
            }
        }
    }
}
