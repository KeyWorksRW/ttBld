/////////////////////////////////////////////////////////////////////////////
// Purpose:   Class to store information for a dry-run of functionality
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <iostream>

#include "dryrun.h"  // CDryRun

void CDryRun::NewFile(std::string_view filename)
{
    ASSERT(!filename.empty());

    m_filename = filename;
}

void CDryRun::DisplayFileDiff(const ttlib::viewfile& orgfile, const ttlib::textfile& newfile)
{
    if (!m_filename.empty())
    {
        std::cout << m_filename << " dryrun changes:" << '\n';
    }

    size_t posOrg = 0;
    size_t posNew = 0;

    for (; posOrg < orgfile.size() && posNew < newfile.size(); ++posOrg, ++posNew)
    {
        if (!newfile[posOrg].is_sameas(orgfile[posNew]))
        {
            auto posSync = newfile.FindLineContaining(orgfile[posOrg]);
            if (posSync != tt::npos)
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
