/////////////////////////////////////////////////////////////////////////////
// Name:		CBldMaster
// Purpose:		Class for
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttstr.h>					// ttCStr

#include "bldmaster.h"				// CBldMaster
#include "../common/strtable.h" 	// String resource IDs

CBldMaster::CBldMaster(bool bReadPrivate) : CSrcFiles()
{
	ReadTwoFiles(".srcfiles", bReadPrivate ? ".private/.srcfiles" : nullptr);

	// if they are both empty, fill them in with defaults. If only one is empty, then it's likely only a single platform
	// is being build (32-bit or 64-bit)

	if (!getDir32() && !getDir64()) {
		if (isExeTypeLib()) {
			if (tt::DirExists("../lib")) {
				UpdateOption(OPT_TARGET_DIR32, "../lib");
				if (tt::DirExists("../lib64"))
					UpdateOption(OPT_TARGET_DIR64, "../lib64");
			}
			else {
				UpdateOption(OPT_TARGET_DIR32, "lib");
				if (tt::DirExists("lib64"))
					UpdateOption(OPT_TARGET_DIR64, "lib64");
			}
		}

		else {
			if (tt::DirExists("../bin")) {
				UpdateOption(OPT_TARGET_DIR32, "../bin");
				if (tt::DirExists("../bin64"))
					UpdateOption(OPT_TARGET_DIR64, "../bin64");
			}
			else {
				UpdateOption(OPT_TARGET_DIR32, "bin");
				if (tt::DirExists("bin64"))
					UpdateOption(OPT_TARGET_DIR64, "bin64");
			}
		}
	}

	if (!GetProjectName()) {
		ttCStr cszCwd;
		cszCwd.getCWD();
		char* pszTmp = (char*) cszCwd.findLastSlash();
		if (!pszTmp[1])		// if path ends with a slash, remove it -- we need that last directory name
			*pszTmp = 0;

		char* pszProj = tt::findFilePortion(cszCwd);
		if (tt::isSameStri(pszProj, "src")) {	// Use the parent folder for the root if the current directory is "src"
			pszTmp = (char*) cszCwd.findLastSlash();
			if (pszTmp)	{
				*pszTmp = 0;	// remove the last slash and filename, forcing the directory name above to be the "filename"
				pszProj = tt::findFilePortion(cszCwd);
			}
		}
		UpdateOption(OPT_PROJECT, pszProj);
	}

	m_lstRcDependencies.SetFlags(ttCList::FLG_URL_STRINGS);

	if (m_cszRcName.isNonEmpty())
		FindRcDependencies(m_cszRcName);

	m_bBin64Exists = tt::findStr(getDir64(), "64");
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
	ttCFile kf;
	if (!kf.ReadFile(pszHdr ? pszHdr : pszRcFile)) {
		if (!pszHdr) {	// we should have already reported a problem with a missing header file
			ttCStr cszErrMsg;
			cszErrMsg.printf(tt::getResString(IDS_CANNOT_OPEN), pszRcFile);
			m_lstErrors += cszErrMsg;
		}
		return false;
	}

	// If passed a path, use that, otherwise see if the SrcFile contained a path, and if so, use that path.
	// If non-empty, the location of any header file is considered relative to the cszRelPath location

	ttCStr cszRelPath;
	if (pszRelPath)
		cszRelPath = pszRelPath;
	else {
		if (pszHdr)	{
			char* pszFilePortion = tt::findFilePortion(pszHdr);
			if (pszFilePortion != pszHdr) {
				cszRelPath = pszHdr;
				pszFilePortion = tt::findFilePortion(cszRelPath);
				*pszFilePortion = 0;
			}
		}
		else {
			char* pszFilePortion = tt::findFilePortion(pszRcFile);
			if (pszFilePortion != pszRcFile) {
				cszRelPath = pszRcFile;
				pszFilePortion = tt::findFilePortion(cszRelPath);
				*pszFilePortion = 0;
			}
		}
	}

	size_t curLine = 0;
	while (kf.ReadLine()) {
		++curLine;
		if (tt::isSameSubStri(tt::findNonSpace(kf), "#include")) {
			char* psz = tt::findNonSpace(tt::findNonSpace(kf) + sizeof("#include"));

			// We only care about header files in quotes -- we're not generating dependeices on system files (#include <foo.h>)

			if (*psz == CH_QUOTE) {
				ttCStr cszHeader;
				cszHeader.getQuotedString(psz);

				// Older versions of Visual Studio do not allow <> to be placed around header files. Since system header files
				// rarely change, and when they do they are not likely to require rebuilding our .rc file, we simply ignore them.

				if (tt::isSameSubStri(cszHeader, "afx") || tt::isSameSubStri(cszHeader, "atl") || tt::isSameSubStri(cszHeader, "winres"))
					continue;

				NormalizeHeader(pszHdr ? pszHdr : pszRcFile, cszHeader);

				if (!tt::FileExists(cszHeader)) {
					ttCStr cszErrMsg;
					cszErrMsg.printf(tt::getResString(IDS_MISSING_INCLUDE),
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
			char* pszKeyWord = tt::findNonSpace(kf);
			if (!pszKeyWord || pszKeyWord[0] == '/' || pszKeyWord[0] == CH_QUOTE)	// TEXTINCLUDE typically puts things in quotes
				continue;	// blank line or comment line
			pszKeyWord = tt::findSpace(pszKeyWord);
			if (!pszKeyWord)
				continue;	// means it's not a line that will include anything
			pszKeyWord = tt::findNonSpace(pszKeyWord);
			if (!pszKeyWord)
				continue;	// means it's not a line that will include anything

			for (size_t pos = 0; lstRcKeywords[pos] ; ++pos) {
				if (tt::isSameSubStr(pszKeyWord, lstRcKeywords[pos])) {
					const char* pszFileName = tt::findChar(pszKeyWord, CH_QUOTE);

					// Some keywords use quotes which aren't actually filenames -- e.g., RCDATA { "string" }

					if (pszFileName && tt::findChar(pszFileName + 1, CH_QUOTE) && !tt::findChar(pszFileName, '{')) {
						ttCStr cszFile;
						cszFile.getQuotedString(pszFileName);

						// Backslashes are doubled -- so convert them into forward slashes

						char* pszSlash = tt::findStr(cszFile, "\\\\");
						if (pszSlash) {
							do {
								*pszSlash++ = '/';
								tt::strCopy(pszSlash, pszSlash + 1);
								pszSlash = tt::findStr(pszSlash, "\\\\");
							} while(pszSlash);
						}

						if (pszHdr) {
							ttCStr cszHdr(pszHdr);
							if (tt::FileExists(cszHdr)) {	// we only want the directory
								char* pszFilePortion = tt::findFilePortion(cszHdr);
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

						if (!tt::FileExists(cszFile)) {
							ttCStr cszErrMsg;
							cszErrMsg.printf(tt::getResString(IDS_MISSING_INCLUDE),
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

const char* CBldMaster::NormalizeHeader(const char* pszRoot, ttCStr& cszHeader)
{
	ttASSERT(cszHeader.isNonEmpty());

	if (pszRoot && *pszRoot)
		tt::ConvertToRelative(pszRoot, cszHeader, cszHeader);

	cszHeader.MakeLower();
	return cszHeader;
}

const char* CBldMaster::GetTargetDebug()
{
	if (cszTargetDebug.isNonEmpty())
		return cszTargetDebug;

	if (isExeTypeLib())	{
		if (getDir32())
			cszTargetDebug = getDir32();
		else
			cszTargetDebug = tt::DirExists("../lib") ? "../lib" : "lib";
		cszTargetDebug.AppendFileName(GetProjectName());
		cszTargetDebug += "D.lib";
		return cszTargetDebug;
	}

	else if (isExeTypeDll()) {
		if (getDir32())
			cszTargetDebug = getDir32();
		else
			cszTargetDebug = tt::DirExists("../bin") ? "../bin" : "bin";
		cszTargetDebug.AppendFileName(GetProjectName());
		cszTargetDebug += ".dll";	// Note that we do NOT add a 'D' to the end of the dll name
		return cszTargetDebug;
	}
	else {
		if (getDir32())
			cszTargetDebug = getDir32();
		else
			cszTargetDebug = tt::DirExists("../bin") ? "../bin" : "bin";
		cszTargetDebug.AppendFileName(GetProjectName());
		cszTargetDebug += "D.exe";
		return cszTargetDebug;
	}
}

const char* CBldMaster::GetTargetRelease()
{
	if (cszTargetRelease.isNonEmpty())
		return cszTargetRelease;

	if (isExeTypeLib())	{
		if (getDir32())
			cszTargetRelease = getDir32();
		else
			cszTargetRelease = tt::DirExists("../lib") ? "../lib" : "lib";
		cszTargetRelease.AppendFileName(GetProjectName());
		cszTargetRelease += ".lib";
		return cszTargetRelease;
	}

	else if (isExeTypeDll()) {
		if (getDir32())
			cszTargetRelease = getDir32();
		else
			cszTargetRelease = tt::DirExists("../bin") ? "../bin" : "bin";
		cszTargetRelease.AppendFileName(GetProjectName());
		cszTargetRelease += ".dll";
		return cszTargetRelease;
	}
	else {
		if (getDir32())
			cszTargetRelease = getDir32();
		else
			cszTargetRelease = tt::DirExists("../bin") ? "../bin" : "bin";
		cszTargetRelease.AppendFileName(GetProjectName());
		cszTargetRelease += ".exe";
		return cszTargetRelease;
	}
}

const char* CBldMaster::GetTargetDebug64()
{
	if (cszTargetDebug64.isNonEmpty())
		return cszTargetDebug64;

	if (isExeTypeLib())	{
		if (getDir32())
			cszTargetDebug64 = getDir32();
		else
			cszTargetDebug64 = tt::DirExists("../lib") ? "../lib" : "lib";
		cszTargetDebug64.AppendFileName(GetProjectName());
		cszTargetDebug64 += GetBoolOption(OPT_BIT_SUFFIX) ? "64D.lib" : "D.lib";
		return cszTargetDebug64;
	}

	else if (isExeTypeDll()) {
		if (getDir64())
			cszTargetDebug64 = getDir64();
		else
			cszTargetDebug64 = isBin64() ? "../bin64" : (tt::DirExists("../bin") ? "../bin" : "bin");
		cszTargetDebug64.AppendFileName(GetProjectName());
		cszTargetDebug64 = (isBin64() || !GetBoolOption(OPT_BIT_SUFFIX)) ? ".dll" : "64.dll";
		return cszTargetDebug64;
	}
	else {
		if (getDir64())
			cszTargetDebug64 = getDir64();
		else
			cszTargetDebug64 = isBin64() ? "../bin64" : (tt::DirExists("../bin") ? "../bin" : "bin");
		cszTargetDebug64.AppendFileName(GetProjectName());
		cszTargetDebug64 += (isBin64() || !GetBoolOption(OPT_BIT_SUFFIX)) ? "D.exe" : "64D.exe";
		return cszTargetDebug64;
	}
}

const char* CBldMaster::GetTargetRelease64()
{
	if (cszTargetRelease64.isNonEmpty())
		return cszTargetRelease64;

	if (isExeTypeLib())	{
		if (getDir64())
			cszTargetRelease64 = getDir64();
		else
			cszTargetRelease64 = tt::DirExists("../lib") ? "../lib" : "lib";
		cszTargetRelease64.AppendFileName(GetProjectName());
		cszTargetRelease64 += GetBoolOption(OPT_BIT_SUFFIX) ? "64.lib" : ".lib";
		return cszTargetRelease64;
	}

	else if (isExeTypeDll()) {
		if (getDir64())
			cszTargetRelease64 = getDir64();
		else
			cszTargetRelease64 = isBin64() ? "../bin64" : (tt::DirExists("../bin") ? "../bin" : "bin");
		cszTargetRelease64.AppendFileName(GetProjectName());
		cszTargetRelease64 = (isBin64() || !GetBoolOption(OPT_BIT_SUFFIX)) ? ".dll" : "64.dll";
		return cszTargetRelease64;
	}
	else {
		if (getDir64())
			cszTargetRelease64 = getDir64();
		else
			cszTargetRelease64 = isBin64() ? "../bin64" : (tt::DirExists("../bin") ? "../bin" : "bin");
		cszTargetRelease64.AppendFileName(GetProjectName());
		cszTargetRelease64 += (isBin64() || !GetBoolOption(OPT_BIT_SUFFIX)) ? ".exe" : "64.exe";;
		return cszTargetRelease64;
	}
}
