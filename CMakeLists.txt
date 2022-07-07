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
add_subdirectory(wxSnapshot)

include(src/ui/wxui_code.cmake)  # This will set ${wxue_generated_code} with a list of generated files
include(src/file_list.cmake)     # This will set ${file_list} with a list of source files

if (WIN32)
    set(win_ttlib_sources
        ttlib/src/winsrc/ttregistry.cpp  # Class for working with the Windows registry

        $<$<CONFIG:Debug>:ttlib/src/winsrc/ttdebug_min.cpp>
    )

    set(win_resource "src/ttBld.rc")
endif()

# Build the library directly instead of using add_subdirectory() to limit the modules we want and to add
# any modules that require wxWidgets.
add_library(ttLib STATIC

    ttLib/src/ttconsole.cpp          # class that sets/restores console foreground color
    ttLib/src/ttcstr.cpp             # Class for handling zero-terminated char strings.
    ttLib/src/ttcvector.cpp          # Vector class for storing ttlib::cstr strings
    ttLib/src/ttcview.cpp            # string_view functionality on a zero-terminated char string.
    ttLib/src/ttlibspace.cpp         # ttlib namespace functions
    ttLib/src/ttmultistr.cpp         # Breaks a single string into multiple strings
    ttLib/src/ttparser.cpp           # Command line parser
    ttLib/src/ttstrings.cpp          # Class for handling zero-terminated char strings.
    ttLib/src/ttsview.cpp            # std::string_view with additional methods
    ttLib/src/tttextfile.cpp         # Classes for reading and writing text files.

    ttLib/src/ttstr.cpp              # Enhanced version of wxString

    ${win_ttlib_sources}
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

target_link_libraries(ttBld PRIVATE ttLib wxCLib wxWidgets)

if (MSVC)
    # /GL -- combined with the Linker flag /LTCG to perform whole program optimization in Release build
    # /FC -- Full path to source code file in diagnostics
    target_compile_options(ttBld PRIVATE "$<$<CONFIG:Release>:/GL>" "/FC" "/W4" "/Zc:__cplusplus" "/utf-8")
    target_link_options(ttBld PRIVATE "$<$<CONFIG:Release>:/LTCG>")

    target_link_options(ttBld PRIVATE "$<$<CONFIG:Debug>:/natvis:../src/ttBld.natvis>")

    # Assume the manifest is in the resource file
    target_link_options(ttBld PRIVATE "/manifest:no")
endif()

target_precompile_headers(ttBld PRIVATE "src/precompile/pch.h")
target_precompile_headers(check_build PRIVATE "src/precompile/pch.h")
target_precompile_headers(ttLib PRIVATE "src/precompile/pch.h")

target_include_directories(ttBld PRIVATE
    wxSnapshot/include
    if (WIN32)
        wxSnapshot/win
    endif()
    src/
    src/precompile
    src/ui
    src/convert
    ttLib/include
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
    ttLib/include
)

target_include_directories(ttLib PRIVATE
    wxSnapshot/include
    if (WIN32)
        wxSnapshot/win
    endif()
    src/precompile
    ttLib/include
)
