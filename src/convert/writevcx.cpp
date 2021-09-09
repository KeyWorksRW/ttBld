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

#if 0
static bool UpdateBuildFile(ttlib::cstr /* filename */)
{
    return false;
}
#endif

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
        AddError("Unable to create a UUID -- cannot create .vcxproj without it.");
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
    pugi::xml_node child;
    pugi::xml_node PropGroup;
    m_Project = &Project;

    Project.append_attribute("DefaultTargets").set_value("Build");
    Project.append_attribute("ToolsVersion").set_value("4.0");
    Project.append_attribute("xmlns").set_value("http://schemas.microsoft.com/developer/msbuild/2003");

    // Add Build types

    auto ItemGroup = Project.append_child("ItemGroup");
    ItemGroup.append_attribute("Label").set_value("ProjectConfigurations");

    AddConfiguration(ItemGroup, GEN_DEBUG);
    AddConfiguration(ItemGroup, GEN_RELEASE);

    if (hasOptValue(OPT::TARGET_DIR32))
    {
        AddConfiguration(ItemGroup, GEN_DEBUG32);
        AddConfiguration(ItemGroup, GEN_RELEASE32);
    }

    PropGroup = Project.append_child("PropertyGroup");
    PropGroup.append_attribute("Label").set_value("Globals");
    {
        ttlib::cstr gd;
        gd << '{' << guid << '}';
        PropGroup.append_child("ProjectGuid").text().set(gd.c_str());
        // PropGroup.append_child("Keyword").text().set("Win32Proj");
        PropGroup.append_child("ProjectName").text().set(GetProjectName().c_str());
    }

    auto Import = Project.append_child("Import");
    Import.append_attribute("Project").set_value("$(VCTargetsPath)\\Microsoft.Cpp.Default.props");

    AddConfigAppType(Project, GEN_DEBUG);
    AddConfigAppType(Project, GEN_RELEASE);

    if (hasOptValue(OPT::TARGET_DIR32))
    {
        AddConfigAppType(Project, GEN_DEBUG32);
        AddConfigAppType(Project, GEN_RELEASE32);
    }

    Import = Project.append_child("Import");
    Import.append_attribute("Project").set_value("$(VCTargetsPath)\\Microsoft.Cpp.props");

    PropGroup = Project.append_child("PropertyGroup");

    AddOutDirs(PropGroup, GEN_DEBUG);
    AddOutDirs(PropGroup, GEN_RELEASE);

    if (hasOptValue(OPT::TARGET_DIR32))
    {
        AddOutDirs(PropGroup, GEN_DEBUG32);
        AddOutDirs(PropGroup, GEN_RELEASE32);
    }

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
        ttlib::cstr header(iter);
        header.replace_extension(".h");
        bool header_found = header.file_exists();
        for (; !header_found;)
        {
            header.replace_extension(".hh");
            header_found = header.file_exists();
            if (header_found)
                break;

            header.replace_extension(".hpp");
            header_found = header.file_exists();
            if (header_found)
                break;

            header.replace_extension(".hxx");
            header_found = header.file_exists();

            break;
        }

        if (header_found)
        {
            auto ClInclude = ItemGroup.append_child("ClInclude");
            ClInclude.append_attribute("Include").set_value(header.c_str());
        }
    }
    if (HasPch())
    {
        auto ClInclude = ItemGroup.append_child("ClInclude");
        ClInclude.append_attribute("Include").set_value(getOptValue(OPT::PCH).c_str());
    }

    ItemGroup = Project.append_child("ItemGroup");
    for (auto& iter: GetSrcFileList())
    {
        if (iter.is_sameas(GetRcFile()))
            continue;
        ClCompile = ItemGroup.append_child("ClCompile");
        ClCompile.append_attribute("Include").set_value(iter.c_str());
    }

    if (GetPchCpp().size())
    {
        ClCompile = ItemGroup.append_child("ClCompile");
        ClCompile.append_attribute("Include").set_value(GetPchCpp().c_str());
        child = ClCompile.append_child("PrecompiledHeader");
        child.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Debug|x64'");
        child.text().set("Create");
        child = ClCompile.append_child("PrecompiledHeader");
        child.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Release|x64'");
        child.text().set("Create");
        if (hasOptValue(OPT::TARGET_DIR32))
        {
            child = ClCompile.append_child("PrecompiledHeader");
            child.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Debug|Win32'");
            child.text().set("Create");
            child = ClCompile.append_child("PrecompiledHeader");
            child.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Release|Win32'");
            child.text().set("Create");
        }
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
        std::cout << "Unable to create or write to " + vc_project_file << '\n';
        return false;
    }
    else
    {
        std::cout << "Created " << vc_project_file << '\n';
    }

    return CreateFilterFile(vc_project_file);
}

bool CVcxWrite::CreateFilterFile(ttlib::cstr vc_project_file)
{
    vc_project_file.remove_extension();
    vc_project_file += ".vcxproj.filters";

    pugi::xml_document doc;

    auto Project = doc.append_child("Project");
    Project.append_attribute("ToolsVersion").set_value("4.0");
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
        Filter.append_child("Extensions").text().set("cpp;c;cc;cxx");
    }

    Filter = ItemGroup.append_child("Filter");
    Filter.append_attribute("Include").set_value("Header Files");
    {
        ttlib::cstr guid;
        CreateGuid(guid);
        ttlib::cstr gd;
        gd << '{' << guid << '}';
        Filter.append_child("UniqueIdentifier").text().set(gd.c_str());
        Filter.append_child("Extensions").text().set("h;hh;hpp;hxx");
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
        std::cout << "Unable to create or write to " + vc_project_file << '\n';
        return false;
    }
    else
    {
        std::cout << "Created " << vc_project_file << '\n';
    }

    return true;
}

void CVcxWrite::AddConfiguration(pugi::xml_node parent, GEN_TYPE gentype)
{
    auto configure = parent.append_child("ProjectConfiguration");
    switch (gentype)
    {
        case GEN_DEBUG:
            configure.append_attribute("Include").set_value("Debug|x64");
            configure.append_child("Configuration").text().set("Debug");
            configure.append_child("Platform").text().set("x64");
            break;

        case GEN_DEBUG32:
            configure.append_attribute("Include").set_value("Debug|Win32");
            configure.append_child("Configuration").text().set("Debug");
            configure.append_child("Platform").text().set("Win32");
            break;

        case GEN_RELEASE:
            configure.append_attribute("Include").set_value("Release|x64");
            configure.append_child("Configuration").text().set("Release");
            configure.append_child("Platform").text().set("x64");
            break;

        case GEN_RELEASE32:
            configure.append_attribute("Include").set_value("Release|Win32");
            configure.append_child("Configuration").text().set("Release");
            configure.append_child("Platform").text().set("Win32");
            break;

        default:
            break;
    }
}

void CVcxWrite::AddConfigAppType(pugi::xml_node parent, GEN_TYPE gentype)
{
    std::string type("Application");
    if (getOptValue(OPT::EXE_TYPE).is_sameas("lib"))
        type = "Static Library";
    if (getOptValue(OPT::EXE_TYPE).is_sameas("dll"))
        type = "Dynamic Library";

    auto PropGroup = parent.append_child("PropertyGroup");

    switch (gentype)
    {
        case GEN_DEBUG:
            PropGroup.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Debug|x64'");
            break;

        case GEN_DEBUG32:
            PropGroup.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Debug|Win32'");
            break;

        case GEN_RELEASE:
            PropGroup.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Release|x64'");
            break;

        case GEN_RELEASE32:
            PropGroup.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Release|Win32'");
            break;

        default:
            break;
    }

    PropGroup.append_attribute("Label").set_value("Configuration");
    PropGroup.append_child("ConfigurationType").text().set(type.c_str());

    if (gentype == GEN_DEBUG || gentype == GEN_DEBUG32)
    {
        PropGroup.append_child("UseDebugLibraries").text().set("true");
    }
    else
    {
        PropGroup.append_child("UseDebugLibraries").text().set("false");
        PropGroup.append_child("WholeProgramOptimization").text().set("true");
    }
    PropGroup.append_child("PlatformToolset").text().set("v142");
}

void CVcxWrite::AddOutDirs(pugi::xml_node parent, GEN_TYPE gentype)
{
    pugi::xml_node dir;

    switch (gentype)
    {
        case GEN_DEBUG:
            dir = parent.append_child("OutDir");
            dir.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Debug|x64'");
            dir.text().set("bld\\msvc_Debug\\");
            dir = parent.append_child("IntDir");
            dir.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Debug|x64'");
            dir.text().set("bld\\msvc_Debug\\");
            break;

        case GEN_DEBUG32:
            dir = parent.append_child("OutDir");
            dir.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Debug|Win32'");
            dir.text().set("bld\\msvc_Debug32\\");
            dir = parent.append_child("IntDir");
            dir.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Debug|Win32'");
            dir.text().set("bld\\msvc_Debug32\\");
            break;

        case GEN_RELEASE:
            dir = parent.append_child("OutDir");
            dir.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Release|x64'");
            dir.text().set("bld\\msvc_Release\\");
            dir = parent.append_child("IntDir");
            dir.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Release|x64'");
            dir.text().set("bld\\msvc_Release\\");
            break;

        case GEN_RELEASE32:
            dir = parent.append_child("OutDir");
            dir.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Release|Win32'");
            dir.text().set("bld\\msvc_Release32\\");
            dir = parent.append_child("IntDir");
            dir.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Release|Win32'");
            dir.text().set("bld\\msvc_Release32\\");
            break;

        default:
            break;
    }
}
