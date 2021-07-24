/////////////////////////////////////////////////////////////////////////////
// Purpose:   Create a Visual Studio project file
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ninja.h"  // CNinja

namespace pugi
{
    class xml_node;
}

class CVcxWrite : public CNinja
{
public:
    CVcxWrite(std::string_view NinjaDir = ttlib::emptystring) : CNinja(NinjaDir) {}

    bool CreateBuildFile();

protected:
    void AddOutDirs(pugi::xml_node parent, GEN_TYPE gentype);
    void AddConfigAppType(pugi::xml_node parent, GEN_TYPE gentype);
    void AddConfiguration(pugi::xml_node parent, GEN_TYPE gentype);
    bool CVcxWrite::CreateFilterFile(ttlib::cstr vc_project_file);

private:
    pugi::xml_node* m_Project;
};
