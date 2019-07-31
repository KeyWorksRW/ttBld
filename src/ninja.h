/////////////////////////////////////////////////////////////////////////////
// Name:      CNinja
// Purpose:   Class for creating/maintaining build.ninja file for use by ninja.exe build tool
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ttlist.h>                     // ttCList, ttCDblList, ttCStrIntList

#include "csrcfiles.h"      // CSrcFiles
#include "dryrun.h"         // CDryRun

class CNinja : public CSrcFiles
{
public:
    CNinja(const char* pszNinjaDir = nullptr);

    typedef enum {
        GEN_NONE,
        GEN_DEBUG32,
        GEN_RELEASE32,
        GEN_DEBUG64,
        GEN_RELEASE64
    } GEN_TYPE;

    // Public functions

    void ProcessBuildLibs();
    bool CreateBuildFile(GEN_TYPE gentype, bool bClang = true);
    bool CreateHelpFile();

    size_t      GetErrorCount() { return m_lstErrors.GetCount(); }
    const char* GetError(size_t pos) { return m_lstErrors[pos]; }
    void        AddError(const char* pszErrMsg) { m_lstErrors += pszErrMsg; }

    size_t getSrcCount() { return m_lstSrcFiles.GetCount(); }

    bool CreateMakeFile(bool bAllVersion = false, const char* pszDir = nullptr);

    const char* GetRcFile()     { return m_cszRcName; }
    const char* GetScriptFile() { return m_cszScriptFile; }

    ttCList* GetSrcFileList()  { return &m_lstSrcFiles; }
    ttCList* GetLibFileList()  { return &m_lstLibFiles; }
    ttCList* GetRcDepList()     { return &m_lstRcDependencies; }

    bool IsValidVersion() { return m_bInvalidVersion != true; }  // returns false if .srcfiles.yaml requires a newer version
    bool IsBin64()        { return m_bBin64Exists; }

    const char* GetLibName() { return m_cszLibName; }        // name and location of any additional library to build
    const char* GetHHPName() { return m_cszHHPName; }

    void EnableDryRun() { m_dryrun.Enable(); }
    void ForceWrite(bool bForceWrite = true) { m_bForceWrite = bForceWrite; }

protected:
    // Protected functions

    void GetLibName(const char* pszBaseName, ttCStr& cszLibName);
    void AddDependentLibrary(const char* pszLib, GEN_TYPE gentype);

    void WriteCompilerComments();
    void WriteCompilerDirectives();
    void WriteCompilerFlags();
    void WriteLibDirective();
    void WriteLinkDirective();
    void WriteMidlDirective(GEN_TYPE gentype);
    void WriteRcDirective();

    void WriteLinkTargets(GEN_TYPE gentype);
    void WriteMidlTargets();

    bool FindRcDependencies(const char* pszSrc, const char* pszHdr = nullptr, const char* pszRelPath = nullptr);
    const char* NormalizeHeader(const char* pszBaseFile, ttCStr& cszHeader);

    CDryRun m_dryrun;
    ttCList m_lstRcDependencies;

private:

    // Class members

    ttCFile* m_pkfOut;
    GEN_TYPE m_gentype;

    ttCStr m_cszPCH;            // the .pch name that will be generated
    ttCStr m_cszCPP_PCH;        // the .cpp name that will be used to create the .pch file
    ttCStr m_cszPCHObj;         // the .obj file that is built to create the .pch file
    ttCStr m_cszChmFile;        // set if a .hhp file was specified in .srcfiles.yaml
    ttCStr m_cszScriptFile;     // the .ninja file

    ttCList m_lstBuildLibs32D;
    ttCList m_lstBuildLibs64D;
    ttCList m_lstBuildLibs32R;
    ttCList m_lstBuildLibs64R;

    ttCDblList m_dlstTargetDir; // target name, directory to use

    bool m_bClang;              // true if generating for CLANG compiler, false if generating for MSVC compiler
    bool m_bBin64Exists;        // if true, the directory ../bin64 exists
    bool m_bForceWrite;         // write the ninja file even if it hasn't changed
    bool m_bInvalidVersion;     // true if a newer version is needed to parse the .srcfiles.yaml
};
