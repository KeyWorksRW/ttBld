/////////////////////////////////////////////////////////////////////////////
// Name:      CConvert
// Purpose:   Class for converting project build files to .srcfiles.yaml
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ttcstr.h>  // cstr -- Classes for handling zero-terminated char strings.

// ttcview.h MUST be included before pugixml.hpp
#include <ttcview.h> // cview -- string_view functionality on a zero-terminated char string.

#include "../pugixml/pugixml.hpp"  // pugixml parser

#include "writesrc.h"   // CWriteSrcFiles -- Writes a new or update srcfiles.yaml file

class CConvert
{
public:
    CConvert() { m_writefile.InitOptions(); }

    bld::RESULT ConvertVc(const std::string& srcFile, std::string_view dstFile);
    bld::RESULT ConvertVcx(const std::string& srcFile, std::string_view dstFile);
    bld::RESULT ConvertDsp(const std::string& srcFile, std::string_view dstFile);

protected:
    void MakeNameRelative(ttlib::cstr& filename);

    void ProcessVcxDebug(pugi::xml_node node);
    void ProcessVcxRelease(pugi::xml_node node);

    void ProcessVcDebug(pugi::xml_node node);
    void ProcessVcRelease(pugi::xml_node node);

private:
    pugi::xml_document m_xmldoc;
    CWriteSrcFiles m_writefile;

    ttlib::cstr m_srcFile;
    ttlib::cstr m_dstFile;
    ttlib::cstr m_srcDir;
    ttlib::cstr m_dstDir;
};
