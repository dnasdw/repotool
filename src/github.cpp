#include "github.h"
#include "curl_holder.h"

const string CGithub::s_sTypeName = "github";

bool SGithubUser::Parse(const vector<string>& a_vLine)
{
	if (a_vLine.size() != 3)
	{
		return false;
	}
	Type = a_vLine[0];
	if (Type != CGithub::s_sTypeName)
	{
		return false;
	}
	User = a_vLine[1];
	PersonalAccessToken = a_vLine[2];
	return true;
}

CGithub::CGithub()
	: m_bVerbose(false)
{
}

CGithub::~CGithub()
{
}

void CGithub::SetWorkspace(const string& a_sWorkspace)
{
	m_sWorkspace = a_sWorkspace;
}

void CGithub::SetRepoName(const string& a_sRepoName)
{
	m_sRepoName = a_sRepoName;
}

void CGithub::SetPersonalAccessToken(const string& a_sPersonalAccessToken)
{
	m_sPersonalAccessToken = a_sPersonalAccessToken;
}

void CGithub::SetUser(const string& a_sUser)
{
	m_sUser = a_sUser;
}

void CGithub::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

bool CGithub::CreateRepo()
{
	bool bOrg = m_sWorkspace != m_sUser;
	if (m_bVerbose)
	{
		if (bOrg)
		{
			UPrintf(USTR("INFO: create org repo %") PRIUS USTR("\n"), U8ToU(m_sWorkspace + "/" + m_sRepoName).c_str());
		}
		else
		{
			UPrintf(USTR("INFO: create user repo %") PRIUS USTR("\n"), U8ToU(m_sWorkspace + "/" + m_sRepoName).c_str());
		}
	}
	CCurlHolder curlHolder;
	CURL* pCurl = curlHolder.GetCurl();
	if (pCurl == nullptr)
	{
		UPrintf(USTR("ERROR: curl init error\n\n"));
		return false;
	}
	curlHolder.SetUserPassword(m_sUser + ":" + m_sPersonalAccessToken);
	curlHolder.HeaderAppend("User-Agent: curl");
	curlHolder.HeaderAppend("Accept: application/vnd.github.v3+json");
	string sPostFields = "{\"name\": \"" + m_sRepoName + "\", \"private\": false}";
	if (bOrg)
	{
		curlHolder.SetUrl("https://api.github.com/orgs/" + m_sWorkspace + "/repos");
	}
	else
	{
		curlHolder.SetUrl("https://api.github.com/user/repos");
	}
	if (curlHolder.IsError())
	{
		UPrintf(USTR("ERROR: curl setup error\n\n"));
		return false;
	}
	CURLcode eCode = curl_easy_setopt(pCurl, CURLOPT_POST, 1L);
	if (eCode != CURLE_OK)
	{
		UPrintf(USTR("ERROR: curl setup error %d\n\n"), eCode);
		return false;
	}
	eCode = curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, curlHolder.GetHeader());
	if (eCode != CURLE_OK)
	{
		UPrintf(USTR("ERROR: curl setup error %d\n\n"), eCode);
		return false;
	}
	eCode = curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, sPostFields.c_str());
	if (eCode != CURLE_OK)
	{
		UPrintf(USTR("ERROR: curl setup error %d\n\n"), eCode);
		return false;
	}
	eCode = curl_easy_perform(pCurl);
	if (eCode != CURLE_OK)
	{
		UPrintf(USTR("ERROR: curl perform error %d\n\n"), eCode);
		return false;
	}
	long nStatusCode = 0;
	eCode = curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, &nStatusCode);
	string sResponse = curlHolder.GetData();
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: response\n%") PRIUS USTR("\n"), U8ToU(sResponse).c_str());
	}
	if (eCode != CURLE_OK || nStatusCode != 201)
	{
		UPrintf(USTR("ERROR: create repo error %d code %ld\n\n"), eCode, nStatusCode);
		return false;
	}
	return true;
}

string CGithub::GetRepoRemoteHttpsURL() const
{
	string sURL = "https://github.com/" + m_sWorkspace + "/" + m_sRepoName + ".git";
	return sURL;
}

string CGithub::GetRepoPushHttpsURL() const
{
	string sUser = m_sUser;
	string sPersonalAccessToken = m_sPersonalAccessToken;
	string sURL = "https://" + sUser + ":" + sPersonalAccessToken + "@github.com/" + m_sWorkspace + "/" + m_sRepoName + ".git";
	CCurlHolder curlHolder;
	CURL* pCurl = curlHolder.GetCurl();
	if (pCurl == nullptr)
	{
		UPrintf(USTR("ERROR: curl init error\n\n"));
		return sURL;
	}
	sUser = curlHolder.EscapeString(sUser);
	sPersonalAccessToken = curlHolder.EscapeString(sPersonalAccessToken);
	sURL = "https://" + sUser + ":" + sPersonalAccessToken + "@github.com/" + m_sWorkspace + "/" + m_sRepoName + ".git";
	return sURL;
}

n32 CGithub::GetAddDataFileSizeMaxCountPerCommit() const
{
	return 50;
}
