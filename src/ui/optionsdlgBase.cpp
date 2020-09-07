////////////////////////////////////////////////////////////////////////////
// Code generated by wxUiEditor ( https://github.com/KeyWorksRW/wxUiEditor/ )
//
// DO NOT EDIT THIS FILE! Your changes will be lost if it is re-generated!
////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/colour.h>
#include <wx/panel.h>
#include <wx/persist.h>
#include <wx/persist/toplevel.h>
#include <wx/radiobut.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/valgen.h>
#include <wx/valtext.h>

#include "optionsdlgBase.h"

OptionsDlgBase::OptionsDlgBase(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) :
	wxDialog(parent, id, title, pos, size, style)
{
	SetSizeHints(wxDefaultSize, wxDefaultSize);

	auto parent_sizer = new wxBoxSizer(wxHORIZONTAL);

	m_notebook = new wxNotebook(this, wxID_ANY);
	m_notebook->SetMinSize(wxSize(400,-1));

	auto m_panel = new wxPanel(m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	m_panel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

	auto parent_sizer2 = new wxBoxSizer(wxVERTICAL);

	auto box_sizer = new wxBoxSizer(wxHORIZONTAL);

	m_staticText = new wxStaticText(m_panel, wxID_ANY, "&Project Name:");
	m_staticText->Wrap(-1);
	box_sizer->Add(m_staticText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	auto textCtrl = new wxTextCtrl(m_panel, wxID_ANY);
	textCtrl->SetValidator(wxTextValidator(wxFILTER_NONE, &m_ProjectName));
	box_sizer->Add(textCtrl, 1, wxALL, 5);
	parent_sizer2->Add(box_sizer, 0, wxALL|wxEXPAND, 5);

	auto static_box = new wxStaticBoxSizer(new wxStaticBox(m_panel, wxID_ANY, "Project Type"), wxVERTICAL);

	auto grid_sizer = new wxGridSizer(0, 2, 0, 0);

	auto radioBtn = new wxRadioButton(static_box->GetStaticBox(), wxID_ANY, "Window");
	radioBtn->SetValidator(wxGenericValidator(&m_isWindow));
	grid_sizer->Add(radioBtn, 0, wxALL, 5);

	auto radioBtn2 = new wxRadioButton(static_box->GetStaticBox(), wxID_ANY, "Console");
	radioBtn2->SetValidator(wxGenericValidator(&m_isConsole));
	grid_sizer->Add(radioBtn2, 0, wxALL, 5);

	auto radioBtn3 = new wxRadioButton(static_box->GetStaticBox(), wxID_ANY, "Library");
	radioBtn3->SetValidator(wxGenericValidator(&m_isLibrary));
	grid_sizer->Add(radioBtn3, 0, wxALL, 5);

	auto radioBtn4 = new wxRadioButton(static_box->GetStaticBox(), wxID_ANY, "DLL");
	radioBtn4->SetValidator(wxGenericValidator(&m_isDll));
	grid_sizer->Add(radioBtn4, 0, wxALL, 5);
	static_box->Add(grid_sizer, 0, wxALL|wxEXPAND, 5);
	parent_sizer2->Add(static_box, 0, wxALL|wxEXPAND, 5);

	auto box_sizer2 = new wxBoxSizer(wxVERTICAL);

	auto staticText = new wxStaticText(m_panel, wxID_ANY, "&Target Directory");
	staticText->Wrap(-1);
	box_sizer2->Add(staticText, 0, wxALL, 5);

	m_TargetDirPicker = new wxDirPickerCtrl(m_panel, wxID_ANY, wxEmptyString, "Select a folder", wxDefaultPosition, wxDefaultSize, wxDIRP_DEFAULT_STYLE);
	box_sizer2->Add(m_TargetDirPicker, 0, wxALL|wxEXPAND, 5);
	parent_sizer2->Add(box_sizer2, 0, wxALL|wxEXPAND, 5);

	m_panel->SetSizerAndFit(parent_sizer2);
	m_notebook->AddPage(m_panel, "General", true);

	auto panel = new wxPanel(m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	panel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

	auto parent_sizer3 = new wxBoxSizer(wxVERTICAL);

	auto box_sizer3 = new wxBoxSizer(wxHORIZONTAL);

	auto staticText2 = new wxStaticText(panel, wxID_ANY, "&Optimize for");
	staticText2->Wrap(-1);
	box_sizer3->Add(staticText2, 0, wxALL, 5);

	auto radioBtn5 = new wxRadioButton(panel, wxID_ANY, "Space", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	radioBtn5->SetValue(true);
	radioBtn5->SetValidator(wxGenericValidator(&m_isSpaceOptimization));
	box_sizer3->Add(radioBtn5, 0, wxALL, 5);

	auto radioBtn6 = new wxRadioButton(panel, wxID_ANY, "Speed");
	radioBtn6->SetValidator(wxGenericValidator(&m_isSpeedOptimization));
	box_sizer3->Add(radioBtn6, 0, wxALL, 5);
	parent_sizer3->Add(box_sizer3, 0, wxALL|wxEXPAND, 5);

	auto box_sizer4 = new wxBoxSizer(wxHORIZONTAL);

	auto staticText3 = new wxStaticText(panel, wxID_ANY, "&Warning level:");
	staticText3->Wrap(-1);
	box_sizer4->Add(staticText3, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	auto spinCtrl = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 4, 4);
	box_sizer4->Add(spinCtrl, 0, wxALL, 5);
	parent_sizer3->Add(box_sizer4, 0, wxALL|wxEXPAND, 5);

	auto box_sizer5 = new wxBoxSizer(wxHORIZONTAL);

	auto staticText4 = new wxStaticText(panel, wxID_ANY, "PCH &Header:");
	staticText4->Wrap(-1);
	box_sizer5->Add(staticText4, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	m_PchHeaderPicker = new wxFilePickerCtrl(panel, wxID_ANY, wxEmptyString, "Select a file", "Header files|*.h", wxDefaultPosition, wxDefaultSize, wxFLP_FILE_MUST_EXIST|wxFLP_OPEN|wxFLP_SMALL|wxFLP_USE_TEXTCTRL);
	box_sizer5->Add(m_PchHeaderPicker, 0, wxALL, 5);
	parent_sizer3->Add(box_sizer5, 0, wxALL|wxEXPAND, 5);

	auto box_sizer6 = new wxBoxSizer(wxHORIZONTAL);

	auto staticText5 = new wxStaticText(panel, wxID_ANY, "PCH &Source:");
	staticText5->Wrap(-1);
	box_sizer6->Add(staticText5, 0, wxALL, 5);

	m_PchSrcPicker = new wxFilePickerCtrl(panel, wxID_ANY, wxEmptyString, "Select a file", "Header files|*.h", wxDefaultPosition, wxDefaultSize, wxFLP_FILE_MUST_EXIST|wxFLP_OPEN|wxFLP_SMALL|wxFLP_USE_TEXTCTRL);
	box_sizer6->Add(m_PchSrcPicker, 0, wxALL, 5);
	parent_sizer3->Add(box_sizer6, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5);

	auto box_sizer7 = new wxBoxSizer(wxHORIZONTAL);

	auto staticText6 = new wxStaticText(panel, wxID_ANY, "&Include Directories:");
	staticText6->Wrap(-1);
	box_sizer7->Add(staticText6, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	auto btnAddInclude = new wxButton(panel, wxID_ANY, "&Add..");
	box_sizer7->Add(btnAddInclude, 0, wxALL, 5);
	parent_sizer3->Add(box_sizer7, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5);

	auto box_sizer8 = new wxBoxSizer(wxHORIZONTAL);

	auto textCtrl2 = new wxTextCtrl(panel, wxID_ANY);
	textCtrl2->SetValidator(wxTextValidator(wxFILTER_NONE, &m_IncludeDirs));
	box_sizer8->Add(textCtrl2, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5);
	parent_sizer3->Add(box_sizer8, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5);

	auto static_box2 = new wxStaticBoxSizer(new wxStaticBox(panel, wxID_ANY, "Compiler Flags"), wxVERTICAL);

	auto flex_grid_sizer = new wxFlexGridSizer(0, 2, 0, 0);

	flex_grid_sizer->AddGrowableCol(1);
	flex_grid_sizer->SetFlexibleDirection(wxHORIZONTAL);
	flex_grid_sizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	auto staticText7 = new wxStaticText(static_box2->GetStaticBox(), wxID_ANY, "&Common:");
	staticText7->Wrap(-1);
	staticText7->SetToolTip("Compiler flags that will be used in all builds.");
	flex_grid_sizer->Add(staticText7, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	auto commonFlags = new wxTextCtrl(static_box2->GetStaticBox(), wxID_ANY);
	commonFlags->SetValidator(wxTextValidator(wxFILTER_NONE, &m_CommonCppFlags));
	commonFlags->SetToolTip("Compiler flags that will be used in all builds.");
	flex_grid_sizer->Add(commonFlags, 0, wxALL|wxEXPAND, 5);

	auto staticText8 = new wxStaticText(static_box2->GetStaticBox(), wxID_ANY, "&Release:");
	staticText8->Wrap(-1);
	flex_grid_sizer->Add(staticText8, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	auto releaseFlags = new wxTextCtrl(static_box2->GetStaticBox(), wxID_ANY);
	releaseFlags->SetValidator(wxTextValidator(wxFILTER_NONE, &m_ReleaseCppFlags));
	flex_grid_sizer->Add(releaseFlags, 0, wxALL|wxEXPAND, 5);

	auto staticText9 = new wxStaticText(static_box2->GetStaticBox(), wxID_ANY, "&Debug:");
	staticText9->Wrap(-1);
	staticText9->SetToolTip("Compiler flags that will only be used in a Debug build.");
	flex_grid_sizer->Add(staticText9, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	auto debugFlags = new wxTextCtrl(static_box2->GetStaticBox(), wxID_ANY);
	debugFlags->SetValidator(wxTextValidator(wxFILTER_NONE, &m_DebugCppFlags));
	debugFlags->SetToolTip("Compiler flags that will only be used in a Debug build.");
	flex_grid_sizer->Add(debugFlags, 0, wxALL|wxEXPAND, 5);
	static_box2->Add(flex_grid_sizer, 0, wxALL|wxEXPAND, 5);
	parent_sizer3->Add(static_box2, 0, wxALL|wxEXPAND, 5);

	panel->SetSizerAndFit(parent_sizer3);
	m_notebook->AddPage(panel, "Compiler", false);

	auto panel2 = new wxPanel(m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	panel2->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

	auto parent_sizer4 = new wxBoxSizer(wxVERTICAL);

	auto box_sizer9 = new wxBoxSizer(wxHORIZONTAL);

	auto staticText10 = new wxStaticText(panel2, wxID_ANY, "&Common Libraries:");
	staticText10->Wrap(-1);
	box_sizer9->Add(staticText10, 1, wxALL|wxALIGN_BOTTOM, 5);

	auto btnAddCommonLibrary = new wxButton(panel2, wxID_ANY, "&Add..");
	box_sizer9->Add(btnAddCommonLibrary, 0, wxALL, 5);
	parent_sizer4->Add(box_sizer9, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5);

	auto box_sizer10 = new wxBoxSizer(wxHORIZONTAL);

	auto commonLibs = new wxTextCtrl(panel2, wxID_ANY);
	commonLibs->SetValidator(wxTextValidator(wxFILTER_NONE, &m_CommonLibs));
	box_sizer10->Add(commonLibs, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5);
	parent_sizer4->Add(box_sizer10, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5);

	auto box_sizer11 = new wxBoxSizer(wxHORIZONTAL);

	auto staticText11 = new wxStaticText(panel2, wxID_ANY, "&Release Libraries:");
	staticText11->Wrap(-1);
	box_sizer11->Add(staticText11, 1, wxALL|wxALIGN_BOTTOM, 5);

	auto btnAddReleaseLibraries = new wxButton(panel2, wxID_ANY, "&Add..");
	box_sizer11->Add(btnAddReleaseLibraries, 0, wxALL, 5);
	parent_sizer4->Add(box_sizer11, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5);

	auto box_sizer14 = new wxBoxSizer(wxHORIZONTAL);

	auto releaseLibs = new wxTextCtrl(panel2, wxID_ANY);
	releaseLibs->SetValidator(wxTextValidator(wxFILTER_NONE, &m_ReleaseLibs));
	box_sizer14->Add(releaseLibs, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5);
	parent_sizer4->Add(box_sizer14, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5);

	auto box_sizer12 = new wxBoxSizer(wxHORIZONTAL);

	auto staticText12 = new wxStaticText(panel2, wxID_ANY, "&Debug Libraries:");
	staticText12->Wrap(-1);
	box_sizer12->Add(staticText12, 1, wxALL|wxALIGN_BOTTOM, 5);

	auto btnAddDebugLibraries = new wxButton(panel2, wxID_ANY, "&Add..");
	box_sizer12->Add(btnAddDebugLibraries, 0, wxALL, 5);
	parent_sizer4->Add(box_sizer12, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5);

	auto box_sizer15 = new wxBoxSizer(wxHORIZONTAL);

	auto debugLibs = new wxTextCtrl(panel2, wxID_ANY);
	debugLibs->SetValidator(wxTextValidator(wxFILTER_NONE, &m_DebugLibs));
	box_sizer15->Add(debugLibs, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5);
	parent_sizer4->Add(box_sizer15, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5);

	auto box_sizer13 = new wxBoxSizer(wxHORIZONTAL);

	auto staticText13 = new wxStaticText(panel2, wxID_ANY, "&Build and link to these libraries:");
	staticText13->Wrap(-1);
	box_sizer13->Add(staticText13, 1, wxALL|wxALIGN_BOTTOM, 5);

	auto btnAddBuildLibraries = new wxButton(panel2, wxID_ANY, "&Add..");
	box_sizer13->Add(btnAddBuildLibraries, 0, wxALL, 5);
	parent_sizer4->Add(box_sizer13, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5);

	auto box_sizer16 = new wxBoxSizer(wxHORIZONTAL);

	auto buildLibs3 = new wxTextCtrl(panel2, wxID_ANY);
	buildLibs3->SetValidator(wxTextValidator(wxFILTER_NONE, &m_BuildLibs));
	box_sizer16->Add(buildLibs3, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5);
	parent_sizer4->Add(box_sizer16, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5);

	panel2->SetSizerAndFit(parent_sizer4);
	m_notebook->AddPage(panel2, "Libs", false);

	auto panel3 = new wxPanel(m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	panel3->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

	auto parent_sizer5 = new wxBoxSizer(wxVERTICAL);

	auto static_box3 = new wxStaticBoxSizer(new wxStaticBox(panel3, wxID_ANY, "Resource compiler flags"), wxVERTICAL);

	auto flex_grid_sizer2 = new wxFlexGridSizer(0, 2, 0, 0);

	flex_grid_sizer2->AddGrowableCol(1);
	flex_grid_sizer2->SetFlexibleDirection(wxBOTH);
	flex_grid_sizer2->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	auto staticText14 = new wxStaticText(static_box3->GetStaticBox(), wxID_ANY, "Common:");
	staticText14->Wrap(-1);
	flex_grid_sizer2->Add(staticText14, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	auto commonRCFlags = new wxTextCtrl(static_box3->GetStaticBox(), wxID_ANY);
	commonRCFlags->SetValidator(wxTextValidator(wxFILTER_NONE, &m_commonRcFlags));
	flex_grid_sizer2->Add(commonRCFlags, 0, wxALL|wxEXPAND, 5);

	auto staticText15 = new wxStaticText(static_box3->GetStaticBox(), wxID_ANY, "Release:");
	staticText15->Wrap(-1);
	flex_grid_sizer2->Add(staticText15, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	auto releaseRCFlags = new wxTextCtrl(static_box3->GetStaticBox(), wxID_ANY);
	releaseRCFlags->SetValidator(wxTextValidator(wxFILTER_NONE, &m_releaseRcFlags));
	flex_grid_sizer2->Add(releaseRCFlags, 0, wxALL|wxEXPAND, 5);

	auto staticText16 = new wxStaticText(static_box3->GetStaticBox(), wxID_ANY, "Debug:");
	staticText16->Wrap(-1);
	flex_grid_sizer2->Add(staticText16, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	auto debugRCFlags = new wxTextCtrl(static_box3->GetStaticBox(), wxID_ANY);
	debugRCFlags->SetValidator(wxTextValidator(wxFILTER_NONE, &m_debugRcFlags));
	flex_grid_sizer2->Add(debugRCFlags, 0, wxALL|wxEXPAND, 5);
	static_box3->Add(flex_grid_sizer2, 0, wxALL|wxEXPAND, 5);
	parent_sizer5->Add(static_box3, 0, wxALL|wxEXPAND, 5);

	panel3->SetSizerAndFit(parent_sizer5);
	m_notebook->AddPage(panel3, "RC", false);

	auto panel5 = new wxPanel(m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	panel5->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

	auto parent_sizer7 = new wxBoxSizer(wxVERTICAL);

	auto static_box5 = new wxStaticBoxSizer(new wxStaticBox(panel5, wxID_ANY, "Additional flags for clang-cl (Windows)"), wxVERTICAL);

	auto flex_grid_sizer4 = new wxFlexGridSizer(0, 2, 0, 0);

	flex_grid_sizer4->AddGrowableCol(1);
	flex_grid_sizer4->SetFlexibleDirection(wxBOTH);
	flex_grid_sizer4->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	auto staticText20 = new wxStaticText(static_box5->GetStaticBox(), wxID_ANY, "Common:");
	staticText20->Wrap(-1);
	flex_grid_sizer4->Add(staticText20, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	auto commonFlags3 = new wxTextCtrl(static_box5->GetStaticBox(), wxID_ANY);
	commonFlags3->SetValidator(wxTextValidator(wxFILTER_NONE, &m_commonClangFlags));
	flex_grid_sizer4->Add(commonFlags3, 0, wxALL|wxEXPAND, 5);

	auto staticText21 = new wxStaticText(static_box5->GetStaticBox(), wxID_ANY, "Release:");
	staticText21->Wrap(-1);
	flex_grid_sizer4->Add(staticText21, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	auto releaseFlags3 = new wxTextCtrl(static_box5->GetStaticBox(), wxID_ANY);
	releaseFlags3->SetValidator(wxTextValidator(wxFILTER_NONE, &m_releaseClangFlags));
	flex_grid_sizer4->Add(releaseFlags3, 0, wxALL|wxEXPAND, 5);

	auto staticText22 = new wxStaticText(static_box5->GetStaticBox(), wxID_ANY, "Debug:");
	staticText22->Wrap(-1);
	flex_grid_sizer4->Add(staticText22, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	auto debugFlags3 = new wxTextCtrl(static_box5->GetStaticBox(), wxID_ANY);
	debugFlags3->SetValidator(wxTextValidator(wxFILTER_NONE, &m_debugClangFlags));
	flex_grid_sizer4->Add(debugFlags3, 0, wxALL|wxEXPAND, 5);
	static_box5->Add(flex_grid_sizer4, 0, wxALL|wxEXPAND, 5);

	auto box_sizer17 = new wxBoxSizer(wxHORIZONTAL);

	auto checkBox = new wxCheckBox(static_box5->GetStaticBox(), wxID_ANY, "Always use &MS Linker (link.exe)");
	checkBox->SetValidator(wxGenericValidator(&m_useMSLinker));
	box_sizer17->Add(checkBox, 0, wxALL, 5);
	static_box5->Add(box_sizer17, 0, wxALL|wxEXPAND, 5);
	parent_sizer7->Add(static_box5, 0, wxALL|wxEXPAND, 5);

	panel5->SetSizerAndFit(parent_sizer7);
	m_notebook->AddPage(panel5, "CLang", false);

	auto panel4 = new wxPanel(m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	panel4->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

	auto parent_sizer6 = new wxBoxSizer(wxVERTICAL);

	auto static_box4 = new wxStaticBoxSizer(new wxStaticBox(panel4, wxID_ANY, "MIDL compiler flags"), wxVERTICAL);

	auto flex_grid_sizer3 = new wxFlexGridSizer(0, 2, 0, 0);

	flex_grid_sizer3->AddGrowableCol(1);
	flex_grid_sizer3->SetFlexibleDirection(wxBOTH);
	flex_grid_sizer3->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	auto staticText17 = new wxStaticText(static_box4->GetStaticBox(), wxID_ANY, "Common:");
	staticText17->Wrap(-1);
	flex_grid_sizer3->Add(staticText17, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	auto commonFlags2 = new wxTextCtrl(static_box4->GetStaticBox(), wxID_ANY);
	commonFlags2->SetValidator(wxTextValidator(wxFILTER_NONE, &m_commonMidlFlags));
	flex_grid_sizer3->Add(commonFlags2, 0, wxALL|wxEXPAND, 5);

	auto staticText18 = new wxStaticText(static_box4->GetStaticBox(), wxID_ANY, "Release:");
	staticText18->Wrap(-1);
	flex_grid_sizer3->Add(staticText18, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	auto releaseFlags2 = new wxTextCtrl(static_box4->GetStaticBox(), wxID_ANY);
	releaseFlags2->SetValidator(wxTextValidator(wxFILTER_NONE, &m_releaseMidlFlags));
	flex_grid_sizer3->Add(releaseFlags2, 0, wxALL|wxEXPAND, 5);

	auto staticText19 = new wxStaticText(static_box4->GetStaticBox(), wxID_ANY, "Debug:");
	staticText19->Wrap(-1);
	flex_grid_sizer3->Add(staticText19, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	auto debugFlags2 = new wxTextCtrl(static_box4->GetStaticBox(), wxID_ANY);
	debugFlags2->SetValidator(wxTextValidator(wxFILTER_NONE, &m_debugMidlFlags));
	flex_grid_sizer3->Add(debugFlags2, 0, wxALL|wxEXPAND, 5);
	static_box4->Add(flex_grid_sizer3, 0, wxALL|wxEXPAND, 5);
	parent_sizer6->Add(static_box4, 0, wxALL|wxEXPAND, 5);

	panel4->SetSizerAndFit(parent_sizer6);
	m_notebook->AddPage(panel4, "MIDL", false);

	parent_sizer->Add(m_notebook, 1, wxEXPAND|wxALL, 5);

	SetSizerAndFit(parent_sizer);

	Centre(wxBOTH);

	SetName("OptionsDlgBase");
	wxPersistentRegisterAndRestore(this);

	// Event handlers
	btnAddInclude->Bind(wxEVT_BUTTON, &OptionsDlgBase::OnAddIncDir, this);
	btnAddCommonLibrary->Bind(wxEVT_BUTTON, &OptionsDlgBase::OnAddCommonLibraries, this);
	btnAddReleaseLibraries->Bind(wxEVT_BUTTON, &OptionsDlgBase::OnAddReleaseLibraries, this);
	btnAddDebugLibraries->Bind(wxEVT_BUTTON, &OptionsDlgBase::OnAddDebugLibraries, this);
	btnAddBuildLibraries->Bind(wxEVT_BUTTON, &OptionsDlgBase::OnAddBuildLibraries, this);
}