/////////////////////////////////////////////////////////////////////////////
// Purpose:   Convert image into png header
// Author:    Ralph Walden
// Copyright: Copyright (c) 2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <iostream>

#include <wx/image.h>     // wxImage class
#include <wx/mstream.h>   // Memory stream classes
#include <wx/wfstream.h>  // File stream classes

#include "ttstr.h"       // ttString -- wxString with additional methods similar to ttlib::cstr
#include "tttextfile.h"  // textfile -- Classes for reading and writing line-oriented files

// clang-format off
static constexpr const char* lst_no_png_conversion[] = {

    "_ani",
    "_cur",
    "_gif",
    "_ico",
    "_jpeg",

};
// clang-format on

bool isConvertibleMime(const ttString& suffix)
{
    for (auto& iter: lst_no_png_conversion)
    {
        if (suffix.is_sameas(iter))
            return false;
    }
    return true;
}

int ConvertImageToHeader(std::vector<ttlib::cstr>& files)
{
    if (files.size() < 2)
    {
        std::cerr << "both src and dest files must be specified" << '\n';
        return 1;
    }

    // Add all image handlers so that the EmbedImage class can be used to convert any type of image that wxWidgets
    // supports.
    wxInitAllImageHandlers();

    wxImage image;
    bool isImageLoaded { false };
    // We need to know what the original file type is because if we convert it to a header, then XPM and BMP files will
    // be converted to PNG before saving.

    ttString mime_type;
    wxFFileInputStream stream(files[0].wx_str());
    if (stream.IsOk())
    {
        wxBufferedInputStream bstream(stream);

        wxImageHandler* handler;
        auto& list = wxImage::GetHandlers();
        for (auto node = list.GetFirst(); node; node = node->GetNext())
        {
            handler = (wxImageHandler*) node->GetData();
            if (handler->CanRead(stream))
            {
                mime_type = handler->GetMimeType();

                if (handler->LoadFile(&image, stream))
                {
                    isImageLoaded = true;
                    break;
                }
                else
                {
                    std::cerr << "Unable to read " << files[0].c_str();

                    return 1;
                }
            }
        }
    }

    if (!isImageLoaded)
    {
        std::cerr << "Unrecognized image file format in " << files[0].c_str();
    }

    ttString suffix(mime_type);
    suffix.Replace("image/", "_");
    suffix.Replace("x-", "");  // if something like x-bmp, just use bmp

    wxMemoryOutputStream save_stream;

    if (isConvertibleMime(suffix))
    {
        // Maximize compression
        image.SetOption(wxIMAGE_OPTION_PNG_COMPRESSION_LEVEL, 9);
        image.SetOption(wxIMAGE_OPTION_PNG_COMPRESSION_MEM_LEVEL, 9);

        image.SaveFile(save_stream, wxBITMAP_TYPE_PNG);
    }
    else
    {
        image.SaveFile(save_stream, mime_type);
    }

    auto read_stream = save_stream.GetOutputStreamBuffer();

    ttlib::textfile file_out;

    ttlib::cstr string_name = files[1].filename();

    string_name.remove_extension();
    string_name.Replace(".", "_", true);

    // Using `inline constexpr` instead of `static` ensures the array won't be duplicated in the executable no matter how
    // many times it has been #included. However, it does require a c++17 or later compiler, so if we move this from
    // experimental to publicly documented, then we'll need an option for the compiler version that will be used and switch
    // to `static` if an older compiler will be used.

    file_out.addEmptyLine().Format("inline constexpr const unsigned char %s[%zu] = {", string_name.c_str(),
                               read_stream->GetBufferSize());

    read_stream->Seek(0, wxFromStart);

    auto buf = static_cast<unsigned char*>(read_stream->GetBufferStart());

    size_t pos = 0;
    auto buf_size = read_stream->GetBufferSize();

    while (pos < buf_size)
    {
        {
            auto& line = file_out.addEmptyLine();
            for (; pos < buf_size && line.size() < 116; ++pos)
            {
                line << static_cast<int>(buf[pos]) << ',';
            }
        }
    }

    if (file_out[file_out.size() - 1].back() == ',')
        file_out[file_out.size() - 1].pop_back();

    file_out.addEmptyLine() << "};";
    if (!file_out.WriteFile(files[1]))
    {
        std::cerr << "Unable to write converted image to " << files[1].c_str();
        return 1;
    }

    return 0;
}
