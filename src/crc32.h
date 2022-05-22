#ifndef CRC32_H_
#define CRC32_H_

#include <sdw.h>

class CCRC32
{
public:
	CCRC32();
	~CCRC32();
	void UpdateCRC32(const void* a_pData, u32 a_uSize);
	string GetCRC32() const;
	static const string& GetEmptyCRC32();
	static string GetCRC32(const void* a_pData, u32 a_uSize);
private:
	static bool s_bInit;
	static string s_sEmptyCRC32;
	u32 m_uCRC32;
};

#endif	// CRC32_H_
