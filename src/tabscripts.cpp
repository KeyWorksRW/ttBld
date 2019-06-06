/////////////////////////////////////////////////////////////////////////////
// Name:      CTabScripts
// Purpose:   IDTAB_SCRIPTS dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "dlgoptions.h"

void CTabScripts::OnBegin(void)
{
    if (m_pOpts->IsCodeBlockIDE())
        SetCheck(DLG_ID(IDCHECK_CODEBLOCKS));
    if (m_pOpts->IsCodeLiteIDE())
        SetCheck(DLG_ID(IDCHECK_CODELITE));
    if (m_pOpts->IsVisualStudioIDE())
        SetCheck(DLG_ID(IDCHECK_MSVC));
}

void CTabScripts::OnOK(void)
{
    ttCStr csz;

    if (GetCheck(DLG_ID(IDCHECK_CODEBLOCKS)))
        csz += "CodeBlocks ";
    if (GetCheck(DLG_ID(IDCHECK_CODELITE)))
        csz += "CodeLite ";
    if (GetCheck(DLG_ID(IDCHECK_MSVC)))
        csz += "VisualStudio ";

    if (csz.IsNonEmpty())
    {
        csz.TrimRight();
        m_pOpts->UpdateOption(OPT_IDE, (char*) csz);
    }
}

