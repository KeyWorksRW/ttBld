/////////////////////////////////////////////////////////////////////////////
// Name:		CNinja
// Purpose:		Class for creating/maintaining build.ninja file for use by ninja.exe build tool
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2018 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "bldmaster.h"	// CBldMaster

class CNinja : public CBldMaster
{
public:
	CNinja() : CBldMaster(true) {}

	typedef enum {
		GEN_NONE,
		GEN_DEBUG,
		GEN_RELEASE,
		GEN_DEBUG64,
		GEN_RELEASE64
	} GEN_TYPE;

	bool CreateBuildFile(GEN_TYPE gentype, size_t Compiler = CSrcFiles::COMPILER_MSVC);

protected:
	// Class functions

	void AddDependentLibrary(const char* pszLib, GEN_TYPE gentype);

	void WriteCompilerComments();
	void WriteCompilerDirectives();
	void WriteCompilerFlags();
	void WriteLibDirective();
	void WriteLinkDirective();
	void WriteLinkTargets(GEN_TYPE gentype);
	void WriteMidlDirective();
	void WriteRcDirective();

	// Class members

	// The members below are reset every time CreateBuildFile() is called

	CKeyFile* m_pkfOut;
	GEN_TYPE m_gentype;
	size_t	 m_Compiler;

	CStr m_cszPCH;			// the .pch name that will be generated
	CStr m_cszCPP_PCH;		// the .cpp name that will be used to create the .pch file
	CStr m_cszPCHObj;		// the .obj file that is built to create the .pch file
};
