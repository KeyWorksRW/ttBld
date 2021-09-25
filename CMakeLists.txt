cmake_minimum_required(VERSION 3.20)

project(ttBld LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    # /O1 often results in faster code than /O2 due to CPU caching
    string(REPLACE "/O2" "/O1" cl_optimize ${CMAKE_CXX_FLAGS_RELEASE})
    set(CMAKE_CXX_FLAGS_RELEASE ${cl_optimize} CACHE STRING "C++ Release flags" FORCE)

    # Using /Z7 instead of /Zi to avoid blocking while parallel compilers write to the pdb file.
    # This can considerably speed up build times at the cost of larger object files.
    string(REPLACE "/Zi" "/Z7" z_seven ${CMAKE_CXX_FLAGS_DEBUG})
    set(CMAKE_CXX_FLAGS_DEBUG ${z_seven} CACHE STRING "C++ Debug flags" FORCE)
endif()

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

add_compile_definitions($<$<CONFIG:Debug>:WXUSINGDLL>)

# Note the requirement that --config Debug is used to get the additional debug files
add_executable(ttBld
    src/ttBld.rc            # Windows resource files

    src/mainapp.cpp         # entry point, global strings, library pragmas

    src/addfiles.cpp        # Used to add one or more filenames to a .srcfiles file
    src/cmplrMsvc.cpp       # Creates .ninja scripts for MSVC and CLANG-CL compilers
    src/createmakefile.cpp  # CreateMakeFile method for creating a makefile
    src/csrcfiles.cpp       # CSrcFiles class for reading .srcfiles
    src/dryrun.cpp          # CDryRun class for testing
    src/finder.cpp          # Routines for finding executables such as the MSVC compiler
    src/gencmdfiles.cpp     # Generates MSVCenv.cmd and Code.cmd files
    src/gitfuncs.cpp        # Functions for working with .git
    src/image_hdr.cpp       # Convert image into png header
    src/make_hgz.cpp        # Converts a file into a .gz and stores as char array header
    src/ninja.cpp           # CNinja for creating .ninja scripts
    src/options.cpp         # contains all Options strings and CSrcOptions class for working with them
    src/rcdep.cpp           # Contains functions for parsing RC dependencies
    src/verninja.cpp        # CVerMakeNinja class
    src/vs.cpp              # Creates .vs/tasks.vs.json and .vs/launch.vs.json
    src/vscode.cpp          # Creates/updates .vscode files
    src/writesrc.cpp        # Writes a new or update srcfiles.yaml file
    src/yamalize.cpp        # Used to convert .srcfiles to .vscode/srcfiles.yaml

    src/convert/convert.cpp       # Various conversion methods
    src/convert/readcodelite.cpp  # Class for converting a CodeLite .project file to .srcfiles.yaml
    src/convert/readdsp.cpp       # Class for converting a Visual Studio .DSP file to .srcfiles.yaml
    src/convert/readvc.cpp        # Class for converting a Visual Studio .vcproj file to .srcfiles.yaml
    src/convert/readvcx.cpp       # Converts a Visual Studio project file into .srcfiles.yaml
    src/convert/writevcx.cpp      # Create a Visual Studio project file
    src/convert/write_cmake.cpp   # Create a CMakeLists.txt file

    # DO NOT RUN clang-format on the following file! We need to be able to sync changes from the original repository
    src/pugixml/pugixml.cpp        # Built directly rather than building a library

    # wxUiEditor generated classes and their derived version for displaying wxWidgets UI

    src/ui/convertdlg.cpp          # Dialog specifying what to convert into a .srcfiles.yaml file
    src/ui/vscodedlg.cpp           # Dialog for setting options to create tasks.json and launch.json
    src/ui/optionsdlg.cpp          # Dialog for setting all .srcfile options

    src/ui/convertdlgBase.cpp       # wxUiEditor generated file
    src/ui/optionsdlgBase.cpp       # wxUiEditor generated file
    src/ui/vscodedlgBase.cpp        # wxUiEditor generated file

    ttlib/src/ttconsole.cpp   # Sets/restores console foreground color
    ttlib/src/ttcstr.cpp      # Class for handling zero-terminated char strings.
    ttlib/src/ttcview.cpp     # string_view functionality on a zero-terminated char string.
    ttlib/src/ttlibspace.cpp  # ttlib namespace functions
    ttlib/src/ttmultistr.cpp  # Breaks a single string into multiple strings
    ttlib/src/ttparser.cpp    # Command line parser
    ttlib/src/ttstr.cpp       # ttString -- enhanced version of wxString
    ttlib/src/ttsview.cpp     # std::string_view with additional methods
    ttlib/src/tttextfile.cpp  # Classes for reading and writing text files.

    ttlib/src/winsrc/ttregistry.cpp  # Class for working with the Windows registry

# When ttBld generates the ninja scripts, the files in the DebugFiles section will ONLY be added to the
# debug ninja scripts.

    $<$<CONFIG:Debug>:ttlib/src/winsrc/ttdebug_min.cpp>  # we use our own version of this to minimize Windows code requirements
)

if (MSVC)
    # /GL -- combined with the Linker flag /LTCG to perform whole program optimization in Release build
    # /FC -- Full path to source code file in diagnostics
    target_compile_options(ttBld PRIVATE "$<$<CONFIG:Release>:/GL>" "/FC" "/Zc:__cplusplus" "/utf-8")
    target_link_options(ttBld PRIVATE "$<$<CONFIG:Release>:/LTCG>")

    target_link_options(ttBld PRIVATE "$<$<CONFIG:Debug>:/natvis:../src/ttBld.natvis>")

    # Assume the manifest is in the resource file
    target_link_options(ttBld PRIVATE "/manifest:no")
endif()

target_precompile_headers(ttBld PRIVATE "src/precompile/pch.h")

target_include_directories(ttBld PRIVATE
    src/
    src/precompile
    src/ui
    src/convert
    src/../ttlib/include
)
