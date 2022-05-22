#ifndef SHA1_H_
#define SHA1_H_

#include <sdw.h>
#include <openssl/sha.h>

class CSHA1
{
public:
	CSHA1();
	~CSHA1();
	void UpdateSHA1(const void* a_pData, u32 a_uSize);
	string GetSHA1() const;
	static const string& GetEmptySHA1();
	static string GetSHA1(const void* a_pData, u32 a_uSize);
private:
	static bool s_bInit;
	static string s_sEmptySHA1;
	SHA_CTX m_Context;
	bool m_bInit;
	u8 m_uSHA1[SHA_DIGEST_LENGTH];
};

#endif	// SHA1_H_
