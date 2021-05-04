/////////////////////////////////////////////////////////////////////////////
// Purpose:   Class for creating/maintaining *.ninja files for use by ninja.exe
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <cctype>

#include <wx/arrstr.h>  // wxArrayString class
#include <wx/dir.h>     // wxDir is a class for enumerating the files in a directory

#include "ttcwd.h"       // Class for storing and optionally restoring the current directory
#include "ttmultistr.h"  // multistr -- Breaks a single string into multiple strings

#include "ninja.h"     // CNinja
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

        if (projname.has_filename("src"))
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
    if (hasOptValue(OPT::TARGET_DIR32))
        ProcessBuildLibs32();
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

    // Note that resout goes to the same directory in all builds. The actual filename will have a 'D' appended for debug
    // builds. Currently, 32 and 64 bit builds of the resource file are identical.

    ttlib::cstr resout("resout = ");
    resout += GetBldDir();
    resout.append_filename("res");

    ttlib::cstr builddir("builddir = ");
    builddir += GetBldDir();

    ttlib::cstr outdir("outdir = ");
    outdir += GetBldDir();
    outdir.addtrailingslash();
    outdir += aszCompilerPrefix[cmplr];

    m_scriptFilename = GetBldDir();
    m_scriptFilename.backslashestoforward();
    m_scriptFilename.addtrailingslash();
    m_scriptFilename += aszCompilerPrefix[cmplr];

    switch (gentype)
    {
        case GEN_DEBUG:
            outdir += "Debug";
            m_scriptFilename += "dbg.ninja";
            break;

        case GEN_DEBUG32:
            outdir += "Debug32";
            m_scriptFilename += "dbg32.ninja";
            break;

        case GEN_RELEASE32:
            outdir += "Release32";
            m_scriptFilename += "rel32.ninja";
            break;

        case GEN_RELEASE:
        default:
            outdir += "Release";
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

    if (HasPch() && !GetPchCpp().is_sameprefix("none"))
    {
        m_pchHdrName = GetPchCpp();
        m_pchHdrName.replace_extension(".pch");

        m_pchCppName = GetPchCpp();
        m_pchHdrNameObj.assign(m_pchCppName.filename());
        m_pchHdrNameObj.replace_extension(".obj");

        if (!m_pchCppName.file_exists())
        {
            AddError(getOptValue(OPT::PCH) + _tt(strIdMissingPchCpp));
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

    if (m_gzip_files.size())
    {
        m_ninjafile.emplace_back("rule gzipHeader");
        m_ninjafile.emplace_back("  command = ttBld -hgz $out $in");
        m_ninjafile.emplace_back("  description = converting $in into $out");
        m_ninjafile.addEmptyLine();
    }

    if (m_xpm_files.size())
    {
        m_ninjafile.emplace_back("rule xpmConversion");
        m_ninjafile.emplace_back("  command = ttBld -xpm $in $out");
        m_ninjafile.emplace_back("  description = converting $in into $out");
        m_ninjafile.addEmptyLine();
    }

    if (m_png_files.size())
    {
        m_ninjafile.emplace_back("rule pngConversion");
        m_ninjafile.emplace_back("  command = ttBld -png $in $out");
        m_ninjafile.emplace_back("  description = converting $in into $out");
        m_ninjafile.addEmptyLine();
    }

    if (m_gzip_files.size())
    {
        for (auto& iter: m_gzip_files)
        {
            if (iter.first.contains("*") || iter.first.contains("?"))
            {
                wxDir dir;
                wxArrayString files;
                ttlib::cstr in_directory(iter.first);
                in_directory.remove_filename();
                if (in_directory.empty())
                    in_directory = ".";
                dir.GetAllFiles(in_directory.wx_str(), &files, iter.first.filename().wx_str(), wxDIR_FILES);

                if (files.size())
                {
                    auto& ninja_line = m_ninjafile.addEmptyLine();
                    ninja_line << "build " << iter.second << ": gzipHeader";
                    for (size_t pos_file = 0; pos_file < files.size(); ++pos_file)
                    {
                        ninja_line << ' ' << files[pos_file].wx_str();
                    }
                    ninja_line.backslashestoforward();
                }
                else
                {
                    AddError(_tt("No files found matching ") + iter.first);
                }
            }
            else
            {
                m_ninjafile.addEmptyLine().Format("build %s: gzipHeader %s", iter.second.c_str(), iter.first.c_str());
            }
            m_ninjafile.addEmptyLine();
        }
    }

    if (m_xpm_files.size())
    {
        for (auto& iter: m_xpm_files)
        {
            m_ninjafile.addEmptyLine().Format("build %s: xpmConversion %s", iter.second.c_str(), iter.first.c_str());
            m_ninjafile.addEmptyLine();
        }
    }

    if (m_png_files.size())
    {
        for (auto& iter: m_png_files)
        {
            m_ninjafile.addEmptyLine().Format("build %s: pngConversion %s", iter.second.c_str(), iter.first.c_str());
            m_ninjafile.addEmptyLine();
        }
    }

    // If the project has a .idl file, then the midl compiler will create a matching header file that will be included in one
    // or more source files. If the .idl file changes, or the header file doesn't exist yet, then we need to run the midl
    // compiler before compiling any source files. If we knew ahead of time which source files included the header file, then
    // we could create a dependency. However, that would essentially require an accurate C/C++ preprocessor to run on every
    // source file which is far beyond the scope of this project. Instead, we add the dependency to the precompiled header if
    // there is one, and if not, we add the dependency to every source m_ninjafile. Unfortunately that does mean that every
    // time the .idl file changes, then every source file will get rebuilt whether or not a particular source file actually
    // uses the generated header m_ninjafile.

    if (HasPch())
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

    for (auto& srcFile: m_lstSrcFiles)
    {
        auto ext = srcFile.extension();
        if (ext.empty() || std::tolower(ext[1] != 'c'))
            continue;
        if (srcFile.is_sameas(m_pchCppName))
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
            // We get here if we don't have a precompiled header. We might have .idl files, which means we're going to need
            // to add all the midl-generated header files as dependencies to each source m_ninjafile. See issue #80 for
            // details.

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

    if ((gentype == GEN_DEBUG || gentype == GEN_DEBUG32) && m_lstDebugFiles.size())
    {
        for (auto& srcFile: m_lstDebugFiles)
        {
            auto ext = srcFile.extension();
            if (ext.empty() || std::tolower(ext[1] != 'c'))
                continue;
            if (srcFile.is_sameas(m_pchCppName))
                continue;

            ttlib::cstr objFile(srcFile.filename());
            objFile.replace_extension(".obj");

            if (!m_pchHdrNameObj.empty())
            {
                // we add m_pchHdrNameObj so it appears as a dependency and gets compiled, but not linked to
                m_ninjafile.addEmptyLine().Format("build $outdir/%s: compile %s | $outdir/%s", objFile.c_str(),
                                                  srcFile.c_str(), m_pchHdrNameObj.c_str());
                m_ninjafile.addEmptyLine();
            }
            else
            {
                m_ninjafile.addEmptyLine().Format("build $outdir/%s: compile %s", objFile.c_str(), srcFile.c_str());
                m_ninjafile.addEmptyLine();
            }
        }
    }

    // Write the build rule for the resource compiler if an .rc file was specified as a source

    if (GetRcFile().file_exists())
    {
        ttlib::cstr resource { GetRcFile() };

        resource.replace_extension("");
        resource += ((m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG32) ? "D.res" : ".res");

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

    if (!GetBldDir().dir_exists())
    {
        if (!fs::create_directory(GetBldDir().c_str()))
        {
            AddError(_tt(strIdCantWrite) + GetBldDir());
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

        if (fileOrg.is_sameas(m_ninjafile))
        {
            return false;  // nothing changed
        }
    }

    if (!m_ninjafile.WriteFile(m_scriptFilename))
    {
        m_ninjafile.clear();
        std::string str(_tt(strIdIgnoredFiles) + m_scriptFilename + '\n');
        AddError(str);
        return false;
    }

    return true;
}

void CNinja::ProcessBuildLibs()
{
    if (!hasOptValue(OPT::BUILD_LIBS))
        return;

    ttlib::multistr enumLib(ttlib::find_nonspace(getOptValue(OPT::BUILD_LIBS)), ';');
    for (auto& libPath: enumLib)
    {
        ttlib::cwd cwd(true);

        // Change to the directory that should contain a .srcfiles.yaml and read it

        if (!ttlib::ChangeDir(libPath))
        {
            AddError(_tt(strIdLibSrcDir) + libPath + _tt(strIdInvalidBuildlib));
            continue;
        }

        // The current directory may just be the name of the library, but not necessarily where srcfiles is located.

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

                if (ttlib::dir_exists(libPath.filename()))
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
                        // We tried changing into a directory to find the file, that didn't work so we need to back out.
                        ttlib::ChangeDir("..");
                    }
                }

                // Any further directory searches should go above this -- once we get here, we can't find a .srcfiles.yaml.
                // We go ahead and break out of the loop. cSrcFiles.ReadFile() will fail -- we'll use whatever error
                // reporting (if any) it uses for a file that cannot be found or read.

                break;
            }
        }

        // We've actually changed to the directory containing the .srcfiles.yaml, so CSrcFiles doesn't actually need the
        // filename. However, if an error occurs, we need to indicate where the .srcfiles.yaml file is that had the problem.

        CSrcFiles cSrcFiles;
        // At this point, we should be in the same directory as .srcfiles.yaml
        if (!cSrcFiles.ReadFile(BuildFile))
        {
            AddError(_tt(strIdMissingSrcfilesIn) + BuildFile);
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

void CNinja::ProcessBuildLibs32()
{
    if (!hasOptValue(OPT::BUILD_LIBS32))
        return;

    ttlib::multistr enumLib(ttlib::find_nonspace(getOptValue(OPT::BUILD_LIBS32)), ';');
    for (auto& libPath: enumLib)
    {
        ttlib::cwd cwd(true);

        // Change to the directory that should contain a .srcfiles.yaml and read it

        if (!ttlib::ChangeDir(libPath))
        {
            AddError(_tt(strIdLibSrcDir) + libPath + _tt(strIdInvalidBuildlib));
            continue;
        }

        // The current directory may just be the name of the library, but not necessarily where srcfiles is located.

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

                if (ttlib::dir_exists(libPath.filename()))
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
                        // We tried changing into a directory to find the file, that didn't work so we need to back out.
                        ttlib::ChangeDir("..");
                    }
                }

                // Any further directory searches should go above this -- once we get here, we can't find a .srcfiles.yaml.
                // We go ahead and break out of the loop. cSrcFiles.ReadFile() will fail -- we'll use whatever error
                // reporting (if any) it uses for a file that cannot be found or read.

                break;
            }
        }

        // We've actually changed to the directory containing the .srcfiles.yaml, so CSrcFiles doesn't actually need the
        // filename. However, if an error occurs, we need to indicate where the .srcfiles.yaml file is that had the problem.

        CSrcFiles cSrcFiles;
        // At this point, we should be in the same directory as .srcfiles.yaml
        if (!cSrcFiles.ReadFile(BuildFile))
        {
            AddError(_tt(strIdMissingSrcfilesIn) + BuildFile);
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

        if (cSrcFiles.GetTargetRelease32().empty() || cSrcFiles.GetTargetDebug32().empty())
        {
            AddError("Invalid .srcfiles.yaml: " + BuildFile);
            continue;
        }

        auto& bldLib = m_bldLibs32.emplace_back();

        bldLib.shortname = cSrcFiles.GetProjectName();

        bldLib.srcDir = BuildFile;
        bldLib.srcDir.make_relative(cwd);
        bldLib.srcDir.remove_filename();
        bldLib.srcDir.backslashestoforward();

        bldLib.libPathDbg.assignCwd();
        bldLib.libPathRel = bldLib.libPathDbg;

        bldLib.libPathDbg.append_filename(cSrcFiles.GetTargetDebug32());
        bldLib.libPathDbg.make_relative(cwd);
        bldLib.libPathDbg.backslashestoforward();

        bldLib.libPathRel.append_filename(cSrcFiles.GetTargetRelease32());
        bldLib.libPathRel.make_relative(cwd);
        bldLib.libPathRel.backslashestoforward();
    }
}
