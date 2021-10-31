/////////////////////////////////////////////////////////////////////////////
// Purpose:   Class for storing/retrieving options in a .srcfiles.yaml file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

/*
    Ideally, s_aInitialOptions would be in a namespace. However, the Visual Studio debugger (2017 version) is not
    able to display the array even if fully qualified with the namespace. That makes debugging rather difficult
    since a lot of the functionality of ttBld relies on comparison between a modified option and the original.
*/

#include <array>

#include "csrcfiles.h"

// Any time you add an option below, you need to increment this version number and then add it to the aoptVersions
// list
const char* txtOptVersion { "1.8.1" };

// clang-format off
static OPT::VERSION aoptVersions[]
{
    { OPT::CRT_REL, 1, 2, 0 },
    { OPT::CRT_DBG, 1, 2, 0 },
    { OPT::WARN, 1, 2, 0 },
    { OPT::OPTIMIZE, 1, 2, 0 },

    { OPT::TARGET_DIR, 1, 4, 0 },
    { OPT::TARGET_DIR32, 1, 4, 0 },
    { OPT::TARGET_DIR64, 1, 4, 0 },

    { OPT::MSVC_CMN, 1, 5, 0 },
    { OPT::MSVC_REL, 1, 5, 0 },
    { OPT::MSVC_DBG, 1, 5, 0 },

    { OPT::MAKE_DIR, 1, 5, 0 },

    { OPT::BUILD_LIBS32, 1, 7, 3 },

    // All options default to 1.0.0, so only add options above that require a newer version of ttBld

    { OPT::LAST, 1, 0, 0  }
};

// ID, name, default value, comment, bool type, required

// These are in the order they should be written in a new .srcfiles.yaml file.
const std::array<OPT::ORIGINAL, OPT::LAST + 1> DefaultOptions
{{

    { OPT::PROJECT,  "Project",  nullptr,   "project name", OPT::any, OPT::required },
    { OPT::EXE_TYPE, "Exe_type", "console", "[window | console | lib | dll | ocx]", OPT::any, OPT::required },
    { OPT::PCH,      "Pch",      "none",    "name of precompiled header file, or \"none\" if not using precompiled headers",
                                                                OPT::any, OPT::required },
    { OPT::PCH_CPP,  "Pch_cpp",  "none",    "source file used to build precompiled header (default uses same name as PCH option)",
                                                                OPT::any, OPT::optional },
    { OPT::OPTIMIZE, "Optimize", "space",  "[space | speed] optimization to use in release builds", OPT::any, OPT::required },
    { OPT::WARN,     "Warn",     "4",      "[1-4] warning level", OPT::any, OPT::required },

    { OPT::CRT_REL,  "Crt_rel",  "static", "[static | dll] type of CRT to link to in release builds", OPT::any, OPT::required },
    { OPT::CRT_DBG,  "Crt_dbg",  "static", "[static | dll] type of CRT to link to in debug builds", OPT::any, OPT::required },

    { OPT::TARGET_DIR, "TargetDir", nullptr, "target directory", OPT::any, OPT::optional },

    { OPT::BIT64, "64Bit", "true", "[true | false] indicates whether buildable as a 64-bit target", OPT::boolean, OPT::optional },
    { OPT::TARGET_DIR64, "TargetDir64",  nullptr, "64-bit target directory", OPT::any, OPT::optional },

    { OPT::BIT32, "32Bit", "false", "[true | false] indicates whether buildable as a 32-bit target", OPT::boolean, OPT::optional },
    { OPT::TARGET_DIR32, "TargetDir32", nullptr, "32-bit target directory", OPT::any, OPT::optional },

    { OPT::CFLAGS_CMN, "CFlags_cmn", nullptr, "common compiler flags", OPT::any, OPT::optional },
    { OPT::CFLAGS_REL, "CFlags_rel", nullptr, "release build compiler flags", OPT::any, OPT::optional },
    { OPT::CFLAGS_DBG, "CFlags_dbg", nullptr, "debug build compiler flags", OPT::any, OPT::optional },

    { OPT::CLANG_CMN, "Clang_cmn", nullptr, "clang common compiler flags", OPT::any, OPT::optional },
    { OPT::CLANG_REL, "Clang_rel", nullptr, "clang release build compiler flags", OPT::any, OPT::optional },
    { OPT::CLANG_DBG, "Clang_dbg", nullptr, "clang debug build compiler flags", OPT::any, OPT::optional },

    { OPT::MSVC_CMN, "msvc_cmn", nullptr, "msvc common compiler flags", OPT::any, OPT::optional },
    { OPT::MSVC_REL, "msvc_rel", nullptr, "msvc release build compiler flags", OPT::any, OPT::optional },
    { OPT::MSVC_DBG, "msvc_dbg", nullptr, "msvc debug build compiler flags", OPT::any, OPT::optional },

    { OPT::LINK_CMN, "LFlags_cmn", nullptr, "common linker flags", OPT::any, OPT::optional },
    { OPT::LINK_REL, "LFlags_rel", nullptr, "release build linker flags", OPT::any, OPT::optional },
    { OPT::LINK_DBG, "LFlags_dbg", nullptr, "debug build linker flags", OPT::any, OPT::optional },

    { OPT::RC_CMN, "Rc_cmn", nullptr, "common compiler flags", OPT::any, OPT::optional },
    { OPT::RC_REL, "Rc_rel", nullptr, "release build compiler flags", OPT::any, OPT::optional },
    { OPT::RC_DBG, "Rc_dbg", nullptr, "debug build compiler flags", OPT::any, OPT::optional },

    { OPT::MIDL_CMN, "Midl_cmn", nullptr, "common midl flags", OPT::any, OPT::optional },
    { OPT::MIDL_REL, "Midl_rel", nullptr, "release build midl flags", OPT::any, OPT::optional },
    { OPT::MIDL_DBG, "Midl_dbg", nullptr, "debug build midl flags", OPT::any, OPT::optional },

    { OPT::NATVIS, "Natvis", nullptr, "Debug visualizer", OPT::any, OPT::optional },

    { OPT::MS_LINKER, "Ms_linker", "true", "true means use link.exe even when compiling with clang", OPT::boolean, OPT::optional },
    { OPT::MS_RC,     "Ms_rc",     "true",  "true means use rc.exe even when compiling with clang", OPT::boolean, OPT::optional },

    { OPT::INC_DIRS,   "IncDirs",   nullptr, "additional directories for header files", OPT::any, OPT::optional },
    { OPT::LIB_DIRS,   "LibDirs",   nullptr, "additional directories for library files", OPT::any, OPT::optional },
    { OPT::LIB_DIRS64, "LibDirs64", nullptr, "additional directories for 64-bit library files", OPT::any, OPT::optional },
    { OPT::LIB_DIRS32, "LibDirs32", nullptr, "additional directories for 32-bit library files", OPT::any, OPT::optional },

    { OPT::BUILD_LIBS, "BuildLibs", nullptr, "libraries to build (built in makefile)", OPT::any, OPT::optional },
    { OPT::BUILD_LIBS32, "BuildLibs32", nullptr, "32-bit libraries to build (built in makefile)", OPT::any, OPT::optional },

    { OPT::LIBS_CMN, "Libs_cmn", nullptr, "additional libraries to link to in all builds", OPT::any, OPT::optional },
    { OPT::LIBS_REL, "Libs_rel", nullptr, "additional libraries to link to in release builds", OPT::any, OPT::optional },
    { OPT::LIBS_DBG, "Libs_dbg", nullptr, "additional libraries to link to in debug builds", OPT::any, OPT::optional },

    { OPT::MAKE_DIR, "MakeDir", nullptr, "auto-generate makefile in specified directory", OPT::any, OPT::optional },

    // The following options are for xgettext/msgfmt support

    { OPT::XGET_OUT,      "XGet_out",     nullptr, "output filename for xgettext", OPT::any, OPT::optional },
    { OPT::XGET_KEYWORDS, "XGet_kwrds",   nullptr, "xgettext keywords (separated by semi-colon)", OPT::any, OPT::optional },
    { OPT::XGET_FLAGS,    "XGet_flags",   nullptr, "xgettext flags", OPT::any, OPT::optional },
    { OPT::MSGFMT_FLAGS,  "Msgfmt_flags", nullptr, "msgfmt flags", OPT::any, OPT::optional },
    { OPT::MSGFMT_XML,    "Msgfmt_xml",   nullptr, "xml template file for msgfmt", OPT::any, OPT::optional },

    { OPT::LAST, "", nullptr, nullptr, false, false}

}};
// clang-format on

void CSrcFiles::InitOptions()
{
    // Calling this twice will wipe out any options changed in between calls.
    ASSERT(!m_Initialized);
    if (m_Initialized)
        return;
    m_Initialized = true;

    for (const auto& original: DefaultOptions)
    {
        auto& option = m_Options[original.optionID];
        if (original.optionID == OPT::LAST)
            break;

        option.OriginalName = original.name;
        option.OriginalValue = original.value;
        if (option.OriginalValue && *option.OriginalValue)
            option.value = option.OriginalValue;
        option.OriginalComment = original.comment;
        if (option.OriginalComment && *option.OriginalComment)
            option.comment = option.OriginalComment;

        option.isBooleanValue = original.isBooleanValue;
        option.isRequired = original.isRequired;
    }

    // We special-case PCH and PCH_CPP to replace the default "none" with an empty string
    // BUGBUG: [KeyWorks - 03-21-2020] OPT::PCH is a required option, so clearing it means it gets written
    // out as an empty string instead of "none"
    // m_Options[OPT::PCH].value.clear();
    m_Options[OPT::PCH_CPP].value.clear();

#if !defined(NDEBUG)
    m_Options[OPT::LAST].OriginalName = "Don't Use this id!";
    // The options are all indexed by the enumerated id, so it is imperative that each option appears in DefaultOptions.
    for (size_t id = 0; id < m_Options.size(); ++id)
    {
        ASSERT_MSG(m_Options[id].OriginalName, "Option is missing a name! It means DefaultOptions is missing an option.");
    }
#endif
}

OPT::value CSrcFiles::FindOption(const std::string_view name) const
{
    assert(!name.empty());
    if (name.empty())
        return OPT::LAST;

    for (const auto& option: optIterator())
    {
        if (ttlib::is_sameas(name, m_Options[option].OriginalName, tt::CASE::either))
            return option;
    }
    return OPT::LAST;
}

void CSrcFiles::setOptValue(OPT::value index, std::string_view value)
{
    assert(index < OPT::LAST);

    if (!m_Options[index].isBooleanValue)
        m_Options[index].value = value;
    else
    {
        if (ttlib::is_sameprefix(value, "yes", tt::CASE::either) || ttlib::is_sameprefix(value, "true", tt::CASE::either))
            m_Options[index].value = "true";
        else
            m_Options[index].value = "false";
    }
}

void CSrcFiles::setBoolOptValue(OPT::value index, bool value)
{
    assert(index < OPT::LAST);
    assert(m_Options[index].isBooleanValue);

    if (value)
        m_Options[index].value = "true";
    else
        m_Options[index].value = "false";
}
