/////////////////////////////////////////////////////////////////////////////
// Purpose:   Contains translatable strings
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <map>

#include "strings.h"

const std::map<int, const char*> englishStrings = {

    { stdIdTitleInclude, "Include Directory" },
    { strIdProjectFiles, "Project Files" },
    { strIdCantWrite, "Unable to create or write to " },
    { strIdCantOpen, "Cannot open " },
    { strIdUpdated, " updated." },
    { strIdFiles, " files" },
    { strIdCreated, "Created " },
    { strIdAllNinjaCurrent, "All ninja scripts are up to date." },
    { strIdRecursiveBld, "You cannot create the current project as a Build Library of itself." },
    { strIdTitleTargetDir, "Target directory" },
    { strIdHelpMakefile, "creates a makefile that doesn't require ttBld.exe" },
    { strIdHelpForce, "create .ninja file even if nothing has changed" },
    { strIdHelpVscode, "creates or updates files used to build and debug a project using VS Code" },
    { strIdHelpOptions, "displays a dialog allowing you to change options in .srcfiles.yaml" },
    { strIdHelpDir, "(directory) -- uses specified directory to create .ninja files (default is bld/)" },
    { strIdHelpCodecmd, "create code32.cmd and code64.cmd batch files" },
    { strIdHelpMsg, "display this help message" },
    { strIdHelpVcxproj, "creates or updates files needed to build project using MS Visual Studio" },
    { strIdMissingSrcfilesIn, "Cannot read .srcfiles.yaml in " },
    { strIdInvalidBuildlib, " specified in BuildLibs: does not exist." },
    { strIdMakefileExists, "-makefile cannot be used if the makefile already exists" },
    { strIdLibSrcDir, "The library source directory " },
    { strIdExceptReading, "An exception occurred while reading " },
    { strIdExeCorrupted, "The ttBld.exe program is corrupted." },
    { strIdMakefileNotCreate, "Makefile not created." },
    { strIdDirAlreadyAdded, "You've already added the directory " },
    { strIdTitleLibDirectory, "Library directory" },
    { strIdMissingPchCpp, " does not have a matching C++ source file -- precompiled header will fail without it!" },
    { strIdOptionsUpdated, " Options: section updated." },
    { strIdCantCreateUuid, "Unable to create a UUID -- cannot create .vcxproj without it." },
    { strIdCurrent, " is up to date" },
    { strIdCantConfigureJson, "Cannot locate a project file (typically .srcfiles.yaml) need to configure .vscode/*.json files." },
    { strIdVscodeIgnored, ".vscode/ added to " },
    { strIdQueryIgnore, "The directory .vscode/ is not being ignored by git. Would you like it to be added to " },
    { strIdCantCreateVsCodeDir, "Unable to create the required .vscode/ directory." },
    { strIdMissingProjectName, "Project name not specified in " },
    { strIdCantCreateVsDir, "Unable to create the required .vs/ directory." },
    { strIdCreatedSuffix, " created" },
    { strIdConfirmReplace, ".srcfiles.yaml already exists in this directory. Are you sure you want to replace it?" },
    { strIdFmtFilesLocated, "%kzd files located" },
    { strIdOldVersion, "This version of ttBld is too old -- you need a newer version to correctly build the script files." },
    { strIdMissingSrcfiles,
      "ttBld was unable to locate a .srcfiles.yaml file -- either use the -new option, or set the location with -dir." },
    { strIdIgnoredFiles, "Added directories and filenames to ignore to " },
    { strIdInternalError, "An internal error has occurred: " },

};

const std::map<int, const char*>* _tt_english = &englishStrings;
const std::map<int, const char*>* _tt_CurLanguage = &englishStrings;
