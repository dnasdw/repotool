#ifndef REPO_H_
#define REPO_H_

#include <sdw.h>

struct SPath;

struct SRepoIndexData
{
	bool Commit;
	string Sequence;
	string Path;
	bool Parse(const string& a_sLine);
};

class CRepo
{
public:
	enum EHashType
	{
		kHashTypeContinuousCRC32,
		kHashTypeContinuousMD5,
		kHashTypeContinuousSHA1,
		kHashTypeContinuousSHA256,
		kHashTypeCRC32,
		kHashTypeMD5,
		kHashTypeSHA1,
		kHashTypeSHA256,
		kHashTypeCount
	};
	CRepo();
	~CRepo();
	void SetInputPath(const UString& a_sInputPath);
	void SetOutputPath(const UString& a_sOutputPath);
	void SetType(const UString& a_sType);
	void SetWorkspace(const UString& a_sWorkspace);
	void SetUser(const UString& a_sUser);
	void SetUpdateImport(bool a_bUpdateImport);
	void SetImportParam(const string& a_sImportParam);
	void SetImportKey(const string& a_sImportKey);
	void SetRemoveLocalRepo(bool a_bRemoveLocalRepo);
	void SetRemoveRemoteUser(bool a_bRemoveRemoteUser);
	void SetVerbose(bool a_bVerbose);
	bool Unpack();
	bool Pack();
	bool Upload();
	bool Download();
	bool Import();
	bool Remove();
	static const n32 s_nHashCountMax = 10000;
	static const n32 s_nRepoFileSizeMax = 1024 * 1024;
	static const n64 s_nRepoSizeMax = 512 * 1024 * 1024;
	static const string s_sContinuousHashDirName;
	static const string s_sDataDirName;
	static const UString s_sRepoDirName;
	static const UString s_sTempDirName;
	static const string s_sReadmeFileName;
	static const string s_sGitAttributesFileName;
	static const string s_sGitIgnoreFileName;
	static const string s_sIndexFileName;
	static const UString s_sRemoteFileName;
	static const string s_sCRC32FileName;
	static const string s_sMD5FileName;
	static const string s_sSHA1FileName;
	static const string s_sSHA256FileName;
	static const string s_sConfigKeyGitUserName;
	static const string s_sConfigKeyGitUserEmail;
	static const string s_sRemoteInitCreateRemoteRepo;
	static const string s_sRemoteInitGitRemoteAdd;
	static const string s_sRemoteInitImporting;
	static const string s_sRemoteInitImportingComplete;
	static const string s_sImportParamFromType;
	static const string s_sImportParamFromWorkspace;
	static const string s_sImportParamFromRepoName;
	static const string s_sImportParamFromUser;
	static const string s_sImportParamFromPassword;
	static const string s_sImportParamToType;
	static const string s_sImportParamToWorkspace;
	static const string s_sImportParamToRepoName;
	static const string s_sImportParamToUser;
	static const string s_sImportParamToPassword;
private:
	static string generateImportRecorderFilePath(const string& a_sType, const string& a_sWorkspace, const string& a_sRepoName);
	bool initDir();
	bool generateDataDirPath(bool a_bMakeDir);
	void generateRepoName(n32 a_nRepoIndex);
	bool generateRepoDirPath(n32 a_nRepoIndex, bool a_bMakeDir);
	bool generateRepoDataFilePath(n32 a_nRepoIndex2, bool a_bMakeDir);
	bool writeTextFile(const UString& a_sPath, const string& a_sText) const;
	bool readTextFile(const UString& a_sPath, string& a_sText) const;
	bool loadConfig();
	bool loadGithubImporterSecretConfig();
	bool loadUser();
	bool writeIndex() const;
	bool loadIndex();
	bool packV1File(const SPath& a_Path);
	bool gitInitMaster() const;
	bool gitInit() const;
	bool gitConfigUser() const;
	bool gitRemoteAdd(const string& a_sRemoteURL) const;
	bool gitAdd(const string& a_sPath) const;
	bool gitCommit(const string& a_sCommitMessage) const;
	bool gitPush(bool a_bQuiet) const;
	bool gitPull(const string& a_sRemoteURL, bool a_bQuiet) const;
	bool removeDir(const UString& a_sDirPath) const;
	bool removeRemoteUser();
	UString m_sInputPath;
	UString m_sOutputPath;
	string m_sType;
	string m_sWorkspace;
	string m_sUser;
	bool m_bUpdateImport;
	string m_sImportParam;
	string m_sImportKey;
	bool m_bRemoveLocalRepo;
	bool m_bRemoveRemoteUser;
	bool m_bVerbose;
	UString m_sSeperator;
	string m_sRepoName;
	string m_sPassword;
	string m_sRemoteName;
	string m_sRepoPushURL;
	map<string, string> m_mConfig;
	bool m_bHasGithubImporter;
	UString m_sRootDirPath;
	UString m_sCurrentWorkingDirPath;
	UString m_sIndexFilePath;
	map<string, string> m_mPathHash;
	map<string, string> m_mHashPath;
	string m_sHash;
	UString m_sDataDirPath;
	UString m_sDataIndexFilePath;
	UString m_sDataRemoteFilePath;
	UString m_sDataCRC32FilePath;
	UString m_sDataMD5FilePath;
	UString m_sDataSHA1FilePath;
	UString m_sDataSHA256FilePath;
	UString m_sRepoDirPath;
	UString m_sRepoContinuousHashDirPath;
	UString m_sRepoDataDirPath;
	UString m_sRepoTempDirPath;
	UString m_sRepoReadmeFilePath;
	UString m_sRepoGitAttributesFilePath;
	UString m_sRepoGitIgnoreFilePath;
	UString m_sRepoIndexFilePath;
	UString m_sRepoContinuousCRC32FilePath;
	UString m_sRepoContinuousMD5FilePath;
	UString m_sRepoContinuousSHA1FilePath;
	UString m_sRepoContinuousSHA256FilePath;
	UString m_sRepoCRC32FilePath;
	UString m_sRepoMD5FilePath;
	UString m_sRepoSHA1FilePath;
	UString m_sRepoSHA256FilePath;
	string m_sRepoDataFileRelativePath;
	UString m_sRepoDataFileLocalPath;
};

#endif	// REPO_H_
