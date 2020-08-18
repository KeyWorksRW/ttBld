/////////////////////////////////////////////////////////////////////////////
// Purpose:   Dialog specifying what to convert into a .srcfiles.yaml file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <wx/arrstr.h>  // wxArrayString class
#include <wx/dir.h>     // wxDir is a class for enumerating the files in a directory

#include <ttstr.h>  // ttString -- Enhanced version of wxString

#include "convertdlg.h"  // auto-generated: convertdlgBase.h and convertdlgBase.cpp

#include "funcs.h"  // List of function declarations

ConvertDlg::ConvertDlg(std::string_view projectFile) : ConvertDlgBase(nullptr)
{
    if (projectFile.size())
        m_cszOutSrcFiles.assign(projectFile);
    else
        m_cszOutSrcFiles = ".srcfiles.yaml";
}

void ConvertDlg::OnInit(wxInitDialogEvent& event)
{
    if (!ttlib::dir_exists(".vscode") && IsVsCodeAvail())
        m_AddVscodeDir = true;
    ;

    // These three directories are the only locations that gitIgnoreAll() will use, so if they don't exist, then there
    // isn't any way to add to the excludes list -- so just hide the control if that's the case.
    if (ttlib::dir_exists(".git") || ttlib::dir_exists("../.git") || ttlib::dir_exists("../../.git"))
        m_gitIgnore = true;
    else
        m_checkGitIgnore->Hide();

    ttString cwd;
    cwd.assignCwd();
	cwd.addtrailingslash();  // Need the trailing slash or dirPicker will think the last name is a file not a directory
    m_dirPickerList->SetDirName(cwd);

    ttString out_dir;

    if (!m_cszOutSrcFiles.empty())
    {
        out_dir = m_cszOutSrcFiles.wx_str();
        out_dir.make_relative_wx(cwd);
        out_dir.remove_filename();
        out_dir.backslashestoforward();
    }

    if (!out_dir.empty())
	{
        m_dirPickerOut->SetDirName(out_dir);
	}
    else
        m_dirPickerOut->SetDirName(cwd);

    wxDir dir;
    wxArrayString files;
    dir.GetAllFiles(".", &files, "*.vcxproj");  // Visual Studio
    dir.GetAllFiles(".", &files, "*.vcproj");   // Old Visual Studio
    dir.GetAllFiles(".", &files, "*.project");  // CodeLite
    dir.GetAllFiles(".", &files, "*.cbp");      // CodeBlocks
    dir.GetAllFiles(".", &files, "*.dsp");      // Very old Visual Studio project

	int sel = 0;
	for (const auto& file: files)
    {
        auto pos = m_choiceProjects->Append(file);
		if (ttlib::is_found(file.find(".vcxproj")))
			sel = pos;
    }
	m_choiceProjects->Select(sel);

	if (files.size())
		m_useProjectFile = true;
	else
		m_useAllFiles = true;

    event.Skip();  // transfer all validator data to their windows and update UI
}

void ConvertDlg::OnProjectFileLocated(wxFileDirPickerEvent& event)
{
	auto path = event.GetPath();
	if (path.empty() || ttlib::is_found(m_choiceProjects->FindString(path)))
		return;
	auto pos = m_choiceProjects->Append(path);
	if (ttlib::is_found(pos))
		m_choiceProjects->Select(pos);
}

void ConvertDlg::OnOK(wxCommandEvent& WXUNUSED(event))
{
    if (!Validate() || !TransferDataFromWindow())
        return;

    if (IsModal())
        EndModal(wxID_OK);
    else
    {
        SetReturnCode(wxID_OK);
        Show(false);
    }
}
