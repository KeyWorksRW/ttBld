/////////////////////////////////////////////////////////////////////////////
// Name:      CTabOptions
// Purpose:   IDDLG_OPTIONS dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "ttlibicons.h"

#include "dlgoptions.h"

bool ChangeOptions(ttCStr* pcszSrcFiles, bool bDryRun)
{
    CTabOptions dlg;
    if (bDryRun)
        dlg.EnableDryRun();

    HWND hwndDlg = dlg.DoModeless(NULL);

    // We aren't a window app, so we need to run a message loop here to handle the modeless dialog box

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!IsDialogMessage(hwndDlg, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    auto result = (msg.message == WM_QUIT) ? msg.wParam : IDCANCEL;

    if (result == IDOK)
    {
        dlg.SaveChanges();
        *pcszSrcFiles = dlg.m_cszSrcFilePath;
        return true;
    }
    else
        return false;
}

CTabOptions::CTabOptions(bool bVsCodeDir) : ttCDlg(IDDLG_OPTIONS), CWriteSrcFiles(bVsCodeDir)
{
    m_tabGeneral.SetParentClass(this);
    m_tabCompiler.SetParentClass(this);
    m_tabCLang.SetParentClass(this);
    m_tabLinker.SetParentClass(this);
    m_tabRcMidl.SetParentClass(this);
#ifdef PRIVATE      // used for testing
    m_tabPrivate.SetParentClass(this);
#endif

    ReadFile(); // read in any existing .srcfiles

    if (ttIsEmpty(GetPchHeader()))
    {
        if (ttFileExists("stdafx.h"))
            UpdateOption(OPT_PCH, "stdafx.h");
        else if (ttFileExists("pch.h"))
            UpdateOption(OPT_PCH, "pch.h");
        else if (ttFileExists("precomp.h"))
            UpdateOption(OPT_PCH, "precomp.h");

        else if (ttFileExists("pch.hh"))
            UpdateOption(OPT_PCH, "pch.hh");
        else if (ttFileExists("pch.hpp"))
            UpdateOption(OPT_PCH, "pch.hpp");
        else if (ttFileExists("pch.hxx"))
            UpdateOption(OPT_PCH, "pch.hxx");
    }
}

void CTabOptions::OnBegin(void)
{
    EnableShadeBtns();
    SetBtnIcon(DLG_ID(IDOK), IDICON_TTLIB_OK);
    SetBtnIcon(DLG_ID(IDCANCEL), IDICON_TTLIB_CANCEL);

    TC_ITEMA ti;
    ZeroMemory(&ti, sizeof(TC_ITEMA));
    ti.iImage = -1;

    ti.mask = TCIF_TEXT;
    ti.pszText = (char*) GETSTRING(IDS_NINJA_TAB_GENERAL);

#ifdef _DEBUG
    auto result =
#endif
    SendItemMsg(IDTAB, TCM_INSERTITEMA, TAB_GENERAL, (LPARAM) &ti);
    ttASSERT(result == 0);

    ti.pszText = (char*) GETSTRING(IDS_NINJA_TAB_COMPILER);
    SendItemMsg(IDTAB, TCM_INSERTITEMA, TAB_COMPILER, (LPARAM) &ti);

    ti.pszText = (char*) "CLang";   // don't translate this
    SendItemMsg(IDTAB, TCM_INSERTITEMA, TAB_CLANG, (LPARAM) &ti);

    ti.pszText = (char*) GETSTRING(IDS_NINJA_TAB_LINKER);
    SendItemMsg(IDTAB, TCM_INSERTITEMA, TAB_LINKER, (LPARAM) &ti);

    ti.pszText = (char*) "Rc/Midl";   // don't translate this
    SendItemMsg(IDTAB, TCM_INSERTITEMA, TAB_RC_MIDL, (LPARAM) &ti);

    SendItemMsg(IDTAB, TCM_SETCURSEL, TAB_GENERAL);
    m_hwndTabSub = m_tabGeneral.DoModeless(*this);
    ::ShowWindow(m_hwndTabSub, SW_SHOW);
}

void CTabOptions::OnOK(void)
{
    ::SendMessage(m_hwndTabSub, WM_COMMAND, IDOK, 0);

#ifdef PRIVATE
    m_tabPrivate.SaveChanges();
#endif

    ::SendMessage(m_hwndTabSub, WM_CLOSE, 0, 0);
    DestroyWindow(*this);
    PostQuitMessage(IDOK);
}

void CTabOptions::OnCancel(void)
{
    ::SendMessage(m_hwndTabSub, WM_CLOSE, 0, 0);
    DestroyWindow(*this);
    PostQuitMessage(IDCANCEL);
}

void CTabOptions::SaveChanges()
{
    if (ttFileExists(m_cszSrcFilePath))
    {
        if (WriteUpdates(m_cszSrcFilePath))
            printf(GETSTRING(IDS_NINJA_OPTIONS_UPDATED), GetSrcFiles());
    }
    else
    {
        if (WriteUpdates())
            printf(GETSTRING(IDS_FILE_CREATED), GetSrcFiles());
    }
}

LRESULT CTabOptions::OnNotify(int /* id */, NMHDR* pNmHdr)
{
    switch (pNmHdr->code)
    {
        case TCN_SELCHANGING:
            ::SendMessage(m_hwndTabSub, WM_COMMAND, IDOK, 0);
            break;

        case TCN_SELCHANGE:
            {
                auto curTab = SendItemMsg(IDTAB, TCM_GETCURSEL);
                switch (curTab)
                {
                    case TAB_GENERAL:
                        m_hwndTabSub = m_tabGeneral.DoModeless(*this);
                        break;

                    case TAB_COMPILER:
                        m_hwndTabSub = m_tabCompiler.DoModeless(*this);
                        break;

                    case TAB_CLANG:
                        m_hwndTabSub = m_tabCLang.DoModeless(*this);
                        break;

                    case TAB_LINKER:
                        m_hwndTabSub = m_tabLinker.DoModeless(*this);
                        break;

                    case TAB_RC_MIDL:
                        m_hwndTabSub = m_tabRcMidl.DoModeless(*this);
                        break;
#ifdef PRIVATE
                    case TAB_PRIVATE:
                        m_hwndTabSub = m_tabPrivate.DoModeless(*this);
                        break;
#endif

                    default:
                        return 0;
                }
                ::ShowWindow(m_hwndTabSub, SW_SHOW);
            }
            break;

        default:
            break;
    }

    return 0;
}
