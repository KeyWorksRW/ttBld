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

#include "convert.h"    // CConvert -- Class for converting project build files to .srcfiles.yaml
#include "csrcfiles.h"  // CSrcFiles
#include "uifuncs.h"    // Miscellaneous functions for displaying UI

#include "../pugixml/pugixml.hpp"  // pugixml parser

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

bld::RESULT CConvert::CreateCmakeProject(ttlib::cstr& projectFile)
{
    ttlib::cwd cur_cwd;

    m_srcFile = projectFile;

    if (m_srcFile.contains("/"))
    {
        m_srcDir = m_srcFile;
        m_srcDir.remove_filename();
    }

    if (m_srcDir.size())
        ttlib::ChangeDir(m_srcDir);

    if (!m_srcfiles.ReadFile(m_srcFile.filename()))
    {
        std::cout << "Cannot locate a .srcfiles.yaml to create CMakeLists.txt from!" << '\n';
        return bld::RESULT::invalid_file;
    }

    if (m_srcDir.size())
        ttlib::ChangeDir(cur_cwd);

    return WriteCmakeProject();
}

bld::RESULT CConvert::WriteCmakeProject()
{
    ttlib::textfile out;

    out += "cmake_minimum_required(VERSION 3.20)";
    out += "";
    out.emplace_back(ttlib::cstr() << "project(" << m_srcfiles.GetProjectName() << " LANGUAGES CXX)");
    out += "";

    int standard = 17;

    ttlib::cstr flags_cmn(m_srcfiles.getOptValue(OPT::CFLAGS_CMN));
    if (m_srcfiles.hasOptValue(OPT::MSVC_CMN))
    {
        if (flags_cmn.size())
            flags_cmn << ' ';
        flags_cmn << m_srcfiles.getOptValue(OPT::MSVC_CMN);
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

    if (m_srcfiles.hasOptValue(OPT::CFLAGS_REL) &&
        (m_srcfiles.getOptValue(OPT::CFLAGS_REL).contains("/D") || m_srcfiles.getOptValue(OPT::CFLAGS_REL).contains("-D")))
    {
        need_blank_line = true;
        ttlib::cstr flags = m_srcfiles.getOptValue(OPT::CFLAGS_REL);
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

    if (m_srcfiles.hasOptValue(OPT::CFLAGS_DBG) &&
        (m_srcfiles.getOptValue(OPT::CFLAGS_DBG).contains("/D") || m_srcfiles.getOptValue(OPT::CFLAGS_DBG).contains("-D")))
    {
        need_blank_line = true;
        ttlib::cstr flags = m_srcfiles.getOptValue(OPT::CFLAGS_DBG);
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

    if (m_srcfiles.GetDebugFileList().size())
    {
        out += "# Note the requirement that --config Debug is used to get the additional debug files";
    }

    out.emplace_back(ttlib::cstr() << "add_executable(" << m_srcfiles.GetProjectName());
    if (m_srcfiles.IsExeTypeWindow())
    {
        out.back() << " WIN32";
    }

    if (m_srcFile.size())
    {
        ttlib::viewfile in;
        if (!in.ReadFile(m_srcFile))
        {
            std::cout << "Unable to read " << m_srcfiles.GetSrcFilesName() << '\n';
            return bld::RESULT::read_failed;
        }

        size_t file_pos = in.FindLineContaining("Files:");

        if (!ttlib::is_found(file_pos))
        {
            std::cout << m_srcfiles.GetSrcFilesName() << " does not contain a Files: section" << '\n';
            return bld::RESULT::invalid_file;
        }
        CMakeAddFilesSection(in, out, file_pos);
    }
    else
    {
        CMakeAddFiles(out);
    }

    if (need_blank_line)
    {
        out += "";
        need_blank_line = false;
    }

    out += "if (MSVC)";
    out += "    # /GL -- combined with the Linker flag /LTCG to perform whole program optimization in Release build";
    out += "    # /FC -- Full path to source code file in diagnostics";
    out.emplace_back(
        ttlib::cstr() << "    target_compile_options(" << m_srcfiles.GetProjectName()
                      << " PRIVATE \"$<$<CONFIG:Release>:/GL>\" \"/FC\" \"/W4\" \"/Zc:__cplusplus\" \"/utf-8\")");
    out.emplace_back(ttlib::cstr() << "    target_link_options(" << m_srcfiles.GetProjectName()
                                   << " PRIVATE \"$<$<CONFIG:Release>:/LTCG>\")\n");

    if (m_srcfiles.hasOptValue(OPT::NATVIS))
    {
        ttlib::cstr file_path;
        if (m_srcDir.size() && !m_srcfiles.getOptValue(OPT::NATVIS).is_sameprefix("../"))
        {
            file_path << "../" << m_srcDir;
        }
        file_path << m_srcfiles.getOptValue(OPT::NATVIS);

        // The natvis file is relative to the build directory which is parellel to any src directory
        out.emplace_back(ttlib::cstr() << "    target_link_options(" << m_srcfiles.GetProjectName()
                                       << " PRIVATE \"$<$<CONFIG:Debug>:/natvis:" << file_path << ">\")");
    }

    if (m_srcfiles.getRcName().size())
    {
        out += "\n    # Assume the manifest is in the resource file";
        out.emplace_back(ttlib::cstr() << "    target_link_options(" << m_srcfiles.GetProjectName()
                                       << " PRIVATE \"/manifest:no\")");
    }

    out += "endif()";
    out += "";

    if (m_srcfiles.HasPch())
    {
        ttlib::cstr pch_path;
        if (m_srcDir.size())
            pch_path = m_srcDir;
        pch_path << m_srcfiles.getOptValue(OPT::PCH);
        if (!pch_path.file_exists() && m_srcfiles.hasOptValue(OPT::INC_DIRS))
        {
            ttlib::multiview inc_list(m_srcfiles.getOptValue(OPT::INC_DIRS));
            for (auto& iter: inc_list)
            {
                pch_path.clear();
                if (m_srcDir.size())
                    pch_path = m_srcDir;
                pch_path.append_filename(iter);
                pch_path.addtrailingslash();
                pch_path << m_srcfiles.getOptValue(OPT::PCH);
                if (pch_path.file_exists())
                    break;
            }
        }
        out.emplace_back(ttlib::cstr() << "target_precompile_headers(" << m_srcfiles.GetProjectName() << " PRIVATE \""
                                       << pch_path << "\")");
        out += "";
    }

    if (m_srcfiles.hasOptValue(OPT::INC_DIRS))
    {
        out.emplace_back(ttlib::cstr() << "target_include_directories(" << m_srcfiles.GetProjectName() << " PRIVATE ");

        ttlib::multiview inc_list(m_srcfiles.getOptValue(OPT::INC_DIRS));
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
            if (m_srcDir.size())
                inc_path << m_srcDir;
            if (!iter.is_sameas("./"))
                inc_path << iter;

            if (m_srcDir.size() && inc_path.is_sameprefix(ttlib::cstr() << "    " << m_srcDir << "../"))
            {
                inc_path = "    ";
                inc_path << iter.subview(3);
            }
            out.emplace_back(inc_path);
        }
        out += ")";
        out += "";
    }

    if (m_srcfiles.hasOptValue(OPT::LIB_DIRS) || m_srcfiles.hasOptValue(OPT::LIB_DIRS64))
        out += "if (MSVC)";

    if (m_srcfiles.hasOptValue(OPT::LIB_DIRS))
    {
        out.emplace_back(ttlib::cstr() << "    target_link_directories(" << m_srcfiles.GetProjectName() << " PRIVATE ");

        ttlib::multiview lib_list(m_srcfiles.getOptValue(OPT::LIB_DIRS));
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

            if (m_srcDir.size() && iter.front() == '.')
                lib_path << m_srcDir;
            lib_path << iter;

            out.emplace_back(lib_path);
        }
        out += "    )";
    }
    else if (m_srcfiles.hasOptValue(OPT::LIB_DIRS64))
    {
        out.emplace_back(ttlib::cstr() << "    target_link_directories(" << m_srcfiles.GetProjectName() << " PRIVATE ");

        ttlib::multiview lib_list(m_srcfiles.getOptValue(OPT::LIB_DIRS64));
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

            if (m_srcDir.size() && iter.front() == '.')
                lib_path << m_srcDir;
            lib_path << iter;

            out.emplace_back(lib_path);
        }
        out += "    )";
    }
    if (m_srcfiles.hasOptValue(OPT::LIB_DIRS) || m_srcfiles.hasOptValue(OPT::LIB_DIRS64))
        out += "endif()";

    ttlib::cstr out_name("CMakeLists.txt");
    if (ttlib::file_exists(out_name))
    {
        out_name.replace_extension(".ttbld");
    }

    if (!out.WriteFile(out_name))
    {
        std::cout << "Cannot write to " << out_name << '\n';
        return bld::RESULT::write_failed;
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

    return bld::RESULT::success;
}

void CConvert::CMakeAddFilesSection(ttlib::viewfile& in, ttlib::textfile& out, size_t file_pos)
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

        if (m_srcDir.size())
        {
            ttlib::cstr path;
            ttlib::cstr non_relative;
            non_relative << m_srcDir << "../";
            path << m_srcDir << ttlib::find_nonspace(in[file_pos]);
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
            path << m_srcDir << ttlib::find_nonspace(in[file_pos]);
            if (m_srcDir.size())
            {
                ttlib::cstr non_relative;
                non_relative << m_srcDir << "../";
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
}

void CConvert::CMakeAddFiles(ttlib::textfile& out)
{
    for (auto& iter: m_srcfiles.GetSrcFileList())
    {
        if (m_srcDir.size())
        {
            ttlib::cstr path;
            ttlib::cstr non_relative;
            non_relative << m_srcDir << "../";
            path << m_srcDir << ttlib::find_nonspace(iter);
            if (path.is_sameprefix(non_relative))
            {
                path.erase(0, non_relative.size());
            }
            out.emplace_back(ttlib::cstr() << "    " << path);
        }
        else
        {
            out += iter;
        }
    }

    for (auto& iter: m_srcfiles.GetDebugFileList())
    {
        if (m_srcDir.size())
        {
            ttlib::cstr path;
            ttlib::cstr non_relative;
            non_relative << m_srcDir << "../";
            path << m_srcDir << ttlib::find_nonspace(iter);
            if (path.is_sameprefix(non_relative))
            {
                path.erase(0, non_relative.size());
            }
            out.emplace_back(ttlib::cstr() << "    $<$<CONFIG:Debug>:" << path << '>');
        }
        else
        {
            out.emplace_back(ttlib::cstr() << "    $<$<CONFIG:Debug>:" << iter << '>');
        }
    }
}
