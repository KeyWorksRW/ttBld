/////////////////////////////////////////////////////////////////////////////
// Name:      yamalize.cpp
// Purpose:   Used to convert .srcfiles.yaml to .vscode/srcfiles.yaml
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "writesrcfiles.h"    // CWriteSrcFiles

// This file will read a .srcfiles.yaml in the current directory and write a .vscode/srcfiles.yaml. If the .vscode directory does not exist, it will be created.
// The srcfiles.yaml is not designed to be tracked, so it's fine to customize it based on the current system (what compilers are available, what platform we're on, etc.).
// A lot of the default options which would not be normally displayed are listed to make them easy to modify in VS Code or any editor that understands YAML format

using namespace sfopt;

bool Yamalize()
{
    CSrcFiles cOrgSrcFiles(false);
    cOrgSrcFiles.ReadFile(".srcfiles.yaml");

    CWriteSrcFiles cNewSrcFiles(true);
    ttCList* plstFiles = cNewSrcFiles.GetSrcFilesList();


    if (ttFileExists(".srcfiles.yaml"))
        *plstFiles += ".include .srcfiles.yaml  # import all the filenames from ${workspaceRoot}/.srcfiles.yaml";
    else
        *plstFiles += "*.c*";      // TODO: this is a placeholder, need to be smarter about what wildcards to use

    cNewSrcFiles.UpdateOption(OPT_PROJECT, cOrgSrcFiles.GetOption(OPT_PROJECT));
    cNewSrcFiles.UpdateOption(OPT_EXE_TYPE, cOrgSrcFiles.GetOption(OPT_EXE_TYPE));
    cNewSrcFiles.UpdateOption(OPT_PCH, cOrgSrcFiles.GetOption(OPT_PCH));
    cNewSrcFiles.UpdateOption(OPT_PCH_CPP, cOrgSrcFiles.GetPchCpp());

    cNewSrcFiles.UpdateOption(OPT_OPTIMIZE, cOrgSrcFiles.GetOption(OPT_OPTIMIZE));
        cNewSrcFiles.SetRequired(OPT_OPTIMIZE);
    cNewSrcFiles.UpdateOption(OPT_WARN_LEVEL, cOrgSrcFiles.GetOption(OPT_WARN_LEVEL));
        cNewSrcFiles.SetRequired(OPT_WARN_LEVEL);

    cNewSrcFiles.UpdateOption(OPT_PERMISSIVE, cOrgSrcFiles.GetOption(OPT_PERMISSIVE));
    cNewSrcFiles.UpdateOption(OPT_STDCALL, cOrgSrcFiles.GetOption(OPT_STDCALL));

    cNewSrcFiles.UpdateOption(OPT_CFLAGS_CMN, cOrgSrcFiles.GetOption(OPT_CFLAGS_CMN));
    cNewSrcFiles.UpdateOption(OPT_CFLAGS_REL, cOrgSrcFiles.GetOption(OPT_CFLAGS_REL));
    cNewSrcFiles.UpdateOption(OPT_CFLAGS_DBG, cOrgSrcFiles.GetOption(OPT_CFLAGS_DBG));

    cNewSrcFiles.UpdateOption(OPT_CLANG_CMN, cOrgSrcFiles.GetOption(OPT_CLANG_CMN));
    cNewSrcFiles.UpdateOption(OPT_CLANG_REL, cOrgSrcFiles.GetOption(OPT_CLANG_REL));
    cNewSrcFiles.UpdateOption(OPT_CLANG_DBG, cOrgSrcFiles.GetOption(OPT_CLANG_DBG));

    cNewSrcFiles.UpdateOption(OPT_LINK_CMN, cOrgSrcFiles.GetOption(OPT_LINK_CMN));
    cNewSrcFiles.UpdateOption(OPT_LINK_REL, cOrgSrcFiles.GetOption(OPT_LINK_REL));
    cNewSrcFiles.UpdateOption(OPT_LINK_DBG, cOrgSrcFiles.GetOption(OPT_LINK_DBG));

#ifdef _WIN32
    cNewSrcFiles.UpdateOption(OPT_NATVIS, cOrgSrcFiles.GetOption(OPT_NATVIS));

    cNewSrcFiles.UpdateOption(OPT_RC_CMN, cOrgSrcFiles.GetOption(OPT_RC_CMN));
    cNewSrcFiles.UpdateOption(OPT_RC_REL, cOrgSrcFiles.GetOption(OPT_RC_REL));
    cNewSrcFiles.UpdateOption(OPT_RC_DBG, cOrgSrcFiles.GetOption(OPT_RC_DBG));

    cNewSrcFiles.UpdateOption(OPT_MDL_CMN, cOrgSrcFiles.GetOption(OPT_MDL_CMN));
    cNewSrcFiles.UpdateOption(OPT_MDL_REL, cOrgSrcFiles.GetOption(OPT_MDL_REL));
    cNewSrcFiles.UpdateOption(OPT_MDL_DBG, cOrgSrcFiles.GetOption(OPT_MDL_DBG));

    cNewSrcFiles.UpdateOption(OPT_DEBUG_RC, cOrgSrcFiles.GetOption(OPT_DEBUG_RC));

    cNewSrcFiles.UpdateOption(OPT_MS_LINKER, cOrgSrcFiles.GetOption(OPT_MS_LINKER));
    cNewSrcFiles.UpdateOption(OPT_MS_RC, cOrgSrcFiles.GetOption(OPT_MS_RC));
#endif

    // TODO: [KeyWorks - 7/24/2019] Hook up Crt once it gets changed

    cNewSrcFiles.UpdateOption(OPT_64BIT, cOrgSrcFiles.GetOption(OPT_64BIT));
    cNewSrcFiles.UpdateOption(OPT_TARGET_DIR64, cOrgSrcFiles.GetOption(OPT_TARGET_DIR64));
    cNewSrcFiles.UpdateOption(OPT_32BIT, cOrgSrcFiles.GetOption(OPT_32BIT));
    cNewSrcFiles.UpdateOption(OPT_TARGET_DIR32, cOrgSrcFiles.GetOption(OPT_TARGET_DIR32));

    cNewSrcFiles.UpdateOption(OPT_INC_DIRS, cOrgSrcFiles.GetOption(OPT_INC_DIRS));
    cNewSrcFiles.UpdateOption(OPT_BUILD_LIBS, cOrgSrcFiles.GetOption(OPT_BUILD_LIBS));
    cNewSrcFiles.UpdateOption(OPT_LIB_DIRS, cOrgSrcFiles.GetOption(OPT_LIB_DIRS));

    cNewSrcFiles.UpdateOption(OPT_LIBS, cOrgSrcFiles.GetOption(OPT_LIBS));
    cNewSrcFiles.UpdateOption(OPT_LIBS_REL, cOrgSrcFiles.GetOption(OPT_LIBS_REL));
    cNewSrcFiles.UpdateOption(OPT_LIBS_DBG, cOrgSrcFiles.GetOption(OPT_LIBS_DBG));

    cNewSrcFiles.UpdateOption(OPT_XGET_FLAGS, cOrgSrcFiles.GetOption(OPT_XGET_FLAGS));

    ttCStr cszVersion;
    cszVersion.printf(txtNinjaVerFormat, cNewSrcFiles.GetMajorRequired(), cNewSrcFiles.GetMinorRequired(), cNewSrcFiles.GetSubRequired());

    if (!cNewSrcFiles.WriteNew(".vscode/srcfiles.yaml", cszVersion))
    {
        ttMsgBoxFmt(GETSTRING(IDS_NINJA_CANT_WRITE), MB_OK | MB_ICONWARNING, ".vscode/srcfiles.yaml");
        return false;
    }

    return true;
}
