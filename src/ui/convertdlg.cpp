/////////////////////////////////////////////////////////////////////////////
// Purpose:   Dialog specifying what to convert into a .srcfiles.yaml file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <wx/arrstr.h>  // wxArrayString class
#include <wx/dir.h>     // wxDir is a class for enumerating the files in a directory

#include "ttstr.h"  // ttString -- Enhanced version of wxString

#include "convertdlg.h"  // auto-generated: convertdlgBase.h and convertdlgBase.cpp

#include "convert.h"  // CConvert -- Class for converting project build files to .srcfiles.yaml
#include "funcs.h"    // List of function declarations
#include "uifuncs.h"  // Miscellaneous functions for displaying UI

ConvertDlg::ConvertDlg(std::string_view projectFile) : ConvertDlgBase(nullptr)
{
    if (projectFile.size())
        m_new_srcfiles.assign(projectFile);
    else
        m_new_srcfiles = ".srcfiles.yaml";
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

    if (!m_new_srcfiles.empty())
    {
        out_dir = m_new_srcfiles.wx_str();
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

    if (m_useProjectFile)
    {
        ttlib::cstr project_file = m_Project.utf8_str().data();
        auto extension = project_file.extension();
        if (!extension.empty())
        {
            bld::RESULT result = bld::failure;
            CConvert convert;
            if (extension.is_sameas(".vcxproj", tt::CASE::either))
            {
                result = convert.ConvertVcx(project_file, m_new_srcfiles);
            }
            else if (extension.is_sameas(".vcproj", tt::CASE::either))
            {
                result = convert.ConvertVc(project_file, m_new_srcfiles);
            }
            else if (extension.is_sameas(".dsp", tt::CASE::either))
            {
                result = convert.ConvertDsp(project_file, m_new_srcfiles);
            }
            else if (extension.is_sameas(".project", tt::CASE::either))
            {
                result = convert.ConvertCodeLite(project_file, m_new_srcfiles);
            }
            else if (project_file.is_sameprefix(".srcfiles", tt::CASE::either))
            {
                result = convert.ConvertSrcfiles(project_file, m_new_srcfiles);
            }

            if (result == bld::read_failed)
            {
                appMsgBox(ttlib::cstr("Cannot open ") << project_file, "Project conversion");
                return;
            }
            else if (result == bld::write_failed)
            {
                appMsgBox(ttlib::cstr("Unable to create or write to ") << m_new_srcfiles, "Project conversion");
                return;
            }

            // REVIEW: [KeyWorks - 03-09-2021] Theoretically, there are other errors that could occur, but currently none of
            // the conversion tools generate those other errors, so we assume that if we get here then the conversion was
            // successful.
        }
    }
    else if (m_useAllFiles)
    {
        CWriteSrcFiles srcfiles;
        srcfiles.InitOptions();

#if defined(_WIN32)
        srcfiles.AddSourcePattern("*.cpp;*.cc;*.cxx;*.rc;*.idl");
#else
        srcfiles.AddSourcePattern("*.cpp;*.cc;*.cxx");
#endif
        if (srcfiles.WriteNew(m_new_srcfiles) != bld::success)
        {
            appMsgBox(ttlib::cstr("Unable to create or write to ") << m_new_srcfiles, "Project conversion");
            return;
        }
    }

    EndModal(wxID_OK);
}
