/////////////////////////////////////////////////////////////////////////////
// Name:      CTabLibs
// Purpose:   IDTAB_LIBS dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019-2020 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttcwd.h>       // cwd -- Class for storing and optionally restoring the current directory
#include <ttopenfile.h>  // openfile -- Wrapper around Windows GetOpenFileName() API

#include "dlgoptions.h"
#include "strtable.h"  // String resource IDs

void CTabLibs::OnBegin(void)
{
    CHECK_DLG_ID(IDEDIT_LIBS_CMN);
    CHECK_DLG_ID(IDEDIT_LIBS_REL);
    CHECK_DLG_ID(IDEDIT_LIBS_DBG);
    CHECK_DLG_ID(IDEDIT_LIBS_BUILD);

    EnableShadeBtns();

    if (m_pOpts->hasOptValue(OPT::LIBS_CMN))
        SetControlText(IDEDIT_LIBS_CMN, m_pOpts->getOptValue(OPT::LINK_CMN));
    if (m_pOpts->hasOptValue(OPT::LIBS_REL))
        SetControlText(IDEDIT_LIBS_REL, m_pOpts->getOptValue(OPT::LIBS_REL));
    if (m_pOpts->hasOptValue(OPT::LIBS_DBG))
        SetControlText(IDEDIT_LIBS_DBG, m_pOpts->getOptValue(OPT::LIBS_DBG));
    if (m_pOpts->hasOptValue(OPT::BUILD_LIBS))
        SetControlText(IDEDIT_LIBS_BUILD, m_pOpts->getOptValue(OPT::BUILD_LIBS));
}

void CTabLibs::OnOK(void)
{
    m_pOpts->setOptValue(OPT::LIBS_CMN, GetControlText(IDEDIT_LIBS_CMN));
    m_pOpts->setOptValue(OPT::LIBS_REL, GetControlText(IDEDIT_LIBS_REL));
    m_pOpts->setOptValue(OPT::LIBS_DBG, GetControlText(IDEDIT_LIBS_DBG));
    m_pOpts->setOptValue(OPT::BUILD_LIBS, GetControlText(IDEDIT_LIBS_BUILD));
}

void CTabLibs::OnBtnAddBuildLibrary()
{
    ttlib::openfile fdlg(*this);
    fdlg.SetFilter("Project Files|.srcfiles*.yaml||");
    ttlib::cwd cwd;

    if (fdlg.GetOpenName())
    {
        fdlg.filename().remove_filename();
        // We need to see if this is the same path, so we need both paths to use forward slashes, and neither path to end with a slash.
        fdlg.filename().backslashestoforward();
        if (fdlg.filename().back() == '/')
            fdlg.filename().pop_back();
        cwd.backslashestoforward();
        if (cwd.back() == '/')
            cwd.pop_back();
        if (cwd.issameas(fdlg.filename(), tt::CASE::either))
        {
            ttlib::MsgBox(_tt(IDS_NO_RECURSIVE_BUILDLIB));
        }
        else
        {
            fdlg.filename().make_relative(cwd);
            fdlg.filename().backslashestoforward();
            ttlib::cstr update = GetControlText(IDEDIT_LIBS_BUILD);
            if (!ttlib::isFound(update.find(fdlg.filename())))
            {
                if (update.size() && update.back() != ';')
                    update += ';';
                update += fdlg.filename();
                SetControlText(IDEDIT_LIBS_BUILD, update);
            }
        }
    }
}

void CTabLibs::OnBtnAddLibCmn()
{
    auto result = AddLibrary(GetControlText(IDEDIT_LIBS_CMN));
    if (result)
        SetControlText(IDEDIT_LIBS_CMN, *result);
}

void CTabLibs::OnBtnAddLibDbg()
{
    auto result = AddLibrary(GetControlText(IDEDIT_LIBS_DBG));
    if (result)
        SetControlText(IDEDIT_LIBS_DBG, *result);
}

void CTabLibs::OnBtnAddLibRel()
{
    auto result = AddLibrary(GetControlText(IDEDIT_LIBS_REL));
    if (result)
        SetControlText(IDEDIT_LIBS_REL, *result);
}

std::optional<ttlib::cstr> CTabLibs::AddLibrary(std::string_view curLibs)
{
    ttlib::openfile fdlg(*this);
    fdlg.SetFilter("Library Files|*.lib||");
    ttlib::cwd cwd;

    if (!fdlg.GetOpenName())
        return {};

    fdlg.filename().make_relative(cwd);
    fdlg.filename().backslashestoforward();
    if (ttlib::contains(curLibs, fdlg.filename()))
        return {};

    ttlib::cstr old(curLibs);
    if (old.size() && old.back() != ';')
        old += ';';
    old += fdlg.filename();
    return { old };
}
