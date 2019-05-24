/////////////////////////////////////////////////////////////////////////////
// Name:      CVcxProj
// Purpose:   Class for creating/maintaining .vcxproj file for use by the msbuild build tool (or VS IDE)
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#ifdef _WINDOWS_
#include <Rpc.h>
#pragma comment(lib, "Rpcrt4.lib")
#endif

#include <ttfile.h>         // ttCFile
#include <ttfindfile.h>     // ttCFindFile

#include "resource.h"
#include "vcxproj.h"        // CVcxProj

static bool CreateGuid(ttCStr& cszGuid)
{
    UUID uuid;
    RPC_STATUS ret_val = ::UuidCreate(&uuid);

    if (ret_val == RPC_S_OK) {
        RPC_CSTR pszUuid = nullptr;
        if (::UuidToStringA(&uuid, &pszUuid) == RPC_S_OK && pszUuid) {
            cszGuid = (char*) pszUuid;
            ::RpcStringFreeA(&pszUuid);
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
#endif  // _WINDOWS_

    ttCStr cszGuid;
    if (!CreateGuid(cszGuid)) {
        AddError("Unable to create a UUID -- cannot create .vcxproj without it.");
        return false;
    }

    ttCStr cszProjVC(GetProjectName());
    cszProjVC.ChangeExtension(".vcxproj");
    if (!ttFileExists(cszProjVC))   {
        ttCFile kf;
        kf.ReadResource(IDR_VCXPROJ_MASTER);
        while (kf.ReplaceStr("%guid%", cszGuid));
        while (kf.ReplaceStr("%%DebugExe%", GetTargetDebug32()));
        while (kf.ReplaceStr("%%ReleaseExe%", GetTargetRelease32()));
        while (kf.ReplaceStr("%%DebugExe64%", GetTargetDebug64()));
        while (kf.ReplaceStr("%%ReleaseExe64%", GetTargetRelease64()));

        ttCStr cszSrcFile;
        for (size_t pos = 0; pos < m_lstSrcFiles.GetCount(); ++pos) {
            if (ttStrStrI(m_lstSrcFiles[pos], ".c")) {  // only add C/C++ files
                cszSrcFile.printf(" <ItemGroup>\n    <ClCompile Include=%kq />\n  </ItemGroup>", m_lstSrcFiles[pos]);
                kf.WriteEol(cszSrcFile);
            }
        }
        if (m_cszRcName.IsNonEmpty()) {
            cszSrcFile.printf(" <ItemGroup>\n    <ResourceCompile Include=%kq />\n  </ItemGroup>", (char*) m_cszRcName);
            kf.WriteEol(cszSrcFile);
        }

        ttCFindFile ff("*.h");  // add all header files in current directory
        if (ff.IsValid()) {
            do {
                cszSrcFile.printf(" <ItemGroup>\n    <ClInclude Include=%kq />\n  </ItemGroup>", (const char*) ff);
                kf.WriteEol(cszSrcFile);
            } while(ff.NextFile());
        }

        kf.WriteEol("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />");
        kf.WriteEol("  <ImportGroup Label=\"ExtensionTargets\">");
        kf.WriteEol("  </ImportGroup>");
        kf.WriteEol("</Project>");

        if (!kf.WriteFile(cszProjVC)) {
            ttCStr cszMsg;
            cszMsg.printf("Unable to write to %s", (char*) cszProjVC);
            AddError(cszMsg);
            return false;
        }
        else
            printf("Created %s\n",  (char*) cszProjVC);

        kf.Delete();
        kf.ReadResource(IDR_VCXPROJ_FILTERS);
        CreateGuid(cszGuid);    // it already succeeded once if we got here, so we don't check for error again
        while (kf.ReplaceStr("%guidSrc%", cszGuid));
        CreateGuid(cszGuid);
        while (kf.ReplaceStr("%guidHdr%", cszGuid));
        CreateGuid(cszGuid);
        while (kf.ReplaceStr("%guidResource%", cszGuid));

        kf.WriteEol("  <ItemGroup>");

        for (size_t pos = 0; pos < m_lstSrcFiles.GetCount(); ++pos) {
            if (ttStrStrI(m_lstSrcFiles[pos], ".c")) {  // only add C/C++ files
                cszSrcFile.printf("    <ClCompile Include=%kq>\n      <Filter>Source Files</Filter>\n    </ClCompile>", m_lstSrcFiles[pos]);
                kf.WriteEol(cszSrcFile);
            }
        }
        kf.WriteEol("  </ItemGroup>");
        kf.WriteEol("</Project>");
        cszProjVC += ".filters";
        if (!kf.WriteFile(cszProjVC)) {
            ttCStr cszMsg;
            cszMsg.printf("Unable to write to %s", (char*) cszProjVC);
            AddError(cszMsg);
            return false;
        }
        else
            printf("Created %s\n",  (char*) cszProjVC);

    }
    return true;
}
