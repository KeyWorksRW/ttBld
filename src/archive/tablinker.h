/////////////////////////////////////////////////////////////////////////////
// Name:      CTabLinker
// Purpose:   IDTAB_LINKER dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDTAB_LINKER
    #include "resource.h"
#endif

#include <ttwindlg.h>  // Class for displaying a dialog

class CTabOptions;

class CTabLinker : public ttlib::dlg
{
public:
    CTabLinker() : ttlib::dlg(IDTAB_LINKER) {}

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

    void OnBegin(void) override;
    void OnOK(void) override;

private:
    // Class members

    CTabOptions* m_pOpts;
};
