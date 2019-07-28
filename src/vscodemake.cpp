/////////////////////////////////////////////////////////////////////////////
// Name:      vscodemake.cpp
// Purpose:   Creates .vscode/makefile
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

/*
    CNinja::CreateMakeFile is used to create a general purpose makefile without knowledge of the user's system.

    CNinja::VsCodeMakefile creates a makefile that is highly customized to the user's machine at the time the makefile
    was created. It will differ based on the platform, the compiler available to it, and it will assume MakeNinja.exe is
    available since that was what was used to create it.

*/

#include "pch.h"

#include <ttfile.h>   // ttCFile

#include "ninja.h"    // CNinja

// Used to create a makefile in .vscode

bool CNinja::VsCodeMakefile()
{
    if (ttFileExists(".vsCode/makefile"))
        return true;    // TODO: [KeyWorks - 7/26/2019] If we end up storing the MSVC path in the makefile, then we need to be able to update it.

    ttCStr cszTmp;
#if defined(_WIN32)
    if (!FindFileEnv("PATH", "mingw32-make.exe", cszTmp))
        return VsCodeNmake();
#endif

    ttCFile file;




    if (GetBoolOption(OPT_64BIT) && GetBoolOption(OPT_32BIT))
    {
    }

    return false;
}

// Call this to create a makefile compatible with nmake.exe

bool CNinja::VsCodeNmake()
{
    return false;
}
