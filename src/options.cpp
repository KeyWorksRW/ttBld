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
    the functionality of MakeNinja relies on comparison between a modified option and the original.
*/

#include "pch.h"

#include "options.h"

using namespace sfopt;

const char* txtOptVersion = "1.0.0"; // any time you add an option below, you need to increment this version number and then add it to the OPT_VERSION list

sfopt::OPT_VERSION aoptVersions[] =
{

    // All options default to 1.0.0, so only add options above that require a newer version of MakeNinja

    { OPT_OVERFLOW, 1, 0, 0  }
};

// Add these in the order you want them written in a new .srcfiles.yaml files.

static const OPT_SETTING s_aOptions[] =
{
    // { OPT_xxx,       "name",         "value",   boolean, required, "comment" }

    { OPT_PROJECT,      "Project",      nullptr,   false,   true,     _XGET("project name") },
    { OPT_EXE_TYPE,     "exe_type",     "console", false,   true,     "[window | console | lib | dll]" },
    { OPT_PCH,          "PCH",          "none",    false,   true,     _XGET("name of precompiled header file, or \042none\042 if not using precompiled headers") },
    { OPT_PCH_CPP,      "PCH_CPP",      "none",    false,   false,    _XGET("source file used to build precompiled header (default uses same name as PCH option)") },

    { OPT_OPTIMIZE,     "optimize",     "space",   false,   false,     _XGET("[space | speed] optimization") },
    { OPT_WARN_LEVEL,   "WarnLevel",    "4",       false,   false,     _XGET("[1-4]") },
    { OPT_PERMISSIVE,   "permissive",   "false",   true,    false,     _XGET("true means add -permissive- compiler flag") },
    { OPT_STDCALL,      "stdcall",      "false",   true,    false,     _XGET("true to use stdcall calling convention, false for cdecl (default)") },

    { OPT_MAKEFILE,     "Makefile",     "missing", false,   false,     "[never | missing | always]" },
    { OPT_COMPILERS,    "Compilers",    nullptr,   false,   false,     _XGET("[MSVC or CLANG] default is both, set this option to limit it to one") },

    { OPT_CFLAGS_CMN,   "CFlags",       nullptr,   false,   false,     _XGET("flags to pass to the compiler in all build targets") },
    { OPT_CFLAGS_REL,   "CFlagsR",      nullptr,   false,   false,     _XGET("flags to pass to the compiler in release builds") },
    { OPT_CFLAGS_DBG,   "CFlagsD",      nullptr,   false,   false,     _XGET("flags to pass to the compiler in debug builds") },

//    { OPT_STATIC_CRT_REL, "CrtRel",   "static",  false,    false,     _XGET("[static | dll] type of CRT to link to in release builds") },
//    { OPT_STATIC_CRT_DBG, "CrtDbg",   "static",  false,    false,     _XGET("[static | dll] type of CRT to link to in debug builds") },

    { OPT_STATIC_CRT_REL, "CrtStaticRel", "true",  true,    false,     _XGET("true means link to static CRT in release builds (default)") },
    { OPT_STATIC_CRT_DBG, "CrtStaticDbg", "true",  true,    false,     _XGET("true means link to static CRT in debug builds (default)") },

    { OPT_LINK_CMN,     "LFlags",       nullptr,   false,   false,     _XGET("flags to pass to the linker in all build targets") },
    { OPT_LINK_REL,     "LFlagsR",      nullptr,   false,   false,     _XGET("flags to pass to the linker in release builds") },
    { OPT_LINK_DBG,     "LFlagsD",      nullptr,   false,   false,     _XGET("flags to pass to the linker in debug builds") },

    { OPT_RC_CMN,       "RC_CMN",       nullptr,   false,   false,     _XGET("flags to pass to the resource compiler in all build targets") },
    { OPT_RC_REL,       "RC_REL",       nullptr,   false,   false,     _XGET("flags to pass to the resource compiler in release builds") },
    { OPT_RC_DBG,       "RC_DBG",       nullptr,   false,   false,     _XGET("flags to pass to the resource compiler in debug builds") },

    { OPT_MDL_CMN,      "MIDL_CMN",     nullptr,   false,   false,     _XGET("flags to pass to the midl compiler in all build targets") },
    { OPT_MDL_REL,      "MIDL_REL",     nullptr,   false,   false,     _XGET("flags to pass to the midl compiler in release builds") },
    { OPT_MDL_DBG,      "MIDL_DBG",     nullptr,   false,   false,     _XGET("flags to pass to the midl compiler in debug builds") },

    { OPT_CLANG_CMN,    "CLANG_CMN",    nullptr,   false,   false,     _XGET("flags to pass to the CLANG compiler in all build targets") },
    { OPT_CLANG_REL,    "CLANG_REL",    nullptr,   false,   false,     _XGET("flags to pass to the CLANG compiler in release builds") },
    { OPT_CLANG_DBG,    "CLANG_DBG",    nullptr,   false,   false,     _XGET("flags to pass to the CLANG compiler in debug builds") },

    { OPT_DEBUG_RC,     "DebugRC",      "false",   true,    false,     _XGET("true means build a -D_DEBUG version of the project's rc file") },

    { OPT_MS_LINKER,    "ms_linker",    "false",   true,    false,     _XGET("true means use link.exe even when compiling with CLANG") },
    { OPT_MS_RC,        "ms_rc",        "true",    true,    false,     _XGET("use rc.exe even when compiling with CLANG") },

    { OPT_64BIT,        "64Bit",        "false",   true,    false,     _XGET("generate scripts for 64-bit build") },
    { OPT_TARGET_DIR64, "TargetDir64",  nullptr,   false,   false,     _XGET("64-bit target directory") },

    { OPT_32BIT,        "32Bit",        "false",   true,    false,     _XGET("generate scripts for 32-bit build") },
    { OPT_TARGET_DIR32, "TargetDir32",  nullptr,   false,   false,     _XGET("32-bit target directory") },

    { OPT_64BIT_SUFFIX, "b64_suffix",   "false",   true,    false,     _XGET("true means append '64' to target's directory or .exe name") },
    { OPT_32BIT_SUFFIX, "b32_suffix",   "false",   true,    false,     _XGET("true means append '32' to target's directory or .exe name") },

    { OPT_INC_DIRS,     "IncDirs",      nullptr,   false,   false,     _XGET("additional directories for header files") },
    { OPT_BUILD_LIBS,   "BuildLibs",    nullptr,   false,   false,     _XGET("libraries that need to be built (added to makefile generation)") },
    { OPT_LIB_DIRS,     "LibDirs",      nullptr,   false,   false,     _XGET("additional directores for lib files") },

    { OPT_LIBS,         "Libs",         nullptr,   false,   false,     _XGET("additional libraries to link to (see OPT_BUILD_LIBS to both build and link to a library)") },
    { OPT_LIBS_REL,     "LibsR",        nullptr,   false,   false,     _XGET("additional libraries to link to (see OPT_BUILD_LIBS to both build and link to a library)") },
    { OPT_LIBS_DBG,     "LibsD",        nullptr,   false,   false,     _XGET("additional libraries to link to (see OPT_BUILD_LIBS to both build and link to a library)") },

    { OPT_NATVIS,       "Natvis",       nullptr,   false,   false,     _XGET("MSVC Debug visualizer") },
    { OPT_XGET_FLAGS,   "xget_flags",   nullptr,   false,   false,     _XGET("flags to pass to xgettext.exe") },

    { OPT_OVERFLOW, "", "", false, false, "" },
};

const OPT_SETTING* CSrcOptions::GetOrgOptions()
{
    return s_aOptions;
}

CSrcOptions::CSrcOptions()
{
    // By adding OPT_UPDATE structures in the same order as OPT_SETTING, a position in m_aUpdateOpts matches the same position in s_aOptions

    for (size_t pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        size_t posOpt = m_aUpdateOpts.Add();
        m_aUpdateOpts[posOpt].pszVal = s_aOptions[pos].pszVal ? ttStrDup(s_aOptions[pos].pszVal) : nullptr;
        m_aUpdateOpts[posOpt].pszComment = ttIsNonEmpty(s_aOptions[pos].pszComment) ? ttStrDup(s_aOptions[pos].pszComment) : nullptr;
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

sfopt::OPT_INDEX CSrcOptions::UpdateOption(sfopt::OPT_INDEX index, const char* pszVal)  // fine to call this for boolean options if pszVal == "true/false" or "yes/no"
{
//    ttASSERT_MSG(pszVal, "NULL pointer!");
    if (!pszVal)
        return OPT_OVERFLOW;

    size_t pos;
    for (pos = 0; s_aOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aOptions[pos].opt == index)
            break;
    }
    ttASSERT(s_aOptions[pos].opt != OPT_OVERFLOW);
    if (s_aOptions[pos].opt == OPT_OVERFLOW)
        return OPT_OVERFLOW;   // invalid option
    ttFree(m_aUpdateOpts[pos].pszVal);

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
        return OPT_OVERFLOW;   // invalid option

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
        return OPT_OVERFLOW;   // unknown option

    UpdateOption(s_aOptions[pos].opt, pszVal);  // we call this so that "yes" or "no" options get converted to "true" or "false"

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
