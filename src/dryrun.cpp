/////////////////////////////////////////////////////////////////////////////
// Name:      CDryRun
// Purpose:   Class to store information for a dry-run of functionality
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <iostream>

#include <ttTR.h>     // Function for translating strings
#include <ttdebug.h>  // ttASSERT macros

#include "dryrun.h"  // CDryRun

void CDryRun::NewFile(std::string_view filename)
{
    ttASSERT(!filename.empty());

    m_filename = filename;
}

void CDryRun::DisplayFileDiff(const ttlib::viewfile& orgfile, const ttlib::textfile& newfile)
{
    if (!m_filename.empty())
    {
        std::cout << m_filename << _tt(" dryrun changes:") << '\n';
    }

    size_t posOrg = 0;
    size_t posNew = 0;

    for (; posOrg < orgfile.size() && posNew < newfile.size(); ++posOrg, ++posNew)
    {
        if (!newfile[posOrg].issameas(orgfile[posNew]))
        {
            auto posSync = newfile.FindLineContaining(orgfile[posOrg]);
            if (posSync != ttlib::npos)
            {
                while (posNew < posSync)
                {
                    std::cout << "new: " << newfile[posNew++] << '\n';
                }
                continue;
            }

            std::cout << posOrg << ": " << orgfile[posOrg] << '\n';
            std::cout << posNew << ": " << newfile[posNew] << '\n';
        }
    }
}
