/////////////////////////////////////////////////////////////////////////////
// Name:      CTabLibs
// Purpose:   IDTAB_LIBS dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <optional>

#ifndef IDTAB_LIBS
    #include "resource.h"
#endif

#include <ttwindlg.h>  // Class for displaying a dialog

class CTabOptions;

class CTabLibs : public ttlib::dlg
{
public:
    CTabLibs() : ttlib::dlg(IDTAB_LIBS) {}

    void SetParentClass(CTabOptions* pclsOptions) { m_pOpts = pclsOptions; }

protected:
    std::optional<ttlib::cstr> AddLibrary(std::string_view curLibs);

    BEGIN_TTCMD_MAP()
        TTCASE_CMD(IDBTN_ADD_LIB_REL, OnBtnAddLibRel)
        TTCASE_CMD(IDBTN_ADD_LIB_DBG, OnBtnAddLibDbg)
        TTCASE_CMD(IDBTN_ADD_LIB_CMN, OnBtnAddLibCmn)
        TTCASE_CMD(IDBTN_ADD_LIB_BLDLIB, OnBtnAddBuildLibrary)
    END_TTMSG_MAP()

    void OnBegin(void) override;
    void OnOK(void) override;

    void OnBtnAddLibRel();
    void OnBtnAddLibDbg();
    void OnBtnAddLibCmn();
    void OnBtnAddBuildLibrary();

    CTabOptions* m_pOpts;
};
