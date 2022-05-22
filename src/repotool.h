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
	int CheckOptions();
	int Help();
	int Action();
	static SOption s_Option[];
private:
	EParseOptionReturn parseOptions(const UChar* a_pName, int& a_nIndex, int a_nArgc, UChar* a_pArgv[]);
	EParseOptionReturn parseOptions(int a_nKey, int& a_nIndex, int a_nArgc, UChar* a_pArgv[]);
	bool unpack();
	bool pack();
	bool upload();
	bool download();
	int sample();
	EAction m_eAction;
	UString m_sInputPath;
	UString m_sOutputPath;
	bool m_bVerbose;
	UString m_sType;
	UString m_sUser;
	UString m_sMessage;
};

#endif	// REPOTOOL_H_
