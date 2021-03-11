/////////////////////////////////////////////////////////////////////////////
// Purpose:   Create a Visual Studio project file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ninja.h"  // CNinja

class CVcxWrite : public CNinja
{
public:
    CVcxWrite(std::string_view NinjaDir = ttlib::emptystring) : CNinja(NinjaDir) {}

    bool CreateBuildFile();
};
