#ifndef CURL_HOLDER_H_
#define CURL_HOLDER_H_

#include <sdw.h>
#include <curl/curl.h>

class CCurlHolder
{
public:
	CCurlHolder();
	~CCurlHolder();
	bool IsError() const;
	CURL* GetCurl() const;
	curl_slist* GetHeader() const;
	curl_slist* GetCookieList();
	void SetUserPassword(const string& a_sUserPassword);
	void SetUrl(const string& a_sUrl);
	const string& GetData() const;
	void HeaderAppend(const string& a_sHeader);
	string EscapeString(const string& a_sUnencoded);
	static size_t OnWrite(char* a_pData, size_t a_uSize, size_t a_uNmemb, void* a_pUserData);
	static void CookieAppend(string& a_sCookieList, const string& a_sCookie);
private:
	bool m_bError;
	CURL* m_pCurl;
	curl_slist* m_pHeader;
	curl_slist* m_pCookieList;
	string m_sData;
};

class CCurlGlobalHolder
{
public:
	CCurlGlobalHolder();
	~CCurlGlobalHolder();
	static bool Initialize();
	static void Finalize();
private:
	static bool s_bInitialized;
};

#endif	// CURL_HOLDER_H_
