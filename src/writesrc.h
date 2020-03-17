/////////////////////////////////////////////////////////////////////////////
// Name:      CWriteSrcFiles
// Purpose:   Writes a new or update srcfiles.yaml file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   MIT License (see MIT License)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "csrcfiles.h"

constexpr const char* txtNinjaVerFormat = "# Requires ttBld.exe version %d.%d.%d or higher to process";

// This class inherits from CSrcFiles and can be used anywhere CSrcFiles is used.
class CWriteSrcFiles : public CSrcFiles
{
public:
    CWriteSrcFiles();

    // Public functions

    // Write updates to the Options: section only. Rest if the file is written unchanged.
    bld::RESULT UpdateOptions(std::string_view filename = ttlib::emptystring);

    // The default filename is determined by the current platform. .srcfiles.win.yaml for
    // Windows, .srcfiles.unix.yaml for Unix, etc.
    //
    // If specified, comment will be added at he the top of the file after the ttBld
    // requirement line.
    bld::RESULT WriteNew(std::string_view filename = ttlib::emptystring,
                         std::string_view comment = ttlib::emptystring);

protected:
private:
    // Class members

    ttlib::cstr m_outFilename;
    ttlib::cstr m_fileComment;  // comment to appear at the top of the file
};
