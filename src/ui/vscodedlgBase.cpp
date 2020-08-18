////////////////////////////////////////////////////////////////////////////
// Code generated by wxUiEditor ( https://github.com/KeyWorksRW/wxUiEditor/ )
//
// DO NOT EDIT THIS FILE! Your changes will be lost if it is re-generated!
////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "vscodedlgBase.h"

VsCodeDlgBase::VsCodeDlgBase(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) :
	wxDialog(parent, id, title, pos, size, style)

{
	SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* parent_sizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* box_sizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticBoxSizer* static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "tasks.json"), wxVERTICAL);

	m_checkBox = new wxCheckBox(static_box_sizer->GetStaticBox(), wxID_ANY, "MSVC builds");

	m_checkBox->SetValidator(wxGenericValidator(&m_taskMSVCBuild));

	static_box_sizer->Add(m_checkBox, 0, wxALL, 5);

	m_checkBox2 = new wxCheckBox(static_box_sizer->GetStaticBox(), wxID_ANY, "CLANG builds");

	m_checkBox2->SetValidator(wxGenericValidator(&m_taskCLANGBuild));

	static_box_sizer->Add(m_checkBox2, 0, wxALL, 5);

	m_checkBox3 = new wxCheckBox(static_box_sizer->GetStaticBox(), wxID_ANY, "Ninja debug build");

	m_checkBox3->SetValue(true);
	m_checkBox3->SetValidator(wxGenericValidator(&m_taskNinjaBuild));

	static_box_sizer->Add(m_checkBox3, 0, wxALL, 5);

	box_sizer->Add(static_box_sizer, 0, wxALL|wxEXPAND, 5);

	wxStaticBoxSizer* static_box_sizer2 = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Default task"), wxVERTICAL);

	m_radioBtn = new wxRadioButton(static_box_sizer2->GetStaticBox(), wxID_ANY, "MSVC debug", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	m_radioBtn->SetValidator(wxGenericValidator(&m_defTaskMSVC));

	static_box_sizer2->Add(m_radioBtn, 0, wxALL, 5);

	m_radioBtn2 = new wxRadioButton(static_box_sizer2->GetStaticBox(), wxID_ANY, "CLANG debug");
	m_radioBtn2->SetValidator(wxGenericValidator(&m_defTaskCLANG));

	static_box_sizer2->Add(m_radioBtn2, 0, wxALL, 5);

	m_radioBtn3 = new wxRadioButton(static_box_sizer2->GetStaticBox(), wxID_ANY, "Ninja debug");
	m_radioBtn3->SetValue(true);
	m_radioBtn3->SetValidator(wxGenericValidator(&m_defTaskNinja));

	static_box_sizer2->Add(m_radioBtn3, 0, wxALL, 5);

	box_sizer->Add(static_box_sizer2, 0, wxALL|wxEXPAND, 5);

	parent_sizer->Add(box_sizer, 0, wxALL|wxEXPAND, 5);

	wxBoxSizer* box_sizer2 = new wxBoxSizer(wxHORIZONTAL);

	wxStaticBoxSizer* static_box_sizer3 = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "launch.json Pre-launch build"), wxVERTICAL);

	m_radioBtn4 = new wxRadioButton(static_box_sizer3->GetStaticBox(), wxID_ANY, "none", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	m_radioBtn4->SetValidator(wxGenericValidator(&m_preLaunchNone));

	static_box_sizer3->Add(m_radioBtn4, 0, wxALL, 5);

	m_radioBtn5 = new wxRadioButton(static_box_sizer3->GetStaticBox(), wxID_ANY, "MSVC debug");
	m_radioBtn5->SetValidator(wxGenericValidator(&m_preLaunchMSVC));

	static_box_sizer3->Add(m_radioBtn5, 0, wxALL, 5);

	m_radioBtn6 = new wxRadioButton(static_box_sizer3->GetStaticBox(), wxID_ANY, "CLANG debug");
	m_radioBtn6->SetValidator(wxGenericValidator(&m_preLaunchCLANG));

	static_box_sizer3->Add(m_radioBtn6, 0, wxALL, 5);

	m_radioBtn7 = new wxRadioButton(static_box_sizer3->GetStaticBox(), wxID_ANY, "Ninja debug");
	m_radioBtn7->SetValue(true);
	m_radioBtn7->SetValidator(wxGenericValidator(&m_preLaunchNinja));

	static_box_sizer3->Add(m_radioBtn7, 0, wxALL, 5);

	box_sizer2->Add(static_box_sizer3, 1, wxALL|wxEXPAND, 5);

	parent_sizer->Add(box_sizer2, 0, wxALL|wxEXPAND, 5);

	wxStdDialogButtonSizer* std_button_sizer = new wxStdDialogButtonSizer();

	std_button_sizerOK = new wxButton(this, wxID_OK);
	std_button_sizer->AddButton(std_button_sizerOK);

	std_button_sizerCancel = new wxButton(this, wxID_CANCEL);
	std_button_sizer->AddButton(std_button_sizerCancel);

	std_button_sizerOK->SetDefault();

	std_button_sizer->Realize();

	parent_sizer->Add(std_button_sizer, 0, wxEXPAND|wxALL, 5);

	SetSizerAndFit(parent_sizer);

	Centre(wxBOTH);

	// Event handlers
	Bind(wxEVT_INIT_DIALOG, &VsCodeDlgBase::OnInit, this);
}
