////////////////////////////////////////////////////////////////////////////////
// Code generated by wxUiEditor -- see https://github.com/KeyWorksRW/wxUiEditor/
//
// DO NOT EDIT THIS FILE! Your changes will be lost if it is re-generated!
////////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/valgen.h>

#include "vscodedlgBase.h"

VsCodeDlgBase::VsCodeDlgBase(wxWindow* parent) : wxDialog()
{
    Create(parent, wxID_ANY, wxString::FromUTF8("Create .vscode files"));

    auto parent_sizer = new wxBoxSizer(wxVERTICAL);

    auto box_sizer = new wxBoxSizer(wxHORIZONTAL);
    parent_sizer->Add(box_sizer, wxSizerFlags().Expand().Border(wxALL));

    auto static_box = new wxStaticBoxSizer(wxVERTICAL, this, wxString::FromUTF8("tasks.json"));
    box_sizer->Add(static_box, wxSizerFlags().Expand().Border(wxALL));

    m_checkBox = new wxCheckBox(static_box->GetStaticBox(), wxID_ANY, wxString::FromUTF8("MSVC builds"));
    m_checkBox->SetValidator(wxGenericValidator(&m_taskMSVCBuild));
    static_box->Add(m_checkBox, wxSizerFlags().Border(wxALL));

    m_checkBox2 = new wxCheckBox(static_box->GetStaticBox(), wxID_ANY, wxString::FromUTF8("CLANG builds"));
    m_checkBox2->SetValidator(wxGenericValidator(&m_taskCLANGBuild));
    static_box->Add(m_checkBox2, wxSizerFlags().Border(wxALL));

    m_checkBox3 = new wxCheckBox(static_box->GetStaticBox(), wxID_ANY, wxString::FromUTF8("Ninja debug build"));
    m_checkBox3->SetValue(true);
    m_checkBox3->SetValidator(wxGenericValidator(&m_taskNinjaBuild));
    static_box->Add(m_checkBox3, wxSizerFlags().Border(wxALL));

    auto static_box_sizer2 = new wxStaticBoxSizer(wxVERTICAL, this, wxString::FromUTF8("Default task"));
    box_sizer->Add(static_box_sizer2, wxSizerFlags().Expand().Border(wxALL));

    m_radioBtn = new wxRadioButton(static_box_sizer2->GetStaticBox(), wxID_ANY, wxString::FromUTF8("MSVC debug"), wxDefaultPosition, wxDefaultSize,
    wxRB_GROUP);
    m_radioBtn->SetValidator(wxGenericValidator(&m_defTaskMSVC));
    static_box_sizer2->Add(m_radioBtn, wxSizerFlags().Border(wxALL));

    m_radioBtn2 = new wxRadioButton(static_box_sizer2->GetStaticBox(), wxID_ANY, wxString::FromUTF8("CLANG debug"));
    m_radioBtn2->SetValidator(wxGenericValidator(&m_defTaskCLANG));
    static_box_sizer2->Add(m_radioBtn2, wxSizerFlags().Border(wxALL));

    m_radioBtn3 = new wxRadioButton(static_box_sizer2->GetStaticBox(), wxID_ANY, wxString::FromUTF8("Ninja debug"));
    m_radioBtn3->SetValidator(wxGenericValidator(&m_defTaskNinja));
    static_box_sizer2->Add(m_radioBtn3, wxSizerFlags().Border(wxALL));

    auto box_sizer2 = new wxBoxSizer(wxHORIZONTAL);
    parent_sizer->Add(box_sizer2, wxSizerFlags().Expand().Border(wxALL));

    auto static_box_sizer3 = new wxStaticBoxSizer(wxVERTICAL, this, wxString::FromUTF8("launch.json Pre-launch build"));
    box_sizer2->Add(static_box_sizer3, wxSizerFlags(1).Expand().Border(wxALL));

    m_radioBtn4 = new wxRadioButton(static_box_sizer3->GetStaticBox(), wxID_ANY, wxString::FromUTF8("none"), wxDefaultPosition, wxDefaultSize,
    wxRB_GROUP);
    m_radioBtn4->SetValidator(wxGenericValidator(&m_preLaunchNone));
    static_box_sizer3->Add(m_radioBtn4, wxSizerFlags().Border(wxALL));

    m_radioBtn5 = new wxRadioButton(static_box_sizer3->GetStaticBox(), wxID_ANY, wxString::FromUTF8("MSVC debug"));
    m_radioBtn5->SetValidator(wxGenericValidator(&m_preLaunchMSVC));
    static_box_sizer3->Add(m_radioBtn5, wxSizerFlags().Border(wxALL));

    m_radioBtn6 = new wxRadioButton(static_box_sizer3->GetStaticBox(), wxID_ANY, wxString::FromUTF8("CLANG debug"));
    m_radioBtn6->SetValidator(wxGenericValidator(&m_preLaunchCLANG));
    static_box_sizer3->Add(m_radioBtn6, wxSizerFlags().Border(wxALL));

    m_radioBtn7 = new wxRadioButton(static_box_sizer3->GetStaticBox(), wxID_ANY, wxString::FromUTF8("Ninja debug"));
    m_radioBtn7->SetValidator(wxGenericValidator(&m_preLaunchNinja));
    static_box_sizer3->Add(m_radioBtn7, wxSizerFlags().Border(wxALL));

    auto stdBtn = CreateStdDialogButtonSizer(wxOK|wxCANCEL);
    parent_sizer->Add(CreateSeparatedSizer(stdBtn), wxSizerFlags().Expand().Border(wxALL));

    SetSizerAndFit(parent_sizer);
    Centre(wxBOTH);

    // Event handlers
    Bind(wxEVT_INIT_DIALOG, &VsCodeDlgBase::OnInit, this);
}
