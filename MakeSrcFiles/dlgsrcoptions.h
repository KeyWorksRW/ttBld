/////////////////////////////////////////////////////////////////////////////
// Name:		CDlgSrcOptions
// Purpose:		Class for displaying a dialog allowing for modification of .SrcFiles
// Author:		Ralph Walden
// Copyright:	Copyright (c) 1998-2018 KeyWorks Software (Ralph Walden)
// License:     Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../ttLib/include/cttdlg.h"	// CTTDlg

#include "../common/writesrcfiles.h"	// CWriteSrcFiles

#ifndef IDDLG_SRCFILES
	#include "res.h"
#endif

class CDlgSrcOptions : public CTTDlg, public CWriteSrcFiles
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
	void OnEnd(void);

	// Class members

	CDlgListBox m_lb;
	CStr m_cszSrcDir;
	CStr m_cszOptComment;
	CStrList m_lstOriginal;
	ptrdiff_t m_posProject;
};
