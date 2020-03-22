/////////////////////////////////////////////////////////////////////////////
// Name:      vs.cpp
// Purpose:   Creates .vs/tasks.vs.json and .vs/launch.vs.json (for devenv.exe)
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

// This generates files needed by Visual Studio. See vscode.cpp for generating files for Visual Studio Code.

#include "pch.h"

#include <filesystem>

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

// returns true unless unable to write to a file
bool CreateVsJson(const char* pszSrcFiles, std::vector<std::string>& results)
{
    CSrcFiles cSrcFiles;
    cSrcFiles.ReadFile(pszSrcFiles);

    if (!std::filesystem::exists(".vs"))
    {
        if (!std::filesystem::create_directory(".vs"))
        {
            results.push_back(_tt("Unable to create the required .vs directory."));
            return false;
        }
    }

    ttCFile file;

    file.ReadStrFile(txtTasks);

    file.ReplaceStr("%tgtDir%", cSrcFiles.GetTargetDir().c_str());

#if defined(_WIN32)
    file.ReplaceStr("%command%", "nmake.exe -nologo debug");
#else
    // REVIEW: [KeyWorks - 8/1/2019] Visual Studio is available for MAC, will make.exe work?
    file.ReplaceStr("%command%", "make.exe debug");
#endif

    if (!file.WriteFile(".vs/tasks.vs.json"))
    {
        std::ostringstream str;
        str << _tt("Unable to create or write to ") << ".vs/tasks.vs.json";
        results.push_back(str.str());
        return false;
    }
    else
    {
        results.push_back(_tt("Created .vs/tasks.vs.json"));
    }

    file.Delete();
    file.ReadStrFile(txtLaunch);

    file.ReplaceStr("%targetD%", cSrcFiles.GetTargetDebug().c_str());

    if (!cSrcFiles.GetProjectName().empty())
        file.ReplaceStr("%projgect%", cSrcFiles.GetProjectName().c_str());
    else
    {
        std::ostringstream str;
        str << _tt("No project name specified in ") << cSrcFiles.GetSrcFilesName();
        results.push_back(str.str());
        return false;
    }

    if (!file.WriteFile(".vs/launch.vs.json"))
    {
        std::ostringstream str;
        str << _tt("Unable to create or write to ") << ".vs/launch.vs.json";
        results.push_back(str.str());
        return false;
    }
    else
    {
        std::string str(_tt("Created .vs/launch.vs.json"));
        results.push_back(str);
    }

    return true;
}
