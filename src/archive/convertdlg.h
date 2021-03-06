/////////////////////////////////////////////////////////////////////////////
// Name:      CConvertDlg
// Purpose:   IDDDLG_CONVERT dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDDDLG_CONVERT
    #include "resource.h"
#endif

#include <ttcwd.h>     // cwd -- Class for storing and optionally restoring the current directory
#include <ttwindlg.h>  // Class for displaying a dialog

#include "writesrc.h"  // CWriteSrcFiles -- Writes a new or update srcfiles.yaml file

class CConvertDlg : public ttlib::dlg
{
public:
    CConvertDlg();
    CConvertDlg(std::string_view projectFile);

    const ttlib::cstr& GetOutSrcFiles() { return m_cszOutSrcFiles; }
    const ttlib::cstr& GetDirSrcFiles() { return m_cszDirSrcFiles; }
    const ttlib::cstr& GetConvertScript() { return m_ConvertFile; }
    void SetConvertScritpt(std::string_view filename);

    bool isCreateVsCode() { return m_CreateVscode; }
    bool isAddToGitExclude() { return m_AddToGitExclude; }

protected:
    BEGIN_TTCMD_MAP()
        TTCASE_CMD(IDBTN_CHANGE_IN, OnBtnChangeIn)
        TTCASE_CMD(IDBTN_CHANGE_OUT, OnBtnChangeOut)
        TTCASE_CMD(IDBTN_LOCATE_SCRIPT, OnBtnLocateScript)
        TTCASE_CMD(IDRADIO_CONVERT, OnCheckConvert)
        TTCASE_CMD(IDRADIO_FILES, OnCheckFiles)
    END_TTMSG_MAP()

    void OnCheckFiles();
    void OnCheckConvert();
    void OnBtnChangeIn();
    void OnBtnChangeOut();
    void OnBtnLocateScript();

    void OnBegin(void) override;
    void OnOK(void) override;

    bool ConvertSrcfiles();

    // BUGBUG: [KeyWorks - 03-28-2020] This should return bld:RESULT
    bool doConversion();

private:
    ttlib::dlgCombo m_comboScripts;

    CWriteSrcFiles m_cSrcFiles;

    ttlib::cstr m_cszOutSrcFiles;  // Where .srcfiles should be created

    ttlib::cstr m_cszDirSrcFiles;
    ttlib::cstr m_ConvertFile;

    ttlib::cstr m_cszScriptRoot;
    ttlib::cstr m_cszOutRoot;
    ttlib::cstr m_cszRelative;  // Used to create a relative location for a source file

    ttlib::cwd m_cwd { true };

    bool m_CreateVscode { false };
    bool m_AddToGitExclude { false };
};
