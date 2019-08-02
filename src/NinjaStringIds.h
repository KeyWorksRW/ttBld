/////////////////////////////////////////////////////////////////////////////
// Name:      NinjaStringIds.h
// Purpose:   Maps IDS_NINJA_ strings into string to translate
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

// Use this macro to return a const char* of a translated string

#define GETSTRING(id) NinjaGetString(id)

// NinjaGetString will store a translated string for fast retrieval if it is needed again.

const char* NinjaGetString(size_t id);      // use id from the following enumerated list (this is preferred)

void NinjaResetStringIDs();                 // call this before switching to a different language

enum
{
    IDS_FILE_UPDATED,                   // %s updated.\n
    IDS_CANT_FIND_COMPILER,             // Unable to find either the cl.exe or clang-cl.exe compilers in PATH.
    IDS_MAKENINJA_CORRUPTED,            // ttMakeNinja.exe is corrupted -- cannot read the necessary resource.
    IDS_CANT_WRITE_MAKEFILE,            // Unable to write to makefile.
    IDS_LIB_DIR_FAIL,                   // The library source directory %s specified in BuildLibs: does not exist.
    IDS_NINJA_32BIT_DIR,                // "Select 32-bit Target directory"
    IDS_NINJA_64BIT_DIR,                // "Select 64-bit target directory"
    IDS_NINJA_ALREADY_ADDED_DIR,        // "You've already added the directory %s"
    IDS_NINJA_ALREAY_IN_FILES,          // "The file %s is alreay in .srcfiles.yaml\n"
    IDS_NINJA_CANNOT_LOCATE,            // "Cannot locate the file %s"
    IDS_NINJA_CANNOT_OPEN,              // "Cannot open \042%s\042."
    IDS_NINJA_CANT_WRITE,               // "Unable to create or write to %s"
    IDS_NINJA_CONVERSION_FAILED,        // "Conversion of %s failed!"
    IDS_FILE_CREATED,                  // "%s created.\n"
    IDS_NINJA_DRURUN_CHANGES,           // "%s dryrun changes:\n"
    IDS_NINJA_FILES_ADDED,              // "%u files added."
    IDS_NINJA_FILE_NOT_ADDED,           // "The file %s doesn't exist -- not added\n"
    IDS_NINJA_INVALID_PROJECT,          // "Cannot locate <CodeLite_Project> in %s"
    IDS_NINJA_LIBDIR,                   // "Library directory"
    IDS_NINJA_MISSING_COLON,            // "Option name is missing a ':' (%s)"
    IDS_NINJA_MISSING_FILES,            // "Cannot locate <Files> in %s"
    IDS_NINJA_MISSING_FILTER,           // "Cannot locate <Filter> in %s"
    IDS_NINJA_MISSING_INCLUDE,          // "%s(%kt,%kt):  warning: cannot locate include file %s"
    IDS_NINJA_MISSING_PROJECT,          // "Cannot locate <Project> in %s"
    IDS_NINJA_MISSING_VSP,              // "Cannot locate <VisualStudioProject> in %s"
    IDS_NINJA_NAME_ADDED,               // "The name %kq has already been added."
    IDS_NINJA_NEW_FILE,                 // "    new: %s\n"
    IDS_NINJA_OLD_FILE,                 // "    old: %s\n"
    IDS_NINJA_OPTIONS_UPDATED,          // "%s Options: section updated.\n"
    IDS_NINJA_OPT_MISSING_COLON,        // "Option name is missing a ':' (%s)"
    IDS_NINJA_PARSE_ERROR,              // "An internal error occurred attempting to parse the file %s"
    IDS_NINJA_SRCFILES_CREATED,         // ".srcfiles.yaml created using %s as the template"
    IDS_NINJA_SRCFILES_EXISTS,          // ".srcfiles.yaml already exists in this directory. Are you sure you want to replace it?"
    IDS_NINJA_TAB_CLANG,                // "CLang"
    IDS_NINJA_TAB_COMPILER,             // "Compiler"
    IDS_NINJA_TAB_GENERAL,              // "General"
    IDS_NINJA_TAB_LINKER,               // "Linker"
    IDS_NINJA_TAB_RC_MIDL,              // "Rc/Midl"
    IDS_NINJA_TAB_SCRIPTS,              // "Scripts"
    IDS_NINJA_TITLE_LIB_DIR,            // "Library directory"
    IDS_NINJA_UNKNOWN_OPTION,           // "%s is an unknown option"
    IDS_NINJA_UP_TO_DATE,               // "All ninja scripts are up to date."

    // IDS_NINJA_END_LIST must always be last to indicate the end of the list. Do NOT hard-code it's value! (or hashed
    // strings may conflict)

    IDS_NINJA_END_LIST
};
