/////////////////////////////////////////////////////////////////////////////
// Name:      CParseHHP
// Purpose:   Parse a .HHP file and generate a list of file dependencies
// Author:    Ralph Walden
// Copyright: Copyright (c) 2018-2019 KeyWorks Software (Ralph Walden)
// License:   Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttTR.h>        // Function for translating strings
#include <ttlibspace.h>  // Contains the ttlib namespace functions/declarations common to all ttLib libraries
#include <tttextfile.h>  // Classes for reading and writing line-oriented files

#include <ttfile.h>      // ttCFile
#include <ttfindfile.h>  // ttCFindFile

#include "ninja.h"     // CNinja
#include "parsehhp.h"  // CParseHHP

CParseHHP::CParseHHP(const char* pszHHPName)
{
    m_section = SECTION_UNKNOWN;
    m_HPPname = pszHHPName;
    m_chmFilename = m_HPPname;
    m_chmFilename.replace_extension(".chm");
    m_cszRoot = pszHHPName;
    m_cszRoot.make_absolute();
    m_cszRoot.remove_filename();
    m_cszCWD.assignCwd();
}

// clang-format off
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
// clang-format on

// This function can be called recursively if the .HHP file has a #include directive to #include another .hhp file

void CParseHHP::ParseHhpFile(std::string_view filename)
{
    ttlib::viewfile file;
    if (!file.ReadFile(filename))
    {
        // REVIEW: [randalphwa - 11/29/2018] Need a way to add error msgs to CNinja
        //      ttCStr cszMsg;
        //      cszMsg.printf("Cannot read the file %s\n", pszHHP);
        //      AddError(cszMsg);
        return;
    }

    /*
        The following are possible sections. We only care about sections which will specify files that are to be
       compiled

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

    for (auto line: file)
    {
        line = ttlib::findnonspace(line);
        if (!line.length())
            continue;

        ttlib::cstr incName;

        if (ttlib::issameprefix(line, "#include", tt::CASE::either))
        {
            line = ttlib::stepover(line);
            if (line.empty())
                continue;

            if (line[0] == '\"')
            {
                incName.ExtractSubString(line);
            }
            else
            {
                incName = line;
                // Remove any comment
                incName.eraseFrom(';');
            }

            // A couple of sections sometimes include .h files -- but we only care about an #included .hhp file

            if (incName.hasExtension(".hhp"))
                ParseHhpFile(incName);
            continue;
        }

        if (line[0] == '[')  // sections are placed in brackets
        {
            m_section = SECTION_UNKNOWN;
            if (ttlib::issameprefix(line, "[ALIAS", tt::CASE::either))
                m_section = SECTION_ALIAS;
            else if (ttlib::issameprefix(line, "[FILE", tt::CASE::either))
                m_section = SECTION_FILES;
            else if (ttlib::issameprefix(line, "[CURRENT", tt::CASE::either))
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
                    if (ttlib::issameprefix(line, aOptions[pos]))
                        break;
                }
                if (aOptions[pos])
                {
                    auto posFile = line.find('=');
                    if (posFile != tt::npos)
                    {
                        incName = ttlib::findnonspace(line.substr(posFile + 1));
                        if (!incName.empty())
                        {
                            // [KeyWorksRW - 11-29-2018] I don't believe the HH compiler from MS supports
                            // comments after filenames, but we can't be certain other compilers and/or
                            // authoring systems don't use them -- so remove any comment just to be sure.

                            incName.eraseFrom(';');
                            AddDependency(filename, incName);
                        }
                    }
                }
                else if (ttlib::issameprefix(line, "Compiled file"))
                {
                    auto posFile = line.find('=');
                    if (posFile != tt::npos)
                    {
                        m_chmFilename = ttlib::findnonspace(line.substr(posFile + 1));
                        m_chmFilename.eraseFrom(';');
                    }
                }
            }
            break;

            case SECTION_ALIAS:
            {
                auto posFile = line.find('=');
                if (posFile != tt::npos)
                {
                    incName = ttlib::findnonspace(line.substr(posFile + 1));
                    if (!incName.empty())
                    {
                        incName.eraseFrom(';');
                        AddDependency(filename, incName);
                    }
                }
            }
            break;

            case SECTION_TEXT_POPUPS:
            {
                incName = line;
                if (!incName.empty())
                {
                    incName.eraseFrom(';');
                    AddDependency(pszHHP, incName.c_str());
                }
            }
            break;

            case SECTION_FILES:
            {
                // [KeyWorksRW - 11-29-2018] I don't think HHC actually allows comments in the [FILES] section,
                // though it should. I'm supporting comments here just to be sure we can support other file
                // formats like YAML which do allow for comments

                incName = line;
                if (!incName.empty())
                {
                    incName.eraseFrom(';');
                    AddDependency(pszHHP, incName.c_str());
                }
            }
            break;
        }
    }
}

void CParseHHP::AddDependency(std::string_view HHPfilename, std::string_view filename)
{
    /*
       The problem we need to solve with this function is that filename locations in the .hhp file will be
       relative to the .hhp file -- which in turn can be in a different location. So for example, you might
       have the following:

            ../Help/foo.hhp

        In foo.hhp, you might have:

            [FILES]
            html/bar.html

        We need to create a dependency to ../Help/html/bar.html
    */

    ttlib::cstr cszRelative;
    ttlib::cstr cszHHP;

    if (!ttlib::issameas(HHPfilename, filename))
    {
        // If we're in a nested .HHP file, then we need to first get the location of the nested .HHP, use that
        // to get the location of the pszFile;


        ttConvertToRelative(m_cszRoot, pszHHP, cszHHP);
        cszHHP.FullPathName();  // now that we have the location relative to our original .hhp file, convert it
                                // to a full path
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
        char* pszFilePortion = ttFindFilePortion(cszRelative);
        ptrdiff_t cFilePortion = (pszFilePortion - cszRelative.GetPtr());
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

    // TODO: [KeyWorksRW - 11-29-2018]  If this is an HTML file, then we need to parse it to find additional
    // dependencies

    m_lstDependencies += (const char*) cszRelative;
}

const char* txtHelpNinja = "bld/ChmHelp.ninja";

bool CNinja::CreateHelpFile()
{
    if (ttIsEmpty(GetHHPName()))
        return false;

    if (!ttlib::dirExists("bld"))
    {
        if (!fs::create_directory("bld"))
        {
            AddError("Unable to create the build directory -- so no place to put the .ninja files!\n");
            return false;
        }
    }

    CParseHHP chhp(GetHHPName());
    chhp.ParseHhpFile();  // this will figure out all of the dependencies
    m_chmFilename = chhp.m_chmFilename;

    ttlib::textfile file;

    file.GetTempLine().Format(
        "# WARNING: THIS FILE IS AUTO-GENERATED by %s. CHANGES YOU MAKE WILL BE LOST IF IT IS AUTO-GENERATED "
        "AGAIN.",
        txtVersion);
    file.WriteTempLine();
    file.addblankline();
    file.push_back("ninja_required_version = 1.8");
    file.GetTempLine().Format("builddir = %s", txtDefBuildDir);
    file.WriteTempLine();

    file.push_back("rule compile");
    file.push_back("  command = hhc.exe $in ");
    file.push_back("  description = compiling $out");
    file.addblankline();

    if (chhp.m_lstDependencies.GetCount() > 0)
    {
        file.GetTempLine().Format("build %s : compile %s | $", m_chmFilename.c_str(), GetHHPName());
        file.WriteTempLine();
        for (size_t pos = 0; chhp.m_lstDependencies.GetCount() - 1; ++pos)
        {
            file.GetTempLine().Format("  %s $", chhp.m_lstDependencies[pos]);
            file.WriteTempLine();
        }
        // last one doesn't have trailing '$' character
        file.GetTempLine() = "  ";
        file.WriteTempLine(chhp.m_lstDependencies[chhp.m_lstDependencies.GetCount() - 1]);
    }
    else
    {
        file.GetTempLine().Format("build %s : compile %s\n", m_chmFilename.c_str(), GetHHPName());
        file.WriteTempLine();
    }

    // We don't want to touch the build script if it hasn't changed, since that would change the timestamp
    // causing git and other tools that check timestamp to think something is different.

    if (m_dryrun.IsEnabled())
    {
        ttlib::viewfile fileOrg;
        if (fileOrg.ReadFile(txtHelpNinja))
        {
            m_dryrun.NewFile(txtHelpNinja);
            m_dryrun.DisplayFileDiff(fileOrg, file);
            return false;  // we didn't actually write anything
        }
    }

    if (!file.WriteFile(txtHelpNinja))
    {
        std::string msg = _tt("Cannot write to ");
        msg += txtHelpNinja;
        AddError(msg);
        return false;
    }

    return true;
}
