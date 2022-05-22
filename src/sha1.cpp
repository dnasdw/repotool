#include "sha1.h"

bool CSHA1::s_bInit = false;

string CSHA1::s_sEmptySHA1;

CSHA1::CSHA1()
{
	m_bInit = SHA1_Init(&m_Context) != 0;
	if (m_bInit)
	{
		SHA_CTX context;
		memcpy(&context, &m_Context, sizeof(SHA_CTX));
		SHA1_Final(m_uSHA1, &context);
	}
}

CSHA1::~CSHA1()
{
}

void CSHA1::UpdateSHA1(const void* a_pData, u32 a_uSize)
{
	if (m_bInit)
	{
		SHA1_Update(&m_Context, a_pData, a_uSize);
		SHA_CTX context;
		memcpy(&context, &m_Context, sizeof(SHA_CTX));
		SHA1_Final(m_uSHA1, &context);
	}
}

string CSHA1::GetSHA1() const
{
	string sSHA1;
	if (m_bInit)
	{
		sSHA1.reserve(SHA_DIGEST_LENGTH * 2);
		for (n32 i = 0; i < SHA_DIGEST_LENGTH; i++)
		{
			sSHA1 += Format("%02x", m_uSHA1[i]);
		}
	}
	return sSHA1;
}

const string& CSHA1::GetEmptySHA1()
{
	if (!s_bInit)
	{
		CSHA1 sha1;
		sha1.UpdateSHA1(nullptr, 0);
		s_sEmptySHA1 = sha1.GetSHA1();
		s_bInit = true;
	}
	return s_sEmptySHA1;
}

string CSHA1::GetSHA1(const void* a_pData, u32 a_uSize)
{
	CSHA1 sha1;
	sha1.UpdateSHA1(a_pData, a_uSize);
	return sha1.GetSHA1();
}
