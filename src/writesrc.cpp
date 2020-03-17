/////////////////////////////////////////////////////////////////////////////
// Name:      CWriteSrcFiles
// Purpose:   Writes a new or update srcfiles.yaml file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   MIT License (see %lic_name%)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "writesrc.h"  // CWriteSrcFiles

CWriteSrcFiles::CWriteSrcFiles() {}

bld::RESULT CWriteSrcFiles::UpdateOptions(std::string_view filename)
{
    if (!filename.empty())
        m_outFilename = filename;
    return bld::failure;
}

bld::RESULT CWriteSrcFiles::WriteNew(std::string_view filename, std::string_view comment)
{
    if (!filename.empty())
        m_outFilename = filename;
    if (!comment.empty())
        m_fileComment = comment;

    return bld::failure;
}
