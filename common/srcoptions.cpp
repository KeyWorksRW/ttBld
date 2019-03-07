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
	m_bBoolean = false;
	m_bRequired = false;
}

void CSrcOption::AddOption(const char* pszName, bool bBoolean, bool bRequired)
{
	m_pszName = pszName;
	m_bBoolean = bBoolean;
	m_bRequired = bRequired;
}

// The true/false strings MUST be lowercase! And yes, they must be strings--their value can be written out without have to convert to a string first

void CSrcOption::UpdateOption(bool bValue, const char* pszComment)
{
	m_bBoolean = true;
	m_bVal = bValue;

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

// It's important that if a zero-length string is passed in for either parameter, then the matching value or comment
// should be set to a nullptr. This makes it so that callers only need to check for a nullptr to determine if there is a
// valid value or comment.

void CSrcOption::UpdateOption(const char* pszVal, const char* pszComment)
{
	if (m_bBoolean) {
		if (pszVal)
			m_bVal = tt::isSameStri(pszVal, "true") || tt::isSameStri(pszVal, "yes");
		else
			m_bVal = false;
	}
	else {
		if (!pszVal || !*pszVal) {
			if (m_pszVal) {
				tt::FreeAlloc(m_pszVal);
				m_pszVal = nullptr;
			}
		}
		else {
			if (m_pszVal) {
				if (!tt::isSameStr(m_pszVal, pszVal)) {		// don't allocate if nothing will change
					tt::FreeAlloc(m_pszVal);
					m_pszVal = tt::StrDup(pszVal);
				}
			}
			else
				m_pszVal = tt::StrDup(pszVal);
		}
	}

	if (!pszComment || !*pszComment) {
		if (m_pszComment) {
			tt::FreeAlloc(m_pszComment);
			m_pszComment = nullptr;
		}
	}
	else {
		if (m_pszComment) {
			if (!tt::isSameStr(m_pszComment, pszComment)) {		// don't allocate if nothing will change
				tt::FreeAlloc(m_pszComment);
				m_pszComment = tt::StrDup(pszComment);
			}
		}
		else
			m_pszComment = tt::StrDup(pszComment);
	}
}

const char* CSrcOption::getOption()
{
	if (!m_bBoolean)
		return m_pszVal;
	else
		return (m_bVal ? "true" : nullptr);
}
