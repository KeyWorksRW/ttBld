/////////////////////////////////////////////////////////////////////////////
// Name:		CDlgSrcOptions
// Purpose:		Class for displaying a dialog allowing for modification of .SrcFiles
// Author:		Ralph Walden
// Copyright:	Copyright (c) 1998-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ttdlg.h>						// ttCDlg, ttCComboBox, ttCListBox, ttCListView

#include "../common/writesrcfiles.h"	// CWriteSrcFiles

#ifndef IDDLG_SRCFILES
	#include "res.h"
#endif

class CDlgSrcOptions : public ttCDlg, public CWriteSrcFiles
{
public:
	CDlgSrcOptions(const char* pszSrcDir = nullptr);

	BEGIN_TTCMD_MAP()
		TTCASE_CMD(IDBTN_PCH, OnBtnChangePch)
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

	ttCListBox	m_lb;
	ttCStr		m_cszSrcDir;
	ttCStr		m_cszOptComment;
	ttCList		m_lstOriginal;
	ptrdiff_t	m_posProject;
};
