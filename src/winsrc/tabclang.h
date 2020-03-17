/////////////////////////////////////////////////////////////////////////////
// Name:      CTabCLang
// Purpose:   IDTAB_CLANG dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDTAB_CLANG
    #include "resource.h"
#endif

#include <ttwindlg.h>  // Class for displaying a dialog

class CTabOptions;

class CTabCLang : public ttlib::dlg
{
public:
    CTabCLang() : ttlib::dlg(IDTAB_CLANG) {}

    // Class functions

    void SetParentClass(CTabOptions* pclsOptions) { m_pOpts = pclsOptions; }

protected:
    // Message handlers

    void OnBegin(void) override;
    void OnOK(void) override;

private:
    // Class members

    CTabOptions* m_pOpts;
};
