/////////////////////////////////////////////////////////////////////////////
// Name:		CBldMaster
// Purpose:		Class for
// Author:		Ralph Walden
// Copyright:	Copyright (c) 2002-2019 KeyWorks Software (Ralph Walden)
// License:		Apache License (see ../LICENSE)
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <ttstr.h>		// ttCStr

#include "bldmaster.h"	// CBldMaster
#include "strtable.h" 	// String resource IDs

CBldMaster::CBldMaster(bool bReadPrivate) : CSrcFiles()
{
	ReadTwoFiles(".srcfiles", bReadPrivate ? ".private/.srcfiles" : nullptr);

	// If no platform was specified, default to 64-bit

	if (!GetBoolOption(OPT_64BIT) && !GetBoolOption(OPT_32BIT)) {
		UpdateOption(OPT_64BIT, true);			// default to 64-bit build
		UpdateOption(OPT_64BIT_SUFFIX, false);	// no suffix
	}

	// Set default target directories if they are missing

	if (GetBoolOption(OPT_64BIT) && !GetDir64()) {
		ttCStr cszCWD, cszDir64;
		cszCWD.GetCWD();
		bool bSrcDir = ttstristr(tt::FindFilePortion(cszCWD), "src") ? true : false;
		if (!bSrcDir) {
			cszCWD.AppendFileName(IsExeTypeLib() ? "../lib" : "../bin");
			if (tt::DirExists(cszCWD))
				bSrcDir = true;
		}
		if (bSrcDir) {
			ttCStr cszDir64(IsExeTypeLib() ? "../lib" : "../bin");
			ttCStr cszTmp(cszDir64);
			cszTmp += "64";
			if (tt::DirExists(cszTmp))		// if there is a ../lib64 or ../bin64, then use that
				cszDir64 = cszTmp;
			UpdateOption(OPT_TARGET_DIR64, (char*) cszDir64);
		}
		else {
			ttCStr cszDir64(IsExeTypeLib() ? "lib" : "bin");
			ttCStr cszTmp(cszDir64);
			cszTmp += "64";
			if (tt::DirExists(cszTmp))		// if there is a ../lib64 or ../bin64, then use that
				cszDir64 = cszTmp;
			UpdateOption(OPT_TARGET_DIR64, (char*) cszDir64);
		}
	}

	if (GetBoolOption(OPT_32BIT) && !GetDir32()) {
		ttCStr cszCWD, cszDir32;
		cszCWD.GetCWD();
		bool bSrcDir = ttstristr(tt::FindFilePortion(cszCWD), "src") ? true : false;
		if (!bSrcDir) {
			cszCWD.AppendFileName(IsExeTypeLib() ? "../lib" : "../bin");
			if (tt::DirExists(cszCWD))
				bSrcDir = true;
		}
		if (bSrcDir) {
			ttCStr cszDir32(IsExeTypeLib() ? "../lib" : "../bin");
			ttCStr cszTmp(cszDir32);
			cszTmp += "32";
			if (tt::DirExists(cszTmp))		// if there is a ../lib32 or ../bin32, then use that
				cszDir32 = cszTmp;
			UpdateOption(OPT_TARGET_DIR32, (char*) cszDir32);
		}
		else {
			ttCStr cszDir32(IsExeTypeLib() ? "lib" : "bin");
			ttCStr cszTmp(cszDir32);
			cszTmp += "32";
			if (tt::DirExists(cszTmp))		// if there is a ../lib32 or ../bin32, then use that
				cszDir32 = cszTmp;
			UpdateOption(OPT_TARGET_DIR32, (char*) cszDir32);
		}
	}

	m_bAddPlatformSuffix = (GetBoolOption(OPT_64BIT) && GetBoolOption(OPT_32BIT) && tt::IsSameStr(GetDir64(), GetDir32())) ? true : false;

	if (!GetProjectName()) {
		ttCStr cszCwd;
		cszCwd.GetCWD();
		char* pszTmp = (char*) cszCwd.FindLastSlash();
		if (!pszTmp[1])		// if path ends with a slash, remove it -- we need that last directory name
			*pszTmp = 0;

		char* pszProj = tt::FindFilePortion(cszCwd);
		if (tt::IsSameStrI(pszProj, "src")) {	// Use the parent folder for the root if the current directory is "src"
			pszTmp = (char*) cszCwd.FindLastSlash();
			if (pszTmp)	{
				*pszTmp = 0;	// remove the last slash and filename, forcing the directory name above to be the "filename"
				pszProj = tt::FindFilePortion(cszCwd);
			}
		}
		UpdateOption(OPT_PROJECT, pszProj);
	}

	m_lstRcDependencies.SetFlags(ttCList::FLG_URL_STRINGS);

	if (m_cszRcName.IsNonEmpty())
		FindRcDependencies(m_cszRcName);

	m_bBin64Exists = tt::FindStr(GetDir64(), "64");
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
			cszErrMsg.printf(tt::GetResString(IDS_CS_CANNOT_OPEN), pszRcFile);
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
			char* pszFilePortion = tt::FindFilePortion(pszHdr);
			if (pszFilePortion != pszHdr) {
				cszRelPath = pszHdr;
				pszFilePortion = tt::FindFilePortion(cszRelPath);
				*pszFilePortion = 0;
			}
		}
		else {
			char* pszFilePortion = tt::FindFilePortion(pszRcFile);
			if (pszFilePortion != pszRcFile) {
				cszRelPath = pszRcFile;
				pszFilePortion = tt::FindFilePortion(cszRelPath);
				*pszFilePortion = 0;
			}
		}
	}

	size_t curLine = 0;
	while (kf.ReadLine()) {
		++curLine;
		if (tt::IsSameSubStrI(tt::FindNonSpace(kf), "#include")) {
			char* psz = tt::FindNonSpace(tt::FindNonSpace(kf) + sizeof("#include"));

			// We only care about header files in quotes -- we're not generating dependeices on system files (#include <foo.h>)

			if (*psz == CH_QUOTE) {
				ttCStr cszHeader;
				cszHeader.GetQuotedString(psz);

				// Older versions of Visual Studio do not allow <> to be placed around header files. Since system header files
				// rarely change, and when they do they are not likely to require rebuilding our .rc file, we simply ignore them.

				if (tt::IsSameSubStrI(cszHeader, "afx") || tt::IsSameSubStrI(cszHeader, "atl") || tt::IsSameSubStrI(cszHeader, "winres"))
					continue;

				NormalizeHeader(pszHdr ? pszHdr : pszRcFile, cszHeader);

				if (!tt::FileExists(cszHeader)) {
					ttCStr cszErrMsg;
					cszErrMsg.printf(tt::GetResString(IDS_CS_MISSING_INCLUDE),
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
			char* pszKeyWord = tt::FindNonSpace(kf);
			if (!pszKeyWord || pszKeyWord[0] == '/' || pszKeyWord[0] == CH_QUOTE)	// TEXTINCLUDE typically puts things in quotes
				continue;	// blank line or comment line
			pszKeyWord = tt::FindSpace(pszKeyWord);
			if (!pszKeyWord)
				continue;	// means it's not a line that will include anything
			pszKeyWord = tt::FindNonSpace(pszKeyWord);
			if (!pszKeyWord)
				continue;	// means it's not a line that will include anything

			for (size_t pos = 0; lstRcKeywords[pos] ; ++pos) {
				if (tt::IsSameSubStr(pszKeyWord, lstRcKeywords[pos])) {
					const char* pszFileName = ttstrchr(pszKeyWord, CH_QUOTE);

					// Some keywords use quotes which aren't actually filenames -- e.g., RCDATA { "string" }

					if (pszFileName && ttstrchr(pszFileName + 1, CH_QUOTE) && !ttstrchr(pszFileName, '{')) {
						ttCStr cszFile;
						cszFile.GetQuotedString(pszFileName);

						// Backslashes are doubled -- so convert them into forward slashes

						char* pszSlash = tt::FindStr(cszFile, "\\\\");
						if (pszSlash) {
							do {
								*pszSlash++ = '/';
								ttstrcpy(pszSlash, pszSlash + 1);
								pszSlash = ttstrstr(pszSlash, "\\\\");
							} while(pszSlash);
						}

						if (pszHdr) {
							ttCStr cszHdr(pszHdr);
							if (tt::FileExists(cszHdr)) {	// we only want the directory
								char* pszFilePortion = tt::FindFilePortion(cszHdr);
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
							cszErrMsg.printf(tt::GetResString(IDS_CS_MISSING_INCLUDE),
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
	ttASSERT(cszHeader.IsNonEmpty());

	if (pszRoot && *pszRoot)
		tt::ConvertToRelative(pszRoot, cszHeader, cszHeader);

	cszHeader.MakeLower();
	return cszHeader;
}

// Don't add the 'D' to the end of DLL's -- it is perfectly viable for a release app to use a debug dll and that won't
// work if the filename has changed. Under MSVC Linker, it will also generate a LNK4070 error if the dll name is
// specified in the matching .def file. The downside, of course, is that without the 'D' only the size of the dll
// indicates if it is a debug or release version.

const char* CBldMaster::GetTargetDebug32()
{
	if (m_cszTargetDebug32.IsNonEmpty())
		return m_cszTargetDebug32;

	ttASSERT_MSG(GetDir32(), "32-bit target must be set in constructor after reading .srcfiles");
	m_cszTargetDebug32 = GetDir32();

	if (IsExeTypeLib())	{
		m_cszTargetDebug32.AppendFileName(GetProjectName());
		m_cszTargetDebug32 += (m_bAddPlatformSuffix || GetBoolOption(OPT_32BIT_SUFFIX)) ? "32D.lib" : "D.lib";
	}
	else if (IsExeTypeDll()) {
		m_cszTargetDebug32.AppendFileName(GetProjectName());
		m_cszTargetDebug32 += (m_bAddPlatformSuffix || GetBoolOption(OPT_32BIT_SUFFIX)) ? "32.dll" : ".dll";
	}
	else {
		m_cszTargetDebug32.AppendFileName(GetProjectName());
		m_cszTargetDebug32 += (m_bAddPlatformSuffix || GetBoolOption(OPT_32BIT_SUFFIX)) ? "32D.exe" : "D.exe";
	}
	return m_cszTargetDebug32;
}

const char* CBldMaster::GetTargetRelease32()
{
	if (m_cszTargetRelease32.IsNonEmpty())
		return m_cszTargetRelease32;

	ttASSERT_MSG(GetDir32(), "32-bit target must be set in constructor after reading .srcfiles");
	m_cszTargetRelease32 = GetDir32();

	if (IsExeTypeLib())	{
		m_cszTargetRelease32.AppendFileName(GetProjectName());
		m_cszTargetRelease32 += (m_bAddPlatformSuffix || GetBoolOption(OPT_32BIT_SUFFIX)) ? "32.lib" : ".lib";
	}
	else if (IsExeTypeDll()) {
		m_cszTargetRelease32.AppendFileName(GetProjectName());
		m_cszTargetRelease32 += (m_bAddPlatformSuffix || GetBoolOption(OPT_32BIT_SUFFIX)) ? "32.dll" : ".dll";
	}
	else {
		m_cszTargetRelease32.AppendFileName(GetProjectName());
		m_cszTargetRelease32 += (m_bAddPlatformSuffix || GetBoolOption(OPT_32BIT_SUFFIX)) ? "32.exe" : ".exe";
	}
	return m_cszTargetRelease32;
}

const char* CBldMaster::GetTargetDebug64()
{
	if (m_cszTargetDebug64.IsNonEmpty())
		return m_cszTargetDebug64;

	ttASSERT_MSG(GetDir64(), "64-bit target must be set in constructor after reading .srcfiles");
	m_cszTargetDebug64 = GetDir64();

	if (IsExeTypeLib())	{
		m_cszTargetDebug64.AppendFileName(GetProjectName());
		m_cszTargetDebug64 += (m_bAddPlatformSuffix || GetBoolOption(OPT_64BIT_SUFFIX)) ? "64D.lib" : "D.lib";
	}
	else if (IsExeTypeDll()) {
		m_cszTargetDebug64.AppendFileName(GetProjectName());
		m_cszTargetDebug64 += (m_bAddPlatformSuffix || GetBoolOption(OPT_64BIT_SUFFIX)) ? "64.dll" : ".dll";
	}
	else {
		m_cszTargetDebug64.AppendFileName(GetProjectName());
		m_cszTargetDebug64 += (m_bAddPlatformSuffix || GetBoolOption(OPT_64BIT_SUFFIX)) ? "64D.exe" : "D.exe";
	}
	return m_cszTargetDebug64;
}

const char* CBldMaster::GetTargetRelease64()
{
	if (m_cszTargetRelease64.IsNonEmpty())
		return m_cszTargetRelease64;

	ttASSERT_MSG(GetDir64(), "64-bit target must be set in constructor after reading .srcfiles");
	m_cszTargetRelease64 = GetDir64();

	if (IsExeTypeLib())	{
		m_cszTargetRelease64.AppendFileName(GetProjectName());
		m_cszTargetRelease64 += (m_bAddPlatformSuffix || GetBoolOption(OPT_64BIT_SUFFIX)) ? "64.lib" : ".lib";
	}
	else if (IsExeTypeDll()) {
		m_cszTargetRelease64.AppendFileName(GetProjectName());
		m_cszTargetRelease64 += (m_bAddPlatformSuffix || GetBoolOption(OPT_64BIT_SUFFIX)) ? "64.dll" : ".dll";
	}
	else {
		m_cszTargetRelease64.AppendFileName(GetProjectName());
		m_cszTargetRelease64 += (m_bAddPlatformSuffix || GetBoolOption(OPT_64BIT_SUFFIX)) ? "64.exe" : ".exe";
	}
	return m_cszTargetRelease64;
}
