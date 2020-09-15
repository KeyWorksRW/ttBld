/////////////////////////////////////////////////////////////////////////////
// Name:      CTabRcMidl
// Purpose:   IDTAB_RCMIDL dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDTAB_RCMIDL
    #include "resource.h"
#endif

#include <ttwindlg.h>  // Class for displaying a dialog

class CTabOptions;

class CTabRcMidl : public ttlib::dlg
{
public:
    CTabRcMidl() : ttlib::dlg(IDTAB_RCMIDL) {}

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
