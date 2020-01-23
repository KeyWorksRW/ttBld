/////////////////////////////////////////////////////////////////////////////
// Name:      CSrcOptions
// Purpose:   Class for storing/retrieving options in a .srcfiles.yaml file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

/*
    Ideally, s_aInitialOptions would be in a namespace. However, the Visual Studio debugger (2017 version) is not
   able to display the array even if fully qualified with the namespace. That makes debugging rather difficult
   since a lot of the functionality of ttBld relies on comparison between a modified option and the original.
*/

#include "pch.h"

#include <array>

#include <ttTR.h>  // Function for translating strings

#include "options.h"

using namespace sfopt;

const char* txtOptVersion = "1.4.0";  // any time you add an option below, you need to increment this version
                                      // number and then add it to the OPT_VERSION list

// clang-format off
OPT_VERSION aoptVersions[] =
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

// clang-format off

// These are in the order they should be written in a new .srcfiles.yaml file.
static const std::array<Opt::ORIGINAL_OPTIONS, Opt::LAST> DefaultOptions =
{{

    { Opt::PROJECT,  "Project",  nullptr,   "project name", false, true },
    { Opt::EXE_TYPE, "Exe_type", "console", "[window | console | lib | dll]", false, true },
    { Opt::PCH,      "Pch",      "none",    "name of precompiled header file, or \"none\" if not using precompiled"
                                             "headers", false, true },
    { Opt::PCH_CPP,  "Pch_cpp",  "none",    "source file used to build precompiled header (default uses same name"                                          "as PCH option)", false, false },
    { Opt::OPTIMIZE, "Optimize", "space",  "[space | speed] optimization to use in release builds", false, true },
    { Opt::WARN,     "Warn",     "4",      "[1-4] warning level", false, true },

    { Opt::CRT_REL,  "Crt_rel",  "static", "[static | dll] type of CRT to link to in release builds", false, true },
    { Opt::CRT_DBG,  "Crt_dbg",  "static", "[static | dll] type of CRT to link to in debug builds", false, true },

    { Opt::TARGET_DIR, "TargetDir", nullptr, "target directory", false, false },

    { Opt::BIT64, "64Bit", "true", "[true | false] indicates whether buildable as a 64-bit target", true, false },
    { Opt::TARGET_DIR64, "TargetDir64",  nullptr, "64-bit target directory", false, false },

    { Opt::BIT32, "32Bit", "true", "[true | false] indicates whether buildable as a 32-bit target", true, false },
    { Opt::TARGET_DIR32, "TargetDir32", nullptr, "32-bit target directory", false, false },

    { Opt::CFLAGS_CMN, "CFlags_cmn", nullptr, "common compiler flags", false, false },
    { Opt::CFLAGS_REL, "CFlags_rel", nullptr, "release build compiler flags", false, false },
    { Opt::CFLAGS_DBG, "CFlags_dbg", nullptr, "debug build compiler flags", false, false },

    { Opt::CLANG_CMN, "Clang_cmn", nullptr, "clang common compiler flags", false, false },
    { Opt::CLANG_REL, "Clang_rel", nullptr, "clang release build compiler flags", false, false },
    { Opt::CLANG_DBG, "Clang_dbg", nullptr, "clang debug build compiler flags", false, false },

    { Opt::LINK_CMN, "LFlags_cmn", nullptr, "common linker flags", false, false },
    { Opt::LINK_REL, "LFlags_rel", nullptr, "release build linker flags", false, false },
    { Opt::LINK_DBG, "LFlags_dbg", nullptr, "debug build linker flags", false, false },

    { Opt::RC_CMN, "Rc_cmn", nullptr, "common compiler flags", false, false },
    { Opt::RC_REL, "Rc_rel", nullptr, "release build compiler flags", false, false },
    { Opt::RC_DBG, "Rc_dbg", nullptr, "debug build compiler flags", false, false },

    { Opt::MIDL_CMN, "Midl_cmn", nullptr, "common compiler flags", false, false },
    { Opt::MIDL_REL, "Midl_rel", nullptr, "release build compiler flags", false, false },
    { Opt::MIDL_DBG, "Midl_dbg", nullptr, "debug build compiler flags", false, false },


    { Opt::NATVIS, "Natvis", nullptr, "Debug visualizer", false, false },

    { Opt::MS_LINKER, "Ms_linker", "false", "true means use link.exe even when compiling with clang", true, false },
    { Opt::MS_RC,     "Ms_rc",     "true",  "true means use rc.exe even when compiling with clang", true, false },

    { Opt::INC_DIRS,   "IncDirs",   nullptr, "additional directories for header files", false, false },
    { Opt::LIB_DIRS,   "LibDirs",   nullptr, "additional directories for library files", false, false },
    { Opt::LIB_DIRS64, "LibDirs64", nullptr, "additional directories for 64-bit library files", false, false },
    { Opt::LIB_DIRS32, "LibDirs32", nullptr, "additional directories for 32-bit library files", false, false },

    { Opt::BUILD_LIBS, "BuildLibs", nullptr, "libraries to build (built in makefile)", false, false },

    { Opt::LIBS_CMN, "Libs_cmn", nullptr, "additional libraries to link to in all builds", false, false },
    { Opt::LIBS_REL, "Libs_rel", nullptr, "additional libraries to link to in release builds", false, false },
    { Opt::LIBS_DBG, "Libs_dbg", nullptr, "additional libraries to link to in debug builds", false, false },

    // The following options are for xgettext/msgfmt support

    { Opt::XGET_OUT,      "XGet_out",     nullptr, "output filename for xgettext", false, false },
    { Opt::XGET_KEYWORDS, "XGet_kwrds",   nullptr, "xgettext keywords (separated by semi-colon)", false, false },
    { Opt::XGET_FLAGS,    "XGet_flags",   nullptr, "xgettext flags", false, false },
    { Opt::MSGFMT_FLAGS,  "Msgfmt_flags", nullptr, "msgfmt flags", false, false },
    { Opt::MSGFMT_XML,    "Msgfmt_xml",   nullptr, "xml template file for msgfmt", false, false },

    { Opt::LAST, nullptr, nullptr, nullptr, false, false}

}};

// clang-format off
static const OPT_SETTING s_aInitialOptions[] =
{
    // { OPT_xxx,       "name",         "value",   boolean, required, "comment" }

    { OPT_PROJECT,      "Project",      nullptr,   false,   true,      ttTR("project name") },
    { OPT_EXE_TYPE,     "Exe_type",     "console", false,   true,     "[window | console | lib | dll]" },
    { OPT_PCH,          "Pch",          "none",    false,   true,      ttTR("name of precompiled header file, or \042none\042 if not using precompiled headers") },
    { OPT_PCH_CPP,      "Pch_cpp",      "none",    false,   false,     ttTR("source file used to build precompiled header (default uses same name as PCH option)") },

    { OPT_OPTIMIZE,     "Optimize",     "space",   false,   true,      ttTR("[space | speed] optimization to use in release builds") },
    { OPT_WARN_LEVEL,   "Warn",         "4",       false,   true,      ttTR("[1-4] warning level") },

    { OPT_STATIC_CRT_REL, "Crt_rel",    "static",  false,   true,      ttTR("[static | dll] type of CRT to link to in release builds") },
    { OPT_STATIC_CRT_DBG, "Crt_dbg",    "static",  false,   true,      ttTR("[static | dll] type of CRT to link to in debug builds") },

    { OPT_TARGET_DIR, "TargetDir",      nullptr,   false,   false,     ttTR("target directory") },

    { OPT_64BIT,        "64Bit",        "true",    true,    false,      ttTR("[true | false] indicates if project can be built as a 64-bit target") },
    { OPT_TARGET_DIR64, "TargetDir64",  nullptr,   false,   false,     ttTR("64-bit target directory") },

    { OPT_32BIT,        "32Bit",        "false",   true,    false,      ttTR("[true | false] indicates if project can be built as a 32-bit target") },
    { OPT_TARGET_DIR32, "TargetDir32",  nullptr,   false,   false,     ttTR("32-bit target directory") },

    { OPT_PERMISSIVE,   "Permissive",   "false",   true,    false,     ttTR("true means add -permissive- compiler flag") },
    { OPT_STDCALL,      "Stdcall",      "false",   true,    false,     ttTR("true to use stdcall calling convention, false for cdecl (default)") },

    { OPT_CFLAGS_CMN,   "CFlags_cmn",   nullptr,   false,   false,     ttTR("flags to pass to the compiler in all builds") },
    { OPT_CFLAGS_REL,   "CFlags_rel",   nullptr,   false,   false,     ttTR("flags to pass to the compiler in release builds") },
    { OPT_CFLAGS_DBG,   "CFlags_dbg",   nullptr,   false,   false,     ttTR("flags to pass to the compiler in debug builds") },

    { OPT_CLANG_CMN,    "Clang_cmn",    nullptr,   false,   false,     ttTR("flags to pass only to the clang compiler in all builds") },
    { OPT_CLANG_REL,    "Clang_rel",    nullptr,   false,   false,     ttTR("flags to pass only to the clang compiler in release builds") },
    { OPT_CLANG_DBG,    "Clang_dbg",    nullptr,   false,   false,     ttTR("flags to pass only to the clang compiler in debug builds") },

    { OPT_LINK_CMN,     "LFlags_cmn",   nullptr,   false,   false,     ttTR("flags to pass to the linker in all builds") },
    { OPT_LINK_REL,     "LFlags_rel",   nullptr,   false,   false,     ttTR("flags to pass to the linker in release builds") },
    { OPT_LINK_DBG,     "LFlags_dbg",   nullptr,   false,   false,     ttTR("flags to pass to the linker in debug builds") },

    { OPT_NATVIS,       "Natvis",       nullptr,   false,   false,     ttTR("MSVC Debug visualizer") },

    { OPT_RC_CMN,       "Rc_cmn",       nullptr,   false,   false,     ttTR("flags to pass to the resource compiler in all builds") },
    { OPT_RC_REL,       "Rc_rel",       nullptr,   false,   false,     ttTR("flags to pass to the resource compiler in release builds") },
    { OPT_RC_DBG,       "Rc_dbg",       nullptr,   false,   false,     ttTR("flags to pass to the resource compiler in debug builds") },

    { OPT_MDL_CMN,      "Midl_cmn",     nullptr,   false,   false,     ttTR("flags to pass to the midl compiler in all builds") },
    { OPT_MDL_REL,      "Midl_rel",     nullptr,   false,   false,     ttTR("flags to pass to the midl compiler in release builds") },
    { OPT_MDL_DBG,      "Midl_dbg",     nullptr,   false,   false,     ttTR("flags to pass to the midl compiler in debug builds") },

    { OPT_DEBUG_RC,     "DebugRC",      "false",   true,    false,     ttTR("true means build a -D_DEBUG version of the project's rc file") },

    { OPT_MS_LINKER,    "Ms_linker",    "true",    true,    false,     ttTR("true means use link.exe even when compiling with clang") },
    { OPT_MS_RC,        "Ms_rc",        "true",    true,    false,     ttTR("use rc.exe even when compiling with CLANG-CL") },

    { OPT_INC_DIRS,     "IncDirs",      nullptr,   false,   false,     ttTR("additional directories for header files") },
    { OPT_BUILD_LIBS,   "BuildLibs",    nullptr,   false,   false,     ttTR("libraries that need to be built (added to makefile generation)") },

    { OPT_LIB_DIRS,     "LibDirs",      nullptr,   false,   false,     ttTR("additional directories for library files") },
    { OPT_LIB_DIRS64,   "LibDirs64",    nullptr,   false,   false,     ttTR("additional directories for 64-bit library files") },
    { OPT_LIB_DIRS32,   "LibDirs32",    nullptr,   false,   false,     ttTR("additional directories for 32-bit library files") },

    { OPT_LIBS_CMN,     "Libs_cmn",     nullptr,   false,   false,     ttTR("additional libraries to link to in all builds") },
    { OPT_LIBS_REL,     "Libs_rel",     nullptr,   false,   false,     ttTR("additional libraries to link to in release builds") },
    { OPT_LIBS_DBG,     "Libs_dbg",     nullptr,   false,   false,     ttTR("additional libraries to link to in debug builds") },

    // The following options are for xgettext/msgfmt support

    { OPT_XGET_OUT,      "XGet_out",     nullptr, false, false, ttTR("output filename for xgettext") },
    { OPT_XGET_KEYWORDS, "XGet_kwrds",   nullptr, false, false, ttTR("additional keywords (separated by semi-colon) to pass to xgettext") },
    { OPT_XGET_FLAGS,    "XGet_flags",   nullptr, false, false, ttTR("additional flags to pass to xgettext") },
    { OPT_MSGFMT_FLAGS,  "Msgfmt_flags", nullptr, false, false, ttTR("additional flags to pass to msgfmt") },
    { OPT_MSGFMT_XML,    "Msgfmt_xml",   nullptr, false, false, ttTR("the name of the xml template file for msgfmt to use") },

//    { OPT_LIB_DIRS,     "LibDirs",      nullptr,   false,   false,     ttTR("additional directores for lib files") },
//    { OPT_LIBS,         "Libs",         nullptr,   false,   false,     ttTR("additional libraries to link to (see OPT_BUILD_LIBS to both build and link to a library)") },
//    { OPT_LIBS_REL,     "LibsR",        nullptr,   false,   false,     ttTR("additional libraries to link to (see OPT_BUILD_LIBS to both build and link to a library)") },
//    { OPT_LIBS_DBG,     "LibsD",        nullptr,   false,   false,     ttTR("additional libraries to link to (see OPT_BUILD_LIBS to both build and link to a library)") },

    { OPT_OVERFLOW, "", "", false, false, "" },
};
// clang-format on

const OPT_SETTING* CSrcOptions::GetOrgOptions()
{
    return s_aInitialOptions;
}

CSrcOptions::CSrcOptions()
{
    // By adding OPT_UPDATE structures in the same order as OPT_SETTING, a position in m_aUpdateOpts matches the
    // same position in s_aInitialOptions

    for (size_t pos = 0; s_aInitialOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        size_t posOpt = m_aUpdateOpts.Add();
        m_aUpdateOpts[posOpt].pszVal =
            s_aInitialOptions[pos].pszVal ? ttStrDup(s_aInitialOptions[pos].pszVal) : nullptr;
        m_aUpdateOpts[posOpt].pszComment = ttIsNonEmpty(s_aInitialOptions[pos].pszComment) ?
                                               ttStrDup(s_aInitialOptions[pos].pszComment) :
                                               nullptr;
        m_aUpdateOpts[posOpt].bRequired = s_aInitialOptions[pos].bRequired;
    }
}

CSrcOptions::~CSrcOptions()
{
    for (size_t pos = 0; s_aInitialOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        ttFree(m_aUpdateOpts[pos].pszVal);
        ttFree(m_aUpdateOpts[pos].pszComment);
    }
}

// fine to call this for boolean options if pszVal == "true/false" or "yes/no"
sfopt::OPT_INDEX CSrcOptions::UpdateOption(sfopt::OPT_INDEX index, const char* pszVal)
{
    size_t pos;
    for (pos = 0; s_aInitialOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aInitialOptions[pos].opt == index)
            break;
    }
    ttASSERT(s_aInitialOptions[pos].opt != OPT_OVERFLOW);
    if (s_aInitialOptions[pos].opt == OPT_OVERFLOW)
        return OPT_OVERFLOW;  // invalid option
    ttFree(m_aUpdateOpts[pos].pszVal);

    if (!pszVal)
    {
        m_aUpdateOpts[pos].pszVal = nullptr;
        return s_aInitialOptions[pos].opt;
    }

    // We want boolean values to be consistent, so if "yes" or "no" is specified, convert to "true" or "false"

    if (ttIsSameStrI(pszVal, "yes"))
        pszVal = "true";
    else if (ttIsSameStrI(pszVal, "no"))
        pszVal = "false";

    m_aUpdateOpts[pos].pszVal = ttStrDup(pszVal);
    return s_aInitialOptions[pos].opt;
}

sfopt::OPT_INDEX CSrcOptions::UpdateOption(sfopt::OPT_INDEX index, bool bVal)
{
    size_t pos;
    for (pos = 0; s_aInitialOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aInitialOptions[pos].opt == index)
            break;
    }
    ttASSERT(s_aInitialOptions[pos].opt != OPT_OVERFLOW);
    if (s_aInitialOptions[pos].opt == OPT_OVERFLOW)
        return OPT_OVERFLOW;  // invalid option

    ttFree(m_aUpdateOpts[pos].pszVal);
    m_aUpdateOpts[pos].pszVal = ttStrDup(bVal ? "true" : "false");
    return s_aInitialOptions[pos].opt;
}

const char* CSrcOptions::GetOption(sfopt::OPT_INDEX index)
{
    const char* pszNewOption = nullptr;
    for (auto where = 0U; s_aInitialOptions[where].opt != OPT_OVERFLOW; where++)
    {
        if (s_aInitialOptions[where].opt == index)
        {
            auto& opt = FindOption(s_aInitialOptions[where].pszName);
            if (opt.optionID != Opt::LAST)
            {
                pszNewOption = opt.value.c_str();
            }
            break;
        }
    }

    for (size_t pos = 0; s_aInitialOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aInitialOptions[pos].opt == index)
        {
#if !defined(NDEBUG)
            const char* pszOldOption = m_aUpdateOpts[pos].pszVal ? m_aUpdateOpts[pos].pszVal : "";
            ttASSERT(tt::issamestr(pszOldOption, pszNewOption));
#endif  // NDEBUG

            return m_aUpdateOpts[pos].pszVal;
        }
    }
    return nullptr;
}

const char* CSrcOptions::GetOptComment(sfopt::OPT_INDEX index)
{
    for (size_t pos = 0; s_aInitialOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aInitialOptions[pos].opt == index)
            return m_aUpdateOpts[pos].pszComment;
    }
    return nullptr;
}

bool CSrcOptions::GetBoolOption(sfopt::OPT_INDEX index)
{
    for (size_t pos = 0; s_aInitialOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aInitialOptions[pos].opt == index)
            return ttIsSameStrI(m_aUpdateOpts[pos].pszVal, "true");
    }
    return false;
}

bool CSrcOptions::GetChanged(sfopt::OPT_INDEX index)
{
    for (size_t pos = 0; s_aInitialOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aInitialOptions[pos].opt == index)
        {
            if (!s_aInitialOptions[pos].pszVal)
                return ttIsNonEmpty(m_aUpdateOpts[pos].pszVal);
            else
                return !ttIsSameStrI(m_aUpdateOpts[pos].pszVal, s_aInitialOptions[pos].pszVal);
        }
    }
    return false;
}

bool CSrcOptions::GetRequired(sfopt::OPT_INDEX index)
{
    for (size_t pos = 0; s_aInitialOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aInitialOptions[pos].opt == index)
            return m_aUpdateOpts[pos].bRequired;
    }
    return false;
}

void CSrcOptions::SetRequired(sfopt::OPT_INDEX index, bool bVal)
{
    for (size_t pos = 0; s_aInitialOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (s_aInitialOptions[pos].opt == index)
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
    for (pos = 0; s_aInitialOptions[pos].opt != OPT_OVERFLOW; ++pos)
    {
        if (ttIsSameStrI(s_aInitialOptions[pos].pszName, pszName))
            break;
    }
    if (s_aInitialOptions[pos].opt == OPT_OVERFLOW)
        return OPT_OVERFLOW;  // unknown option

    // we call this so that "yes" or "no" options get converted to "true" or "false"
    UpdateOption(s_aInitialOptions[pos].opt, pszVal);

    if (pszComment)
    {
        ttFree(m_aUpdateOpts[pos].pszComment);
        m_aUpdateOpts[pos].pszComment = ttStrDup(pszComment);
    }
    return s_aInitialOptions[pos].opt;
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

void CSrcOptions::InitOptions()
{
    // Note that we always add a final Opt::LAST entry

    for (size_t option = 0; option < Opt::LAST; ++option)
    {
        auto& opt = FindOriginal(option);
        if (opt.optionID != Opt::LAST)
        {
            auto& update = m_opt.m_Options.emplace_back();
            update.optionID = opt.optionID;
            update.OriginalName = opt.name;
            update.OriginalValue = opt.value;
            update.OriginalComment = opt.comment;
            update.BooleanValue = opt.BooleanValue;
            update.Required = opt.Required;

            if (opt.value)
                update.value = opt.value;
            if (opt.comment)
                update.comment = opt.comment;
        }
    }
    auto last = m_opt.m_Options.emplace_back();
    last.optionID = Opt::LAST;
}

const Opt::ORIGINAL_OPTIONS& CSrcOptions::FindOriginal(size_t option)
{
    ttASSERT(option < Opt::LAST);

    if (option >= Opt::LAST)
    {
        return DefaultOptions.back();
    }

    for (size_t pos = 0; pos < DefaultOptions.size(); ++pos)
    {
        if (DefaultOptions[pos].optionID == option)
            return DefaultOptions[pos];
    }
    return DefaultOptions.back();
}

Opt::OPTION& CSrcOptions::FindOption(const std::string& name)
{
    ttASSERT(!name.empty());
    if (name.empty())
        return m_opt.m_Options.back();

    for (size_t pos = 0; pos < m_opt.m_Options.size(); ++pos)
    {
        if (m_opt.m_Options[pos].optionID == Opt::LAST)
            return m_opt.m_Options[pos];
        else if (tt::issamestri(name, m_opt.m_Options[pos].OriginalName))
            return m_opt.m_Options[pos];
    }
    return m_opt.m_Options.back();
}

void Opt::setOptValue(size_t index, std::string_view value)
{
    ttASSERT(index < LAST);

    if (!m_Options.at(index).BooleanValue)
        m_Options.at(index).value = value;
    else
    {
        if (tt::issamestri(value, "yes") || tt::issamestri(value, "true"))
            m_Options.at(index).value = "true";
        else
            m_Options.at(index).value = "false";
    }
}
