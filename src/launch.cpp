/////////////////////////////////////////////////////////////////////////////
// Name:      launch.cpp
// Purpose:   Used to launch editors with MSVC environment setup
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

bool LaunchVsCode()
{
    ttCStr cszCodePath, cszMSVCPath;
    if (!FindVsCode(cszCodePath))
    {
        puts(TRANSLATE("Visual Studio Code does not appear to be installed."));
        return false;
    }

    if (!FindCurMsvcPath(cszMSVCPath))
    {
        puts(TRANSLATE("Visual Studio does not appear to be installed -- unable to determine path to MS compiler."));
        return false;
    }

    return true;
}
