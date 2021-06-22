/////////////////////////////////////////////////////////////////////////////
// Purpose:   Creates .vs/tasks.vs.json and .vs/launch.vs.json (for devenv.exe)
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

// This generates files needed by Visual Studio. See vscode.cpp for generating files for Visual Studio Code.

#include "pch.h"

#include <filesystem>
#include <vector>
#include <sstream>

#include "tttextfile.h"  // textfile -- Classes for reading and writing line-oriented files

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
            results.push_back(_tt(strIdCantCreateVsDir));
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
        str << _tt(strIdCantWrite) << ".vs/tasks.vs.json";
        results.push_back(str.str());
        return false;
    }
    else
    {
        results.push_back(_ttc(strIdCreated) + ".vs/tasks.vs.json");
    }

    file.clear();
    file.ReadString(txtLaunch);

    file.at(file.FindLineContaining("%targetD%")).Replace("%targetD%", cSrcFiles.GetTargetDebug());

    if (!cSrcFiles.GetProjectName().empty())
        file.at(file.FindLineContaining("%project%")).Replace("%project%", cSrcFiles.GetProjectName());
    else
    {
        results.push_back(_tt(strIdMissingProjectName) + cSrcFiles.GetSrcFilesName());
        return false;
    }

    if (!file.WriteFile(".vs/launch.vs.json"))
    {
        std::ostringstream str;
        str << _tt(strIdCantWrite) << ".vs/launch.vs.json";
        results.push_back(str.str());
        return false;
    }
    else
    {
        results.push_back(_tt(strIdCreated));
    }

    return true;
}
