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
	void SetSourceRemoteURL(const string& a_sSourceRemoteURL);
	void SetVerbose(bool a_bVerbose);
	bool CreateRepo() const;
	bool GetImportStatus();
	bool StartImportRepo();
	bool PatchImportRepo();
	string GetImportStatusString() const;
	string GetRepoRemoteHttpsURL() const;
	string GetRepoPushHttpsURL() const;
	n32 GetAddDataFileSizeMaxCountPerCommit() const;
	static const string s_sTypeName;
	static const string s_sImportStatusImporting;
	static const string s_sImportStatusComplete;
	static const string s_sImportStatusError;
private:
	bool getRepo() const;
	bool parseImportResponse(const string& a_sResponse);
	string m_sWorkspace;
	string m_sRepoName;
	string m_sUser;
	string m_sPersonalAccessToken;
	string m_sSourceRemoteURL;
	bool m_bVerbose;
	string m_sImportStatusString;
};

#endif	// GITHUB_H_
