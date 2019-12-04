/////////////////////////////////////////////////////////////////////////////
// Name:      CSrcOptions
// Purpose:   Class for storing/retrieving options in a .srcfiles.yaml file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

/*
    Ideally, s_aOptions would be in a namespace. However, the Visual Studio debugger (2017 version) is not able to
    display the array even if fully qualified with the namespace. That makes debugging rather difficult since a lot of
    the functionality of ttBld relies on comparison between a modified option and the original.
*/

#include "pch.h"

#include "options.h"

using namespace sfopt;

const char* txtOptVersion = "1.4.0";  // any time you add an option below, you need to increment this version number and
                                      // then add it to the OPT_VERSION list

// clang-format off
sfopt::OPT_VERSION aoptVersions[] =
{
    { OPT_STATIC_CRT_REL, 1, 2, 0 },
    { OPT_WARN_LEVEL, 1, 2, 0 },
    { OPT_OPTIMIZE, 1, 2, 0 },

    { OPT_XGET_OUT, 1, 3, 0 },
    { OPT_XGET_KEYWORDS, 1, 3, 0 },
    { OPT_XGET_FLAGS, 1, 3, 0 },
    { OPT_MSGFMT_FLAGS, 1, 3, 0 },
    { OPT_MSGFMT_XML, 1, 3, 0 },

    { OPT_TARGET_DIR, 1, 4, 0 },

    // All options default to 1.0.0, so only add options above that require a newer version of ttBld

    { OPT_OVERFLOW, 1, 0, 0  }
};
// clang-format on

// Add these in the order you want them written in a new .srcfiles.yaml files.

// clang-format off
static const OPT_SETTING s_aOptions[] =
{
    // { OPT_xxx,       "name",         "value",   boolean, required, "comment" }

    { OPT_PROJECT,      "Project",      nullptr,   false,   true,      _XGET("project name") },
    { OPT_EXE_TYPE,     "Exe_type",     "console", false,   true,     "[window | console | lib | dll]" },
    { OPT_PCH,          "Pch",          "none",    false,   true,      _XGET("name of precompiled header file, or \042none\042 if not using precompiled headers") },
    { OPT_PCH_CPP,      "Pch_cpp",      "none",    false,   false,     _XGET("source file used to build precompiled header (default uses same name as PCH option)") },

    { OPT_OPTIMIZE,     "Optimize",     "space",   false,   true,      _XGET("[space | speed] optimization to use in release builds") },
    { OPT_WARN_LEVEL,   "Warn",         "4",       false,   true,      _XGET("[1-4] warning level") },

    { OPT_STATIC_CRT_REL, "Crt_rel",    "static",  false,   true,      _XGET("[static | dll] type of CRT to link to in release builds") },
    { OPT_STATIC_CRT_DBG, "Crt_dbg",    "static",  false,   true,      _XGET("[static | dll] type of CRT to link to in debug builds") },

    { OPT_TARGET_DIR, "TargetDir",      nullptr,   false,   false,     _XGET("target directory") },

    { OPT_64BIT,        "64Bit",        "true",    true,    false,      _XGET("[true | false] indicates if project can be built as a 64-bit target") },
    { OPT_TARGET_DIR64, "TargetDir64",  nullptr,   false,   false,     _XGET("64-bit target directory") },

    { OPT_32BIT,        "32Bit",        "false",   true,    false,      _XGET("[true | false] indicates if project can be built as a 32-bit target") },
    { OPT_TARGET_DIR32, "TargetDir32",  nullptr,   false,   false,     _XGET("32-bit target directory") },

    { OPT_PERMISSIVE,   "Permissive",   "false",   true,    false,     _XGET("true means add -permissive- compiler flag") },
    { OPT_STDCALL,      "Stdcall",      "false",   true,    false,     _XGET("true to use stdcall calling convention, false for cdecl (default)") },

    { OPT_CFLAGS_CMN,   "CFlags_cmn",   nullptr,   false,   false,     _XGET("flags to pass to the compiler in all builds") },
    { OPT_CFLAGS_REL,   "CFlags_rel",   nullptr,   false,   false,     _XGET("flags to pass to the compiler in release builds") },
    { OPT_CFLAGS_DBG,   "CFlags_dbg",   nullptr,   false,   false,     _XGET("flags to pass to the compiler in debug builds") },

    { OPT_CLANG_CMN,    "Clang_cmn",    nullptr,   false,   false,     _XGET("flags to pass only to the clang compiler in all builds") },
    { OPT_CLANG_REL,    "Clang_rel",    nullptr,   false,   false,     _XGET("flags to pass only to the clang compiler in release builds") },
    { OPT_CLANG_DBG,    "Clang_dbg",    nullptr,   false,   false,     _XGET("flags to pass only to the clang compiler in debug builds") },

    { OPT_LINK_CMN,     "LFlags_cmn",   nullptr,   false,   false,     _XGET("flags to pass to the linker in all builds") },
    { OPT_LINK_REL,     "LFlags_rel",   nullptr,   false,   false,     _XGET("flags to pass to the linker in release builds") },
    { OPT_LINK_DBG,     "LFlags_dbg",   nullptr,   false,   false,     _XGET("flags to pass to the linker in debug builds") },

    { OPT_NATVIS,       "Natvis",       nullptr,   false,   false,     _XGET("MSVC Debug visualizer") },

    { OPT_RC_CMN,       "Rc_cmn",       nullptr,   false,   false,     _XGET("flags to pass to the resource compiler in all builds") },
    { OPT_RC_REL,       "Rc_rel",       nullptr,   false,   false,     _XGET("flags to pass to the resource compiler in release builds") },
    { OPT_RC_DBG,       "Rc_dbg",       nullptr,   false,   false,     _XGET("flags to pass to the resource compiler in debug builds") },

    { OPT_MDL_CMN,      "Midl_cmn",     nullptr,   false,   false,     _XGET("flags to pass to the midl compiler in all builds") },
    { OPT_MDL_REL,      "Midl_rel",     nullptr,   false,   false,     _XGET("flags to pass to the midl compiler in release builds") },
    { OPT_MDL_DBG,      "Midl_dbg",     nullptr,   false,   false,     _XGET("flags to pass to the midl compiler in debug builds") },

    { OPT_DEBUG_RC,     "DebugRC",      "false",   true,    false,     _XGET("true means build a -D_DEBUG version of the project's rc file") },

    { OPT_MS_LINKER,    "Ms_linker",    "true",    true,    false,     _XGET("true means use link.exe even when compiling with clang") },
    { OPT_MS_RC,        "Ms_rc",        "true",    true,    false,     _XGET("use rc.exe even when compiling with CLANG-CL") },

    { OPT_INC_DIRS,     "IncDirs",      nullptr,   false,   false,     _XGET("additional directories for header files") },
    { OPT_BUILD_LIBS,   "BuildLibs",    nullptr,   false,   false,     _XGET("libraries that need to be built (added to makefile generation)") },

    { OPT_LIB_DIRS,     "LibDirs",      nullptr,   false,   false,     _XGET("additional directories for library files") },
    { OPT_LIB_DIRS64,   "LibDirs64",    nullptr,   false,   false,     _XGET("additional directories for 64-bit library files") },
    { OPT_LIB_DIRS32,   "LibDirs32",    nullptr,   false,   false,     _XGET("additional directories for 32-bit library files") },

    { OPT_LIBS_CMN,     "Libs_cmn",     nullptr,   false,   false,     _XGET("additional libraries to link to in all builds") },
    { OPT_LIBS_REL,     "Libs_rel",     nullptr,   false,   false,     _XGET("additional libraries to link to in release builds") },
    { OPT_LIBS_DBG,     "Libs_dbg",     nullptr,   false,   false,     _XGET("additional libraries to link to in debug builds") },

    // The following options are for xgettext/msgfmt support

    { OPT_XGET_OUT,      "XGet_out",     nullptr, false, false, _XGET("output filename for xgettext") },
    { OPT_XGET_KEYWORDS, "XGet_kwrds",   nullptr, false, false, _XGET("additional keywords (separated by semi-colon) to pass to xgettext") },
    { OPT_XGET_FLAGS,    "XGet_flags",   nullptr, false, false, _XGET("additional flags to pass to xgettext") },
    { OPT_MSGFMT_FLAGS,  "Msgfmt_flags", nullptr, false, false, _XGET("additional flags to pass to msgfmt") },
    { OPT_MSGFMT_XML,    "Msgfmt_xml",   nullptr, false, false, _XGET("the name of the xml template file for msgfmt to use") },

//    { OPT_LIB_DIRS,     "LibDirs",      nullptr,   false,   false,     _XGET("additional directores for lib files") },
//    { OPT_LIBS,         "Libs",         nullptr,   false,   false,     _XGET("additional libraries to link to (see OPT_BUILD_LIBS to both build and link to a library)") },
//    { OPT_LIBS_REL,     "LibsR",        nullptr,   false,   false,     _XGET("additional libraries to link to (see OPT_BUILD_LIBS to both build and link to a library)") },
//    { OPT_LIBS_DBG,     "LibsD",        nullptr,   false,   false,     _XGET("additional libraries to link to (see OPT_BUILD_LIBS to both build and link to a library)") },

    { OPT_OVERFLOW, "", "", false, false, "" },
};
// clang-format on

const OPT_SETTING* CSrcOptions::GetOrgOptions()
{
    return s_aOptions;
}

CSrcOptions::CSrcOptions()
{
    // By adding OPT_UPDATE structures in the same order as OPT_SETTING, a position in m_aUpdateOpts matches the same
    // position in s_aOptions

    for (size_t pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        size_t posOpt = m_aUpdateOpts.Add();
        m_aUpdateOpts[posOpt].pszVal = s_aOptions[pos].pszVal ? ttStrDup(s_aOptions[pos].pszVal) : nullptr;
        m_aUpdateOpts[posOpt].pszComment =
            ttIsNonEmpty(s_aOptions[pos].pszComment) ? ttStrDup(s_aOptions[pos].pszComment) : nullptr;
        m_aUpdateOpts[posOpt].bRequired = s_aOptions[pos].bRequired;
    }
}

CSrcOptions::~CSrcOptions()
{
    for (size_t pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        ttFree(m_aUpdateOpts[pos].pszVal);
        ttFree(m_aUpdateOpts[pos].pszComment);
    }
}

// fine to call this for boolean options if pszVal == "true/false" or "yes/no"
sfopt::OPT_INDEX CSrcOptions::UpdateOption(sfopt::OPT_INDEX index, const char* pszVal)
{
    size_t pos;
    for (pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aOptions[pos].opt == index)
            break;
    }
    ttASSERT(s_aOptions[pos].opt != OPT_OVERFLOW);
    if (s_aOptions[pos].opt == OPT_OVERFLOW)
        return OPT_OVERFLOW;  // invalid option
    ttFree(m_aUpdateOpts[pos].pszVal);

    if (!pszVal)
    {
        m_aUpdateOpts[pos].pszVal = nullptr;
        return s_aOptions[pos].opt;
    }

    // We want boolean values to be consistent, so if "yes" or "no" is specified, convert to "true" or "false"

    if (ttIsSameStrI(pszVal, "yes"))
        pszVal = "true";
    else if (ttIsSameStrI(pszVal, "no"))
        pszVal = "false";

    m_aUpdateOpts[pos].pszVal = ttStrDup(pszVal);
    return s_aOptions[pos].opt;
}

sfopt::OPT_INDEX CSrcOptions::UpdateOption(sfopt::OPT_INDEX index, bool bVal)
{
    size_t pos;
    for (pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aOptions[pos].opt == index)
            break;
    }
    ttASSERT(s_aOptions[pos].opt != OPT_OVERFLOW);
    if (s_aOptions[pos].opt == OPT_OVERFLOW)
        return OPT_OVERFLOW;  // invalid option

    ttFree(m_aUpdateOpts[pos].pszVal);
    m_aUpdateOpts[pos].pszVal = ttStrDup(bVal ? "true" : "false");
    return s_aOptions[pos].opt;
}

const char* CSrcOptions::GetOption(sfopt::OPT_INDEX index)
{
    for (size_t pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aOptions[pos].opt == index)
            return m_aUpdateOpts[pos].pszVal;
    }
    return nullptr;
}

const char* CSrcOptions::GetOptComment(sfopt::OPT_INDEX index)
{
    for (size_t pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aOptions[pos].opt == index)
            return m_aUpdateOpts[pos].pszComment;
    }
    return nullptr;
}

bool CSrcOptions::GetBoolOption(sfopt::OPT_INDEX index)
{
    for (size_t pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aOptions[pos].opt == index)
            return ttIsSameStrI(m_aUpdateOpts[pos].pszVal, "true");
    }
    return false;
}

bool CSrcOptions::GetChanged(sfopt::OPT_INDEX index)
{
    for (size_t pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aOptions[pos].opt == index)
        {
            if (!s_aOptions[pos].pszVal)
                return ttIsNonEmpty(m_aUpdateOpts[pos].pszVal);
            else
                return !ttIsSameStrI(m_aUpdateOpts[pos].pszVal, s_aOptions[pos].pszVal);
        }
    }
    return false;
}

bool CSrcOptions::GetRequired(sfopt::OPT_INDEX index)
{
    for (size_t pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aOptions[pos].opt == index)
            return m_aUpdateOpts[pos].bRequired;
    }
    return false;
}

void CSrcOptions::SetRequired(sfopt::OPT_INDEX index, bool bVal)
{
    for (size_t pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aOptions[pos].opt == index)
        {
            m_aUpdateOpts[pos].bRequired = bVal;
            break;
        }
    }
}

sfopt::OPT_INDEX CSrcOptions::UpdateReadOption(const char* pszName, const char* pszVal, const char* pszComment)
{
    ttASSERT_NONEMPTY(pszName);
    ttASSERT_MSG(pszVal, "NULL pointer!");

    size_t pos;
    for (pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (ttIsSameStrI(s_aOptions[pos].pszName, pszName))
            break;
    }
    if (s_aOptions[pos].opt == OPT_OVERFLOW)
        return OPT_OVERFLOW;  // unknown option

    // we call this so that "yes" or "no" options get converted to "true" or "false"
    UpdateOption(s_aOptions[pos].opt, pszVal);

    if (pszComment)
    {
        ttFree(m_aUpdateOpts[pos].pszComment);
        m_aUpdateOpts[pos].pszComment = ttStrDup(pszComment);
    }
    return s_aOptions[pos].opt;
}

const sfopt::OPT_VERSION* CSrcOptions::GetOptionMinVersion(sfopt::OPT_INDEX index)
{
    for (size_t pos = 0; aoptVersions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (aoptVersions[pos].opt == index)
            return (&aoptVersions[pos]);
    }
    return nullptr;
}
