/////////////////////////////////////////////////////////////////////////////
// Name:      CParseHHP
// Purpose:   Parse an HHP file to collect dependencies
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ttcstr.h>     // cstr -- Classes for handling zero-terminated char strings.
#include <ttcvector.h>  // cstrVector -- Vector of ttlib::cstr strings

// Parse an HHP file to collect dependencies
class CParseHHP
{
public:
    CParseHHP(const char* pszHHPName);

    enum
    {
        SECTION_UNKNOWN,
        SECTION_ALIAS,
        SECTION_FILES,
        SECTION_OPTIONS,
        SECTION_TEXT_POPUPS,
    };

    // Class functions

    void ParseHhpFile(std::string_view filename);

    ttlib::cstrVector m_lstDependencies;
    ttlib::cstr m_chmFilename;

protected:
    void AddDependency(std::string_view HHPfilename, std::string_view file);

private:
    // Class members

    ttlib::cstr m_cszCWD;
    ttlib::cstr m_cszRoot;  // Root directory to base all filenames and includes to
    ttlib::cstr m_HPPname;  // Root level HHP filename

    size_t m_section;
};
