/////////////////////////////////////////////////////////////////////////////
// Purpose:   Create a Visual Studio project file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <cctype>

#if defined(_WIN32)
    #include <Rpc.h>
    #pragma comment(lib, "Rpcrt4.lib")

#elif defined(__WXMAC__)
    #include <array>
    #include <stdio.h>

    #include <CoreFoundation/CFUUID.h>

#else
    #include <array>
    #include <stdio.h>

    #include <uuid/uuid.h>

#endif  // _WIN32

#include "writevcx.h"  // CVcxWrite

const char* res_vcxproj_xml =
#include "res/vcxproj.xml"
    ;

const char* res_vcxproj_filters_xml =
#include "res/vcxproj.filters.xml"
    ;

static bool CreateGuid(ttlib::cstr& Result)
{
#if defined(_WIN32)

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

#elif defined(__WXMAC__)
    auto newId = CFUUIDCreate(NULL);
    auto bytes = CFUUIDGetUUIDBytes(newId);
    CFRelease(newId);

    std::array<unsigned char, 16> byteArray = {
        { bytes.byte0, bytes.byte1, bytes.byte2, bytes.byte3, bytes.byte4, bytes.byte5, bytes.byte6, bytes.byte7,
          bytes.byte8, bytes.byte9, bytes.byte10, bytes.byte11, bytes.byte12, bytes.byte13, bytes.byte14, bytes.byte15 }
    };

#else
    std::array<unsigned char, 16> byteArray;
    uuid_generate(byteArray.data());
#endif

#if !defined(_WIN32)
    char one[10], two[6], three[6], four[6], five[14];

    snprintf(one, 10, "%02x%02x%02x%02x", _bytes[0], _bytes[1], _bytes[2], _bytes[3]);
    snprintf(two, 6, "%02x%02x", _bytes[4], _bytes[5]);
    snprintf(three, 6, "%02x%02x", _bytes[6], _bytes[7]);
    snprintf(four, 6, "%02x%02x", _bytes[8], _bytes[9]);
    snprintf(five, 14, "%02x%02x%02x%02x%02x%02x", _bytes[10], _bytes[11], _bytes[12], _bytes[13], _bytes[14], _bytes[15]);

    const std::string sep("-");

    Result = one;
    Result += sep + two;
    Result += sep + three;
    Result += sep + four;
    Result += sep + five;
#endif

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

#if 0
// REVIEW: [KeyWorks - 06-22-2021] This won't work -- it adds files that might not be part of the project, leaves out header files in sub-directories, and requires a .h extension.

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
#endif

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
