/////////////////////////////////////////////////////////////////////////////
// Name:      CVcxWrite
// Purpose:   Class creating a Visual Studio build script
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ttxml.h>  // ttCXMLBranch, ttCParseXML

#include "ninja.h"          // CNinja
#include "writesrc.h"  // CWriteSrcFiles

// Class for reading a vcxproj file and converting it into a .srcfiles.yaml file.
class CVcxRead
{
public:
    CVcxRead(ttCParseXML* pxml, CWriteSrcFiles* pcSrcFiles, ttCStr* pcszConvertScript);
    bool ConvertVcxProj();

protected:
    // Protected functions

    void ProcessCompiler(ttCXMLBranch* pSection, bool bDebug);
    void ProcessLink(ttCXMLBranch* pSection, bool bDebug);
    void ProcessMidl(ttCXMLBranch* pSection, bool bDebug);
    void ProcessRC(ttCXMLBranch* pSection, bool bDebug);

    void  ConvertScriptDir(const char* pszDir, ttlib::cstr& cszResult);
    char* MakeSrcRelative(const char* pszFile);

private:
    ttCParseXML*    m_pxml;
    CWriteSrcFiles* m_pcSrcFiles;

    ttCStr* m_pcszConvertScript;
    ttCStr  m_cszScriptRoot;
    ttCStr  m_cszOutRoot;
    ttCStr  m_cszRelative;  // Used to create a relative location for a source file
};

// Class creating a Visual Studio build script
class CVcxWrite : public CNinja
{
public:
    CVcxWrite(const char* pszNinjaDir = nullptr)
        : CNinja(pszNinjaDir)
    {
    }

    // Class functions

    bool CreateBuildFile();

private:
    // Class members
};
