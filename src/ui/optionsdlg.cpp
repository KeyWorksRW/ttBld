/////////////////////////////////////////////////////////////////////////////
// Purpose:   Dialog for setting all .srcfile options
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <wx/dirdlg.h>   // wxDirDialog base class
#include <wx/filedlg.h>  // wxFileDialog base header

#include <ttmultistr.h>  // multistr -- Breaks a single string into multiple strings

#include "optionsdlg.h"  // auto-generated: optionsdlgBase.h and optionsdlgBase.cpp

#include "uifuncs.h"  // Miscellaneous functions for displaying UI

OptionsDlg::OptionsDlg(const std::string& ProjectFile) : OptionsDlgBase(nullptr)
{
    ReadFile(ProjectFile);  // read in any existing .srcfiles

    m_cwd.assignCwd();
    m_cwd.backslashestoforward();  // We do this on Windows to use for comparison later
    if (m_cwd.Last() == '/')
        m_cwd.RemoveLast();

    m_PchHeaderPicker->SetInitialDirectory(m_cwd);
    m_PchSrcPicker->SetInitialDirectory(m_cwd);
    m_NatvisPicker->SetInitialDirectory(m_cwd);
    m_TargetDirPicker->SetInitialDirectory(m_cwd);

    if (!HasPch())
    {
        if (ttlib::file_exists("stdafx.h"))
            setOptValue(OPT::PCH, "stdafx.h");
        else if (ttlib::file_exists("pch.h"))
            setOptValue(OPT::PCH, "pch.h");
        else if (ttlib::file_exists("precomp.h"))
            setOptValue(OPT::PCH, "precomp.h");

        else if (ttlib::file_exists("pch.hh"))
            setOptValue(OPT::PCH, "pch.hh");
        else if (ttlib::file_exists("pch.hpp"))
            setOptValue(OPT::PCH, "pch.hpp");
        else if (ttlib::file_exists("pch.hxx"))
            setOptValue(OPT::PCH, "pch.hxx");
    }

    if (hasOptValue(OPT::PROJECT))
    {
        m_ProjectName = GetProjectName().wx_str();
    }
    else
    {
        // If project name not specified, use the current directory name as the project name.

        // If current directory is src or source, then the project name should be the parent directory name.
        if (m_cwd.filename().is_sameas("src", tt::CASE::either) || m_cwd.filename().is_sameas("source", tt::CASE::either))
        {
            ttString cwd(m_cwd);
            cwd.remove_filename();

            m_ProjectName = cwd.filename();
        }
    }

    if (IsExeTypeConsole())
        m_isConsole = true;
    else if (IsExeTypeLib())
        m_isLibrary = true;
    else if (getOptValue(OPT::EXE_TYPE).is_sameas("dll", tt::CASE::either))
        m_isDll = true;
    else if (getOptValue(OPT::EXE_TYPE).is_sameas("ocx", tt::CASE::either))
        m_isOcx = true;
    else
        m_isWindow = true;

    if (hasOptValue(OPT::TARGET_DIR))
    {
        m_TargetDirPicker->SetPath(getOptValue(OPT::TARGET_DIR).wx_str());
    }

    if (IsOptimizeSpeed())
    {
        m_isSpaceOptimization = false;
        m_isSpeedOptimization = true;
    }

    m_WarningLevel = ttlib::atoi(getOptValue(OPT::WARN));

    if (hasOptValue(OPT::PCH))
        m_PchHeaderPicker->SetPath(getOptValue(OPT::PCH).wx_str());
    if (hasOptValue(OPT::PCH_CPP))
        m_PchSrcPicker->SetPath(getOptValue(OPT::PCH_CPP).wx_str());
    if (hasOptValue(OPT::INC_DIRS))
        m_IncludeDirs = getOptValue(OPT::INC_DIRS).wx_str();

    if (hasOptValue(OPT::CFLAGS_CMN))
        m_CommonCppFlags = getOptValue(OPT::CFLAGS_CMN);
    if (hasOptValue(OPT::CFLAGS_REL))
        m_ReleaseCppFlags = getOptValue(OPT::CFLAGS_REL);
    if (hasOptValue(OPT::CFLAGS_DBG))
        m_DebugCppFlags = getOptValue(OPT::CFLAGS_DBG);

    if (hasOptValue(OPT::LIBS_CMN))
        m_CommonLibs = getOptValue(OPT::LINK_CMN);
    if (hasOptValue(OPT::LIBS_REL))
        m_ReleaseLibs = getOptValue(OPT::LIBS_REL);
    if (hasOptValue(OPT::LIBS_DBG))
        m_DebugLibs = getOptValue(OPT::LIBS_DBG);
    if (hasOptValue(OPT::BUILD_LIBS))
        m_BuildLibs = getOptValue(OPT::BUILD_LIBS);

    if (hasOptValue(OPT::RC_CMN))
        m_commonRcFlags = getOptValue(OPT::RC_CMN);
    if (hasOptValue(OPT::RC_REL))
        m_releaseRcFlags = getOptValue(OPT::RC_REL);
    if (hasOptValue(OPT::RC_DBG))
        m_debugRcFlags = getOptValue(OPT::RC_DBG);

    if (hasOptValue(OPT::MIDL_CMN))
        m_commonMidlFlags = getOptValue(OPT::MIDL_CMN);
    if (hasOptValue(OPT::MIDL_REL))
        m_releaseMidlFlags = getOptValue(OPT::MIDL_REL);
    if (hasOptValue(OPT::MIDL_DBG))
        m_debugMidlFlags = getOptValue(OPT::MIDL_DBG);

    if (hasOptValue(OPT::CLANG_CMN))
        m_commonClangFlags = getOptValue(OPT::CLANG_CMN);
    if (hasOptValue(OPT::CLANG_REL))
        m_releaseClangFlags = getOptValue(OPT::CLANG_REL);
    if (hasOptValue(OPT::CLANG_DBG))
        m_debugClangFlags = getOptValue(OPT::CLANG_DBG);

    m_useMSLinker = isOptTrue(OPT::MS_LINKER);

    m_isReleaseDllCRT = !IsStaticCrtRel();
    m_isDebugDllCRT = !IsStaticCrtDbg();

    if (hasOptValue(OPT::LINK_CMN))
        m_CommonLinkFlags = getOptValue(OPT::LINK_CMN);
    if (hasOptValue(OPT::LINK_REL))
        m_ReleaseLinkFlags = getOptValue(OPT::LINK_REL);
    if (hasOptValue(OPT::LINK_DBG))
        m_DebugLinkFlags = getOptValue(OPT::LINK_DBG);

    if (hasOptValue(OPT::NATVIS))
        m_NatvisPicker->SetPath(getOptValue(OPT::NATVIS).wx_str());
    else
    {
        if (ttlib::file_exists("../ttLib/ttlibwin.natvis"))
        {
            m_NatvisPicker->SetPath("../ttLib/ttlibwin.natvis");
        }
        else if (ttlib::file_exists("../ttLib/ttlibwin.natvis"))
        {
            m_NatvisPicker->SetPath("../ttLib/ttlibwin.natvis");
        }
    }
}

void OptionsDlg::SaveChanges()
{
    if (m_ProjectName.size())
        setOptValue(OPT::PROJECT, ttlib::cstr() << m_ProjectName.wx_str());

    if (m_isConsole)
        setOptValue(OPT::EXE_TYPE, "console");
    else if (m_isWindow)
        setOptValue(OPT::EXE_TYPE, "window");
    else if (m_isLibrary)
        setOptValue(OPT::EXE_TYPE, "lib");
    else if (m_isDll)
        setOptValue(OPT::EXE_TYPE, "dll");
    else if (m_isOcx)
        setOptValue(OPT::EXE_TYPE, "ocx");

    ttString path;

    // Note that we always call path.backslashestoforward(); -- this is for consistency so that it doesn't matter if the
    // path is being used on Windows, Unix, or Mac. (Windows works just fine using forward slashes, it just defaults to
    // backslashes).

    if (m_PchHeaderPicker->GetPath().size())
    {
        path = m_PchHeaderPicker->GetPath();
        path.make_relative_wx(m_cwd);
        path.backslashestoforward();
        setOptValue(OPT::PCH, ttlib::cstr() << path.wx_str());
    }
    else
        setOptValue(OPT::PCH, {});

    if (m_PchSrcPicker->GetPath().size())
    {
        path = m_PchSrcPicker->GetPath();
        path.make_relative_wx(m_cwd);
        path.backslashestoforward();
        setOptValue(OPT::PCH_CPP, ttlib::cstr() << path.wx_str());
    }
    else
        setOptValue(OPT::PCH_CPP, {});

    setOptValue(OPT::OPTIMIZE, m_isSpaceOptimization ? "space" : "speed");

    setOptValue(OPT::WARN, ttlib::itoa(m_WarningLevel));

    setOptValue(OPT::CRT_DBG, m_isDebugDllCRT ? "dll" : "static");
    if (!m_isReleaseDllCRT)
        setOptValue(OPT::CRT_REL, "static");

    if (m_TargetDirPicker->GetPath().size())
    {
        ttString path = m_TargetDirPicker->GetPath();
        path.make_relative_wx(m_cwd);
        path.backslashestoforward();
        setOptValue(OPT::TARGET_DIR, ttlib::cstr() << path.wx_str());
    }
    else
        setOptValue(OPT::TARGET_DIR, "");

    setOptValue(OPT::CFLAGS_CMN, ttlib::cstr() << m_CommonCppFlags.wx_str());
    setOptValue(OPT::CFLAGS_REL, ttlib::cstr() << m_ReleaseCppFlags.wx_str());
    setOptValue(OPT::CFLAGS_DBG, ttlib::cstr() << m_DebugCppFlags.wx_str());

    setOptValue(OPT::CLANG_CMN, ttlib::cstr() << m_commonClangFlags.wx_str());
    setOptValue(OPT::CLANG_REL, ttlib::cstr() << m_releaseClangFlags.wx_str());
    setOptValue(OPT::CLANG_DBG, ttlib::cstr() << m_debugClangFlags.wx_str());

    setOptValue(OPT::LIBS_CMN, ttlib::cstr() << m_CommonLibs.wx_str());
    setOptValue(OPT::LIBS_REL, ttlib::cstr() << m_ReleaseLibs.wx_str());
    setOptValue(OPT::LIBS_DBG, ttlib::cstr() << m_DebugLibs.wx_str());

    setOptValue(OPT::LINK_CMN, ttlib::cstr() << m_CommonLinkFlags.wx_str());
    setOptValue(OPT::LINK_REL, ttlib::cstr() << m_ReleaseLinkFlags.wx_str());
    setOptValue(OPT::LINK_DBG, ttlib::cstr() << m_DebugLinkFlags.wx_str());

    setOptValue(OPT::RC_CMN, ttlib::cstr() << m_commonRcFlags.wx_str());
    setOptValue(OPT::RC_REL, ttlib::cstr() << m_releaseRcFlags.wx_str());
    setOptValue(OPT::RC_DBG, ttlib::cstr() << m_debugRcFlags.wx_str());

    setOptValue(OPT::MIDL_CMN, ttlib::cstr() << m_commonMidlFlags.wx_str());
    setOptValue(OPT::MIDL_REL, ttlib::cstr() << m_releaseMidlFlags.wx_str());
    setOptValue(OPT::MIDL_DBG, ttlib::cstr() << m_debugMidlFlags.wx_str());

    if (m_NatvisPicker->GetPath().size())
    {
        path = m_NatvisPicker->GetPath();
        path.make_relative_wx(m_cwd);
        path.backslashestoforward();
        setOptValue(OPT::NATVIS, ttlib::cstr() << path.wx_str());
    }
    else
        setOptValue(OPT::NATVIS, {});

    setBoolOptValue(OPT::MS_LINKER, m_useMSLinker);

    setOptValue(OPT::INC_DIRS, ttlib::cstr() << m_IncludeDirs.wx_str());
    setOptValue(OPT::BUILD_LIBS, ttlib::cstr() << m_BuildLibs.wx_str());

    if (GetSrcFilesName().file_exists())
    {
        if (UpdateOptions(GetSrcFilesName()) == bld::success)
            std::cout << GetSrcFilesName() + _tt(strIdOptionsUpdated) << '\n';
    }
    else
    {
        if (UpdateOptions() == bld::success)
            std::cout << GetSrcFilesName() + _tt(strIdCreatedSuffix) << '\n';
    }
}

void OptionsDlg::OnAddIncDir(wxCommandEvent& WXUNUSED(event))
{
    wxDirDialog dlg(this, wxT("Include Directory"), m_cwd);
    if (dlg.ShowModal() == wxID_OK)
    {
        ttString path = dlg.GetPath();
        path.make_relative_wx(m_cwd);
#if defined(_WIN32)
        path.backslashestoforward();
#endif
        ttString olddirs = m_textIncludeDirs->GetValue();
        if (!olddirs.contains_wx(path))
        {
            if (olddirs.size() && olddirs.Last() != ';')
                olddirs += ';';
            olddirs += path;
            m_textIncludeDirs->SetValue(olddirs);
        }
    }
}

void OptionsDlg::OnAddCommonLibraries(wxCommandEvent& WXUNUSED(event))
{
#if defined(_WIN32)
    wxFileDialog openFileDialog(this, "Library", m_cwd, "", "Library files (*.lib)|*.lib", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
#else
    wxFileDialog openFileDialog(this, "Library", m_cwd, "", "Library files (*.so)|*.so", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
#endif  // _WIN32
    if (openFileDialog.ShowModal() != wxID_OK)
        return;

    ttString new_lib = openFileDialog.GetPath();
    new_lib.make_relative_wx(m_cwd);
    new_lib.backslashestoforward();

    auto result = AddLibrary(m_commonLibs->GetValue(), new_lib);
    if (result)
        m_commonLibs->SetValue(result->wx_str());
}

void OptionsDlg::OnAddReleaseLibraries(wxCommandEvent& WXUNUSED(event))
{
#if defined(_WIN32)
    wxFileDialog openFileDialog(this, "Library", m_cwd, "", "Library files (*.lib)|*.lib", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
#else
    wxFileDialog openFileDialog(this, "Library", m_cwd, "", "Library files (*.so)|*.so", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
#endif  // _WIN32
    if (openFileDialog.ShowModal() != wxID_OK)
        return;

    ttString new_lib = openFileDialog.GetPath();
    new_lib.make_relative_wx(m_cwd);
    new_lib.backslashestoforward();

    auto result = AddLibrary(m_releaseLibs->GetValue(), new_lib);
    if (result)
        m_releaseLibs->SetValue(result->wx_str());
}

void OptionsDlg::OnAddDebugLibraries(wxCommandEvent& WXUNUSED(event))
{
#if defined(_WIN32)
    wxFileDialog openFileDialog(this, "Library", m_cwd, "", "Library files (*.lib)|*.lib", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
#else
    wxFileDialog openFileDialog(this, "Library", m_cwd, "", "Library files (*.so)|*.so", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
#endif  // _WIN32
    if (openFileDialog.ShowModal() != wxID_OK)
        return;

    ttString new_lib = openFileDialog.GetPath();
    new_lib.make_relative_wx(m_cwd);
    new_lib.backslashestoforward();

    auto result = AddLibrary(m_debugLibs->GetValue(), new_lib);
    if (result)
        m_debugLibs->SetValue(result->wx_str());
}

void OptionsDlg::OnAddBuildLibraries(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog openFileDialog(this, "Library", m_cwd, "", "Project Files|.srcfiles*.yaml||", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() != wxID_OK)
        return;

    ttString path = openFileDialog.GetPath();

    path.remove_filename();
    // We need to see if this is the same path, so we need both paths to use forward slashes, and neither path to end with a slash.
    path.backslashestoforward();
    if (path.Last() == '/')
        path.RemoveLast();

    if (m_cwd.is_sameas_wx(path, tt::CASE::either))
    {
        appMsgBox(_tt(strIdRecursiveBld));
    }
    else
    {
        path.make_relative_wx(m_cwd);
        path.backslashestoforward();

        auto result = AddLibrary(m_buildLibs->GetValue(), path);
        if (result)
            m_buildLibs->SetValue(result->wx_str());
    }
}

void OptionsDlg::OnTargetDirChanged(wxFileDirPickerEvent& WXUNUSED(event))
{
    ttString path = m_TargetDirPicker->GetPath();
    path.make_relative_wx(m_cwd);
#if defined(_WIN32)
    path.backslashestoforward();
#endif  // _WIN32
    m_TargetDirPicker->SetPath(path);
}

void OptionsDlg::OnPchHeaderChanged(wxFileDirPickerEvent& WXUNUSED(event))
{
    ttString path = m_PchHeaderPicker->GetPath();
    path.make_relative_wx(m_cwd);
#if defined(_WIN32)
    path.backslashestoforward();
#endif  // _WIN32
    m_PchHeaderPicker->SetPath(path);
}

void OptionsDlg::OnPchSrcChanged(wxFileDirPickerEvent& WXUNUSED(event))
{
    ttString path = m_PchSrcPicker->GetPath();
    path.make_relative_wx(m_cwd);
#if defined(_WIN32)
    path.backslashestoforward();
#endif  // _WIN32
    m_PchSrcPicker->SetPath(path);
}

std::optional<ttlib::cstr> OptionsDlg::AddLibrary(const wxString& cur_libs, const wxString& new_libs)
{
    if (new_libs.empty())
        return {};

    if (cur_libs.empty())
    {
        ttString old(new_libs);
        return { old };
    }

    ttlib::multistr current(cur_libs.utf8_str().data());
    ttlib::multistr addition(new_libs.utf8_str().data());

    ttlib::cstr updated(cur_libs.wx_str());

    // Make certain the library hasn't already been added
    for (auto& iter_new: addition)
    {
        iter_new.make_relative(m_cwd.utf8_str().data());
        iter_new.backslashestoforward();

        for (auto& iter_current: current)
        {
            if (iter_new.is_sameas(iter_current, tt::CASE::either))
                return {};  // It's already been added
        }

        if (updated.back() != ';')
            updated += ';';
        updated += iter_new;
        current.SetString(updated);
    }
    return { updated };
}
