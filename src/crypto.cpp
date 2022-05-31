#include "crypto.h"
#include <openssl/aes.h>
#include <openssl/modes.h>
#include "crc32.h"
#include "md5.h"
#include "sha1.h"
#include "sha256.h"

void FEncryptAesCtrData(void* a_pData, const CBigNum& a_Key, const CBigNum& a_Counter, n64 a_nDataSize, n64 a_nXorOffset)
{
	u8 uKey[16] = {};
	a_Key.GetBytes(uKey, 16);
	CBigNum counter = a_Counter;
	counter += static_cast<int>(a_nXorOffset / 0x10);
	a_nXorOffset %= 0x10;
	u8 uCounter[16] = {};
	counter.GetBytes(uCounter, 16);
	AES_KEY key;
	AES_set_encrypt_key(uKey, 128, &key);
	u8 uEcountBuf[16] = {};
	u32 uNum = 0;
	if (a_nDataSize > 0)
	{
		if (a_nXorOffset == 0)
		{
			CRYPTO_ctr128_encrypt(reinterpret_cast<u8*>(a_pData), reinterpret_cast<u8*>(a_pData), static_cast<size_t>(a_nDataSize), &key, uCounter, uEcountBuf, &uNum, reinterpret_cast<block128_f>(AES_encrypt));
		}
		else
		{
			const n64 nBufferSize = 0x10;
			u8 uBuffer[nBufferSize] = {};
			n64 nSize = a_nXorOffset + a_nDataSize > nBufferSize ? nBufferSize : a_nXorOffset + a_nDataSize;
			memcpy(uBuffer + a_nXorOffset, a_pData, static_cast<size_t>(nSize - a_nXorOffset));
			CRYPTO_ctr128_encrypt(uBuffer, uBuffer, static_cast<size_t>(nSize), &key, uCounter, uEcountBuf, &uNum, reinterpret_cast<block128_f>(AES_encrypt));
			memcpy(a_pData, uBuffer + a_nXorOffset, static_cast<size_t>(nSize - a_nXorOffset));
			a_nDataSize -= nSize - a_nXorOffset;
			if (a_nDataSize > 0)
			{
				CRYPTO_ctr128_encrypt(reinterpret_cast<u8*>(a_pData) + (nSize - a_nXorOffset), reinterpret_cast<u8*>(a_pData) + (nSize - a_nXorOffset), static_cast<size_t>(a_nDataSize), &key, uCounter, uEcountBuf, &uNum, reinterpret_cast<block128_f>(AES_encrypt));
			}
		}
	}
}

string FGenerateKey(const string& a_sKey)
{
	string sImportKey = a_sKey + CCRC32::GetCRC32(a_sKey.c_str(), static_cast<u32>(a_sKey.size())) + CSHA1::GetSHA1(a_sKey.c_str(), static_cast<u32>(a_sKey.size())) + CSHA256::GetSHA256(a_sKey.c_str(), static_cast<u32>(a_sKey.size()));
	sImportKey = CMD5::GetMD5(sImportKey.c_str(), static_cast<u32>(sImportKey.size()));
	return sImportKey;
}

string FEncryptString(const string& a_sData, const string& a_sKey)
{
	if (a_sData.empty())
	{
		return a_sData;
	}
	string sData(a_sData);
	CBigNum key(a_sKey.c_str());
	FEncryptAesCtrData(&*sData.begin(), key, 0, static_cast<n64>(sData.size()), 0);
	string sEncryptedData;
	sEncryptedData.reserve(sData.size() * 2);
	for (n32 i = 0; i < static_cast<n32>(sData.size()); i++)
	{
		sEncryptedData += Format("%02X", sData[i] & 0xFF);
	}
	return sEncryptedData;
}

string FDecryptString(const string& a_sData, const string& a_sKey)
{
	vector<u8> vData(a_sData.size() / 2);
	if (vData.empty())
	{
		return a_sData;
	}
	for (n32 i = 0; i < static_cast<n32>(a_sData.size()) / 2; i++)
	{
		vData[i] = SToU8(a_sData.substr(i * 2, 2), 16);
	}
	CBigNum key(a_sKey.c_str());
	FEncryptAesCtrData(&*vData.begin(), key, 0, static_cast<n64>(vData.size()), 0);
	string sDecryptedData(vData.begin(), vData.end());
	return sDecryptedData;
}
