/////////////////////////////////////////////////////////////////////////////
// Name:      bldmaster.h
// Purpose:   Class providing interface to CSrcFiles
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

// All CSrcFiles members are public so they can be accessed directly, however this class provides a level of abstraction
// that allows CSrcFiles to change without breaking any of it's callers.

// In addition to the abstraction layer, this class also adds specific initialization and methods needed for actually
// creating build scripts. Note that nothing in this class is specific to building .ninja scripts -- which is why it's
// used by the classes that create IDE project files.

#pragma once

#include <ttlist.h>                     // ttCList, ttCDblList, ttCStrIntList

#include "csrcfiles.h"      // CSrcFiles
#include "dryrun.h"         // CDryRun

class CBldMaster : public CSrcFiles
{
public:
    CBldMaster(bool bVsCodeDir = false);

    // Public functions


    size_t      GetErrorCount() { return m_lstErrors.GetCount(); }
    const char* GetError(size_t pos) { return m_lstErrors[pos]; }
    void        AddError(const char* pszErrMsg) { m_lstErrors += pszErrMsg; }

    size_t getSrcCount() { return m_lstSrcFiles.GetCount(); }

    bool CreateMakeFile();

    const char* GetRcFile()     { return m_cszRcName; }

    ttCList* GetSrcFileList()  { return &m_lstSrcFiles; }
    ttCList* GetLibFileList()  { return &m_lstLibFiles; }
    ttCList* GetRcDepList()     { return &m_lstRcDependencies; }

    bool IsValidVersion() { return m_bInvalidVersion != true; }  // returns false if .srcfiles requires a newer version
    bool IsBin64()        { return m_bBin64Exists; }

    const char* GetLibName() { return m_cszLibName; }        // name and location of any additional library to build

    const char* GetHHPName() { return m_cszHHPName; }

    void EnableDryRun() { m_dryrun.Enable(); }

    ttCList m_lstRcDependencies;

protected:
    // Protected functions

    bool FindRcDependencies(const char* pszSrc, const char* pszHdr = nullptr, const char* pszRelPath = nullptr);
    const char* NormalizeHeader(const char* pszBaseFile, ttCStr& cszHeader);

    CDryRun m_dryrun;

private:
    // Class members

    bool m_bBin64Exists;        // if true, the directory ../bin64 exists
    bool m_bPrivateSrcfiles;

    bool m_bWritePrivate;    // if true, write to .private\build
    bool m_bInvalidVersion;  // true if a newer version is needed to parse the .srcfiles
};
