/////////////////////////////////////////////////////////////////////////////
// Name:      CWriteSrcFiles
// Purpose:   Version of CSrcFiles that is capable of writing out a new or updated file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ttstr.h>   // ttCStr -- SBCS string class
#include <ttlist.h>  // ttCList, ttCDblList, ttCStrIntList

#include "csrcfiles.h"  // CSrcFiles
#include "dryrun.h"     // CDryRun

class CWriteSrcFiles : public CSrcFiles
{
public:
    CWriteSrcFiles(const char* pszNinjaDir = nullptr)
        : CSrcFiles(pszNinjaDir)
    {
        m_posOptions = -1;
        m_posInsert = 0;
    }

    typedef enum
    {
        RSLT_SUCCESS,
        RSLT_NOCHANGES,
        RSLT_DRYRUN,
        RSLT_WRITE_FAILED,
        RSLT_READ_FAILED,
    } CWRT_RESULT;

    // Public functions

    CWRT_RESULT WriteUpdates(const char* pszFile = txtSrcFilesFileName);  // write updates to the [OPTIONS] section

    // Writes complete .srcfiles.yaml file (replacing any file that already exists)
    bool WriteNew(const char* pszFile, const char* pszCommentHdr = nullptr);

    ttCList* GetOrgList() { return &m_lstOriginal; }

    void UpdateOptionsSection(bool bAddSpacing = false);
    void UpdateLongOption(const char* pszOption, const char* pszVal, const char* pszComment = nullptr);
    void UpdateShortOption(const char* pszOption, const char* pszVal, const char* pszComment,
                           bool bAlwaysWrite = false);
    void UpdateWriteOption(size_t pos);

    void EnableDryRun() { m_dryrun.Enable(); }

protected:
    // Protected functions

    void      PreProcessOptions();
    ptrdiff_t GetOptionLine(const char* pszOption);  // on success m_cszOptComment will be filled in
    ptrdiff_t FindOption(const char* pszOption, ttCStr& cszDst);
    ptrdiff_t FindSection(const char* pszSection);

private:
    // Class members

    ttCStr  m_cszOptComment;
    ttCList m_lstOriginal;
    CDryRun m_dryrun;

    ptrdiff_t m_posOptions;
    ptrdiff_t m_posInsert;
};
