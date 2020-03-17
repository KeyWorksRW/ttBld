/////////////////////////////////////////////////////////////////////////////
// Name:      CTabCompiler
// Purpose:   IDTAB_COMPILER dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDTAB_COMPILER
    #include "resource.h"
#endif

#include <ttwindlg.h>  // Class for displaying a dialog

class CTabOptions;

class CTabCompiler : public ttlib::dlg
{
public:
    CTabCompiler() : ttlib::dlg(IDTAB_COMPILER) {}

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

    void OnBegin(void) override;
    void OnOK(void) override;

private:
    // Class members

    CTabOptions* m_pOpts;
};
