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
	void setRequired(bool bRequire = true) { m_bRequired = bRequire; }

	const char* getName() { return m_pszName; }
	const char* getComment() { return m_pszComment; }

	const char* getOption();	// for boolean, will return pointer to "true" for true, or nullptr for false
	bool 		getBoolOption()   { ttASSERT(m_bBoolean); return m_bVal; }	// in case you really, really need a bool return

	void AddOption(const char* pszName, bool bBoolean = false, bool bRequired = false);
	void UpdateOption(bool bValue, const char* pszComment = nullptr);
	void UpdateOption(const char* pszVal, const char* pszComment = nullptr);	// fine to call this for boolean options if pszVal == "true/false" or "yes/no"

protected:
	// Class members

	const char* m_pszName;	// points to callers string, NOT allocated
	char*		m_pszVal;	// only used if m_bBoolean is false
	char*		m_pszComment;

	bool	m_bVal; 		// only used if m_bBoolean is true
	bool	m_bBoolean;		// argument is a boolean value
	bool	m_bRequired;
};
