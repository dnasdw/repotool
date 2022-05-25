#include "repotool.h"
#include "bitbucket.h"
#include "curl_holder.h"
#include "repo.h"

CRepoTool::SOption CRepoTool::s_Option[] =
{
	{ nullptr, 0, USTR("action:") },
	{ USTR("unpack"), 0, USTR("unpack input to output") },
	{ USTR("pack"), 0, USTR("pack input to output") },
	{ USTR("upload"), USTR('u'), USTR("upload local to remote") },
	{ USTR("download"), USTR('d'), USTR("download remote to local") },
	{ USTR("sample"), 0, USTR("show the samples") },
	{ USTR("help"), USTR('h'), USTR("show this help") },
	{ nullptr, 0, USTR("\ncommon:") },
	{ USTR("input"), USTR('i'), USTR("the input") },
	{ USTR("output"), USTR('o'), USTR("the output") },
	{ USTR("verbose"), USTR('v'), USTR("show the info") },
	{ nullptr, 0, USTR("\nupload:") },
	{ USTR("type"), USTR('t'), USTR("[bitbucket]\n\t\tthe repository type") },
	{ USTR("workspace"), 0, USTR("the workspace of the repository") },
	{ USTR("user"), 0, USTR("the user for the repository") },
	{ nullptr, 0, nullptr }
};

CRepoTool::CRepoTool()
	: m_eAction(kActionNone)
	, m_bVerbose(false)
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

int CRepoTool::CheckOptions()
{
	if (m_eAction == kActionNone)
	{
		UPrintf(USTR("ERROR: nothing to do\n\n"));
		return 1;
	}
	if (m_eAction != kActionSample && m_eAction != kActionHelp)
	{
		if (m_sInputPath.empty())
		{
			UPrintf(USTR("ERROR: no --input option\n\n"));
			return 1;
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
	return 0;
}

int CRepoTool::Help()
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

int CRepoTool::Action()
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
		if (m_sType != U8ToU(CBitbucket::s_sTypeName))
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

bool CRepoTool::unpack()
{
	CRepo repo;
	repo.SetInputPath(m_sInputPath);
	repo.SetOutputPath(m_sOutputPath);
	repo.SetVerbose(m_bVerbose);
	bool bResult = repo.Unpack();
	return bResult;
}

bool CRepoTool::pack()
{
	CRepo repo;
	repo.SetInputPath(m_sInputPath);
	repo.SetOutputPath(m_sOutputPath);
	repo.SetVerbose(m_bVerbose);
	bool bResult = repo.Pack();
	return bResult;
}

bool CRepoTool::upload()
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
	repo.SetVerbose(m_bVerbose);
	bool bResult = repo.Upload();
	CCurlGlobalHolder::Finalize();
	return bResult;
}

bool CRepoTool::download()
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

int CRepoTool::sample()
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
