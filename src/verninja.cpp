/////////////////////////////////////////////////////////////////////////////
// Name:      CVerMakeNinja
// Purpose:   used to read, write, and compare ttBld version number
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "verninja.h"  // CVerMakeNinja

extern const char* txtOptVersion;  // The minimum version of ttBld required by a .srcfiles.yaml

CVerMakeNinja::CVerMakeNinja()
{
    m_minMajor = 1;  // minimum required version for all options
    m_minMinor = 0;
    m_minSub = 0;

    const char* psz = txtOptVersion;  // we assume this string to be "n.n.n" where n is an integer for major, minor, and sub version
    m_major = (int) ttAtoi(psz);
    psz = ttStrChr(psz, '.') + 1;
    m_minor = (int) ttAtoi(psz);
    psz = ttStrChr(psz, '.') + 1;
    ttASSERT_MSG(ttIsDigit(*psz), "Invalid txtOptVersion string! Many bad things can happen if this isn't fixed!");
    m_sub = (int) ttAtoi(psz);
}

bool CVerMakeNinja::IsSrcFilesNewer(const char* pszRequired)
{
    ttASSERT_MSG(pszRequired, "NULL pointer!");

    while (!ttIsDigit(*pszRequired))
        ++pszRequired;

    int major = (int) ttAtoi(pszRequired);

    pszRequired = ttStrChr(pszRequired, '.');
    ttASSERT_MSG(pszRequired && ttIsDigit(pszRequired[1]), "Invalid ttBld version line!");
    if (!pszRequired || !ttIsDigit(pszRequired[1]))
        return false;  // we don't know what the version number is
    int minor = (int) ttAtoi(++pszRequired);

    pszRequired = ttStrChr(pszRequired, '.');
    ttASSERT_MSG(pszRequired && ttIsDigit(pszRequired[1]), "Invalid ttBld version line!");
    if (!pszRequired || !ttIsDigit(pszRequired[1]))
        return false;  // we don't know what the version number is
    int sub = (int) ttAtoi(++pszRequired);

    return IsSrcFilesNewer(major, minor, sub);
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
