/////////////////////////////////////////////////////////////////////////////
// Name:		CTabOptions
// Purpose:		IDDLG_OPTIONS dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

// As a way to ensure that .private/.srcfiles projects works correctly, we conditionalize a private tab to add to the end
// of the tab list. However, the files to support the tab are not part of this project (which is the whole point of a
// .private version). See https://github.com/KeyWorksRW/MakeNinja/issues/60 for more information.

#include "pch.h"

#include "ttlibicons.h"
#include "strtable.h"

#include "dlgoptions.h"

bool ChangeOptions(bool bDryRun)
{
	CTabOptions dlg;
	if (bDryRun)
		dlg.EnableDryRun();

	HWND hwndDlg = dlg.DoModeless(NULL);

	// We aren't a window app, so we need to run a message loop here to handle the modeless dialog box

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!IsDialogMessage(hwndDlg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	auto result = (msg.message == WM_QUIT) ? msg.wParam : IDCANCEL;

	if (result == IDOK) {
		dlg.SaveChanges();
		return true;
	}
	else
		return false;
}

CTabOptions::CTabOptions() : ttCDlg(IDDLG_OPTIONS)
{
	m_tabGeneral.SetParentClass(this);
	m_tabCompiler.SetParentClass(this);
	m_tabCLang.SetParentClass(this);
	m_tabLinker.SetParentClass(this);
	m_tabRcMidl.SetParentClass(this);
	m_tabScripts.SetParentClass(this);
#ifdef PRIVATE
	m_tabPrivate.SetParentClass(this);
#endif

	ReadFile();	// read in any existing .srcfiles

	if (tt::IsEmpty(GetPchHeader())) {
		if (tt::FileExists("stdafx.h"))
			UpdateOption(OPT_PCH, "stdafx.h");
		else if (tt::FileExists("pch.h"))
			UpdateOption(OPT_PCH, "pch.h");
		else if (tt::FileExists("precomp.h"))
			UpdateOption(OPT_PCH, "precomp.h");

		else if (tt::FileExists("pch.hh"))
			UpdateOption(OPT_PCH, "pch.hh");
		else if (tt::FileExists("pch.hpp"))
			UpdateOption(OPT_PCH, "pch.hpp");
		else if (tt::FileExists("pch.hxx"))
			UpdateOption(OPT_PCH, "pch.hxx");
	}
}

void CTabOptions::OnBegin(void)
{
	EnableShadeBtns();
	SetBtnIcon(DLG_ID(IDOK), IDICON_TTLIB_OK);
	SetBtnIcon(DLG_ID(IDCANCEL), IDICON_TTLIB_CANCEL);

	TC_ITEM ti;
	ZeroMemory(&ti, sizeof(TC_ITEM));
	ti.iImage = -1;

	ti.mask = TCIF_TEXT;
	ti.pszText = (char*) tt::GetResString(IDS_GENERAL);

#ifdef _DEBUG
	auto result =
#endif
	SendItemMsg(IDTAB, TCM_INSERTITEM, TAB_GENERAL, (LPARAM) &ti);
	ttASSERT(result == 0);

	ti.pszText = (char*) tt::GetResString(IDS_TAB_COMPILER);
	SendItemMsg(IDTAB, TCM_INSERTITEM, TAB_COMPILER, (LPARAM) &ti);

	ti.pszText = (char*) tt::GetResString(IDS_TAB_CLANG);
	SendItemMsg(IDTAB, TCM_INSERTITEM, TAB_CLANG, (LPARAM) &ti);

	ti.pszText = (char*) tt::GetResString(IDS_TAB_LINKER);
	SendItemMsg(IDTAB, TCM_INSERTITEM, TAB_LINKER, (LPARAM) &ti);

	ti.pszText = (char*) tt::GetResString(IDS_TAB_RC_MIDL);
	SendItemMsg(IDTAB, TCM_INSERTITEM, TAB_RC_MIDL, (LPARAM) &ti);

	ti.pszText = (char*) tt::GetResString(IDS_TAB_SCRIPTS);
	SendItemMsg(IDTAB, TCM_INSERTITEM, TAB_SCRIPTS, (LPARAM) &ti);

#ifdef PRIVATE
	if (tt::FileExists(".private/.srcfiles")) {
		ti.pszText = (char*) tt::GetResString(IDS_TAB_PRIVATE);
		SendItemMsg(IDTAB, TCM_INSERTITEM, TAB_PRIVATE, (LPARAM) &ti);
	}
#endif

	SendItemMsg(IDTAB, TCM_SETCURSEL, TAB_GENERAL);
	m_hwndTabSub = m_tabGeneral.DoModeless(*this);
	::ShowWindow(m_hwndTabSub, SW_SHOW);
}

void CTabOptions::OnOK(void)
{
#ifdef PRIVATE
	m_tabPrivate.SaveChanges();
#endif

	::SendMessage(m_hwndTabSub, WM_COMMAND, IDOK, 0);
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
	if (tt::FileExists(txtSrcFilesFileName)) {
		if (WriteUpdates())
			puts(".srcfiles Options: section updated");
	}
	else {
		if (WriteUpdates())
			puts(".srcfiles created");
	}
}

LRESULT CTabOptions::OnNotify(int /* id */, NMHDR* pNmHdr)
{
	switch (pNmHdr->code) {
		case TCN_SELCHANGING:
			::SendMessage(m_hwndTabSub, WM_COMMAND, IDOK, 0);
			break;

		case TCN_SELCHANGE:
			{
				auto curTab = SendItemMsg(IDTAB, TCM_GETCURSEL);
				switch (curTab) {
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

					case TAB_SCRIPTS:
						m_hwndTabSub = m_tabScripts.DoModeless(*this);
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
