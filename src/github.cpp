#include "github.h"
#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include "curl_holder.h"

const string CGithub::s_sTypeName = "github";
const string CGithub::s_sImportStatusImporting = "importing";
const string CGithub::s_sImportStatusComplete = "complete";
const string CGithub::s_sImportStatusError = "error";
const string CGithub::s_sConfigKeyImportRecorderWorkspace = "github.import.recorder.workspace";
const string CGithub::s_sConfigKeyImportRecorderRepoName = "github.import.recorder.repo.name";
const string CGithub::s_sConfigKeyImportRecorderUser = "github.import.recorder.user";
const string CGithub::s_sConfigKeyImportRecorderPersonalAccessToken = "github.import.recorder.personal_access_token";
const string CGithub::s_sConfigKeyImporterWorkspace = "github.importer.workspace";
const string CGithub::s_sConfigKeyImporterRepoName = "github.importer.repo.name";
const string CGithub::s_sConfigKeyImporterWorkflowFileName = "github.importer.workflow.file.name";
const string CGithub::s_sConfigKeyImporterUser = "github.importer.user";
const string CGithub::s_sConfigKeyImporterPersonalAccessToken = "github.importer.personal_access_token";
const string CGithub::s_sConfigKeyImporterImportKey = "github.importer.import.key";
const bool CGithub::s_bImportNeedDisableActions = true;

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
	//curlHolder.HeaderAppend("Accept: application/vnd.github+json");
	//curlHolder.HeaderAppend("Authorization: Bearer " + m_sPersonalAccessToken);
	//curlHolder.HeaderAppend("X-GitHub-Api-Version: 2022-11-28");
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

bool CGithub::DeleteRepo() const
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
	curlHolder.SetUserPassword(m_sUser + ":" + m_sPersonalAccessToken);
	curlHolder.HeaderAppend("User-Agent: curl");
	curlHolder.HeaderAppend("Accept: application/vnd.github.v3+json");
	//curlHolder.HeaderAppend("Accept: application/vnd.github+json");
	//curlHolder.HeaderAppend("Authorization: Bearer " + m_sPersonalAccessToken);
	//curlHolder.HeaderAppend("X-GitHub-Api-Version: 2022-11-28");
	curlHolder.SetUrl("https://api.github.com/repos/" + m_sWorkspace + "/" + m_sRepoName);
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
	if (eCode != CURLE_OK || (nStatusCode != 204 && nStatusCode != 404))
	{
		UPrintf(USTR("ERROR: create repo error %d code %ld\n\n"), eCode, nStatusCode);
		return false;
	}
	return true;
}

bool CGithub::SetActionsPermissions(bool a_bEnabled) const
{
	bool bEnabled = false;
	if (!getActionsPermissions(bEnabled))
	{
		return false;
	}
	if (a_bEnabled == bEnabled)
	{
		return true;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: set actions permissions for repo %") PRIUS USTR("\n"), U8ToU(m_sWorkspace + "/" + m_sRepoName).c_str());
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
	//curlHolder.HeaderAppend("Accept: application/vnd.github+json");
	//curlHolder.HeaderAppend("Authorization: Bearer " + m_sPersonalAccessToken);
	//curlHolder.HeaderAppend("X-GitHub-Api-Version: 2022-11-28");
	string sPostFields = "{\"enabled\": false}";
	if (a_bEnabled)
	{
		sPostFields = "{\"enabled\": true, \"allowed_actions\": \"all\"}";
	}
	curlHolder.SetUrl("https://api.github.com/repos/" + m_sWorkspace + "/" + m_sRepoName + "/actions/permissions");
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
	if (eCode != CURLE_OK || nStatusCode != 204)
	{
		UPrintf(USTR("ERROR: set actions permissions for repo error %d code %ld\n\n"), eCode, nStatusCode);
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
	//curlHolder.HeaderAppend("Accept: application/vnd.github+json");
	//curlHolder.HeaderAppend("Authorization: Bearer " + m_sPersonalAccessToken);
	//curlHolder.HeaderAppend("X-GitHub-Api-Version: 2022-11-28");
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

bool CGithub::StartImportRepo(const string& a_sSourceRemoteURL)
{
	if (GetImportStatus())
	{
		return true;
	}
	if (s_bImportNeedDisableActions)
	{
		if (!SetActionsPermissions(false))
		{
			return false;
		}
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
	//curlHolder.HeaderAppend("Accept: application/vnd.github+json");
	//curlHolder.HeaderAppend("Authorization: Bearer " + m_sPersonalAccessToken);
	//curlHolder.HeaderAppend("X-GitHub-Api-Version: 2022-11-28");
	string sPostFields = "{\"vcs\": \"git\", \"vcs_url\": \"" + a_sSourceRemoteURL + "\"}";
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
	//curlHolder.HeaderAppend("Accept: application/vnd.github+json");
	//curlHolder.HeaderAppend("Authorization: Bearer " + m_sPersonalAccessToken);
	//curlHolder.HeaderAppend("X-GitHub-Api-Version: 2022-11-28");
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

bool CGithub::TriggerWorkflowImport(const string& a_sWorkflowFileName, const string& a_sEncryptedImportParam) const
{
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: trigger workflow %") PRIUS USTR("\n"), U8ToU(m_sWorkspace + "/" + m_sRepoName + "/.github/workflows/" + a_sWorkflowFileName).c_str());
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
	//curlHolder.HeaderAppend("Accept: application/vnd.github+json");
	//curlHolder.HeaderAppend("Authorization: Bearer " + m_sPersonalAccessToken);
	//curlHolder.HeaderAppend("X-GitHub-Api-Version: 2022-11-28");
	string sPostFields = "{\"ref\": \"master\", \"inputs\": {\"import_param\": \"" + a_sEncryptedImportParam + "\"}}";
	curlHolder.SetUrl("https://api.github.com/repos/" + m_sWorkspace + "/" + m_sRepoName + "/actions/workflows/" + a_sWorkflowFileName + "/dispatches");
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
	if (eCode != CURLE_OK || nStatusCode != 204)
	{
		UPrintf(USTR("ERROR: trigger workflow error %d code %ld\n\n"), eCode, nStatusCode);
		return false;
	}
	return true;
}

bool CGithub::CreateEmptyFile(const string& a_sPath) const
{
	if (FileExist(a_sPath))
	{
		return true;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: create empty file %") PRIUS USTR(" in repo %") PRIUS USTR("\n"), U8ToU(a_sPath).c_str(), U8ToU(m_sWorkspace + "/" + m_sRepoName).c_str());
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
	//curlHolder.HeaderAppend("Accept: application/vnd.github+json");
	//curlHolder.HeaderAppend("Authorization: Bearer " + m_sPersonalAccessToken);
	//curlHolder.HeaderAppend("X-GitHub-Api-Version: 2022-11-28");
	string sPostFields = "{\"message\": \"" + a_sPath + "\", \"content\": \"\"}";
	curlHolder.SetUrl("https://api.github.com/repos/" + m_sWorkspace + "/" + m_sRepoName + "/contents/" + a_sPath);
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
	if (eCode != CURLE_OK || (nStatusCode != 201))
	{
		UPrintf(USTR("ERROR: create empty file error %d code %ld\n\n"), eCode, nStatusCode);
		return false;
	}
	return true;
}

bool CGithub::FileExist(const string& a_sPath) const
{
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: get file %") PRIUS USTR(" in repo %") PRIUS USTR("\n"), U8ToU(a_sPath).c_str(), U8ToU(m_sWorkspace + "/" + m_sRepoName).c_str());
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
	//curlHolder.HeaderAppend("Accept: application/vnd.github+json");
	//curlHolder.HeaderAppend("Authorization: Bearer " + m_sPersonalAccessToken);
	//curlHolder.HeaderAppend("X-GitHub-Api-Version: 2022-11-28");
	curlHolder.SetUrl("https://api.github.com/repos/" + m_sWorkspace + "/" + m_sRepoName + "/contents/" + a_sPath);
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
	if (eCode != CURLE_OK || (nStatusCode != 200))
	{
		UPrintf(USTR("ERROR: get file error %d code %ld\n\n"), eCode, nStatusCode);
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
	//curlHolder.HeaderAppend("Accept: application/vnd.github+json");
	//curlHolder.HeaderAppend("Authorization: Bearer " + m_sPersonalAccessToken);
	//curlHolder.HeaderAppend("X-GitHub-Api-Version: 2022-11-28");
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

bool CGithub::getActionsPermissions(bool& a_bEnabled) const
{
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: get actions permissions for repo %") PRIUS USTR("\n"), U8ToU(m_sWorkspace + "/" + m_sRepoName).c_str());
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
	//curlHolder.HeaderAppend("Accept: application/vnd.github+json");
	//curlHolder.HeaderAppend("Authorization: Bearer " + m_sPersonalAccessToken);
	//curlHolder.HeaderAppend("X-GitHub-Api-Version: 2022-11-28");
	curlHolder.SetUrl("https://api.github.com/repos/" + m_sWorkspace + "/" + m_sRepoName + "/actions/permissions");
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
		UPrintf(USTR("ERROR: get actions permissions for repo error %d code %ld\n\n"), eCode, nStatusCode);
		return false;
	}
	RAPIDJSON_NAMESPACE::Document responseJson;
	if (responseJson.Parse(sResponse).HasParseError() || !responseJson.IsObject())
	{
		UPrintf(USTR("ERROR: parse json response failed\n\n"));
		return false;
	}
	RAPIDJSON_NAMESPACE::Value::ConstMemberIterator itJsonEnabled = responseJson.FindMember("enabled");
	if (itJsonEnabled == responseJson.MemberEnd())
	{
		UPrintf(USTR("ERROR: no enabled in json response\n\n"));
		return false;
	}
	const RAPIDJSON_NAMESPACE::Value& enabled = itJsonEnabled->value;
	if (!enabled.IsBool())
	{
		UPrintf(USTR("ERROR: enabled is not a bool member\n\n"));
		return false;
	}
	a_bEnabled = enabled.GetBool();
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
