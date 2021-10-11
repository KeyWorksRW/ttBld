/////////////////////////////////////////////////////////////////////////////
// Purpose:   Converts a file into a .gz and stores as char array header
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include <wx/archive.h>   // Streams for archive formats
#include <wx/file.h>      // wxFile - encapsulates low-level "file descriptor"
#include <wx/mstream.h>   // Memory stream classes
#include <wx/stream.h>    // stream classes
#include <wx/wfstream.h>  // File stream classes

#include "tttextfile.h"  // textfile -- Classes for reading and writing line-oriented files

#include "pugixml/pugixml.hpp"

static bool CopyStreamData(wxInputStream* inputStream, wxOutputStream* outputStream, size_t size);

int MakeHgz(std::vector<ttlib::cstr>& files)
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

    // XML files are special-cased. If the file can be parsed by pugixml, then only one source file is allowed and comments
    // and formatting are removed in the output file.

    if (files[1].has_extension(".xml"))
    {
        pugi::xml_document doc;
        auto result = doc.load_file(files[1].c_str(), pugi::parse_default | pugi::parse_trim_pcdata);

        if (result)
        {
            std::stringstream strm;
            doc.save(strm, "", pugi::format_raw);
            // TODO: [KeyWorks - 05-10-2021] C++20 adds a view() method so that we don't have to make a copy of the string
            auto string = strm.str();
            wxMemoryInputStream inputFileStream(string.c_str(), string.size());

            wxMemoryOutputStream stream_out;
            wxScopedPtr<wxFilterOutputStream> filterOutputStream(filterClassFactory->NewStream(stream_out));

            if (!CopyStreamData(&inputFileStream, filterOutputStream.get(), inputFileStream.GetLength()))
            {
                std::cerr << "An internal error has occurred: " << '\n';
                return 1;
            }
            filterOutputStream->Close();
            auto strm_buffer = stream_out.GetOutputStreamBuffer();
            strm_buffer->Seek(0, wxFromStart);

            ttlib::cstr str_name = files[1].filename();
            ttlib::cstr ext = str_name.extension();
            str_name.remove_extension();
            if (ext.size())
            {
                // replace the leading '.' with a '_' so that it looks like a normal string
                ext[0] = '_';
                str_name << ext;
            }
            str_name << "_gz";

            ttlib::textfile file;

            file.insertEmptyLine(0) << "// " << files[1].filename()
                                    << " -- comments and formatting removed, compressed with gizp";
            file.addEmptyLine();
            file.addEmptyLine() << "static const unsigned char " << str_name << '[' << strm_buffer->GetBufferSize()
                                << "] = {";

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

            if (!file.WriteFile(files[0]))
            {
                std::cerr << "Unable to create or write to " << files[0];
                return 1;
            }
            return 0;
        }
    }

    std::vector<ttlib::cstr> src_filenames;
    ttlib::textfile file;

    for (size_t file_pos = 1; file_pos < files.size(); ++file_pos)
    {
        wxMemoryOutputStream stream_out;
        wxScopedPtr<wxFilterOutputStream> filterOutputStream(filterClassFactory->NewStream(stream_out));

        wxFileInputStream inputFileStream(files[file_pos]);
        if (!inputFileStream.IsOk())
        {
            std::cerr << "Cannot open " << files[0] << '\n';
            return 1;
        }

        if (!CopyStreamData(&inputFileStream, filterOutputStream.get(), inputFileStream.GetLength()))
        {
            std::cerr << "An internal error has occurred: " << '\n';
            return 1;
        }

        src_filenames.emplace_back(files[file_pos]);
        filterOutputStream->Close();
        auto strm_buffer = stream_out.GetOutputStreamBuffer();
        strm_buffer->Seek(0, wxFromStart);

        ttlib::cstr str_name = files[file_pos].filename();
        ttlib::cstr ext = str_name.extension();
        str_name.remove_extension();
        if (ext.size())
        {
            // replace the leading '.' with a '_' so that it looks like a more normal string
            ext[0] = '_';
            str_name << ext;
        }
        str_name << "_gz";

        file.insertEmptyLine(0) << "// " << str_name << "[] == " << files[file_pos];

        file.addEmptyLine();
        file.addEmptyLine() << "static const unsigned char " << str_name << '[' << strm_buffer->GetBufferSize() << "] = {";

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
    }

    for (size_t pos_cmt = 0; pos_cmt < file.size(); ++pos_cmt)
    {
        if (!file[pos_cmt].is_sameprefix("//"))
        {
            file.insertEmptyLine(pos_cmt++);
            file.insertEmptyLine(pos_cmt++) << "#pragma once";
            break;
        }
    }

    if (!file.WriteFile(files[0]))
    {
        std::cerr << "Unable to create or write to " << files[0];
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
