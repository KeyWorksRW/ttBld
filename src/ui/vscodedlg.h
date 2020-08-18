/////////////////////////////////////////////////////////////////////////////
// Purpose:   Dialog for setting options to create tasks.json and launch.json
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ttcvector.h>  // cstrVector -- Vector of ttlib::cstr strings

#include "vscodedlgBase.h"

class CSrcFiles;

class VsCodeDlg : public VsCodeDlgBase
{
public:
	VsCodeDlg(wxWindow* parent = nullptr);

	bool CreateVsCodeLaunch(CSrcFiles& cSrcFiles, ttlib::cstrVector& Results);
	bool CreateVsCodeTasks(CSrcFiles& cSrcFiles, ttlib::cstrVector& Results);

protected:

	// Handlers for VsCodeDlgBase events.

	void OnInit(wxInitDialogEvent& event) override;

private:
    bool m_hasMakefileTask;  // MSVC on Windows, GCC otherwise
    bool m_hasClangTask;
    bool m_hasNinjaTask;
    bool m_hasMingwMake;  // true if mingw32-make.exe has been located
};
