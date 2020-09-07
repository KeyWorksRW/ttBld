/////////////////////////////////////////////////////////////////////////////
// Purpose:   Dialog for setting all .srcfile options
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "optionsdlgBase.h"

class OptionsDlg : public OptionsDlgBase
{
public:
    OptionsDlg(wxWindow* parent = nullptr);

protected:
    // Handlers for OptionsDlgBase events.
    void OnAddIncDir(wxCommandEvent& event) override;
    void OnAddCommonLibraries(wxCommandEvent& event) override;
    void OnAddReleaseLibraries(wxCommandEvent& event) override;
    void OnAddDebugLibraries(wxCommandEvent& event) override;
    void OnAddBuildLibraries(wxCommandEvent& event) override;
};
