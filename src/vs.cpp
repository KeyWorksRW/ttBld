/////////////////////////////////////////////////////////////////////////////
// Name:      vs.cpp
// Purpose:   Creates .vs/tasks.vs.json and .vs/launch.vs.json
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttfile.h>     // ttCFile
#include "csrcfiles.h"  // CSrcFiles

static const char* txtTasks =

    "{\n"
    "    \"version\": \"0.2.1\",\n"
    "    \"outDir\": \"${workspaceRoot}%tgtDir%\",\n"
    "    \"tasks\": [\n"
    "        {\n"
    "            \"taskName\": \"makefile-build\",\n"
    "            \"appliesTo\": \"makefile\",\n"
    "            \"type\": \"launch\",\n"
    "            \"contextType\": \"build\",\n"
    "            \"command\": \"%command%\",\n"
    "            \"args\": [ \"build\" ],\n"
    "            \"envVars\": {\n"
    "                \"VSCMD_START_DIR\": \"${workspaceRoot}\"\n"
    "            }\n"
    "        }\n"
    "    ]\n"
    "}\n";

static const char* txtLaunch =

    "{\n"
    "    \"version\": \"0.2.1\",\n"
    "    \"defaults\": {},\n"
    "    \"configurations\": [\n"
    "        {\n"
    "            \"type\": \"default\",\n"
    "            \"project\": \"%targetD%\",\n"
    "            \"name\": \"%projgect%\"\n"
    "        }\n"
    "    ]\n"
    "}\n";

bool CreateVsJson(const char* pszSrcFiles, ttCList* plstResults)  // returns true unless unable to write to a file
{
    CSrcFiles cSrcFiles;
    cSrcFiles.ReadFile(pszSrcFiles);

    if (!ttDirExists(".vs"))
    {
        if (!ttCreateDir(".vs"))
        {
            ttMsgBox(_("Unable to create the required .vs directory."));
            return false;
        }
    }

    ttCFile file;

    file.ReadStrFile(txtTasks);

    if (cSrcFiles.GetBoolOption(OPT_64BIT) && cSrcFiles.GetDir64())
        file.ReplaceStr("%tgtDir%", cSrcFiles.GetDir64());
    else if (cSrcFiles.GetBoolOption(OPT_32BIT) && cSrcFiles.GetDir32())
        file.ReplaceStr("%tgtDir%", cSrcFiles.GetDir32());
    else
        file.ReplaceStr("%tgtDir%", "bin");

#if defined(_WIN32)
    file.ReplaceStr("%command%", "nmake.exe -nologo debug");
#else
    // REVIEW: [KeyWorks - 8/1/2019] Visual Studio is available for MAC, will make.exe work?
    file.ReplaceStr("%command%", "make.exe debug");
#endif

    if (!file.WriteFile(".vs/tasks.vs.json"))
    {
        if (plstResults)
        {
            ttCStr cszMsg;
            cszMsg.printf(GETSTRING(IDS_NINJA_CANT_WRITE), ".vs/tasks.vs.json");
            *plstResults += (char*) cszMsg;
        }
        return false;
    }
    else
    {
        if (plstResults)
            *plstResults += _("Created .vs/tasks.vs.json");
    }

    file.Delete();
    file.ReadStrFile(txtLaunch);

    if (cSrcFiles.GetBoolOption(OPT_64BIT) && cSrcFiles.GetTargetDebug64())
        file.ReplaceStr("%targetD%", cSrcFiles.GetTargetDebug64());
    else if (cSrcFiles.GetBoolOption(OPT_32BIT) && cSrcFiles.GetTargetDebug32())
        file.ReplaceStr("%targetD%", cSrcFiles.GetTargetDebug32());
    else
    {
        ttCStr cszMsg;
        cszMsg.printf("No build target specified in %s.", cSrcFiles.GetSrcFiles());
        *plstResults += (char*) cszMsg;
        return false;
    }

    if (cSrcFiles.GetProjectName())
        file.ReplaceStr("%projgect%", cSrcFiles.GetProjectName());
    else
    {
        ttCStr cszMsg;
        cszMsg.printf("No project name specified in %s.", cSrcFiles.GetSrcFiles());
        *plstResults += (char*) cszMsg;
        return false;
    }

    if (!file.WriteFile(".vs/launch.vs.json"))
    {
        if (plstResults)
        {
            ttCStr cszMsg;
            cszMsg.printf(GETSTRING(IDS_NINJA_CANT_WRITE), ".vs/launch.vs.json");
            *plstResults += (char*) cszMsg;
        }
        return false;
    }
    else
    {
        if (plstResults)
            *plstResults += _("Created .vs/launch.vs.json");
    }

    return true;
}
