/////////////////////////////////////////////////////////////////////////////
// Name:      CSrcOptions
// Purpose:   Class for storing/retrieving options in a .srcfiles.yaml file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <ttarray.h>  // ttCArray

#define OLD_OPTIONS

class Opt
{
public:
    enum : size_t
    {
        BIT32,
        BIT64,
        BUILD_LIBS,
        CFLAGS_CMN,
        CFLAGS_DBG,
        CFLAGS_REL,
        CLANG_CMN,
        CLANG_DBG,
        CLANG_REL,
        CRT_DBG,
        CRT_REL,
        EXE_TYPE,
        INC_DIRS,
        LIBS_CMN,
        LIBS_DBG,
        LIBS_REL,
        LIB_DIRS,
        LIB_DIRS32,
        LIB_DIRS64,
        LINK_CMN,
        LINK_DBG,
        LINK_REL,
        MIDL_CMN,
        MIDL_DBG,
        MIDL_REL,
        MSGFMT_FLAGS,
        MSGFMT_XML,
        MS_LINKER,
        MS_RC,
        NATVIS,
        OPTIMIZE,
        PCH,
        PCH_CPP,
        PROJECT,
        RC_CMN,
        RC_DBG,
        RC_REL,
        TARGET_DIR,
        TARGET_DIR32,
        TARGET_DIR64,
        WARN,
        XGET_FLAGS,
        XGET_KEYWORDS,
        XGET_OUT,

        // Last enumeration used to determine size of containers
        LAST
    };

    struct ORIGINAL_OPTIONS
    {
        size_t optionID;

        const char* name;
        const char* value;
        const char* comment;

        bool BooleanValue;
        bool Required;
    };

    // Contains current information about an option.
    struct OPTION
    {
        size_t optionID;

        const char* OriginalName;
        const char* OriginalValue;
        const char* OriginalComment;

        std::string value;
        std::string comment;

        bool BooleanValue;
        bool Required;
    };

    std::vector<OPTION> m_Options;

    const char* getOptValue(size_t index)
    {
        ttASSERT(index < LAST);
        return m_Options.at(index).value.c_str();
    }

    void setOptValue(size_t index, std::string_view value);

    const char* getCmtValue(size_t index)
    {
        ttASSERT(index < LAST);
        return m_Options.at(index).comment.c_str();
    }
    void setCmtValue(size_t index, std::string_view value)
    {
        ttASSERT(index < LAST);
        m_Options.at(index).comment = value;
    }
};

namespace sfopt  // .srcfiles.yaml otpions
{
    extern const char* txtNinjaVerFormat;  // "# Requires ttBld version %d.%d.%d or higher to process";

    // Used as an index into all currently supported options.
    typedef enum
    {
        // Don't wrap the comments -- wee need them on one line to show up in Intellisense
        // clang-format off
        OPT_ERROR = 0,

        OPT_PROJECT,        // name of the project--will be used as the base target name (i.e., project: foo, target: foo.exe, fooD.exe, etc.)
        OPT_EXE_TYPE,       // [window | console | lib | dll]
        OPT_PCH,            // name of precompiled header file, or "none" if not using precompiled headers
        OPT_PCH_CPP,        // source file used to build precompiled header (default uses same name as PCH option)

        OPT_DEBUG_RC,       // true means build a -D_DEBUG version of the project's rc file
        OPT_PERMISSIVE,     // true means add -permissive- compiler flag
        OPT_STDCALL,        // use stdcall calling convention
        OPT_MS_LINKER,      // use link.exe even when compiling with CLANG
        OPT_MS_RC,          // use rc.exe even when compiling with CLANG

        OPT_STATIC_CRT_REL, // true means link to static CRT in release builds
        OPT_STATIC_CRT_DBG, // true means link to static CRT in debug builgs

        OPT_CFLAGS_CMN,     // flags to pass to the compiler in all build targets
        OPT_CFLAGS_REL,     // flags to pass to the compiler in release builds
        OPT_CFLAGS_DBG,     // flags to pass to the compiler in debug builds

        OPT_NATVIS,         // Specifies a .natvis file to link into the pdb file

        OPT_LINK_CMN,       // flags to pass to the linker in all build targets
        OPT_LINK_REL,       // flags to pass to the linker in release builds
        OPT_LINK_DBG,       // flags to pass to the linker in debug builds

        OPT_RC_CMN,         // flags to pass to the resource compiler in all build targets
        OPT_RC_REL,         // flags to pass to the resource compiler in release builds
        OPT_RC_DBG,         // flags to pass to the resource compiler in debug builds

        OPT_MDL_CMN,        // flags to pass to the midl compiler in all build targets
        OPT_MDL_REL,        // flags to pass to the midl compiler in release builds
        OPT_MDL_DBG,        // flags to pass to the midl compiler in debug builds

        OPT_CLANG_CMN,      // flags to pass to the CLANG compiler in all build targets
        OPT_CLANG_REL,      // flags to pass to the CLANG compiler in release builds
        OPT_CLANG_DBG,      // flags to pass to the CLANG compiler in debug builds

        OPT_TARGET_DIR,     // target directory
        OPT_64BIT,          // Generate scripts for 64-bit build
        OPT_TARGET_DIR64,   // 64-bit target directory

        OPT_32BIT,          // Generate scripts for 32-bit build
        OPT_TARGET_DIR32,   // 32-bit target directory

        OPT_INC_DIRS,       // Additional directories for header files
        OPT_LIB_DIRS,       // Additional directories for library files

        OPT_LIB_DIRS64,     // 64-bit library directories
        OPT_LIB_DIRS32,     // 32-bit library directories
        OPT_LIBS_CMN,       // Additional libraries to link to in all builds
        OPT_LIBS_REL,       // Additional libraries to link to in release builds
        OPT_LIBS_DBG,       // additional libraries to link to in debug builds
        OPT_OPTIMIZE,       // [space | speed] optimization (optimizing for speed can actually make the code run slower due to caching issues) -- default, if not specified, is space
        OPT_WARN_LEVEL,     // [1-4] default, if not specified, is 4
        OPT_BUILD_LIBS,     // libraries that need to be built (added to makefile generation)

        OPT_XGET_OUT,       // output filename for xgettext
        OPT_XGET_KEYWORDS,  // additional keywords (separated by semi-colon) to pass to xgettext
        OPT_XGET_FLAGS,     // additional flags to pass to xgettext
        OPT_MSGFMT_FLAGS,   // additional flags to pass to msgfmt
        OPT_MSGFMT_XML,     // the name of the xml template file for msgfmt to use

        OPT_OVERFLOW
        // clang-format on
    } OPT_INDEX;

    // { OPT_xxx, "name", "def value", boolean, required, "def comment" }

    typedef struct  // The original default settings for an option
    {
        OPT_INDEX   opt;
        const char* pszName;
        const char* pszVal;
        bool        bBoolType;
        bool        bRequired;
        const char* pszComment;
    } OPT_SETTING;

    typedef struct  // The updated version of the option
    {
        char* pszVal;
        char* pszComment;
        bool  bRequired;
    } OPT_UPDATE;

    typedef struct
    {
        OPT_INDEX opt;
        int       major;
        int       minor;
        int       sub;
    } OPT_VERSION;

}  // namespace sfopt

class CSrcOptions
{
public:
    CSrcOptions();
    ~CSrcOptions();

    // Public functions

    // Call this when a .srcfiles.yaml file is first read. It will initialize m_Options with
    // all of the default values and comments.
    void InitOptions();

    // [KeyWorks - 01-31-2020] The Get/Set functions use the old s_aInitialOptions/m_aUpdateOpts arrays.
    // Newer code should use the get/set functions (lowercase leading letter) to use the new Opt class vector.

    std::string_view getOptValue(size_t index) const
    {
        assert(index < Opt::LAST);
        return m_opt.m_Options.at(index).value;
    }

    bool getOptBoolean(size_t index) const
    {
        assert(index < Opt::LAST);
        return tt::issamestr(m_opt.m_Options.at(index).value, "true");
    }

    std::string_view getCmtValue(size_t index) const
    {
        assert(index < Opt::LAST);
        return m_opt.m_Options.at(index).comment;
    }

    void setOptValue(size_t index, std::string_view value) { m_opt.setOptValue(index, value); }
    void setOptComment(size_t index, std::string_view value) { m_opt.setCmtValue(index, value); }

    void setOptRequired(sfopt::OPT_INDEX index, bool bVal = true);

    // Call this when reading an entire option line
    void setOptLine(std::string& name, std::string& value, std::string comment);

    size_t findID(std::string_view name) const;

    // Convert an sfopt: id to a Opt:: id
    size_t ConvertID(sfopt::OPT_INDEX index) const;

    const Opt::ORIGINAL_OPTIONS& FindOriginal(size_t option) const;

    Opt::OPTION& FindOption(const std::string_view name);

    Opt m_opt;

    // [KeyWorks - 01-31-2020] All of the following functions are obsolete!

    const char* GetOption(sfopt::OPT_INDEX index);
    bool        GetBoolOption(sfopt::OPT_INDEX index);
    const char* GetOptComment(sfopt::OPT_INDEX index);

    // Returns true if the option has changed
    bool GetChanged(sfopt::OPT_INDEX index);

    bool GetRequired(sfopt::OPT_INDEX index);  // returns true if the option is required
    void SetRequired(sfopt::OPT_INDEX index, bool bVal = true);

    // Fine to call this for boolean options if pszVal == "true/false" or "yes/no"
    sfopt::OPT_INDEX UpdateOption(sfopt::OPT_INDEX index, const char* pszVal);
    sfopt::OPT_INDEX UpdateOption(sfopt::OPT_INDEX index, bool bVal);
    sfopt::OPT_INDEX UpdateReadOption(const char* pszName, const char* pszVal, const char* pszComment);

    const sfopt::OPT_VERSION* GetOptionMinVersion(sfopt::OPT_INDEX index);

    const sfopt::OPT_SETTING* GetOrgOptions();

    const char* GetOptVal(size_t pos) const { return m_aUpdateOpts[pos].pszVal; }

private:
    // Class members

    // This structure is similar to OPT_SETTINGS, but is used to store changes to an option
    ttCArray<sfopt::OPT_UPDATE> m_aUpdateOpts;
};
