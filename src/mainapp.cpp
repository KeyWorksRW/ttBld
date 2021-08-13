/////////////////////////////////////////////////////////////////////////////
// Purpose:   entry point, global strings, library pragmas
// Author:    Ralph Walden
// Copyright: Copyright (c) 2002-2021 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#ifdef _MSC_VER
    #define wxMSVC_VERSION_ABI_COMPAT
    #include <msvc/wx/setup.h>  // This will add #pragmas for the wxWidgets libraries

    #if defined(_WIN32)

        #pragma comment(lib, "kernel32.lib")
        #pragma comment(lib, "user32.lib")
        #pragma comment(lib, "gdi32.lib")
        #pragma comment(lib, "comctl32.lib")
        #pragma comment(lib, "comdlg32.lib")
        #pragma comment(lib, "shell32.lib")

        #pragma comment(lib, "rpcrt4.lib")
        #pragma comment(lib, "advapi32.lib")

        #if wxUSE_URL_NATIVE
            #pragma comment(lib, "wininet.lib")
        #endif
    #endif
#endif

#include <vector>

#include <wx/cshelp.h>  // Context-sensitive help support classes
#include <wx/image.h>   // wxImage class
#include <wx/log.h>     // Assorted wxLogXXX functions, and wxLog (sink for logs)

#include "mainapp.h"  // CMainApp -- Main application class

#include <cstdlib>
#include <direct.h>  // Functions for directory handling and creation
#include <iostream>

#include "ttconsole.h"  // ttConsoleColor
#include "ttcwd.h"      // cwd -- Class for storing and optionally restoring the current directory
#include "ttparser.h"   // cmd -- Command line parser

#include "convert.h"    // CConvert
#include "funcs.h"      // List of function declarations
#include "ninja.h"      // CNinja
#include "stackwalk.h"  // Walk the stack filtering out anything unrelated to current app
#include "uifuncs.h"    // Miscellaneous functions for displaying UI
#include "writevcx.h"   // CVcxWrite -- Create a Visual Studio project file

#include "ui/optionsdlg.h"  // OptionsDlg -- Dialog for setting all .srcfile options

#if defined(TESTING)
    #include "dlgvscode.h"  // CDlgVsCode -- IDDLG_VSCODE dialog handler
#endif

void AddFiles(const std::vector<ttlib::cstr>& lstFiles);
int ConvertImageToHeader(std::vector<ttlib::cstr>& files);
int MakeHgz(std::vector<ttlib::cstr>& files);

enum UPDATE_TYPE
{
    UPDATE_NORMAL,

    UPDATE_MSVCD,
    UPDATE_MSVC32D,
    UPDATE_CLANG_CLD,
    UPDATE_CLANG_CL32D,
    UPDATE_CLANGD,
    UPDATE_CLANG32D,
    UPDATE_GCCD,
    UPDATE_GCC32D,

    UPDATE_MSVC,
    UPDATE_MSVC32,
    UPDATE_CLANG_CL,
    UPDATE_CLANG_CL32,
    UPDATE_CLANG,
    UPDATE_CLANG32,
    UPDATE_GCC,
    UPDATE_GCC32,

};

void MakeFileCaller(UPDATE_TYPE upType, const char* pszRootDir);

wxIMPLEMENT_APP_CONSOLE(CMainApp);

bool CMainApp::OnInit()
{
    SetAppDisplayName(txtAppname);
    SetVendorName("KeyWorks");
    SetVendorDisplayName("KeyWorks Software");

#if defined(wxUSE_ON_FATAL_EXCEPTION)
    ::wxHandleFatalExceptions(true);
#endif

    // If we're just providing text-popups for help, then this is all we need.
    wxHelpProvider::Set(new wxSimpleHelpProvider);

    return true;
}

int CMainApp::OnRun()
{
    // wxWidgets can return argv as char** but it does so using ToAscii(), which in some cases will trash a UTF16
    // filename that Windows passed to us. Casting to <wchar_t**> on Windows results in correctly converting UTF16
    // filenames to UTF8.

#if defined(_WIN32)
    ttlib::cmd cmd(argc, static_cast<wchar_t**>(argv));
#else
    ttlib::cmd cmd(argc, argv);
#endif  // _WIN32

    cmd.addHelpOption("h|help", "display this help message");

    cmd.addOption("codecmd", "create code32.cmd and code64.cmd batch files used to run VS Code on Windows");

    cmd.addOption("dir", "(directory) -- uses specified directory to create .ninja files (default is bld/)",
                  ttlib::cmd::needsarg);

    cmd.addOption("options", "displays a dialog allowing you to change options in .srcfiles.yaml");
    cmd.addOption("force", "create .ninja file(s) even if nothing has changed");
    cmd.addOption("makefile", "creates a makefile that doesn't require ttBld.exe");

    cmd.addOption("vscode", "creates or updates .vscode/*.json files used to build and debug a project using VS Code");
    cmd.addOption("vcxproj", "creates or updates Visual Studio project file (.vcxproj)");
    cmd.addOption("vs", "adds or updates .vs/*.json files used by Visual Studio");

    // The following options are all hidden -- they will not be displayed in the -help command list

    cmd.addHiddenOption("add");

    cmd.addHiddenOption("msvcenv64", ttlib::cmd::needsarg);
    cmd.addHiddenOption("msvcenv32", ttlib::cmd::needsarg);
    cmd.addHiddenOption("dryrun");

    cmd.addHiddenOption("umsvc");
    cmd.addHiddenOption("umsvc_x86");
    cmd.addHiddenOption("uclang");
    cmd.addHiddenOption("uclang_x86");
    cmd.addHiddenOption("umsvcD");
    cmd.addHiddenOption("umsvc_x86D");
    cmd.addHiddenOption("uclangD");
    cmd.addHiddenOption("uclang_x86D");

    cmd.addHiddenOption("hgz");  // -hgz dst src (converts src into gzip, saves as char array header file)

    cmd.addHiddenOption("xpm");  // -xpm src dst
    cmd.addHiddenOption("png");  // -png src dst

#if defined(TESTING) && !defined(NDEBUG)
    cmd.addHiddenOption("tvdlg", ttlib::cmd::needsarg);
#endif

    cmd.parse();

    if (cmd.isHelpRequested())
    {
        std::cout << txtVersion << '\n' << txtCopyRight << "\n\n";
        std::cout << "ttBld.exe [options] [project file]" << '\n';

        for (auto& iter: cmd.getUsage())
        {
            std::cout << iter << '\n';
        }
        return 0;
    }

    else if (cmd.isOption("hgz"))
    {
        return MakeHgz(cmd.getExtras());
    }
    else if (cmd.isOption("png"))
    {
        return ConvertImageToHeader(cmd.getExtras());
    }
    else if (cmd.isOption("xpm"))
    {
        auto files = cmd.getExtras();
        if (files.size() < 2)
        {
            std::cerr << "both src and dest files must be specified" << '\n';
            return 1;
        }

        // Add all image handlers so that any type of image that wxWidgets supports can be converted.
        wxInitAllImageHandlers();

        wxImage image;
        if (!image.LoadFile(files[0].wx_str()))
        {
            std::cerr << "Cannot load the image file " << files[0].wx_str() << '\n';
            return 1;
        }

        if (image.HasAlpha())
            image.ConvertAlphaToMask(wxIMAGE_ALPHA_THRESHOLD);

        if (!image.SaveFile(files[1].wx_str(), wxBITMAP_TYPE_XPM))
        {
            std::cerr << "Cannot save the XPM file " << files[1].wx_str() << '\n';
            return 1;
        }

        return 0;
    }

    UPDATE_TYPE upType = UPDATE_NORMAL;

    // this will only be initialized if the user specifies a -dir option
    ttlib::cstr RootDir;

    // This will be initialized by the user if they specifies a name.yaml file on the command line. Otherwise, a search is
    // made to find the most likely .srcfiles.yaml (or it's platform-variations) to use and this will be initialized to point
    // to it.
    ttlib::cstr projectFile;

// Change 0 to 1 to confirm that our locating functions are actually working as expected
#if 0 && !defined(NDEBUG) && defined(_WIN32)
    {
        ttlib::cstr str;
        if (FindCurMsvcPath(str))
            std::cout << str;
        else
            std::cout << "Cannot locate MSVC path";

        if (FindVsCode(str))
            std::cout << str;
        else
            std::cout << "Cannot locate VSCode.exe";

        if (FindFileEnv("PATH", "mingw32-make.exe", str))
            std::cout << str;
        else
            std::cout << "Cannot locate mingw32-make.exe";
    }
#endif

    if (cmd.isOption("codecmd"))
    {
        CreateCodeCmd("code32.cmd");
        CreateCodeCmd("code64.cmd");
        return 1;
    }

    if (cmd.isOption("msvcenv64"))
    {
        CreateMSVCEnvCmd(cmd.getOption("msvcenv64").value_or("msvcenv64.cmd").c_str(), true);
        return 1;
    }

    if (cmd.isOption("msvcenv32"))
    {
        CreateMSVCEnvCmd(cmd.getOption("msvcenv32").value_or("msvcenv32.cmd").c_str(), false);
        return 1;
    }

    if (cmd.isOption("dir"))
    {
        RootDir.assign(cmd.getOption("dir").value_or("bld"));
    }

    // Allow the user to specify a path to the project file to use.
    if (cmd.getExtras().size())
    {
        projectFile = cmd.getExtras().at(0);
    }

#if defined(TESTING) && !defined(NDEBUG)
    if (cmd.isOption("msvcenv32"))
    {
        CDlgVsCode dlg;
        if (dlg.DoModal(NULL) != IDOK)
            return 1;

        fs::remove(".vscode/launch.json");
        fs::remove(".vscode/tasks.json");

        // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
        auto results = CreateVsCodeProject(projectFile);
        for (auto& iter: results)
            std::cout << iter << '\n';
    }
#endif

    // The following commands are called from a makefile to update one .ninja script and immediately exit

    if (cmd.isOption("umsvc"))
        upType = UPDATE_MSVC;
    else if (cmd.isOption("umsvc_x86"))
        upType = UPDATE_MSVC32;
    else if (cmd.isOption("uclang"))
        upType = UPDATE_CLANG_CL;
    else if (cmd.isOption("uclang_x86"))
        upType = UPDATE_CLANG_CL32;
    else if (cmd.isOption("umsvcD"))
        upType = UPDATE_MSVCD;
    else if (cmd.isOption("umumsvc_x86Dsvc"))
        upType = UPDATE_MSVC32D;
    else if (cmd.isOption("uclangD"))
        upType = UPDATE_CLANG_CLD;
    else if (cmd.isOption("uclang_x86D"))
        upType = UPDATE_CLANG_CL32D;

    // If we are being called from a makefile then this is the only option we will now process. Note that we ignore
    // -dryrun even though we are generating a .ninja script. We do process the -dir option.

    if (upType != UPDATE_NORMAL)
    {
        MakeFileCaller(upType, RootDir.c_str());
        return 0;
    }

    // If a project file gets created, the options and vscode have already been set, and this flag will have been set to
    // true.
    bool projectCreated = false;

    // Unless the user specified this on the command line, locate a version, or create one if none can be found.
    if (projectFile.empty())
    {
        projectFile.assign(locateProjectFile(RootDir));
        if (projectFile.empty())
        {
            if (!MakeNewProject(projectFile))
            {
                ttlib::concolor clr(ttlib::concolor::LIGHTRED);
                std::cout << _tt(strIdMissingSrcfiles) << '\n';
                return 1;
            }
            projectCreated = true;
        }
    }

#if 0
    // BUGBUG: [KeyWorks - 05-08-2020] Can't add a filename without also specifying the project file to add it to (since
    // the user might have specified the one they want)
    if (cmd.isOption("add"))
    {
        AddFiles(cmd.getExtras());
    }
#endif

    if (!projectCreated && cmd.isOption("options"))
    {
        OptionsDlg dlg(projectFile);
        if (dlg.ShowModal() != wxID_OK)
            return 1;
        dlg.SaveChanges();
    }

    if (!projectCreated && cmd.isOption("vscode"))
    {
        if (cmd.isOption("force"))
            std::filesystem::remove(".vscode/c_cpp_properties.json");
        // Create .vscode/ and any of the three .json files that are missing, and update c_cpp_properties.json
        auto results = CreateVsCodeProject(projectFile);
        for (auto& iter: results)
            std::cout << iter << '\n';
    }

    if (cmd.isOption("vcxproj"))
    {
        CVcxWrite vcx(projectFile);
        return (vcx.CreateBuildFile() ? 0 : 1);
    }
    else if (cmd.isOption("vs"))
    {
        // Create .vs/ and any of the three .json files that are missing, and update c_cpp_properties.json
        std::vector<std::string> results;
        CreateVsJson(projectFile.c_str(), results);
        for (auto msg: results)
            std::cout << msg << '\n';
    }

    // It's possible that the user specified a different location and even a different name for the project file to use.
    // If the project file is in a different directory, then we change to that directory now before continuing. The
    // destructor will restore the original directory.

    ttlib::cwd cwd(true);
    {
        ttlib::cstr path(projectFile);
        path.remove_filename();
        if (path.size())
        {
            ttlib::ChangeDir(path);
            // Now that we're in the same directory as the project file, remove the path portion and just use the filename.
            path = projectFile.filename();
            projectFile = path;
        }
    }

    CNinja cNinja(projectFile);
    if (!cNinja.IsValidVersion())
    {
        std::cerr << _tt(strIdOldVersion) << '\n';
        return 1;
    }

    if (cmd.isOption("makefile"))
    {
        cNinja.CreateMakeFile(CNinja::MAKE_TYPE::normal);
    }

    cNinja.CreateMakeFile(CNinja::MAKE_TYPE::autogen);

    if (cmd.isOption("force"))  // force write ignores any request for dryrun
        cNinja.ForceWrite();
    else if (cmd.isOption("dryrun"))
        cNinja.EnableDryRun();

    int countNinjas = 0;
#if defined(_WIN32)
    if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_MSVC))
        countNinjas++;
    if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_MSVC))
        countNinjas++;
    if (cNinja.hasOptValue(OPT::TARGET_DIR32))
    {
        if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG32, CNinja::CMPLR_MSVC))
            countNinjas++;
        if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE32, CNinja::CMPLR_MSVC))
            countNinjas++;
    }
#endif
    if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_CLANG))
        countNinjas++;
    if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_CLANG))
        countNinjas++;
    if (cNinja.hasOptValue(OPT::TARGET_DIR32))
    {
        if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG32, CNinja::CMPLR_CLANG))
            countNinjas++;
        if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE32, CNinja::CMPLR_CLANG))
            countNinjas++;
    }

    // Display any errors that occurred during processing

    if (cNinja.getErrorMsgs().size())
    {
        ttlib::concolor clr(ttlib::concolor::LIGHTRED);
        for (auto iter: cNinja.getErrorMsgs())
        {
            std::cout << iter << '\n';
#if defined(_DEBUG)
            wxLogDebug(iter.to_utf16().c_str());
#endif  // _DEBUG
        }
        std::cout << "\n\n";
    }

    if (countNinjas > 0)
    {
        std::cout << _tt(strIdCreated) << countNinjas << " .ninja" << _tt(strIdFiles) << '\n';
#if defined(_DEBUG)
        ttlib::cstr msg(_ttc(strIdCreated) << countNinjas << " .ninja" << _tt(strIdFiles) << '\n');
        wxLogDebug(msg.to_utf16().c_str());
#endif  // _DEBUG
    }
    else
    {
        std::cout << _ttc(strIdAllNinjaCurrent) << '\n';
#if defined(_DEBUG)
        wxLogDebug(_ttc(strIdAllNinjaCurrent).to_utf16().c_str());
        wxLogDebug(L"\n");
#endif  // _DEBUG
    }
    return 0;
}

void MakeFileCaller(UPDATE_TYPE upType, const char* pszRootDir)
{
    CNinja cNinja(pszRootDir);
    cNinja.ForceWrite();

    if (cNinja.IsValidVersion())
    {
        switch (upType)
        {
            case UPDATE_MSVC:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_MSVC))
                    std::cout << cNinja.GetScriptFile() << _tt(strIdUpdated) << '\n';
                break;

            case UPDATE_MSVC32:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE32, CNinja::CMPLR_MSVC))
                    std::cout << cNinja.GetScriptFile() << _tt(strIdUpdated) << '\n';
                break;

            case UPDATE_CLANG_CL:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE, CNinja::CMPLR_CLANG))
                    std::cout << cNinja.GetScriptFile() << _tt(strIdUpdated) << '\n';
                break;

            case UPDATE_CLANG_CL32:
                if (cNinja.CreateBuildFile(CNinja::GEN_RELEASE32, CNinja::CMPLR_CLANG))
                    std::cout << cNinja.GetScriptFile() << _tt(strIdUpdated) << '\n';
                break;

            case UPDATE_MSVCD:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_MSVC))
                    std::cout << cNinja.GetScriptFile() << _tt(strIdUpdated) << '\n';
                break;

            case UPDATE_MSVC32D:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG32, CNinja::CMPLR_MSVC))
                    std::cout << cNinja.GetScriptFile() << _tt(strIdUpdated) << '\n';
                break;

            case UPDATE_CLANG_CLD:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG, CNinja::CMPLR_CLANG))
                    std::cout << cNinja.GetScriptFile() << _tt(strIdUpdated) << '\n';
                break;

            case UPDATE_CLANG_CL32D:
                if (cNinja.CreateBuildFile(CNinja::GEN_DEBUG32, CNinja::CMPLR_CLANG))
                    std::cout << cNinja.GetScriptFile() << _tt(strIdUpdated) << '\n';
                break;

            default:
                break;
        }
    }

    else
    {
        ttlib::concolor clr(ttlib::concolor::LIGHTRED);
        std::cout << _tt(strIdOldVersion);
    }
}

int CMainApp::OnExit()
{
    delete wxHelpProvider::Set(NULL);

    return wxApp::OnExit();
}

// clang-format off
#if defined(NDEBUG)

void CMainApp::OnFatalException()
{
    // Let the user know something terrible happened.
    appMsgBox(_tt(strIdInternalError), txtVersion);
}

#else  // not defined(NDEBUG)

// In Debug builds, we create a filtered list and send it to the debugger, then execute a breakpoint to break into
// the debugger. This will only be useful if the app was already being run in the debugger.

#include "stackwalk.h"  // StackLogger -- Walk the stack filtering out anything unrelated to current app

void CMainApp::OnFatalException()
{
    StackLogger logger;
    logger.WalkFromException();
    for (auto& iter: logger.GetCalls())
    {
        // We're only interested in our own source code, so ignore the rest.
        if (!iter.contains(txtAppname))
            continue;

        wxLogDebug(iter.c_str());
    }

#if defined(_WIN32)

    // We now have the relevant call stack displayed in the debugger, so break into it.

    if (wxIsDebuggerRunning())  // On Windows, break into debugger if it's running.
        wxTrap();

    else
        wxFAIL_MSG(_tt(strIdInternalError));

 #else  // not defined(_WIN32)

    wxFAIL_MSG(_tt(strIdInternalError));

#endif  // defined(_WIN32)
}

#endif  // defined(NDEBUG)
// clang-format on
