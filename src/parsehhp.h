/////////////////////////////////////////////////////////////////////////////
// Name:      CParseHHP
// Purpose:   Parse an HHP file to collect dependencies
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ttlist.h>  // ttCList, ttCDblList, ttCStrIntList
#include <ttstr.h>   // ttCStr

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

    void ParseHhpFile(const char* pszHHP = nullptr);

    ttCList m_lstDependencies;
    ttCStr  m_cszChmFile;

protected:
    void AddDependency(const char* pszHHP, const char* pszFile);

private:
    // Class members

    ttCStr m_cszCWD;
    ttCStr m_cszRoot;     // root directory to base all filenames and includes to
    ttCStr m_cszHHPName;  // root level HHP filename

    size_t m_section;
};
