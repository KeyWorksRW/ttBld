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

#include "uifuncs.h"  // Miscellaneous functions for displaying UI

#include "..\pugixml\pugixml.hpp"

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

static bool UpdateBuildFile(ttlib::cstr filename)
{
    return false;
}

bool CVcxWrite::CreateBuildFile()
{
    if (!IsProcessed())
    {
        // TODO: [KeyWorks - 06-22-2021] We should use a file open dialog to search for it.
        appMsgBox("Unable to locate .srcfiles.yaml");
        return false;
    }

    ttlib::cstr guid;
    if (!CreateGuid(guid))
    {
        AddError(_tt(strIdCantCreateUuid));
        return false;
    }

    ttlib::cstr vc_project_file(GetProjectName());
    vc_project_file.replace_extension(".vcxproj");

#if 0
    if (vc_project_file.file_exists())
        return UpdateBuildFile(vc_project_file);
#endif

    pugi::xml_document doc;

    auto Project = doc.append_child("Project");
    Project.append_attribute("DefaultTargets").set_value("Build");
    Project.append_attribute("ToolsVersion").set_value("15.0");
    Project.append_attribute("xmlns").set_value("http://schemas.microsoft.com/developer/msbuild/2003");

    auto ItemGroup = Project.append_child("ItemGroup");
    ItemGroup.append_attribute("Label").set_value("ProjectConfigurations");

    auto child = ItemGroup.append_child("ProjectConfiguration");
    child.append_attribute("Include").set_value("Debug|x64");
    child.append_child("Configuration").text().set("Debug");
    child.append_child("Platform").text().set("x64");

    child = ItemGroup.append_child("ProjectConfiguration");
    child.append_attribute("Include").set_value("Release|x64");
    child.append_child("Configuration").text().set("Release");
    child.append_child("Platform").text().set("x64");

    if (hasOptValue(OPT::TARGET_DIR32))
    {
        child = ItemGroup.append_child("ProjectConfiguration");
        child.append_attribute("Include").set_value("Debug|Win32");
        child.append_child("Configuration").text().set("Debug");
        child.append_child("Platform").text().set("Win32");

        child = ItemGroup.append_child("ProjectConfiguration");
        child.append_attribute("Include").set_value("Release|Win32");
        child.append_child("Configuration").text().set("Release");
        child.append_child("Platform").text().set("Win32");
    }

    auto PropGroup = Project.append_child("PropertyGroup");
    PropGroup.append_attribute("Label").set_value("Globals");
    {
        ttlib::cstr gd;
        gd << '{' << guid << '}';
        PropGroup.append_child("ProjectGuid").text().set(gd.c_str());
        PropGroup.append_child("Keyword").text().set("Win32Proj");
        PropGroup.append_child("ProjectName").text().set(GetProjectName().c_str());
    }

    auto Import = Project.append_child("Import");
    Import.append_attribute("Project").set_value("$(VCTargetsPath)\\Microsoft.Cpp.Default.props");

    PropGroup = Project.append_child("PropertyGroup");
    PropGroup.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Debug|x64'");
    PropGroup.append_attribute("Label").set_value("Configuration");
    PropGroup.append_child("ConfigurationType").text().set("Application");

    PropGroup = Project.append_child("PropertyGroup");
    PropGroup.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Release|x64'");
    PropGroup.append_attribute("Label").set_value("Configuration");
    PropGroup.append_child("ConfigurationType").text().set("Application");

    auto ItemDef = Project.append_child("ItemDefinitionGroup");
    ItemDef.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Debug|x64'");
    auto ClCompile = ItemDef.append_child("ClCompile");
    ClCompile.append_child("Optimization").text().set("Disabled");
    ClCompile.append_child("RuntimeLibrary").text().set("MultiThreadedDebugDLL");

    auto Link = ItemDef.append_child("Link");
    Link.append_child("GenerateDebugInformation").text().set("true");

    ItemGroup = Project.append_child("ItemGroup");
    for (auto& iter: GetSrcFileList())
    {
        if (iter.is_sameas(GetRcFile()))
            continue;
        ClCompile = ItemGroup.append_child("ClCompile");
        ClCompile.append_attribute("Include").set_value(iter.c_str());
    }

    if (GetRcFile().size())
    {
        ItemGroup = Project.append_child("ItemGroup");
        auto ResourceCompile = ItemGroup.append_child("ResourceCompile");
        ResourceCompile.append_attribute("Include").set_value(GetRcFile().c_str());
    }

    Import = Project.append_child("Import");
    Import.append_attribute("Project").set_value("$(VCTargetsPath)\\Microsoft.Cpp.targets");

    if (!doc.save_file(vc_project_file.c_str()))
    {
        std::cout << _tt(strIdCantWrite) + vc_project_file << '\n';
        return false;
    }
    else
    {
        std::cout << _tt(strIdCreated) << vc_project_file << '\n';
    }

    return CreateFilterFile(vc_project_file);
}

bool CVcxWrite::CreateFilterFile(ttlib::cstr vc_project_file)
{
    vc_project_file.remove_extension();
    vc_project_file += ".vcxproj.filters";

    pugi::xml_document doc;

    auto Project = doc.append_child("Project");
    Project.append_attribute("ToolsVersion").set_value("15.0");
    Project.append_attribute("xmlns").set_value("http://schemas.microsoft.com/developer/msbuild/2003");

    auto ItemGroup = Project.append_child("ItemGroup");
    auto Filter = ItemGroup.append_child("Filter");
    Filter.append_attribute("Include").set_value("Source Files");
    {
        ttlib::cstr guid;
        CreateGuid(guid);
        ttlib::cstr gd;
        gd << '{' << guid << '}';
        Filter.append_child("UniqueIdentifier").text().set(gd.c_str());
        Filter.append_child("Extensions").text().set("cpp;c;cc;cxx;def;odl;idl;hpj;bat;asm;asmx");
    }

    ItemGroup = Project.append_child("ItemGroup");
    for (auto& iter: GetSrcFileList())
    {
        if (iter.is_sameas(GetRcFile()))
            continue;
        auto ClCompile = ItemGroup.append_child("ClCompile");
        ttlib::cstr tmp("Source Files\\");
        tmp += iter;
        ClCompile.append_attribute("Include").set_value(tmp.c_str());
    }

    if (!doc.save_file(vc_project_file.c_str()))
    {
        std::cout << _tt(strIdCantWrite) + vc_project_file << '\n';
        return false;
    }
    else
    {
        std::cout << _tt(strIdCreated) << vc_project_file << '\n';
    }

    return true;
}
