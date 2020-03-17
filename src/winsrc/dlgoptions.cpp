/////////////////////////////////////////////////////////////////////////////
// Name:      CTabOptions
// Purpose:   IDDLG_OPTIONS dialog handler
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttTR.h>  // Function for translating strings

#include "ttlibicons.h"

#include "dlgoptions.h"

bool ChangeOptions(std::string& ProjectFile)
{
    CTabOptions dlg;

    HWND hwndDlg = dlg.DoModeless(NULL);

    // ttBld isn't a window app, so run a message loop here to handle the modeless dialog box

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
        ProjectFile = dlg.GetSrcFilesName();
        return true;
    }
    else
        return false;
}

CTabOptions::CTabOptions()
    : ttCDlg(IDDLG_OPTIONS)
    , CWriteSrcFiles()
{
    m_tabGeneral.SetParentClass(this);
    m_tabCompiler.SetParentClass(this);
    m_tabLibs.SetParentClass(this);
    m_tabLinker.SetParentClass(this);
#if defined(_WIN32)
    m_tabRcMidl.SetParentClass(this);
    m_tabCLang.SetParentClass(this);
#endif

#ifdef PRIVATE  // used for testing
    m_tabPrivate.SetParentClass(this);
#endif

    ReadFile();  // read in any existing .srcfiles

    if (!hasOptValue(OPT::PCH))
    {
        if (ttFileExists("stdafx.h"))
            setOptValue(OPT::PCH, "stdafx.h");
        else if (ttFileExists("pch.h"))
            setOptValue(OPT::PCH, "pch.h");
        else if (ttFileExists("precomp.h"))
            setOptValue(OPT::PCH, "precomp.h");

        else if (ttFileExists("pch.hh"))
            setOptValue(OPT::PCH, "pch.hh");
        else if (ttFileExists("pch.hpp"))
            setOptValue(OPT::PCH, "pch.hpp");
        else if (ttFileExists("pch.hxx"))
            setOptValue(OPT::PCH, "pch.hxx");
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
    ti.pszText = (char*) (const char*) _tt("General");

#if !defined(NDEBUG)  // Starts debug section.
    auto result =
#endif
        SendItemMsg(IDTAB, TCM_INSERTITEMA, TAB_GENERAL, (LPARAM) &ti);
    assert(result == 0);

    ti.pszText = (char*) ((const char*) _tt("Compiler"));
    SendItemMsg(IDTAB, TCM_INSERTITEMA, TAB_COMPILER, (LPARAM) &ti);

    ti.pszText = (char*) ((const char*) _tt("Libs"));
    SendItemMsg(IDTAB, TCM_INSERTITEMA, TAB_LIBS, (LPARAM) &ti);

    ti.pszText = (char*) ((const char*) _tt("Linker"));
    SendItemMsg(IDTAB, TCM_INSERTITEMA, TAB_LINKER, (LPARAM) &ti);

#if defined(_WIN32)
    ti.pszText = (char*) "Rc/Midl";  // don't translate this
    SendItemMsg(IDTAB, TCM_INSERTITEMA, TAB_RC_MIDL, (LPARAM) &ti);

    // The following tab is for clang-cl, which is why it is Windows-only

    ti.pszText = (char*) "CLang";  // don't translate this
    SendItemMsg(IDTAB, TCM_INSERTITEMA, TAB_CLANG, (LPARAM) &ti);
#endif

#if defined(PRIVATE)
    ti.pszText = (char*) "Private";  // don't translate this
    SendItemMsg(IDTAB, TCM_INSERTITEMA, TAB_PRIVATE, (LPARAM) &ti);
#endif

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
    if (GetSrcFilesName().fileExists())
    {
        if (UpdateOptions(GetSrcFilesName()) == bld::success)
            std::cout << GetSrcFilesName() + _tt("%s Options: section updated.") << '\n';
    }
    else
    {
        if (UpdateOptions() == bld::success)
            std::cout << GetSrcFilesName() + _tt("%s created.") << '\n';
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

                case TAB_LIBS:
                    m_hwndTabSub = m_tabLibs.DoModeless(*this);
                    break;

                case TAB_LINKER:
                    m_hwndTabSub = m_tabLinker.DoModeless(*this);
                    break;

#if defined(_WIN32)
                case TAB_RC_MIDL:
                    m_hwndTabSub = m_tabRcMidl.DoModeless(*this);
                    break;

                case TAB_CLANG:
                    m_hwndTabSub = m_tabCLang.DoModeless(*this);
                    break;
#endif
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
