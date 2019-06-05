/////////////////////////////////////////////////////////////////////////////
// Name:      addfiles.cpp
// Purpose:   Adds files to .srcfiles Files: section
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttlist.h>     // ttCList, ttCDblList, ttCStrIntList
#include <ttfile.h>     // ttCFile

#include "csrcfiles.h"  // CSrcFiles
#include "dryrun.h"     // CDryRun
#include "strtable.h"   // String resource IDs

void AddFiles(ttCList& lstFiles, bool bDryRun)
{
    if (lstFiles.GetCount() < 1)
    {
        puts("You didn't specify any files to add!");
        return;
    }

    CSrcFiles cSrcFiles;
    if (!cSrcFiles.ReadFile())
    {
        ttCStr cszMsg;
        cszMsg.printf(IDS_CANNOT_LOCATE, (char*) cSrcFiles.m_cszSrcFilePath);
        puts(cszMsg);
        return;
    }

    for (size_t pos = 0; pos < lstFiles.GetCount(); ++pos)
    {
        if (cSrcFiles.m_lstSrcFiles.Find(lstFiles[pos]))
        {
            printf("%s already in .srcfiles\n", lstFiles[pos]);
            cSrcFiles.m_lstSrcFiles.Remove(pos);
        }
    }

    size_t cFilesAdded = 0;

    ttCFile kfIn, kfOut;
    if (!kfIn.ReadFile(cSrcFiles.m_cszSrcFilePath))
    {
        ttCStr cszMsg;
        cszMsg.printf(IDS_CS_CANNOT_OPEN, (char*) cSrcFiles.m_cszSrcFilePath);
        puts(cszMsg);
        return;
    }
    kfIn.MakeCopy();

    while (kfIn.ReadLine() && !ttIsSameSubStrI(kfIn, "Files:"))
        kfOut.WriteEol(kfIn);

    if (kfOut.IsEndOfFile())  // means there was no Files: section
    {
        kfOut.WriteEol("Files:");
        for (size_t pos = 0; pos < lstFiles.GetCount(); ++pos)
        {
            if (!ttFileExists(lstFiles[pos]))
            {
                printf("%s doesn't exist -- not added\n", lstFiles[pos]);
                continue;
            }
            kfOut.printf("  %s\n", lstFiles[pos]);
            ++cFilesAdded;
        }
        if (!kfOut.WriteFile(cSrcFiles.GetSrcFiles()))
        {
            puts("Cannot write to .srcfiles!");
            return;
        }

        // Use ttCStr to print the count to properly add the "s" to "file" as needed. I.e., 0 files, 1 file, 2 files -- 1
        // file is singular

        ttCStr cszResults;
        cszResults.printf("%u file%ks added.", cFilesAdded, cFilesAdded);
        puts(cszResults);
        return;
    }

    kfOut.WriteEol(kfIn);   // This will be the Files: line
    while (kfIn.ReadLine())
    {
        if (ttIsAlpha(*(const char*)kfIn))  // Alphabetical character in first column is a new section
            break;
        else if (!ttFindNonSpace(kfIn))   // blank line, insert after it
        {
            kfOut.WriteEol(kfIn);
            break;
        }
        else
            kfOut.WriteEol(kfIn);
    }

    // We are now after a blank line, at the beginning of a section, or at the end of the file

    for (size_t pos = 0; pos < lstFiles.GetCount(); ++pos)
    {
        if (!ttFileExists(lstFiles[pos]))
        {
            printf("%s doesn't exist -- not added\n", lstFiles[pos]);
            continue;
        }
        kfOut.printf("  %s\n", lstFiles[pos]);
        ++cFilesAdded;
    }

    // Now that the file(s) have been inserted, add everything else

    while (kfIn.ReadLine())
        kfOut.WriteEol(kfIn);

    if (bDryRun)
    {
        CDryRun dryrun;
        dryrun.Enable();
        dryrun.NewFile(cSrcFiles.GetSrcFiles());
        dryrun.DisplayFileDiff(kfIn, kfOut);
        return;
    }

    if (!kfOut.WriteFile(cSrcFiles.GetSrcFiles()))
    {
        printf(ttGetResString(IDS_CS_CANT_WRITE), cSrcFiles.GetSrcFiles());
        puts("Cannot write to .srcfiles!");
        return;
    }

    // Use ttCStr to print the count to properly add the "s" to "file" as needed. I.e., 0 files, 1 file, 2 files

    ttCStr cszResults;
    cszResults.printf("%u file%ks added.", cFilesAdded, cFilesAdded);
    puts(cszResults);
}
