/////////////////////////////////////////////////////////////////////////////
// Purpose:   Convert wxWidgets build/file to CMake file list
// Author:    Ralph Walden
// Copyright: Copyright (c) 2022 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include <iostream>

#include "wxWidgets_file.h"

#include "ttcview_wx.h"

// build/files doesn't include the C files, so we hard-code them here

inline constexpr const auto txt_wxCLib_sources = {

    "jpeg/jaricom.c",
    "jpeg/jcapimin.c",
    "jpeg/jcapistd.c",
    "jpeg/jcarith.c",
    "jpeg/jccoefct.c",
    "jpeg/jccolor.c",
    "jpeg/jcdctmgr.c",
    "jpeg/jchuff.c",
    "jpeg/jcinit.c",
    "jpeg/jcmainct.c",
    "jpeg/jcmarker.c",
    "jpeg/jcmaster.c",
    "jpeg/jcomapi.c",
    "jpeg/jcparam.c",
    "jpeg/jcprepct.c",
    "jpeg/jcsample.c",
    "jpeg/jctrans.c",
    "jpeg/jdapimin.c",
    "jpeg/jdapistd.c",
    "jpeg/jdarith.c",
    "jpeg/jdatadst.c",
    "jpeg/jdatasrc.c",
    "jpeg/jdcoefct.c",
    "jpeg/jdcolor.c",
    "jpeg/jddctmgr.c",
    "jpeg/jdhuff.c",
    "jpeg/jdinput.c",
    "jpeg/jdmainct.c",
    "jpeg/jdmarker.c",
    "jpeg/jdmaster.c",
    "jpeg/jdmerge.c",
    "jpeg/jdpostct.c",
    "jpeg/jdsample.c",
    "jpeg/jdtrans.c",
    "jpeg/jerror.c",
    "jpeg/jfdctflt.c",
    "jpeg/jfdctfst.c",
    "jpeg/jfdctint.c",
    "jpeg/jidctflt.c",
    "jpeg/jidctfst.c",
    "jpeg/jidctint.c",
    "jpeg/jmemmgr.c",
    "jpeg/jmemnobs.c",
    "jpeg/jquant1.c",
    "jpeg/jquant2.c",
    "jpeg/jutils.c",

    "tiff/libtiff/tif_aux.c",
    "tiff/libtiff/tif_close.c",
    "tiff/libtiff/tif_codec.c",
    "tiff/libtiff/tif_color.c",
    "tiff/libtiff/tif_compress.c",
    "tiff/libtiff/tif_dir.c",
    "tiff/libtiff/tif_dirinfo.c",
    "tiff/libtiff/tif_dirread.c",
    "tiff/libtiff/tif_dirwrite.c",
    "tiff/libtiff/tif_dumpmode.c",
    "tiff/libtiff/tif_error.c",
    "tiff/libtiff/tif_extension.c",
    "tiff/libtiff/tif_fax3.c",
    "tiff/libtiff/tif_fax3sm.c",
    "tiff/libtiff/tif_flush.c",
    "tiff/libtiff/tif_getimage.c",
    "tiff/libtiff/tif_jbig.c",
    "tiff/libtiff/tif_jpeg.c",
    "tiff/libtiff/tif_jpeg_12.c",
    "tiff/libtiff/tif_luv.c",
    "tiff/libtiff/tif_lzma.c",
    "tiff/libtiff/tif_lzw.c",
    "tiff/libtiff/tif_next.c",
    "tiff/libtiff/tif_ojpeg.c",
    "tiff/libtiff/tif_open.c",
    "tiff/libtiff/tif_packbits.c",
    "tiff/libtiff/tif_pixarlog.c",
    "tiff/libtiff/tif_predict.c",
    "tiff/libtiff/tif_print.c",
    "tiff/libtiff/tif_read.c",
    "tiff/libtiff/tif_strip.c",
    "tiff/libtiff/tif_swab.c",
    "tiff/libtiff/tif_thunder.c",
    "tiff/libtiff/tif_tile.c",
    "tiff/libtiff/tif_version.c",
    "tiff/libtiff/tif_warning.c",
    "tiff/libtiff/tif_webp.c",
    "tiff/libtiff/tif_win32.c",
    "tiff/libtiff/tif_write.c",
    "tiff/libtiff/tif_zip.c",
    "tiff/libtiff/tif_zstd.c",

    "png/png.c",
    "png/pngerror.c",
    "png/pngget.c",
    "png/pngmem.c",
    "png/pngpread.c",
    "png/pngread.c",
    "png/pngrio.c",
    "png/pngrtran.c",
    "png/pngrutil.c",
    "png/pngset.c",
    "png/pngtrans.c",
    "png/pngwio.c",
    "png/pngwrite.c",
    "png/pngwtran.c",
    "png/pngwutil.c",
    "png/arm/arm_init.c",
    "png/arm/filter_neon_intrinsics.c",
    "png/arm/palette_neon_intrinsics.c",
    "png/intel/intel_init.c",
    "png/intel/filter_sse2_intrinsics.c",

    "zlib/adler32.c",
    "zlib/compress.c",
    "zlib/crc32.c",
    "zlib/deflate.c",
    "zlib/gzclose.c",
    "zlib/gzlib.c",
    "zlib/gzread.c",
    "zlib/gzwrite.c",
    "zlib/infback.c",
    "zlib/inffast.c",
    "zlib/inflate.c",
    "zlib/inftrees.c",
    "zlib/trees.c",
    "zlib/uncompr.c",
    "zlib/zutil.c",

    "regex/regcomp.c",
    "regex/regerror.c",
    "regex/regexec.c",
    "regex/regfree.c",

    "expat/expat/lib/xmlparse.c",
    "expat/expat/lib/xmlrole.c",
    "expat/expat/lib/xmltok.c",

    "common/extended.c"
};

// build/files doesn't include the scintilla *.cxx files, so we hard-code them here

inline constexpr const auto txt_scintilla_files = {

    "stc/scintilla/src/AutoComplete.cxx",
    "stc/scintilla/src/CallTip.cxx",
    "stc/scintilla/src/CaseConvert.cxx",
    "stc/scintilla/src/CaseFolder.cxx",
    "stc/scintilla/src/Catalogue.cxx",
    "stc/scintilla/src/CellBuffer.cxx",
    "stc/scintilla/src/CellBuffer.cxx",
    "stc/scintilla/src/CharClassify.cxx",
    "stc/scintilla/src/ContractionState.cxx",
    "stc/scintilla/src/Decoration.cxx",
    "stc/scintilla/src/Document.cxx",
    "stc/scintilla/src/EditModel.cxx",
    "stc/scintilla/src/EditView.cxx",
    "stc/scintilla/src/Editor.cxx",
    "stc/scintilla/src/ExternalLexer.cxx",
    "stc/scintilla/src/Indicator.cxx",
    "stc/scintilla/src/KeyMap.cxx",
    "stc/scintilla/src/LineMarker.cxx",
    "stc/scintilla/src/MarginView.cxx",
    "stc/scintilla/src/PerLine.cxx",
    "stc/scintilla/src/PositionCache.cxx",
    "stc/scintilla/src/RESearch.cxx",
    "stc/scintilla/src/RunStyles.cxx",
    "stc/scintilla/src/ScintillaBase.cxx",
    "stc/scintilla/src/Selection.cxx",
    "stc/scintilla/src/Style.cxx",
    "stc/scintilla/src/UniConversion.cxx",
    "stc/scintilla/src/ViewStyle.cxx",
    "stc/scintilla/src/XPM.cxx",
    "stc/scintilla/lexlib/Accessor.cxx",
    "stc/scintilla/lexlib/CharacterCategory.cxx",
    "stc/scintilla/lexlib/CharacterSet.cxx",
    "stc/scintilla/lexlib/LexerBase.cxx",
    "stc/scintilla/lexlib/LexerModule.cxx",
    "stc/scintilla/lexlib/LexerNoExceptions.cxx",
    "stc/scintilla/lexlib/LexerSimple.cxx",
    "stc/scintilla/lexlib/PropSetSimple.cxx",
    "stc/scintilla/lexlib/StyleContext.cxx",
    "stc/scintilla/lexlib/WordList.cxx",
    "stc/scintilla/lexers/LexA68k.cxx",
    "stc/scintilla/lexers/LexAbaqus.cxx",
    "stc/scintilla/lexers/LexAda.cxx",
    "stc/scintilla/lexers/LexAPDL.cxx",
    "stc/scintilla/lexers/LexAsm.cxx",
    "stc/scintilla/lexers/LexAsn1.cxx",
    "stc/scintilla/lexers/LexASY.cxx",
    "stc/scintilla/lexers/LexAU3.cxx",
    "stc/scintilla/lexers/LexAVE.cxx",
    "stc/scintilla/lexers/LexAVS.cxx",
    "stc/scintilla/lexers/LexBaan.cxx",
    "stc/scintilla/lexers/LexBash.cxx",
    "stc/scintilla/lexers/LexBasic.cxx",
    "stc/scintilla/lexers/LexBatch.cxx",
    "stc/scintilla/lexers/LexBibTeX.cxx",
    "stc/scintilla/lexers/LexBullant.cxx",
    "stc/scintilla/lexers/LexCaml.cxx",
    "stc/scintilla/lexers/LexCLW.cxx",
    "stc/scintilla/lexers/LexCmake.cxx",
    "stc/scintilla/lexers/LexCOBOL.cxx",
    "stc/scintilla/lexers/LexCoffeeScript.cxx",
    "stc/scintilla/lexers/LexConf.cxx",
    "stc/scintilla/lexers/LexCPP.cxx",
    "stc/scintilla/lexers/LexCrontab.cxx",
    "stc/scintilla/lexers/LexCsound.cxx",
    "stc/scintilla/lexers/LexCSS.cxx",
    "stc/scintilla/lexers/LexD.cxx",
    "stc/scintilla/lexers/LexDiff.cxx",
    "stc/scintilla/lexers/LexDMAP.cxx",
    "stc/scintilla/lexers/LexDMIS.cxx",
    "stc/scintilla/lexers/LexECL.cxx",
    "stc/scintilla/lexers/LexEDIFACT.cxx",
    "stc/scintilla/lexers/LexEiffel.cxx",
    "stc/scintilla/lexers/LexErlang.cxx",
    "stc/scintilla/lexers/LexErrorList.cxx",
    "stc/scintilla/lexers/LexEScript.cxx",
    "stc/scintilla/lexers/LexFlagship.cxx",
    "stc/scintilla/lexers/LexForth.cxx",
    "stc/scintilla/lexers/LexFortran.cxx",
    "stc/scintilla/lexers/LexGAP.cxx",
    "stc/scintilla/lexers/LexGui4Cli.cxx",
    "stc/scintilla/lexers/LexHaskell.cxx",
    "stc/scintilla/lexers/LexHex.cxx",
    "stc/scintilla/lexers/LexHTML.cxx",
    "stc/scintilla/lexers/LexInno.cxx",
    "stc/scintilla/lexers/LexJSON.cxx",
    "stc/scintilla/lexers/LexKix.cxx",
    "stc/scintilla/lexers/LexKVIrc.cxx",
    "stc/scintilla/lexers/LexLaTeX.cxx",
    "stc/scintilla/lexers/LexLisp.cxx",
    "stc/scintilla/lexers/LexLout.cxx",
    "stc/scintilla/lexers/LexLua.cxx",
    "stc/scintilla/lexers/LexMagik.cxx",
    "stc/scintilla/lexers/LexMake.cxx",
    "stc/scintilla/lexers/LexMarkdown.cxx",
    "stc/scintilla/lexers/LexMatlab.cxx",
    "stc/scintilla/lexers/LexMetapost.cxx",
    "stc/scintilla/lexers/LexMMIXAL.cxx",
    "stc/scintilla/lexers/LexModula.cxx",
    "stc/scintilla/lexers/LexMPT.cxx",
    "stc/scintilla/lexers/LexMSSQL.cxx",
    "stc/scintilla/lexers/LexMySQL.cxx",
    "stc/scintilla/lexers/LexNimrod.cxx",
    "stc/scintilla/lexers/LexNsis.cxx",
    "stc/scintilla/lexers/LexNull.cxx",
    "stc/scintilla/lexers/LexOpal.cxx",
    "stc/scintilla/lexers/LexOScript.cxx",
    "stc/scintilla/lexers/LexPascal.cxx",
    "stc/scintilla/lexers/LexPB.cxx",
    "stc/scintilla/lexers/LexPerl.cxx",
    "stc/scintilla/lexers/LexPLM.cxx",
    "stc/scintilla/lexers/LexPO.cxx",
    "stc/scintilla/lexers/LexPOV.cxx",
    "stc/scintilla/lexers/LexPowerPro.cxx",
    "stc/scintilla/lexers/LexPowerShell.cxx",
    "stc/scintilla/lexers/LexProgress.cxx",
    "stc/scintilla/lexers/LexProps.cxx",
    "stc/scintilla/lexers/LexPS.cxx",
    "stc/scintilla/lexers/LexPython.cxx",
    "stc/scintilla/lexers/LexR.cxx",
    "stc/scintilla/lexers/LexRebol.cxx",
    "stc/scintilla/lexers/LexRegistry.cxx",
    "stc/scintilla/lexers/LexRuby.cxx",
    "stc/scintilla/lexers/LexRust.cxx",
    "stc/scintilla/lexers/LexScriptol.cxx",
    "stc/scintilla/lexers/LexSmalltalk.cxx",
    "stc/scintilla/lexers/LexSML.cxx",
    "stc/scintilla/lexers/LexSorcus.cxx",
    "stc/scintilla/lexers/LexSpecman.cxx",
    "stc/scintilla/lexers/LexSpice.cxx",
    "stc/scintilla/lexers/LexSQL.cxx",
    "stc/scintilla/lexers/LexSTTXT.cxx",
    "stc/scintilla/lexers/LexTACL.cxx",
    "stc/scintilla/lexers/LexTADS3.cxx",
    "stc/scintilla/lexers/LexTAL.cxx",
    "stc/scintilla/lexers/LexTCL.cxx",
    "stc/scintilla/lexers/LexTCMD.cxx",
    "stc/scintilla/lexers/LexTeX.cxx",
    "stc/scintilla/lexers/LexTxt2tags.cxx",
    "stc/scintilla/lexers/LexVB.cxx",
    "stc/scintilla/lexers/LexVerilog.cxx",
    "stc/scintilla/lexers/LexVHDL.cxx",
    "stc/scintilla/lexers/LexVisualProlog.cxx",
    "stc/scintilla/lexers/LexYAML.cxx",

};

int WidgetsFile::Convert(std::vector<ttlib::cstr>& files)
{
    if (files.size() < 2)
    {
        std::cerr << "both src and dest files must be specified" << '\n';
        return 1;
    }

    ttlib::textfile widget_files;
    if (!widget_files.ReadFile(files[0]))
    {
        std::cerr << "Unable to read " << files[0] << '\n';
        return 1;
    }

    size_t current_line = 0;

    bool ignore_all = true;

    bool is_gtk = false;
    bool is_osx = false;
    bool is_msw = false;

    for (auto& line: widget_files)
    {
        ++current_line;

        if (line.empty() || line[0] == '#')
        {
            continue;
        }
        else
        {
            if (auto pos = line.find('#'); ttlib::is_found(pos))
            {
                line.erase(pos);
                line.RightTrim();
                if (line.empty())
                {
                    continue;
                }
            }

            ttlib::cview view(line);
            view.moveto_nonspace();
            if (view.is_sameprefix("src/"))
            {
                view.remove_prefix(4);
            }

            if (view.contains(" ="))
            {
                ignore_all = false;
                is_gtk = (view.is_sameprefix("GTK") || view.is_sameprefix("NET_UNIX") || view.is_sameprefix("BASE_UNIX") ||
                          view.is_sameprefix("AUI_GTK"));
                is_osx = (view.is_sameprefix("OSX_") || view.is_sameprefix("NET_OSX") ||
                          view.is_sameprefix("BASE_COREFOUNDATION") || view.is_sameprefix("BASE_OSX"));
                is_msw = (view.is_sameprefix("MSW_") || view.is_sameprefix("NET_WIN32") ||
                          view.is_sameprefix("BASE_WIN32") || view.is_sameprefix("AUI_MSW"));
            }

            if (view.contains("_HDR "))
            {
                ignore_all = true;
                continue;  // we don't care about header files
            }
            else if (view.is_sameprefix("QT_"))
            {
                ignore_all = true;
                continue;  // we don't create wxQT
            }
            else if (view.is_sameprefix("DFB_"))
            {
                ignore_all = true;
                continue;  // we don't build wxDFB
            }
            else if (view.is_sameprefix("MOTIF_"))
            {
                ignore_all = true;
                continue;  // we don't build wxMOTIF
            }
            else if (view.is_sameprefix("XWIN_"))
            {
                ignore_all = true;
                continue;  // we don't build wxXWIN
            }
            else if (view.is_sameprefix("UNIV_"))
            {
                ignore_all = true;
                continue;  // we don't build wxUNIVERSAL
            }
            else if (view.contains("$("))
            {
                ignore_all = true;
                continue;  // we don't expand macros
            }

            if (!ignore_all && ttlib::is_whitespace(line[0]))
            {
                if (!view.contains(".cpp") && !view.contains(".cxx"))
                {
                    continue;  // ignore .manifest, .cur, .ico, .h, .etc
                }

                if (view.front() == '#')
                {
                    continue;
                }

                if (view.contains("unix") || view.contains("gtk"))
                {
                    m_unix_list.emplace(view.c_str());
                }
                else if (view.contains("msw"))
                {
                    m_msw_list.emplace(view.c_str());
                }
                else if (view.contains("osx") || view.contains("motif"))
                {
                    m_osx_list.emplace(view.c_str());
                }
                else if (view.contains("qt"))
                {
                    continue;  // this shouldn't happen, but let's be certain
                }
                else if (view.contains("x11"))
                {
                    continue;  // we don't build x11 code
                }
                else if (view.contains("univ"))
                {
                    continue;  // we don't build universal
                }
                else
                {
                    if (is_gtk)
                    {
                        m_unix_list.emplace(view.c_str());
                    }
                    else if (is_osx)
                    {
                        m_osx_list.emplace(view.c_str());
                    }
                    else if (is_msw)
                    {
                        m_msw_list.emplace(view.c_str());
                    }
                    else
                    {
                        ASSERT(!view.contains("accel.cpp"))
                        m_common_list.emplace(view.c_str());
                    }
                }
            }
        }
    }

    for (auto& iter: txt_scintilla_files)
    {
        m_common_list.emplace(iter);
    }

    ttlib::textfile output;

    output += "set (common_sources";
    for (auto& iter: m_common_list)
    {
        output.addEmptyLine() << "    ${CMAKE_CURRENT_LIST_DIR}/" << iter;
    }
    output += ")";
    output.addEmptyLine();

    output += "set (msw_sources";
    for (auto& iter: m_msw_list)
    {
        output.addEmptyLine() << "    ${CMAKE_CURRENT_LIST_DIR}/" << iter;
    }
    output += ")";
    output.addEmptyLine();

    output += "set (unix_sources";
    for (auto& iter: m_unix_list)
    {
        output.addEmptyLine() << "    ${CMAKE_CURRENT_LIST_DIR}/" << iter;
    }
    output += ")";
    output.addEmptyLine();

    output += "set (osx_sources";
    for (auto& iter: m_osx_list)
    {
        output.addEmptyLine() << "    ${CMAKE_CURRENT_LIST_DIR}/" << iter;
    }
    output += ")";
    output.addEmptyLine();

    output += "set (wxCLib_sources";
    for (auto& iter: txt_wxCLib_sources)
    {
        output.addEmptyLine() << "    ${CMAKE_CURRENT_LIST_DIR}/" << iter;
    }
    output += ")";

    output.WriteFile(files[1]);

    return 0;
}
