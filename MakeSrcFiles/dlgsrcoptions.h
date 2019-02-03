/////////////////////////////////////////////////////////////////////////////
// Name:		CDlgSrcOptions
// Purpose:		Class for displaying a dialog allowing for modification of .SrcFiles
// Author:		Ralph Walden
// Copyright:	Copyright (c) 1998-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../ttLib/include/ttdlg.h" 	// ttDlg, ttComboBox, ttListBox, ttListView

#include "../common/writesrcfiles.h"	// CWriteSrcFiles

#ifndef IDDLG_SRCFILES
	#include "res.h"
#endif

class CDlgSrcOptions : public ttDlg, public CWriteSrcFiles
{
public:
	CDlgSrcOptions(const char* pszSrcDir = nullptr);

	BEGIN_TTMSG_MAP()
		MSG_BUTTON_CLICK(IDBTN_PCH, OnBtnChangePch)
	END_TTMSG_MAP()

	void SaveChanges();

protected:
	// Class functions

protected:
	// Msg handlers

	void OnBtnChangePch();

	// Msg functions

	void OnBegin(void);
	void OnOK(void);

	// Class members

	ttListBox	m_lb;
	ttString	m_cszSrcDir;
	ttString	m_cszOptComment;
	ttList		m_lstOriginal;
	ptrdiff_t	m_posProject;
};
