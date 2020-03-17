/////////////////////////////////////////////////////////////////////////////
// Name:      CTabLibs
// Purpose:   IDTAB_LIBS dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDTAB_LIBS
    #include "resource.h"
#endif

#include <ttwindlg.h>  // Class for displaying a dialog

class CTabOptions;

class CTabLibs : public ttlib::dlg
{
public:
    CTabLibs() : ttlib::dlg(IDTAB_LIBS) {}

    // Class functions

    void SetParentClass(CTabOptions* pclsOptions) { m_pOpts = pclsOptions; }

protected:
    // Message handlers

    void OnBegin(void) override;
    void OnOK(void) override;

    // Class members

    CTabOptions* m_pOpts;
};
