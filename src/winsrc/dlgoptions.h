/////////////////////////////////////////////////////////////////////////////
// Name:      CTabOptions
// Purpose:   IDDLG_OPTIONS dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDDLG_OPTIONS
    #include "resource.h"
#endif

#include <ttwindlg.h>  // Class for displaying a dialog

#include "writesrc.h"  // CWriteSrcFiles -- Writes a new or update srcfiles.yaml file

#include "tabclang.h"     // CTabCLang
#include "tabcompiler.h"  // CTabCompiler
#include "tabgeneral.h"   // CTabGeneral
#include "tablibs.h"      // CTabLibs
#include "tablinker.h"    // CTabLinker
#include "tabrcmidl.h"    // CTabRcMidl

#ifdef PRIVATE
    #include ".private/tabprivate.h"  // CTabPrivate
#endif

class CTabOptions : public ttlib::dlg, public CWriteSrcFiles
{
public:
    CTabOptions();

    // Class functions

    void SaveChanges();

protected:
    BEGIN_TTMSG_MAP()
        TTMSG_WM_NOTIFY(OnNotify)
    END_TTMSG_MAP()

    // Message handlers

    LRESULT OnNotify(int id, NMHDR* pNmHdr);

    void OnBegin() override;
    void OnOK() override;
    void OnCancel() override;

private:
    // Class members

    enum TAB_ID : size_t
    {
        TAB_GENERAL = 0,
        TAB_COMPILER,
        TAB_LIBS,
        TAB_LINKER,
        TAB_RC_MIDL,
        TAB_CLANG,
        TAB_PRIVATE,
    };

    CTabGeneral m_tabGeneral;
    CTabCompiler m_tabCompiler;
    CTabLibs m_tabLibs;
    CTabLinker m_tabLinker;
    CTabRcMidl m_tabRcMidl;
    CTabCLang m_tabCLang;

#ifdef PRIVATE
    CTabPrivate m_tabPrivate;
#endif

    HWND m_hwndTabSub;
};
