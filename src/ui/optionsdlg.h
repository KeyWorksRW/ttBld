/////////////////////////////////////////////////////////////////////////////
// Purpose:   Dialog for setting all .srcfile options
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <optional>

#include <ttstring_wx.h>     // ttString -- wxString with additional methods similar to ttlib::cstr
#include "writesrc.h"  // CWriteSrcFiles -- Writes a new or update srcfiles.yaml file

#include "optionsdlgBase.h"

class OptionsDlg : public OptionsDlgBase, public CWriteSrcFiles
{
public:
    OptionsDlg(const std::string& ProjectFile);

    void SaveChanges();

protected:
    void OnPchSrcChanged(wxFileDirPickerEvent& event) override;
    void OnPchHeaderChanged(wxFileDirPickerEvent& event) override;
    void OnTargetDirChanged(wxFileDirPickerEvent& event) override;
    void OnAddIncDir(wxCommandEvent& event) override;
    void OnAddCommonLibraries(wxCommandEvent& event) override;
    void OnAddReleaseLibraries(wxCommandEvent& event) override;
    void OnAddDebugLibraries(wxCommandEvent& event) override;
    void OnAddBuildLibraries(wxCommandEvent& event) override;

    std::optional<ttlib::cstr> AddLibrary(const wxString& cur_libs, const wxString& new_libs);

private:
    ttString m_cwd;
};
