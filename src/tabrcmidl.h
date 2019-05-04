/////////////////////////////////////////////////////////////////////////////
// Name:		CTabRcMidl
// Purpose:		IDTAB_RCMIDL dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDTAB_RCMIDL
	#include "resource.h"
#endif

#include <ttdlg.h>		// ttCDlg, ttCComboBox, ttCListBox, ttCListView

class CTabRcMidl : public ttCDlg
{
public:
	CTabRcMidl() : ttCDlg(IDTAB_RCMIDL) { }

	// Class functions

	void SetParentClass(CTabOptions* pclsOptions) { m_pOpts = pclsOptions; }

protected:
	// Message handlers

	void OnBegin(void);
	void OnOK(void);

	// Class members

	CTabOptions* m_pOpts;
};
