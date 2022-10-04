/////////////////////////////////////////////////////////////////////////////
// Name:      ttcwd.h
// Purpose:   Class for storing and optionally restoring the current directory
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#pragma once

// Note: this is a header-only class

#if !defined(_WX_DEFS_H_)
    #error "You must include wx/defs.h before including this file."
#endif

#include "ttcstr_wx.h"    // ttString -- wxString with additional methods similar to ttlib::cstr
#include "ttstring_wx.h"  // ttString -- wxString with additional methods similar to ttlib::cstr

namespace ttlib
{
    /// Retrieves the current working directory. Construct with (true) to restore the
    /// directory in the dtor.
    class cwd : public ttlib::cstr
    {
    public:
        enum : bool
        {
            no_restore = false,
            restore = true
        };

        // Specify true to restore the directory in the destructor
        cwd(bool restore = cwd::no_restore)
        {
            assignCwd();
            if (restore)
                m_restore.assign(*this);
        }

        ~cwd()
        {
            if (!m_restore.empty())
                ttlib::ChangeDir(m_restore);
        }

        /// Assumes the current string is a directory path, and changes the directory to it.
        bool ChangeDir() { return ttlib::ChangeDir(*this); }

    private:
        ttlib::cstr m_restore;
    };

    /// Retrieves the current working directory. Construct with (true) to restore the
    /// directory in the dtor.
    class cwd_wx : public ttString
    {
    public:
        enum : bool
        {
            no_restore = false,
            restore = true
        };

        // Specify true to restore the directory in the destructor
        cwd_wx(bool restore = cwd::no_restore)
        {
            assignCwd();
            if (restore)
                m_restore.assign(*this);
        }

        ~cwd_wx()
        {
            if (m_restore.size())
                m_restore.ChangeDir();
        }

    private:
        ttString m_restore;
    };

}  // namespace ttlib
