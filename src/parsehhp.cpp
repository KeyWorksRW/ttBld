/////////////////////////////////////////////////////////////////////////////
// Name:      CParseHHP
// Purpose:   Parse a .HHP file and generate a list of file dependencies
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttfile.h>      // ttCFile
#include <ttfindfile.h>  // ttCFindFile

#include "parsehhp.h"  // CParseHHP
#include "ninja.h"     // CNinja

extern const char* txtDefBuildDir;  // "bld";

CParseHHP::CParseHHP(const char* pszHHPName)
{
    m_section = SECTION_UNKNOWN;
    m_cszHHPName = pszHHPName;
    m_cszChmFile = (char*) m_cszHHPName;
    m_cszChmFile.ChangeExtension(".chm");
    m_cszRoot = pszHHPName;
    m_cszRoot.FullPathName();
    char* pszFile = ttFindFilePortion(m_cszRoot);
    if (pszFile)
        *pszFile = 0;
    m_cszCWD.GetCWD();
}

namespace
{
    const char* aOptions[] =  // array of options that specify files that will be compiled
        {
            "Contents file",
            "Index file",
            "DAT FILE",
            "Default topic",

            nullptr
        };
}

// This function can be called recursively if the .HHP file has a #include directive to #include another .hhp file

void CParseHHP::ParseHhpFile(const char* pszHHP)
{
    if (!pszHHP)
        pszHHP = m_cszHHPName;  // use root name

    ttCFile kf;
    if (!kf.ReadFile(pszHHP))
    {
        // REVIEW: [randalphwa - 11/29/2018] Need a way to add error msgs to CNinja
        //      ttCStr cszMsg;
        //      cszMsg.printf("Cannot read the file %s\n", pszHHP);
        //      AddError(cszMsg);
        return;
    }

    /*
        The following are possible sections. We only care about sections which will specify files that are to be compiled

        [ALIAS]
        [FILES]
        [INFOTYPES]
        [MAP]
        [MERGE FILES]
        [OPTIONS]
        [SUBSETS]
        [TEXT POPUPS]
        [WINDOWS]
    */

    while (kf.ReadLine())
    {
        if (!kf[0])
            continue;  // blank line
        if (ttIsSameSubStrI(kf, "#include"))
        {
            char* pszFile = ttFindNonSpace(ttFindSpace(kf));
            if (!*pszFile)
                continue;  // invalid #include -- doesn't specify a filename
            ttCStr cszFile;
            if (*pszFile == CH_QUOTE)
                cszFile.GetQuotedString(pszFile);
            else
                cszFile = pszFile;

            // [KeyWorksRW - 11-29-2018] It's not clear from HH docs when comments are allowed on a line -- we remove it
            // here just in case.

            char* pszComment = ttStrChr(cszFile, ';');
            if (pszComment)
                *pszComment = 0;
            ttTrimRight(cszFile);

            // A couple of sections sometimes include .h files -- but we only care about an #included .hhp file

            if (ttStrStrI(cszFile, ".hhp"))
                ParseHhpFile(cszFile);
            continue;
        }

        if (kf[0] == '[')  // sections are placed in brackets
        {
            m_section = SECTION_UNKNOWN;
            if (ttIsSameSubStrI(kf, "[ALIAS"))
                m_section = SECTION_ALIAS;
            else if (ttIsSameSubStrI(kf, "[FILE"))
                m_section = SECTION_FILES;
            else if (ttIsSameSubStrI(kf, "[OPTION"))
                m_section = SECTION_OPTIONS;
            continue;
        }

        switch (m_section)
        {
            case SECTION_UNKNOWN:
            default:
                break;

            case SECTION_OPTIONS:
            {
                size_t pos;
                for (pos = 0; aOptions[pos]; ++pos)
                {
                    if (ttIsSameSubStrI(kf, aOptions[pos]))
                        break;
                }
                if (aOptions[pos])
                {
                    char* pszFile = ttStrChr(kf, '=');
                    if (pszFile)
                    {
                        pszFile = ttFindNonSpace(pszFile + 1);
                        if (*pszFile)
                        {
                            // [KeyWorksRW - 11-29-2018] I don't believe the HH compiler from MS supports comments after filenames, but
                            // we can't be certain other compilers and/or authoring systems don't use them -- so remove any comment just
                            // to be sure.

                            char* pszComment = ttStrChr(pszFile, ';');
                            if (pszComment)
                                *pszComment = 0;
                            ttTrimRight(pszFile);
                            AddDependency(pszHHP, pszFile);
                        }
                    }
                }
                else if (ttIsSameSubStrI(kf, "Compiled file"))
                {
                    char* pszFile = ttStrChr(kf, '=');
                    if (pszFile)
                    {
                        char* pszComment = ttStrChr(pszFile, ';');
                        if (pszComment)
                            *pszComment = 0;
                        ttTrimRight(pszFile);
                        m_cszChmFile = ttFindNonSpace(pszFile + 1);
                    }
                }
            }
            break;

            case SECTION_ALIAS:
            {
                char* pszFile = ttStrChr(kf, ';');  // remove any comment
                if (pszFile)
                {
                    *pszFile = 0;
                    ttTrimRight(kf);
                }
                pszFile = ttStrChr(kf, '=');
                if (pszFile)
                {
                    pszFile = ttFindSpace(pszFile + 1);
                    if (*pszFile)
                    {
                        AddDependency(pszHHP, pszFile);
                    }
                }
            }
            break;

            case SECTION_TEXT_POPUPS:
            {
                char* pszFile = ttStrChr(kf, ';');  // remove any comment
                if (pszFile)
                {
                    *pszFile = 0;
                    ttTrimRight(kf);
                }

                // This section may include .h files that are parsed, but not compiled, but we need them for dependency checking.

                AddDependency(pszHHP, kf);
            }
            break;

            case SECTION_FILES:
            {
                // [KeyWorksRW - 11-29-2018] I don't think HHC actually allows comments in the [FILES] section,
                // though it should. I'm supporting comments here just to be sure we can support other file formats
                // like YAML which do allow for comments

                char* pszFile = ttStrChr(kf, ';');  // remove any comment
                if (pszFile)
                {
                    *pszFile = 0;
                    ttTrimRight(kf);
                }

                // This section may include .h files that are parsed, but not compiled, but we need them for dependency checking.

                AddDependency(pszHHP, kf);
            }
            break;
        }
    }
}

void CParseHHP::AddDependency(const char* pszHHP, const char* pszFile)
{
    /*
        The problem we need to solve with this function is that filename locations in the .hhp file will be relative to the
        .hhp file -- which in turn can be in a different location. So for example, you might have the following:

            ../Help/foo.hhp

        In foo.hhp, you might have:

            [FILES]
            html/bar.html

        We need to create a dependency to ../Help/html/bar.html
    */

    ttCStr cszRelative;
    ttCStr cszHHP;

    if (!ttIsSameStrI(pszHHP, m_cszHHPName))
    {
        // If we're in a nested .HHP file, then we need to first get the location of the nested .HHP, use that to get the location
        // of the pszFile;

        ttConvertToRelative(m_cszRoot, pszHHP, cszHHP);
        cszHHP.FullPathName();  // now that we have the location relative to our original .hhp file, convert it to a full path
        char* pszFilePortion = ttFindFilePortion(cszHHP);
        *pszFilePortion = 0;
        ttConvertToRelative(cszHHP, pszFile, cszRelative);
        cszHHP.AppendFileName(cszRelative);
        cszHHP.FullPathName();
        ttConvertToRelative(m_cszCWD, cszHHP, cszRelative);
    }
    else
    {
        cszHHP = (char*) m_cszRoot;
        cszHHP.AppendFileName(pszFile);
        ttConvertToRelative(m_cszCWD, cszHHP, cszRelative);
    }

    if (ttStrChr(cszRelative, '*') || ttStrChr(cszRelative, '?'))
    {
        ttCFindFile ff(cszRelative);
        char*       pszFilePortion = ttFindFilePortion(cszRelative);
        ptrdiff_t   cFilePortion = (pszFilePortion - cszRelative.GetPtr());
        if (ff.IsValid())
        {
            do
            {
                if (!ff.IsDir())
                {
                    cszRelative.GetPtr()[cFilePortion] = 0;
                    cszRelative.AppendFileName(ff);
                    m_lstDependencies += (const char*) cszRelative;
                }
            } while (ff.NextFile());
        }
        return;
    }

    // TODO: [KeyWorksRW - 11-29-2018]  If this is an HTML file, then we need to parse it to find additional dependencies

    m_lstDependencies += (const char*) cszRelative;
}

const char* txtHelpNinja = "build/ChmHelp.ninja";

bool CNinja::CreateHelpFile()
{
    if (ttIsEmpty(GetHHPName()))
        return false;

    if (!ttDirExists("build"))
    {
        if (!ttCreateDir("build"))
        {
            AddError("Unable to create the build directory -- so no place to put the .ninja files!\n");
            return false;
        }
    }

    CParseHHP chhp(GetHHPName());
    chhp.ParseHhpFile();  // this will figure out all of the dependencies
    m_cszChmFile = (const char*) chhp.m_cszChmFile;

    ttCFile file;
    file.SetUnixLF();  // WARNING!!! NINJA doesn't allow \r characters (or \t for that matter)
    file.printf("# WARNING: THIS FILE IS AUTO-GENERATED by %s. CHANGES YOU MAKE WILL BE LOST IF IT IS AUTO-GENERATED AGAIN.\n\n", txtVersion);
    file.WriteEol("ninja_required_version = 1.8\n");
    file.WriteStr("builddir = ");
    file.WriteEol(txtDefBuildDir);

    file.WriteEol("rule compile");
    file.WriteEol("  command = hhc.exe $in ");
    file.WriteEol("  description = compiling $out\n");

    if (chhp.m_lstDependencies.GetCount() > 0)
    {
        file.printf("build %s : compile %s | $\n", (char*) m_cszChmFile, GetHHPName());
        for (size_t pos = 0; chhp.m_lstDependencies.GetCount() - 1; ++pos)
        {
            file.printf("  %s $\n", chhp.m_lstDependencies[pos]);
        }
        file.printf("  %s\n", chhp.m_lstDependencies[chhp.m_lstDependencies.GetCount() - 1]);  // last one doesn't have trailing '$' character
    }
    else
    {
        file.printf("build %s : compile %s\n", (char*) m_cszChmFile, GetHHPName());
    }

    // We don't want to touch the build script if it hasn't changed, since that would change the timestamp causing git
    // and other tools that check timestamp to think something is different.

    ttCFile fileOrg;
    if (fileOrg.ReadFile(txtHelpNinja))
    {
        if (strcmp(fileOrg, file) == 0)
            return false;
        else if (m_dryrun.IsEnabled())
        {
            m_dryrun.NewFile(txtHelpNinja);
            m_dryrun.DisplayFileDiff(fileOrg, file);
            return false;  // we didn't actually write anything
        }
    }

    if (!file.WriteFile(txtHelpNinja))
    {
        ttCStr cszMsg;
        cszMsg.printf("Cannot write to %s\n", txtHelpNinja);
        AddError(cszMsg);
        return false;
    }

    return true;
}
