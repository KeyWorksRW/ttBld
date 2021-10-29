////////////////////////////////////////////////////////////////////////////////
// Code generated by wxUiEditor -- see https://github.com/KeyWorksRW/wxUiEditor/
//
// DO NOT EDIT THIS FILE! Your changes will be lost if it is re-generated!
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/dialog.h>
#include <wx/event.h>
#include <wx/filepicker.h>
#include <wx/gdicmn.h>
#include <wx/notebook.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

class OptionsDlgBase : public wxDialog
{
public:
    OptionsDlgBase() {}
    OptionsDlgBase(wxWindow *parent, wxWindowID id = wxID_ANY, const wxString& title = "Options for .srcfiles",
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER, const wxString &name = wxDialogNameStr)
    {
        Create(parent, id, title, pos, size, style, name);
    }

    bool Create(wxWindow *parent, wxWindowID id = wxID_ANY, const wxString& title = "Options for .srcfiles",
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER, const wxString &name = wxDialogNameStr);

protected:

    // Validator variables

    bool m_isConsole { false };
    bool m_isDebugDllCRT { true };
    bool m_isDll { false };
    bool m_isLibrary { false };
    bool m_isOcx { false };
    bool m_isReleaseDllCRT { true };
    bool m_isSpaceOptimization { true };
    bool m_isSpeedOptimization { false };
    bool m_isWindow { false };
    bool m_useMSLinker { false };
    int m_WarningLevel { 4 };
    wxString m_BuildLibs;
    wxString m_CommonCppFlags;
    wxString m_CommonLibs;
    wxString m_CommonLinkFlags;
    wxString m_DebugCppFlags;
    wxString m_DebugLibs;
    wxString m_DebugLinkFlags;
    wxString m_IncludeDirs;
    wxString m_ProjectName;
    wxString m_ReleaseCppFlags;
    wxString m_ReleaseLibs;
    wxString m_ReleaseLinkFlags;
    wxString m_commonClangFlags;
    wxString m_commonMidlFlags;
    wxString m_commonRcFlags;
    wxString m_debugClangFlags;
    wxString m_debugMidlFlags;
    wxString m_debugRcFlags;
    wxString m_releaseClangFlags;
    wxString m_releaseMidlFlags;
    wxString m_releaseRcFlags;

    // Class member variables

    wxDirPickerCtrl* m_TargetDirPicker;
    wxFilePickerCtrl* m_NatvisPicker;
    wxFilePickerCtrl* m_PchHeaderPicker;
    wxFilePickerCtrl* m_PchSrcPicker;
    wxNotebook* m_notebook;
    wxStaticText* m_staticText;
    wxTextCtrl* m_buildLibs;
    wxTextCtrl* m_commonLibs;
    wxTextCtrl* m_debugLibs;
    wxTextCtrl* m_releaseLibs;
    wxTextCtrl* m_textIncludeDirs;

    // Virtual event handlers -- override them in your derived class

    virtual void OnAddBuildLibraries(wxCommandEvent& event) { event.Skip(); }
    virtual void OnAddCommonLibraries(wxCommandEvent& event) { event.Skip(); }
    virtual void OnAddDebugLibraries(wxCommandEvent& event) { event.Skip(); }
    virtual void OnAddIncDir(wxCommandEvent& event) { event.Skip(); }
    virtual void OnAddReleaseLibraries(wxCommandEvent& event) { event.Skip(); }
    virtual void OnPchHeaderChanged(wxFileDirPickerEvent& event) { event.Skip(); }
    virtual void OnPchSrcChanged(wxFileDirPickerEvent& event) { event.Skip(); }
    virtual void OnTargetDirChanged(wxFileDirPickerEvent& event) { event.Skip(); }
};
