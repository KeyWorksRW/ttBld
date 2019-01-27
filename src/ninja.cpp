/////////////////////////////////////////////////////////////////////////////
// Name:		CNinja
// Purpose:		Class for creating/maintaining build.ninja file for use by ninja.exe build tool
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

// This will create one build?.ninja for each build target (debug, debug64, release, release64). Each build file will be
// placed in a build directory with a compiler prefix (build/clangBuild.ninja or build/msvcBuild.ninja). The build
// directory will also hold the dependency files that ninja creates.

#include "pch.h"

#include "../ttLib/include/ttfile.h"	// ttFile
#include "../ttLib/include/enumstr.h"	// ttEnumStr

#include "ninja.h"				// CNinja
#include "parsehhp.h"			// CParseHHP

const char* aCppExt[] = {
	".cpp",
	".cxx",
	".cc",

	nullptr
};

bool CNinja::CreateBuildFile(GEN_TYPE gentype, size_t Compiler)
{
	ttFile file;
	m_pkfOut = &file;
	m_gentype = gentype;
	m_Compiler = Compiler;

	ttString cszExtraLib;		// used to specify the exact name/path of the "extra" library (if any)
	if (getLibName()) {		// start with the extra lib if there is one
		cszExtraLib = "$libout/";
		cszExtraLib += tt::FindFilePortion(getLibName());
		cszExtraLib.ChangeExtension(".lib");
	}

	ttString cszProj(getProjName());	// If needed, add a suffix to the project name

	// Don't add the 'D' to the end of DLL's -- it is perfectly viable for a release app to use a debug dll and that
	// won't work if the filename has changed. Under MSVC Linker, it will also generate a LNK4070 error if the dll name
	// is specified in the matching .def file. The downside, of course, is that without the 'D' only the size of the dll
	// indicates if it is a debug or release version.

	if (isExeTypeDll())	{
		if (is64BitSuffix())
			cszProj += "64";
	}
	else {
		if (gentype == GEN_DEBUG64 && is64BitSuffix())
			cszProj += isBin64() ? "D" : "64D";		// isBin64() checks to see if ../bin64 exists
		else if (gentype == GEN_DEBUG)
			cszProj += "D";
		else if (gentype == GEN_RELEASE64 && is64BitSuffix() && !isBin64())
			cszProj += "64";
	}

	file.SetUnixLF();	// WARNING!!! NINJA doesn't allow \r characters (or \t for that matter)
	file.printf("# WARNING: THIS FILE IS AUTO-GENERATED by %s. CHANGES YOU MAKE WILL BE LOST IF IT IS AUTO-GENERATED AGAIN.\n\n", txtVersion);
	file.WriteEol("ninja_required_version = 1.8\n");
	file.WriteEol("builddir = build");

	ttString cszTmp;
	if (gentype == GEN_DEBUG)
		cszTmp.printf("outdir = build/debug");
	else if (gentype == GEN_DEBUG64)
		cszTmp.printf("outdir = build/debug64");
	else if (gentype == GEN_RELEASE)
		cszTmp.printf("outdir = build/release");
	else if (gentype == GEN_RELEASE64)
		cszTmp.printf("outdir = build/release64");
	file.WriteEol(cszTmp);
	file.WriteEol("resout = build/res");

	if (getLibName()) {
		cszTmp.Delete();
		if (gentype == GEN_DEBUG)
			cszTmp.printf("libout = build/debugLib");
		else if (gentype == GEN_DEBUG64)
			cszTmp.printf("libout = build/debugLib64");
		else if (gentype == GEN_RELEASE)
			cszTmp.printf("libout = build/releaseLib");
		else if (gentype == GEN_RELEASE64)
			cszTmp.printf("libout = build/releaseLib64");
		file.WriteEol(cszTmp);
		file.WriteEol("libres = build/resLib");
	}

	file.WriteEol();
	cszTmp.Delete();

	WriteCompilerComments();

	// Figure out the filenames to use for the source and output for a precompiled header

	if (getPchName()) {
		m_cszPCH = getProjName();
		m_cszPCH.ChangeExtension(".pch");
		m_cszCPP_PCH = getPchName();

		size_t pos;
		for (pos = 0; aCppExt[pos]; pos++) {		// find out what type of C++ extension is being used
			m_cszCPP_PCH.ChangeExtension(aCppExt[pos]);
			if (tt::FileExists(m_cszCPP_PCH))
				break;
		}
		ttASSERT_MSG(aCppExt[pos], "No C++ source file found to generate precompiled header.");	// Probably means getPchName() is pointing to an invalid header file
		if (!aCppExt[pos]) {
			ttString cszErrorMsg;
			cszErrorMsg.printf("No C++ source file found that matches %s -- precompiled header will not build correctly.\n", getPchName());
			puts(cszErrorMsg);

			for (pos = 0; pos < getSrcFileList()->GetCount(); pos++) {
				const char* pszExt = tt::findstri(getSrcFileList()->GetAt(pos), ".c");
				if (pszExt) {
					m_cszCPP_PCH.ChangeExtension(pszExt);	// match extension used by other source files
					break;
				}
			}
		}
		if (m_cszCPP_PCH.isnonempty()) {
			m_cszPCHObj = m_cszCPP_PCH;
			m_cszPCHObj.ChangeExtension(".obj");
		}
	}

	WriteCompilerFlags();
	WriteCompilerDirectives();
	WriteRcDirective();
	WriteMidlDirective(gentype);
	WriteLibDirective();
	WriteLinkDirective();

	// Write the build rule for the precompiled header

	if (getPchName()) {
		ttString cszPchObj = m_cszCPP_PCH;
		cszPchObj.ChangeExtension(".obj");
		file.printf("build $outdir/%s: compilePCH %s\n\n", (char*) m_cszPCHObj, (char*) m_cszCPP_PCH);
	}

	// TODO: [randalphwa - 09-02-2018] Need to add build rule for any .idl files (uses midl compiler to make a .tlb file)

	// Write the build rules for all source files

	for (size_t iPos = 0; iPos < getSrcFileList()->GetCount(); iPos++) {
		ttString cszFile(tt::FindFilePortion(getSrcFileList()->GetAt(iPos)));
		if (!tt::findstri(cszFile, ".c") || (m_cszCPP_PCH.isnonempty() &&  tt::samestri(cszFile, m_cszCPP_PCH)))	// we already handled resources and pre-compiled headers
			continue;	// we already handled this
		cszFile.ChangeExtension(".obj");

		if (m_cszPCH.isnonempty())
			file.printf("build $outdir/%s: compile %s | $outdir/%s\n\n", (char*) cszFile, getSrcFileList()->GetAt(iPos), (char*) m_cszPCHObj);
		else
			file.printf("build $outdir/%s: compile %s\n\n", (char*) cszFile, getSrcFileList()->GetAt(iPos));
	}

	// Write the build rules for all lib files

	for (size_t iPos = 0; iPos < getLibFileList()->GetCount(); iPos++) {
		ttString cszFile(tt::FindFilePortion(getLibFileList()->GetAt(iPos)));
		if (!tt::findstri(cszFile, ".c") || (m_cszCPP_PCH.isnonempty() && tt::samestri(cszFile, m_cszCPP_PCH)))	// we already handled resources and pre-compiled headers
			continue;	// we already handled this
		cszFile.ChangeExtension(".obj");

		if (m_cszPCH.isnonempty())
			file.printf("build $libout/%s: compile %s | $outdir/%s\n\n", (char*) cszFile, getLibFileList()->GetAt(iPos), (char*) m_cszPCHObj);
		else
			file.printf("build $libout/%s: compile %s\n\n", (char*) cszFile, tt::FindFilePortion(getLibFileList()->GetAt(iPos)));
	}

	// Write the build rule for the resource compiler if an .rc file was specified as a source

	ttString cszRES;
	if (tt::FileExists(getRcFile())) {
		cszRES = getRcFile();
		cszRES.ChangeExtension(".res");
		file.printf("build $resout/%s: rc %s", (char*) cszRES, getRcFile());

		if (getRcDepList()->GetCount())
			file.WriteStr(" |");
		for (size_t nPos = 0; nPos < getRcDepList()->GetCount(); nPos++) {
			file.WriteStr(" $\n  ");
			file.WriteStr(getRcDepList()->Get(nPos));
		}
		file.WriteEol("\n");
	}

	// Write the final build rules to complete the project

	if (!isExeTypeLib() && getLibName()) {	// If an extra library was specified, add it's build rule first
		file.printf("build %s : lib", (char*) cszExtraLib);
		for (size_t posLib = 0; posLib < getLibFileList()->GetCount(); posLib++) {
			ttString cszFile(tt::FindFilePortion(getLibFileList()->GetAt(posLib)));
			if (!tt::findstri(cszFile, ".c"))	// we don't care about any type of file that wasn't compiled into an .obj file
				continue;
			cszFile.ChangeExtension(".obj");
			file.printf(" $\n  $libout/%s", (char*) cszFile);
		}
		file.WriteEol("\n");
	}

	WriteMidlTargets();
	WriteLinkTargets(gentype);

	if (!tt::DirExists("build")) {
		if (!tt::CreateDir("build")) {
			AddError("Unable to create the build directory -- so no place to put the .ninja files!\n");
			return false;
		}
	}

	switch (gentype) {
		case GEN_DEBUG:
			cszTmp.AppendFileName((m_Compiler == CSrcFiles::COMPILER_CLANG) ? "build/clangBuildD.ninja" : "build/msvcBuildD.ninja");
			break;
		case GEN_DEBUG64:
			cszTmp.AppendFileName((m_Compiler == CSrcFiles::COMPILER_CLANG) ? "build/clangBuild64D.ninja" : "build/msvcBuild64D.ninja");
			break;
		case GEN_RELEASE64:
			cszTmp.AppendFileName((m_Compiler == CSrcFiles::COMPILER_CLANG) ? "build/clangBuild64.ninja" : "build/msvcBuild64.ninja");
			break;
		case GEN_RELEASE:
		default:
			cszTmp.AppendFileName((m_Compiler == CSrcFiles::COMPILER_CLANG) ? "build/clangBuild.ninja" : "build/msvcBuild.ninja");
			break;
	}

	// We don't want to touch the build script if it hasn't changed, since that would change the timestamp causing git
	// and other tools that check timestamp to think something is different.

	ttFile fileOrg;
	if (fileOrg.ReadFile(cszTmp)) {
		if (!m_bForceOutput && strcmp(fileOrg, file) == 0)
			return false;
		else if (dryrun.isEnabled()) {
			dryrun.NewFile(cszTmp);
			dryrun.DisplayFileDiff(fileOrg, file);
			return false;	// we didn't actually write anything
		}
	}

	if (!file.WriteFile(cszTmp)) {
		ttString cszMsg;
		cszMsg.printf("Cannot write to %s\n", (char*) cszTmp);
		AddError(cszMsg);
		return false;
	}
	return true;
}

void CNinja::WriteCompilerComments()
{
	if (m_Compiler == CSrcFiles::COMPILER_MSVC) {
		m_pkfOut->WriteEol("msvc_deps_prefix = Note: including file:\n");

		// Write comment section explaining the compiler flags in use

		m_pkfOut->WriteEol("# -EHsc\t// Structured exception handling");
		if (m_gentype == GEN_RELEASE || m_gentype == GEN_RELEASE64)	{
			m_pkfOut->WriteEol("# -GL\t// Whole program optimization");
			m_pkfOut->WriteEol("# -GS-\t// Turn off buffer security checks");
			if (isStdcall())
				m_pkfOut->WriteEol("# -Gz\t// __stdcall calling convention");
			if (isStaticCrt())
				m_pkfOut->WriteEol("# -MT\t// Static multi-threaded library");
			else
				m_pkfOut->WriteEol("# -MD\t// DLL version of multi-threaded library");
			if (isExeTypeLib())
				m_pkfOut->WriteEol("# -Zl\t// Don't specify default runtime library in .obj file");
			if (isBuildSpeed())
				m_pkfOut->WriteEol("# -O2\t// Optimize for speed (/Og /Oi /Ot /Oy /Ob2 /Gs /GF /Gy)");
			else
				m_pkfOut->WriteEol("# -O1\t// Optimize for size (/Og /Os /Oy /Ob2 /Gs /GF /Gy)");
		}
		else {	// Presumably GEN_DEBUG or GEN_DEBUG64
			m_pkfOut->WriteEol("# -MDd\t// Multithreaded debug dll (MSVCRTD)");
			m_pkfOut->WriteEol("# -Z7\t// produces object files with full symbolic debugging information");
		}
	}
	else if (m_Compiler == CSrcFiles::COMPILER_CLANG) {
		m_pkfOut->WriteEol("msvc_deps_prefix = Note: including file:\n");

		// Write comment section explaining the compiler flags in use

		m_pkfOut->WriteEol("# -EHsc\t// Structured exception handling");
		if (m_gentype == GEN_RELEASE || m_gentype == GEN_RELEASE64)	{
			if (isStdcall())
				m_pkfOut->WriteEol("# -Gz\t// __stdcall calling convention");
			if (isStaticCrt())
				m_pkfOut->WriteEol("# -MT\t// Static multi-threaded library");
			else
				m_pkfOut->WriteEol("# -MD\t// DLL version of multi-threaded library");
			if (isExeTypeLib())
				m_pkfOut->WriteEol("# -Zl\t// Don't specify default runtime library in .obj file");
			if (isBuildSpeed())
				m_pkfOut->WriteEol("# -O2\t// Optimize for speed (/Og /Oi /Ot /Oy /Ob2 /Gs /GF /Gy)");
			else
				m_pkfOut->WriteEol("# -O1\t// Optimize for size (/Og /Os /Oy /Ob2 /Gs /GF /Gy)");
		}
		else {	// Presumably GEN_DEBUG or GEN_DEBUG64
			m_pkfOut->WriteEol("# -MDd\t// Multithreaded debug dll (MSVCRTD)");
			m_pkfOut->WriteEol("# -Z7\t// produces object files with full symbolic debugging information");
		}
	}
	m_pkfOut->WriteEol();	// force a blank line after the options are listed
}

// The CLANG compiler we are writing for is clang-cl.exe, which means most of the compiler flags are common for both CLANG and MSVC

void CNinja::WriteCompilerFlags()
{
	// First we write the flags common to both compilers

	if (m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG64)
		m_pkfOut->printf("cflags = -nologo -D_DEBUG -showIncludes -EHsc%s -W%d%s%s -Od -Z7 -GS-",
				isExeTypeConsole() ? " -D_CONSOLE" : "",

				(int) getWarnLevel(),

				isStdcall() ?	 " -Gz" : "",
				isExeTypeLib() ? " -Zl" : " -MDd"   // Note use of -MDd -- assumption is to always use this for debug builds. Release builds track isStaticCrt()
			);
	else	// Presumably GEN_RELEASE or GEN_RELEASE64
		m_pkfOut->printf("cflags = -nologo -DNDEBUG -showIncludes -EHsc%s -W%d%s%s%s",
				isExeTypeConsole() ? " -D_CONSOLE" : "",

				(int) getWarnLevel(),

				isStdcall() ?    " -Gz" : "",
				isExeTypeLib() ? " -Zl" :  (isStaticCrt() ? " -MT" : " -MD"),
				isBuildSpeed() ? " -O2" : " -O1"
			);

	if (getIncDirs()) {
		ttEnumStr cEnumStr(getIncDirs());
		while (cEnumStr.Enum()) {
			m_pkfOut->printf(" -I%kq", (const char*) cEnumStr);
		}
	}

	if (getCFlags()) {
		m_pkfOut->WriteChar(' ');
		m_pkfOut->WriteStr(getCFlags());
	}

	// Now write out the compiler-specific flags

	if (m_Compiler == CSrcFiles::COMPILER_MSVC) {
		if (isPermissive())
			m_pkfOut->WriteStr(" -permissive-");
		if (m_gentype == GEN_RELEASE || m_gentype == GEN_RELEASE64)
			m_pkfOut->WriteStr(" -GL");	// whole program optimization
	}

	else if (m_Compiler == CSrcFiles::COMPILER_CLANG) {
		m_pkfOut->WriteStr(" -D__clang__");	// unlike the non-MSVC compatible version, clang-cl.exe (version 7) doesn't define this
		m_pkfOut->WriteStr(" -fms-compatibility-version=19");	// Version of MSVC to be compatible with
		m_pkfOut->WriteStr(m_gentype == GEN_DEBUG64 || m_gentype == GEN_RELEASE64 ? " -m64" : " -m32");	// specify the platform
		if (m_gentype == GEN_RELEASE || m_gentype == GEN_RELEASE64)
			m_pkfOut->WriteStr(" -flto -fwhole-program-vtables");	// whole program optimization
	}

	if (getPchName())
		m_pkfOut->printf(" /Fp$outdir/%s", (char*) m_cszPCH);

	m_pkfOut->WriteEol("\n");
}

void CNinja::WriteCompilerDirectives()
{
	if (m_Compiler == CSrcFiles::COMPILER_MSVC) {
		if (getPchName()) {
			m_pkfOut->printf(
					"rule compilePCH\n"
					"  deps = msvc\n"
					"  command = cl.exe -c $cflags -Fo$outdir/ $in -Fd$outdir/%s.pdb -Yc%s\n",
					getProjName(),
					getPchName() 					// typically stdafx.h or precomp.h
					);
			m_pkfOut->WriteEol("  description = compiling $in\n");
		}

		// Write compile directive

		if (m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG64)	{
			m_pkfOut->printf(
					"rule compile\n"
					"  deps = msvc\n"
					"  command = cl.exe -c $cflags -Fo$out $in -Fd$outdir/%s.pdb %s%s\n",
					getProjName(),
					getPchName() ? "-Yu" : "", getPchName() ? getPchName() : ""
					);
		}
		else {
			m_pkfOut->printf(
					"rule compile\n"
					"  deps = msvc\n"
					"  command = cl.exe -c $cflags -Fo$out $in %s%s\n",
					getPchName() ? "-Yu" : "",
					getPchName() ? getPchName() : ""
					);
		}
		m_pkfOut->WriteEol("  description = compiling $in\n");
	}
	else if (m_Compiler == CSrcFiles::COMPILER_CLANG) {
		if (getPchName()) {
			m_pkfOut->printf(
					"rule compilePCH\n"
					"  deps = msvc\n"
					"  command = clang-cl.exe -c $cflags -Fo$outdir/ $in -Fd$outdir/%s.pdb -Yc%s\n",
					getProjName(),
					getPchName() 					// typically stdafx.h or precomp.h
					);
			m_pkfOut->WriteEol("  description = compiling $in\n");
		}

		// Write compile directive

		if (m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG64)	{
			m_pkfOut->printf(
					"rule compile\n"
					"  deps = msvc\n"	// clang-cl supports -showIncludes, same as msvc
					"  command = clang-cl.exe -c $cflags -Fo$out $in -Fd$outdir/%s.pdb %s%s\n",
					getProjName(),
					getPchName() ? "-Yu" : "", getPchName() ? getPchName() : ""
					);
		}
		else {
			m_pkfOut->printf(
					"rule compile\n"
					"  deps = msvc\n"
					"  command = clang-cl.exe -c $cflags -Fo$out $in %s%s\n",
					getPchName() ? "-Yu" : "",
					getPchName() ? getPchName() : ""
					);
		}
		m_pkfOut->WriteEol("  description = compiling $in\n");
	}
}

void CNinja::WriteLinkDirective()
{
	if (isExeTypeLib())
		return;	// lib directive should be used if the project is a library

	if (m_Compiler == CSrcFiles::COMPILER_MSVC || m_bUseMsvcLinker) {
		ttString cszRule("rule link\n  command = link.exe /OUT:$out /NOLOGO /MANIFEST:NO ");
		cszRule += (m_gentype == GEN_DEBUG64 || m_gentype == GEN_RELEASE64 ? "/MACHINE:x64" : "/MACHINE:x86");

		if (getLinkFlags()) {
			cszRule += " ";
			cszRule += getLinkFlags();
		}

		if (m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG64)	{
			cszRule +=" /DEBUG /PDB:$outdir/";
			cszRule += getProjName();
			cszRule += ".pdb";
		}
		else
			cszRule += " /LTCG /OPT:REF /OPT:ICF";

		cszRule += isExeTypeConsole() ? " /SUBSYSTEM:CONSOLE" : " /SUBSYSTEM:WINDOWS";
		cszRule += " $in";
		m_pkfOut->WriteEol(cszRule);
		m_pkfOut->WriteEol("  description = linking $out\n");
	}
	else if (m_Compiler == CSrcFiles::COMPILER_CLANG) {
		ttString cszRule("rule link\n  command = lld-link.exe");
		if (isExeTypeDll())
			cszRule += " /dll";
		cszRule += " /out:$out /manifest:no";
		cszRule += (m_gentype == GEN_DEBUG64 || m_gentype == GEN_RELEASE64 ? " /machine:x64" : " /machine:x86");

		if (getLinkFlags()) {
			cszRule += " ";
			cszRule += getLinkFlags();
		}

		if (m_gentype == GEN_DEBUG || m_gentype == GEN_DEBUG64)	{
			cszRule +=" /debug /pdb:$outdir/";
			cszRule += getProjName();
			cszRule += ".pdb";
		}
		else {
			// MSVC -LTCG option is not supported by lld
			cszRule += " /opt:ref /opt:icf";
		}
		cszRule += isExeTypeConsole() ? " /subsystem:console" : " /subsystem:windows";
		cszRule += " $in";
		m_pkfOut->WriteEol(cszRule);
		m_pkfOut->WriteEol("  description = linking $out\n");
	}
}

void CNinja::WriteLibDirective()
{
	if (m_Compiler == CSrcFiles::COMPILER_MSVC) {
		if (isExeTypeLib() || getLibName())	{
			m_pkfOut->printf("rule lib\n  command = lib.exe /MACHINE:%s /LTCG /NOLOGO /OUT:$out $in\n",
				(m_gentype == GEN_DEBUG64 || m_gentype == GEN_RELEASE64) ? "x64" : "x86");
			m_pkfOut->WriteEol("  description = creating library $out\n");
		}
	}
	else if (m_Compiler == CSrcFiles::COMPILER_CLANG) {
		if (isExeTypeLib() || getLibName())	{
			// MSVC -LTCG option is not supported by lld
			m_pkfOut->printf("rule lib\n  command = lld-link.exe /lib /machine:%s /out:$out $in\n",
				(m_gentype == GEN_DEBUG64 || m_gentype == GEN_RELEASE64) ? "x64" : "x86");
			m_pkfOut->WriteEol("  description = creating library $out\n");
		}
	}
}

void CNinja::WriteRcDirective()
{
	if (tt::FileExists(getRcFile())) {
// REVIEW: [randalphwa - 10/12/2018] Currently the llvm-rc has a LOT of trouble reading rc files generated by Visual Studio
//		if (m_Compiler == CSrcFiles::COMPILER_CLANG)
//			m_pkfOut->WriteEol("rule rc\n  command = llvm-rc.exe -nologo -r /l 0x409 -fo$resout $in");
//		else
			m_pkfOut->WriteEol("rule rc\n  command = rc.exe -nologo -r /l 0x409 -fo$out $in");
		m_pkfOut->WriteEol("  description = resource compiler... $in\n");
	}
}

void CNinja::WriteMidlDirective(GEN_TYPE gentype)
{
	if (m_lstIdlFiles.GetCount()) {
		if (gentype == GEN_DEBUG64 || gentype == GEN_RELEASE64)
			m_pkfOut->WriteEol("rule midl\n  command = midl.exe /nologo /x64 /tlb $out $in");
		else
			m_pkfOut->WriteEol("rule midl\n  command = midl.exe /nologo /win32 /tlb $out $in");
		m_pkfOut->WriteEol("  description = midl compiler... $in\n");
	}
}

void CNinja::WriteMidlTargets()
{
	for (size_t pos = 0; pos < m_lstIdlFiles.GetCount(); ++pos) {
		ttString cszTypeLib(m_lstIdlFiles[pos]);
		cszTypeLib.ChangeExtension(".tlb");
		ttString cszIdlC(m_lstIdlFiles[pos]);

		// By default, midl will generate a file_i.c file which gets #included in one of the source files. The C++
		// compilers should generate a dependency for it, so we use that file as the target rather then the .tlb file so
		// that ninja will know to recompile the file that included the file_i.c file.

		cszIdlC.RemoveExtension();
		cszIdlC += "_i.c";
		if (m_cszMidlFlags.isnonempty())
			m_pkfOut->printf("build %s : midl %s /tlb %s %s\n\n",
				(char*) cszIdlC, (char*) m_cszMidlFlags, (char*) cszTypeLib, m_lstIdlFiles[pos]);
		else
			m_pkfOut->printf("build %s : midl /tlb %s %s\n\n",
				(char*) cszIdlC, (char*) cszTypeLib, m_lstIdlFiles[pos]);
	}
}

void CNinja::WriteLinkTargets(GEN_TYPE gentype)
{
	// Note that bin and lib don't need to exist ahead of time as ninja will create them, however if the output is
	// supposed to be up one directory (../bin, ../lib) then the directories MUST exist ahead of time. Only way around
	// this would be to add support for an "OutPrefix: ../" option in .srcfiles.

	const char* pszTarget;
	switch (gentype) {
		case GEN_DEBUG:
			pszTarget = GetTargetDebug();
			break;
		case GEN_DEBUG64:
			pszTarget = GetTargetDebug64();
			break;
		case GEN_RELEASE:
			pszTarget = GetTargetRelease();
			break;
		case GEN_RELEASE64:
		default:
			pszTarget = GetTargetRelease64();
			break;
	}

	if (isExeTypeLib())	{
		m_pkfOut->printf("build %s : lib", pszTarget);
	}
	else {
		m_pkfOut->printf("build %s : link", pszTarget);
	}

	if (tt::FileExists(getRcFile())) {
		ttString cszRes(getRcFile());
		cszRes.ChangeExtension(".res");
		m_pkfOut->printf(" $resout/%s", (char*) cszRes);
	}

	if (!isExeTypeLib() && getLibName()) {
		ttString cszExtraLib("$libout/");
		cszExtraLib += tt::FindFilePortion(getLibName());
		cszExtraLib.ChangeExtension(".lib");

		m_pkfOut->WriteChar(CH_SPACE);
		m_pkfOut->WriteStr(cszExtraLib);
	}

	if (getBuildLibs()) {
		ttEnumStr enumLib(tt::nextnonspace(getBuildLibs()), ';');
		while (enumLib.Enum())
			AddDependentLibrary(enumLib, gentype);
	}

	for (size_t iPos = 0; iPos < getSrcCount(); iPos++) {
		ttString cszFile(tt::FindFilePortion(getSrcFileList()->GetAt(iPos)));
		if (!tt::findstri(cszFile, ".c"))	// we don't care about any type of file that wasn't compiled into an .obj file
			continue;
		cszFile.ChangeExtension(".obj");
		m_pkfOut->printf(" $\n  $outdir/%s", (char*) cszFile);
	}
	m_pkfOut->WriteEol("\n");
}

void CNinja::AddDependentLibrary(const char* pszLib, GEN_TYPE gentype)
{
	ttString cszLib(pszLib);
	char* pszTmp = tt::findstri(pszLib, ".lib");
	if (pszTmp)
		*pszTmp = 0;

	// Note that if the path to the library contains a "64" then the "64" suffix is not added. I.e., ../ttLib/lib64/ttLib will turn
	// into ttLibD.lib not ttLib64D.lib. Otherwise we always add the "64" to the 64-bit builds.

	switch (gentype) {
		case GEN_DEBUG:
			cszLib += "D.lib";
			break;
		case GEN_DEBUG64:
			cszLib += !tt::findstr(cszLib, "64") ? "64D.lib" : "D.lib";
			break;
		case GEN_RELEASE:
			cszLib += ".lib";
			break;
		case GEN_RELEASE64:
		default:
			cszLib += !tt::findstr(cszLib, "64") ? "64.lib" : ".lib";
			break;
	}

#if 1
	m_pkfOut->WriteChar(CH_SPACE);
	m_pkfOut->WriteStr(cszLib);
#else
	// [randalphwa - 12/5/2018] The problem with this code is that it can mess up a valid library name if the library
	// doesn't exist when MakeNinja is being run. That can result in "foo64D.lib" being turned into "foo.lib" which is
	// probably the wrong platform version.

	if (FileExists(cszLib))	{
		m_pkfOut->WriteChar(CH_SPACE);
		m_pkfOut->WriteStr(cszLib);
	}
	else {
		cszLib = pszLib;
		cszLib.ChangeExtension(".lib");
		m_pkfOut->WriteChar(CH_SPACE);
		m_pkfOut->WriteStr(cszLib);
	}
#endif
}
