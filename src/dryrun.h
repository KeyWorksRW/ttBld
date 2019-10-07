/////////////////////////////////////////////////////////////////////////////
// Name:      CDryRun
// Purpose:   Class to store information for a dry-run of functionality
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ttstr.h>   // ttCStr
#include <ttfile.h>  // ttCFile

// Class to store information for a dry-run of functionality
class CDryRun
{
public:
    CDryRun() { m_bEnabled = false; }

    // Class functions

    void        Enable() { m_bEnabled = true; }
    bool        IsEnabled() { return m_bEnabled; }
    const char* GetFileName() { return m_cszFilename; }

    void NewFile(const char* pszFile);
    void DisplayFileDiff(ttCFile& fileOrg, ttCFile& fileNew);

private:
    // Class members

    ttCStr m_cszFilename;
    bool   m_bEnabled;
};
