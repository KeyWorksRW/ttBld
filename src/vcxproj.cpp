/////////////////////////////////////////////////////////////////////////////
// Name:		CVcxProj
// Purpose:		Class for creating/maintaining .vcxproj file for use by the msbuild build tool (or VS IDE)
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2018 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#ifdef _WINDOWS_
#include <Rpc.h>
#pragma comment(lib, "Rpcrt4.lib")
#endif

#include "../ttLib/include/keyfile.h"		// CKeyFile

#include "resource.h"
#include "vcxproj.h"						// CVcxProj

bool CVcxProj::CreateBuildFile()
{
#ifndef _WINDOWS_
	return false;	// we only support create VisualStudio projects on Windows
#else
    UUID uuid;
	CStr cszGuid;
    RPC_STATUS ret_val = ::UuidCreate(&uuid);

    if (ret_val == RPC_S_OK) {
        RPC_CSTR* pszUuid = nullptr;
        ::UuidToStringA(&uuid, (RPC_CSTR*) &pszUuid);
        if (pszUuid) {
            cszGuid = (char*) pszUuid;
            ::RpcStringFreeA(pszUuid);
        }
    }
	if (cszGuid.IsEmpty()) {
		AddError("Unable to create a UUID -- cannot create .vcxproj without it.");
		return false;
	}

	CStr cszProjVC(getProjName());
	cszProjVC.ChangeExtension(".vcxproj");
	if (!FileExists(cszProjVC))	{
		CKeyFile kf;
		kf.ReadResource(IDR_VCXPROJ_MASTER);
		while (kf.ReplaceStr("%guid%", cszGuid));
		while (kf.ReplaceStr("%%DebugExe%", GetTargetDebug()));
		while (kf.ReplaceStr("%%ReleaseExe%", GetTargetRelease()));
		while (kf.ReplaceStr("%%DebugExe64%", GetTargetDebug64()));
		while (kf.ReplaceStr("%%ReleaseExe64%", GetTargetRelease64()));
		if (!kf.WriteFile(cszProjVC)) {
			CStr cszMsg;
			cszMsg.printf("Unable to write to %s", (char*) cszProjVC);
			AddError(cszMsg);
			return false;
		}

		// TODO: [randalphwa - 10-15-2018]	Add all the source files to project.vcxproj.filters

	}

	return true;
#endif
}
