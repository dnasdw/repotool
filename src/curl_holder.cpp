#include "curl_holder.h"

CCurlHolder::CCurlHolder()
	: m_bError(false)
	, m_pCurl(nullptr)
	, m_pHeader(nullptr)
	, m_pCookieList(nullptr)
{
	m_pCurl = curl_easy_init();
	m_bError = m_pCurl == nullptr;
	if (m_pCurl != nullptr)
	{
		m_bError = m_bError || curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &OnWrite) != CURLE_OK;
		m_bError = m_bError || curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, this) != CURLE_OK;
	}
}

CCurlHolder::~CCurlHolder()
{
	if (m_pCookieList != nullptr)
	{
		curl_slist_free_all(m_pCookieList);
	}
	if (m_pHeader != nullptr)
	{
		curl_slist_free_all(m_pHeader);
	}
	if (m_pCurl != nullptr)
	{
		curl_easy_cleanup(m_pCurl);
	}
}

bool CCurlHolder::IsError() const
{
	return m_bError;
}

CURL* CCurlHolder::GetCurl() const
{
	return m_pCurl;
}

curl_slist* CCurlHolder::GetHeader() const
{
	return m_pHeader;
}

curl_slist* CCurlHolder::GetCookieList()
{
	if (m_pCookieList == nullptr)
	{
		m_bError = m_bError || curl_easy_getinfo(m_pCurl, CURLINFO_COOKIELIST, &m_pCookieList) != CURLE_OK;
	}
	return m_pCookieList;
}

void CCurlHolder::SetUserPassword(const string& a_sUserPassword)
{
	if (m_pCurl != nullptr)
	{
		m_bError = m_bError || curl_easy_setopt(m_pCurl, CURLOPT_USERPWD, a_sUserPassword.c_str()) != CURLE_OK;
	}
}

void CCurlHolder::SetUrl(const string& a_sUrl)
{
	if (m_pCurl != nullptr)
	{
		m_bError = m_bError || curl_easy_setopt(m_pCurl, CURLOPT_URL, a_sUrl.c_str()) != CURLE_OK;
		if (StartWith(a_sUrl, "https://"))
		{
			m_bError = m_bError || curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L) != CURLE_OK;
			m_bError = m_bError || curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L) != CURLE_OK;
		}
	}
}

const string& CCurlHolder::GetData() const
{
	return m_sData;
}

void CCurlHolder::HeaderAppend(const string& a_sHeader)
{
	m_pHeader = curl_slist_append(m_pHeader, a_sHeader.c_str());
}

string CCurlHolder::EscapeString(const string& a_sUnencoded)
{
	string sValue;
	if (m_pCurl != nullptr)
	{
		char* pValue = curl_easy_escape(m_pCurl, a_sUnencoded.c_str(), 0);
		m_bError = m_bError || pValue == nullptr;
		if (pValue != nullptr)
		{
			sValue = pValue;
			curl_free(pValue);
		}
	}
	return sValue;
}

size_t CCurlHolder::OnWrite(char* a_pData, size_t a_uSize, size_t a_uNmemb, void* a_pUserData)
{
	size_t uSize = a_uSize * a_uNmemb;
	CCurlHolder* pUrl = reinterpret_cast<CCurlHolder*>(a_pUserData);
	if (pUrl != nullptr)
	{
		pUrl->m_sData.append(a_pData, uSize);
	}
	return uSize;
}

void CCurlHolder::CookieAppend(string& a_sCookieList, const string& a_sCookie)
{
	if (!a_sCookieList.empty())
	{
		a_sCookieList += "; ";
	}
	a_sCookieList += a_sCookie;
}

bool CCurlGlobalHolder::s_bInitialized = false;

CCurlGlobalHolder::CCurlGlobalHolder()
{
}

CCurlGlobalHolder::~CCurlGlobalHolder()
{
}

bool CCurlGlobalHolder::Initialize()
{
	if (!s_bInitialized)
	{
		CURLcode eError = curl_global_init(CURL_GLOBAL_DEFAULT);
		if (eError == CURLE_OK)
		{
			s_bInitialized = true;
		}
	}
	return s_bInitialized;
}

void CCurlGlobalHolder::Finalize()
{
	if (s_bInitialized)
	{
		curl_global_cleanup();
		s_bInitialized = false;
	}
}
