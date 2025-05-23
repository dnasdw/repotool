#include "bitbucket.h"
#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include "curl_holder.h"

const string CBitbucket::s_sTypeName = "bitbucket";
const string CBitbucket::s_sConfigKeyProjectName = "bitbucket.project.name";
const string CBitbucket::s_sConfigKeyProjectKey = "bitbucket.project.key";

bool SBitbucketUser::Parse(const vector<string>& a_vLine)
{
	if (a_vLine.size() != 3)
	{
		return false;
	}
	Type = a_vLine[0];
	if (Type != CBitbucket::s_sTypeName)
	{
		return false;
	}
	User = a_vLine[1];
	AppPassword = a_vLine[2];
	return true;
}

CBitbucket::CBitbucket()
	: m_bVerbose(false)
	, m_bRepoIsPrivate(false)
{
}

CBitbucket::~CBitbucket()
{
}

void CBitbucket::SetWorkspace(const string& a_sWorkspace)
{
	m_sWorkspace = a_sWorkspace;
}

void CBitbucket::SetRepoName(const string& a_sRepoName)
{
	m_sRepoName = a_sRepoName;
}

void CBitbucket::SetAppPassword(const string& a_sAppPassword)
{
	m_sAppPassword = a_sAppPassword;
}

void CBitbucket::SetUser(const string& a_sUser)
{
	m_sUser = a_sUser;
}

void CBitbucket::SetProjectName(const string& a_sProjectName)
{
	m_sProjectName = a_sProjectName;
}

void CBitbucket::SetProjectKey(const string& a_sProjectKey)
{
	m_sProjectKey = a_sProjectKey;
}

void CBitbucket::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

bool CBitbucket::CreateRepo()
{
	if (!createProject())
	{
		return false;
	}
	if (getRepo())
	{
		return true;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: create repo %") PRIUS USTR(" project key %") PRIUS USTR("\n"), U8ToU(m_sWorkspace + "/" + m_sRepoName).c_str(), U8ToU(m_sProjectKey).c_str());
	}
	CCurlHolder curlHolder;
	CURL* pCurl = curlHolder.GetCurl();
	if (pCurl == nullptr)
	{
		UPrintf(USTR("ERROR: curl init error\n\n"));
		return false;
	}
	curlHolder.SetUserPassword(m_sUser + ":" + m_sAppPassword);
	curlHolder.HeaderAppend("Content-Type: application/json");
	string sPostFields = Format("{\"is_private\": %s, \"scm\": \"git\", \"project\": {\"key\": \"%s\"}}", (m_bRepoIsPrivate ? "true" : "false"), m_sProjectKey.c_str());
	curlHolder.SetUrl("https://api.bitbucket.org/2.0/repositories/" + m_sWorkspace + "/" + m_sRepoName);
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
	if (eCode != CURLE_OK || nStatusCode != 200)
	{
		UPrintf(USTR("ERROR: create repo error %d code %ld\n\n"), eCode, nStatusCode);
		return false;
	}
	return true;
}

bool CBitbucket::DeleteRepo() const
{
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: delete repo %") PRIUS USTR("\n"), U8ToU(m_sWorkspace + "/" + m_sRepoName).c_str());
	}
	CCurlHolder curlHolder;
	CURL* pCurl = curlHolder.GetCurl();
	if (pCurl == nullptr)
	{
		UPrintf(USTR("ERROR: curl init error\n\n"));
		return false;
	}
	curlHolder.SetUserPassword(m_sUser + ":" + m_sAppPassword);
	curlHolder.SetUrl("https://api.bitbucket.org/2.0/repositories/" + m_sWorkspace + "/" + m_sRepoName);
	if (curlHolder.IsError())
	{
		UPrintf(USTR("ERROR: curl setup error\n\n"));
		return false;
	}
	CURLcode eCode = curl_easy_setopt(pCurl, CURLOPT_CUSTOMREQUEST, "DELETE");
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
	if (eCode != CURLE_OK || (nStatusCode != 204 && nStatusCode != 404))
	{
		UPrintf(USTR("ERROR: create repo error %d code %ld\n\n"), eCode, nStatusCode);
		return false;
	}
	return true;
}

string CBitbucket::GetRepoRemoteHttpsURL() const
{
	string sURL = "https://bitbucket.org/" + m_sWorkspace + "/" + m_sRepoName + ".git";
	return sURL;
}

string CBitbucket::GetRepoPushHttpsURL() const
{
	string sUser = m_sUser;
	string sAppPassword = m_sAppPassword;
	string sURL = "https://" + sUser + ":" + sAppPassword + "@bitbucket.org/" + m_sWorkspace + "/" + m_sRepoName + ".git";
	CCurlHolder curlHolder;
	CURL* pCurl = curlHolder.GetCurl();
	if (pCurl == nullptr)
	{
		UPrintf(USTR("ERROR: curl init error\n\n"));
		return sURL;
	}
	sUser = curlHolder.EscapeString(sUser);
	sAppPassword = curlHolder.EscapeString(sAppPassword);
	sURL = "https://" + sUser + ":" + sAppPassword + "@bitbucket.org/" + m_sWorkspace + "/" + m_sRepoName + ".git";
	return sURL;
}

n32 CBitbucket::GetAddDataFileSizeMaxCountPerCommit() const
{
	return 50;
}

bool CBitbucket::createProject()
{
	if (getProject())
	{
		return true;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: create project name %") PRIUS USTR(" key %") PRIUS USTR("\n"), U8ToU(m_sProjectName).c_str(), U8ToU(m_sProjectKey).c_str());
	}
	CCurlHolder curlHolder;
	CURL* pCurl = curlHolder.GetCurl();
	if (pCurl == nullptr)
	{
		UPrintf(USTR("ERROR: curl init error\n\n"));
		return false;
	}
	curlHolder.SetUserPassword(m_sUser + ":" + m_sAppPassword);
	curlHolder.HeaderAppend("Content-Type: application/json");
	string sPostFields = "{\"name\": \"" + m_sProjectName + "\", \"key\": \"" + m_sProjectKey + "\", \"is_private\": false}";
	curlHolder.SetUrl("https://api.bitbucket.org/2.0/workspaces/" + m_sWorkspace + "/projects");
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
		UPrintf(USTR("ERROR: create project error %d code %ld\n\n"), eCode, nStatusCode);
		return false;
	}
	return true;
}

bool CBitbucket::getProject()
{
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: get project key %") PRIUS USTR("\n"), U8ToU(m_sProjectKey).c_str());
	}
	CCurlHolder curlHolder;
	CURL* pCurl = curlHolder.GetCurl();
	if (pCurl == nullptr)
	{
		UPrintf(USTR("ERROR: curl init error\n\n"));
		return false;
	}
	curlHolder.SetUserPassword(m_sUser + ":" + m_sAppPassword);
	curlHolder.SetUrl("https://api.bitbucket.org/2.0/workspaces/" + m_sWorkspace + "/projects/" + m_sProjectKey);
	if (curlHolder.IsError())
	{
		UPrintf(USTR("ERROR: curl setup error\n\n"));
		return false;
	}
	CURLcode eCode = curl_easy_perform(pCurl);
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
	if (eCode != CURLE_OK || nStatusCode != 200)
	{
		UPrintf(USTR("ERROR: get project error %d code %ld\n\n"), eCode, nStatusCode);
		return false;
	}
	RAPIDJSON_NAMESPACE::Document responseJson;
	if (responseJson.Parse(sResponse).HasParseError() || !responseJson.IsObject())
	{
		UPrintf(USTR("ERROR: parse json response failed\n\n"));
		return false;
	}
	RAPIDJSON_NAMESPACE::Value::ConstMemberIterator itJsonIsPrivate = responseJson.FindMember("is_private");
	if (itJsonIsPrivate == responseJson.MemberEnd())
	{
		UPrintf(USTR("ERROR: no is_private in json response\n\n"));
		return false;
	}
	const RAPIDJSON_NAMESPACE::Value& isPrivate = itJsonIsPrivate->value;
	if (!isPrivate.IsBool())
	{
		UPrintf(USTR("ERROR: is_private is not a bool member\n\n"));
		return false;
	}
	m_bRepoIsPrivate = isPrivate.GetBool();
	return true;
}

bool CBitbucket::getRepo() const
{
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: get repo %") PRIUS USTR("\n"), U8ToU(m_sWorkspace + "/" + m_sRepoName).c_str());
	}
	CCurlHolder curlHolder;
	CURL* pCurl = curlHolder.GetCurl();
	if (pCurl == nullptr)
	{
		UPrintf(USTR("ERROR: curl init error\n\n"));
		return false;
	}
	curlHolder.SetUserPassword(m_sUser + ":" + m_sAppPassword);
	curlHolder.SetUrl("https://api.bitbucket.org/2.0/repositories/" + m_sWorkspace + "/" + m_sRepoName);
	if (curlHolder.IsError())
	{
		UPrintf(USTR("ERROR: curl setup error\n\n"));
		return false;
	}
	CURLcode eCode = curl_easy_perform(pCurl);
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
	if (eCode != CURLE_OK || nStatusCode != 200)
	{
		UPrintf(USTR("ERROR: get repo error %d code %ld\n\n"), eCode, nStatusCode);
		return false;
	}
	return true;
}
