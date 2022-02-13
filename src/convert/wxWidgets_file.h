/////////////////////////////////////////////////////////////////////////////
// Purpose:   Convert wxWidgets build/file to CMake file list
// Author:    Ralph Walden
// Copyright: Copyright (c) 2022 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include <set>

#include "tttextfile.h"  // textfile -- Classes for reading and writing line-oriented files

class WidgetsFile
{
public:
    WidgetsFile() {};

    int Convert(std::vector<ttlib::cstr>& files);

private:
    ttlib::textfile m_widget_files;

    std::set<ttlib::cstr> m_unix_list;
    std::set<ttlib::cstr> m_osx_list;
    std::set<ttlib::cstr> m_msw_list;
    std::set<ttlib::cstr> m_common_list;
};
