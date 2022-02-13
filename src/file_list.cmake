set (file_list

    ${CMAKE_CURRENT_LIST_DIR}/mainapp.cpp         # entry point, global strings, library pragmas

    ${CMAKE_CURRENT_LIST_DIR}/addfiles.cpp        # Used to add one or more filenames to a .srcfiles file
    ${CMAKE_CURRENT_LIST_DIR}/cmplrMsvc.cpp       # Creates .ninja scripts for MSVC and CLANG-CL compilers
    ${CMAKE_CURRENT_LIST_DIR}/createmakefile.cpp  # CreateMakeFile method for creating a makefile
    ${CMAKE_CURRENT_LIST_DIR}/csrcfiles.cpp       # CSrcFiles class for reading .srcfiles
    ${CMAKE_CURRENT_LIST_DIR}/dryrun.cpp          # CDryRun class for testing
    ${CMAKE_CURRENT_LIST_DIR}/finder.cpp          # Routines for finding executables such as the MSVC compiler
    ${CMAKE_CURRENT_LIST_DIR}/gencmdfiles.cpp     # Generates MSVCenv.cmd and Code.cmd files
    ${CMAKE_CURRENT_LIST_DIR}/gitfuncs.cpp        # Functions for working with .git
    ${CMAKE_CURRENT_LIST_DIR}/image_hdr.cpp       # Convert image into png header
    ${CMAKE_CURRENT_LIST_DIR}/make_hgz.cpp        # Converts a file into a .gz and stores as char array header
    ${CMAKE_CURRENT_LIST_DIR}/ninja.cpp           # CNinja for creating .ninja scripts
    ${CMAKE_CURRENT_LIST_DIR}/options.cpp         # contains all Options strings and CSrcOptions class for working with them
    ${CMAKE_CURRENT_LIST_DIR}/rcdep.cpp           # Contains functions for parsing RC dependencies
    ${CMAKE_CURRENT_LIST_DIR}/verninja.cpp        # CVerMakeNinja class
    ${CMAKE_CURRENT_LIST_DIR}/vs.cpp              # Creates .vs/tasks.vs.json and .vs/launch.vs.json
    ${CMAKE_CURRENT_LIST_DIR}/vscode.cpp          # Creates/updates .vscode files
    ${CMAKE_CURRENT_LIST_DIR}/writesrc.cpp        # Writes a new or update srcfiles.yaml file
    ${CMAKE_CURRENT_LIST_DIR}/yamalize.cpp        # Used to convert .srcfiles to .vscode/srcfiles.yaml

    ${CMAKE_CURRENT_LIST_DIR}/convert/convert.cpp          # Various conversion methods
    ${CMAKE_CURRENT_LIST_DIR}/convert/readcodelite.cpp     # Class for converting a CodeLite .project file to .srcfiles.yaml
    ${CMAKE_CURRENT_LIST_DIR}/convert/readdsp.cpp          # Class for converting a Visual Studio .DSP file to .srcfiles.yaml
    ${CMAKE_CURRENT_LIST_DIR}/convert/readvc.cpp           # Class for converting a Visual Studio .vcproj file to .srcfiles.yaml
    ${CMAKE_CURRENT_LIST_DIR}/convert/readvcx.cpp          # Converts a Visual Studio project file into .srcfiles.yaml
    ${CMAKE_CURRENT_LIST_DIR}/convert/writevcx.cpp         # Create a Visual Studio project file
    ${CMAKE_CURRENT_LIST_DIR}/convert/write_cmake.cpp      # Create a CMakeLists.txt file
    ${CMAKE_CURRENT_LIST_DIR}/convert/wxWidgets_file.cpp   # Convert wxWidgets build/file to CMake file list

    # DO NOT RUN clang-format on the following file! We need to be able to sync changes from the original repository
    ${CMAKE_CURRENT_LIST_DIR}/pugixml/pugixml.cpp          # Built directly rather than building a library

    # wxUiEditor derived classes

    ${CMAKE_CURRENT_LIST_DIR}/ui/convertdlg.cpp            # Dialog specifying what to convert into a .srcfiles.yaml file
    ${CMAKE_CURRENT_LIST_DIR}/ui/optionsdlg.cpp            # Dialog for setting all .srcfile options
    ${CMAKE_CURRENT_LIST_DIR}/ui/vscodedlg.cpp             # Dialog for setting options to create tasks.json and launch.json

)
