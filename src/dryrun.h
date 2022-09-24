/////////////////////////////////////////////////////////////////////////////
// Purpose:   Class to store information for a dry-run of functionality
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "tttextfile_wx.h"

// Class to store information for a dry-run of functionality
class CDryRun
{
public:
    CDryRun() {}

    // Class functions

    void Enable() { isEnabled = true; }
    bool IsEnabled() { return isEnabled; }
    const ttlib::cstr& GetFileName() { return m_filename; }

    void NewFile(std::string_view filename);
    // Display each line that differs from the original
    void DisplayFileDiff(const ttlib::viewfile& orgfile, const ttlib::textfile& newfile);

private:
    // Class members

    ttlib::cstr m_filename;
    bool isEnabled { false };
};
