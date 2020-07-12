/////////////////////////////////////////////////////////////////////////////
// Name:      options.cpp
// Purpose:   Class for storing/retrieving options in a .srcfiles.yaml file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

/*
    Ideally, s_aInitialOptions would be in a namespace. However, the Visual Studio debugger (2017 version) is not
   able to display the array even if fully qualified with the namespace. That makes debugging rather difficult
   since a lot of the functionality of ttBld relies on comparison between a modified option and the original.
*/

#include "pch.h"

#include <array>

#include "csrcfiles.h"

// Any time you add an option below, you need to increment this version number and then add it to the aoptVersions
// list
const char* txtOptVersion { "1.4.0" };

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

    { OPT::MAKE_DIR, 1, 5, 0 },

    // All options default to 1.0.0, so only add options above that require a newer version of ttBld

    { OPT::LAST, 1, 0, 0  }
};

// ID, name, default value, comment, bool type, required

// These are in the order they should be written in a new .srcfiles.yaml file.
const std::array<OPT::ORIGINAL, OPT::LAST + 1> DefaultOptions
{{

    { OPT::PROJECT,  "Project",  nullptr,   "project name", false, true },
    { OPT::EXE_TYPE, "Exe_type", "console", "[window | console | lib | dll]", false, true },
    { OPT::PCH,      "Pch",      "none",    "name of precompiled header file, or \"none\" if not using precompiled"
                                             "headers", false, true },
    { OPT::PCH_CPP,  "Pch_cpp",  "none",    "source file used to build precompiled header (default uses same name"                                          "as PCH option)", false, false },
    { OPT::OPTIMIZE, "Optimize", "space",  "[space | speed] optimization to use in release builds", false, true },
    { OPT::WARN,     "Warn",     "4",      "[1-4] warning level", false, true },

    { OPT::CRT_REL,  "Crt_rel",  "static", "[static | dll] type of CRT to link to in release builds", false, true },
    { OPT::CRT_DBG,  "Crt_dbg",  "static", "[static | dll] type of CRT to link to in debug builds", false, true },

    { OPT::TARGET_DIR, "TargetDir", nullptr, "target directory", false, false },

    { OPT::BIT64, "64Bit", "true", "[true | false] indicates whether buildable as a 64-bit target", true, false },
    { OPT::TARGET_DIR64, "TargetDir64",  nullptr, "64-bit target directory", false, false },

    { OPT::BIT32, "32Bit", "false", "[true | false] indicates whether buildable as a 32-bit target", true, false },
    { OPT::TARGET_DIR32, "TargetDir32", nullptr, "32-bit target directory", false, false },

    { OPT::CFLAGS_CMN, "CFlags_cmn", nullptr, "common compiler flags", false, false },
    { OPT::CFLAGS_REL, "CFlags_rel", nullptr, "release build compiler flags", false, false },
    { OPT::CFLAGS_DBG, "CFlags_dbg", nullptr, "debug build compiler flags", false, false },

    { OPT::CLANG_CMN, "Clang_cmn", nullptr, "clang common compiler flags", false, false },
    { OPT::CLANG_REL, "Clang_rel", nullptr, "clang release build compiler flags", false, false },
    { OPT::CLANG_DBG, "Clang_dbg", nullptr, "clang debug build compiler flags", false, false },

    { OPT::LINK_CMN, "LFlags_cmn", nullptr, "common linker flags", false, false },
    { OPT::LINK_REL, "LFlags_rel", nullptr, "release build linker flags", false, false },
    { OPT::LINK_DBG, "LFlags_dbg", nullptr, "debug build linker flags", false, false },

    { OPT::RC_CMN, "Rc_cmn", nullptr, "common compiler flags", false, false },
    { OPT::RC_REL, "Rc_rel", nullptr, "release build compiler flags", false, false },
    { OPT::RC_DBG, "Rc_dbg", nullptr, "debug build compiler flags", false, false },

    { OPT::MIDL_CMN, "Midl_cmn", nullptr, "common compiler flags", false, false },
    { OPT::MIDL_REL, "Midl_rel", nullptr, "release build compiler flags", false, false },
    { OPT::MIDL_DBG, "Midl_dbg", nullptr, "debug build compiler flags", false, false },

    { OPT::NATVIS, "Natvis", nullptr, "Debug visualizer", false, false },

    { OPT::MS_LINKER, "Ms_linker", "true", "true means use link.exe even when compiling with clang", true, false },
    { OPT::MS_RC,     "Ms_rc",     "true",  "true means use rc.exe even when compiling with clang", true, false },

    { OPT::INC_DIRS,   "IncDirs",   nullptr, "additional directories for header files", false, false },
    { OPT::LIB_DIRS,   "LibDirs",   nullptr, "additional directories for library files", false, false },
    { OPT::LIB_DIRS64, "LibDirs64", nullptr, "additional directories for 64-bit library files", false, false },
    { OPT::LIB_DIRS32, "LibDirs32", nullptr, "additional directories for 32-bit library files", false, false },

    { OPT::BUILD_LIBS, "BuildLibs", nullptr, "libraries to build (built in makefile)", false, false },

    { OPT::LIBS_CMN, "Libs_cmn", nullptr, "additional libraries to link to in all builds", false, false },
    { OPT::LIBS_REL, "Libs_rel", nullptr, "additional libraries to link to in release builds", false, false },
    { OPT::LIBS_DBG, "Libs_dbg", nullptr, "additional libraries to link to in debug builds", false, false },

    { OPT::MAKE_DIR, "MakeDir", nullptr, "auto-generate makefile in specified directory", false, false },

    // The following options are for xgettext/msgfmt support

    { OPT::XGET_OUT,      "XGet_out",     nullptr, "output filename for xgettext", false, false },
    { OPT::XGET_KEYWORDS, "XGet_kwrds",   nullptr, "xgettext keywords (separated by semi-colon)", false, false },
    { OPT::XGET_FLAGS,    "XGet_flags",   nullptr, "xgettext flags", false, false },
    { OPT::MSGFMT_FLAGS,  "Msgfmt_flags", nullptr, "msgfmt flags", false, false },
    { OPT::MSGFMT_XML,    "Msgfmt_xml",   nullptr, "xml template file for msgfmt", false, false },

    { OPT::LAST, "", nullptr, nullptr, false, false}

}};
// clang-format on

void CSrcFiles::InitOptions()
{
    // Calling this twice will wipe out any options changed in between calls.
    ttASSERT(!m_Initialized);
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
    // The options are all indexed by the enumerated id, so it is imperative that each option appears in
    // DefaultOptions.
    for (size_t id = 0; id < m_Options.size(); ++id)
    {
        assertm(m_Options[id].OriginalName,
                "Option is missing a name! It means DefaultOptions is missing an option.");
    }
#endif
}

size_t CSrcFiles::FindOption(const std::string_view name) const
{
    assert(!name.empty());
    if (name.empty())
        return OPT::LAST;

    size_t pos { 0 };
    for (; pos < m_Options.size(); ++pos)
    {
        if (ttlib::is_sameas(name, m_Options[pos].OriginalName, tt::CASE::either))
            return pos;
    }
    return OPT::LAST;
}

void CSrcFiles::setOptValue(size_t index, std::string_view value)
{
    assert(index < OPT::LAST);

    if (!m_Options[index].isBooleanValue)
        m_Options[index].value = value;
    else
    {
        if (ttlib::is_sameprefix(value, "yes", tt::CASE::either) ||
            ttlib::is_sameprefix(value, "true", tt::CASE::either))
            m_Options[index].value = "true";
        else
            m_Options[index].value = "false";
    }
}

void CSrcFiles::setBoolOptValue(size_t index, bool value)
{
    assert(index < OPT::LAST);
    assert(m_Options[index].isBooleanValue);

    if (value)
        m_Options[index].value = "true";
    else
        m_Options[index].value = "false";
}
