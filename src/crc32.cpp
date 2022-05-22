#include "crc32.h"

bool CCRC32::s_bInit = false;

string CCRC32::s_sEmptyCRC32;

CCRC32::CCRC32()
{
	m_uCRC32 = ::UpdateCRC32(nullptr, 0);
}

CCRC32::~CCRC32()
{
}

void CCRC32::UpdateCRC32(const void* a_pData, u32 a_uSize)
{
	m_uCRC32 = ::UpdateCRC32(a_pData, a_uSize, m_uCRC32);
}

string CCRC32::GetCRC32() const
{
	string sCRC32 = Format("%08x", m_uCRC32);
	return sCRC32;
}

const string& CCRC32::GetEmptyCRC32()
{
	if (!s_bInit)
	{
		CCRC32 crc32;
		crc32.UpdateCRC32(nullptr, 0);
		s_sEmptyCRC32 = crc32.GetCRC32();
		s_bInit = true;
	}
	return s_sEmptyCRC32;
}

string CCRC32::GetCRC32(const void* a_pData, u32 a_uSize)
{
	CCRC32 crc32;
	crc32.UpdateCRC32(a_pData, a_uSize);
	return crc32.GetCRC32();
}
