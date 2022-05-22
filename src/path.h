#ifndef PATH_H_
#define PATH_H_

#include <sdw.h>

struct SRepoDataV1
{
	n64 RepoSize;
	n32 RepoIndex;
	n32 RepoIndex2;
	string CRC32Hex;
	string MD5Hex;
	string SHA1Hex;
	string SHA256Hex;
	SRepoDataV1();
	bool Parse(vector<string>& a_vLine, n32 a_nIndex);
};

struct SPath
{
	UString PathU;
	string Path;
	bool IsDir;
	n64 Size;
	n32 RuleVersion;
	SRepoDataV1 RepoDataV1;
	bool operator<(const SPath& a_Path) const;
	bool Parse(const string& a_sLine);
	string GetIndexLine() const;
	string GetCRC32HashLine() const;
	string GetMD5HashLine() const;
	string GetSHA1HashLine() const;
	string GetSHA256HashLine() const;
};

#endif	// PATH_H_
