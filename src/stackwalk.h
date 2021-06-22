/////////////////////////////////////////////////////////////////////////////
// Purpose:   Walk the stack filtering out anything unrelated to current app
// Author:    Ralph Walden
// Copyright: Copyright (c) 2020 KeyWorks Software (Ralph Walden)
// License:   Apache License -- see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/stackwalk.h>

#if defined(_DEBUG) && defined(wxUSE_ON_FATAL_EXCEPTION) && defined(wxUSE_STACKWALKER)

class StackLogger : public wxStackWalker
{
public:
    std::vector<ttlib::cstr>& GetCalls() { return m_calls; }

protected:
    void OnStackFrame(const wxStackFrame& frame) override
    {
        if (frame.HasSourceLocation())
        {
            ttlib::cstr source;
            source << frame.GetFileName().wx_str() << ':' << static_cast<int>(frame.GetLine());

            wxString params;
            if (auto paramCount = frame.GetParamCount(); paramCount > 0)
            {
                params << "(";

                for (size_t i = 0; i < paramCount; ++i)
                {
                    wxString type, name, value;
                    if (frame.GetParam(i, &type, &name, &value))
                    {
                        params << type << " " << name << " = " << value << ", ";
                    }
                }

                params << ")";
            }

            if (params.size() > 100)
                params = "(...)";

            m_calls.emplace_back() << static_cast<int>(frame.GetLevel()) << ' ' << frame.GetName().wx_str()
                                   << params.wx_str() << ' ' << source;
        }
        else
        {
            m_calls.emplace_back() << static_cast<int>(frame.GetLevel()) << ' ' << frame.GetName().wx_str();
        }
    }

    std::vector<ttlib::cstr> m_calls;
};

#endif  // defined(_DEBUG) && defined(wxUSE_ON_FATAL_EXCEPTION) && defined(wxUSE_STACKWALKER)
