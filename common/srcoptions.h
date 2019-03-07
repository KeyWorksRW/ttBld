/////////////////////////////////////////////////////////////////////////////
// Name:		CSrcOption
// Purpose:		Class for storing/retrieving a single option in .srcfiles
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

/*

 * This class is designed for use by CSrcFiles and CWriteSrcFiles. It allows the parent classes to create an array of
 * CSrcOption classes and then walk through the array reading or writing the values.

 */

#pragma once

class CSrcOption
{
public:
	CSrcOption();

	// Class functions

	bool isComment() { return (m_pszComment && *m_pszComment) ? true : false; }
	bool isRequired() { return m_bRequired; }

	const char* getName() { return m_pszName; }
	const char* getComment() { return m_pszComment; }

	// The following two functions assume you already know if the option uses a string value or a boolean value

	const char* getStringOption() { return m_pszVal; }
	bool 		getBoolOption()   { return (m_pszVal ? true : false); }

	void AddOption(const char* pszName, bool bRequired = false);
	void UpdateOption(bool bValue, const char* pszComment = nullptr);
	void UpdateOption(const char* pszVal, const char* pszComment = nullptr);

protected:
	// Class members

	// Note: for boolean options, m_pszVal will be 0 or 1

	const char* m_pszName;	// points to callers string, NOT allocated
	char*		m_pszVal;
	char*		m_pszComment;

	bool	m_bRequired;
};
