/////////////////////////////////////////////////////////////////////////////
// Name:      CTabLinker
// Purpose:   IDTAB_LINKER dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDTAB_LINKER
#include "resource.h"
#endif

#include <ttdlg.h>  // ttCDlg, ttCComboBox, ttCListBox, ttCListView

class CTabOptions;

class CTabLinker : public ttCDlg
{
public:
    CTabLinker()
        : ttCDlg(IDTAB_LINKER)
    {
    }

    // Class functions

    void SetParentClass(CTabOptions* pclsOptions) { m_pOpts = pclsOptions; }

protected:
    BEGIN_TTCMD_MAP()
        TTCASE_CMD(IDBTN_CHANGE, OnBtnChange)
        TTCASE_CMD(IDBTN_ADD_LIBDIR, OnBtnLibDir)
        TTCASE_CMD(IDBTN_ADD_LIB, OnBtnAddLib)
    END_TTMSG_MAP()

    // Message handlers

    void OnBtnChange();
    void OnBtnLibDir();
    void OnBtnAddLib();

    void OnBegin(void);
    void OnOK(void);

private:
    // Class members

    CTabOptions* m_pOpts;
};
