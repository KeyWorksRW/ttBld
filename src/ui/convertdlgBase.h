////////////////////////////////////////////////////////////////////////////
// Code generated by wxUiEditor ( https://github.com/KeyWorksRW/wxUiEditor/ )
//
// DO NOT EDIT THIS FILE! Your changes will be lost if it is re-generated!
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/valgen.h>
#include <wx/filepicker.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

class ConvertDlgBase : public wxDialog
{
public:
	ConvertDlgBase(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = "Create new .srcfiles.yaml file",
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);

protected:
	bool m_useAllFiles { false };
	bool m_useProjectFile { false };
	wxString m_Project;
	bool m_AddVscodeDir { false };
	bool m_gitIgnore { false };

	wxStaticText* m_staticText;
	wxDirPickerCtrl* m_dirPickerOut;
	wxRadioButton* m_radioBtn;
	wxDirPickerCtrl* m_dirPickerList;
	wxRadioButton* m_radioBtn2;
	wxChoice* m_choiceProjects;
	wxFilePickerCtrl* m_filePickerProject;
	wxCheckBox* m_checkBox;
	wxCheckBox* m_checkGitIgnore;
	wxStaticLine* m_staticline;

	wxButton* std_button_sizerOK;
	wxButton* std_button_sizerCancel;

	// Virtual event handlers, overide them in your derived class
	virtual void OnInit(wxInitDialogEvent& event) { event.Skip(); }
	virtual void OnProjectFileLocated(wxFileDirPickerEvent& event) { event.Skip(); }
	virtual void OnOK(wxCommandEvent& event) { event.Skip(); }

private:
};