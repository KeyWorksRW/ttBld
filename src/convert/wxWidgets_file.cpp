/////////////////////////////////////////////////////////////////////////////
// Purpose:   Convert wxWidgets build/file to CMake file list
// Author:    Ralph Walden
// Copyright: Copyright (c) 2022 KeyWorks Software (Ralph Walden)
// License:   Apache License see ../../LICENSE
/////////////////////////////////////////////////////////////////////////////

#include <iostream>

#include "wxWidgets_file.h"

// build/files doesn't include the C files, so we hard-code them here

inline constexpr const auto txt_wxCLib_sources = R"===(set (wxCLib_sources
    ../src/jpeg/jaricom.c
    ../src/jpeg/jcapimin.c
    ../src/jpeg/jcapistd.c
    ../src/jpeg/jcarith.c
    ../src/jpeg/jccoefct.c
    ../src/jpeg/jccolor.c
    ../src/jpeg/jcdctmgr.c
    ../src/jpeg/jchuff.c
    ../src/jpeg/jcinit.c
    ../src/jpeg/jcmainct.c
    ../src/jpeg/jcmarker.c
    ../src/jpeg/jcmaster.c
    ../src/jpeg/jcomapi.c
    ../src/jpeg/jcparam.c
    ../src/jpeg/jcprepct.c
    ../src/jpeg/jcsample.c
    ../src/jpeg/jctrans.c
    ../src/jpeg/jdapimin.c
    ../src/jpeg/jdapistd.c
    ../src/jpeg/jdarith.c
    ../src/jpeg/jdatadst.c
    ../src/jpeg/jdatasrc.c
    ../src/jpeg/jdcoefct.c
    ../src/jpeg/jdcolor.c
    ../src/jpeg/jddctmgr.c
    ../src/jpeg/jdhuff.c
    ../src/jpeg/jdinput.c
    ../src/jpeg/jdmainct.c
    ../src/jpeg/jdmarker.c
    ../src/jpeg/jdmaster.c
    ../src/jpeg/jdmerge.c
    ../src/jpeg/jdpostct.c
    ../src/jpeg/jdsample.c
    ../src/jpeg/jdtrans.c
    ../src/jpeg/jerror.c
    ../src/jpeg/jfdctflt.c
    ../src/jpeg/jfdctfst.c
    ../src/jpeg/jfdctint.c
    ../src/jpeg/jidctflt.c
    ../src/jpeg/jidctfst.c
    ../src/jpeg/jidctint.c
    ../src/jpeg/jmemmgr.c
    ../src/jpeg/jmemnobs.c
    ../src/jpeg/jquant1.c
    ../src/jpeg/jquant2.c
    ../src/jpeg/jutils.c

    ../src/tiff/libtiff/tif_aux.c
    ../src/tiff/libtiff/tif_close.c
    ../src/tiff/libtiff/tif_codec.c
    ../src/tiff/libtiff/tif_color.c
    ../src/tiff/libtiff/tif_compress.c
    ../src/tiff/libtiff/tif_dir.c
    ../src/tiff/libtiff/tif_dirinfo.c
    ../src/tiff/libtiff/tif_dirread.c
    ../src/tiff/libtiff/tif_dirwrite.c
    ../src/tiff/libtiff/tif_dumpmode.c
    ../src/tiff/libtiff/tif_error.c
    ../src/tiff/libtiff/tif_extension.c
    ../src/tiff/libtiff/tif_fax3.c
    ../src/tiff/libtiff/tif_fax3sm.c
    ../src/tiff/libtiff/tif_flush.c
    ../src/tiff/libtiff/tif_getimage.c
    ../src/tiff/libtiff/tif_jbig.c
    ../src/tiff/libtiff/tif_jpeg.c
    ../src/tiff/libtiff/tif_jpeg_12.c
    ../src/tiff/libtiff/tif_luv.c
    ../src/tiff/libtiff/tif_lzma.c
    ../src/tiff/libtiff/tif_lzw.c
    ../src/tiff/libtiff/tif_next.c
    ../src/tiff/libtiff/tif_ojpeg.c
    ../src/tiff/libtiff/tif_open.c
    ../src/tiff/libtiff/tif_packbits.c
    ../src/tiff/libtiff/tif_pixarlog.c
    ../src/tiff/libtiff/tif_predict.c
    ../src/tiff/libtiff/tif_print.c
    ../src/tiff/libtiff/tif_read.c
    ../src/tiff/libtiff/tif_strip.c
    ../src/tiff/libtiff/tif_swab.c
    ../src/tiff/libtiff/tif_thunder.c
    ../src/tiff/libtiff/tif_tile.c
    ../src/tiff/libtiff/tif_version.c
    ../src/tiff/libtiff/tif_warning.c
    ../src/tiff/libtiff/tif_webp.c
    ../src/tiff/libtiff/tif_win32.c
    ../src/tiff/libtiff/tif_write.c
    ../src/tiff/libtiff/tif_zip.c
    ../src/tiff/libtiff/tif_zstd.c

    ../src/png/png.c
    ../src/png/pngerror.c
    ../src/png/pngget.c
    ../src/png/pngmem.c
    ../src/png/pngpread.c
    ../src/png/pngread.c
    ../src/png/pngrio.c
    ../src/png/pngrtran.c
    ../src/png/pngrutil.c
    ../src/png/pngset.c
    ../src/png/pngtrans.c
    ../src/png/pngwio.c
    ../src/png/pngwrite.c
    ../src/png/pngwtran.c
    ../src/png/pngwutil.c
    ../src/png/arm/arm_init.c
    ../src/png/arm/filter_neon_intrinsics.c
    ../src/png/arm/palette_neon_intrinsics.c
    ../src/png/intel/intel_init.c
    ../src/png/intel/filter_sse2_intrinsics.c

    ../src/zlib/adler32.c
    ../src/zlib/compress.c
    ../src/zlib/crc32.c
    ../src/zlib/deflate.c
    ../src/zlib/gzclose.c
    ../src/zlib/gzlib.c
    ../src/zlib/gzread.c
    ../src/zlib/gzwrite.c
    ../src/zlib/infback.c
    ../src/zlib/inffast.c
    ../src/zlib/inflate.c
    ../src/zlib/inftrees.c
    ../src/zlib/trees.c
    ../src/zlib/uncompr.c
    ../src/zlib/zutil.c

    ../src/regex/regcomp.c
    ../src/regex/regerror.c
    ../src/regex/regexec.c
    ../src/regex/regfree.c

    ../src/expat/expat/lib/xmlparse.c
    ../src/expat/expat/lib/xmlrole.c
    ../src/expat/expat/lib/xmltok.c

    ../src/common/extended.c
)
)===";

// build/files doesn't include the scintilla *.cxx files, so we hard-code them here

inline constexpr const auto txt_scintilla_files = {

    "src/stc/scintilla/src/AutoComplete.cxx",
    "src/stc/scintilla/src/CallTip.cxx",
    "src/stc/scintilla/src/CaseConvert.cxx",
    "src/stc/scintilla/src/CaseFolder.cxx",
    "src/stc/scintilla/src/Catalogue.cxx",
    "src/stc/scintilla/src/CellBuffer.cxx",
    "src/stc/scintilla/src/CellBuffer.cxx",
    "src/stc/scintilla/src/CharClassify.cxx",
    "src/stc/scintilla/src/ContractionState.cxx",
    "src/stc/scintilla/src/Decoration.cxx",
    "src/stc/scintilla/src/Document.cxx",
    "src/stc/scintilla/src/EditModel.cxx",
    "src/stc/scintilla/src/EditView.cxx",
    "src/stc/scintilla/src/Editor.cxx",
    "src/stc/scintilla/src/ExternalLexer.cxx",
    "src/stc/scintilla/src/Indicator.cxx",
    "src/stc/scintilla/src/KeyMap.cxx",
    "src/stc/scintilla/src/LineMarker.cxx",
    "src/stc/scintilla/src/MarginView.cxx",
    "src/stc/scintilla/src/PerLine.cxx",
    "src/stc/scintilla/src/PositionCache.cxx",
    "src/stc/scintilla/src/RESearch.cxx",
    "src/stc/scintilla/src/RunStyles.cxx",
    "src/stc/scintilla/src/ScintillaBase.cxx",
    "src/stc/scintilla/src/Selection.cxx",
    "src/stc/scintilla/src/Style.cxx",
    "src/stc/scintilla/src/UniConversion.cxx",
    "src/stc/scintilla/src/ViewStyle.cxx",
    "src/stc/scintilla/src/XPM.cxx",
    "src/stc/scintilla/lexlib/Accessor.cxx",
    "src/stc/scintilla/lexlib/CharacterCategory.cxx",
    "src/stc/scintilla/lexlib/CharacterSet.cxx",
    "src/stc/scintilla/lexlib/LexerBase.cxx",
    "src/stc/scintilla/lexlib/LexerModule.cxx",
    "src/stc/scintilla/lexlib/LexerNoExceptions.cxx",
    "src/stc/scintilla/lexlib/LexerSimple.cxx",
    "src/stc/scintilla/lexlib/PropSetSimple.cxx",
    "src/stc/scintilla/lexlib/StyleContext.cxx",
    "src/stc/scintilla/lexlib/WordList.cxx",
    "src/stc/scintilla/lexers/LexA68k.cxx",
    "src/stc/scintilla/lexers/LexAbaqus.cxx",
    "src/stc/scintilla/lexers/LexAda.cxx",
    "src/stc/scintilla/lexers/LexAPDL.cxx",
    "src/stc/scintilla/lexers/LexAsm.cxx",
    "src/stc/scintilla/lexers/LexAsn1.cxx",
    "src/stc/scintilla/lexers/LexASY.cxx",
    "src/stc/scintilla/lexers/LexAU3.cxx",
    "src/stc/scintilla/lexers/LexAVE.cxx",
    "src/stc/scintilla/lexers/LexAVS.cxx",
    "src/stc/scintilla/lexers/LexBaan.cxx",
    "src/stc/scintilla/lexers/LexBash.cxx",
    "src/stc/scintilla/lexers/LexBasic.cxx",
    "src/stc/scintilla/lexers/LexBatch.cxx",
    "src/stc/scintilla/lexers/LexBibTeX.cxx",
    "src/stc/scintilla/lexers/LexBullant.cxx",
    "src/stc/scintilla/lexers/LexCaml.cxx",
    "src/stc/scintilla/lexers/LexCLW.cxx",
    "src/stc/scintilla/lexers/LexCmake.cxx",
    "src/stc/scintilla/lexers/LexCOBOL.cxx",
    "src/stc/scintilla/lexers/LexCoffeeScript.cxx",
    "src/stc/scintilla/lexers/LexConf.cxx",
    "src/stc/scintilla/lexers/LexCPP.cxx",
    "src/stc/scintilla/lexers/LexCrontab.cxx",
    "src/stc/scintilla/lexers/LexCsound.cxx",
    "src/stc/scintilla/lexers/LexCSS.cxx",
    "src/stc/scintilla/lexers/LexD.cxx",
    "src/stc/scintilla/lexers/LexDiff.cxx",
    "src/stc/scintilla/lexers/LexDMAP.cxx",
    "src/stc/scintilla/lexers/LexDMIS.cxx",
    "src/stc/scintilla/lexers/LexECL.cxx",
    "src/stc/scintilla/lexers/LexEDIFACT.cxx",
    "src/stc/scintilla/lexers/LexEiffel.cxx",
    "src/stc/scintilla/lexers/LexErlang.cxx",
    "src/stc/scintilla/lexers/LexErrorList.cxx",
    "src/stc/scintilla/lexers/LexEScript.cxx",
    "src/stc/scintilla/lexers/LexFlagship.cxx",
    "src/stc/scintilla/lexers/LexForth.cxx",
    "src/stc/scintilla/lexers/LexFortran.cxx",
    "src/stc/scintilla/lexers/LexGAP.cxx",
    "src/stc/scintilla/lexers/LexGui4Cli.cxx",
    "src/stc/scintilla/lexers/LexHaskell.cxx",
    "src/stc/scintilla/lexers/LexHex.cxx",
    "src/stc/scintilla/lexers/LexHTML.cxx",
    "src/stc/scintilla/lexers/LexInno.cxx",
    "src/stc/scintilla/lexers/LexJSON.cxx",
    "src/stc/scintilla/lexers/LexKix.cxx",
    "src/stc/scintilla/lexers/LexKVIrc.cxx",
    "src/stc/scintilla/lexers/LexLaTeX.cxx",
    "src/stc/scintilla/lexers/LexLisp.cxx",
    "src/stc/scintilla/lexers/LexLout.cxx",
    "src/stc/scintilla/lexers/LexLua.cxx",
    "src/stc/scintilla/lexers/LexMagik.cxx",
    "src/stc/scintilla/lexers/LexMake.cxx",
    "src/stc/scintilla/lexers/LexMarkdown.cxx",
    "src/stc/scintilla/lexers/LexMatlab.cxx",
    "src/stc/scintilla/lexers/LexMetapost.cxx",
    "src/stc/scintilla/lexers/LexMMIXAL.cxx",
    "src/stc/scintilla/lexers/LexModula.cxx",
    "src/stc/scintilla/lexers/LexMPT.cxx",
    "src/stc/scintilla/lexers/LexMSSQL.cxx",
    "src/stc/scintilla/lexers/LexMySQL.cxx",
    "src/stc/scintilla/lexers/LexNimrod.cxx",
    "src/stc/scintilla/lexers/LexNsis.cxx",
    "src/stc/scintilla/lexers/LexNull.cxx",
    "src/stc/scintilla/lexers/LexOpal.cxx",
    "src/stc/scintilla/lexers/LexOScript.cxx",
    "src/stc/scintilla/lexers/LexPascal.cxx",
    "src/stc/scintilla/lexers/LexPB.cxx",
    "src/stc/scintilla/lexers/LexPerl.cxx",
    "src/stc/scintilla/lexers/LexPLM.cxx",
    "src/stc/scintilla/lexers/LexPO.cxx",
    "src/stc/scintilla/lexers/LexPOV.cxx",
    "src/stc/scintilla/lexers/LexPowerPro.cxx",
    "src/stc/scintilla/lexers/LexPowerShell.cxx",
    "src/stc/scintilla/lexers/LexProgress.cxx",
    "src/stc/scintilla/lexers/LexProps.cxx",
    "src/stc/scintilla/lexers/LexPS.cxx",
    "src/stc/scintilla/lexers/LexPython.cxx",
    "src/stc/scintilla/lexers/LexR.cxx",
    "src/stc/scintilla/lexers/LexRebol.cxx",
    "src/stc/scintilla/lexers/LexRegistry.cxx",
    "src/stc/scintilla/lexers/LexRuby.cxx",
    "src/stc/scintilla/lexers/LexRust.cxx",
    "src/stc/scintilla/lexers/LexScriptol.cxx",
    "src/stc/scintilla/lexers/LexSmalltalk.cxx",
    "src/stc/scintilla/lexers/LexSML.cxx",
    "src/stc/scintilla/lexers/LexSorcus.cxx",
    "src/stc/scintilla/lexers/LexSpecman.cxx",
    "src/stc/scintilla/lexers/LexSpice.cxx",
    "src/stc/scintilla/lexers/LexSQL.cxx",
    "src/stc/scintilla/lexers/LexSTTXT.cxx",
    "src/stc/scintilla/lexers/LexTACL.cxx",
    "src/stc/scintilla/lexers/LexTADS3.cxx",
    "src/stc/scintilla/lexers/LexTAL.cxx",
    "src/stc/scintilla/lexers/LexTCL.cxx",
    "src/stc/scintilla/lexers/LexTCMD.cxx",
    "src/stc/scintilla/lexers/LexTeX.cxx",
    "src/stc/scintilla/lexers/LexTxt2tags.cxx",
    "src/stc/scintilla/lexers/LexVB.cxx",
    "src/stc/scintilla/lexers/LexVerilog.cxx",
    "src/stc/scintilla/lexers/LexVHDL.cxx",
    "src/stc/scintilla/lexers/LexVisualProlog.cxx",
    "src/stc/scintilla/lexers/LexYAML.cxx",

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

                if (view.contains("src/unix") || view.contains("src/gtk"))
                {
                    m_unix_list.emplace(view.c_str());
                }
                else if (view.contains("src/msw"))
                {
                    m_msw_list.emplace(view.c_str());
                }
                else if (view.contains("src/osx") || view.contains("src/motif"))
                {
                    m_osx_list.emplace(view.c_str());
                }
                else if (view.contains("src/qt"))
                {
                    continue;  // this shouldn't happen, but let's be certain
                }
                else if (view.contains("src/x11"))
                {
                    continue;  // we don't build x11 code
                }
                else if (view.contains("src/univ"))
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
        output.addEmptyLine() << "    ../" << iter;
    }
    output += ")";
    output.addEmptyLine();

    output += "set (msw_sources";
    for (auto& iter: m_msw_list)
    {
        output.addEmptyLine() << "    ../" << iter;
    }
    output += ")";
    output.addEmptyLine();

    output += "set (unix_sources";
    for (auto& iter: m_unix_list)
    {
        output.addEmptyLine() << "    ../" << iter;
    }
    output += ")";
    output.addEmptyLine();

    output += "set (osx_sources";
    for (auto& iter: m_osx_list)
    {
        output.addEmptyLine() << "    ../" << iter;
    }
    output += ")";
    output.addEmptyLine();

    output += txt_wxCLib_sources;

    output.WriteFile(files[1]);

    return 0;
}
