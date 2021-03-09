/////////////////////////////////////////////////////////////////////////////
// Purpose:   Dialog specifying what to convert into a .srcfiles.yaml file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "convertdlgBase.h"

class ConvertDlg : public ConvertDlgBase
{
public:
    ConvertDlg(std::string_view projectFile);

    const ttlib::cstr& GetOutSrcFiles() { return m_new_srcfiles; }

protected:
    // Handlers for ConvertDlgBase events.

    void OnInit(wxInitDialogEvent& event) override;
    void OnProjectFileLocated(wxFileDirPickerEvent& WXUNUSED(event)) override;
    void OnOK(wxCommandEvent& WXUNUSED(event)) override;

private:
    ttlib::cstr m_new_srcfiles;  // Where .srcfiles should be created
};
