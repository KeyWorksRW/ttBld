/////////////////////////////////////////////////////////////////////////////
// Purpose:   Structures and enum for storing/retrieving options in a .srcfiles.yaml file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>

class OPT
{
public:
    // We use this instead of "enum class Opt : size_t" because it allows iterating through the vector and returning a
    // position as the option id.
    enum value : size_t
    {
        FIRST,
        BIT32 = FIRST,
        BIT64,
        BUILD_LIBS,
        BUILD_LIBS32,
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
        MSVC_CMN,
        MSVC_DBG,
        MSVC_REL,
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
        MAKE_DIR,

        // Last enumeration used to determine size of containers
        LAST
    };

    enum : bool
    {
        optional = false,
        required = true,

        any = false,
        boolean = true,
    };

    struct CURRENT
    {
        ttlib::cstr value;
        ttlib::cstr comment;

        const char* OriginalName;
        const char* OriginalValue;
        const char* OriginalComment;

        bool isBooleanValue;
        bool isRequired;
    };

    struct ORIGINAL
    {
        // This must be one of the ids from the enumeration above
        OPT::value optionID;

        const char* name;
        const char* value;
        const char* comment;

        bool isBooleanValue;
        bool isRequired;
    };

    struct VERSION
    {
        OPT::value optionID;
        size_t major;
        size_t minor;
        size_t sub;
    };
};

extern const std::array<OPT::ORIGINAL, OPT::LAST + 1> DefaultOptions;

// The following code makes it possible to iterate through all of the OPT values via:
//
// for (const auto& option: optIterator()) {}

#include <type_traits>

template <typename C, C beginVal, C endVal>
class Iterator
{
    typedef typename std::underlying_type<C>::type val_t;
    size_t val;

public:
    Iterator(const C& f) : val(static_cast<val_t>(f)) {}
    Iterator() : val(static_cast<val_t>(beginVal)) {}
    Iterator operator++()
    {
        ++val;
        return *this;
    }
    C operator*() { return static_cast<C>(val); }
    Iterator begin() { return *this; }  // default ctor is good
    Iterator end()
    {
        static const Iterator endIter = ++Iterator(endVal);  // cache it
        return endIter;
    }
    bool operator!=(const Iterator& i) { return val != i.val; }
};

typedef Iterator<OPT::value, OPT::BIT32, static_cast<OPT::value>(OPT::LAST - 1)> optIterator;
