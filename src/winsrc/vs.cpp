/////////////////////////////////////////////////////////////////////////////
// Name:      vs.cpp
// Purpose:   Creates .vs/tasks.vs.json and .vs/launch.vs.json (for devenv.exe)
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

// This generates files needed by Visual Studio. See vscode.cpp for generating files for Visual Studio Code.

#include "pch.h"

#include <filesystem>

#include <tttextfile.h>  // textfile -- Classes for reading and writing line-oriented files

#include "csrcfiles.h"  // CSrcFiles
#include "strtable.h"   // String resource IDs

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
            results.push_back(_tt(IDS_CANT_CREATE_VS_DIR));
            return false;
        }
    }

    ttlib::textfile file;

    file.ReadString(txtTasks);

    file.at(file.FindLineContaining("%tgtDir%")).Replace("%tgtDir%", cSrcFiles.GetTargetDir());

#if defined(_WIN32)
    file.at(file.FindLineContaining("%command%")).Replace("%command%", "nmake.exe -nologo debug");
#else
    // REVIEW: [KeyWorks - 8/1/2019] Visual Studio is available for MAC, will make.exe work?
    file.at(file.FindLineContaining("%command%")).Replace("%command%", "nmake.exe -nologo debug");
#endif

    if (!file.WriteFile(".vs/tasks.vs.json"))
    {
        std::ostringstream str;
        str << _tt(IDS_CANT_CREATE) << ".vs/tasks.vs.json";
        results.push_back(str.str());
        return false;
    }
    else
    {
        results.push_back(_tt(IDS_CREATED) + ".vs/tasks.vs.json");
    }

    file.clear();
    file.ReadString(txtLaunch);

    file.at(file.FindLineContaining("%targetD%")).Replace("%targetD%", cSrcFiles.GetTargetDebug());

    if (!cSrcFiles.GetProjectName().empty())
        file.at(file.FindLineContaining("%project%")).Replace("%project%", cSrcFiles.GetProjectName());
    else
    {
        std::ostringstream str;
        str << _tt(IDS_MISSING_PROJ_NAME) << cSrcFiles.GetSrcFilesName();
        results.push_back(str.str());
        return false;
    }

    if (!file.WriteFile(".vs/launch.vs.json"))
    {
        std::ostringstream str;
        str << _tt(IDS_CANT_CREATE) << ".vs/launch.vs.json";
        results.push_back(str.str());
        return false;
    }
    else
    {
        std::string str(_tt(IDS_CREATED));
        results.push_back(str);
    }

    return true;
}
