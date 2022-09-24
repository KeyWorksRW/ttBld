cmake_minimum_required(VERSION 3.20)

project(ttBld LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    # /O1 often results in faster code than /O2 due to CPU caching
    string(REPLACE "/O2" "/O1" cl_optimize ${CMAKE_CXX_FLAGS_RELEASE})
    set(CMAKE_CXX_FLAGS_RELEASE ${cl_optimize} CACHE STRING "C++ Release flags" FORCE)

    string(REPLACE "/O2" "/O1" cl_optimize ${CMAKE_C_FLAGS_RELEASE})
    set(CMAKE_C_FLAGS_RELEASE ${cl_optimize} CACHE STRING "C Release flags" FORCE)

    # Using /Z7 instead of /Zi to avoid blocking while parallel compilers write to the pdb file.
    # This can considerably speed up build times at the cost of larger object files.
    string(REPLACE "/Zi" "/Z7" z_seven ${CMAKE_CXX_FLAGS_DEBUG})
    set(CMAKE_CXX_FLAGS_DEBUG ${z_seven} CACHE STRING "C++ Debug flags" FORCE)

    string(REPLACE "/Zi" "/Z7" z_seven ${CMAKE_C_FLAGS_DEBUG})
    set(CMAKE_C_FLAGS_DEBUG ${z_seven} CACHE STRING "C Debug flags" FORCE)

    # Use static runtime for Release builds to run with Wine without needing to install the dlls
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    # This should work for gcc and clang (including xcode which is based on clang)
    # -O2 can result in faster code than -O3 due to CPU caching.
    string(REPLACE "-O3" "-O2" cl_optimize ${CMAKE_CXX_FLAGS_RELEASE})
    set(CMAKE_CXX_FLAGS_RELEASE ${cl_optimize} CACHE STRING "C++ Release flags" FORCE)
    string(REPLACE "-O3" "-O2" cl_optimize ${CMAKE_C_FLAGS_RELEASE})
    set(CMAKE_C_FLAGS_RELEASE ${cl_optimize} CACHE STRING "C Release flags" FORCE)
endif()

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

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

# This will build wxCLib.lib and wxWidgets.lib
set (widget_dir ${CMAKE_CURRENT_LIST_DIR}/wxSnapshot)
set (widget_cmake_dir ${CMAKE_CURRENT_LIST_DIR}/wxSnapshot)
add_subdirectory(${widget_cmake_dir})

# wxUiEditor maintains the wxui_code.cmake file which contains the list of generated files
include(src/ui/wxui_code.cmake)  # This will set ${wxue_generated_code}
include(src/file_list.cmake)     # This will set ${file_list} with a list of source files

if (WIN32)
    set(setup_dir ${widget_cmake_dir}/win)
    set(win_ttlib_sources
        ttlib/src/winsrc/ttregistry.cpp  # Class for working with the Windows registry

        $<$<CONFIG:Debug>:ttlib/src/winsrc/ttdebug_min.cpp>
    )

    set(win_resource "src/ttBld.rc")
else()
    # Currently we don't support building on unix, but if we did, this is what we would need
    set(setup_dir ${widget_cmake_dir}/unix)
endif()

# MSVC release builds will already define this, but let's be sure in case we build with a
# different compiler
add_compile_definitions($<$<CONFIG:Release>:NDEBUG>)

# Instructions for building the ttlib_wx library using this project's precompiled header
include(ttLib_wx/src/ttlib_file_list.cmake)
add_library(ttLib_wx STATIC ${ttlib_file_list})
target_precompile_headers(ttLib_wx PRIVATE "src/precompile/pch_wx.h")
target_include_directories(ttLib_wx PRIVATE
    ${widget_dir}/include
    ${setup_dir}
    src/
    ttLib_wx/src
)

# Note the requirement that --config Debug is used to get the additional debug files
add_executable(ttBld

    ${win_resource}
    ${file_list}
    ${wxue_generated_code}
)

# This is just used by a github action to confirm that all the source code can be compiled
add_library(check_build STATIC EXCLUDE_FROM_ALL
    ${win_resource}
    ${file_list}
    ${wxue_generated_code}
)

target_link_libraries(ttBld PRIVATE ttLib_wx wxCLib wxWidgets)

if (MSVC)
    # /GL -- combined with the Linker flag /LTCG to perform whole program optimization in Release build
    # /FC -- Full path to source code file in diagnostics
    target_compile_options(ttBld PRIVATE "$<$<CONFIG:Release>:/GL>" "/FC" "/W4" "/Zc:__cplusplus" "/utf-8")
    target_link_options(ttBld PRIVATE "$<$<CONFIG:Release>:/LTCG>")

    target_link_options(ttBld PRIVATE "$<$<CONFIG:Debug>:/natvis:../src/ttBld.natvis>")

    # Assume the manifest is in the resource file
    target_link_options(ttBld PRIVATE "/manifest:no")
endif()

target_precompile_headers(ttBld PRIVATE "src/precompile/pch_wx.h")
target_precompile_headers(check_build PRIVATE "src/precompile/pch_wx.h")

target_include_directories(ttBld PRIVATE
    wxSnapshot/include
    if (WIN32)
        wxSnapshot/win
    endif()
    src/
    src/precompile
    src/ui
    src/convert
    ttLib_wx/src
)

target_include_directories(check_build PRIVATE
    wxSnapshot/include
    if (WIN32)
        wxSnapshot/win
    endif()
    src/
    src/precompile
    src/ui
    src/convert
    ttLib_wx/src
)
