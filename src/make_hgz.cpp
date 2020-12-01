/////////////////////////////////////////////////////////////////////////////
// Purpose:   Converts a file into a .gz and stores as char array header
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <fstream>
#include <iostream>

#include <wx/archive.h>   // Streams for archive formats
#include <wx/file.h>      // wxFile - encapsulates low-level "file descriptor"
#include <wx/mstream.h>   // Memory stream classes
#include <wx/stream.h>    // stream classes
#include <wx/wfstream.h>  // File stream classes

#include <ttcvector.h>   // Vector of ttlib::cstr strings
#include <tttextfile.h>  // textfile -- Classes for reading and writing line-oriented files

static bool CopyStreamData(wxInputStream* inputStream, wxOutputStream* outputStream, size_t size);

int MakeHgz(ttlib::cstrVector& files)
{
    if (files.size() < 2)
    {
        std::cerr << "both src and dest files must be specified" << '\n';
        return 1;
    }

    auto filterClassFactory = wxFilterClassFactory::Find(".gz", wxSTREAM_FILEEXT);
    if (!filterClassFactory)
    {
        std::cerr << "internal error -- .gz support not enabled" << '\n';
        return 1;
    }

    wxFileInputStream inputFileStream(files[0]);
    auto len = inputFileStream.GetLength();
    if (!inputFileStream.IsOk())
    {
        std::cerr << _tt(strIdCantOpen) << files[0] << '\n';
        return 1;
    }

    wxMemoryOutputStream stream_out;

    wxScopedPtr<wxFilterOutputStream> filterOutputStream(filterClassFactory->NewStream(stream_out));
    if (!CopyStreamData(&inputFileStream, filterOutputStream.get(), inputFileStream.GetLength()))
    {
        std::cerr << _tt(strIdInternalError) << '\n';
        return 1;
    }
    filterOutputStream->Close();

    auto strm_buffer = stream_out.GetOutputStreamBuffer();
    strm_buffer->Seek(0, wxFromStart);

    ttlib::textfile file;
    {
        file.addEmptyLine() << "// .gz version of " << files[0].filename();
        file.addEmptyLine();
        file.addEmptyLine() << "#pragma once";
        file.addEmptyLine();

        ttlib::cstr str_name = files[0].filename();
        ttlib::cstr ext = str_name.extension();
        str_name.remove_extension();
        if (ext.size())
        {
            // replace the leading '.' with a '_' so that it looks like a more normal string
            ext[0] = '_';
            str_name << ext;
        }
        str_name << "_gz";

        file.addEmptyLine() << "static const unsigned char " << str_name << '[' << strm_buffer->GetBufferSize() << "] = {";
    }

    auto buf = static_cast<unsigned char*>(strm_buffer->GetBufferStart());

    size_t pos = 0;
    auto buf_size = strm_buffer->GetBufferSize();

    while (pos < buf_size)
    {
        {
            auto& line = file.addEmptyLine();
            for (; pos < buf_size && line.size() < 116; ++pos)
            {
                line << static_cast<int>(buf[pos]) << ',';
            }
        }
    }

    if (file[file.size() - 1].back() == ',')
        file[file.size() - 1].pop_back();

    file.addEmptyLine() << "};";

    if (!file.WriteFile(files[1]))
    {
        std::cerr << _tt(strIdCantWrite) << files[1];
        return 1;
    }

    return 0;
}

static bool CopyStreamData(wxInputStream* inputStream, wxOutputStream* outputStream, size_t size)
{
    size_t buf_size;
    if (size == tt::npos || size > (64 * 1024))
        buf_size = (64 * 1024);
    else
        buf_size = static_cast<size_t>(size);

    auto read_buf = std::make_unique<unsigned char[]>(buf_size);
    auto read_size = buf_size;

    size_t copied_data = 0;
    for (;;)
    {
        if (size != tt::npos && copied_data + read_size > size)
            read_size = size - copied_data;
        inputStream->Read(read_buf.get(), read_size);

        auto actually_read = inputStream->LastRead();
        outputStream->Write(read_buf.get(), actually_read);
        if (outputStream->LastWrite() != actually_read)
        {
            return false;
        }

        if (size == tt::npos)
        {
            if (inputStream->Eof())
                break;
        }
        else
        {
            copied_data += actually_read;
            if (copied_data >= size)
                break;
        }
    }

    return true;
}
