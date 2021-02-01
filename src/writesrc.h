/////////////////////////////////////////////////////////////////////////////
// Purpose:   Writes a new or update srcfiles.yaml file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "csrcfiles.h"

inline constexpr const char* txtNinjaVerFormat = "# Updated by ttBld.exe version 1.7.2 -- see https://github.com/KeyWorksRW/ttBld";

// This class inherits from CSrcFiles and can be used anywhere CSrcFiles is used.
class CWriteSrcFiles : public CSrcFiles
{
public:
    CWriteSrcFiles();

    // Write updates to the Options: section only. Rest of the file is written unchanged.
    bld::RESULT UpdateOptions(std::string_view filename = ttlib::emptystring);

    // The default filename is determined by the current platform. .srcfiles.win.yaml for
    // Windows, .srcfiles.unix.yaml for Unix, etc.
    //
    // If specified, comment will be added at he the top of the file after the ttBld
    // version line.
    bld::RESULT WriteNew(std::string_view filename = ttlib::emptystring, std::string_view comment = ttlib::emptystring);

private:
    ttlib::cstr m_outFilename;
    ttlib::cstr m_fileComment;  // comment to appear at the top of the file
};
