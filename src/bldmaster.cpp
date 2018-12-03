/////////////////////////////////////////////////////////////////////////////
// Name:		CBldMaster
// Purpose:		Class for
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2018 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#include "../ttLib/include/cstr.h"	// CStr

#include "bldmaster.h"				// CBldMaster

CBldMaster::CBldMaster(bool bReadPrivate) : CSrcFiles()
{
	ReadTwoFiles(".srcfiles", bReadPrivate ? ".private/.srcfiles" : nullptr);

	// if they are both empty, fill them in with defaults. If only one is empty, then it's likely only a single platform
	// is being build (32-bit or 64-bit)

	if (m_cszTarget32.IsEmpty() && m_cszTarget64.IsEmpty()) {
		if (m_exeType == EXE_LIB) {
			if (DirExists("../lib")) {
				m_cszTarget32 = "../lib";
				if (DirExists("../lib64"))
					m_cszTarget64 = "../lib64";
			}
			else {
				m_cszTarget32 = "lib";
				if (DirExists("lib64"))
					m_cszTarget64 = "lib64";
			}
		}

		else {
			if (DirExists("../bin")) {
				m_cszTarget32 = "../bin";
				if (DirExists("../bin64"))
					m_cszTarget64 = "../bin64";
			}
			else {
				m_cszTarget32 = "bin";
				if (DirExists("bin64"))
					m_cszTarget64 = "bin64";
			}
		}
	}

	if (!getProjName()) {
		CStr cszCwd;
		cszCwd.GetCWD();
		char* pszTmp = (char*) cszCwd.FindLastSlash();
		if (!pszTmp[1])		// if path ends with a slash, remove it -- we need that last directory name
			*pszTmp = 0;

		char* pszProj = FindFilePortion(cszCwd);
		if (IsSameString(pszProj, "src")) {	// Use the parent folder for the root if the current directory is "src"
			pszTmp = (char*) cszCwd.FindLastSlash();
			if (pszTmp)	{
				*pszTmp = 0;	// remove the last slash and filename, forcing the directory name above to be the "filename"
				pszProj = FindFilePortion(cszCwd);
			}
		}
		setProjName(pszProj);
	}

	m_lstRcDependencies.SetFlags(CStrList::FLG_URL_STRINGS);

	if (m_cszRcName.IsNonEmpty())
		FindRcDependencies(m_cszRcName);

	m_bBin64Exists = (m_cszTarget64.IsNonEmpty() && kstrstr(m_cszTarget64, "64"));
}

const char* lstRcKeywords[] = {		// list of keywords that load a file
	"BITMP",
	"CURSOR",
	"FONT",
	"HTML",
	"ICON",
	"RCDATA",
	"TYPELIB",
	"MESSAGETABLE",

	nullptr
};

// This is called to parse .rc files to find dependencies

bool CBldMaster::FindRcDependencies(const char* pszRcFile, const char* pszHdr, const char* pszRelPath)
{
	CKeyFile kf;
	if (!kf.ReadFile(pszHdr ? pszHdr : pszRcFile)) {
		if (!pszHdr) {	// we should have already reported a problem with a missing header file
			CStr cszErrMsg;
			cszErrMsg.printf("Cannot open %s", pszRcFile);
			m_lstErrors += cszErrMsg;
		}
		return false;
	}

	// If passed a path, use that, otherwise see if the SrcFile contained a path, and if so, use that path.
	// If non-empty, the location of any header file is considered relative to the cszRelPath location

	CStr cszRelPath;
	if (pszRelPath)
		cszRelPath = pszRelPath;
	else {
		if (pszHdr)	{
			char* pszFilePortion = FindFilePortion(pszHdr);
			if (pszFilePortion != pszHdr) {
				cszRelPath = pszHdr;
				pszFilePortion = FindFilePortion(cszRelPath);
				*pszFilePortion = 0;
			}
		}
		else {
			char* pszFilePortion = FindFilePortion(pszRcFile);
			if (pszFilePortion != pszRcFile) {
				cszRelPath = pszRcFile;
				pszFilePortion = FindFilePortion(cszRelPath);
				*pszFilePortion = 0;
			}
		}
	}

	size_t curLine = 0;
	while (kf.readline()) {
		++curLine;
		if (IsSameSubString(FindNonSpace(kf), "#include")) {
			char* psz = FindNonSpace(FindNonSpace(kf) + sizeof("#include"));

			// We only care about header files in quotes -- we're not generating dependeices on system files (#include <foo.h>)

			if (*psz == CH_QUOTE) {
				CStr cszHeader;
				cszHeader.GetQuotedString(psz);

				// Older versions of Visual Studio do not allow <> to be placed around header files. Since system header files
				// rarely change, and when they do they are not likely to require rebuilding our .rc file, we simply ignore them.

				if (IsSameSubString(cszHeader, "afx") || IsSameSubString(cszHeader, "atl") || IsSameSubString(cszHeader, "winres"))
					continue;

				NormalizeHeader(pszHdr ? pszHdr : pszRcFile, cszHeader);

				if (!FileExists(cszHeader)) {
					CStr cszErrMsg;
					cszErrMsg.printf("%s(%kt,%kt):  warning: cannot locate include file %s",
						pszHdr ? pszHdr : pszRcFile, curLine, (size_t) (psz - kf.GetLnPtr()),  (char*) cszHeader);
					m_lstErrors += cszErrMsg;
					continue;
				}

				size_t posHdr = m_lstRcDependencies.GetPos(cszHeader);
				bool bHdrSeenBefore = (posHdr != (size_t) -1);
				if (!bHdrSeenBefore)
					posHdr = m_lstRcDependencies.Add(cszHeader);

				if (!bHdrSeenBefore)
					FindRcDependencies(pszRcFile, cszHeader, cszRelPath);		// now search the header file for any #includes it might have
			}
		}

		// Not a header file, but might still be something we are dependent on

		else {
			char* pszKeyWord = FindNonSpace(kf);
			if (!pszKeyWord || pszKeyWord[0] == '/' || pszKeyWord[0] == CH_QUOTE)	// TEXTINCLUDE typically puts things in quotes
				continue;	// blank line or comment line
			pszKeyWord = FindNextSpace(pszKeyWord);
			if (!pszKeyWord)
				continue;	// means it's not a line that will include anything
			pszKeyWord = FindNonSpace(pszKeyWord);
			if (!pszKeyWord)
				continue;	// means it's not a line that will include anything

			for (size_t pos = 0; lstRcKeywords[pos] ; ++pos) {
				if (IsCSSameSubString(pszKeyWord, lstRcKeywords[pos])) {
					const char* pszFileName = kstrchr(pszKeyWord, CH_QUOTE);

					// Some keywords use quotes which aren't actually filenames -- e.g., RCDATA { "string" }

					if (pszFileName && kstrchr(pszFileName + 1, CH_QUOTE) && !kstrchr(pszFileName, '{')) {
						CStr cszFile;
						cszFile.GetQuotedString(pszFileName);

						// Backslashes are doubled -- so convert them into forward slashes

						char* pszSlash = kstrstr(cszFile, "\\\\");
						if (pszSlash) {
							do {
								*pszSlash++ = '/';
								kstrcpy(pszSlash, pszSlash + 1);
								pszSlash = kstrstr(pszSlash, "\\\\");
							} while(pszSlash);
						}

						if (pszHdr) {
							CStr cszHdr(pszHdr);
							if (FileExists(cszHdr)) {	// we only want the directory
								char* pszFilePortion = FindFilePortion(cszHdr);
								*pszFilePortion = 0;
							}
							// First we normalize it to the header
							NormalizeHeader(pszHdr, cszFile);
							cszHdr.AppendFileName(cszFile);
							// Then we normalize it to our RC file
							NormalizeHeader(pszRcFile, cszHdr);
							cszFile = cszHdr;
						}
						else
							NormalizeHeader(pszRcFile, cszFile);

						if (!FileExists(cszFile)) {
							CStr cszErrMsg;
							cszErrMsg.printf("%s(%kt,%kt):  warning: cannot locate include file %s",
								pszHdr ? pszHdr : pszRcFile, curLine, (size_t) (pszFileName - kf.GetLnPtr()),  (char*) cszFile);
							m_lstErrors += cszErrMsg;
							break;
						}
						size_t posHdr = m_lstRcDependencies.GetPos(cszFile);
						bool bHdrSeenBefore = (posHdr != (size_t) -1);
						if (!bHdrSeenBefore)
							posHdr = m_lstRcDependencies.Add(cszFile);
					}
					break;
				}
			}
		}
	}
	return true;
}

// We need all header files to use the same path for comparison purposes

const char* CBldMaster::NormalizeHeader(const char* pszRoot, CStr& cszHeader)
{
	ASSERT(cszHeader.IsNonEmpty());

	if (pszRoot && *pszRoot)
		ConvertToRelative(pszRoot, cszHeader, cszHeader);

	cszHeader.MakeLower();
	return cszHeader;
}

const char* CBldMaster::GetTargetDebug()
{
	if (cszTargetDebug.IsNonEmpty())
		return cszTargetDebug;

	if (isExeTypeLib())	{
		if (m_cszTarget32.IsNonEmpty())
			cszTargetDebug = m_cszTarget32;
		else
			cszTargetDebug = DirExists("../lib") ? "../lib" : "lib";
		cszTargetDebug.AppendFileName(getProjName());
		cszTargetDebug += "D.lib";
		return cszTargetDebug;
	}

	else if (isExeTypeDll()) {
		if (m_cszTarget32.IsNonEmpty())
			cszTargetDebug = m_cszTarget32;
		else
			cszTargetDebug = DirExists("../bin") ? "../bin" : "bin";
		cszTargetDebug.AppendFileName(getProjName());
		cszTargetDebug += ".dll";	// Note that we do NOT add a 'D' to the end of the dll name
		return cszTargetDebug;
	}
	else {
		if (m_cszTarget32.IsNonEmpty())
			cszTargetDebug = m_cszTarget32;
		else
			cszTargetDebug = DirExists("../bin") ? "../bin" : "bin";
		cszTargetDebug.AppendFileName(getProjName());
		cszTargetDebug += "D.exe";
		return cszTargetDebug;
	}
}

const char* CBldMaster::GetTargetRelease()
{
	if (cszTargetRelease.IsNonEmpty())
		return cszTargetRelease;

	if (isExeTypeLib())	{
		if (m_cszTarget32.IsNonEmpty())
			cszTargetRelease = m_cszTarget32;
		else
			cszTargetRelease = DirExists("../lib") ? "../lib" : "lib";
		cszTargetRelease.AppendFileName(getProjName());
		cszTargetRelease += ".lib";
		return cszTargetRelease;
	}

	else if (isExeTypeDll()) {
		if (m_cszTarget32.IsNonEmpty())
			cszTargetRelease = m_cszTarget32;
		else
			cszTargetRelease = DirExists("../bin") ? "../bin" : "bin";
		cszTargetRelease.AppendFileName(getProjName());
		cszTargetRelease += "dll";
		return cszTargetRelease;
	}
	else {
		if (m_cszTarget32.IsNonEmpty())
			cszTargetRelease = m_cszTarget32;
		else
			cszTargetRelease = DirExists("../bin") ? "../bin" : "bin";
		cszTargetRelease.AppendFileName(getProjName());
		cszTargetRelease += ".exe";
		return cszTargetRelease;
	}
}

const char* CBldMaster::GetTargetDebug64()
{
	if (cszTargetDebug64.IsNonEmpty())
		return cszTargetDebug64;

	if (isExeTypeLib())	{
		if (m_cszTarget64.IsNonEmpty())
			cszTargetDebug64 = m_cszTarget64;
		else
			cszTargetDebug64 = DirExists("../lib") ? "../lib" : "lib";
		cszTargetDebug64.AppendFileName(getProjName());
		cszTargetDebug64 += is64BitSuffix() ? "64D.lib" : "D.lib";
		return cszTargetDebug64;
	}

	else if (isExeTypeDll()) {
		if (m_cszTarget64.IsNonEmpty())
			cszTargetDebug64 = m_cszTarget64;
		else
			cszTargetDebug64 = isBin64() ? "../bin64" : (DirExists("../bin") ? "../bin" : "bin");
		cszTargetDebug64.AppendFileName(getProjName());
		cszTargetDebug64 = (isBin64() || !is64BitSuffix()) ? ".dll" : "64.dll";
		return cszTargetDebug64;
	}
	else {
		if (m_cszTarget64.IsNonEmpty())
			cszTargetDebug64 = m_cszTarget64;
		else
			cszTargetDebug64 = isBin64() ? "../bin64" : (DirExists("../bin") ? "../bin" : "bin");
		cszTargetDebug64.AppendFileName(getProjName());
		cszTargetDebug64 += (isBin64() || !is64BitSuffix()) ? "D.exe" : "64D.exe";
		return cszTargetDebug64;
	}
}

const char* CBldMaster::GetTargetRelease64()
{
	if (cszTargetRelease64.IsNonEmpty())
		return cszTargetRelease64;

	if (isExeTypeLib())	{
		if (m_cszTarget64.IsNonEmpty())
			cszTargetRelease64 = m_cszTarget64;
		else
			cszTargetRelease64 = DirExists("../lib") ? "../lib" : "lib";
		cszTargetRelease64.AppendFileName(getProjName());
		cszTargetRelease64 += is64BitSuffix() ? "64.lib" : ".lib";
		return cszTargetRelease64;
	}

	else if (isExeTypeDll()) {
		if (m_cszTarget64.IsNonEmpty())
			cszTargetRelease64 = m_cszTarget64;
		else
			cszTargetRelease64 = isBin64() ? "../bin64" : (DirExists("../bin") ? "../bin" : "bin");
		cszTargetRelease64.AppendFileName(getProjName());
		cszTargetRelease64 = (isBin64() || !is64BitSuffix()) ? ".dll" : "64.dll";
		return cszTargetRelease64;
	}
	else {
		if (m_cszTarget64.IsNonEmpty())
			cszTargetRelease64 = m_cszTarget64;
		else
			cszTargetRelease64 = isBin64() ? "../bin64" : (DirExists("../bin") ? "../bin" : "bin");
		cszTargetRelease64.AppendFileName(getProjName());
		cszTargetRelease64 += (isBin64() || !is64BitSuffix()) ? ".exe" : "64.exe";;
		return cszTargetRelease64;
	}
}
