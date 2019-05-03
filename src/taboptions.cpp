/////////////////////////////////////////////////////////////////////////////
// Name:		CTabOptions
// Purpose:		IDDLG_OPTIONS dialog handler
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "ttlibicons.h"
#include "strtable.h"

#include "taboptions.h"

bool ChangeOptions(bool bDryRun)
{
	CTabOptions dlg;
	if (bDryRun)
		dlg.EnableDryRun();

	HWND hwndDlg = dlg.DoModeless(NULL);

	// We aren't a window app, so we'll need to run a message loop here to handle the modeless dialog box

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!IsDialogMessage(hwndDlg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	auto result = (msg.message == WM_QUIT) ? msg.wParam : IDCANCEL;

	if (result == IDOK) {
		// dlg.SaveChanges();
		return true;
	}
	else
		return false;
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

	ti.pszText = (char*) tt::GetResString(IDS_TAB_LINKER);
	SendItemMsg(IDTAB, TCM_INSERTITEM, TAB_LINKER, (LPARAM) &ti);

	ti.pszText = (char*) tt::GetResString(IDS_TAB_SCRIPTS);
	SendItemMsg(IDTAB, TCM_INSERTITEM, TAB_SCRIPTS, (LPARAM) &ti);

	SendItemMsg(IDTAB, TCM_SETCURSEL, TAB_GENERAL);
	m_hwndTabSub = m_tabGeneral.DoModeless(*this);
	::ShowWindow(m_hwndTabSub, SW_SHOW);
}

void CTabOptions::OnOK(void)
{
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

					case TAB_LINKER:
						m_hwndTabSub = m_tabLinker.DoModeless(*this);
						break;

					case TAB_SCRIPTS:
						m_hwndTabSub = m_tabScripts.DoModeless(*this);
						break;

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
