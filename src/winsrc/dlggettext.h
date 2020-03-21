/////////////////////////////////////////////////////////////////////////////
// Name:      CDlgGetText
// Purpose:   IDDLG_XGETTEXT dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#if (defined(_WIN64) || defined(_WIN32)) && !defined(IDDLG_XGETTEXT)
    #include "resource.h"
#endif

#include <ttwindlg.h>  // Class for displaying a dialog

class CDlgGetText : public ttlib::dlg
{
public:
    CDlgGetText() : ttlib::dlg(IDDLG_XGETTEXT)
    {
        m_Indents = false;
        m_NoHeaders = false;
    }

    // Public functions

    void ProcessCurOptions(const char* pszOptions);

protected:
    BEGIN_TTCMD_MAP()
        TTCASE_CMD(IDBTN_ADD, OnBtnAdd)
    END_TTMSG_MAP()

    // Message handlers

    void OnBtnAdd();
    void OnBegin(void) override;
    void OnOK(void) override;

    // Protected functions

    void DisplayFunctionList();

private:
    // Class members

    ttlib::dlgListBox m_lbKeywords;
    ttlib::cstr m_cszInputFile;
    std::vector<std::string> m_lstKeywords;

    bool m_Indents;
    bool m_NoHeaders;
};
