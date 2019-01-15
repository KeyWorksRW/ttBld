/////////////////////////////////////////////////////////////////////////////
// Name:		CDryRun
// Purpose:		Class to store information for a dry-run of functionality
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../ttLib/include/ttstr.h" 	// ttStr
#include "../ttLib/include/ttfile.h"	// ttFile

class CDryRun
{
public:
	CDryRun() { m_bEnabled = false; }

	// Class functions

	void Enable() { m_bEnabled = true; }
	bool isEnabled() { return m_bEnabled; }

	void NewFile(const char* pszFile);	// will delete m_lstOut and replace m_cszFilename
	void DisplayFileDiff(ttFile& fileOrg, ttFile& fileNew);

protected:
	// Class members

	ttStr	m_cszFilename;
	bool	m_bEnabled;
};
