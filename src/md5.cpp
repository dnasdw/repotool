#include "md5.h"

bool CMD5::s_bInit = false;

string CMD5::s_sEmptyMD5;

CMD5::CMD5()
{
	m_bInit = MD5_Init(&m_Context) != 0;
	if (m_bInit)
	{
		MD5_CTX context;
		memcpy(&context, &m_Context, sizeof(MD5_CTX));
		MD5_Final(m_uMD5, &context);
	}
}

CMD5::~CMD5()
{
}

void CMD5::UpdateMD5(const void* a_pData, u32 a_uSize)
{
	if (m_bInit)
	{
		MD5_Update(&m_Context, a_pData, a_uSize);
		MD5_CTX context;
		memcpy(&context, &m_Context, sizeof(MD5_CTX));
		MD5_Final(m_uMD5, &context);
	}
}

string CMD5::GetMD5() const
{
	string sMD5;
	if (m_bInit)
	{
		sMD5.reserve(MD5_DIGEST_LENGTH * 2);
		for (n32 i = 0; i < MD5_DIGEST_LENGTH; i++)
		{
			sMD5 += Format("%02x", m_uMD5[i]);
		}
	}
	return sMD5;
}

const string& CMD5::GetEmptyMD5()
{
	if (!s_bInit)
	{
		CMD5 md5;
		md5.UpdateMD5(nullptr, 0);
		s_sEmptyMD5 = md5.GetMD5();
		s_bInit = true;
	}
	return s_sEmptyMD5;
}

string CMD5::GetMD5(const void* a_pData, u32 a_uSize)
{
	CMD5 md5;
	md5.UpdateMD5(a_pData, a_uSize);
	return md5.GetMD5();
}
