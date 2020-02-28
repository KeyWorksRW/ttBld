/////////////////////////////////////////////////////////////////////////////
// Name:      CVerMakeNinja
// Purpose:   Used to read, write, and compare ttBld version number
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

// Used to read, write, and compare ttBld version number
class CVerMakeNinja
{
public:
    // This will initialize our current version of major, minor, and sub values
    CVerMakeNinja();

    // Public functions

    bool IsSrcFilesNewer(int majorSrcFiles, int minorSrcFiles, int subSrcFiles);
    // Call this with the string "# Requires MakeNinja version n.n.n or higher to process"

private:
    // Class members

    int m_major;
    int m_minor;
    int m_sub;

    int m_minMajor;
    int m_minMinor;
    int m_minSub;
};
