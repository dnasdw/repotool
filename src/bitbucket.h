#ifndef BITBUCKET_H_
#define BITBUCKET_H_

#include <sdw.h>

struct SBitbucketUser
{
	string Type;
	string User;
	string AppPassword;
	bool Parse(const vector<string>& a_vLine);
};

class CBitbucket
{
public:
	CBitbucket();
	~CBitbucket();
	void SetUser(const string& a_sUser);
	void SetRepoName(const string& a_sRepoName);
	void SetAppPassword(const string& a_sAppPassword);
	void SetProjectName(const string& a_sProjectName);
	void SetProjectKey(const string& a_sProjectKey);
	void SetVerbose(bool a_bVerbose);
	bool CreateRepo();
	string GetRepoRemoteHttpsURL() const;
	string GetRepoPushHttpsURL() const;
	n32 GetAddDataFileSizeMaxCountPerCommit() const;
	static const string s_sTypeName;
	static const string s_sConfigKeyProjectName;
	static const string s_sConfigKeyProjectKey;
private:
	bool createProject();
	bool getProject();
	string m_sUser;
	string m_sRepoName;
	string m_sAppPassword;
	string m_sProjectName;
	string m_sProjectKey;
	bool m_bVerbose;
};

#endif	// BITBUCKET_H_
