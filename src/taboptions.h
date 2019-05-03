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

class CTabOptions : public ttCDlg
{
public:
	CTabOptions() : ttCDlg(IDDLG_OPTIONS) { }

	// Class functions


protected:
	// Message handlers

	void OnBegin(void);
	void OnOK(void);

	// Class members


};
