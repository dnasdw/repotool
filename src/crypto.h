#ifndef CRYPTO_H_
#define CRYPTO_H_

#include <sdw.h>
#include "bignum.h"

void FEncryptAesCtrData(void* a_pData, const CBigNum& a_Key, const CBigNum& a_Counter, n64 a_nDataSize, n64 a_nXorOffset);

string FGenerateKey(const string& a_sKey);

string FEncryptString(const string& a_sData, const string& a_sKey);

string FDecryptString(const string& a_sData, const string& a_sKey);

#endif	// CRYPTO_H_
