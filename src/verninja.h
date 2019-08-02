/////////////////////////////////////////////////////////////////////////////
// Name:      CVerMakeNinja
// Purpose:   used to read, write, and compare ttMakeNinja version number
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ttstr.h>    // ttCStr

class CVerMakeNinja
{
public:
    CVerMakeNinja();    // this will initial our current version of major, minor, and sub values

    // Public functions

    bool IsSrcFilesNewer(int majorSrcFiles, int minorSrcFiles, int subSrcFiles);
    bool IsSrcFilesNewer(const char* pszRequired);  // call this with the string "# Requires MakeNinja version n.n.n or higher to process"

private:
    // Class members

    ttCStr m_cszString;

    int m_major;
    int m_minor;
    int m_sub;

    int m_minMajor;
    int m_minMinor;
    int m_minSub;
};
