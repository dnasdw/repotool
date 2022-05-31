#ifndef REPOTOOL_H_
#define REPOTOOL_H_

#include <sdw.h>

class CRepoTool
{
public:
	enum EParseOptionReturn
	{
		kParseOptionReturnSuccess,
		kParseOptionReturnIllegalOption,
		kParseOptionReturnNoArgument,
		kParseOptionReturnUnknownArgument,
		kParseOptionReturnOptionConflict
	};
	enum EAction
	{
		kActionNone,
		kActionUnpack,
		kActionPack,
		kActionUpload,
		kActionDownload,
		kActionImport,
		kActionSample,
		kActionHelp
	};
	struct SOption
	{
		const UChar* Name;
		int Key;
		const UChar* Doc;
	};
	CRepoTool();
	~CRepoTool();
	int ParseOptions(int a_nArgc, UChar* a_pArgv[]);
	int CheckOptions() const;
	int Help() const;
	int Action() const;
	static SOption s_Option[];
private:
	EParseOptionReturn parseOptions(const UChar* a_pName, int& a_nIndex, int a_nArgc, UChar* a_pArgv[]);
	EParseOptionReturn parseOptions(int a_nKey, int& a_nIndex, int a_nArgc, UChar* a_pArgv[]);
	bool unpack() const;
	bool pack() const;
	bool upload() const;
	bool download() const;
	bool import() const;
	int sample() const;
	EAction m_eAction;
	UString m_sInputPath;
	UString m_sOutputPath;
	bool m_bVerbose;
	UString m_sType;
	UString m_sWorkspace;
	UString m_sUser;
	bool m_bUpdateImport;
	string m_sImportParam;
	string m_sImportKey;
	UString m_sMessage;
};

#endif	// REPOTOOL_H_
