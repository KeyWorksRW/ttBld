/////////////////////////////////////////////////////////////////////////////
// Name:		CWriteSrcFiles
// Purpose:		Version of CSrcFiles that is capable of writing out a new or updated file
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2018 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "csrcfiles.h"		// CSrcFiles

class CWriteSrcFiles : public CSrcFiles
{
public:
	CWriteSrcFiles() : CSrcFiles() { }

	bool WriteUpdates();	// write updates to the [OPTIONS] section
	bool WriteNew();		// write complete .srcfiles file (replacing any file that already exists)
	void CreateTargetsString(CStr& cszTargets);	// will set m_cszTarget32 and maybe m_cszTarget64 if they are both empty

protected:
	// Class functions

	void UpdateLongOption(const char* pszOption, const char* pszVal, const char* pszComment = nullptr);
	void UpdateOption(const char* pszOption, const char* pszVal, const char* pszComment, bool bAlwaysWrite = false);
	void UpdateOptionsSection();

	const char* GetExeType();

	ptrdiff_t GetOptionLine(const char* pszOption);		// on success m_cszOptComment will be filled in
	ptrdiff_t FindOption(const char* pszOption, CStr& cszDst);
	ptrdiff_t FindSection(const char* pszSection);

	// Class members

	CStr m_cszOptComment;
	CStrList m_lstOriginal;
	ptrdiff_t m_posOptions;
	ptrdiff_t m_posInsert;
};
