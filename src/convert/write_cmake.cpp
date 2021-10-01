/////////////////////////////////////////////////////////////////////////////
// Purpose:   Create a CMakeLists.txt file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "ttcwd.h"       // cwd -- Class for storing and optionally restoring the current directory
#include "ttmultistr.h"  // multistr -- Breaks a single string into multiple strings
#include "ttsview.h"     // sview -- std::string_view with additional methods
#include "tttextfile.h"  // textfile -- Classes for reading and writing line-oriented files

#include "csrcfiles.h"  // CSrcFiles

const char* multi_config = R"===(
get_property(isMultiConfig GLOBAL
  PROPERTY GENERATOR_IS_MULTI_CONFIG
)

if (NOT isMultiConfig)
    message("\nBecause you are using a single target generator, you MUST specify")
    message("    a \"--config [Debug|Release]\" option with the cmake --build command\n")

    set(allowedBuildTypes Debug Release)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${allowedBuildTypes}")

    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
    elseif (NOT CMAKE_BUILD_TYPE IN_LIST allowedBuildTypes)
        message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE}")
    endif()
endif()
)===";

const char* preset_json = R"===({
  "version": 3,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 20,
    "patch": 0
  },
  "configurePresets": [
        {
            "name": "ninja-multi",
            "displayName": "Ninja Multi-Config",
            "description": "Default build using Ninja Multi-Config generator",
            "generator": "Ninja Multi-Config",
            "binaryDir": "${sourceDir}/build"
        }
    ]
}
)===";

bool CreateCmakeProject(ttlib::cstr& projectFile)
{
    ttlib::cwd cur_cwd;

    ttlib::cstr file_prefix;
    if (projectFile.contains("/"))
    {
        file_prefix = projectFile;
        file_prefix.remove_filename();
    }

    if (file_prefix.size())
        ttlib::ChangeDir(file_prefix);

    CSrcFiles srcfiles;
    if (!srcfiles.ReadFile(projectFile.filename()))
    {
        std::cout << "Cannot locate a .srcfiles.yaml to create CMakeLists.txt from!" << '\n';
        return false;
    }

    if (file_prefix.size())
        ttlib::ChangeDir(cur_cwd);

    ttlib::textfile out;

    out += "cmake_minimum_required(VERSION 3.20)";
    out += "";
    out.emplace_back(ttlib::cstr() << "project(" << srcfiles.GetProjectName() << " LANGUAGES CXX)");
    out += "";

    int standard = 17;

    ttlib::cstr flags_cmn(srcfiles.getOptValue(OPT::CFLAGS_CMN));
    if (srcfiles.hasOptValue(OPT::MSVC_CMN))
    {
        if (flags_cmn.size())
            flags_cmn << ' ';
        flags_cmn << srcfiles.getOptValue(OPT::MSVC_CMN);
    }

    if (flags_cmn.contains("std:c++"))
    {
        if (flags_cmn.contains("std:c++latest"))
        {
            standard = 20;
        }
        else
        {
            if (auto result = flags_cmn.find("std:c++"); ttlib::is_found(result))
            {
                result += 7;
                while (!ttlib::is_digit(flags_cmn[result]) && result < flags_cmn.size())
                    ++result;
                if (result < flags_cmn.size())
                    standard = flags_cmn.atoi(result);
            }
        }
    }

    out.emplace_back(ttlib::cstr() << "set(CMAKE_CXX_STANDARD " << standard << ")");
    out += "set(CMAKE_CXX_STANDARD_REQUIRED True)";
    out += "set(CMAKE_CXX_EXTENSIONS OFF)";
    out += "";

    out += "if (MSVC)";
    out += "    # /O1 often results in faster code than /O2 due to CPU caching";
    out += "    string(REPLACE \"/O2\" \"/O1\" cl_optimize ${CMAKE_CXX_FLAGS_RELEASE})";
    out += "    set(CMAKE_CXX_FLAGS_RELEASE ${cl_optimize} CACHE STRING \"C++ Release flags\" FORCE)";
    out += "";

    out += "    # Using /Z7 instead of /Zi to avoid blocking while parallel compilers write to the pdb file.";
    out += "    # This can considerably speed up build times at the cost of larger object files.";
    out += "    string(REPLACE \"/Zi\" \"/Z7\" z_seven ${CMAKE_CXX_FLAGS_DEBUG})";
    out += "    set(CMAKE_CXX_FLAGS_DEBUG ${z_seven} CACHE STRING \"C++ Debug flags\" FORCE)";

    out += "endif()";
    out += multi_config;

    bool need_blank_line { false };

    // Note that we make definitions global for the project rather than for just the target. That's because flags like
    // -DWXUSINGDLL *MUST* be global.

    if (flags_cmn.size() && (flags_cmn.contains("/D") || flags_cmn.contains("-D")))
    {
        need_blank_line = true;
        ttlib::cstr flags = flags_cmn;
        flags.Replace("-D", "/D", true, tt::CASE::exact);

        ttlib::cstr definitions;
        auto start = flags.find("/D");
        while (ttlib::is_found(start))
        {
            start += 2;
            auto end = flags.find_space(start);
            if (!ttlib::is_found(end))
                end = flags.size();
            if (definitions.size())
                definitions << ' ';
            definitions << flags.subview(start, end - start);
            start = flags.find("/D", end);
        }

        out.emplace_back(ttlib::cstr() << "add_compile_definitions(" << definitions << ')');
    }

    if (srcfiles.hasOptValue(OPT::CFLAGS_REL) &&
        (srcfiles.getOptValue(OPT::CFLAGS_REL).contains("/D") || srcfiles.getOptValue(OPT::CFLAGS_REL).contains("-D")))
    {
        need_blank_line = true;
        ttlib::cstr flags = srcfiles.getOptValue(OPT::CFLAGS_REL);
        flags.Replace("-D", "/D", true, tt::CASE::exact);

        ttlib::cstr definitions;
        auto start = flags.find("/D");
        while (ttlib::is_found(start))
        {
            start += 2;
            auto end = flags.find_space(start);
            if (!ttlib::is_found(end))
                end = flags.size();
            if (definitions.size())
                definitions << ' ';
            definitions << "$<$<CONFIG:Release>:" << flags.subview(start, end - start) << '>';
            start = flags.find("/D", end);
        }

        out.emplace_back(ttlib::cstr() << "add_compile_definitions(" << definitions << ')');
    }

    if (srcfiles.hasOptValue(OPT::CFLAGS_DBG) &&
        (srcfiles.getOptValue(OPT::CFLAGS_DBG).contains("/D") || srcfiles.getOptValue(OPT::CFLAGS_DBG).contains("-D")))
    {
        need_blank_line = true;
        ttlib::cstr flags = srcfiles.getOptValue(OPT::CFLAGS_DBG);
        flags.Replace("-D", "/D", true, tt::CASE::exact);

        ttlib::cstr definitions;
        auto start = flags.find("/D");
        while (ttlib::is_found(start))
        {
            start += 2;
            auto end = flags.find_space(start);
            if (!ttlib::is_found(end))
                end = flags.size();
            if (definitions.size())
                definitions << ' ';
            definitions << "$<$<CONFIG:Debug>:" << flags.subview(start, end - start) << '>';
            start = flags.find("/D", end);
        }

        out.emplace_back(ttlib::cstr() << "add_compile_definitions(" << definitions << ')');
    }

    if (need_blank_line)
    {
        out += "";
        need_blank_line = false;
    }

    if (srcfiles.GetDebugFileList().size())
    {
        out += "# Note the requirement that --config Debug is used to get the additional debug files";
    }

    ttlib::viewfile in;
    if (!in.ReadFile(projectFile))
    {
        std::cout << "Unable to read " << srcfiles.GetSrcFilesName() << '\n';
        return false;
    }

    auto file_pos = in.FindLineContaining("Files:");
    if (!ttlib::is_found(file_pos))
    {
        std::cout << srcfiles.GetSrcFilesName() << " does not contain a Files: section" << '\n';
        return false;
    }

    out.emplace_back(ttlib::cstr() << "add_executable(" << srcfiles.GetProjectName());
    if (srcfiles.IsExeTypeWindow())
    {
        out.back() << " WIN32";
    }

    for (++file_pos; file_pos < in.size(); ++file_pos)
    {
        if (in[file_pos].empty())
        {
            out += "";
            continue;
        }

        if (ttlib::is_alpha(in[file_pos].front()))
        {
            // Alphabetic character in first position starts a new session
            break;
        }

        ttlib::sview view(in[file_pos]);
        view.moveto_nonspace();
        if (view.front() == '#')
        {
            out += in[file_pos];
            continue;
        }

        if (file_prefix.size())
        {
            ttlib::cstr path;
            ttlib::cstr non_relative;
            non_relative << file_prefix << "../";
            path << file_prefix << ttlib::find_nonspace(in[file_pos]);
            if (path.is_sameprefix(non_relative))
            {
                path.erase(0, non_relative.size());
            }
            out.emplace_back(ttlib::cstr() << "    " << path);
        }
        else
        {
            out += in[file_pos];
        }
    }

    if (file_pos = in.FindLineContaining("DebugFiles:"); ttlib::is_found(file_pos))
    {
        for (++file_pos; file_pos < in.size(); ++file_pos)
        {
            if (in[file_pos].empty())
            {
                out += "";
                continue;
            }

            if (ttlib::is_alpha(in[file_pos].front()))
            {
                // Alphabetic character in first position starts a new session
                break;
            }

            ttlib::sview view(in[file_pos]);
            view.moveto_nonspace();
            if (view.front() == '#')
            {
                out += in[file_pos];
                continue;
            }

            ttlib::cstr path;
            path << file_prefix << ttlib::find_nonspace(in[file_pos]);
            if (file_prefix.size())
            {
                ttlib::cstr non_relative;
                non_relative << file_prefix << "../";
                if (path.is_sameprefix(non_relative))
                {
                    path.erase(0, non_relative.size());
                }
            }
            if (auto result = path.find('#'); ttlib::is_found(result))
            {
                ttlib::cstr comment = path.substr(result);
                path.erase(result);
                path.RightTrim();
                path << '>' << "  " << comment;
                out.emplace_back(ttlib::cstr() << "    $<$<CONFIG:Debug>:" << path);
            }
            else
            {
                out.emplace_back(ttlib::cstr() << "    $<$<CONFIG:Debug>:" << path << '>');
            }
        }
    }

    out += ")";
    out += "";

    if (need_blank_line)
    {
        out += "";
        need_blank_line = false;
    }

    out += "if (MSVC)";
    out += "    # /GL -- combined with the Linker flag /LTCG to perform whole program optimization in Release build";
    out += "    # /FC -- Full path to source code file in diagnostics";
    out.emplace_back(
        ttlib::cstr() << "    target_compile_options(" << srcfiles.GetProjectName()
                      << " PRIVATE \"$<$<CONFIG:Release>:/GL>\" \"/FC\" \"/W4\" \"/Zc:__cplusplus\" \"/utf-8\")");
    out.emplace_back(ttlib::cstr() << "    target_link_options(" << srcfiles.GetProjectName()
                                   << " PRIVATE \"$<$<CONFIG:Release>:/LTCG>\")\n");

    if (srcfiles.hasOptValue(OPT::NATVIS))
    {
        ttlib::cstr file_path;
        if (file_prefix.size() && !srcfiles.getOptValue(OPT::NATVIS).is_sameprefix("../"))
        {
            file_path << "../" << file_prefix;
        }
        file_path << srcfiles.getOptValue(OPT::NATVIS);

        // The natvis file is relative to the build directory which is parellel to any src directory
        out.emplace_back(ttlib::cstr() << "    target_link_options(" << srcfiles.GetProjectName()
                                       << " PRIVATE \"$<$<CONFIG:Debug>:/natvis:" << file_path << ">\")");
    }

    if (srcfiles.getRcName().size())
    {
        out += "\n    # Assume the manifest is in the resource file";
        out.emplace_back(ttlib::cstr() << "    target_link_options(" << srcfiles.GetProjectName()
                                       << " PRIVATE \"/manifest:no\")");
    }

    out += "endif()";
    out += "";

    if (srcfiles.HasPch())
    {
        ttlib::cstr pch_path;
        if (file_prefix.size())
            pch_path = file_prefix;
        pch_path << srcfiles.getOptValue(OPT::PCH);
        if (!pch_path.file_exists() && srcfiles.hasOptValue(OPT::INC_DIRS))
        {
            ttlib::multiview inc_list(srcfiles.getOptValue(OPT::INC_DIRS));
            for (auto& iter: inc_list)
            {
                pch_path.clear();
                if (file_prefix.size())
                    pch_path = file_prefix;
                pch_path.append_filename(iter);
                pch_path.addtrailingslash();
                pch_path << srcfiles.getOptValue(OPT::PCH);
                if (pch_path.file_exists())
                    break;
            }
        }
        out.emplace_back(ttlib::cstr() << "target_precompile_headers(" << srcfiles.GetProjectName() << " PRIVATE \""
                                       << pch_path << "\")");
        out += "";
    }

    if (srcfiles.hasOptValue(OPT::INC_DIRS))
    {
        out.emplace_back(ttlib::cstr() << "target_include_directories(" << srcfiles.GetProjectName() << " PRIVATE ");

        ttlib::multiview inc_list(srcfiles.getOptValue(OPT::INC_DIRS));
        for (auto& iter: inc_list)
        {
            ttlib::cstr inc_path("    ");
            if (iter.front() == '$')
            {
                auto end = iter.find(')');
                inc_path << "$ENV{" << iter.substr(2, end - 2) << '}' << iter.subview(end + 1);
                out.emplace_back(inc_path);
                continue;
            }
            if (file_prefix.size())
                inc_path << file_prefix;
            if (!iter.is_sameas("./"))
                inc_path << iter;

            if (file_prefix.size() && inc_path.is_sameprefix(ttlib::cstr() << "    " << file_prefix << "../"))
            {
                inc_path = "    ";
                inc_path << iter.subview(3);
            }
            out.emplace_back(inc_path);
        }
        out += ")";
        out += "";
    }

    if (srcfiles.hasOptValue(OPT::LIB_DIRS) || srcfiles.hasOptValue(OPT::LIB_DIRS64))
        out += "if (MSVC)";

    if (srcfiles.hasOptValue(OPT::LIB_DIRS))
    {
        out.emplace_back(ttlib::cstr() << "    target_link_directories(" << srcfiles.GetProjectName() << " PRIVATE ");

        ttlib::multiview lib_list(srcfiles.getOptValue(OPT::LIB_DIRS));
        for (auto& iter: lib_list)
        {
            ttlib::cstr lib_path("        ");
            if (iter.front() == '$')
            {
                auto end = iter.find(')');
                lib_path << "$ENV{" << iter.substr(2, end - 2) << '}' << iter.subview(end + 1);
                out.emplace_back(lib_path);
                continue;
            }

            if (file_prefix.size() && iter.front() == '.')
                lib_path << file_prefix;
            lib_path << iter;

            out.emplace_back(lib_path);
        }
        out += "    )";
    }
    else if (srcfiles.hasOptValue(OPT::LIB_DIRS64))
    {
        out.emplace_back(ttlib::cstr() << "    target_link_directories(" << srcfiles.GetProjectName() << " PRIVATE ");

        ttlib::multiview lib_list(srcfiles.getOptValue(OPT::LIB_DIRS64));
        for (auto& iter: lib_list)
        {
            ttlib::cstr lib_path("        ");
            if (iter.front() == '$')
            {
                auto end = iter.find(')');
                lib_path << "$ENV{" << iter.substr(2, end - 2) << '}' << iter.subview(end + 1);
                out.emplace_back(lib_path);
                continue;
            }

            if (file_prefix.size() && iter.front() == '.')
                lib_path << file_prefix;
            lib_path << iter;

            out.emplace_back(lib_path);
        }
        out += "    )";
    }
    if (srcfiles.hasOptValue(OPT::LIB_DIRS) || srcfiles.hasOptValue(OPT::LIB_DIRS64))
        out += "endif()";

    ttlib::cstr out_name("CMakeLists.txt");
    if (ttlib::file_exists(out_name))
    {
        out_name.replace_extension(".ttbld");
    }

    if (!out.WriteFile(out_name))
    {
        std::cout << "Cannot write to " << out_name << '\n';
        return false;
    }

    std::cout << "Created " << out_name << '\n';
    if (!ttlib::file_exists("CMakePresets.json"))
    {
        out.clear();
        out.ReadString(preset_json);
        if (out.WriteFile("CMakePresets.json"))
        {
            std::cout << "Created CMakePresets.json" << '\n';
        }
    }

    return true;
}
