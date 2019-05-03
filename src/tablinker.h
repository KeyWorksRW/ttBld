/////////////////////////////////////////////////////////////////////////////
// Name:		CTabLinker
// Purpose:		IDTAB_LINKER dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDTAB_LINKER
	#include "resource.h"
#endif

#include <ttdlg.h>		// ttCDlg, ttCComboBox, ttCListBox, ttCListView

class CTabLinker : public ttCDlg
{
public:
	CTabLinker() : ttCDlg(IDTAB_LINKER) { }

	// Class functions


protected:
	// Message handlers

	void OnBegin(void);
	void OnOK(void);

	// Class members


};
