/////////////////////////////////////////////////////////////////////////////
// Name:      CVcxWrite
// Purpose:   Create a Visual Studio project file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   MIT License (see MIT License)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ninja.h"  // CNinja

class CVcxWrite : public CNinja
{
public:
    CVcxWrite(const char* pszNinjaDir = nullptr) : CNinja(pszNinjaDir) {}

    bool CreateBuildFile();
};
