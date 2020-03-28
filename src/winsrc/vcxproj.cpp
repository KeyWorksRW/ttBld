/////////////////////////////////////////////////////////////////////////////
// Name:      CVcxWrite
// Purpose:   Class for creating/maintaining .vcxproj file for use by the msbuild build tool (or VS IDE)
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#if defined(_WIN32)
    #include <Rpc.h>
    #pragma comment(lib, "Rpcrt4.lib")
#endif

#include <ttcwd.h>      // cwd -- Class for storing and optionally restoring the current directory
#include <ttenumstr.h>  // ttlib::enumstr, ttEnumView -- Enumerate through substrings in a string
#include <ttwinff.h>    // winff -- Wrapper around Windows FindFile

#include "resource.h"
#include "vcxproj.h"  // CVcxRead, CVcxWrite

bld::RESULT CVcxRead::Convert(const std::string& srcFile, std::string_view dstFile)
{
    ttlib::cwd cwd;
    m_srcFile.assign(srcFile);
    m_dstFile.assign(dstFile);
    m_srcFile.make_relative(cwd);
    m_dstFile.make_relative(cwd);

#if defined(_WIN32)
    m_srcFile.backslashestoforward();
    m_dstFile.backslashestoforward();
#endif  // _WIN32

    m_srcDir = srcFile;
    m_srcDir.make_absolute();
    m_srcDir.remove_filename();

    m_dstDir = dstFile;
    m_dstDir.make_absolute();
    m_dstDir.remove_filename();

    std::wstring str16;
    ttlib::utf8to16(srcFile, str16);
    // auto result = m_xmldoc.load_file(str16.c_str());
    auto result = m_xmldoc.load_file(srcFile.c_str());

    m_writefile.InitOptions();

    if (!result)
    {
        ttlib::cstr msg;
        ttlib::MsgBox(msg.Format(_tt("Unable to read %s.\n\n%s"), m_srcFile.c_str(), result.description()));
        return bld::RESULT::read_failed;
    }

    auto files = m_xmldoc.select_nodes("/Project/ItemGroup/ClCompile[@Include]");

    for (size_t pos = 0; pos < files.size(); ++pos)
    {
        auto name = files[pos].node().first_attribute().value();
        if (name)
        {
            // The filename will be relative to the location of the xml file, so first we need to make it relative
            // to that. Since the .srcfiles may be in a different location, we then need to make the file relative
            // to that.

            ttlib::cstr filename(name);
            MakeNameRelative(filename);
            m_writefile.GetSrcFileList().addfilename(filename);
        }
    }

    // All Debug| sections are shared, so only need to process one of them.
    bool DebugProcessed = false;  // Same as Debug|, only one gets processed
    bool ReleaseProcessed = false;

    auto configs = m_xmldoc.select_nodes("/Project/ItemDefinitionGroup[@Condition]");
    for (size_t pos = 0; pos < configs.size(); ++pos)
    {
        auto condition = configs[pos].node().first_attribute().value();
        if (!condition)
            continue;  // theoretically impossible
        if (ttlib::contains(condition, "Debug|"))
        {
            if (DebugProcessed)
                continue;
            ProcessDebug(configs[pos].node());
            DebugProcessed = true;
        }
        else if (ttlib::contains(condition, "Release|"))
        {
            if (ReleaseProcessed)
                continue;
            ProcessRelease(configs[pos].node());
            ReleaseProcessed = true;
        }
    }

    // If debug and release libraries are identical, switch them to shared libraries
    if (m_writefile.hasOptValue(OPT::LIBS_DBG) && m_writefile.hasOptValue(OPT::LIBS_REL) &&
        m_writefile.getOptValue(OPT::LIBS_DBG) == m_writefile.getOptValue(OPT::LIBS_REL))
    {
        m_writefile.setOptValue(OPT::LIBS_CMN, m_writefile.getOptValue(OPT::LIBS_DBG));
        m_writefile.setOptValue(OPT::LIBS_DBG, {});
        m_writefile.setOptValue(OPT::LIBS_REL, {});
    }

    return m_writefile.WriteNew(dstFile);
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

void CVcxRead::MakeNameRelative(ttlib::cstr& filename)
{
    filename.make_absolute();
    filename.make_relative(m_srcDir);
    filename.make_absolute();
    filename.make_relative(m_dstDir);
    filename.backslashestoforward();
}

void CVcxRead::ProcessDebug(pugi::xml_node node)
{
    auto compile = node.child("ClCompile");
    if (compile)
    {
        auto val = compile.child("PrecompiledHeader").first_child().value();
        if (val && !ttlib::issameprefix(val, "NotUsing", tt::CASE::either))
            m_writefile.setOptValue(OPT::PCH, val);

        val = compile.child("WarningLevel").first_child().value();
        if (val)
        {
            while (*val && !ttlib::isdigit(*val))
                ++val;
            m_writefile.setOptValue(OPT::WARN, val);
        }

        val = compile.child("AdditionalIncludeDirectories").first_child().value();
        if (val)
        {
            ttlib::cstr incs(val);
            incs.Replace(";%(AdditionalIncludeDirectories)", "");

            ttlib::enumstr enumPaths(incs);
            ttlib::cstr Includes;
            for (auto& filename: enumPaths)
            {
                MakeNameRelative(filename);
                if (!Includes.empty())
                    Includes += ";";
                Includes += filename;
            }
            if (!Includes.empty())
                m_writefile.setOptValue(OPT::INC_DIRS, Includes);
        }

        val = compile.child("PreprocessorDefinitions").first_child().value();
        if (val)
        {
            ttlib::cstr Flags("-D");
            Flags += val;
            Flags.Replace("_DEBUG;", "");
            Flags.Replace("WIN32;", "");
            Flags.Replace("WINDOWS;", "");
            Flags.Replace(";%(PreprocessorDefinitions)", "");
            Flags.Replace(";", " -D", true);
            m_writefile.setOptValue(OPT::CFLAGS_DBG, Flags);
        }
    }

    auto link = node.child("Link");
    if (link)
    {
        auto val = link.child("SubSystem").first_child().value();
        if (val)
        {
            if (ttlib::issameprefix(val, "Console", tt::CASE::either))
                m_writefile.setOptValue(OPT::EXE_TYPE, "console");
        }

        val = link.child("AdditionalDependencies").first_child().value();
        if (val)
        {
            ttlib::cstr libs(val);
            libs.Replace(";%(AdditionalDependencies)", "");

            if (!libs.empty())
                m_writefile.setOptValue(OPT::LIBS_DBG, libs);
        }

        val = link.child("AdditionalOptions").first_child().value();
        if (val)
        {
            ttlib::cstr options(val);
            options.Replace("%(AdditionalOptions)", "");
            if (!options.empty())
                m_writefile.setOptValue(OPT::LINK_DBG, options);
        }
    }
}

void CVcxRead::ProcessRelease(pugi::xml_node node)
{
    auto compile = node.child("ClCompile");
    if (compile)
    {
        auto val = compile.child("FavorSizeOrSpeed").first_child().value();
        if (val && ttlib::issameprefix(val, "Speed", tt::CASE::either))
            m_writefile.setOptValue(OPT::OPTIMIZE, "speed");

        val = compile.child("PreprocessorDefinitions").first_child().value();
        if (val)
        {
            ttlib::cstr Flags("-D");
            Flags += val;
            Flags.Replace("NDEBUG;", "");
            Flags.Replace("WIN32;", "");
            Flags.Replace("WINDOWS;", "");
            Flags.Replace(";%(PreprocessorDefinitions)", "");
            Flags.Replace(";", " -D", true);
            m_writefile.setOptValue(OPT::CFLAGS_DBG, Flags);
        }
    }

    auto link = node.child("Link");
    if (link)
    {
        auto val = link.child("SubSystem").first_child().value();
        if (val)
        {
            if (ttlib::issameprefix(val, "Console", tt::CASE::either))
                m_writefile.setOptValue(OPT::EXE_TYPE, "console");
        }

        val = link.child("AdditionalDependencies").first_child().value();
        if (val)
        {
            ttlib::cstr libs(val);
            libs.Replace(";%(AdditionalDependencies)", "");

            if (!libs.empty())
                m_writefile.setOptValue(OPT::LIBS_REL, libs);
        }

        val = link.child("AdditionalOptions").first_child().value();
        if (val)
        {
            ttlib::cstr options(val);
            options.Replace("%(AdditionalOptions)", "");
            if (!options.empty())
                m_writefile.setOptValue(OPT::LINK_DBG, options);
        }
    }
}
