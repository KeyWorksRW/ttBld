// This header file is used to create a pre-compiled header for use in the entire project

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

#include <wx/defs.h>

#if defined(__WINDOWS__)
    #include <wx/msw/wrapcctl.h>

    #if wxUSE_COMMON_DIALOGS
        #include <commdlg.h>
    #endif
#endif

// Ensure that _DEBUG is defined in non-release builds
#if !defined(NDEBUG) && !defined(_DEBUG)
    #define _DEBUG
#endif

#include <stdexcept>
#include <string>
#include <string_view>

#include <filesystem>
namespace fs = std::filesystem;

#include <ttlibspace.h>  // Master header file for ttLib

#if defined(_WIN32)
    #include <ttdebug.h>  // ttASSERT macros
#endif

#include <ttcstr.h>   // Classes for handling zero-terminated char strings.
#include <ttcview.h>  // cview -- string_view functionality on a zero-terminated char string.
#include <ttstrings.h>

#include "strings.h"

constexpr const auto txtVersion = "ttBld 1.7.1";
constexpr const auto txtCopyRight = "Copyright (c) 2002-2020 KeyWorks Software";
constexpr const auto txtAppname = "ttBld";

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
