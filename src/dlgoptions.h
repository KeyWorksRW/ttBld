/////////////////////////////////////////////////////////////////////////////
// Name:		CTabOptions
// Purpose:		IDDLG_OPTIONS dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDDLG_OPTIONS
	#include "resource.h"
#endif

#include <ttdlg.h>		// ttCDlg, ttCComboBox, ttCListBox, ttCListView

#include "writesrcfiles.h"	// CWriteSrcFiles

#include "tabgeneral.h"	// CTabGeneral
#include "tabcompiler.h"// CTabCompiler
#include "tabclang.h"	// CTabCLang
#include "tablinker.h"	// CTabLinker
#include "tabscripts.h"	// CTabScripts
#include "tabrcmidl.h"	// CTabRcMidl

class CTabOptions : public ttCDlg, public CWriteSrcFiles
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

	void OnBegin();
	void OnOK();
	void OnCancel();

	// Class members

	typedef enum {
		TAB_GENERAL = 0,
		TAB_COMPILER,
		TAB_CLANG,
		TAB_LINKER,
		TAB_RC_MIDL,
		TAB_SCRIPTS,
	} TAB_ID;

	CTabGeneral	 m_tabGeneral;
	CTabCompiler m_tabCompiler;
	CTabCLang	 m_tabCLang;
	CTabLinker	 m_tabLinker;
	CTabRcMidl	 m_tabRcMidl;
	CTabScripts	 m_tabScripts;

	HWND m_hwndTabSub;
};