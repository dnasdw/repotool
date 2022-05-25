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
	bool CreateRepo();
	string GetRepoRemoteHttpsURL() const;
	string GetRepoPushHttpsURL() const;
	n32 GetAddDataFileSizeMaxCountPerCommit() const;
	static const string s_sTypeName;
private:
	string m_sWorkspace;
	string m_sRepoName;
	string m_sUser;
	string m_sPersonalAccessToken;
	bool m_bVerbose;
};

#endif	// GITHUB_H_
