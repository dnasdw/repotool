#include "repotool.h"
#include "bitbucket.h"
#include "curl_holder.h"
#include "github.h"
#include "repo.h"

CRepoTool::SOption CRepoTool::s_Option[] =
{
	{ nullptr, 0, USTR("action:") },
	{ USTR("unpack"), 0, USTR("unpack input to output") },
	{ USTR("pack"), 0, USTR("pack input to output") },
	{ USTR("upload"), USTR('u'), USTR("upload local to remote") },
	{ USTR("download"), USTR('d'), USTR("download remote to local") },
	{ USTR("import"), 0, USTR("import repo") },
	{ USTR("remove"), 0, USTR("remove repo") },
	{ USTR("sample"), 0, USTR("show the samples") },
	{ USTR("help"), USTR('h'), USTR("show this help") },
	{ nullptr, 0, USTR("\ncommon:") },
	{ USTR("input"), USTR('i'), USTR("the input") },
	{ USTR("output"), USTR('o'), USTR("the output") },
	{ USTR("verbose"), USTR('v'), USTR("show the info") },
	{ USTR("type"), USTR('t'), USTR("[bitbucket|github]\n\t\tthe repository type") },
	{ USTR("workspace"), 0, USTR("the workspace of the repository") },
	{ USTR("user"), 0, USTR("the user for the repository") },
	{ nullptr, 0, USTR("\nupload:") },
	{ USTR("update-import"), 0, USTR("force update import if not complete") },
	{ nullptr, 0, USTR("\nimport:") },
	{ USTR("import-param"), 0, USTR("[0-9A-Fa-f]*\n\t\thex encrypted param for import action") },
	{ USTR("import-key"), 0, USTR("[\\x21-\\x7E]*\n\t\tkey for decrypt import param") },
	{ nullptr, 0, USTR("\nremove:") },
	{ USTR("remove-local-repo"), 0, USTR("remove local repo only") },
	{ USTR("remove-remote-user"), 0, USTR("remove remote user from data only") },
	{ nullptr, 0, nullptr }
};

CRepoTool::CRepoTool()
	: m_eAction(kActionNone)
	, m_bVerbose(false)
	, m_bUpdateImport(false)
	, m_bRemoveLocalRepo(false)
	, m_bRemoveRemoteUser(false)
{
}

CRepoTool::~CRepoTool()
{
}

int CRepoTool::ParseOptions(int a_nArgc, UChar* a_pArgv[])
{
	if (a_nArgc <= 1)
	{
		return 1;
	}
	for (int i = 1; i < a_nArgc; i++)
	{
		int nArgpc = static_cast<int>(UCslen(a_pArgv[i]));
		if (nArgpc == 0)
		{
			continue;
		}
		int nIndex = i;
		if (a_pArgv[i][0] != USTR('-'))
		{
			UPrintf(USTR("ERROR: illegal option\n\n"));
			return 1;
		}
		else if (nArgpc > 1 && a_pArgv[i][1] != USTR('-'))
		{
			for (int j = 1; j < nArgpc; j++)
			{
				switch (parseOptions(a_pArgv[i][j], nIndex, a_nArgc, a_pArgv))
				{
				case kParseOptionReturnSuccess:
					break;
				case kParseOptionReturnIllegalOption:
					UPrintf(USTR("ERROR: illegal option\n\n"));
					return 1;
				case kParseOptionReturnNoArgument:
					UPrintf(USTR("ERROR: no argument\n\n"));
					return 1;
				case kParseOptionReturnUnknownArgument:
					UPrintf(USTR("ERROR: unknown argument \"%") PRIUS USTR("\"\n\n"), m_sMessage.c_str());
					return 1;
				case kParseOptionReturnOptionConflict:
					UPrintf(USTR("ERROR: option conflict\n\n"));
					return 1;
				}
			}
		}
		else if (nArgpc > 2 && a_pArgv[i][1] == USTR('-'))
		{
			switch (parseOptions(a_pArgv[i] + 2, nIndex, a_nArgc, a_pArgv))
			{
			case kParseOptionReturnSuccess:
				break;
			case kParseOptionReturnIllegalOption:
				UPrintf(USTR("ERROR: illegal option\n\n"));
				return 1;
			case kParseOptionReturnNoArgument:
				UPrintf(USTR("ERROR: no argument\n\n"));
				return 1;
			case kParseOptionReturnUnknownArgument:
				UPrintf(USTR("ERROR: unknown argument \"%") PRIUS USTR("\"\n\n"), m_sMessage.c_str());
				return 1;
			case kParseOptionReturnOptionConflict:
				UPrintf(USTR("ERROR: option conflict\n\n"));
				return 1;
			}
		}
		i = nIndex;
	}
	return 0;
}

int CRepoTool::CheckOptions() const
{
	if (m_eAction == kActionNone)
	{
		UPrintf(USTR("ERROR: nothing to do\n\n"));
		return 1;
	}
	if (m_eAction == kActionUnpack || m_eAction == kActionPack || m_eAction == kActionUpload || m_eAction == kActionDownload)
	{
		if (m_sInputPath.empty())
		{
			UPrintf(USTR("ERROR: no --input option\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionRemove)
	{
		if (m_bRemoveRemoteUser)
		{
			if (m_sType.empty())
			{
				UPrintf(USTR("ERROR: no --type option\n\n"));
				return 1;
			}
			if (m_sUser.empty())
			{
				UPrintf(USTR("ERROR: no --user option\n\n"));
				return 1;
			}
		}
		else
		{
			if (m_sInputPath.empty())
			{
				UPrintf(USTR("ERROR: no --input option\n\n"));
				return 1;
			}
		}
	}
	if (m_eAction == kActionUnpack || m_eAction == kActionPack)
	{
		if (m_sOutputPath.empty())
		{
			UPrintf(USTR("ERROR: no --output option\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionUpload)
	{
		if (m_sType.empty())
		{
			UPrintf(USTR("ERROR: no --type option\n\n"));
			return 1;
		}
		if (m_sUser.empty())
		{
			UPrintf(USTR("ERROR: no --user option\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionImport)
	{
		if (m_sImportParam.empty())
		{
			UPrintf(USTR("ERROR: no --import-param option\n\n"));
			return 1;
		}
		if (m_sImportKey.empty())
		{
			UPrintf(USTR("ERROR: no --import-key option\n\n"));
			return 1;
		}
	}
	return 0;
}

int CRepoTool::Help() const
{
	UPrintf(USTR("repotool %") PRIUS USTR(" by dnasdw\n\n"), AToU(REPOTOOL_VERSION).c_str());
	UPrintf(USTR("usage: repotool [option...] [option]...\n\n"));
	UPrintf(USTR("option:\n"));
	SOption* pOption = s_Option;
	while (pOption->Name != nullptr || pOption->Doc != nullptr)
	{
		if (pOption->Name != nullptr)
		{
			UPrintf(USTR("  "));
			if (pOption->Key != 0)
			{
				UPrintf(USTR("-%c,"), pOption->Key);
			}
			else
			{
				UPrintf(USTR("   "));
			}
			UPrintf(USTR(" --%-8") PRIUS, pOption->Name);
			if (UCslen(pOption->Name) >= 8 && pOption->Doc != nullptr)
			{
				UPrintf(USTR("\n%16") PRIUS, USTR(""));
			}
		}
		if (pOption->Doc != nullptr)
		{
			UPrintf(USTR("%") PRIUS, pOption->Doc);
		}
		UPrintf(USTR("\n"));
		pOption++;
	}
	return 0;
}

int CRepoTool::Action() const
{
	if (m_eAction == kActionUnpack)
	{
		if (!unpack())
		{
			UPrintf(USTR("ERROR: unpack failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionPack)
	{
		if (!pack())
		{
			UPrintf(USTR("ERROR: pack failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionUpload)
	{
		if (!upload())
		{
			UPrintf(USTR("ERROR: upload failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionDownload)
	{
		if (!download())
		{
			UPrintf(USTR("ERROR: download failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionImport)
	{
		if (!import())
		{
			UPrintf(USTR("ERROR: import failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionRemove)
	{
		if (!remove())
		{
			UPrintf(USTR("ERROR: remove failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionSample)
	{
		return sample();
	}
	if (m_eAction == kActionHelp)
	{
		return Help();
	}
	return 0;
}

CRepoTool::EParseOptionReturn CRepoTool::parseOptions(const UChar* a_pName, int& a_nIndex, int a_nArgc, UChar* a_pArgv[])
{
	if (UCscmp(a_pName, USTR("unpack")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionUnpack;
		}
		else if (m_eAction != kActionUnpack && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("pack")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionPack;
		}
		else if (m_eAction != kActionPack && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("upload")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionUpload;
		}
		else if (m_eAction != kActionUpload && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("download")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionDownload;
		}
		else if (m_eAction != kActionDownload && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("import")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionImport;
		}
		else if (m_eAction != kActionImport && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("remove")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionRemove;
		}
		else if (m_eAction != kActionRemove && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("sample")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionSample;
		}
		else if (m_eAction != kActionSample && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("help")) == 0)
	{
		m_eAction = kActionHelp;
	}
	else if (UCscmp(a_pName, USTR("input")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sInputPath = a_pArgv[++a_nIndex];
		if (m_sInputPath.empty())
		{
			return kParseOptionReturnIllegalOption;
		}
	}
	else if (UCscmp(a_pName, USTR("output")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sOutputPath = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("verbose")) == 0)
	{
		m_bVerbose = true;
	}
	else if (UCscmp(a_pName, USTR("type")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sType = a_pArgv[++a_nIndex];
		if (m_sType != U8ToU(CBitbucket::s_sTypeName)
			&& m_sType != U8ToU(CGithub::s_sTypeName))
		{
			m_sMessage = m_sType;
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (UCscmp(a_pName, USTR("workspace")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sWorkspace = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("user")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sUser = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("update-import")) == 0)
	{
		m_bUpdateImport = true;
	}
	else if (UCscmp(a_pName, USTR("import-param")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		string sImportParam = UToU8(a_pArgv[++a_nIndex]);
		static regex c_rParam("[0-9A-Fa-f]*", regex_constants::ECMAScript | regex_constants::icase);
		if (!regex_match(sImportParam, c_rParam) || sImportParam.size() % 2 != 0)
		{
			m_sMessage = U8ToU(sImportParam);
			return kParseOptionReturnUnknownArgument;
		}
		m_sImportParam = sImportParam;
	}
	else if (UCscmp(a_pName, USTR("import-key")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		string sImportKey = UToU8(a_pArgv[++a_nIndex]);
		static regex c_rKey("[\x21-\x7E]*", regex_constants::ECMAScript | regex_constants::icase);
		if (!regex_match(sImportKey, c_rKey))
		{
			m_sMessage = U8ToU(sImportKey);
			return kParseOptionReturnUnknownArgument;
		}
		m_sImportKey = sImportKey;
	}
	else if (UCscmp(a_pName, USTR("remove-local-repo")) == 0)
	{
		if (m_bRemoveRemoteUser)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_bRemoveLocalRepo = true;
	}
	else if (UCscmp(a_pName, USTR("remove-remote-user")) == 0)
	{
		if (m_bRemoveLocalRepo)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_bRemoveRemoteUser = true;
	}
	return kParseOptionReturnSuccess;
}

CRepoTool::EParseOptionReturn CRepoTool::parseOptions(int a_nKey, int& a_nIndex, int a_nArgc, UChar* a_pArgv[])
{
	for (SOption* pOption = s_Option; pOption->Name != nullptr || pOption->Key != 0 || pOption->Doc != nullptr; pOption++)
	{
		if (pOption->Key == a_nKey)
		{
			return parseOptions(pOption->Name, a_nIndex, a_nArgc, a_pArgv);
		}
	}
	return kParseOptionReturnIllegalOption;
}

bool CRepoTool::unpack() const
{
	CRepo repo;
	repo.SetInputPath(m_sInputPath);
	repo.SetOutputPath(m_sOutputPath);
	repo.SetVerbose(m_bVerbose);
	bool bResult = repo.Unpack();
	return bResult;
}

bool CRepoTool::pack() const
{
	CRepo repo;
	repo.SetInputPath(m_sInputPath);
	repo.SetOutputPath(m_sOutputPath);
	repo.SetVerbose(m_bVerbose);
	bool bResult = repo.Pack();
	return bResult;
}

bool CRepoTool::upload() const
{
	bool bInitialized = CCurlGlobalHolder::Initialize();
	if (!bInitialized)
	{
		UPrintf(USTR("ERROR: curl initialize failed\n\n"));
		return false;
	}
	CRepo repo;
	repo.SetInputPath(m_sInputPath);
	repo.SetType(m_sType);
	if (!m_sWorkspace.empty())
	{
		repo.SetWorkspace(m_sWorkspace);
	}
	else
	{
		repo.SetWorkspace(m_sUser);
	}
	repo.SetUser(m_sUser);
	repo.SetUpdateImport(m_bUpdateImport);
	repo.SetVerbose(m_bVerbose);
	bool bResult = repo.Upload();
	CCurlGlobalHolder::Finalize();
	return bResult;
}

bool CRepoTool::download() const
{
	CRepo repo;
	repo.SetInputPath(m_sInputPath);
	repo.SetType(m_sType);
	if (!m_sWorkspace.empty())
	{
		repo.SetWorkspace(m_sWorkspace);
	}
	else
	{
		repo.SetWorkspace(m_sUser);
	}
	repo.SetUser(m_sUser);
	repo.SetVerbose(m_bVerbose);
	bool bResult = repo.Download();
	return bResult;
}

bool CRepoTool::import() const
{
	bool bInitialized = CCurlGlobalHolder::Initialize();
	if (!bInitialized)
	{
		UPrintf(USTR("ERROR: curl initialize failed\n\n"));
		return false;
	}
	CRepo repo;
	repo.SetImportParam(m_sImportParam);
	repo.SetImportKey(m_sImportKey);
	repo.SetVerbose(m_bVerbose);
	bool bResult = repo.Import();
	CCurlGlobalHolder::Finalize();
	return bResult;
}

bool CRepoTool::remove() const
{
	bool bInitialized = CCurlGlobalHolder::Initialize();
	if (!bInitialized)
	{
		UPrintf(USTR("ERROR: curl initialize failed\n\n"));
		return false;
	}
	CRepo repo;
	repo.SetInputPath(m_sInputPath);
	repo.SetType(m_sType);
	if (!m_sWorkspace.empty())
	{
		repo.SetWorkspace(m_sWorkspace);
	}
	else
	{
		repo.SetWorkspace(m_sUser);
	}
	repo.SetUser(m_sUser);
	repo.SetRemoveLocalRepo(m_bRemoveLocalRepo);
	repo.SetRemoveRemoteUser(m_bRemoveRemoteUser);
	repo.SetVerbose(m_bVerbose);
	bool bResult = repo.Remove();
	CCurlGlobalHolder::Finalize();
	return bResult;
}

int CRepoTool::sample() const
{
	UPrintf(USTR("sample:\n"));
	UPrintf(USTR("# unpack file(s)\n"));
	UPrintf(USTR("repotool -v --unpack -i \"D:\\github\\upload\" -o \"/github/upload\"\n\n"));
	UPrintf(USTR("# pack file(s)\n"));
	UPrintf(USTR("repotool -v --pack -i \"/github/upload\" -o \"D:\\github\\upload_pack\"\n\n"));
	UPrintf(USTR("# upload file(s)\n"));
	UPrintf(USTR("repotool -v --upload -i \"/github/upload\" -t bitbucket --user someone\n\n"));
	UPrintf(USTR("# download file(s)\n"));
	UPrintf(USTR("repotool -v --download -i \"/github/upload\"\n\n"));
	UPrintf(USTR("# remove file(s)\n"));
	UPrintf(USTR("repotool -v --remove -i \"/github/upload\"\n\n"));
	UPrintf(USTR("# remove local repo file(s) only\n"));
	UPrintf(USTR("repotool -v --remove --remove-local-repo -i \"/github/upload\"\n\n"));
	UPrintf(USTR("# remove remote user from remote.txt file(s) only\n"));
	UPrintf(USTR("repotool -v --remove --remove-remote-user -t github --user someone\n\n"));
	return 0;
}

int UMain(int argc, UChar* argv[])
{
	CRepoTool tool;
	if (tool.ParseOptions(argc, argv) != 0)
	{
		return tool.Help();
	}
	if (tool.CheckOptions() != 0)
	{
		return 1;
	}
	return tool.Action();
}
