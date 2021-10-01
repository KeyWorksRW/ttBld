/////////////////////////////////////////////////////////////////////////////
// Purpose:   used to read, write, and compare ttBld version number
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "verninja.h"  // CVerMakeNinja

extern const char* txtOptVersion;  // The minimum version of ttBld required by a .srcfiles.yaml

CVerMakeNinja::CVerMakeNinja()
{
    m_minMajor = 1;  // minimum required version for all options
    m_minMinor = 0;
    m_minSub = 0;

    ttlib::cview version = txtOptVersion;  // we assume this string to be "n.n.n" where n is an integer for major,
                                           // minor, and sub version
    m_major = ttlib::atoi(version);
    auto pos = version.find('.');
    version = version.subview(pos + 1);

    m_minor = ttlib::atoi(version);
    pos = version.find('.');
    version = version.subview(pos + 1);

    m_sub = ttlib::atoi(version);
}

bool CVerMakeNinja::IsSrcFilesNewer(int majorSrcFiles, int minorSrcFiles, int subSrcFiles)
{
    if (majorSrcFiles > m_major)
        return true;
    else if (majorSrcFiles < m_major)
        return false;

    // Major number identical, check minor number

    if (minorSrcFiles > m_minor)
        return true;
    else if (minorSrcFiles < m_minor)
        return false;

    // Major and minor number identical, check sub number

    return (subSrcFiles > m_sub);
}
