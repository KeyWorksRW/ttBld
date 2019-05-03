/////////////////////////////////////////////////////////////////////////////
// Name:		CTabGeneral
// Purpose:		IDTAB_GENERAL dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDTAB_GENERAL
	#include "resource.h"
#endif

#include <ttdlg.h>		// ttCDlg, ttCComboBox, ttCListBox, ttCListView

class CTabGeneral : public ttCDlg
{
public:
	CTabGeneral() : ttCDlg(IDTAB_GENERAL) { }

	// Class functions


protected:
	// Message handlers

	void OnBegin(void);
	void OnOK(void);

	// Class members


};
