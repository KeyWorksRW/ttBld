/////////////////////////////////////////////////////////////////////////////
// Name:      CVcxRead, CVcxWrite
// Purpose:   Classes for converting/from a Visual Studio build script
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ninja.h"      // CNinja
#include "pugixml.hpp"  // pugixml parser
#include "writesrc.h"   // CWriteSrcFiles -- Writes a new or update srcfiles.yaml file

// Class for reading a vcxproj file and converting it into a .srcfiles.yaml file.
class CVcxRead
{
public:
    CVcxRead() {}

    bld::RESULT Convert(const std::string& srcFile, std::string_view dstFile);

protected:
    void MakeNameRelative(ttlib::cstr& filename);

    // Process the Debug section of ItemDefinitionGroup
    void ProcessDebug(pugi::xml_node node);
    void ProcessRelease(pugi::xml_node node);

private:
    pugi::xml_document m_xmldoc;
    CWriteSrcFiles m_writefile;

    ttlib::cstr m_srcFile;
    ttlib::cstr m_dstFile;
    ttlib::cstr m_srcDir;
    ttlib::cstr m_dstDir;
};

// Class creating a Visual Studio build script
class CVcxWrite : public CNinja
{
public:
    CVcxWrite(const char* pszNinjaDir = nullptr) : CNinja(pszNinjaDir) {}

    bool CreateBuildFile();

private:
};
