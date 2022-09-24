/////////////////////////////////////////////////////////////////////////////
// Name:      ttcview.h
// Purpose:   string_view functionality on a zero-terminated char string.
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020-2022 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

/// @file
/// Provides a view of a zero-terminated char string. Includes a c_str() function and a const char*() operator
/// to make it easier to pass to functions expecting a C-style string.
///
/// Unlike std::string_view, there is no remove_suffix() function since you cannot change the length of the buffer
/// (it would no longer be zero-terminated.). There is a subview() function which returns a cview, but you can
/// only specify the starting position, not the length. You can use substr() to get a non-zero terminated
/// std::string_view.
///
/// Caution: as with std::string_view, the view is only valid as long as the string you are viewing has not
/// been modified or destroyed. This is also true of substr() and subview().

#pragma once

#if !(__cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L))
    #error "The contents of <ttcview.h> are available only with C++17 or later."
#endif

#include <ttsview_wx.h>

// This can be used to conditionalize code where <ttcview.h> is available or not
#define _TTLIB_CVIEW_AVAILABLE_

namespace ttlib
{
    class cview : public sview
    {
    public:
        cview(const std::string& str) : sview(str) {}
        cview(const char* str, size_t len) : sview(str, len) {}
        cview(const char* str) : sview(str) {}

        // A string view is not guarenteed to be zero-terminated, so you can't construct from it
        cview(std::string_view str) = delete;

        /// cview is zero-terminated, so c_str() can be used wherever std::string.c_str()
        /// would be used.
        constexpr const char* c_str() const noexcept { return data(); };

        /// Can be used to pass the view to a function that expects a C-style string.
        operator const char*() const noexcept { return data(); }

#if 0
        /// Returns a zero-terminated view. Unlike substr(), you can only specify the starting position.
        cview subview(size_t start = 0) const
        {
            if (start > length())
                start = length();
            return cview(c_str() + start, length() - start);
        }
#endif
    };
}  // namespace ttlib
