/////////////////////////////////////////////////////////////////////////////
// Purpose:   Class for converting project build files to .srcfiles.yaml
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../pugixml/pugixml.hpp"  // pugixml parser

#include "writesrc.h"  // CWriteSrcFiles -- Writes a new or update srcfiles.yaml file

class CConvert
{
public:
    CConvert() { m_srcfiles.InitOptions(); }

    bld::RESULT ConvertVc(const std::string& srcFile, std::string_view dstFile);
    bld::RESULT ConvertVcx(const std::string& srcFile, std::string_view dstFile);
    bld::RESULT ConvertDsp(const std::string& srcFile, std::string_view dstFile);
    bld::RESULT ConvertCodeLite(const std::string& srcFile, std::string_view dstFile);
    bld::RESULT ConvertSrcfiles(const std::string& srcFile, std::string_view dstFile);

protected:
    void MakeNameRelative(ttlib::cstr& filename);

    void ProcessVcxDebug(pugi::xml_node node);
    void ProcessVcxRelease(pugi::xml_node node);

    void ProcessVcDebug(pugi::xml_node node);
    void ProcessVcRelease(pugi::xml_node node);

private:
    pugi::xml_document m_xmldoc;
    CWriteSrcFiles m_srcfiles;

    ttlib::cstr m_srcFile;
    ttlib::cstr m_dstFile;
    ttlib::cstr m_srcDir;
    ttlib::cstr m_dstDir;
};
