/////////////////////////////////////////////////////////////////////////////
// Name:		CParseHHP
// Purpose:		Parse an HHP file to collect dependencies
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2018 KeyWorks Software (Ralph Walden)
// Licence:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../ttLib/include/strlist.h"	// CStrList
#include "../ttLib/include/cstr.h"		// CStr

class CParseHHP
{
public:
	CParseHHP(const char* pszHHPName);

	enum {
		SECTION_UNKNOWN,
		SECTION_ALIAS,
		SECTION_FILES,
		SECTION_OPTIONS,
		SECTION_TEXT_POPUPS,
	};

	// Class functions

	void ParseHhpFile(const char* pszHHP = nullptr);

	CStrList	m_lstDependencies;
	CStr		m_cszChmFile;

protected:
	void AddDependency(const char* pszHHP, const char* pszFile);

	// Class members

	CStr	m_cszCWD;
	size_t	m_section;
	CStr	m_cszRoot;		// root directory to base all filenames and includes to
	CStr	m_cszHHPName;	// root level HHP filename
};
