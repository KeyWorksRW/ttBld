// Purpose: String resource IDs

// If you use the _tt() macro (see ttTR.h in ttLib) then it will load a string resource when compiled for Windows. If compiled with
// wxWidgets on a non-Windows platform, it will load the translated string produced by the xgettext system that wxWidgets supports.

// Note: Windows saves strings in consecutive blocks of 16, so it is useful to keep id numbers consecutive.

#ifdef _WIN32
// #if 0  // comment out above and uncomment this to test non-Windows builds on a Windows platform

    #define IDS_CANNOT_OPEN_VIEW         1024  // "Cannot open %v"
    #define IDS_CANNOT_OPEN              1025  // "Cannot open "
    #define IDS_CANT_FIND_SRCFILES       1026  // "ttBld was unable to locate a .srcfiles.yaml file..."
    #define IDS_OLD_TTBLD                1027  // "This version of ttBld is too old -- you need a newer version..."
    #define IDS_UP_TO_DATE               1028  // "All ninja scripts are up to date."
    #define IDS_CREATED                  1029  // "Created "
    #define IDS_FILES                    1030  // " files"
    #define IDS_UPDATED                  1031  // " updated."
    #define IDS_FMT_FILES_LOCATED        1032  // "%kzd files located"
    #define IDS_PROJECT_FILES            1033  // "Project Files"
    #define IDS_CONFIRM_REPLACE_SRCFILES 1034  // ".srcfiles.yaml already exists in this directory. Are you sure you want to replace it?"
    #define IDS_ADDED_IGNORE_FILES       1035  // "Added directories and filenames to ignore to "
    #define IDS_CANT_CREATE              1036  // "Unable to create or write to "
    #define IDS_CREATED_SUFFIX           1037  // " created"
    #define IDS_CANT_CREATE_VS_DIR       1038  // "Unable to create the required .vs directory."
    #define IDS_MISSING_PROJ_NAME        1039  // "Project name not specified in "
    #define IDS_CANT_CREATE_VSCODE_DIR   1040  // "Unable to create the required .vscode directory."
    #define IDS_ASK_IGNORE_VSCODE        1041  // "The directory .vscode is not being ignored by git. Would you like it to be added to "
    #define IDS_VSCODE_IGNORED           1042  // ".vscode/ added to "
    #define IDS_CANT_CONFIGURE_JSON      1043  // "Cannot locate a .srcfiles.yaml file need to configure .vscode/*.json files."
    #define IDS_UPTODATE_SUFFIX          1044  // " is up to date"
    #define IDS_CANT_CREATE_VCX_GUID     1045  // "Unable to create a UUID -- cannot create .vcxproj without it."
    #define IDS_OPTIONS_UPDATED          1046  // "%s Options: section updated."
    #define IDS_MISSING_PCH_CPP          1047  // " does not have a matching C++ source file -- precompiled header will fail without it!"
    #define IDS_TITLE_LIB_DIR            1048  // "Library directory"
    #define IDS_ALREADY_ADDED_DIR        1049  // "You've already added the directory "
    #define IDS_CANT_LOCATE_SRCFILES     1050  // "Cannot locate .srcfiles.yaml."
    #define IDS_MAKEFILE_NOT_CREATED     1051  // "Makefile not created."
    #define IDS_APP_CORRUPTED            1052  // "ttBld.exe is corrupted."
    #define IDS_EXCEPTION_READING        1053  // "An exception occurred while reading "
    #define IDS_LIB_SOURCE_DIR           1054  // "The library source directory "
    #define IDS_INVALID_BUILDLIB_SUFFIX  1055  // " specified in BuildLibs: does not exist."
    #define IDS_MISSING_SRCFILES_IN      1056  // "Cannot read .srcfiles.yaml in "
    #define IDS_DISPLAY_HELP_MSG         1057  // "display this help message"
    #define IDS_MISSING_MAKEFILE_DIR     1058  // "Directory for makefile doesn't exist: "
    #define IDS_CODECMD_HELP_MSG         1059  // "create code32.cmd and code64.cmd batch files"
    #define IDS_CONVERT_HELP_MSG         1060  // "converts build script file (.e.g., file.vcxproj) to .srcfiles.yaml"
    #define IDS_MAKEFILE_EXISTS          1061  // "-makefile cannot be used if the makefile already exists"
    #define IDS_VCXPROJ_HELP_MSG         1062  // "creates or updates files needed to build project using MS Visual Studio"
    #define IDS_FORCE_HELP_MSG           1063  // "create .ninja file even if nothing has changed"
    #define IDS_DIR_HELP_MSG             1064  // "(directory) -- uses specified directory to create .ninja files (default is bld/)"
    #define IDS_MAKEFILE_HELP_MSG        1065  // "creates a makefile that doesn't require ttBld.exe"
    #define IDS_OPTIONS_HELP_MSG         1066  // "displays a dialog allowing you to change options in .srcfiles.yaml"
    #define IDS_VSCODE_HELP_MSG          1067  // "creates or updates files used to build and debug a project using VS Code"

#else

constexpr auto IDS_CANNOT_OPEN_VIEW = "Cannot open %v";
constexpr auto IDS_ADDED_IGNORE_FILES = "Added directories and filenames to ignore to ";
constexpr auto IDS_CANT_CREATE = "Unable to create or write to ";
constexpr auto IDS_CREATED_SUFFIX = " created";
constexpr auto IDS_CANT_CREATE_VS_DIR = "Unable to create the required .vs directory.";
constexpr auto IDS_MISSING_PROJ_NAME = "Project name not specified in ";
constexpr auto IDS_CANT_CREATE_VSCODE_DIR = "Unable to create the required .vscode directory.";
constexpr auto IDS_ASK_IGNORE_VSCODE = "The directory .vscode is not being ignored by git. Would you like it to be added to ";
constexpr auto IDS_VSCODE_IGNORED = ".vscode/ added to ";
constexpr auto IDS_CANT_CONFIGURE_JSON = "Cannot locate a .srcfiles.yaml file need to configure .vscode/*.json files.";
constexpr auto IDS_UPTODATE_SUFFIX = " is up to date";
constexpr auto IDS_CANT_CREATE_VCX_GUID = "Unable to create a UUID -- cannot create .vcxproj without it.";
constexpr auto IDS_OPTIONS_UPDATED = " Options: section updated.";
constexpr auto IDS_MISSING_PCH_CPP = " does not have a matching C++ source file -- precompiled header will fail without it!";
constexpr auto IDS_TITLE_LIB_DIR = "Library directory";
constexpr auto IDS_ALREADY_ADDED_DIR = "You've already added the directory ";
constexpr auto IDS_CANT_LOCATE_SRCFILES = "Cannot locate .srcfiles.yaml.";
constexpr auto IDS_MAKEFILE_NOT_CREATED = "Makefile not created.";
constexpr auto IDS_APP_CORRUPTED = "ttBld.exe is corrupted.";
constexpr auto IDS_EXCEPTION_READING = "An exception occurred while reading ";
constexpr auto IDS_LIB_SOURCE_DIR = "The library source directory ";
constexpr auto IDS_INVALID_BUILDLIB_SUFFIX = " specified in BuildLibs: does not exist.";
constexpr auto IDS_MISSING_SRCFILES_IN = "Cannot read .srcfiles.yaml in ";
constexpr auto IDS_DISPLAY_HELP_MSG = "display this help message";
constexpr auto IDS_MISSING_MAKEFILE_DIR = "Directory for makefile doesn't exist: ";
constexpr auto IDS_CODECMD_HELP_MSG = "create code32.cmd and code64.cmd batch files";
constexpr auto IDS_CONVERT_HELP_MSG = "converts build script file (.e.g., file.vcxproj) to .srcfiles.yaml";
constexpr auto IDS_MAKEFILE_EXISTS = "-makefile cannot be used if the makefile already exists";
constexpr auto IDS_VCXPROJ_HELP_MSG = "creates or updates files needed to build project using MS Visual Studio";
constexpr auto IDS_FORCE_HELP_MSG = "create .ninja file even if nothing has changed";
constexpr auto IDS_DIR_HELP_MSG = "(directory) -- uses specified directory to create .ninja files (default is bld/)";
constexpr auto IDS_MAKEFILE_HELP_MSG = "creates a makefile that doesn't require ttBld.exe";
constexpr auto IDS_OPTIONS_HELP_MSG = "displays a dialog allowing you to change options in .srcfiles.yaml";
constexpr auto IDS_VSCODE_HELP_MSG = "creates or updates files used to build and debug a project using VS Code";
constexpr auto IDS_CANT_FIND_SRCFILES =
    "ttBld was unable to locate a .srcfiles.yaml file -- either use the -new option, or set the location with -dir.";
constexpr auto IDS_OLD_TTBLD = "This version of ttBld is too old -- you need a newer version to correctly build the script files.";
constexpr auto IDS_UP_TO_DATE = "All ninja scripts are up to date.";
constexpr auto IDS_CREATED = "Created ";
constexpr auto IDS_FILES = " files";
constexpr auto IDS_UPDATED = " updated.";
constexpr auto IDS_FMT_FILES_LOCATED = "%kzd files located";
constexpr auto IDS_PROJECT_FILES = "Project Files";
constexpr auto IDS_CONFIRM_REPLACE_SRCFILES = ".srcfiles.yaml already exists in this directory. Are you sure you want to replace it?";
constexpr auto IDS_CANNOT_OPEN = "Cannot open ";

#endif
