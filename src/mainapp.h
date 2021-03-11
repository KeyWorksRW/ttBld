/////////////////////////////////////////////////////////////////////////////
// Purpose:   Entry point
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/app.h>

class CMainApp : public wxApp
{
public:
protected:
    virtual bool OnInit() override;
    virtual int OnRun() override;
    virtual void OnFatalException() override;
    virtual int OnExit() override;
};

wxDECLARE_APP(CMainApp);
