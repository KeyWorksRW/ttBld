/////////////////////////////////////////////////////////////////////////////
// Name:      CNinja
// Purpose:   Class for creating/maintaining build.ninja file for use by ninja.exe build tool
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ttcstr.h>      // Classes for handling zero-terminated char strings.
#include <ttcvector.h>   // Vector of ttlib::cstr strings
#include <tttextfile.h>  // Classes for reading and writing line-oriented files

#include "csrcfiles.h"  // CSrcFiles
#include "dryrun.h"     // CDryRun

// Class for creating/maintaining build.ninja file for use by ninja.exe build tool
class CNinja : public CSrcFiles
{
public:
    CNinja(std::string_view NinjaDir = ttlib::emptystring);

    enum GEN_TYPE : size_t
    {
        GEN_NONE,
        GEN_DEBUG,
        GEN_RELEASE
    };

    enum CMPLR_TYPE : size_t
    {
        // These MUST match the array of strings aszCompilerPrefix[] in ninja.cpp
        CMPLR_MSVC = 0,
        CMPLR_CLANG = 1,
        CMPLR_GCC = 2,
    };

    // Public functions

    void ProcessBuildLibs();

    // Warning: this will first clear m_ninjafile.
    bool CreateBuildFile(GEN_TYPE gentype, CMPLR_TYPE cmplr);
    bool CreateHelpFile();

    const ttlib::cstrVector& getErrorMsgs() { return m_lstErrMessages; }

    size_t getSrcCount() { return m_lstSrcFiles.size(); }

    bool CreateMakeFile(bool bAllVersion = false, std::string_view Dir = std::string_view{});

    const ttlib::cstr& GetRcFile() { return m_RCname; }
    std::string_view GetScriptFile() { return m_scriptFilename; }

    ttlib::cstrVector& GetSrcFileList() { return m_lstSrcFiles; }
    ttlib::cstrVector* GetLibFileList() { return &m_lstLibFiles; }

    // Returns false if .srcfiles.yaml requires a newer version
    bool IsValidVersion() { return m_isInvalidVersion != true; }

    // Name and location of any additional library to build
    ttlib::cview GetLibName() { return m_LIBname; }
    ttlib::cview GetHHPName() { return m_HPPname; }

    void EnableDryRun() { m_dryrun.Enable(); }
    void ForceWrite(bool bForceWrite = true) { m_isWriteIfNoChange = bForceWrite; }

protected:
    // Protected functions

#if defined(_WIN32)
    void msvcWriteCompilerComments(CMPLR_TYPE cmplr);
    void msvcWriteCompilerFlags(CMPLR_TYPE cmplr);
    void msvcWriteCompilerDirectives(CMPLR_TYPE cmplr);
    void msvcWriteRcDirective(CMPLR_TYPE cmplr);
    void msvcWriteMidlDirective(CMPLR_TYPE cmplr);
    void msvcWriteLibDirective(CMPLR_TYPE cmplr);
    void msvcWriteLinkDirective(CMPLR_TYPE cmplr);

    void msvcWriteLinkTargets(CMPLR_TYPE cmplr);
    void msvcWriteMidlTargets(CMPLR_TYPE cmplr);
#endif

    bool FindRcDependencies(std::string_view rcfile, std::string_view header = {}, std::string_view relpath = {});

    CDryRun m_dryrun;

private:
    // Class members

    ttlib::textfile m_ninjafile;
    GEN_TYPE m_gentype;

    ttlib::cstr m_pchHdrName;     // The .pch name that will be generated
    ttlib::cstr m_pchCppName;     // The .cpp name that will be used to create the .pch file
    ttlib::cstr m_pchHdrNameObj;  // The .obj file that is built to create the .pch file
    ttlib::cstr m_chmFilename;    // Set if a .hhp file was specified in .srcfiles.yaml

    ttlib::cstr m_scriptFilename;  // The .ninja file

    ttlib::cstrVector m_RcDependencies;
    ttlib::cstrVector m_BldLibsDbg;
    ttlib::cstrVector m_lstBldLibsRel;

    // ttCDblList m_dlstTargetDir;  // Target name, directory to use

    bool m_isWriteIfNoChange { false };  // Write the ninja file even if it hasn't changed
    bool m_isInvalidVersion { false };   // True if a newer version is needed to parse the .srcfiles.yaml
};
