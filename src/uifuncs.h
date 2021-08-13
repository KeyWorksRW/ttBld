/////////////////////////////////////////////////////////////////////////////
// Purpose:   Miscellaneous functions for displaying UI
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/msgdlg.h>
#include <wx/string.h>

/////////////////////// inline functions ////////////////////////////////////////

inline int appMsgBox(const ttlib::cstr str, ttlib::cview caption = txtAppname, int style = wxICON_WARNING | wxOK,
                     wxWindow* parent = nullptr)
{
    return wxMessageBox(str.wx_str(), caption.c_str(), style, parent);
}
