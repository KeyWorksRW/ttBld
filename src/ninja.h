/////////////////////////////////////////////////////////////////////////////
// Name:      CNinja
// Purpose:   Class for creating/maintaining build.ninja file for use by ninja.exe build tool
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ttstring.h>  // ttString, ttStrlist -- String and vector classes with some additional functionality
#include <ttlist.h>    // ttCList, ttCDblList, ttCStrIntList

#include "csrcfiles.h"  // CSrcFiles
#include "dryrun.h"     // CDryRun

// Class for creating/maintaining build.ninja file for use by ninja.exe build tool
class CNinja : public CSrcFiles
{
public:
    CNinja(const char* pszNinjaDir = nullptr);

    typedef enum
    {
        GEN_NONE,
        GEN_DEBUG,
        GEN_RELEASE
    } GEN_TYPE;

    typedef enum
    {
        // clang-format off
        // These MUST match the array of strings aszCompilerPrefix[] in ninja.cpp
        CMPLR_MSVC     = 0,
        CMPLR_CLANG    = 1,
        CMPLR_GCC      = 2,
        // clang-format on
    } CMPLR_TYPE;

    // Public functions

    void ProcessBuildLibs();
    bool CreateBuildFile(GEN_TYPE gentype, CMPLR_TYPE cmplr);
    bool CreateHelpFile();

    size_t      GetErrorCount() { return m_lstErrMessages.size(); }
    const char* GetError(size_t pos) { return m_lstErrMessages[pos].c_str(); }

    size_t getSrcCount() { return m_lstSrcFiles.size(); }

    bool CreateMakeFile(bool bAllVersion = false, std::string_view Dir = ttEmptyString);

    const char*      GetRcFile() { return m_RCname.c_str(); }
    std::string_view GetScriptFile() { return m_cszScriptFile; }

    ttStrVector& GetSrcFileList() { return m_lstSrcFiles; }
    ttStrVector* GetLibFileList() { return &m_lstLibFiles; }
    ttCList*     GetRcDepList() { return &m_lstRcDependencies; }

    // Returns false if .srcfiles.yaml requires a newer version
    bool IsValidVersion() { return m_bInvalidVersion != true; }

    // Name and location of any additional library to build
    const char* GetLibName() { return m_LIBname.c_str(); }
    const char* GetHHPName() { return m_HPPname.c_str(); }

    void EnableDryRun() { m_dryrun.Enable(); }
    void ForceWrite(bool bForceWrite = true) { m_bForceWrite = bForceWrite; }

protected:
    // Protected functions

    void AddDependentLibrary(const char* pszLib, GEN_TYPE gentype);

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

    bool FindRcDependencies(const char* pszSrc, const char* pszHdr = nullptr, const char* pszRelPath = nullptr);
    const char* NormalizeHeader(const char* pszBaseFile, ttCStr& cszHeader);

    CDryRun m_dryrun;
    ttCList m_lstRcDependencies;

private:
    // Class members

    ttCFile* m_pkfOut;
    GEN_TYPE m_gentype;

    ttCStr m_cszPCH;      // The .pch name that will be generated
    ttCStr m_cszCPP_PCH;  // The .cpp name that will be used to create the .pch file
    ttCStr m_cszPCHObj;   // The .obj file that is built to create the .pch file
    ttCStr m_cszChmFile;  // Set if a .hhp file was specified in .srcfiles.yaml

    ttString m_cszScriptFile;  // The .ninja file

    ttCList m_lstBldLibsD;
    ttCList m_lstBldLibsR;

    ttCDblList m_dlstTargetDir;  // Target name, directory to use

    bool m_bForceWrite;      // Write the ninja file even if it hasn't changed
    bool m_bInvalidVersion;  // True if a newer version is needed to parse the .srcfiles.yaml
};
