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

	char* GetDirOutput() { return m_cszDirOutput; }
	char* GetDirSrcFiles() { return m_cszDirSrcFiles; }
	char* GetConvertScript() { return m_cszConvertScript; }

protected:
	BEGIN_TTCMD_MAP()
		TTCASE_CMD(IDBTN_CHANGE_IN, OnBtnChangeIn)
		TTCASE_CMD(IDBTN_CHANGE_OUT, OnBtnChangeOut)
		TTCASE_CMD(IDBTN_LOCATE_SCRIPT, OnBtnLocateScript)
	END_TTMSG_MAP()

	// Message handlers

	void OnBtnChangeIn();
	void OnBtnChangeOut();
	void OnBtnLocateScript();
	void OnBegin(void);
	void OnOK(void);

	// Class members

	ttCComboBox m_comboScripts;

	ttCStr m_cszDirOutput;		// where .srcfiles should be created
	ttCStr m_cszDirSrcFiles;
	ttCStr m_cszConvertScript;
};
