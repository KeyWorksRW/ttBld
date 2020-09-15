////////////////////////////////////////////////////////////////////////////
// Code generated by wxUiEditor ( https://github.com/KeyWorksRW/wxUiEditor/ )
//
// DO NOT EDIT THIS FILE! Your changes will be lost if it is re-generated!
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/checkbox.h>
#include <wx/dialog.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/string.h>

class VsCodeDlgBase : public wxDialog
{
public:
	VsCodeDlgBase(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = "Create .vscode files",
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_DIALOG_STYLE);

protected:
	// Validator variables

	bool m_defTaskCLANG { false };
	bool m_defTaskMSVC { false };
	bool m_defTaskNinja { true };
	bool m_preLaunchCLANG { false };
	bool m_preLaunchMSVC { false };
	bool m_preLaunchNinja { true };
	bool m_preLaunchNone { false };
	bool m_taskCLANGBuild { false };
	bool m_taskMSVCBuild { false };
	bool m_taskNinjaBuild { true };

	// Class member variables

	wxCheckBox* m_checkBox2;
	wxCheckBox* m_checkBox3;
	wxCheckBox* m_checkBox;
	wxRadioButton* m_radioBtn2;
	wxRadioButton* m_radioBtn3;
	wxRadioButton* m_radioBtn4;
	wxRadioButton* m_radioBtn5;
	wxRadioButton* m_radioBtn6;
	wxRadioButton* m_radioBtn7;
	wxRadioButton* m_radioBtn;
	wxStaticBoxSizer* m_static_box;
	wxStaticBoxSizer* static_box_sizer2;
	wxStaticBoxSizer* static_box_sizer3;

	// Virtual event handlers -- overide them in your derived class

	virtual void OnInit(wxInitDialogEvent& event) { event.Skip(); }

private:
};
