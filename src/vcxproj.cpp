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

static bool CreateGuid(CStr& cszGuid)
{
	UUID uuid;
	RPC_STATUS ret_val = ::UuidCreate(&uuid);

	if (ret_val == RPC_S_OK) {
		RPC_CSTR* pszUuid = nullptr;
		::UuidToStringA(&uuid, (RPC_CSTR*) &pszUuid);
		if (pszUuid) {
			cszGuid = (char*) pszUuid;
			::RpcStringFreeA(pszUuid);
		}
	}
	return cszGuid.IsNonEmpty();
}

bool CVcxProj::CreateBuildFile()
{
#ifndef _WINDOWS_
	// Currently we only support creating VisualStudio projects on Windows. To get this to work on another platform, a
	// replacement would be needed for creating a GUID, and the templates we store in the .rc file would need to be added
	// in a different way (perhaps including them directly into the source code instead of the resource).

	return false;
#endif	// _WINDOWS_

	CStr cszGuid;
	if (!CreateGuid(cszGuid)) {
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

		kf.Delete();
		kf.ReadResource(IDR_VCXPROJ_MASTER);
		CreateGuid(cszGuid);	// it already succeeded once if we got here, so we don't check for error again
		while (kf.ReplaceStr("%guidSrc%", cszGuid));
		CreateGuid(cszGuid);
		while (kf.ReplaceStr("%guidHdr%", cszGuid));
		CreateGuid(cszGuid);
		while (kf.ReplaceStr("%guidResource%", cszGuid));

		kf.WriteEol("  <ItemGroup>");

		CStr cszSrcFile;
		for (size_t pos = 0; pos < m_lstSrcFiles.GetCount(); ++pos) {
			cszSrcFile.printf("    <ClCompile Include=%kq>\n      <Filter>Source Files</Filter>\n    </ClCompile>", m_lstSrcFiles[pos]);
			kf.WriteEol(cszSrcFile);
		}
		kf.WriteEol("  </ItemGroup>");
		kf.WriteEol("</Project>");
		cszProjVC += ".filters";
		if (!kf.WriteFile(cszProjVC)) {
			CStr cszMsg;
			cszMsg.printf("Unable to write to %s", (char*) cszProjVC);
			AddError(cszMsg);
			return false;
		}
	}
	return true;
}
