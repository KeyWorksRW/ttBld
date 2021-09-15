////////////////////////////////////////////////////////////////////////////////
// Code generated by wxUiEditor -- see https://github.com/KeyWorksRW/wxUiEditor/
//
// DO NOT EDIT THIS FILE! Your changes will be lost if it is re-generated!
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/event.h>
#include <wx/filepicker.h>
#include <wx/gdicmn.h>
#include <wx/radiobut.h>

class ConvertDlgBase : public wxDialog
{
public:
    ConvertDlgBase() {}
    ConvertDlgBase(wxWindow *parent, wxWindowID id = wxID_ANY, const wxString& title = wxString::FromUTF8("Create new .srcfiles.yaml file"),
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER, const wxString &name = wxDialogNameStr)
    {
        Create(parent, id, title, pos, size, style, name);
    }

    bool Create(wxWindow *parent, wxWindowID id = wxID_ANY, const wxString& title = wxString::FromUTF8("Create new .srcfiles.yaml file"),
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER, const wxString &name = wxDialogNameStr);

    bool isCreateVsCode() const { return m_AddVscodeDir; }
    bool isAddToGitExclude() const { return m_gitIgnore; }

protected:

    // Validator variables

    bool m_AddVscodeDir { false };
    bool m_gitIgnore { false };
    bool m_useAllFiles { false };
    bool m_useProjectFile { false };
    wxString m_Project;

    // Class member variables

    wxCheckBox* m_checkBox;
    wxCheckBox* m_checkGitIgnore;
    wxChoice* m_choiceProjects;
    wxDirPickerCtrl* m_dirPickerList;
    wxDirPickerCtrl* m_dirPickerOut;
    wxFilePickerCtrl* m_filePickerProject;
    wxRadioButton* m_radioBtn2;
    wxRadioButton* m_radioBtn;

    // Virtual event handlers -- override them in your derived class

    virtual void OnInit(wxInitDialogEvent& event) { event.Skip(); }
    virtual void OnOK(wxCommandEvent& event) { event.Skip(); }
    virtual void OnProjectFileLocated(wxFileDirPickerEvent& event) { event.Skip(); }
};
