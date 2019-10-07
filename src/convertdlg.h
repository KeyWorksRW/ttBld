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

#include <ttdlg.h>  // ttCDlg, ttCComboBox, ttCListBox, ttCListView
#include <ttxml.h>  // ttCXMLBranch, ttCParseXML

#include "writesrcfiles.h"  // CWriteSrcFiles

class CConvertDlg : public ttCDlg
{
public:
    CConvertDlg(const char* pszDstSrcFiles = nullptr);

    // Public functions

    char* GetOutSrcFiles() { return m_cszOutSrcFiles; }
    char* GetDirSrcFiles() { return m_cszDirSrcFiles; }
    char* GetConvertScript() { return m_cszConvertScript; }
    void  SetConvertScritpt(const char* pszFile) { m_cszConvertScript = pszFile; }

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
    void OnBegin(void);
    void OnOK(void);

    // Protected functions

    bool ConvertDsp();
    bool ConvertCodeLite();
    bool ConvertCodeBlocks();
    bool ConvertSrcfiles();
    bool ConvertVcxProj();
    bool ConvertVcProj();

    bool doConversion(const char* pszInFile = nullptr);

    char* MakeSrcRelative(const char* pszFile);
    void  AddCodeLiteFiles(ttCXMLBranch* pParent);
    bool  isValidSrcFile(const char* pszFile) const;

private:
    // Class members

    ttCComboBox    m_comboScripts;
    ttCParseXML    m_xml;
    CWriteSrcFiles m_cSrcFiles;

    ttCStr m_cszOutSrcFiles;  // Where .srcfiles should be created

    ttCStr m_cszDirSrcFiles;
    ttCStr m_cszConvertScript;

    ttCStr m_cszScriptRoot;
    ttCStr m_cszOutRoot;
    ttCStr m_cszRelative;  // Used to create a relative location for a source file

    ttCStr m_cszCWD;

    bool m_bCreateVsCode;
    bool m_bGitIgnore;
};
