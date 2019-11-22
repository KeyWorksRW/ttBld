/////////////////////////////////////////////////////////////////////////////
// Name:      CMainApp
// Purpose:   Entry point
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/app.h>

class CMainApp : public wxApp
{
public:
    // Public functions

protected:
    virtual bool OnInit() wxOVERRIDE;
    virtual int  OnRun() wxOVERRIDE;
};

wxDECLARE_APP(CMainApp);
