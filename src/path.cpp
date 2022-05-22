#include "path.h"
#include "crc32.h"
#include "md5.h"
#include "sha1.h"
#include "sha256.h"

SRepoDataV1::SRepoDataV1()
	: RepoSize(0)
	, RepoIndex(0)
	, RepoIndex2(0)
{
	CRC32Hex = CCRC32::GetEmptyCRC32();
	MD5Hex = CMD5::GetEmptyMD5();
	SHA1Hex = CSHA1::GetEmptySHA1();
	SHA256Hex = CSHA256::GetEmptySHA256();
}

bool SRepoDataV1::Parse(vector<string>& a_vLine, n32 a_nIndex)
{
	if (static_cast<n32>(a_vLine.size()) != a_nIndex + 3)
	{
		return false;
	}
	RepoSize = SToN64(a_vLine[a_nIndex]);
	RepoIndex = SToN32(a_vLine[a_nIndex + 1]);
	RepoIndex2 = SToN32(a_vLine[a_nIndex + 2]);
	return true;
}

bool SPath::operator<(const SPath& a_Path) const
{
	string sLHS = Path;
	transform(sLHS.begin(), sLHS.end(), sLHS.begin(), ::toupper);
	string sRHS = a_Path.Path;
	transform(sRHS.begin(), sRHS.end(), sRHS.begin(), ::toupper);
	return sLHS < sRHS;
}

bool SPath::Parse(const string& a_sLine)
{
	vector<string> vLine = Split(a_sLine, "\t");
	if (vLine.size() < 4)
	{
		return false;
	}
	Path = vLine[0];
	PathU = U8ToU(Path);
	if (vLine[1] == "d")
	{
		IsDir = true;
	}
	else if (vLine[1] == "-")
	{
		IsDir = false;
	}
	else
	{
		return false;
	}
	Size = SToN64(vLine[2]);
	RuleVersion = SToN32(vLine[3]);
	switch (RuleVersion)
	{
	case 1:
		if (!RepoDataV1.Parse(vLine, 4))
		{
			return false;
		}
		break;
	default:
		return false;
	}
	return true;
}

string SPath::GetIndexLine() const
{
	string sIndexLine;
	switch (RuleVersion)
	{
	case 1:
		{
			sIndexLine = Format("%s\t%s\t%" PRId64 "\t%d\t%" PRId64 "\t%d\t%d\n", Path.c_str(), (IsDir ? "d" : "-"), Size, RuleVersion, RepoDataV1.RepoSize, RepoDataV1.RepoIndex, RepoDataV1.RepoIndex2);
		}
		break;
	default:
		break;
	}
	return sIndexLine;
}

string SPath::GetCRC32HashLine() const
{
	string sHashLine;
	switch (RuleVersion)
	{
	case 1:
		{
			sHashLine = Format("%s\t%s\n", Path.c_str(), RepoDataV1.CRC32Hex.c_str());
		}
		break;
	default:
		break;
	}
	return sHashLine;
}

string SPath::GetMD5HashLine() const
{
	string sHashLine;
	switch (RuleVersion)
	{
	case 1:
		{
			sHashLine = Format("%s\t%s\n", Path.c_str(), RepoDataV1.MD5Hex.c_str());
		}
		break;
	default:
		break;
	}
	return sHashLine;
}

string SPath::GetSHA1HashLine() const
{
	string sHashLine;
	switch (RuleVersion)
	{
	case 1:
		{
			sHashLine = Format("%s\t%s\n", Path.c_str(), RepoDataV1.SHA1Hex.c_str());
		}
		break;
	default:
		break;
	}
	return sHashLine;
}

string SPath::GetSHA256HashLine() const
{
	string sHashLine;
	switch (RuleVersion)
	{
	case 1:
		{
			sHashLine = Format("%s\t%s\n", Path.c_str(), RepoDataV1.SHA256Hex.c_str());
		}
		break;
	default:
		break;
	}
	return sHashLine;
}
