/////////////////////////////////////////////////////////////////////////////
// Purpose:   Class for converting project build files to .srcfiles.yaml
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "tttextfile.h"  // textfile -- Classes for reading and writing line-oriented files

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

    bld::RESULT CreateCmakeProject(ttlib::cstr& projectFile);
    bld::RESULT WriteCmakeProject();
    bld::RESULT ConvertToCmakeProject(ttlib::cstr& projectFile);

    void DontCreateSrcFiles() { m_CreateSrcFiles = false; }

protected:
    void MakeNameRelative(ttlib::cstr& filename);

    void ProcessVcxDebug(pugi::xml_node node);
    void ProcessVcxRelease(pugi::xml_node node);

    void ProcessVcDebug(pugi::xml_node node);
    void ProcessVcRelease(pugi::xml_node node);

    // If .srcfiles.yaml is availalbe, this will add the Files: and DebugFiles: sections including their comments
    void CMakeAddFilesSection(ttlib::viewfile& in, ttlib::textfile& out, size_t file_pos);

    // Adds CSrcFiles::m_lstSrcFiles and CSrcFiles::m_lstDebugFiles with no comments
    void CMakeAddFiles(ttlib::textfile& out);

private:
    pugi::xml_document m_xmldoc;
    CWriteSrcFiles m_srcfiles;

    ttlib::cstr m_srcFile;
    ttlib::cstr m_dstFile;
    ttlib::cstr m_srcDir;
    ttlib::cstr m_dstDir;

    bool m_CreateSrcFiles { true };
    bool m_isConvertToCmake { false };
};
