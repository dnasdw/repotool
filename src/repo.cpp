#include "repo.h"
#include <omp.h>
#include "bitbucket.h"
#include "crc32.h"
#include "crypto.h"
#include "github.h"
#include "md5.h"
#include "path.h"
#include "sha1.h"
#include "sha256.h"

const string CRepo::s_sContinuousHashDirName = "continuous_hash";
const string CRepo::s_sDataDirName = "data";
const UString CRepo::s_sRepoDirName = USTR("repo");
const UString CRepo::s_sTempDirName = USTR("temp");
const string CRepo::s_sReadmeFileName = "README.md";
const string CRepo::s_sGitAttributesFileName = ".gitattributes";
const string CRepo::s_sGitIgnoreFileName = ".gitignore";
const string CRepo::s_sIndexFileName = "index.txt";
const UString CRepo::s_sRemoteFileName = USTR("remote.txt");
const string CRepo::s_sCRC32FileName = "crc32.txt";
const string CRepo::s_sMD5FileName = "md5.txt";
const string CRepo::s_sSHA1FileName = "sha1.txt";
const string CRepo::s_sSHA256FileName = "sha256.txt";
const string CRepo::s_sConfigKeyGitUserName = "git.user.name";
const string CRepo::s_sConfigKeyGitUserEmail = "git.user.email";
const string CRepo::s_sRemoteInitCreateRemoteRepo = "create remote repo";
const string CRepo::s_sRemoteInitGitRemoteAdd = "git remote add";
const string CRepo::s_sRemoteInitImporting = "importing";
const string CRepo::s_sRemoteInitImportingComplete = "importing complete";
const string CRepo::s_sImportParamFromType = "from_type";
const string CRepo::s_sImportParamFromWorkspace = "from_workspace";
const string CRepo::s_sImportParamFromRepoName = "from_repo_name";
const string CRepo::s_sImportParamFromUser = "from_user";
const string CRepo::s_sImportParamFromPassword = "from_password";
const string CRepo::s_sImportParamToType = "to_type";
const string CRepo::s_sImportParamToWorkspace = "to_workspace";
const string CRepo::s_sImportParamToRepoName = "to_repo_name";
const string CRepo::s_sImportParamToUser = "to_user";
const string CRepo::s_sImportParamToPassword = "to_password";

bool SRepoIndexData::Parse(const string& a_sLine)
{
	vector<string> vLine = Split(a_sLine, "\t");
	if (vLine.size() != 3)
	{
		return false;
	}
	Commit = !vLine[0].empty();
	Sequence = vLine[1];
	Path = vLine[2];
	return true;
}

CRepo::CRepo()
	: m_bUpdateImport(false)
	, m_bVerbose(false)
	, m_sSeperator(USTR("/"))
	, m_bHasGithubImporter(false)
{
}

CRepo::~CRepo()
{
	if (!m_sCurrentWorkingDirPath.empty())
	{
		UChdir(m_sCurrentWorkingDirPath.c_str());
	}
}

void CRepo::SetInputPath(const UString& a_sInputPath)
{
	m_sInputPath = a_sInputPath;
}

void CRepo::SetOutputPath(const UString& a_sOutputPath)
{
	m_sOutputPath = a_sOutputPath;
}

void CRepo::SetType(const UString& a_sType)
{
	m_sType = UToU8(a_sType);
}

void CRepo::SetWorkspace(const UString& a_sWorkspace)
{
	m_sWorkspace = UToU8(a_sWorkspace);
}

void CRepo::SetUser(const UString& a_sUser)
{
	m_sUser = UToU8(a_sUser);
}

void CRepo::SetUpdateImport(bool a_bUpdateImport)
{
	m_bUpdateImport = a_bUpdateImport;
}

void CRepo::SetImportParam(const string& a_sImportParam)
{
	m_sImportParam = a_sImportParam;
}

void CRepo::SetImportKey(const string& a_sImportKey)
{
	m_sImportKey = a_sImportKey;
}

void CRepo::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

bool CRepo::Unpack()
{
	if (!initDir())
	{
		return false;
	}
	if (!loadIndex())
	{
		return false;
	}
	if (!StartWith(m_sOutputPath, USTR("/")))
	{
		m_sOutputPath = USTR("/") + m_sOutputPath;
	}
	m_sOutputPath = Replace(m_sOutputPath, USTR("\\"), USTR("/"));
	if (EndWith(m_sOutputPath, USTR("/")))
	{
		m_sOutputPath.erase(m_sOutputPath.size() - 1);
	}
	string sOutputPath = UToU8(m_sOutputPath);
	string sSHA1 = CSHA1::GetSHA1(sOutputPath.c_str(), static_cast<u32>(sOutputPath.size()));
	string sCRC32 = CCRC32::GetCRC32(sOutputPath.c_str(), static_cast<u32>(sOutputPath.size()));
	string sHashPrefix = sSHA1 + sCRC32;
	map<string, string>::const_iterator itPathHash = m_mPathHash.find(sOutputPath);
	if (itPathHash != m_mPathHash.end())
	{
		UPrintf(USTR("ERROR: %") PRIUS USTR(" already exists\n\n"), m_sOutputPath.c_str());
		return false;
	}
	n32 nHashIndex = 0;
	while (nHashIndex < s_nHashCountMax)
	{
		m_sHash = sHashPrefix + Format("%04d", nHashIndex);
		map<string, string>::const_iterator itHashPath = m_mHashPath.find(m_sHash);
		if (itHashPath == m_mHashPath.end())
		{
			break;
		}
		else
		{
			nHashIndex++;
		}
	}
	if (nHashIndex >= s_nHashCountMax)
	{
		UPrintf(USTR("ERROR: SHA1 %") PRIUS USTR(" CRC32 %") PRIUS USTR(" count > %d\n\n"), U8ToU(sSHA1).c_str(), U8ToU(sCRC32).c_str(), s_nHashCountMax);
		return false;
	}
	m_mPathHash.insert(make_pair(sOutputPath, m_sHash));
	m_mHashPath.insert(make_pair(m_sHash, sOutputPath));
	m_sInputPath = Replace(m_sInputPath, USTR("\\"), USTR("/"));
	if (EndWith(m_sInputPath, USTR("/")))
	{
		m_sInputPath.erase(m_sInputPath.size() - 1);
	}
	m_sInputPath = Replace(m_sInputPath, USTR("//?/"), USTR("\\\\?\\"));
	m_sInputPath = Replace(m_sInputPath, USTR("//"), USTR("\\\\"));
	if (StartWith(m_sInputPath, USTR("\\\\")))
	{
		m_sSeperator = USTR("\\");
		m_sInputPath = Replace(m_sInputPath, USTR("/"), m_sSeperator);
	}
	bool bInputPathIsDir = false;
#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
	u32 uAttributes = GetFileAttributesW(m_sInputPath.c_str());
	if (uAttributes != INVALID_FILE_ATTRIBUTES)
	{
		bInputPathIsDir = (uAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}
	else
	{
		UPrintf(USTR("ERROR: GetFileAttributesW %") PRIUS USTR(" failed\n\n"), m_sInputPath.c_str());
		return false;
	}
#else
	Stat stInputPath;
	if (UStat(m_sInputPath.c_str(), &stInputPath) == 0)
	{
		bInputPathIsDir = (stInputPath.st_mode & S_IFDIR) == S_IFDIR;
	}
	else
	{
		UPrintf(USTR("ERROR: stat %") PRIUS USTR(" failed\n\n"), m_sInputPath.c_str());
		return false;
	}
#endif
	vector<SPath> vPath;
	if (bInputPathIsDir)
	{
		vector<UString> vDir;
		queue<vector<UString>> qPath;
		qPath.push(vDir);
		queue<UString> qDir;
		qDir.push(m_sInputPath);
		while (!qDir.empty())
		{
			UString& sParent = qDir.front();
			vDir = qPath.front();
#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
			WIN32_FIND_DATAW ffd;
			HANDLE hFind = INVALID_HANDLE_VALUE;
			wstring sPattern = sParent + m_sSeperator + L"*";
			hFind = FindFirstFileW(sPattern.c_str(), &ffd);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
					{
						SPath path;
						for (vector<wstring>::iterator it = vDir.begin(); it != vDir.end(); ++it)
						{
							path.PathU += *it + L"/";
						}
						path.PathU += ffd.cFileName;
						path.Path = UToU8(path.PathU);
						path.IsDir = false;
						path.Size = 0;
						path.RuleVersion = 1;
						vPath.push_back(path);
					}
					else if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0)
					{
						SPath path;
						for (vector<wstring>::iterator it = vDir.begin(); it != vDir.end(); ++it)
						{
							path.PathU += *it + L"/";
						}
						path.PathU += ffd.cFileName;
						path.Path = UToU8(path.PathU);
						path.IsDir = true;
						path.Size = 0;
						path.RuleVersion = 1;
						vPath.push_back(path);
						vDir.push_back(ffd.cFileName);
						qPath.push(vDir);
						vDir.pop_back();
						wstring sDir = sParent + m_sSeperator + ffd.cFileName;
						qDir.push(sDir);
					}
				} while (FindNextFileW(hFind, &ffd) != 0);
				FindClose(hFind);
			}
#else
			DIR* pDir = opendir(sParent.c_str());
			if (pDir != nullptr)
			{
				dirent* pDirent = nullptr;
				while ((pDirent = readdir(pDir)) != nullptr)
				{
					string sName = pDirent->d_name;
#if SDW_PLATFORM == SDW_PLATFORM_MACOS
					sName = TSToS<string, string>(sName, "UTF-8-MAC", "UTF-8");
#endif
					// handle cases where d_type is DT_UNKNOWN
					if (pDirent->d_type == DT_UNKNOWN)
					{
						string sPath = sParent + "/" + sName;
						Stat st;
						if (UStat(sPath.c_str(), &st) == 0)
						{
							if (S_ISREG(st.st_mode))
							{
								pDirent->d_type = DT_REG;
							}
							else if (S_ISDIR(st.st_mode))
							{
								pDirent->d_type = DT_DIR;
							}
						}
					}
					if (pDirent->d_type == DT_REG)
					{
						SPath path;
						for (vector<string>::iterator it = vDir.begin(); it != vDir.end(); ++it)
						{
							path.PathU += *it + "/";
						}
						path.PathU += sName;
						path.Path = UToU8(path.PathU);
						path.IsDir = false;
						path.Size = 0;
						path.RuleVersion = 1;
						vPath.push_back(path);
					}
					else if (pDirent->d_type == DT_DIR && strcmp(pDirent->d_name, ".") != 0 && strcmp(pDirent->d_name, "..") != 0)
					{
						SPath path;
						for (vector<string>::iterator it = vDir.begin(); it != vDir.end(); ++it)
						{
							path.PathU += *it + "/";
						}
						path.PathU += sName;
						path.Path = UToU8(path.PathU);
						path.IsDir = true;
						path.Size = 0;
						path.RuleVersion = 1;
						vPath.push_back(path);
						vDir.push_back(sName);
						qPath.push(vDir);
						vDir.pop_back();
						string sDir = sParent + "/" + sName;
						qDir.push(sDir);
					}
				}
				closedir(pDir);
			}
#endif
			qPath.pop();
			qDir.pop();
		}
	}
	else
	{
		SPath path;
		path.PathU = USTR(".");
		path.Path = UToU8(path.PathU);
		path.IsDir = false;
		path.Size = 0;
		path.RuleVersion = 1;
		vPath.push_back(path);
	}
	sort(vPath.begin(), vPath.end());
	string sDataIndex;
	string sDataCRC32;
	string sDataMD5;
	string sDataSHA1;
	string sDataSHA256;
	if (!generateDataDirPath(true))
	{
		return false;
	}
	n64 nRepoSize = 0;
	n32 nRepoIndex = 0;
	n32 nRepoIndex2 = 0;
	n64 nGitPushSize = 0;
	n32 nGitPushIndex = 0;
	static const string c_sRepoReadme = "# repo4arc\n";
	static const string c_sRepoGitAttributes =
		"*.txt text eol=lf\n"
		"*.bin binary\n";
	static const string c_sRepoGitIgnore = "/temp";
	static const string c_sRepoIndex =
		"\t0\t" + s_sReadmeFileName + "\n"
		"\t0\t" + s_sGitAttributesFileName + "\n"
		"\t0\t" + s_sGitIgnoreFileName + "\n"
		"\t0\t" + s_sIndexFileName + "\n"
		"\t1\t" + s_sContinuousHashDirName + "/" + s_sCRC32FileName + "\n"
		"\t1\t" + s_sContinuousHashDirName + "/" + s_sMD5FileName + "\n"
		"\t1\t" + s_sContinuousHashDirName + "/" + s_sSHA1FileName + "\n"
		"\t1\t" + s_sContinuousHashDirName + "/" + s_sSHA256FileName + "\n"
		"\t1\t" + s_sCRC32FileName + "\n"
		"\t1\t" + s_sMD5FileName + "\n"
		"\t1\t" + s_sSHA1FileName + "\n"
		"\t1\t" + s_sSHA256FileName + "\n";
	string sRepoIndex = c_sRepoIndex;
	string sRepoContinuousCRC32;
	string sRepoContinuousMD5;
	string sRepoContinuousSHA1;
	string sRepoContinuousSHA256;
	string sRepoCRC32;
	string sRepoMD5;
	string sRepoSHA1;
	string sRepoSHA256;
	if (!generateRepoDirPath(nRepoIndex, true))
	{
		return false;
	}
	bool bNeedWriteRepoFile = true;
	for (n32 i = 0; i < static_cast<n32>(vPath.size()); i++)
	{
		SPath& path = vPath[i];
		path.RepoDataV1.RepoSize = nRepoSize;
		path.RepoDataV1.RepoIndex = nRepoIndex;
		path.RepoDataV1.RepoIndex2 = nRepoIndex2;
		if (path.IsDir)
		{
			sDataIndex += path.GetIndexLine();
			sDataCRC32 += path.GetCRC32HashLine();
			sDataMD5 += path.GetMD5HashLine();
			sDataSHA1 += path.GetSHA1HashLine();
			sDataSHA256 += path.GetSHA256HashLine();
			if (m_bVerbose)
			{
				UPrintf(USTR("\t<\t%") PRIUS USTR("\n"), path.PathU.c_str());
			}
			continue;
		}
		else
		{
			UString sInputFilePath = m_sInputPath;
			if (bInputPathIsDir)
			{
				sInputFilePath += m_sSeperator + Replace(path.PathU, USTR("/"), m_sSeperator);
			}
			FILE* fpInput = UFopen(sInputFilePath, USTR("rb"));
			if (fpInput == nullptr)
			{
				if (m_bVerbose)
				{
					UPrintf(USTR("\t<\t%") PRIUS USTR("\n"), path.PathU.c_str());
				}
				return false;
			}
			Fseek(fpInput, 0, SEEK_END);
			path.Size = Ftell(fpInput);
			Fseek(fpInput, 0, SEEK_SET);
			if (path.Size == 0)
			{
				fclose(fpInput);
				sDataIndex += path.GetIndexLine();
				sDataCRC32 += path.GetCRC32HashLine();
				sDataMD5 += path.GetMD5HashLine();
				sDataSHA1 += path.GetSHA1HashLine();
				sDataSHA256 += path.GetSHA256HashLine();
				if (m_bVerbose)
				{
					UPrintf(USTR("\t<\t%") PRIUS USTR("\n"), path.PathU.c_str());
				}
				continue;
			}
			CCRC32 crc32;
			CMD5 md5;
			CSHA1 sha1;
			CSHA256 sha256;
			n64 nFileSize = path.Size;
			static u8 c_uBuffer[s_nRepoFileSizeMax] = {};
			while (nFileSize > 0)
			{
				if (nRepoSize >= s_nRepoSizeMax)
				{
					if (bNeedWriteRepoFile)
					{
						if (!writeTextFile(m_sRepoReadmeFilePath, c_sRepoReadme))
						{
							fclose(fpInput);
							return false;
						}
						if (!writeTextFile(m_sRepoGitAttributesFilePath, c_sRepoGitAttributes))
						{
							fclose(fpInput);
							return false;
						}
						if (!writeTextFile(m_sRepoGitIgnoreFilePath, c_sRepoGitIgnore))
						{
							fclose(fpInput);
							return false;
						}
						if (!writeTextFile(m_sRepoIndexFilePath, sRepoIndex))
						{
							fclose(fpInput);
							return false;
						}
						if (!writeTextFile(m_sRepoContinuousCRC32FilePath, sRepoContinuousCRC32))
						{
							fclose(fpInput);
							return false;
						}
						if (!writeTextFile(m_sRepoContinuousMD5FilePath, sRepoContinuousMD5))
						{
							fclose(fpInput);
							return false;
						}
						if (!writeTextFile(m_sRepoContinuousSHA1FilePath, sRepoContinuousSHA1))
						{
							fclose(fpInput);
							return false;
						}
						if (!writeTextFile(m_sRepoContinuousSHA256FilePath, sRepoContinuousSHA256))
						{
							fclose(fpInput);
							return false;
						}
						if (!writeTextFile(m_sRepoCRC32FilePath, sRepoCRC32))
						{
							fclose(fpInput);
							return false;
						}
						if (!writeTextFile(m_sRepoMD5FilePath, sRepoMD5))
						{
							fclose(fpInput);
							return false;
						}
						if (!writeTextFile(m_sRepoSHA1FilePath, sRepoSHA1))
						{
							fclose(fpInput);
							return false;
						}
						if (!writeTextFile(m_sRepoSHA256FilePath, sRepoSHA256))
						{
							fclose(fpInput);
							return false;
						}
						bNeedWriteRepoFile = false;
					}
					nRepoSize = 0;
					nRepoIndex++;
					nRepoIndex2 = 0;
					nGitPushSize = 0;
					nGitPushIndex = 0;
					sRepoIndex = c_sRepoIndex;
					sRepoContinuousCRC32.clear();
					sRepoContinuousMD5.clear();
					sRepoContinuousSHA1.clear();
					sRepoContinuousSHA256.clear();
					sRepoCRC32.clear();
					sRepoMD5.clear();
					sRepoSHA1.clear();
					sRepoSHA256.clear();
					if (!generateRepoDirPath(nRepoIndex, true))
					{
						fclose(fpInput);
						return false;
					}
					if (nFileSize == path.Size)
					{
						path.RepoDataV1.RepoSize = nRepoSize;
						path.RepoDataV1.RepoIndex = nRepoIndex;
						path.RepoDataV1.RepoIndex2 = nRepoIndex2;
					}
				}
				if (!generateRepoDataFilePath(nRepoIndex2, true))
				{
					fclose(fpInput);
					return false;
				}
				if (m_bVerbose)
				{
					UPrintf(USTR("%") PRIUS USTR("\t<\t%") PRIUS USTR("\n"), m_sRepoDataFileLocalPath.c_str() + m_sRootDirPath.size() + 1, path.PathU.c_str());
				}
				FILE* fpOutput = UFopen(m_sRepoDataFileLocalPath, USTR("wb"));
				if (fpOutput == nullptr)
				{
					fclose(fpInput);
					return false;
				}
				n64 nSize = nFileSize > s_nRepoFileSizeMax ? s_nRepoFileSizeMax : nFileSize;
				fread(c_uBuffer, 1, static_cast<size_t>(nSize), fpInput);
				fwrite(c_uBuffer, 1, static_cast<size_t>(nSize), fpOutput);
				fclose(fpOutput);
				sRepoIndex += Format("\t%d\t%s\n", nGitPushIndex, m_sRepoDataFileRelativePath.c_str());
				nGitPushSize += nSize;
				if (nGitPushSize >= s_nRepoFileSizeMax)
				{
					nGitPushSize = 0;
					nGitPushIndex = 1 - nGitPushIndex;
				}
#pragma omp parallel for
				for (n32 nHashType = 0; nHashType < kHashTypeCount; nHashType++)
				{
					switch (nHashType)
					{
					case kHashTypeContinuousCRC32:
						{
							crc32.UpdateCRC32(c_uBuffer, static_cast<u32>(nSize));
							string sCRC32 = crc32.GetCRC32();
							sRepoContinuousCRC32 += m_sRepoDataFileRelativePath + "\t" + sCRC32 + "\n";
						}
						break;
					case kHashTypeContinuousMD5:
						{
							md5.UpdateMD5(c_uBuffer, static_cast<u32>(nSize));
							string sMD5 = md5.GetMD5();
							sRepoContinuousMD5 += m_sRepoDataFileRelativePath + "\t" + sMD5 + "\n";
						}
						break;
					case kHashTypeContinuousSHA1:
						{
							sha1.UpdateSHA1(c_uBuffer, static_cast<u32>(nSize));
							string sSHA1 = sha1.GetSHA1();
							sRepoContinuousSHA1 += m_sRepoDataFileRelativePath + "\t" + sSHA1 + "\n";
						}
						break;
					case kHashTypeContinuousSHA256:
						{
							sha256.UpdateSHA256(c_uBuffer, static_cast<u32>(nSize));
							string sSHA256 = sha256.GetSHA256();
							sRepoContinuousSHA256 += m_sRepoDataFileRelativePath + "\t" + sSHA256 + "\n";
						}
						break;
					case kHashTypeCRC32:
						{
							string sCRC32 = CCRC32::GetCRC32(c_uBuffer, static_cast<u32>(nSize));
							sRepoCRC32 += m_sRepoDataFileRelativePath + "\t" + sCRC32 + "\n";
						}
						break;
					case kHashTypeMD5:
						{
							string sMD5 = CMD5::GetMD5(c_uBuffer, static_cast<u32>(nSize));
							sRepoMD5 += m_sRepoDataFileRelativePath + "\t" + sMD5 + "\n";
						}
						break;
					case kHashTypeSHA1:
						{
							string sSHA1 = CSHA1::GetSHA1(c_uBuffer, static_cast<u32>(nSize));
							sRepoSHA1 += m_sRepoDataFileRelativePath + "\t" + sSHA1 + "\n";
						}
						break;
					case kHashTypeSHA256:
						{
							string sSHA256 = CSHA256::GetSHA256(c_uBuffer, static_cast<u32>(nSize));
							sRepoSHA256 += m_sRepoDataFileRelativePath + "\t" + sSHA256 + "\n";
						}
						break;
					}
				}
				bNeedWriteRepoFile = true;
				nFileSize -= nSize;
				nRepoSize += nSize;
				nRepoIndex2++;
			}
			fclose(fpInput);
			path.RepoDataV1.CRC32Hex = crc32.GetCRC32();
			path.RepoDataV1.MD5Hex = md5.GetMD5();
			path.RepoDataV1.SHA1Hex = sha1.GetSHA1();
			path.RepoDataV1.SHA256Hex = sha256.GetSHA256();
			sDataIndex += path.GetIndexLine();
			sDataCRC32 += path.GetCRC32HashLine();
			sDataMD5 += path.GetMD5HashLine();
			sDataSHA1 += path.GetSHA1HashLine();
			sDataSHA256 += path.GetSHA256HashLine();
		}
	}
	if (bNeedWriteRepoFile)
	{
		if (!writeTextFile(m_sRepoReadmeFilePath, c_sRepoReadme))
		{
			return false;
		}
		if (!writeTextFile(m_sRepoGitAttributesFilePath, c_sRepoGitAttributes))
		{
			return false;
		}
		if (!writeTextFile(m_sRepoGitIgnoreFilePath, c_sRepoGitIgnore))
		{
			return false;
		}
		if (!writeTextFile(m_sRepoIndexFilePath, sRepoIndex))
		{
			return false;
		}
		if (!writeTextFile(m_sRepoContinuousCRC32FilePath, sRepoContinuousCRC32))
		{
			return false;
		}
		if (!writeTextFile(m_sRepoContinuousMD5FilePath, sRepoContinuousMD5))
		{
			return false;
		}
		if (!writeTextFile(m_sRepoContinuousSHA1FilePath, sRepoContinuousSHA1))
		{
			return false;
		}
		if (!writeTextFile(m_sRepoContinuousSHA256FilePath, sRepoContinuousSHA256))
		{
			return false;
		}
		if (!writeTextFile(m_sRepoCRC32FilePath, sRepoCRC32))
		{
			return false;
		}
		if (!writeTextFile(m_sRepoMD5FilePath, sRepoMD5))
		{
			return false;
		}
		if (!writeTextFile(m_sRepoSHA1FilePath, sRepoSHA1))
		{
			return false;
		}
		if (!writeTextFile(m_sRepoSHA256FilePath, sRepoSHA256))
		{
			return false;
		}
	}
	if (!writeTextFile(m_sDataIndexFilePath, sDataIndex))
	{
		return false;
	}
	if (!writeTextFile(m_sDataRemoteFilePath, ""))
	{
		return false;
	}
	if (!writeTextFile(m_sDataCRC32FilePath, sDataCRC32))
	{
		return false;
	}
	if (!writeTextFile(m_sDataMD5FilePath, sDataMD5))
	{
		return false;
	}
	if (!writeTextFile(m_sDataSHA1FilePath, sDataSHA1))
	{
		return false;
	}
	if (!writeTextFile(m_sDataSHA256FilePath, sDataSHA256))
	{
		return false;
	}
	bool bResult = writeIndex();
	return bResult;
}

bool CRepo::Pack()
{
	if (!initDir())
	{
		return false;
	}
	if (!loadIndex())
	{
		return false;
	}
	if (!StartWith(m_sInputPath, USTR("/")))
	{
		m_sInputPath = USTR("/") + m_sInputPath;
	}
	m_sInputPath = Replace(m_sInputPath, USTR("\\"), USTR("/"));
	if (EndWith(m_sInputPath, USTR("/")))
	{
		m_sInputPath.erase(m_sInputPath.size() - 1);
	}
	string sInputPath = UToU8(m_sInputPath);
	map<string, string>::const_iterator itPathHash = m_mPathHash.find(sInputPath);
	if (itPathHash == m_mPathHash.end())
	{
		UPrintf(USTR("ERROR: %") PRIUS USTR(" does not exists\n\n"), m_sInputPath.c_str());
		return false;
	}
	m_sHash = itPathHash->second;
	m_sOutputPath = Replace(m_sOutputPath, USTR("\\"), USTR("/"));
	if (EndWith(m_sOutputPath, USTR("/")))
	{
		m_sOutputPath.erase(m_sOutputPath.size() - 1);
	}
	m_sOutputPath = Replace(m_sOutputPath, USTR("//?/"), USTR("\\\\?\\"));
	m_sOutputPath = Replace(m_sOutputPath, USTR("//"), USTR("\\\\"));
	if (StartWith(m_sOutputPath, USTR("\\\\")))
	{
		m_sSeperator = USTR("\\");
		m_sOutputPath = Replace(m_sOutputPath, USTR("/"), m_sSeperator);
	}
	if (!generateDataDirPath(false))
	{
		return false;
	}
	string sDataIndex;
	if (!readTextFile(m_sDataIndexFilePath, sDataIndex))
	{
		return false;
	}
	bool bMakeDir = false;
	vector<string> vIndex = SplitOf(sDataIndex, "\r\n");
	for (vector<string>::const_iterator it = vIndex.begin(); it != vIndex.end(); ++it)
	{
		const string& sLine = *it;
		if (sLine.empty())
		{
			continue;
		}
		SPath path;
		if (!path.Parse(sLine))
		{
			UPrintf(USTR("ERROR: parse index %") PRIUS USTR(" failed\n\n"), U8ToU(sLine).c_str());
			return false;
		}
		if (!bMakeDir && path.PathU != USTR("."))
		{
			if (!UMakeDir(m_sOutputPath))
			{
				UPrintf(USTR("ERROR: make dir %") PRIUS USTR(" failed\n\n"), m_sOutputPath.c_str());
				return false;
			}
			bMakeDir = true;
		}
		switch (path.RuleVersion)
		{
		case 1:
			if (!packV1File(path))
			{
				return false;
			}
			break;
		default:
			UPrintf(USTR("ERROR: unsupport rule version %d\n\n"), path.RuleVersion);
			return false;
		}
	}
	return true;
}

bool CRepo::Upload()
{
	if (!initDir())
	{
		return false;
	}
	if (!loadConfig())
	{
		return false;
	}
	m_bHasGithubImporter = loadGithubImporterSecretConfig();
	if (!loadUser())
	{
		return false;
	}
	if (!loadIndex())
	{
		return false;
	}
	if (!StartWith(m_sInputPath, USTR("/")))
	{
		m_sInputPath = USTR("/") + m_sInputPath;
	}
	m_sInputPath = Replace(m_sInputPath, USTR("\\"), USTR("/"));
	if (EndWith(m_sInputPath, USTR("/")))
	{
		m_sInputPath.erase(m_sInputPath.size() - 1);
	}
	string sInputPath = UToU8(m_sInputPath);
	map<string, string>::const_iterator itPathHash = m_mPathHash.find(sInputPath);
	if (itPathHash == m_mPathHash.end())
	{
		UPrintf(USTR("ERROR: %") PRIUS USTR(" does not exists\n\n"), m_sInputPath.c_str());
		return false;
	}
	m_sHash = itPathHash->second;
	if (!generateDataDirPath(false))
	{
		return false;
	}
	string sRemote;
	if (!readTextFile(m_sDataRemoteFilePath, sRemote))
	{
		return false;
	}
	vector<pair<string, string>> vTypeWorkspace;
	vector<string> vRemote = SplitOf(sRemote, "\r\n");
	for (vector<string>::const_iterator it = vRemote.begin(); it != vRemote.end(); ++it)
	{
		const string& sLine = *it;
		if (sLine.empty())
		{
			continue;
		}
		vector<string> vLine = Split(sLine, "\t");
		if (vLine.size() != 2)
		{
			UPrintf(USTR("ERROR: parse remote %") PRIUS USTR(" failed\n\n"), U8ToU(sLine).c_str());
			return false;
		}
		const string& sType = vLine[0];
		const string& sWorkspace = vLine[1];
		if (sType == m_sType && sWorkspace == m_sWorkspace)
		{
			return true;
		}
		vTypeWorkspace.push_back(make_pair(sType, sWorkspace));
	}
	m_sRemoteName = m_sType + "_" + m_sWorkspace;
	string sDataIndex;
	if (!readTextFile(m_sDataIndexFilePath, sDataIndex))
	{
		return false;
	}
	vector<string> vIndex = SplitOf(sDataIndex, "\r\n");
	n32 nRepoCount = 0;
	for (vector<string>::const_reverse_iterator it = vIndex.rbegin(); it != vIndex.rend(); ++it)
	{
		const string& sLine = *it;
		if (sLine.empty())
		{
			continue;
		}
		SPath path;
		if (!path.Parse(sLine))
		{
			UPrintf(USTR("ERROR: parse index %") PRIUS USTR(" failed\n\n"), U8ToU(sLine).c_str());
			return false;
		}
		n64 nFileSize = path.Size;
		n64 nRepoSize = path.RepoDataV1.RepoSize;
		n32 nRepoIndex = path.RepoDataV1.RepoIndex;
		n32 nRepoIndex2 = path.RepoDataV1.RepoIndex2;
		while (nFileSize > 0)
		{
			if (nRepoSize >= s_nRepoSizeMax)
			{
				nRepoSize = 0;
				nRepoIndex++;
				nRepoIndex2 = 0;
			}
			n64 nSize = nFileSize > s_nRepoFileSizeMax ? s_nRepoFileSizeMax : nFileSize;
			nFileSize -= nSize;
			nRepoSize += nSize;
			nRepoIndex2++;
		}
		nRepoCount = nRepoIndex + 1;
		break;
	}
	bool bAllComplete = true;
	for (n32 nRepoIndex = 0; nRepoIndex < nRepoCount; nRepoIndex++)
	{
		if (!generateRepoDirPath(nRepoIndex, true))
		{
			return false;
		}
		if (UChdir(m_sRepoDirPath.c_str()) != 0)
		{
			UPrintf(USTR("ERROR: change dir %") PRIUS USTR(" failed\n\n"), m_sRepoDirPath.c_str());
			return false;
		}
		UString sRemoteImportTempFilePath = s_sTempDirName + U8ToU("/" + m_sRemoteName + "_import.txt");
		UString sRemoteTempFilePath = s_sTempDirName + U8ToU("/" + m_sRemoteName + ".txt");
		string sRemoteTemp;
		FILE* fp = UFopen(sRemoteTempFilePath, USTR("rb"), false);
		if (fp != nullptr)
		{
			fclose(fp);
			if (!readTextFile(sRemoteTempFilePath, sRemoteTemp))
			{
				return false;
			}
		}
		set<string> sRemoteInit;
		vector<string> vRemoteTemp = SplitOf(sRemoteTemp, "\r\n");
		for (vector<string>::const_iterator it = vRemoteTemp.begin(); it != vRemoteTemp.end(); ++it)
		{
			const string& sLine = *it;
			if (!sLine.empty())
			{
				sRemoteInit.insert(sLine);
			}
		}
		if (sRemoteInit.find(s_sRemoteInitImportingComplete) != sRemoteInit.end())
		{
			continue;
		}
		if (sRemoteInit.find(s_sRemoteInitImporting) != sRemoteInit.end())
		{
			if (m_sType == CGithub::s_sTypeName)
			{
				CGithub github;
				github.SetWorkspace(m_sWorkspace);
				github.SetRepoName(m_sRepoName);
				github.SetUser(m_sUser);
				github.SetPersonalAccessToken(m_sPassword);
				github.SetVerbose(m_bVerbose);
				bool bResult = github.GetImportStatus();
				if (bResult)
				{
					string sImportStatusString = github.GetImportStatusString();
					if (sImportStatusString == CGithub::s_sImportStatusImporting)
					{
						if (m_bUpdateImport)
						{
							bResult = github.PatchImportRepo();
							if (bResult)
							{
								sImportStatusString = github.GetImportStatusString();
								if (sImportStatusString == CGithub::s_sImportStatusComplete)
								{
									sRemoteInit.insert(s_sRemoteInitImportingComplete);
									sRemoteTemp += s_sRemoteInitImportingComplete + "\n";
									writeTextFile(sRemoteTempFilePath, sRemoteTemp);
								}
								else
								{
									bAllComplete = false;
								}
							}
						}
						else
						{
							bAllComplete = false;
						}
					}
					else if (sImportStatusString == CGithub::s_sImportStatusComplete)
					{
						sRemoteInit.insert(s_sRemoteInitImportingComplete);
						sRemoteTemp += s_sRemoteInitImportingComplete + "\n";
						writeTextFile(sRemoteTempFilePath, sRemoteTemp);
					}
					else if (sImportStatusString == CGithub::s_sImportStatusError)
					{
						bResult = github.PatchImportRepo();
						if (bResult)
						{
							sImportStatusString = github.GetImportStatusString();
							if (sImportStatusString == CGithub::s_sImportStatusComplete)
							{
								sRemoteInit.insert(s_sRemoteInitImportingComplete);
								sRemoteTemp += s_sRemoteInitImportingComplete + "\n";
								writeTextFile(sRemoteTempFilePath, sRemoteTemp);
							}
							else
							{
								bAllComplete = false;
							}
						}
					}
				}
				if (bResult)
				{
					continue;
				}
				else
				{
					return false;
				}
			}
			else if (m_bHasGithubImporter)
			{
				CGithub githubImportRecorder;
				githubImportRecorder.SetWorkspace(m_mConfig[CGithub::s_sConfigKeyImportRecorderWorkspace]);
				githubImportRecorder.SetRepoName(m_mConfig[CGithub::s_sConfigKeyImportRecorderRepoName]);
				githubImportRecorder.SetUser(m_mConfig[CGithub::s_sConfigKeyImportRecorderUser]);
				githubImportRecorder.SetPersonalAccessToken(m_mConfig[CGithub::s_sConfigKeyImportRecorderPersonalAccessToken]);
				githubImportRecorder.SetVerbose(m_bVerbose);
				string sImportRecorderFilePath = generateImportRecorderFilePath(m_sType, m_sWorkspace, m_sRepoName);
				bool bResult = githubImportRecorder.FileExist(sImportRecorderFilePath);
				if (bResult)
				{
					sRemoteInit.insert(s_sRemoteInitImportingComplete);
					sRemoteTemp += s_sRemoteInitImportingComplete + "\n";
					writeTextFile(sRemoteTempFilePath, sRemoteTemp);
				}
				else
				{
					if (m_bUpdateImport)
					{
						string sImportParam;
						if (!readTextFile(sRemoteImportTempFilePath, sImportParam))
						{
							return false;
						}
						string sImportKey = FGenerateKey(m_sImportKey);
						m_sImportParam = FEncryptString(sImportParam, sImportKey);
#ifdef DEBUG_LOCAL_IMPORT
						bResult = Import();
#else	// DEBUG_LOCAL_IMPORT
						CGithub github;
						github.SetWorkspace(m_mConfig[CGithub::s_sConfigKeyImporterWorkspace]);
						github.SetRepoName(m_mConfig[CGithub::s_sConfigKeyImporterRepoName]);
						github.SetUser(m_mConfig[CGithub::s_sConfigKeyImporterUser]);
						github.SetPersonalAccessToken(m_mConfig[CGithub::s_sConfigKeyImporterPersonalAccessToken]);
						github.SetVerbose(m_bVerbose);
						bResult = github.TriggerWorkflowImport(m_mConfig[CGithub::s_sConfigKeyImporterWorkflowFileName], m_sImportParam);
#endif	// DEBUG_LOCAL_IMPORT
						if (bResult)
						{
							bAllComplete = false;
						}
					}
					else
					{
						bResult = true;
						bAllComplete = false;
					}
				}
				if (bResult)
				{
					continue;
				}
				else
				{
					return false;
				}
			}
		}
		if (!gitInitMaster())
		{
			return false;
		}
		if (!gitConfigUser())
		{
			return false;
		}
		string sRemoteURL;
		n32 nAddDataFileSizeMaxCountPerCommit = 1;
		if (m_sType == CBitbucket::s_sTypeName)
		{
			CBitbucket bitbucket;
			bitbucket.SetWorkspace(m_sWorkspace);
			bitbucket.SetRepoName(m_sRepoName);
			bitbucket.SetUser(m_sUser);
			bitbucket.SetAppPassword(m_sPassword);
			bitbucket.SetProjectName(m_mConfig[CBitbucket::s_sConfigKeyProjectName]);
			bitbucket.SetProjectKey(m_mConfig[CBitbucket::s_sConfigKeyProjectKey]);
			bitbucket.SetVerbose(m_bVerbose);
			if (sRemoteInit.find(s_sRemoteInitCreateRemoteRepo) == sRemoteInit.end())
			{
				if (!bitbucket.CreateRepo())
				{
					return false;
				}
				sRemoteInit.insert(s_sRemoteInitCreateRemoteRepo);
				sRemoteTemp += s_sRemoteInitCreateRemoteRepo + "\n";
				writeTextFile(sRemoteTempFilePath, sRemoteTemp);
			}
			sRemoteURL = bitbucket.GetRepoRemoteHttpsURL();
			m_sRepoPushURL = bitbucket.GetRepoPushHttpsURL();
			nAddDataFileSizeMaxCountPerCommit = bitbucket.GetAddDataFileSizeMaxCountPerCommit();
		}
		else if (m_sType == CGithub::s_sTypeName)
		{
			CGithub github;
			github.SetWorkspace(m_sWorkspace);
			github.SetRepoName(m_sRepoName);
			github.SetUser(m_sUser);
			github.SetPersonalAccessToken(m_sPassword);
			github.SetVerbose(m_bVerbose);
			if (sRemoteInit.find(s_sRemoteInitCreateRemoteRepo) == sRemoteInit.end())
			{
				if (!github.CreateRepo())
				{
					return false;
				}
				sRemoteInit.insert(s_sRemoteInitCreateRemoteRepo);
				sRemoteTemp += s_sRemoteInitCreateRemoteRepo + "\n";
				writeTextFile(sRemoteTempFilePath, sRemoteTemp);
			}
			sRemoteURL = github.GetRepoRemoteHttpsURL();
			m_sRepoPushURL = github.GetRepoPushHttpsURL();
			nAddDataFileSizeMaxCountPerCommit = github.GetAddDataFileSizeMaxCountPerCommit();
		}
		if (nAddDataFileSizeMaxCountPerCommit < 1)
		{
			nAddDataFileSizeMaxCountPerCommit = 1;
		}
		if (sRemoteInit.find(s_sRemoteInitGitRemoteAdd) == sRemoteInit.end())
		{
			gitRemoteAdd(sRemoteURL);
			sRemoteInit.insert(s_sRemoteInitGitRemoteAdd);
			sRemoteTemp += s_sRemoteInitGitRemoteAdd + "\n";
			writeTextFile(sRemoteTempFilePath, sRemoteTemp);
		}
		bool bResult = false;
		for (vector<pair<string, string>>::const_iterator itTypeWorkspace = vTypeWorkspace.begin(); itTypeWorkspace != vTypeWorkspace.end(); ++itTypeWorkspace)
		{
			const string& sType = itTypeWorkspace->first;
			const string& sWorkspace = itTypeWorkspace->second;
			string sSourceRemoteURL;
			if (sType == CBitbucket::s_sTypeName)
			{
				CBitbucket bitbucket;
				bitbucket.SetWorkspace(sWorkspace);
				bitbucket.SetRepoName(m_sRepoName);
				bitbucket.SetVerbose(m_bVerbose);
				sSourceRemoteURL = bitbucket.GetRepoRemoteHttpsURL();
			}
			else if (sType == CGithub::s_sTypeName)
			{
				CGithub github;
				github.SetWorkspace(sWorkspace);
				github.SetRepoName(m_sRepoName);
				github.SetVerbose(m_bVerbose);
				sSourceRemoteURL = github.GetRepoRemoteHttpsURL();
			}
			if (m_sType == CGithub::s_sTypeName)
			{
				CGithub github;
				github.SetWorkspace(m_sWorkspace);
				github.SetRepoName(m_sRepoName);
				github.SetUser(m_sUser);
				github.SetPersonalAccessToken(m_sPassword);
				github.SetVerbose(m_bVerbose);
				bResult = github.StartImportRepo(sSourceRemoteURL);
				if (bResult)
				{
					sRemoteInit.insert(s_sRemoteInitImporting);
					sRemoteTemp += s_sRemoteInitImporting + "\n";
					writeTextFile(sRemoteTempFilePath, sRemoteTemp);
					string sImportStatusString = github.GetImportStatusString();
					if (sImportStatusString == CGithub::s_sImportStatusImporting)
					{
						bAllComplete = false;
					}
					else if (sImportStatusString == CGithub::s_sImportStatusComplete)
					{
						sRemoteInit.insert(s_sRemoteInitImportingComplete);
						sRemoteTemp += s_sRemoteInitImportingComplete + "\n";
						writeTextFile(sRemoteTempFilePath, sRemoteTemp);
					}
					else if (sImportStatusString == CGithub::s_sImportStatusError)
					{
						bAllComplete = false;
					}
				}
			}
			else if (m_bHasGithubImporter)
			{
				string sImportParam = s_sImportParamFromType + "=" + sType;
				sImportParam += " " + s_sImportParamFromWorkspace + "=" + sWorkspace;
				sImportParam += " " + s_sImportParamFromRepoName + "=" + m_sRepoName;
				sImportParam += " " + s_sImportParamToType + "=" + m_sType;
				sImportParam += " " + s_sImportParamToWorkspace + "=" + m_sWorkspace;
				sImportParam += " " + s_sImportParamToRepoName + "=" + m_sRepoName;
				sImportParam += " " + s_sImportParamToUser + "=" + m_sUser;
				sImportParam += " " + s_sImportParamToPassword + "=" + m_sPassword;
				sImportParam += " " + CGithub::s_sConfigKeyImportRecorderWorkspace + "=" + m_mConfig[CGithub::s_sConfigKeyImportRecorderWorkspace];
				sImportParam += " " + CGithub::s_sConfigKeyImportRecorderRepoName + "=" + m_mConfig[CGithub::s_sConfigKeyImportRecorderRepoName];
				sImportParam += " " + CGithub::s_sConfigKeyImportRecorderUser + "=" + m_mConfig[CGithub::s_sConfigKeyImportRecorderUser];
				sImportParam += " " + CGithub::s_sConfigKeyImportRecorderPersonalAccessToken + "=" + m_mConfig[CGithub::s_sConfigKeyImportRecorderPersonalAccessToken];
				bool bResultForUpdateImport = writeTextFile(sRemoteImportTempFilePath, sImportParam);
				string sImportKey = FGenerateKey(m_sImportKey);
				m_sImportParam = FEncryptString(sImportParam, sImportKey);
#ifdef DEBUG_LOCAL_IMPORT
				bResult = Import();
#else	// DEBUG_LOCAL_IMPORT
				CGithub github;
				github.SetWorkspace(m_mConfig[CGithub::s_sConfigKeyImporterWorkspace]);
				github.SetRepoName(m_mConfig[CGithub::s_sConfigKeyImporterRepoName]);
				github.SetUser(m_mConfig[CGithub::s_sConfigKeyImporterUser]);
				github.SetPersonalAccessToken(m_mConfig[CGithub::s_sConfigKeyImporterPersonalAccessToken]);
				github.SetVerbose(m_bVerbose);
				bResult = github.TriggerWorkflowImport(m_mConfig[CGithub::s_sConfigKeyImporterWorkflowFileName], m_sImportParam);
#endif	// DEBUG_LOCAL_IMPORT
				if (bResult)
				{
					if (bResultForUpdateImport)
					{
						sRemoteInit.insert(s_sRemoteInitImporting);
						sRemoteTemp += s_sRemoteInitImporting + "\n";
						writeTextFile(sRemoteTempFilePath, sRemoteTemp);
					}
					bAllComplete = false;
				}
			}
			if (bResult)
			{
				break;
			}
		}
		if (bResult)
		{
			continue;
		}
		vector<SRepoIndexData> vRepoIndexData;
		bool bSpecialPath[2] = { false, false };
		n32 nAddDataFileSizeMaxCount = 0;
		n32 nSequence = 0;
		map<n32, vector<n32>> mSequenceRepoIndexDataIndex;
		string sRepoIndex;
		if (!readTextFile(m_sRepoIndexFilePath, sRepoIndex))
		{
			return false;
		}
		vector<string> vRepoIndex = SplitOf(sRepoIndex, "\r\n");
		for (vector<string>::const_iterator it = vRepoIndex.begin(); it != vRepoIndex.end(); ++it)
		{
			const string& sLine = *it;
			if (sLine.empty())
			{
				continue;
			}
			SRepoIndexData repoIndexData;
			if (!repoIndexData.Parse(sLine))
			{
				UPrintf(USTR("ERROR: parse repo index data %") PRIUS USTR(" failed\n\n"), U8ToU(sLine).c_str());
				return false;
			}
			vRepoIndexData.push_back(repoIndexData);
			if (!repoIndexData.Commit)
			{
				const string& sPath = repoIndexData.Path;
				bSpecialPath[1] = sPath == s_sReadmeFileName
					|| sPath == s_sGitAttributesFileName
					|| sPath == s_sGitIgnoreFileName
					|| sPath == s_sIndexFileName
					|| sPath == s_sContinuousHashDirName + "/" + s_sCRC32FileName
					|| sPath == s_sContinuousHashDirName + "/" + s_sMD5FileName
					|| sPath == s_sContinuousHashDirName + "/" + s_sSHA1FileName
					|| sPath == s_sContinuousHashDirName + "/" + s_sSHA256FileName
					|| sPath == s_sCRC32FileName
					|| sPath == s_sMD5FileName
					|| sPath == s_sSHA1FileName
					|| sPath == s_sSHA256FileName;
				static string c_sSequence = repoIndexData.Sequence;
				if (repoIndexData.Sequence != c_sSequence)
				{
					c_sSequence = repoIndexData.Sequence;
					if (bSpecialPath[0] || bSpecialPath[1])
					{
						nAddDataFileSizeMaxCount = nAddDataFileSizeMaxCountPerCommit;
					}
					else
					{
						nAddDataFileSizeMaxCount++;
					}
				}
				bSpecialPath[0] = bSpecialPath[1];
				if (nAddDataFileSizeMaxCount >= nAddDataFileSizeMaxCountPerCommit)
				{
					nAddDataFileSizeMaxCount = 0;
					nSequence++;
				}
				mSequenceRepoIndexDataIndex[nSequence].push_back(static_cast<n32>(vRepoIndexData.size() - 1));
			}
		}
		bool bPush = false;
		for (map<n32, vector<n32>>::const_iterator it = mSequenceRepoIndexDataIndex.begin(); it != mSequenceRepoIndexDataIndex.end(); ++it)
		{
			string sCommitMessage;
			const vector<n32>& vRepoIndexDataIndex = it->second;
			for (n32 i = 0; i < static_cast<n32>(vRepoIndexDataIndex.size()); i++)
			{
				n32 nRepoIndexDataIndex = vRepoIndexDataIndex[i];
				SRepoIndexData& repoIndexData = vRepoIndexData[nRepoIndexDataIndex];
				if (repoIndexData.Path == s_sReadmeFileName)
				{
					sCommitMessage = "Initial commit";
				}
				else if (repoIndexData.Path == s_sSHA256FileName)
				{
					sCommitMessage = "add hash";
				}
				if (!gitAdd(repoIndexData.Path))
				{
					return false;
				}
				repoIndexData.Commit = true;
			}
			sRepoIndex.clear();
			for (n32 i = 0; i < static_cast<n32>(vRepoIndexData.size()); i++)
			{
				SRepoIndexData& repoIndexData = vRepoIndexData[i];
				sRepoIndex += Format("%s\t%s\t%s\n", (repoIndexData.Commit ? "-" : ""), repoIndexData.Sequence.c_str(), repoIndexData.Path.c_str());
			}
			if (!writeTextFile(m_sRepoIndexFilePath, sRepoIndex))
			{
				return false;
			}
			if (!gitAdd(s_sIndexFileName))
			{
				return false;
			}
			if (sCommitMessage.empty())
			{
				sCommitMessage = "add data";
			}
			if (!gitCommit(sCommitMessage))
			{
				return false;
			}
			if (!gitPush(false))
			{
				return false;
			}
			bPush = true;
		}
		if (!bPush)
		{
			if (!gitPush(false))
			{
				return false;
			}
		}
	}
	if (bAllComplete)
	{
		sRemote += m_sType + "\t" + m_sWorkspace + "\n";
		if (!writeTextFile(m_sDataRemoteFilePath, sRemote))
		{
			return false;
		}
	}
	UChdir(m_sCurrentWorkingDirPath.c_str());
	return true;
}

bool CRepo::Download()
{
	if (!initDir())
	{
		return false;
	}
	if (!loadIndex())
	{
		return false;
	}
	if (!StartWith(m_sInputPath, USTR("/")))
	{
		m_sInputPath = USTR("/") + m_sInputPath;
	}
	m_sInputPath = Replace(m_sInputPath, USTR("\\"), USTR("/"));
	if (EndWith(m_sInputPath, USTR("/")))
	{
		m_sInputPath.erase(m_sInputPath.size() - 1);
	}
	string sInputPath = UToU8(m_sInputPath);
	map<string, string>::const_iterator itPathHash = m_mPathHash.find(sInputPath);
	if (itPathHash == m_mPathHash.end())
	{
		UPrintf(USTR("ERROR: %") PRIUS USTR(" does not exists\n\n"), m_sInputPath.c_str());
		return false;
	}
	m_sHash = itPathHash->second;
	if (!generateDataDirPath(false))
	{
		return false;
	}
	string sRemote;
	if (!readTextFile(m_sDataRemoteFilePath, sRemote))
	{
		return false;
	}
	vector<pair<string, string>> vTypeWorkspace;
	vector<string> vRemote = SplitOf(sRemote, "\r\n");
	for (vector<string>::const_iterator it = vRemote.begin(); it != vRemote.end(); ++it)
	{
		const string& sLine = *it;
		if (sLine.empty())
		{
			continue;
		}
		vector<string> vLine = Split(sLine, "\t");
		if (vLine.size() != 2)
		{
			UPrintf(USTR("ERROR: parse remote %") PRIUS USTR(" failed\n\n"), U8ToU(sLine).c_str());
			return false;
		}
		const string& sType = vLine[0];
		const string& sWorkspace = vLine[1];
		if (!m_sType.empty())
		{
			if (!m_sWorkspace.empty())
			{
				if (sType == m_sType && sWorkspace == m_sWorkspace)
				{
					vTypeWorkspace.push_back(make_pair(sType, sWorkspace));
				}
			}
			else
			{
				if (sType == m_sType)
				{
					vTypeWorkspace.push_back(make_pair(sType, sWorkspace));
				}
			}
		}
		else
		{
			if (!m_sWorkspace.empty())
			{
				if (sWorkspace == m_sWorkspace)
				{
					vTypeWorkspace.push_back(make_pair(sType, sWorkspace));
				}
			}
			else
			{
				vTypeWorkspace.push_back(make_pair(sType, sWorkspace));
			}
		}
	}
	if (vTypeWorkspace.empty())
	{
		UPrintf(USTR("ERROR: no remote for %") PRIUS USTR("\n\n"), m_sInputPath.c_str());
		return false;
	}
	string sDataIndex;
	if (!readTextFile(m_sDataIndexFilePath, sDataIndex))
	{
		return false;
	}
	vector<string> vIndex = SplitOf(sDataIndex, "\r\n");
	n32 nRepoCount = 0;
	for (vector<string>::const_reverse_iterator it = vIndex.rbegin(); it != vIndex.rend(); ++it)
	{
		const string& sLine = *it;
		if (sLine.empty())
		{
			continue;
		}
		SPath path;
		if (!path.Parse(sLine))
		{
			UPrintf(USTR("ERROR: parse index %") PRIUS USTR(" failed\n\n"), U8ToU(sLine).c_str());
			return false;
		}
		n64 nFileSize = path.Size;
		n64 nRepoSize = path.RepoDataV1.RepoSize;
		n32 nRepoIndex = path.RepoDataV1.RepoIndex;
		n32 nRepoIndex2 = path.RepoDataV1.RepoIndex2;
		while (nFileSize > 0)
		{
			if (nRepoSize >= s_nRepoSizeMax)
			{
				nRepoSize = 0;
				nRepoIndex++;
				nRepoIndex2 = 0;
			}
			n64 nSize = nFileSize > s_nRepoFileSizeMax ? s_nRepoFileSizeMax : nFileSize;
			nFileSize -= nSize;
			nRepoSize += nSize;
			nRepoIndex2++;
		}
		nRepoCount = nRepoIndex + 1;
		break;
	}
	for (n32 nRepoIndex = 0; nRepoIndex < nRepoCount; nRepoIndex++)
	{
		if (!generateRepoDirPath(nRepoIndex, true))
		{
			return false;
		}
		if (UChdir(m_sRepoDirPath.c_str()) != 0)
		{
			UPrintf(USTR("ERROR: change dir %") PRIUS USTR(" failed\n\n"), m_sRepoDirPath.c_str());
			return false;
		}
		if (!gitInitMaster())
		{
			return false;
		}
		bool bResult = false;
		for (vector<pair<string, string>>::const_iterator itTypeWorkspace = vTypeWorkspace.begin(); itTypeWorkspace != vTypeWorkspace.end(); ++itTypeWorkspace)
		{
			m_sType = itTypeWorkspace->first;
			m_sWorkspace = itTypeWorkspace->second;
			m_sRemoteName = m_sType + "_" + m_sWorkspace;
			UString sRemoteTempFileName = U8ToU(m_sRemoteName + ".txt");
			UString sRemoteTempFilePath = s_sTempDirName + USTR("/") + sRemoteTempFileName;
			string sRemoteTemp;
			FILE* fp = UFopen(sRemoteTempFilePath, USTR("rb"), false);
			if (fp != nullptr)
			{
				fclose(fp);
				if (!readTextFile(sRemoteTempFilePath, sRemoteTemp))
				{
					continue;
				}
			}
			set<string> sRemoteInit;
			vector<string> vRemoteTemp = SplitOf(sRemoteTemp, "\r\n");
			for (vector<string>::const_iterator it = vRemoteTemp.begin(); it != vRemoteTemp.end(); ++it)
			{
				const string& sLine = *it;
				if (!sLine.empty())
				{
					sRemoteInit.insert(sLine);
				}
			}
			string sRemoteURL;
			if (m_sType == CBitbucket::s_sTypeName)
			{
				CBitbucket bitbucket;
				bitbucket.SetWorkspace(m_sWorkspace);
				bitbucket.SetRepoName(m_sRepoName);
				bitbucket.SetVerbose(m_bVerbose);
				sRemoteURL = bitbucket.GetRepoRemoteHttpsURL();
			}
			else if (m_sType == CGithub::s_sTypeName)
			{
				CGithub github;
				github.SetWorkspace(m_sWorkspace);
				github.SetRepoName(m_sRepoName);
				github.SetVerbose(m_bVerbose);
				sRemoteURL = github.GetRepoRemoteHttpsURL();
			}
			if (sRemoteInit.find(s_sRemoteInitGitRemoteAdd) == sRemoteInit.end())
			{
				gitRemoteAdd(sRemoteURL);
				sRemoteInit.insert(s_sRemoteInitGitRemoteAdd);
				sRemoteTemp += s_sRemoteInitGitRemoteAdd + "\n";
				writeTextFile(sRemoteTempFilePath, sRemoteTemp);
			}
			if (!gitPull(sRemoteURL, false))
			{
				continue;
			}
			bResult = true;
			break;
		}
		if (!bResult)
		{
			UPrintf(USTR("ERROR: download %") PRIUS USTR(" failed\n\n"), U8ToU(m_sRepoName).c_str());
			return false;
		}
	}
	return true;
}

bool CRepo::Import()
{
	string sImportKey = FGenerateKey(m_sImportKey);
	string sDecryptedImportParam = FDecryptString(m_sImportParam, sImportKey);
	string sFromType;
	string sFromWorkspace;
	string sFromRepoName;
	string sFromUser;
	string sFromPassword;
	string sToType;
	string sToWorkspace;
	string sToRepoName;
	string sToUser;
	string sToPassword;
	string sImportRecorderWorkspace;
	string sImportRecorderRepoName;
	string sImportRecorderUser;
	string sImportRecorderPersonalAccessToken;
	vector<string> vParam = Split(sDecryptedImportParam, " ");
	for (vector<string>::const_iterator it = vParam.begin(); it != vParam.end(); ++it)
	{
		const string& sParam = *it;
		string::size_type uPos = sParam.find("=");
		if (uPos == string::npos)
		{
			UPrintf(USTR("ERROR: import param format error\n\n"));
			return false;
		}
		string sParamKey = sParam.substr(0, uPos);
		string sParamValue = sParam.substr(uPos + 1);
		if (sParamKey == s_sImportParamFromType)
		{
			sFromType = sParamValue;
		}
		else if (sParamKey == s_sImportParamFromWorkspace)
		{
			sFromWorkspace = sParamValue;
		}
		else if (sParamKey == s_sImportParamFromRepoName)
		{
			sFromRepoName = sParamValue;
		}
		else if (sParamKey == s_sImportParamFromUser)
		{
			sFromUser = sParamValue;
		}
		else if (sParamKey == s_sImportParamFromPassword)
		{
			sFromPassword = sParamValue;
		}
		else if (sParamKey == s_sImportParamToType)
		{
			sToType = sParamValue;
		}
		else if (sParamKey == s_sImportParamToWorkspace)
		{
			sToWorkspace = sParamValue;
		}
		else if (sParamKey == s_sImportParamToRepoName)
		{
			sToRepoName = sParamValue;
		}
		else if (sParamKey == s_sImportParamToUser)
		{
			sToUser = sParamValue;
		}
		else if (sParamKey == s_sImportParamToPassword)
		{
			sToPassword = sParamValue;
		}
		else if (sParamKey == CGithub::s_sConfigKeyImportRecorderWorkspace)
		{
			sImportRecorderWorkspace = sParamValue;
		}
		else if (sParamKey == CGithub::s_sConfigKeyImportRecorderRepoName)
		{
			sImportRecorderRepoName = sParamValue;
		}
		else if (sParamKey == CGithub::s_sConfigKeyImportRecorderUser)
		{
			sImportRecorderUser = sParamValue;
		}
		else if (sParamKey == CGithub::s_sConfigKeyImportRecorderPersonalAccessToken)
		{
			sImportRecorderPersonalAccessToken = sParamValue;
		}
		else
		{
			UPrintf(USTR("ERROR: import param has unknown key\n\n"));
			return false;
		}
	}
	if (sFromType != CBitbucket::s_sTypeName && sFromType != CGithub::s_sTypeName)
	{
		UPrintf(USTR("ERROR: unknown %") PRIUS USTR("\n\n"), U8ToU(s_sImportParamFromType).c_str());
		return false;
	}
	if (sFromWorkspace.empty())
	{
		UPrintf(USTR("ERROR: %") PRIUS USTR(" is empty\n\n"), U8ToU(s_sImportParamFromWorkspace).c_str());
		return false;
	}
	if (sFromRepoName.empty())
	{
		UPrintf(USTR("ERROR: %") PRIUS USTR(" is empty\n\n"), U8ToU(s_sImportParamFromRepoName).c_str());
		return false;
	}
	if (sToType != CBitbucket::s_sTypeName && sToType != CGithub::s_sTypeName)
	{
		UPrintf(USTR("ERROR: unknown %") PRIUS USTR("\n\n"), U8ToU(s_sImportParamToType).c_str());
		return false;
	}
	if (sToWorkspace.empty())
	{
		UPrintf(USTR("ERROR: %") PRIUS USTR(" is empty\n\n"), U8ToU(s_sImportParamToWorkspace).c_str());
		return false;
	}
	if (sToRepoName.empty())
	{
		UPrintf(USTR("ERROR: %") PRIUS USTR(" is empty\n\n"), U8ToU(s_sImportParamToRepoName).c_str());
		return false;
	}
	if (sToUser.empty())
	{
		UPrintf(USTR("ERROR: %") PRIUS USTR(" is empty\n\n"), U8ToU(s_sImportParamToUser).c_str());
		return false;
	}
	if (sToPassword.empty())
	{
		UPrintf(USTR("ERROR: %") PRIUS USTR(" is empty\n\n"), U8ToU(s_sImportParamToPassword).c_str());
		return false;
	}
	if (sImportRecorderWorkspace.empty())
	{
		UPrintf(USTR("ERROR: %") PRIUS USTR(" is empty\n\n"), U8ToU(CGithub::s_sConfigKeyImportRecorderWorkspace).c_str());
		return false;
	}
	if (sImportRecorderRepoName.empty())
	{
		UPrintf(USTR("ERROR: %") PRIUS USTR(" is empty\n\n"), U8ToU(CGithub::s_sConfigKeyImportRecorderRepoName).c_str());
		return false;
	}
	if (sImportRecorderUser.empty())
	{
		UPrintf(USTR("ERROR: %") PRIUS USTR(" is empty\n\n"), U8ToU(CGithub::s_sConfigKeyImportRecorderUser).c_str());
		return false;
	}
	if (sImportRecorderPersonalAccessToken.empty())
	{
		UPrintf(USTR("ERROR: %") PRIUS USTR(" is empty\n\n"), U8ToU(CGithub::s_sConfigKeyImportRecorderPersonalAccessToken).c_str());
		return false;
	}
	string sRemoteURL;
	if (sFromType == CBitbucket::s_sTypeName)
	{
		CBitbucket bitbucket;
		bitbucket.SetWorkspace(sFromWorkspace);
		bitbucket.SetRepoName(sFromRepoName);
		bitbucket.SetVerbose(m_bVerbose);
		if (!sFromUser.empty() && !sFromPassword.empty())
		{
			bitbucket.SetUser(sFromUser);
			bitbucket.SetAppPassword(sFromPassword);
			sRemoteURL = bitbucket.GetRepoPushHttpsURL();
		}
		else
		{
			sRemoteURL = bitbucket.GetRepoRemoteHttpsURL();
		}
	}
	else if (sFromType == CGithub::s_sTypeName)
	{
		CGithub github;
		github.SetWorkspace(sFromWorkspace);
		github.SetRepoName(sFromRepoName);
		github.SetVerbose(m_bVerbose);
		if (!sFromUser.empty() && !sFromPassword.empty())
		{
			github.SetUser(sFromUser);
			github.SetPersonalAccessToken(sFromPassword);
			sRemoteURL = github.GetRepoPushHttpsURL();
		}
		else
		{
			sRemoteURL = github.GetRepoRemoteHttpsURL();
		}
	}
	if (sToType == CBitbucket::s_sTypeName)
	{
		CBitbucket bitbucket;
		bitbucket.SetWorkspace(sToWorkspace);
		bitbucket.SetRepoName(sToRepoName);
		bitbucket.SetUser(sToUser);
		bitbucket.SetAppPassword(sToPassword);
		bitbucket.SetVerbose(m_bVerbose);
		m_sRepoPushURL = bitbucket.GetRepoPushHttpsURL();
	}
	else if (sToType == CGithub::s_sTypeName)
	{
		CGithub github;
		github.SetWorkspace(sToWorkspace);
		github.SetRepoName(sToRepoName);
		github.SetUser(sToUser);
		github.SetPersonalAccessToken(sToPassword);
		github.SetVerbose(m_bVerbose);
		m_sRepoPushURL = github.GetRepoPushHttpsURL();
	}
	if (!UMakeDir(s_sTempDirName))
	{
		UPrintf(USTR("ERROR: make dir %") PRIUS USTR(" failed\n\n"), s_sTempDirName.c_str());
		return false;
	}
	if (UChdir(s_sTempDirName.c_str()) != 0)
	{
		UPrintf(USTR("ERROR: change dir %") PRIUS USTR(" failed\n\n"), s_sTempDirName.c_str());
		return false;
	}
	bool bResult = true;
	do
	{
		if (!gitInitMaster())
		{
			break;
		}
		if (!gitPull(sRemoteURL, true))
		{
			break;
		}
		if (!gitPush(true))
		{
			break;
		}
	} while (false);
	UChdir(USTR(".."));
	if (!bResult)
	{
		return false;
	}
	CGithub github;
	github.SetWorkspace(sImportRecorderWorkspace);
	github.SetRepoName(sImportRecorderRepoName);
	github.SetUser(sImportRecorderUser);
	github.SetPersonalAccessToken(sImportRecorderPersonalAccessToken);
	github.SetVerbose(m_bVerbose);
	string sImportRecorderFilePath = generateImportRecorderFilePath(sToType, sToWorkspace, sToRepoName);
	bResult = github.CreateEmptyFile(sImportRecorderFilePath);
	if (!bResult)
	{
		UPrintf(USTR("ERROR: create empty file %") PRIUS USTR(" failed\n\n"), U8ToU(sImportRecorderFilePath).c_str());
		return false;
	}
	return true;
}

string CRepo::generateImportRecorderFilePath(const string& a_sType, const string& a_sWorkspace, const string& a_sRepoName)
{
	string sImportRecorderFilePath = "data/" + a_sType + "/" + a_sWorkspace + "/" + a_sRepoName + ".txt";
	return sImportRecorderFilePath;
}

bool CRepo::initDir()
{
	m_sRootDirPath = UGetModuleDirName();
	const u32 uMaxPath = 4096;
	m_sCurrentWorkingDirPath.resize(uMaxPath, 0);
	if (UGetcwd(&*m_sCurrentWorkingDirPath.begin(), uMaxPath) == nullptr)
	{
		UPrintf(USTR("ERROR: getcwd error\n\n"));
		return false;
	}
	m_sCurrentWorkingDirPath.resize(UCslen(m_sCurrentWorkingDirPath.c_str()));
	m_sIndexFilePath = m_sRootDirPath + USTR("/") + U8ToU(s_sIndexFileName);
	return true;
}

bool CRepo::generateDataDirPath(bool a_bMakeDir)
{
	m_sDataDirPath = m_sRootDirPath + USTR("/") + U8ToU(s_sDataDirName) + USTR("/") + U8ToU(m_sHash);
	if (a_bMakeDir)
	{
		if (!UMakeDir(m_sDataDirPath))
		{
			UPrintf(USTR("ERROR: make dir %") PRIUS USTR(" failed\n\n"), m_sDataDirPath.c_str());
			return false;
		}
	}
	m_sDataIndexFilePath = m_sDataDirPath + USTR("/") + U8ToU(s_sIndexFileName);
	m_sDataRemoteFilePath = m_sDataDirPath + USTR("/") + s_sRemoteFileName;
	m_sDataCRC32FilePath = m_sDataDirPath + USTR("/") + U8ToU(s_sCRC32FileName);
	m_sDataMD5FilePath = m_sDataDirPath + USTR("/") + U8ToU(s_sMD5FileName);
	m_sDataSHA1FilePath = m_sDataDirPath + USTR("/") + U8ToU(s_sSHA1FileName);
	m_sDataSHA256FilePath = m_sDataDirPath + USTR("/") + U8ToU(s_sSHA256FileName);
	return true;
}

bool CRepo::generateRepoDirPath(n32 a_nRepoIndex, bool a_bMakeDir)
{
	m_sRepoName = Format("%s%08d", m_sHash.c_str(), a_nRepoIndex);
	m_sRepoDirPath = m_sRootDirPath + USTR("/") + s_sRepoDirName + USTR("/") + U8ToU(m_sRepoName);
	if (a_bMakeDir)
	{
		if (!UMakeDir(m_sRepoDirPath))
		{
			UPrintf(USTR("ERROR: make dir %") PRIUS USTR(" failed\n\n"), m_sRepoDirPath.c_str());
			return false;
		}
	}
	m_sRepoContinuousHashDirPath = m_sRepoDirPath + USTR("/") + U8ToU(s_sContinuousHashDirName);
	if (a_bMakeDir)
	{
		if (!UMakeDir(m_sRepoContinuousHashDirPath))
		{
			UPrintf(USTR("ERROR: make dir %") PRIUS USTR(" failed\n\n"), m_sRepoContinuousHashDirPath.c_str());
			return false;
		}
	}
	m_sRepoDataDirPath = m_sRepoDirPath + USTR("/") + U8ToU(s_sDataDirName);
	if (a_bMakeDir)
	{
		if (!UMakeDir(m_sRepoDataDirPath))
		{
			UPrintf(USTR("ERROR: make dir %") PRIUS USTR(" failed\n\n"), m_sRepoDataDirPath.c_str());
			return false;
		}
	}
	m_sRepoTempDirPath = m_sRepoDirPath + USTR("/") + s_sTempDirName;
	UMakeDir(m_sRepoTempDirPath);
	m_sRepoReadmeFilePath = m_sRepoDirPath + USTR("/") + U8ToU(s_sReadmeFileName);
	m_sRepoGitAttributesFilePath = m_sRepoDirPath + USTR("/") + U8ToU(s_sGitAttributesFileName);
	m_sRepoGitIgnoreFilePath = m_sRepoDirPath + USTR("/") + U8ToU(s_sGitIgnoreFileName);
	m_sRepoIndexFilePath = m_sRepoDirPath + USTR("/") + U8ToU(s_sIndexFileName);
	m_sRepoContinuousCRC32FilePath = m_sRepoContinuousHashDirPath + USTR("/") + U8ToU(s_sCRC32FileName);
	m_sRepoContinuousMD5FilePath = m_sRepoContinuousHashDirPath + USTR("/") + U8ToU(s_sMD5FileName);
	m_sRepoContinuousSHA1FilePath = m_sRepoContinuousHashDirPath + USTR("/") + U8ToU(s_sSHA1FileName);
	m_sRepoContinuousSHA256FilePath = m_sRepoContinuousHashDirPath + USTR("/") + U8ToU(s_sSHA256FileName);
	m_sRepoCRC32FilePath = m_sRepoDirPath + USTR("/") + U8ToU(s_sCRC32FileName);
	m_sRepoMD5FilePath = m_sRepoDirPath + USTR("/") + U8ToU(s_sMD5FileName);
	m_sRepoSHA1FilePath = m_sRepoDirPath + USTR("/") + U8ToU(s_sSHA1FileName);
	m_sRepoSHA256FilePath = m_sRepoDirPath + USTR("/") + U8ToU(s_sSHA256FileName);
	return true;
}

bool CRepo::generateRepoDataFilePath(n32 a_nRepoIndex2, bool a_bMakeDir)
{
	static n32 c_nIndex[5] = {};
	c_nIndex[0] = a_nRepoIndex2 / 100000000;
	c_nIndex[1] = a_nRepoIndex2 / 1000000 % 100;
	c_nIndex[2] = a_nRepoIndex2 / 10000 % 100;
	c_nIndex[3] = a_nRepoIndex2 / 100 % 100;
	c_nIndex[4] = a_nRepoIndex2 % 100;
	static const string c_sRepoDataDirName = s_sDataDirName;
	m_sRepoDataFileRelativePath = c_sRepoDataDirName;
	for (n32 i = 0; i < SDW_ARRAY_COUNT(c_nIndex); i++)
	{
		m_sRepoDataFileRelativePath += Format("/%02d", c_nIndex[i]);
	}
	m_sRepoDataFileRelativePath += ".bin";
	static UString c_sDirPath[4] = {};
	c_sDirPath[0] = m_sRepoDataDirPath + Format(USTR("/%02d"), c_nIndex[0]);
	c_sDirPath[1] = c_sDirPath[0] + Format(USTR("/%02d"), c_nIndex[1]);
	c_sDirPath[2] = c_sDirPath[1] + Format(USTR("/%02d"), c_nIndex[2]);
	c_sDirPath[3] = c_sDirPath[2] + Format(USTR("/%02d"), c_nIndex[3]);
	m_sRepoDataFileLocalPath = c_sDirPath[3] + Format(USTR("/%02d.bin"), c_nIndex[4]);
	if (a_bMakeDir)
	{
		if (c_nIndex[4] == 0)
		{
			if (c_nIndex[3] == 0)
			{
				if (c_nIndex[2] == 0)
				{
					if (c_nIndex[1] == 0)
					{
						if (!UMakeDir(c_sDirPath[0]))
						{
							UPrintf(USTR("ERROR: make dir %") PRIUS USTR(" failed\n\n"), c_sDirPath[0].c_str());
							return false;
						}
					}
					if (!UMakeDir(c_sDirPath[1]))
					{
						UPrintf(USTR("ERROR: make dir %") PRIUS USTR(" failed\n\n"), c_sDirPath[1].c_str());
						return false;
					}
				}
				if (!UMakeDir(c_sDirPath[2]))
				{
					UPrintf(USTR("ERROR: make dir %") PRIUS USTR(" failed\n\n"), c_sDirPath[2].c_str());
					return false;
				}
			}
			if (!UMakeDir(c_sDirPath[3]))
			{
				UPrintf(USTR("ERROR: make dir %") PRIUS USTR(" failed\n\n"), c_sDirPath[3].c_str());
				return false;
			}
		}
	}
	return true;
}

bool CRepo::writeTextFile(const UString& a_sPath, const string& a_sText) const
{
	FILE* fp = UFopen(a_sPath, USTR("wb"));
	if (fp == nullptr)
	{
		return false;
	}
	fwrite(a_sText.c_str(), 1, a_sText.size(), fp);
	fclose(fp);
	return true;
}

bool CRepo::readTextFile(const UString& a_sPath, string& a_sText) const
{
	FILE* fp = UFopen(a_sPath, USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	fseek(fp, 0, SEEK_END);
	u32 uFileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* pTemp = new char[uFileSize + 1];
	fread(pTemp, 1, uFileSize, fp);
	fclose(fp);
	pTemp[uFileSize] = 0;
	a_sText = pTemp;
	delete[] pTemp;
	return true;
}

bool CRepo::loadConfig()
{
	UString sConfigFilePath = m_sRootDirPath + USTR("/config.txt");
	string sConfig;
	if (!readTextFile(sConfigFilePath, sConfig))
	{
		return false;
	}
	vector<string> vConfig = SplitOf(sConfig, "\r\n");
	for (vector<string>::const_iterator it = vConfig.begin(); it != vConfig.end(); ++it)
	{
		const string& sLine = *it;
		if (sLine.empty())
		{
			continue;
		}
		vector<string> vLine = Split(sLine, "\t");
		if (vLine.size() != 2)
		{
			UPrintf(USTR("ERROR: parse config %") PRIUS USTR(" failed\n\n"), U8ToU(sLine).c_str());
			return false;
		}
		const string& sKey = vLine[0];
		const string& sValue = vLine[1];
		if (!m_mConfig.insert(make_pair(sKey, sValue)).second)
		{
			UPrintf(USTR("ERROR: key %") PRIUS USTR(" already exists\n\n"), U8ToU(sKey).c_str());
			return false;
		}
	}
	if (m_mConfig.find(s_sConfigKeyGitUserName) == m_mConfig.end())
	{
		UPrintf(USTR("ERROR: config key %") PRIUS USTR(" does not exists\n\n"), U8ToU(s_sConfigKeyGitUserName).c_str());
		return false;
	}
	if (m_mConfig.find(s_sConfigKeyGitUserEmail) == m_mConfig.end())
	{
		UPrintf(USTR("ERROR: config key %") PRIUS USTR(" does not exists\n\n"), U8ToU(s_sConfigKeyGitUserEmail).c_str());
		return false;
	}
	if (m_sType == CBitbucket::s_sTypeName)
	{
		if (m_mConfig.find(CBitbucket::s_sConfigKeyProjectName) == m_mConfig.end())
		{
			UPrintf(USTR("ERROR: config key %") PRIUS USTR(" does not exists\n\n"), U8ToU(CBitbucket::s_sConfigKeyProjectName).c_str());
			return false;
		}
		if (m_mConfig.find(CBitbucket::s_sConfigKeyProjectKey) == m_mConfig.end())
		{
			UPrintf(USTR("ERROR: config key %") PRIUS USTR(" does not exists\n\n"), U8ToU(CBitbucket::s_sConfigKeyProjectKey).c_str());
			return false;
		}
	}
	return true;
}

bool CRepo::loadGithubImporterSecretConfig()
{
	UString sGithubImporterConfigFilePath = m_sRootDirPath + USTR("/secret_config_github_importer.txt");
	string sGithubImporterConfig;
	if (!readTextFile(sGithubImporterConfigFilePath, sGithubImporterConfig))
	{
		return false;
	}
	vector<string> vGithubImporterConfig = SplitOf(sGithubImporterConfig, "\r\n");
	for (vector<string>::const_iterator it = vGithubImporterConfig.begin(); it != vGithubImporterConfig.end(); ++it)
	{
		const string& sLine = *it;
		if (sLine.empty())
		{
			continue;
		}
		vector<string> vLine = Split(sLine, "\t");
		if (vLine.size() != 2)
		{
			UPrintf(USTR("ERROR: parse github importer secret config failed\n\n"));
			return false;
		}
		const string& sKey = vLine[0];
		const string& sValue = vLine[1];
		if (!m_mConfig.insert(make_pair(sKey, sValue)).second)
		{
			UPrintf(USTR("ERROR: secret key %") PRIUS USTR(" already exists\n\n"), U8ToU(sKey).c_str());
			return false;
		}
	}
	if (m_mConfig.find(CGithub::s_sConfigKeyImportRecorderWorkspace) == m_mConfig.end())
	{
		UPrintf(USTR("ERROR: github importer secret config key %") PRIUS USTR(" does not exists\n\n"), U8ToU(CGithub::s_sConfigKeyImportRecorderWorkspace).c_str());
		return false;
	}
	if (m_mConfig.find(CGithub::s_sConfigKeyImportRecorderRepoName) == m_mConfig.end())
	{
		UPrintf(USTR("ERROR: github importer secret config key %") PRIUS USTR(" does not exists\n\n"), U8ToU(CGithub::s_sConfigKeyImportRecorderRepoName).c_str());
		return false;
	}
	if (m_mConfig.find(CGithub::s_sConfigKeyImportRecorderUser) == m_mConfig.end())
	{
		UPrintf(USTR("ERROR: github importer secret config key %") PRIUS USTR(" does not exists\n\n"), U8ToU(CGithub::s_sConfigKeyImportRecorderUser).c_str());
		return false;
	}
	if (m_mConfig.find(CGithub::s_sConfigKeyImportRecorderPersonalAccessToken) == m_mConfig.end())
	{
		UPrintf(USTR("ERROR: github importer secret config key %") PRIUS USTR(" does not exists\n\n"), U8ToU(CGithub::s_sConfigKeyImportRecorderPersonalAccessToken).c_str());
		return false;
	}
	if (m_mConfig.find(CGithub::s_sConfigKeyImporterWorkspace) == m_mConfig.end())
	{
		UPrintf(USTR("ERROR: github importer secret config key %") PRIUS USTR(" does not exists\n\n"), U8ToU(CGithub::s_sConfigKeyImporterWorkspace).c_str());
		return false;
	}
	if (m_mConfig.find(CGithub::s_sConfigKeyImporterRepoName) == m_mConfig.end())
	{
		UPrintf(USTR("ERROR: github importer secret config key %") PRIUS USTR(" does not exists\n\n"), U8ToU(CGithub::s_sConfigKeyImporterRepoName).c_str());
		return false;
	}
	if (m_mConfig.find(CGithub::s_sConfigKeyImporterWorkflowFileName) == m_mConfig.end())
	{
		UPrintf(USTR("ERROR: github importer secret config key %") PRIUS USTR(" does not exists\n\n"), U8ToU(CGithub::s_sConfigKeyImporterWorkflowFileName).c_str());
		return false;
	}
	if (m_mConfig.find(CGithub::s_sConfigKeyImporterUser) == m_mConfig.end())
	{
		UPrintf(USTR("ERROR: github importer secret config key %") PRIUS USTR(" does not exists\n\n"), U8ToU(CGithub::s_sConfigKeyImporterUser).c_str());
		return false;
	}
	if (m_mConfig.find(CGithub::s_sConfigKeyImporterPersonalAccessToken) == m_mConfig.end())
	{
		UPrintf(USTR("ERROR: github importer secret config key %") PRIUS USTR(" does not exists\n\n"), U8ToU(CGithub::s_sConfigKeyImporterPersonalAccessToken).c_str());
		return false;
	}
	if (m_mConfig.find(CGithub::s_sConfigKeyImporterImportKey) == m_mConfig.end())
	{
		UPrintf(USTR("ERROR: github importer secret config key %") PRIUS USTR(" does not exists\n\n"), U8ToU(CGithub::s_sConfigKeyImporterImportKey).c_str());
		return false;
	}
	m_sImportKey = m_mConfig[CGithub::s_sConfigKeyImporterImportKey];
	return true;
}

bool CRepo::loadUser()
{
	UString sUserFilePath = m_sRootDirPath + USTR("/user.txt");
	string sUser;
	if (!readTextFile(sUserFilePath, sUser))
	{
		return false;
	}
	vector<string> vUser = SplitOf(sUser, "\r\n");
	for (vector<string>::const_iterator it = vUser.begin(); it != vUser.end(); ++it)
	{
		const string& sLine = *it;
		if (sLine.empty())
		{
			continue;
		}
		vector<string> vLine = Split(sLine, "\t");
		const string& sType = vLine[0];
		if (sType == CBitbucket::s_sTypeName && m_sType == sType)
		{
			SBitbucketUser user;
			if (!user.Parse(vLine))
			{
				UPrintf(USTR("ERROR: parse user %") PRIUS USTR(" failed\n\n"), U8ToU(sLine).c_str());
				return false;
			}
			if (user.User == m_sUser)
			{
				m_sPassword = user.AppPassword;
				break;
			}
		}
		else if (sType == CGithub::s_sTypeName && m_sType == sType)
		{
			SGithubUser user;
			if (!user.Parse(vLine))
			{
				UPrintf(USTR("ERROR: parse user %") PRIUS USTR(" failed\n\n"), U8ToU(sLine).c_str());
				return false;
			}
			if (user.User == m_sUser)
			{
				m_sPassword = user.PersonalAccessToken;
				break;
			}
		}
	}
	if (m_sPassword.empty())
	{
		UPrintf(USTR("ERROR: user %") PRIUS USTR(" does not exists\n\n"), U8ToU(m_sUser).c_str());
		return false;
	}
	return true;
}

bool CRepo::writeIndex() const
{
	FILE* fp = UFopen(m_sIndexFilePath, USTR("wb"));
	if (fp == nullptr)
	{
		return false;
	}
	for (map<string, string>::const_iterator it = m_mPathHash.begin(); it != m_mPathHash.end(); ++it)
	{
		const string& sPath = it->first;
		const string& sHash = it->second;
		fprintf(fp, "%s\t%s\n", sHash.c_str(), sPath.c_str());
	}
	fclose(fp);
	return true;
}

bool CRepo::loadIndex()
{
	FILE* fp = UFopen(m_sIndexFilePath, USTR("rb"), false);
	if (fp == nullptr)
	{
		fp = UFopen(m_sIndexFilePath, USTR("wb"));
		if (fp == nullptr)
		{
			return false;
		}
		fclose(fp);
	}
	else
	{
		fclose(fp);
		string sIndex;
		if (!readTextFile(m_sIndexFilePath, sIndex))
		{
			return false;
		}
		vector<string> vIndex = SplitOf(sIndex, "\r\n");
		for (vector<string>::const_iterator it = vIndex.begin(); it != vIndex.end(); ++it)
		{
			const string& sLine = *it;
			if (sLine.empty())
			{
				continue;
			}
			vector<string> vLine = Split(sLine, "\t");
			if (vLine.size() != 2)
			{
				UPrintf(USTR("ERROR: parse index %") PRIUS USTR(" failed\n\n"), U8ToU(sLine).c_str());
				return false;
			}
			const string& sHash = vLine[0];
			const string& sPath = vLine[1];
			if (!m_mPathHash.insert(make_pair(sPath, sHash)).second)
			{
				UPrintf(USTR("ERROR: path %") PRIUS USTR(" already exists\n\n"), U8ToU(sPath).c_str());
				return false;
			}
			if (!m_mHashPath.insert(make_pair(sHash, sPath)).second)
			{
				UPrintf(USTR("ERROR: hash %") PRIUS USTR(" already exists\n\n"), U8ToU(sHash).c_str());
				return false;
			}
		}
	}
	UString sDataDirPath = m_sRootDirPath + USTR("/") + U8ToU(s_sDataDirName);
	if (!UMakeDir(sDataDirPath))
	{
		UPrintf(USTR("ERROR: make dir %") PRIUS USTR(" failed\n\n"), sDataDirPath.c_str());
		return false;
	}
	UString sRepoDirPath = m_sRootDirPath + USTR("/") + s_sRepoDirName;
	if (!UMakeDir(sRepoDirPath))
	{
		UPrintf(USTR("ERROR: make dir %") PRIUS USTR(" failed\n\n"), sRepoDirPath.c_str());
		return false;
	}
	return true;
}

bool CRepo::packV1File(const SPath& a_Path)
{
	UString sOutputPath = m_sOutputPath + m_sSeperator + Replace(a_Path.PathU, USTR("/"), m_sSeperator);
	if (a_Path.IsDir)
	{
		if (m_bVerbose)
		{
			UPrintf(USTR("\t>\t%") PRIUS USTR("\n"), a_Path.PathU.c_str());
		}
		if (!UMakeDir(sOutputPath))
		{
			UPrintf(USTR("ERROR: make dir %") PRIUS USTR(" failed\n\n"), sOutputPath.c_str());
			return false;
		}
	}
	else
	{
		if (a_Path.PathU == USTR("."))
		{
			sOutputPath = m_sOutputPath;
		}
		FILE* fpOutput = UFopen(sOutputPath, USTR("wb"));
		if (fpOutput == nullptr)
		{
			return false;
		}
		n64 nFileSize = a_Path.Size;
		if (nFileSize == 0)
		{
			fclose(fpOutput);
			if (m_bVerbose)
			{
				UPrintf(USTR("\t>\t%") PRIUS USTR("\n"), a_Path.PathU.c_str());
			}
		}
		n64 nRepoSize = a_Path.RepoDataV1.RepoSize;
		n32 nRepoIndex = a_Path.RepoDataV1.RepoIndex;
		n32 nRepoIndex2 = a_Path.RepoDataV1.RepoIndex2;
		if (!generateRepoDirPath(nRepoIndex, false))
		{
			fclose(fpOutput);
			return false;
		}
		static u8 c_uBuffer[s_nRepoFileSizeMax] = {};
		while (nFileSize > 0)
		{
			if (nRepoSize >= s_nRepoSizeMax)
			{
				nRepoSize = 0;
				nRepoIndex++;
				nRepoIndex2 = 0;
				if (!generateRepoDirPath(nRepoIndex, false))
				{
					fclose(fpOutput);
					return false;
				}
			}
			if (!generateRepoDataFilePath(nRepoIndex2, false))
			{
				fclose(fpOutput);
				return false;
			}
			if (m_bVerbose)
			{
				UPrintf(USTR("%") PRIUS USTR("\t>\t%") PRIUS USTR("\n"), m_sRepoDataFileLocalPath.c_str() + m_sRootDirPath.size() + 1, a_Path.PathU.c_str());
			}
			FILE* fpInput = UFopen(m_sRepoDataFileLocalPath, USTR("rb"));
			if (fpInput == nullptr)
			{
				fclose(fpOutput);
				return false;
			}
			n64 nSize = nFileSize > s_nRepoFileSizeMax ? s_nRepoFileSizeMax : nFileSize;
			fread(c_uBuffer, 1, static_cast<size_t>(nSize), fpInput);
			fwrite(c_uBuffer, 1, static_cast<size_t>(nSize), fpOutput);
			fclose(fpInput);
			nFileSize -= nSize;
			nRepoSize += nSize;
			nRepoIndex2++;
		}
		fclose(fpOutput);
	}
	return true;
}

bool CRepo::gitInitMaster() const
{
	// git init -b master from 2.28.0
	if (!gitInit())
	{
		return false;
	}
	if (!writeTextFile(USTR(".git/HEAD"), "ref: refs/heads/master\n"))
	{
		return false;
	}
	if (!gitInit())
	{
		return false;
	}
	return true;
}

bool CRepo::gitInit() const
{
	n32 nResult = system("git init");
	if (nResult != 0)
	{
		UPrintf(USTR("ERROR: git init failed\n\n"));
		return false;
	}
	return true;
}

bool CRepo::gitConfigUser() const
{
	map<string, string>::const_iterator it = m_mConfig.find(s_sConfigKeyGitUserName);
	if (it == m_mConfig.end())
	{
		return false;
	}
	string sUserName = it->second;
	it = m_mConfig.find(s_sConfigKeyGitUserEmail);
	if (it == m_mConfig.end())
	{
		return false;
	}
	string sUserEmail = it->second;
	n32 nResult = system(("git config user.name \"" + sUserName + "\"").c_str());
	if (nResult != 0)
	{
		UPrintf(USTR("ERROR: git config user.name \"%") PRIUS USTR("\" failed\n\n"), U8ToU(sUserName).c_str());
		return false;
	}
	nResult = system(("git config user.email \"" + sUserEmail + "\"").c_str());
	if (nResult != 0)
	{
		UPrintf(USTR("ERROR: git config user.email \"%") PRIUS USTR("\" failed\n\n"), U8ToU(sUserEmail).c_str());
		return false;
	}
	return true;
}

bool CRepo::gitRemoteAdd(const string& a_sRemoteURL) const
{
	n32 nResult = system(("git remote add " + m_sRemoteName + " " + a_sRemoteURL).c_str());
	if (nResult != 0)
	{
		UPrintf(USTR("ERROR: git remote add %") PRIUS USTR(" %") PRIUS USTR(" failed\n\n"), U8ToU(m_sRemoteName).c_str(), U8ToU(a_sRemoteURL).c_str());
		return false;
	}
	return true;
}

bool CRepo::gitAdd(const string& a_sPath) const
{
	n32 nResult = system(("git add " + a_sPath).c_str());
	if (nResult != 0)
	{
		UPrintf(USTR("ERROR: git add %") PRIUS USTR(" failed\n\n"), U8ToU(a_sPath).c_str());
		return false;
	}
	return true;
}

bool CRepo::gitCommit(const string& a_sCommitMessage) const
{
	n32 nAdd = system("git diff-index --quiet HEAD");
	if (nAdd == 0)
	{
		return true;
	}
	n32 nResult = system(("git commit -m \"" + a_sCommitMessage + "\"").c_str());
	if (nResult != 0)
	{
		UPrintf(USTR("ERROR: git commit failed\n\n"));
		return false;
	}
	return true;
}

bool CRepo::gitPush(bool a_bQuiet) const
{
	n32 nResult = 0;
	if (a_bQuiet)
	{
		nResult = system(("git push --quiet -u " + m_sRepoPushURL + " master").c_str());
	}
	else
	{
		nResult = system(("git push -u " + m_sRepoPushURL + " master").c_str());
	}
	if (nResult != 0)
	{
		UPrintf(USTR("ERROR: git push -u %") PRIUS USTR(" master failed\n\n"), U8ToU(m_sRepoPushURL).c_str());
		return false;
	}
	return true;
}

bool CRepo::gitPull(const string& a_sRemoteURL, bool a_bQuiet) const
{
	n32 nResult = 0;
	if (a_bQuiet)
	{
		nResult = system(("git pull --quiet " + a_sRemoteURL + " master").c_str());
	}
	else
	{
		nResult = system(("git pull " + a_sRemoteURL + " master").c_str());
	}
	if (nResult != 0)
	{
		UPrintf(USTR("ERROR: git pull %") PRIUS USTR(" master failed\n\n"), U8ToU(a_sRemoteURL).c_str());
		return false;
	}
	return true;
}
