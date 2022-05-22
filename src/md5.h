#ifndef MD5_H_
#define MD5_H_

#include <sdw.h>
#include <openssl/md5.h>

class CMD5
{
public:
	CMD5();
	~CMD5();
	void UpdateMD5(const void* a_pData, u32 a_uSize);
	string GetMD5() const;
	static const string& GetEmptyMD5();
	static string GetMD5(const void* a_pData, u32 a_uSize);
private:
	static bool s_bInit;
	static string s_sEmptyMD5;
	MD5_CTX m_Context;
	bool m_bInit;
	u8 m_uMD5[MD5_DIGEST_LENGTH];
};

#endif	// MD5_H_
