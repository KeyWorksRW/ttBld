/////////////////////////////////////////////////////////////////////////////
// Name:      CDryRun
// Purpose:   Class to store information for a dry-run of functionality
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <stdio.h>

#include "dryrun.h"  // CDryRun

void CDryRun::NewFile(const char* pszFile)
{
    ttASSERT_MSG(pszFile, "NULL pointer!");

    m_cszFilename = pszFile;
}

void CDryRun::DisplayFileDiff(ttCFile& fileOrg, ttCFile& fileNew)
{
    if (m_cszFilename.IsNonEmpty())
        printf(_("%s dryrun changes:\n"), (char*) m_cszFilename);

    fileNew.PrepForReadLine();
    while (fileNew.ReadLine())
    {
        fileOrg.ReadLine();
        if (!ttIsSameStr(fileOrg, fileNew))
        {
            printf(_("%s Options: section updated.\n"), (char*) fileOrg);
            printf(_("    new: %s\n"), (char*) fileNew);
        }
    }
}
