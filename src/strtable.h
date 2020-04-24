// Purpose: String resource IDs

// If you use the _tt() macro (see ttTR.h in ttLib) then it will load a string resource when compiled for Windows. If compiled with
// wxWidgets on a non-Windows platform, it will load the translated string produced by the xgettext system that wxWidgets supports.

// Note: Windows saves strings in consecutive blocks of 16, so it is useful to keep id numbers consecutive.

#ifdef _WIN32
// #if 0  // comment out above and uncomment this to test non-Windows builds on a Windows platform

    #define IDS_CANNOT_OPEN_VIEW    1024  // "Cannot open %v"

#else

constexpr auto IDS_CANNOT_OPEN_VIEW = "Cannot open %v";

#endif
