/////////////////////////////////////////////////////////////////////////////
// Name:		CTabCLang
// Purpose:		IDTAB_CLANG dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDTAB_CLANG
	#include "resource.h"
#endif

#include <ttdlg.h>		// ttCDlg, ttCComboBox, ttCListBox, ttCListView

class CTabCLang : public ttCDlg
{
public:
	CTabCLang() : ttCDlg(IDTAB_CLANG) { }

	// Class functions

	void SetParentClass(CTabOptions* pclsOptions) { m_pOpts = pclsOptions; }

protected:
	// Message handlers

	void OnBegin(void);
	void OnOK(void);

	// Class members

	CTabOptions* m_pOpts;
};
