/////////////////////////////////////////////////////////////////////////////
// Name:      CNinja
// Purpose:   Class for creating/maintaining build.ninja file for use by ninja.exe build tool
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "bldmaster.h"  // CBldMaster

class CNinja : public CBldMaster
{
public:
    CNinja(bool bVsCodeDir = false);

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

private:

    // Class members

    // The members below are reset every time CreateBuildFile() is called

    ttCFile* m_pkfOut;
    GEN_TYPE m_gentype;

    ttCStr m_cszPCH;            // the .pch name that will be generated
    ttCStr m_cszCPP_PCH;        // the .cpp name that will be used to create the .pch file
    ttCStr m_cszPCHObj;         // the .obj file that is built to create the .pch file
    ttCStr m_cszChmFile;        // set if a .hhp file was specified in .srcfiles

    ttCList m_lstBuildLibs32D;
    ttCList m_lstBuildLibs64D;
    ttCList m_lstBuildLibs32R;
    ttCList m_lstBuildLibs64R;

    bool    m_bClang;           // true if generating for CLANG compiler, false if generating for MSVC compiler
};
