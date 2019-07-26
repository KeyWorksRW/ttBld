/////////////////////////////////////////////////////////////////////////////
// Name:      CDlgGetText
// Purpose:   IDDLG_XGETTEXT dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#if (defined(_WIN64) || defined(_WIN32)) && !defined(IDDLG_XGETTEXT)
    #include "resource.h"
#endif

#include <ttdlg.h>      // ttCDlg, ttCComboBox, ttCListBox, ttCListView
#include <ttlist.h>     // ttCList
#include <ttstr.h>      // ttCStr

class CDlgGetText : public ttCDlg
{
public:
    CDlgGetText() : ttCDlg(IDDLG_XGETTEXT) {
        m_bIndents = false;
        m_bNoHeaders = false;
    }

    // Public functions

    void ProcessCurOptions(const char* pszOptions);

protected:
    BEGIN_TTCMD_MAP()
        TTCASE_CMD(IDBTN_ADD, OnBtnAdd)
    END_TTMSG_MAP()

    // Message handlers

    void OnBtnAdd();
    void OnBegin(void);
    void OnOK(void);

    // Protected functions

    void DisplayFunctionList();

private:
    // Class members

    ttCListBox m_lbKeywords;
    ttCStr m_cszInputFile;
    ttCList m_lstKeywords;

    bool m_bIndents;
    bool m_bNoHeaders;
};
