/////////////////////////////////////////////////////////////////////////////
// Name:		CSrcOption
// Purpose:		Class for storing/retrieving a single option in .srcfiles
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttheap.h> // ttCHeap

#include "srcoptions.h"

CSrcOption::CSrcOption()
{
	m_pszName    = nullptr;
	m_pszVal     = nullptr;
	m_pszComment = nullptr;
	m_bRequired = false;
}

void CSrcOption::AddOption(const char* pszName, bool bRequired)
{
	m_pszName = pszName;
	m_bRequired = bRequired;
}

void CSrcOption::UpdateOption(bool bValue, const char* pszComment)
{
	m_pszVal = (bValue ? (char*) 1 : (char*) 0);
	if (!pszComment) {
		if (m_pszComment) {
			tt::FreeAlloc(m_pszComment);
			m_pszComment = nullptr;
		}
	}
	else if (m_pszComment)
		tt::FreeAlloc(m_pszComment);

	m_pszComment = tt::StrDup(pszComment);
}

void CSrcOption::UpdateOption(const char* pszVal, const char* pszComment)
{
	ttASSERT_MSG(pszVal, "Option value not specified!");
	if (!pszVal)
		throw;	// updating an option with a nullptr is going to break reading the option later on

	ttASSERT_MSG(m_pszVal != (char*) 1, "Attempting to update a boolean option with a string value!");

	if (m_pszVal > (char*) 1)
		tt::FreeAlloc(m_pszVal);
	m_pszVal = tt::StrDup(pszVal);

	if (!pszComment) {
		if (m_pszComment) {
			tt::FreeAlloc(m_pszComment);
			m_pszComment = nullptr;
		}
	}
	else if (m_pszComment)
		tt::FreeAlloc(m_pszComment);

	m_pszComment = tt::StrDup(pszComment);
}
