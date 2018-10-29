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
		MSG_BUTTON_CLICK(IDBTN_REMOVE, OnBtnREmove)
		MSG_BUTTON_CLICK(IDBTN_ADD, OnBtnAddFile)
		MSG_BUTTON_CLICK(IDBTN_PCH, OnBtnChangePch)
	END_TTMSG_MAP()

	void SaveChanges();

protected:
	// Class functions

	void UpdateOption(const char* pszOption, ptrdiff_t posInsert, const char* pszVal, const char* pszComment, bool bAlwaysWrite = false);
	void UpdateLongOption(const char* pszOption, ptrdiff_t posInsert, const char* pszVal);
	void UpdateIdeSection();
	void UpdateCompilerSection();
	void UpdateOptionsSection();

	const char* GetExeType();

	ptrdiff_t GetOptionLine(const char* pszOption, ptrdiff_t posStart);	// on success m_cszOptComment will be filled in
	ptrdiff_t FindOption(const char* pszOption, CStr& cszDst);
	ptrdiff_t FindSection(const char* pszSection);

protected:
	// Msg handlers

	void OnBtnREmove();
	void OnBtnAddFile();
	void OnBtnChangePch();

	// Msg functions

	void OnBegin(void);
	void OnEnd(void);

	// Class members

	bool m_fFileListChanged;
	CDlgListBox m_lb;
	CStr m_cszSrcDir;
	CStr m_cszOptComment;
	CStrList m_lstOriginal;
	ptrdiff_t m_posProject;
};
