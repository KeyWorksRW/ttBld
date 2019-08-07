/////////////////////////////////////////////////////////////////////////////
// Name:      CWriteSrcFiles
// Purpose:   Class for writing out a new or updated version of .srcfiles.yaml
// Author:    Ralph Walden
// Copyright: Copyright (c) 1998-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <stdio.h>  // for sprintf

#include <ttfile.h> // ttCFile

#include "writesrcfiles.h"
#include "verninja.h"    // CVerMakeNinja
#include "options.h"     // CSrcOptions

extern sfopt::OPT_VERSION aoptVersions[];

// At the very least, we preserve any comments the user may have added, as well as any non-blank lines that we don't
// process (such as '------' used to delimit sections). We do prevent multiple blank lines, trailing spaces, and we do
// enforce LF EOL (CR/LF will be replaced with just LF). If it's a line we write, we may change the spacing used
// within the line.

// Put another way, we may change spacing and formating, but we will preserve any custom data like comments

const char* sfopt::txtNinjaVerFormat = "# Requires ttBld.exe version %d.%d.%d or higher to process";

bool CWriteSrcFiles::WriteUpdates(const char* pszFile)
{
#if defined(_DEBUG)
    // We need to be certain that the current ttBld version is at least as high as any option in the OPT_VERSION
    // array. There isn't a way to check that during compile time, so we check it in DEBUG builds whenever we try to
    // write the .srcfiles.yaml

    static bool bWarned = false;
    if (!bWarned)   // we can be called twice if a private Tab writes to a .private .srcfiles
    {
        bWarned = true;
        int major = 1;     // these three get filled in to the minum ttBld version required to process
        int minor = 0;
        int sub   = 0;

        for (size_t pos = 0; aoptVersions[pos].opt != OPT_OVERFLOW; ++pos)
        {
            if (aoptVersions[pos].major > major)
                major = aoptVersions[pos].major;
            if (aoptVersions[pos].minor > minor)
                minor = aoptVersions[pos].minor;
            if (aoptVersions[pos].sub > sub)
                sub = aoptVersions[pos].sub;
        }

        ttCStr cszVersion;
        cszVersion.printf(txtNinjaVerFormat, major, minor, sub);

        CVerMakeNinja verinfo;
        ttASSERT_MSG(!verinfo.IsSrcFilesNewer(cszVersion),
                     "An OPT_ in aoptVersions has a higher version number then txtOptVersion."
                     " If you write to .srcfiles.yaml without fixing the code first, this version of ttBld will no longer be able to use it.");
    }
#endif

    m_lstOriginal.SetFlags(ttCList::FLG_ADD_DUPLICATES);
    ttCFile kfIn;
    if (!kfIn.ReadFile(pszFile))
        return false;
    kfIn.MakeCopy();

    CVerMakeNinja verMakeNinja;

    bool bSeenVersion = false;
    while (kfIn.ReadLine())
    {
        if (ttIsSameSubStrI(kfIn, "# Requires ttBld"))
        {
            bSeenVersion = true;
            CVerMakeNinja verinfo;
            if (verinfo.IsSrcFilesNewer(kfIn)) // if .srcfiles is newer then our version, then leave the required version alone
            {
                m_lstOriginal += (const char*) kfIn;
            }
            else
            {
                ttCStr cszVersion;
                cszVersion.printf(txtNinjaVerFormat, GetMajorRequired(), GetMinorRequired(), GetSubRequired());
                m_lstOriginal += (char*) cszVersion;    // always start with the require minimum version
                m_lstOriginal += "";    // blank line after the version
            }
            continue;   // we've already added this
        }

        // Don't add any old options that have been replaced
        if (ttStrStr(kfIn, "TargetDirs:"))
            continue;
        else if (ttStrStr(kfIn, "LinkFlags:"))
            continue;
        else if (ttStrStr(kfIn, "bit_suffix:"))
            continue;
        else if (ttStrStr(kfIn, "static_crt:"))
            continue;
        else if (ttStrStr(kfIn, "CrtStaticRel:"))
            continue;
        else if (ttStrStr(kfIn, "CrtStaticDbg:"))
            continue;
        else if (ttStrStr(kfIn, "makefile:"))
            continue;
        else if (ttStrStr(kfIn, "compiler:"))
            continue;

        if (!bSeenVersion && ttIsSameSubStrI(kfIn, "Options:"))
        {
            bSeenVersion = true;
            ttCStr cszVersion;
            cszVersion.printf(txtNinjaVerFormat, GetMajorRequired(), GetMinorRequired(), GetSubRequired());
            m_lstOriginal += (char*) cszVersion;    // always start with the require minimum version
            m_lstOriginal += "";    // blank line after the version
        }

        m_lstOriginal += (const char*) kfIn;
    }

    ttCStr cszOrg;
    ttCStr cszComment;

    // The calling order below will determine the order Sections appear (if they don't already exist)

    PreProcessOptions();
    UpdateOptionsSection();

    ttCFile kfOut;

    // Write all the lines, but prevent more then one blank line at a time

    size_t cBlankLines = 0;
    for (size_t pos = 0; pos < m_lstOriginal.GetCount(); ++pos)
    {
        char* pszLine = (char*) m_lstOriginal[pos]; // since we're reducing the size, and about to delete it, okay to modify
        ttTrimRight(pszLine);

        // Add a blank line before 64Bit and IncDirs so that They are a bit easier to spot

        if (!cBlankLines && ttIsSameSubStr(ttFindNonSpace(m_lstOriginal[pos]), "64Bit"))
        {
            ++cBlankLines;
            kfOut.WriteEol();
            kfOut.WriteEol(m_lstOriginal[pos]);
            continue;
        }
        else if (!cBlankLines && ttIsSameSubStr(ttFindNonSpace(m_lstOriginal[pos]), "IncDirs"))
        {
            ++cBlankLines;
            kfOut.WriteEol();
            kfOut.WriteEol(m_lstOriginal[pos]);
            continue;
        }

        if (*pszLine)
           cBlankLines = 0;
        else if (cBlankLines > 0)   // ignore if last line was also blank
            continue;
        else
            ++cBlankLines;
        kfOut.WriteEol(m_lstOriginal[pos]);
    }

    kfIn.RestoreCopy();
    if (strcmp(kfIn, kfOut) == 0)
        return false;   // nothing has changed
    if (m_dryrun.IsEnabled())
    {
        m_dryrun.NewFile(pszFile);
        m_dryrun.DisplayFileDiff(kfIn, kfOut);
        return false;   // since we didn't actually change anything
    }
    return kfOut.WriteFile(pszFile);
}

bool CWriteSrcFiles::WriteNew(const char* pszFile, const char* pszCommentHdr)
{
    m_lstOriginal.SetFlags(ttCList::FLG_ADD_DUPLICATES);    // required to add blank lines
    if (pszCommentHdr)
    {
        m_lstOriginal += pszCommentHdr;
        m_lstOriginal += "";
    }
    m_lstOriginal += "Options:";
    if (m_lstSrcFiles.GetCount() || m_lstIdlFiles.GetCount() ||  m_cszRcName.IsNonEmpty())
    {
        ttCStr cszFile;
        m_lstOriginal += "";
        m_lstOriginal += "Files:";
        if (m_cszRcName.IsNonEmpty())
        {
            cszFile = "    ";
            cszFile += (const char*) m_cszRcName;
            m_lstOriginal += cszFile;
        }
        for (size_t pos = 0; pos < m_lstIdlFiles.GetCount(); ++pos)
        {
            cszFile = "    ";
            cszFile += m_lstIdlFiles[pos];
            m_lstOriginal += cszFile;
        }
        for (size_t pos = 0; pos < m_lstSrcFiles.GetCount(); ++pos)
        {
            cszFile = "    ";
            cszFile += m_lstSrcFiles[pos];
            m_lstOriginal += cszFile;
        }
    }

    UpdateOptionsSection(true);
    ttCFile kfOut;
    kfOut.SetUnixLF();

    for (size_t pos = 0; pos < m_lstOriginal.GetCount(); ++pos)
        kfOut.WriteEol(m_lstOriginal[pos]);
    return kfOut.WriteFile(pszFile);
}

ptrdiff_t CWriteSrcFiles::FindOption(const char* pszOption, ttCStr& cszDst)
{
    for (size_t pos = 0; pos < m_lstOriginal.GetCount(); ++pos)
    {
        if (ttIsSameSubStrI(ttFindNonSpace(m_lstOriginal[pos]), pszOption))
        {
            cszDst = m_lstOriginal[pos];
            return (ptrdiff_t) pos;
        }
    }
    return -1;
}

ptrdiff_t CWriteSrcFiles::FindSection(const char* pszSection)
{
    for (size_t pos = 0; pos < m_lstOriginal.GetCount(); ++pos)
    {
        if (ttIsSameSubStrI(m_lstOriginal[pos], pszSection))
        {
            return (ptrdiff_t) pos;
        }
    }
    return -1;
}

// The goal here is to see if an option was already specified, and if so update it to it's new value. If the option
// already had a comment, we leave it intact, otherwise we add our own.

// Note that some options must always be written -- those will be created if they don't already exist.

void CWriteSrcFiles::UpdateOptionsSection(bool bAddSpacing)
{
    m_posOptions = FindSection("Options:");
    if (m_posOptions < 0)
    {
        m_posOptions = 0;
        m_lstOriginal.InsertAt(m_posOptions, "Options:");
        m_posInsert = m_posOptions + 1;
        m_lstOriginal.InsertAt(m_posInsert, "");    // insert a blank line
    }
    else
    {
        // Set the insertion point at the first blank line, or before the next Section

        for (m_posInsert = m_posOptions + 1; m_posInsert < (ptrdiff_t) m_lstOriginal.GetCount(); ++m_posInsert)
        {
            if (ttStrLen(m_lstOriginal[m_posInsert]) < 1)
                break;
            else if (ttIsAlpha(m_lstOriginal[m_posInsert][0]))
            {
                m_lstOriginal.InsertAt(m_posInsert, "");    // insert a blank line
                break;
            }
        }
    }

    const sfopt::OPT_SETTING* aOptions = GetOrgOptions();

    if (bAddSpacing)
    {
        bool bSeenFlags = false;
        bool bSeenBits = false;
        bool bSeenDirs = false;

        for (size_t pos = 0; aOptions[pos].opt != OPT_OVERFLOW; ++pos)
        {
            switch (aOptions[pos].opt)
            {
                case OPT_CFLAGS_CMN:
                case OPT_CFLAGS_REL:
                case OPT_CFLAGS_DBG:
                    if (!bSeenFlags)
                    {
                        bSeenFlags = true;
                        m_lstOriginal.InsertAt(m_posInsert++, "");    // insert a blank line
                    }
                    break;

                case OPT_64BIT:
                case OPT_32BIT:
                    if (!bSeenBits)
                    {
                        bSeenBits = true;
                        m_lstOriginal.InsertAt(m_posInsert++, "");    // insert a blank line
                    }
                    break;

                case OPT_INC_DIRS:
                case OPT_BUILD_LIBS:
                case OPT_LIB_DIRS:
                case OPT_LIBS:
                case OPT_LIBS_REL:
                case OPT_LIBS_DBG:
                    if (!bSeenDirs)
                    {
                        bSeenDirs = true;
                        m_lstOriginal.InsertAt(m_posInsert++, "");    // insert a blank line
                    }
                    break;

                case OPT_OPTIMIZE:
                    m_lstOriginal.InsertAt(m_posInsert++, "");    // insert a blank line
                    break;

                default:
                    break;
            }
            UpdateWriteOption(pos);
        }
    }
    else
    {
        for (size_t pos = 0; aOptions[pos].opt != OPT_OVERFLOW; ++pos)
            UpdateWriteOption(pos);
    }
}

// If the option already exists then update it. If it doesn't exist, only write it if bAlwaysWrite is true

static const char* pszOptionFmt =      "    %-12s %-12s # %s";
static const char* pszLongOptionFmt =  "    %-12s %-30s    # %s";
static const char* pszNoCmtOptionFmt = "    %-12s %s";  // used when there isn't a comment

void CWriteSrcFiles::UpdateWriteOption(size_t pos)
{
    const sfopt::OPT_SETTING* aOptions = GetOrgOptions();
    ttCStr cszName(aOptions[pos].pszName);
    cszName += ":";    // add the colon that separates a YAML key/value pair

    char szLine[4096];
    ptrdiff_t posOption = GetOptionLine(aOptions[pos].pszName); // this will set m_cszOptComment
    if (posOption >= 0)
    {
        // The option already exists, so add any changes that might have been made

        if (m_cszOptComment.IsEmpty())      // we keep any comment that was previously used
            // REVIEW: [randalphwa - 3/13/2019] we don't allow changing the comment after it has been read in
            m_cszOptComment = aOptions[pos].pszComment;

        if (m_cszOptComment.IsNonEmpty() && ttStrLen(GetOptVal(pos)) > 12)
            // we use sprintf instead of ttCStr::printf because ttCStr doesn't support the %-12s width format specifier that we need
            sprintf_s(szLine, sizeof(szLine), pszLongOptionFmt,
                (char*) cszName, GetOptVal(pos), (char*) m_cszOptComment);
        else
            // we use sprintf instead of ttCStr::printf because ttCStr doesn't support the %-12s width format specifier that we need
            sprintf_s(szLine, sizeof(szLine), m_cszOptComment.IsNonEmpty() ? pszOptionFmt : pszNoCmtOptionFmt,
                (char*) cszName, GetOptVal(pos), (char*) m_cszOptComment);

        m_lstOriginal.Replace(posOption, szLine);   // replace the original line
    }
    else if (GetChanged(aOptions[pos].opt) || GetRequired(aOptions[pos].opt))
    {
        sprintf_s(szLine, sizeof(szLine), ttStrLen(GetOptVal(pos)) > 12 ? pszLongOptionFmt : pszOptionFmt,
            (char*) cszName,
                GetOptVal(pos) ? GetOptVal(pos) : "",
                aOptions[pos].pszComment ? aOptions[pos].pszComment : "");
        m_lstOriginal.InsertAt(m_posInsert++, szLine);
    }
}

// If option is found and it has a comment, m_cszOptComment will be set to the comment

ptrdiff_t CWriteSrcFiles::GetOptionLine(const char* pszOption)
{
    m_cszOptComment.Delete();
    for (ptrdiff_t pos = m_posOptions + 1; pos < (ptrdiff_t) m_lstOriginal.GetCount(); ++pos)
    {
        if (ttIsAlpha(*m_lstOriginal[pos]))
            break;  // New sections start with an alphabetical character in the first column

        const char* pszLine = ttFindNonSpace(m_lstOriginal[pos]);
        if (ttIsSameSubStrI(pszLine, pszOption))
        {
            const char* pszComment = ttStrChr(pszLine, '#');
            if (pszComment)
                m_cszOptComment = ttFindNonSpace(pszComment + 1);
            else
                m_cszOptComment.Delete();
            return pos;
        }
        // Options are supposed to be indented -- any alphabetical character that is non-indented presumably starts a new section
        else if (ttIsAlpha(*m_lstOriginal[pos]))
            break;
    }
    return -1;
}

void CWriteSrcFiles::PreProcessOptions()
{
    if (GetBoolOption(OPT_64BIT))
    {
        SetRequired(OPT_64BIT, true);
        SetRequired(OPT_TARGET_DIR64, true);
    }
    if (GetBoolOption(OPT_32BIT))
    {
        SetRequired(OPT_32BIT, true);
        SetRequired(OPT_TARGET_DIR32, true);
    }

    if (GetPchHeader())
    {
        ttCStr cszHdr(GetPchHeader());
        ttCStr cszSrc(GetPchCpp());     // GetPchCpp() will always return a string if GetPchHeader returns a string
        cszHdr.RemoveExtension();
        cszSrc.RemoveExtension();
        if (ttIsSameStrI(cszHdr, cszSrc))
            UpdateOption(OPT_PCH_CPP, "none");  // set it back to it's default value so it won't be written
    }
}
