/////////////////////////////////////////////////////////////////////////////
// Name:      gencmdfiles.cpp
// Purpose:   Generates MSVCenv.cmd and Code.cmd files
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttfile.h>    // ttCFile


bool CreateMSVCenvCmd()
{
#if !defined(_WIN32)
    return false;   // MSVC compiler and environment is only available on Windows

#else    // Windows-only code below

    return false;
#endif    // end Windows-only code
}
