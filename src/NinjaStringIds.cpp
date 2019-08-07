/////////////////////////////////////////////////////////////////////////////
// Name:      NinjaStringIds.cpp
// Purpose:   Maps IDS_NINJA_ strings into string to translate
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

// The risk of putting strings directly into the code is that you can end up with strings that were supposed to be the same, but have a
// a slight variation, such as an extra space that results in two or more strings that have to be translated. If all strings are placed
// here and accessed with the GETSTRING(id) macro then you avoid any accidental variations.

#include "pch.h"

#include <ttlist.h>    // ttCList

#include "NinjaStringIds.h"

typedef struct
{
    size_t id;
    const char* psz;
} ttIDS_LIST;

static const ttIDS_LIST aNinjaStringIds[] =
{
    { IDS_CANT_FIND_COMPILER,        _XGET("Unable to find either the cl.exe or clang-cl.exe compilers in PATH.") },
    { IDS_MAKENINJA_CORRUPTED,       _XGET("ttBld.exe is corrupted -- cannot read the necessary resource.") },

    // TRANSLATORS: "makefile" is a filename
    { IDS_CANT_WRITE_MAKEFILE,       _XGET("Unable to write to makefile.") },

    { IDS_LIB_DIR_FAIL,              _XGET("The library source directory %s specified in BuildLibs: does not exist.") },
    { IDS_NINJA_32BIT_DIR,           _XGET("Select 32-bit Target directory") },
    { IDS_NINJA_64BIT_DIR,           _XGET("Select 64-bit target directory") },
    { IDS_NINJA_ALREADY_ADDED_DIR,   _XGET("You've already added the directory %s") },
    { IDS_NINJA_CANNOT_OPEN,         _XGET("Cannot open \042%s\042.") },
    { IDS_NINJA_CANT_WRITE,          _XGET("Unable to create or write to %s") },
    { IDS_NINJA_CONVERSION_FAILED,   _XGET("Conversion of %s failed!") },
    { IDS_NINJA_LIBDIR,              _XGET("Library directory") },
    { IDS_NINJA_CANNOT_LOCATE,       _XGET("Cannot locate the file %s") },
    { IDS_NINJA_ALREAY_IN_FILES,     _XGET("The file %s is alreay in .srcfiles.yaml\n") },
    { IDS_NINJA_FILE_NOT_ADDED,      _XGET("The file %s doesn't exist -- not added\n") },

    { IDS_NINJA_FILES_ADDED,         _XGET("%u files added.") },
    { IDS_NINJA_NAME_ADDED,          _XGET("The name %kq has already been added.") },
    { IDS_NINJA_MISSING_COLON,       _XGET("Option name is missing a ':' (%s)") },
    { IDS_NINJA_DRURUN_CHANGES,      _XGET("%s dryrun changes:\n") },

    // TRANSLATORS: Don't change the amount of leading spaces
    { IDS_NINJA_OLD_FILE,            _XGET("    old: %s\n") },
    // TRANSLATORS: Don't change the amount of leading spaces
    { IDS_NINJA_NEW_FILE,            _XGET("    new: %s\n") },

    // TRANSLATORS: Error while converting an IDE script into .srcfiles.yaml
    { IDS_NINJA_INVALID_PROJECT,     _XGET("Cannot locate <CodeLite_Project> in %s") },
    // TRANSLATORS: Error while converting an IDE script into .srcfiles.yaml
    { IDS_NINJA_MISSING_FILES,       _XGET("Cannot locate <Files> in %s") },
    // TRANSLATORS: Error while converting an IDE script into .srcfiles.yaml
    { IDS_NINJA_MISSING_FILTER,      _XGET("Cannot locate <Filter> in %s") },
    // TRANSLATORS: Error while converting an IDE script into .srcfiles.yaml
    { IDS_NINJA_MISSING_INCLUDE,     _XGET("%s(%kt,%kt):  warning: cannot locate include file %s") },
    // TRANSLATORS: Error while converting an IDE script into .srcfiles.yaml
    { IDS_NINJA_MISSING_PROJECT,     _XGET("Cannot locate <Project> in %s") },
    // TRANSLATORS: Error while converting an IDE script into .srcfiles.yaml
    { IDS_NINJA_MISSING_VSP,         _XGET("Cannot locate <VisualStudioProject> in %s") },

    { IDS_NINJA_OPT_MISSING_COLON,   _XGET("Option name is missing a ':' (%s)") },
    { IDS_NINJA_PARSE_ERROR,         _XGET("An internal error occurred attempting to parse the file %s") },
    { IDS_NINJA_SRCFILES_CREATED,    _XGET(".srcfiles.yaml created using %s as the template") },
    { IDS_NINJA_SRCFILES_EXISTS,     _XGET(".srcfiles.yaml already exists in this directory. Are you sure you want to replace it?") },

    // TRANSLATORS: Used for the Tab name of the tab that sets general options
    { IDS_NINJA_TAB_GENERAL,             _XGET("General") },
    // TRANSLATORS: Used for the Tab name of the tab that sets general C compiler options
    { IDS_NINJA_TAB_COMPILER,        _XGET("Compiler") },
    // TRANSLATORS: Used for the Tab name of the tab that sets Linker options
    { IDS_NINJA_TAB_LINKER,          _XGET("Linker") },
    // TRANSLATORS: Used for the Tab name of the tab that sets general scripting options (for IDE, makefile, etc.)
    { IDS_NINJA_TAB_SCRIPTS,         _XGET("Scripts") },

    // TRANSLATORS: Displayed when ttBld doesn't recognize an option in the Options section of a .srcfiles.yaml file
    { IDS_NINJA_UNKNOWN_OPTION,      _XGET("%s is an unknown option") },
    { IDS_NINJA_OPTIONS_UPDATED,     _XGET("%s Options: section updated.\n") },

    { IDS_FILE_CREATED,    _XGET("%s created.\n") },
    { IDS_FILE_UPDATED,    _XGET("%s updated.\n") },

    { IDS_NINJA_UP_TO_DATE,          _XGET("All ninja scripts are up to date.") },

    // TRANSLATORS: Uses a dialog box title string
    { IDS_NINJA_TITLE_LIB_DIR,          _XGET("Library directory") },

    // TRANSLATORS: this string should never be used unless there is a bug in the code
    { IDS_NINJA_END_LIST,            _XGET("missing string id") },
};

static ttCIntStrList clsIntStrList;

const char* NinjaGetString(size_t id)
{
    const char* pszString = clsIntStrList.Find(id);
    if (pszString)
        return pszString;

    size_t pos;
    for (pos = 0; aNinjaStringIds[pos].id != IDS_NINJA_END_LIST; ++pos)
    {
        if (aNinjaStringIds[pos].id == id)
            break;
    }

    // Note that if the id is not found, it will get mapped to the same string as IDS_END_LIST

#if defined(_WX_DEFS_H_)
    return clsIntStrList.Add(id, wxGetTranslation(aNinjaStringIds[pos].psz).utf8_str());
#else
    return clsIntStrList.Add(id, aNinjaStringIds[pos].psz);
#endif
}

void NinjaResetStringIDs()
{
    clsIntStrList.Delete();
}
