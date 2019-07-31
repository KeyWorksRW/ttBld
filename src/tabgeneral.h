/////////////////////////////////////////////////////////////////////////////
// Name:      CTabGeneral
// Purpose:   IDTAB_GENERAL dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDTAB_GENERAL
    #include "resource.h"
#endif

#include <ttdlg.h>      // ttCDlg, ttCComboBox, ttCListBox, ttCListView

class CTabOptions;

class CTabGeneral : public ttCDlg
{
public:
    CTabGeneral() : ttCDlg(IDTAB_GENERAL) { }

    // Class functions

    void SetTargetDirs();
    void SetParentClass(CTabOptions* pclsOptions) { m_pOpts = pclsOptions; }

protected:
    BEGIN_TTCMD_MAP()
        TTCASE_CMD(IDRADIO_CONSOLE, OnCheckExe)
        TTCASE_CMD(IDRADIO_DLL, OnCheckExe)
        TTCASE_CMD(IDRADIO_NORMAL, OnCheckExe)
        TTCASE_CMD(IDRADIO_LIB, OnCheckLib)
        TTCASE_CMD(IDBTN_DIR64, OnBtnDir64)
        TTCASE_CMD(IDBTN_DIR32, OnBtnDir32)
    END_TTMSG_MAP()

    // Message handlers

    void OnCheckExe();
    void OnCheckLib();
    void OnBtnDir64();
    void OnBtnDir32();

    void OnBegin(void);
    void OnOK(void);

private:
    // Class members

    CTabOptions* m_pOpts;
};
