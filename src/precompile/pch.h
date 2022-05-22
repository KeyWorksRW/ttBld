// This header file is used to create a pre-compiled header for use in the entire project

#pragma once

#if defined(_WIN32)

// Reduce the number of Windows files that get read

    #define NOATOM
    #define NOCOMM
    #define NODRIVERS
    #define NOEXTDEVMODEPROPSHEET
    #define NOIME
    #define NOKANJI
    #define NOLOGERROR
    #define NOMCX
    #define NOPROFILER
    #define NOSCALABLEFONT
    #define NOSERVICE
    #define NOSOUND
    #define NOWINDOWSX
    #define NOENHMETAFILE

    #define OEMRESOURCE
    #define _CRT_SECURE_NO_WARNINGS

    #define STRICT

    #define WINVER       0x0601  // Windows 7
    #define _WIN32_WINNT 0x0600

#endif  // _WIN32

#define wxUSE_UNICODE     1
#define wxUSE_NO_MANIFEST 1  // This is required for compiling using CLANG 8 and earlier

#ifdef _MSC_VER
    // REVIEW: [KeyWorks - 09-10-2021] VS 11.2 with latest windows platform sdk is generating tons of these warnings
    #pragma warning(disable : 4251)  // needs to have dll-interface
#endif

#ifdef _MSC_VER
    #pragma warning(push)
#endif

#include <wx/defs.h>

#if defined(__WINDOWS__)
    #include <wx/msw/wrapcctl.h>

    #if wxUSE_COMMON_DIALOGS
        #include <commdlg.h>
    #endif
#endif

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

// Ensure that _DEBUG is defined in non-release builds
#if !defined(NDEBUG) && !defined(_DEBUG)
    #define _DEBUG
#endif

#include <stdexcept>
#include <string>
#include <string_view>

#include "ttlibspace.h"  // Master header file for ttLib

// Currently, the order of the ttLib includes is critical, so use blank lines to keep clang-format from reordering them.

#include "ttcview.h"  // ttlib::cview -- std::string_view functionality on a zero-terminated char string.
#include "ttsview.h"  // ttlib::sview -- std::string_view with additional methods

#include "ttcstr.h"      // ttlib::cstr -- std::string with additional functions

// Version is also set in writesrc.h and ttBld.rc -- if changed, change in all three locations

inline constexpr const auto txtVersion = "ttBld 1.8.1";
inline constexpr const auto txtCopyRight = "Copyright (c) 2002-2021 KeyWorks Software";
inline constexpr const auto txtAppname = "ttBld";

// Use THROW to throw this exception class
class CExcept
{
public:
    CExcept(const std::string& what) : m_what(what) {}

    const ttlib::cstr& what() const { return m_what; }

private:
    ttlib::cstr m_what;
};

/////////////////////// macros ////////////////////////////////////////

// There are some parts of the code that have different code paths depending on whether _WIN32 is defined or not.
// In order to test the non-Windows code paths, you can uncomment the following line. Note that this only affects
// wxTest code
// -- ttLib will still use _WIN32 code.

// #define NONWIN_TEST

#if defined(_WIN32) && !defined(NONWIN_TEST)
    // Use this macro to comment out parameters that are not used when building for Windows
    #define NONWIN_PARAM(param) /* param */
#else
    // Use this macro to comment out parameters that are not used when building for Windows
    #define NONWIN_PARAM(param) param
#endif

#if defined(NDEBUG)
    #define ASSERT(cond)
    #define ASSERT_MSG(cond, msg)
    #define FAIL_MSG(msg)
    #define THROW(msg) throw CExcept(msg)
#else  // not defined(NDEBUG)
    #if defined(_WIN32) && !defined(NONWIN_TEST)

bool ttAssertionMsg(const char* filename, const char* function, int line, const char* cond, const std::string& msg);

        #define ASSERT(cond)                                                            \
            {                                                                           \
                if (!(cond) && ttAssertionMsg(__FILE__, __func__, __LINE__, #cond, "")) \
                {                                                                       \
                    wxTrap();                                                           \
                }                                                                       \
            }

        #define ASSERT_MSG(cond, msg)                                                  \
            if (!(cond) && ttAssertionMsg(__FILE__, __func__, __LINE__, #cond, (msg))) \
            {                                                                          \
                wxTrap();                                                              \
            }

        #define FAIL_MSG(msg)                                               \
            if (ttAssertionMsg(__FILE__, __func__, __LINE__, nullptr, msg)) \
            {                                                               \
                wxTrap();                                                   \
            }

        // In _DEBUG builds this will display an assertion dialog first then it will throw
        // an excpetion. In Release builds, only the exception is thrown.
        #define THROW(msg)                                                      \
            {                                                                   \
                if (ttAssertionMsg(__FILE__, __func__, __LINE__, nullptr, msg)) \
                {                                                               \
                    wxTrap();                                                   \
                }                                                               \
                throw CExcept(msg);                                             \
            }

    #else  // not defined(_WIN32)
        #define ASSERT(cond)          wxASSERT(cond)
        #define ASSERT_MSG(cond, msg) wxASSERT_MSG(cond, msg)
        #define FAIL_MSG(msg)         wxFAIL_MSG(msg)

        // In _DEBUG builds this will display an assertion dialog first then it will throw
        // an excpetion. In Release builds, only the exception is thrown.
        #define THROW(msg)         \
            {                      \
                wxFAIL_MSG(msg);   \
                throw CExcept(msg) \
            }
    #endif  // _WIN32
#endif      // defined(NDEBUG)
