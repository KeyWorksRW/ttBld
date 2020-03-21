/////////////////////////////////////////////////////////////////////////////
// Name:      CConvertDlg
// Purpose:   IDDDLG_CONVERT dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDDDLG_CONVERT
    #include "resource.h"
#endif

#include <ttcwd.h>     // cwd -- Class for storing and optionally restoring the current directory
#include <ttwindlg.h>  // Class for displaying a dialog
#include <ttxml.h>     // ttCXMLBranch, ttCParseXML

#include "writesrc.h"  // CWriteSrcFiles -- Writes a new or update srcfiles.yaml file

class CConvertDlg : public ttlib::dlg
{
public:
    CConvertDlg(const char* pszDstSrcFiles = nullptr);

    // Public functions

    const ttlib::cstr& GetOutSrcFiles() { return m_cszOutSrcFiles; }
    const ttlib::cstr& GetDirSrcFiles() { return m_cszDirSrcFiles; }
    const ttlib::cstr& GetConvertScript() { return m_cszConvertScript; }
    void SetConvertScritpt(std::string_view filename) { m_cszConvertScript = filename; }

    bool isCreateVsCode() { return m_bCreateVsCode; }
    bool isGitIgnoreAll() { return m_bGitIgnore; }

protected:
    BEGIN_TTCMD_MAP()
        TTCASE_CMD(IDBTN_CHANGE_IN, OnBtnChangeIn)
        TTCASE_CMD(IDBTN_CHANGE_OUT, OnBtnChangeOut)
        TTCASE_CMD(IDBTN_LOCATE_SCRIPT, OnBtnLocateScript)
    END_TTMSG_MAP()

    // Message handlers

    void OnBtnChangeIn();
    void OnBtnChangeOut();
    void OnBtnLocateScript();
    void OnBegin(void) override;
    void OnOK(void) override;

    // Protected functions

    bool ConvertDsp();
    bool ConvertCodeLite();
    bool ConvertCodeBlocks();
    bool ConvertSrcfiles();
    bool ConvertVcxProj();
    bool ConvertVcProj();

    bool doConversion(const char* pszInFile = nullptr);

    char* MakeSrcRelative(const char* pszFile);
    void AddCodeLiteFiles(ttCXMLBranch* pParent);
    bool isValidSrcFile(const char* pszFile) const;

private:
    // Class members

    ttlib::dlgCombo m_comboScripts;
    ttCParseXML m_xml;
    CWriteSrcFiles m_cSrcFiles;

    ttlib::cstr m_cszOutSrcFiles;  // Where .srcfiles should be created

    ttlib::cstr m_cszDirSrcFiles;
    ttlib::cstr m_cszConvertScript;

    ttlib::cstr m_cszScriptRoot;
    ttlib::cstr m_cszOutRoot;
    ttlib::cstr m_cszRelative;  // Used to create a relative location for a source file

    ttlib::cwd m_cszCWD { true };

    bool m_bCreateVsCode;
    bool m_bGitIgnore;
};
