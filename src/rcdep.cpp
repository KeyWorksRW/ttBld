/////////////////////////////////////////////////////////////////////////////
// Name:      rcdep.cpp
// Purpose:   Contains functions for parsing RC dependencies
// Author:    Ralph Walden
// Copyright: Copyright (c) 2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#if defined(_WIN32)  // only Windows builds use .rc files

#include "ninja.h"  // CNinja

// clang-format off
static const char* lstRcKeywords[] = {  // list of keywords that load a file
    "BITMP",
    "CURSOR",
    "FONT",
    "HTML",
    "ICON",
    "RCDATA",
    "TYPELIB",
    "MESSAGETABLE",

    nullptr
};
// clang-format on

// This is called to parse .rc files to find dependencies

bool CNinja::FindRcDependencies(const char* pszRcFile, const char* pszHdr, const char* pszRelPath)
{
    ttCFile kf;
    if (!kf.ReadFile(pszHdr ? pszHdr : pszRcFile))
    {
        if (!pszHdr)  // we should have already reported a problem with a missing header file
        {
            ttCStr cszErrMsg;
            cszErrMsg.printf(_("Cannot open \"%s\"."), pszRcFile);
            m_lstErrors += cszErrMsg;
        }
        return false;
    }

    // If passed a path, use that, otherwise see if the SrcFile contained a path, and if so, use that path.
    // If non-empty, the location of any header file is considered relative to the cszRelPath location

    ttCStr cszRelPath;
    if (pszRelPath)
        cszRelPath = pszRelPath;
    else
    {
        if (pszHdr)
        {
            char* pszFilePortion = ttFindFilePortion(pszHdr);
            if (pszFilePortion != pszHdr)
            {
                cszRelPath = pszHdr;
                pszFilePortion = ttFindFilePortion(cszRelPath);
                *pszFilePortion = 0;
            }
        }
        else
        {
            char* pszFilePortion = ttFindFilePortion(pszRcFile);
            if (pszFilePortion != pszRcFile)
            {
                cszRelPath = pszRcFile;
                pszFilePortion = ttFindFilePortion(cszRelPath);
                *pszFilePortion = 0;
            }
        }
    }

    size_t curLine = 0;
    while (kf.ReadLine())
    {
        ++curLine;
        if (ttIsSameSubStrI(ttFindNonSpace(kf), "#include"))
        {
            char* psz = ttFindNonSpace(ttFindNonSpace(kf) + sizeof("#include"));

            // We only care about header files in quotes -- we're not generating dependeices on system files (#include
            // <foo.h>)

            if (*psz == CH_QUOTE)
            {
                ttCStr cszHeader;
                cszHeader.GetQuotedString(psz);

                // Older versions of Visual Studio do not allow <> to be placed around header files. Since system header
                // files rarely change, and when they do they are not likely to require rebuilding our .rc file, we
                // simply ignore them.

                if (ttIsSameSubStrI(cszHeader, "afx") || ttIsSameSubStrI(cszHeader, "atl") ||
                    ttIsSameSubStrI(cszHeader, "winres"))
                    continue;

                NormalizeHeader(pszHdr ? pszHdr : pszRcFile, cszHeader);

                if (!ttFileExists(cszHeader))
                {
#if 0
                    // REVIEW: [randalphwa - 5/16/2019]  We can't really report this as an error unless we first check
                    // the INCLUDE environment variable as well as the IncDIRs option. The resource compiler is going to
                    // report the error, so there's not a huge advantage to reporting here.

                    ttCStr cszErrMsg;
                    cszErrMsg.printf(_(IDS_NINJA_MISSING_INCLUDE),
                        pszHdr ? pszHdr : pszRcFile, curLine, (size_t) (psz - kf.GetLnPtr()),  (char*) cszHeader);
                    m_lstErrors += cszErrMsg;
#endif
                    continue;
                }

                size_t posHdr = m_lstRcDependencies.GetPos(cszHeader);
                bool   bHdrSeenBefore = (posHdr != (size_t) -1);
                if (!bHdrSeenBefore)
                    posHdr = m_lstRcDependencies.Add(cszHeader);

                if (!bHdrSeenBefore)
                    FindRcDependencies(pszRcFile, cszHeader,
                                       cszRelPath);  // now search the header file for any #includes it might have
            }
        }

        // Not a header file, but might still be something we are dependent on

        else
        {
            char* pszKeyWord = ttFindNonSpace(kf);
            if (!pszKeyWord || pszKeyWord[0] == '/' ||
                pszKeyWord[0] == CH_QUOTE)  // TEXTINCLUDE typically puts things in quotes
                continue;                   // blank line or comment line
            pszKeyWord = ttFindSpace(pszKeyWord);
            if (!pszKeyWord)
                continue;  // means it's not a line that will include anything
            pszKeyWord = ttFindNonSpace(pszKeyWord);
            if (!pszKeyWord)
                continue;  // means it's not a line that will include anything

            for (size_t pos = 0; lstRcKeywords[pos]; ++pos)
            {
                if (ttIsSameSubStr(pszKeyWord, lstRcKeywords[pos]))
                {
                    const char* pszFileName = ttStrChr(pszKeyWord, CH_QUOTE);

                    // Some keywords use quotes which aren't actually filenames -- e.g., RCDATA { "string" }

                    if (pszFileName && ttStrChr(pszFileName + 1, CH_QUOTE) && !ttStrChr(pszFileName, '{'))
                    {
                        ttCStr cszFile;
                        cszFile.GetQuotedString(pszFileName);

                        // Backslashes are doubled -- so convert them into forward slashes

                        char* pszSlash = ttStrStr(cszFile, "\\\\");
                        if (pszSlash)
                        {
                            do
                            {
                                *pszSlash++ = '/';
                                ttStrCpy(pszSlash, pszSlash + 1);
                                pszSlash = ttStrStr(pszSlash, "\\\\");
                            } while (pszSlash);
                        }

                        if (pszHdr)
                        {
                            ttCStr cszHdr(pszHdr);
                            if (ttFileExists(cszHdr))  // we only want the directory
                            {
                                char* pszFilePortion = ttFindFilePortion(cszHdr);
                                *pszFilePortion = 0;
                            }
                            // First we normalize it to the header
                            NormalizeHeader(pszHdr, cszFile);
                            cszHdr.AppendFileName(cszFile);
                            // Then we normalize it to our RC file
                            NormalizeHeader(pszRcFile, cszHdr);
                            cszFile = cszHdr;
                        }
                        else
                            NormalizeHeader(pszRcFile, cszFile);

                        if (!ttFileExists(cszFile))
                        {
                            ttCStr cszErrMsg;
                            // BUGBUG: [KeyWorks - 7/11/2019] See Issue #46
                            // (https://github.com/KeyWorksRW/keyBld/issues/46) Once we commit to wxWidgets, we need to
                            // use wxNumberFormatter to deal with the number.
                            cszErrMsg.printf(_("%s(%kt,%kt):  warning: cannot locate include file %s"),
                                             pszHdr ? pszHdr : pszRcFile, curLine,
                                             (size_t)(pszFileName - kf.GetLnPtr()), (char*) cszFile);
                            m_lstErrors += cszErrMsg;
                            break;
                        }
                        size_t posHdr = m_lstRcDependencies.GetPos(cszFile);
                        bool   bHdrSeenBefore = (posHdr != (size_t) -1);
                        if (!bHdrSeenBefore)
                            posHdr = m_lstRcDependencies.Add(cszFile);
                    }
                    break;
                }
            }
        }
    }
    return true;
}

// We need all header files to use the same path for comparison purposes

const char* CNinja::NormalizeHeader(const char* pszRoot, ttCStr& cszHeader)
{
    ttASSERT(cszHeader.IsNonEmpty());

    if (pszRoot && *pszRoot)
        ttConvertToRelative(pszRoot, cszHeader, cszHeader);

    cszHeader.MakeLower();
    return cszHeader;
}

#endif
