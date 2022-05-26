#include "github.h"
#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include "curl_holder.h"

const string CGithub::s_sTypeName = "github";
const string CGithub::s_sImportStatusImporting = "importing";
const string CGithub::s_sImportStatusComplete = "complete";
const string CGithub::s_sImportStatusError = "error";

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

void CGithub::SetSourceRemoteURL(const string& a_sSourceRemoteURL)
{
	m_sSourceRemoteURL = a_sSourceRemoteURL;
}

void CGithub::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

bool CGithub::CreateRepo() const
{
	if (getRepo())
	{
		return true;
	}
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

bool CGithub::GetImportStatus()
{
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: get import repo %") PRIUS USTR(" status\n"), U8ToU(m_sWorkspace + "/" + m_sRepoName).c_str());
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
	curlHolder.SetUrl("https://api.github.com/repos/" + m_sWorkspace + "/" + m_sRepoName + "/import");
	if (curlHolder.IsError())
	{
		UPrintf(USTR("ERROR: curl setup error\n\n"));
		return false;
	}
	CURLcode eCode = curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, curlHolder.GetHeader());
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
		UPrintf(USTR("ERROR: get import repo status error %d code %ld\n\n"), eCode, nStatusCode);
		return false;
	}
	if (!parseImportResponse(sResponse))
	{
		return false;
	}
	return true;
}

bool CGithub::StartImportRepo()
{
	if (GetImportStatus())
	{
		return true;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: start import repo %") PRIUS USTR("\n"), U8ToU(m_sWorkspace + "/" + m_sRepoName).c_str());
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
	string sPostFields = "{\"vcs\": \"git\", \"vcs_url\": \"" + m_sSourceRemoteURL + "\"}";
	curlHolder.SetUrl("https://api.github.com/repos/" + m_sWorkspace + "/" + m_sRepoName + "/import");
	if (curlHolder.IsError())
	{
		UPrintf(USTR("ERROR: curl setup error\n\n"));
		return false;
	}
	CURLcode eCode = curl_easy_setopt(pCurl, CURLOPT_CUSTOMREQUEST, "PUT");
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
		UPrintf(USTR("ERROR: start import repo error %d code %ld\n\n"), eCode, nStatusCode);
		return false;
	}
	if (!parseImportResponse(sResponse))
	{
		return false;
	}
	return true;
}

bool CGithub::PatchImportRepo()
{
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: patch import repo %") PRIUS USTR("\n"), U8ToU(m_sWorkspace + "/" + m_sRepoName).c_str());
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
	curlHolder.SetUrl("https://api.github.com/repos/" + m_sWorkspace + "/" + m_sRepoName + "/import");
	if (curlHolder.IsError())
	{
		UPrintf(USTR("ERROR: curl setup error\n\n"));
		return false;
	}
	CURLcode eCode = curl_easy_setopt(pCurl, CURLOPT_CUSTOMREQUEST, "PATCH");
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
		UPrintf(USTR("ERROR: patch import repo error %d code %ld\n\n"), eCode, nStatusCode);
		return false;
	}
	if (!parseImportResponse(sResponse))
	{
		return false;
	}
	return true;
}

string CGithub::GetImportStatusString() const
{
	return m_sImportStatusString;
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

bool CGithub::getRepo() const
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
	curlHolder.SetUserPassword(m_sUser + ":" + m_sPersonalAccessToken);
	curlHolder.HeaderAppend("User-Agent: curl");
	curlHolder.HeaderAppend("Accept: application/vnd.github.v3+json");
	curlHolder.SetUrl("https://api.github.com/repos/" + m_sWorkspace + "/" + m_sRepoName);
	if (curlHolder.IsError())
	{
		UPrintf(USTR("ERROR: curl setup error\n\n"));
		return false;
	}
	CURLcode eCode = curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, curlHolder.GetHeader());
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
		UPrintf(USTR("ERROR: get repo error %d code %ld\n\n"), eCode, nStatusCode);
		return false;
	}
	return true;
}

bool CGithub::parseImportResponse(const string& a_sResponse)
{
	RAPIDJSON_NAMESPACE::Document responseJson;
	if (responseJson.Parse(a_sResponse).HasParseError() || !responseJson.IsObject())
	{
		UPrintf(USTR("ERROR: parse json response failed\n\n"));
		return false;
	}
	RAPIDJSON_NAMESPACE::Value::ConstMemberIterator itJsonStatus = responseJson.FindMember("status");
	if (itJsonStatus == responseJson.MemberEnd())
	{
		UPrintf(USTR("ERROR: no status in json response\n\n"));
		return false;
	}
	const RAPIDJSON_NAMESPACE::Value& status = itJsonStatus->value;
	if (!status.IsString())
	{
		UPrintf(USTR("ERROR: status is not a string member\n\n"));
		return false;
	}
	m_sImportStatusString = status.GetString();
	if (m_sImportStatusString == "detecting"
		|| m_sImportStatusString == "importing"
		|| m_sImportStatusString == "mapping"
		|| m_sImportStatusString == "pushing")
	{
		m_sImportStatusString = s_sImportStatusImporting;
	}
	else if (m_sImportStatusString == "complete")
	{
		m_sImportStatusString = s_sImportStatusComplete;
	}
	else if (m_sImportStatusString == "auth_failed"
		|| m_sImportStatusString == "error"
		|| m_sImportStatusString == "detection_needs_auth"
		|| m_sImportStatusString == "detection_found_nothing")
	{
		m_sImportStatusString = s_sImportStatusError;
	}
	else
	{
		UPrintf(USTR("ERROR: unknown status %") PRIUS USTR("\n\n"), U8ToU(m_sImportStatusString).c_str());
		m_sImportStatusString.clear();
		return false;
	}
	return true;
}
