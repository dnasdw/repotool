#ifndef SHA256_H_
#define SHA256_H_

#include <sdw.h>
#include <openssl/sha.h>

class CSHA256
{
public:
	CSHA256();
	~CSHA256();
	void UpdateSHA256(const void* a_pData, u32 a_uSize);
	string GetSHA256() const;
	static const string& GetEmptySHA256();
	static string GetSHA256(const void* a_pData, u32 a_uSize);
private:
	static bool s_bInit;
	static string s_sEmptySHA256;
	SHA256_CTX m_Context;
	bool m_bInit;
	u8 m_uSHA256[SHA256_DIGEST_LENGTH];
};

#endif	// SHA256_H_
