/////////////////////////////////////////////////////////////////////////////
// Name:      CTabCompiler
// Purpose:   IDTAB_COMPILER dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDTAB_COMPILER
    #include "resource.h"
#endif

#include <ttdlg.h>  // ttCDlg, ttCComboBox, ttCListBox, ttCListView

class CTabOptions;

class CTabCompiler : public ttCDlg
{
public:
    CTabCompiler()
        : ttCDlg(IDTAB_COMPILER)
    {
    }

    // Public functions

    void SetParentClass(CTabOptions* pclsOptions) { m_pOpts = pclsOptions; }

protected:
    BEGIN_TTCMD_MAP()
        TTCASE_CMD(IDBTN_STD, OnBtnStd)
        TTCASE_CMD(IDBTN_CPLUSPLUS, OnBtnCplusplus)
        //        TTCASE_CMD(IDBTN_INC_ADD, OnBtnAddInclude)
        TTCASE_CMD(IDBTN_PCH_CPP, OnBtnPchCpp)
        TTCASE_CMD(IDBTN_PCH, OnBtnChangePch)
    END_TTMSG_MAP()

    // Message handlers

    void OnBtnStd();
    void OnBtnCplusplus();
    //    void OnBtnAddInclude();
    void OnBtnPchCpp();
    void OnBtnChangePch();

    void OnBegin(void);
    void OnOK(void);

private:
    // Class members

    CTabOptions* m_pOpts;
};