#ifndef GITHUB_H_
#define GITHUB_H_

#include <sdw.h>

struct SGithubUser
{
	string Type;
	string User;
	string PersonalAccessToken;
	bool Parse(const vector<string>& a_vLine);
};

class CGithub
{
public:
	CGithub();
	~CGithub();
	void SetWorkspace(const string& a_sWorkspace);
	void SetRepoName(const string& a_sRepoName);
	void SetUser(const string& a_sUser);
	void SetPersonalAccessToken(const string& a_sAppPassword);
	void SetVerbose(bool a_bVerbose);
	bool CreateRepo() const;
	bool DeleteRepo() const;
	bool SetActionsPermissions(bool a_bEnabled) const;
	bool GetImportStatus();
	bool StartImportRepo(const string& a_sSourceRemoteURL);
	bool PatchImportRepo();
	bool TriggerWorkflowImport(const string& a_sWorkflowFileName, const string& a_sEncryptedImportParam) const;
	bool CreateEmptyFile(const string& a_sPath) const;
	bool FileExist(const string& a_sPath) const;
	string GetImportStatusString() const;
	string GetRepoRemoteHttpsURL() const;
	string GetRepoPushHttpsURL() const;
	n32 GetAddDataFileSizeMaxCountPerCommit() const;
	static const string s_sTypeName;
	static const string s_sImportStatusImporting;
	static const string s_sImportStatusComplete;
	static const string s_sImportStatusError;
	static const string s_sConfigKeyImportRecorderWorkspace;
	static const string s_sConfigKeyImportRecorderRepoName;
	static const string s_sConfigKeyImportRecorderUser;
	static const string s_sConfigKeyImportRecorderPersonalAccessToken;
	static const string s_sConfigKeyImporterWorkspace;
	static const string s_sConfigKeyImporterRepoName;
	static const string s_sConfigKeyImporterWorkflowFileName;
	static const string s_sConfigKeyImporterUser;
	static const string s_sConfigKeyImporterPersonalAccessToken;
	static const string s_sConfigKeyImporterImportKey;
	static const bool s_bImportNeedDisableActions;
private:
	bool getRepo() const;
	bool getActionsPermissions(bool& a_bEnabled) const;
	bool parseImportResponse(const string& a_sResponse);
	string m_sWorkspace;
	string m_sRepoName;
	string m_sUser;
	string m_sPersonalAccessToken;
	bool m_bVerbose;
	string m_sImportStatusString;
};

#endif	// GITHUB_H_
