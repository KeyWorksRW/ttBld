/////////////////////////////////////////////////////////////////////////////
// Name:      CVcxProj
// Purpose:   Class creating a Visual Studio build script
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ninja.h"    // CNinja

class CVcxProj : public CNinja
{
public:
    CVcxProj() : CNinja(true) { }

    // Class functions

    bool CreateBuildFile();

private:
    // Class members

};
