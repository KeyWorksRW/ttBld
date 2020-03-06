/////////////////////////////////////////////////////////////////////////////
// Name:      addfiles.cpp
// Purpose:   Adds files to .srcfiles Files: section
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttlist.h>  // ttCList, ttCDblList, ttCStrIntList
#include <ttfile.h>  // ttCFile

#include <ttTR.h>  // Function for translating strings

#include "csrcfiles.h"  // CSrcFiles
#include "dryrun.h"     // CDryRun

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
        cszMsg.printf(_tt("Cannot locate the file %s"), (char*) cSrcFiles.GetSrcFiles());
        puts(cszMsg);
        return;
    }

    size_t cFilesAdded = 0;
    for (size_t pos = 0; pos < lstFiles.GetCount(); ++pos)
    {
        if (cSrcFiles.GetSrcFilesList().appendi(lstFiles[pos]))
        {
            ++cFilesAdded;
        }
    }

    ttCFile kfIn, kfOut;
    if (!kfIn.ReadFile(cSrcFiles.GetSrcFiles()))
    {
        ttCStr cszMsg;
        cszMsg.printf(_tt("Cannot open \"%s\"."), (char*) cSrcFiles.GetSrcFiles());
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
                printf(_tt("The file %s is already in .srcfiles.yaml\n"), lstFiles[pos]);
                continue;
            }
            kfOut.printf("  %s\n", lstFiles[pos]);
            ++cFilesAdded;
        }
        if (!kfOut.WriteFile(cSrcFiles.GetSrcFiles()))
        {
            ttMsgBoxFmt(_tt("Unable to create or write to %s"), MB_OK | MB_ICONWARNING, cSrcFiles.GetSrcFiles());
            return;
        }

        printf(_tt("%u files added."), cFilesAdded);
        return;
    }

    kfOut.WriteEol(kfIn);  // This will be the Files: line
    while (kfIn.ReadLine())
    {
        if (ttIsAlpha(*(const char*) kfIn))  // Alphabetical character in first column is a new section
            break;
        else if (!ttFindNonSpace(kfIn))  // blank line, insert after it
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
            printf(_tt("The file %s is already in .srcfiles.yaml\n"), lstFiles[pos]);
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
        printf(_tt("Unable to create or write to %s"), cSrcFiles.GetSrcFiles());
        return;
    }

    printf(_tt("%u files added."), cFilesAdded);
}