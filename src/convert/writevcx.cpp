/////////////////////////////////////////////////////////////////////////////
// Purpose:   Create a Visual Studio project file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <cctype>

#if !defined(_WIN32)
    #error "This module can only be compiled for Windows"
#endif

#include <Rpc.h>
#pragma comment(lib, "Rpcrt4.lib")

#include "ttwinff.h"  // winff -- Wrapper around Windows FindFile

#include "writevcx.h"  // CVcxWrite

const char* res_vcxproj_xml =
#include "res/vcxproj.xml"
    ;

const char* res_vcxproj_filters_xml =
#include "res/vcxproj.filters.xml"
    ;

static bool CreateGuid(ttlib::cstr& Result)
{
    UUID uuid;
    auto ret_val = ::UuidCreate(&uuid);

    if (ret_val == RPC_S_OK)
    {
        RPC_CSTR pszUuid = nullptr;
        if (::UuidToStringA(&uuid, &pszUuid) == RPC_S_OK && pszUuid)
        {
            Result = reinterpret_cast<const char*>(pszUuid);
            ::RpcStringFreeA(&pszUuid);
        }
    }
    return !Result.empty();
}

bool CVcxWrite::CreateBuildFile()
{
    ttlib::cstr guid;
    if (!CreateGuid(guid))
    {
        AddError(_tt(strIdCantCreateUuid));
        return false;
    }

    ttlib::cstr vc_project_file(GetProjectName());
    vc_project_file.replace_extension(".vcxproj");
    if (!vc_project_file.file_exists())
    {
        ttlib::cstr master(ttlib::find_nonspace(res_vcxproj_xml));

        master.Replace("%guid%", guid, true);
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

        if (!out.WriteFile(vc_project_file))
        {
            AddError(_tt(strIdCantWrite) + vc_project_file);
            return false;
        }
        else
            std::cout << _tt(strIdCreated) << vc_project_file << '\n';

        master = ttlib::find_nonspace(res_vcxproj_filters_xml);

        CreateGuid(guid);  // it already succeeded once if we got here, so we don't check for error again
        master.Replace("%guidSrc%", guid, true);
        CreateGuid(guid);
        master.Replace("%guidHdr%", guid, true);
        CreateGuid(guid);
        master.Replace("%guidResource%", guid, true);

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
        vc_project_file << ".filters";
        if (!out.WriteFile(vc_project_file))
        {
            AddError(_tt(strIdCantWrite) + vc_project_file);
            return false;
        }
        else
            std::cout << _tt(strIdCreated) << vc_project_file << '\n';
    }
    else
    {
        // TODO: [KeyWorks - 04-19-2020] Need to support changing the files to compile to match what's in
        // .srcfiles.
        return false;
    }
    return true;
}
