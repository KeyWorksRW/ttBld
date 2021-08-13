/////////////////////////////////////////////////////////////////////////////
// Purpose:   Used to convert .srcfiles.yaml to .vscode/srcfiles.yaml
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "uifuncs.h"   // Miscellaneous functions for displaying UI
#include "writesrc.h"  // CWriteSrcFiles -- Writes a new or update srcfiles.yaml file

// This file will read a .srcfiles.yaml in the current directory and write a .vscode/srcfiles.yaml. If the .vscode directory
// does not exist, it will be created. The srcfiles.yaml is not designed to be tracked, so it's fine to customize it based on
// the current system (what compilers are available, what platform we're on, etc.). A lot of the default options which would
// not be normally displayed are listed to make them easy to modify in VS Code or any editor that understands YAML format

bool Yamalize()
{
    CSrcFiles cOrgSrcFiles;
    cOrgSrcFiles.ReadFile(".srcfiles.yaml");

    CWriteSrcFiles cNewSrcFiles;
    auto& lstSrcFiles = cNewSrcFiles.GetSrcFileList();

    if (ttlib::file_exists(".srcfiles.yaml"))
        lstSrcFiles.emplace_back(".include .srcfiles.yaml  # import all the filenames from ${workspaceRoot}/.srcfiles.yaml");
    else
    {
        // TODO: this is a placeholder, need to be smarter about what wildcards to use
        lstSrcFiles.emplace_back("*.c*");
    }

    cNewSrcFiles.setOptValue(OPT::PROJECT, cOrgSrcFiles.getOptValue(OPT::PROJECT));
    cNewSrcFiles.setOptValue(OPT::EXE_TYPE, cOrgSrcFiles.getOptValue(OPT::EXE_TYPE));
    cNewSrcFiles.setOptValue(OPT::PCH, cOrgSrcFiles.getOptValue(OPT::PCH));
    cNewSrcFiles.setOptValue(OPT::PCH_CPP, cOrgSrcFiles.GetPchCpp());

    cNewSrcFiles.setOptValue(OPT::OPTIMIZE, cOrgSrcFiles.getOptValue(OPT::OPTIMIZE));
    cNewSrcFiles.SetRequired(OPT::OPTIMIZE);
    cNewSrcFiles.setOptValue(OPT::WARN, cOrgSrcFiles.getOptValue(OPT::WARN));
    cNewSrcFiles.SetRequired(OPT::WARN);

    cNewSrcFiles.setOptValue(OPT::CFLAGS_CMN, cOrgSrcFiles.getOptValue(OPT::CFLAGS_CMN));
    cNewSrcFiles.setOptValue(OPT::CFLAGS_REL, cOrgSrcFiles.getOptValue(OPT::CFLAGS_REL));
    cNewSrcFiles.setOptValue(OPT::CFLAGS_DBG, cOrgSrcFiles.getOptValue(OPT::CFLAGS_DBG));

    cNewSrcFiles.setOptValue(OPT::CLANG_CMN, cOrgSrcFiles.getOptValue(OPT::CLANG_CMN));
    cNewSrcFiles.setOptValue(OPT::CLANG_REL, cOrgSrcFiles.getOptValue(OPT::CLANG_REL));
    cNewSrcFiles.setOptValue(OPT::CLANG_DBG, cOrgSrcFiles.getOptValue(OPT::CLANG_DBG));

    cNewSrcFiles.setOptValue(OPT::MSVC_CMN, cOrgSrcFiles.getOptValue(OPT::MSVC_CMN));
    cNewSrcFiles.setOptValue(OPT::MSVC_REL, cOrgSrcFiles.getOptValue(OPT::MSVC_REL));
    cNewSrcFiles.setOptValue(OPT::MSVC_DBG, cOrgSrcFiles.getOptValue(OPT::MSVC_DBG));

    cNewSrcFiles.setOptValue(OPT::LINK_CMN, cOrgSrcFiles.getOptValue(OPT::LINK_CMN));
    cNewSrcFiles.setOptValue(OPT::LINK_REL, cOrgSrcFiles.getOptValue(OPT::LINK_REL));
    cNewSrcFiles.setOptValue(OPT::LINK_DBG, cOrgSrcFiles.getOptValue(OPT::LINK_DBG));

#ifdef _WIN32
    cNewSrcFiles.setOptValue(OPT::NATVIS, cOrgSrcFiles.getOptValue(OPT::NATVIS));

    cNewSrcFiles.setOptValue(OPT::RC_CMN, cOrgSrcFiles.getOptValue(OPT::RC_CMN));
    cNewSrcFiles.setOptValue(OPT::RC_REL, cOrgSrcFiles.getOptValue(OPT::RC_REL));
    cNewSrcFiles.setOptValue(OPT::RC_DBG, cOrgSrcFiles.getOptValue(OPT::RC_DBG));

    cNewSrcFiles.setOptValue(OPT::MIDL_CMN, cOrgSrcFiles.getOptValue(OPT::MIDL_CMN));
    cNewSrcFiles.setOptValue(OPT::MIDL_REL, cOrgSrcFiles.getOptValue(OPT::MIDL_REL));
    cNewSrcFiles.setOptValue(OPT::MIDL_DBG, cOrgSrcFiles.getOptValue(OPT::MIDL_DBG));

    cNewSrcFiles.setOptValue(OPT::RC_DBG, cOrgSrcFiles.getOptValue(OPT::RC_DBG));

    cNewSrcFiles.setOptValue(OPT::MS_LINKER, cOrgSrcFiles.getOptValue(OPT::MS_LINKER));
    cNewSrcFiles.setOptValue(OPT::MS_RC, cOrgSrcFiles.getOptValue(OPT::MS_RC));
#endif

    // TODO: [KeyWorks - 7/24/2019] Hook up Crt once it gets changed

    cNewSrcFiles.setOptValue(OPT::BIT64, cOrgSrcFiles.getOptValue(OPT::BIT64));
    cNewSrcFiles.setOptValue(OPT::TARGET_DIR, cOrgSrcFiles.getOptValue(OPT::TARGET_DIR));
    cNewSrcFiles.setOptValue(OPT::BIT32, cOrgSrcFiles.getOptValue(OPT::BIT32));
    cNewSrcFiles.setOptValue(OPT::TARGET_DIR32, cOrgSrcFiles.getOptValue(OPT::TARGET_DIR32));

    cNewSrcFiles.setOptValue(OPT::INC_DIRS, cOrgSrcFiles.getOptValue(OPT::INC_DIRS));
    cNewSrcFiles.setOptValue(OPT::BUILD_LIBS, cOrgSrcFiles.getOptValue(OPT::BUILD_LIBS));
    cNewSrcFiles.setOptValue(OPT::LIB_DIRS32, cOrgSrcFiles.getOptValue(OPT::LIB_DIRS32));
    cNewSrcFiles.setOptValue(OPT::LIB_DIRS, cOrgSrcFiles.getOptValue(OPT::LIB_DIRS));

    cNewSrcFiles.setOptValue(OPT::LIBS_CMN, cOrgSrcFiles.getOptValue(OPT::LIBS_CMN));
    cNewSrcFiles.setOptValue(OPT::LIBS_REL, cOrgSrcFiles.getOptValue(OPT::LIBS_REL));
    cNewSrcFiles.setOptValue(OPT::LIBS_DBG, cOrgSrcFiles.getOptValue(OPT::LIBS_DBG));

    cNewSrcFiles.setOptValue(OPT::XGET_FLAGS, cOrgSrcFiles.getOptValue(OPT::XGET_FLAGS));

    ttlib::cstr cszVersion;
    cszVersion.Format(txtNinjaVerFormat, cNewSrcFiles.GetMajorRequired(), cNewSrcFiles.GetMinorRequired(),
                      cNewSrcFiles.GetSubRequired());

    if (cNewSrcFiles.WriteNew(".vscode/srcfiles.yaml", cszVersion) != bld::success)
    {
        appMsgBox(ttlib::cstr("Unable to create or write to ") + ".vscode/srcfiles.yaml");
        return false;
    }

    return true;
}
