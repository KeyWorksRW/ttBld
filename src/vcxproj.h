/////////////////////////////////////////////////////////////////////////////
// Name:		CVcxProj
// Purpose:		Class creating a Visual Studio build script
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2018 KeyWorks Software (Ralph Walden)
// License:     Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "bldmaster.h"	// CBldMaster

class CVcxProj : public CBldMaster
{
public:
	CVcxProj() : CBldMaster(true) { }

	// Class functions

	bool CreateBuildFile();

protected:
	// Class members

};
