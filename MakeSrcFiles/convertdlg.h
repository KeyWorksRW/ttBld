/////////////////////////////////////////////////////////////////////////////
// Name:		CConvertDlg
// Purpose:		IDDDLG_CONVERT dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IDDDLG_CONVERT
	#include "res.h"
#endif

#include <ttdlg.h>		// ttCDlg, ttCComboBox, ttCListBox, ttCListView

class CConvertDlg : public ttCDlg
{
public:
	CConvertDlg() : ttCDlg(IDDDLG_CONVERT) {
		EnableShadeBtns();
	}

	// Class functions


protected:
	BEGIN_TTCMD_MAP()
		TTCASE_CMD(IDBTN_LOCATE_SCRIPT, OnBtnLocateScript)
	END_TTMSG_MAP()

	// Message handlers

	void OnBtnLocateScript();
	void OnBegin(void);
	void OnOK(void);

	// Class members

	ttCComboBox m_comboScripts;
};
