#include "sha256.h"

bool CSHA256::s_bInit = false;

string CSHA256::s_sEmptySHA256;

CSHA256::CSHA256()
{
	m_bInit = SHA256_Init(&m_Context) != 0;
	if (m_bInit)
	{
		SHA256_CTX context;
		memcpy(&context, &m_Context, sizeof(SHA256_CTX));
		SHA256_Final(m_uSHA256, &context);
	}
}

CSHA256::~CSHA256()
{
}

void CSHA256::UpdateSHA256(const void* a_pData, u32 a_uSize)
{
	if (m_bInit)
	{
		SHA256_Update(&m_Context, a_pData, a_uSize);
		SHA256_CTX context;
		memcpy(&context, &m_Context, sizeof(SHA256_CTX));
		SHA256_Final(m_uSHA256, &context);
	}
}

string CSHA256::GetSHA256() const
{
	string sSHA256;
	if (m_bInit)
	{
		sSHA256.reserve(SHA256_DIGEST_LENGTH * 2);
		for (n32 i = 0; i < SHA256_DIGEST_LENGTH; i++)
		{
			sSHA256 += Format("%02x", m_uSHA256[i]);
		}
	}
	return sSHA256;
}

const string& CSHA256::GetEmptySHA256()
{
	if (!s_bInit)
	{
		CSHA256 sha256;
		sha256.UpdateSHA256(nullptr, 0);
		s_sEmptySHA256 = sha256.GetSHA256();
		s_bInit = true;
	}
	return s_sEmptySHA256;
}

string CSHA256::GetSHA256(const void* a_pData, u32 a_uSize)
{
	CSHA256 sha256;
	sha256.UpdateSHA256(a_pData, a_uSize);
	return sha256.GetSHA256();
}
