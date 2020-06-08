/////////////////////////////////////////////////////////////////////////////
// Name:      CVcxWrite
// Purpose:   Create a Visual Studio project file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   MIT License (see %lic_name%)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#if !defined(_WIN32)
    #error "This module can only be compiled for Windows"
#endif

#include <Rpc.h>
#pragma comment(lib, "Rpcrt4.lib")

#include <ttwinff.h>  // winff -- Wrapper around Windows FindFile

#include "../winsrc/resource.h"
#include "writevcx.h"  // CVcxWrite

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
    ttlib::cstr cszGuid;
    if (!CreateGuid(cszGuid))
    {
        AddError(_tt(strIdCantCreateUuid));
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
            AddError(_tt(strIdCantWrite) + cszProjVC);
            return false;
        }
        else
            std::cout << _tt(strIdCreated) << cszProjVC << '\n';

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
            AddError(_tt(strIdCantWrite) + cszProjVC);
            return false;
        }
        else
            std::cout << _tt(strIdCreated) << cszProjVC << '\n';
    }
    else
    {
        // TODO: [KeyWorks - 04-19-2020] Need to support changing the files to compile to match what's in
        // .srcfiles.
        return false;
    }
    return true;
}
