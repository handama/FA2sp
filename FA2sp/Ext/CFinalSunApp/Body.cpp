#include "Body.h"

#include <CFinalSunDlg.h>
#include <CINI.h>
#include <CLoading.h>
#include "../../FA2sp.h"
#include "../../Miscs/FileWatcher.h"
#include "../../Miscs/SaveMap.h"
#include <thread>
#include <iostream>
#include <future>
#include <string.h>
#include <filesystem>
#include "../../Helpers/Translations.h"
#include "../../Miscs/VoxelDrawer.h"
#include "../CLoading/Body.h"
#include "../../Miscs/DialogStyle.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Miscs/Hooks.INI.h"
#include "../CFinalSunDlg/Body.h"
#include <wininet.h>
#include "../../FA2sp.Constants.h"

namespace fs = std::filesystem;

#pragma warning(disable : 6262)

std::vector<std::string> CFinalSunAppExt::RecentFilesExt;
bool CFinalSunAppExt::HoldingKey = false;
bool CFinalSunAppExt::HasNewVersion = false;
FString CFinalSunAppExt::NewVersion;
FString CFinalSunAppExt::ExePathExt;
FString CFinalSunAppExt::LauncherName;
std::array<std::pair<std::string, std::string>, 7> CFinalSunAppExt::ExternalLinks
{
	std::make_pair("https://github.com/secsome/FA2sp", ""),
	std::make_pair("https://github.com/handama/FA2sp", ""),
	std::make_pair("https://phobos.readthedocs.io/en/latest/", ""),
	std::make_pair("https://www.ppmforums.com/", ""),
	std::make_pair("https://modenc.renegadeprojects.com/Main_Page", ""),
	std::make_pair("https://ra2map.com/", ""),
	std::make_pair("", "")
};

CFinalSunAppExt* CFinalSunAppExt::GetInstance()
{
	return static_cast<CFinalSunAppExt*>(&CFinalSunApp::Instance.get());
}

BOOL CFinalSunAppExt::InitInstanceExt()
{
	this->AccTable = ::LoadAccelerators(static_cast<HINSTANCE>(FA2sp::hInstance), MAKEINTRESOURCE(0x81));

	HWND hDesktop = ::GetDesktopWindow();
	HDC hDC = ::GetDC(hDesktop);
	if (::GetDeviceCaps(hDC, BITSPIXEL) <= 8)
	{

		ppmfc::CString pMessage = Translations::TranslateOrDefault("EightBitStart",
			"You currently only have 8 bit color mode enabled. "
			"FinalAlert 2(tm)will not work in 8 bit color mode. "
			"See readme.txt for further information!");

		::MessageBox(
			NULL,
			pMessage,
			Translations::TranslateOrDefault("Error", "Error"),
			MB_OK
		);
		exit(0);
	}
	
	CFinalSunDlg::SE2KMODE = FALSE; // We don't need SE2K stuff
	CFinalSunApp::MapPath[0] = '\0';
	// Now let's parse the command line
	ParseCommandLine(CFinalSunApp::Instance->m_lpCmdLine);
	
	CFinalSunAppExt::ExePathExt = CFinalSunApp::ExePath();

	((CINIExt*)&CINI::FAData())->LoadFASetting("FAData.ini");
	((CINIExt*)&CINI::FALanguage())->LoadFASetting("FALanguage.ini");

	FA2sp::ExtConfigsInitialize(); // ExtConfigs
	VoxelDrawer::Initalize();

	std::string newGamePath;
	if (ExtConfigs::LoadGameFromMapFolder_OnInit && strlen(CFinalSunApp::MapPath()))
	{
		newGamePath = CFinalSunApp::MapPath();
		newGamePath = newGamePath.substr(0, newGamePath.find_last_of("\\") + 1);
		if (!fs::exists(newGamePath + "ra2.mix"))
		{
			newGamePath.clear();
		}
	}

	CINI ini;
	std::string path;
	path = CFinalSunAppExt::ExePathExt;
	path += "\\FinalAlert.ini";

	bool firstRun = !fs::exists(path);

	ini.ClearAndLoad(path.c_str());
	std::string installpath = std::string(ini.GetString("TS", "Exe"));
	installpath = installpath.substr(0, installpath.find_last_of("\\") + 1);
	if (!newGamePath.empty())
		installpath = newGamePath;

	if (!ExtConfigs::DisableDirectoryCheck)
	{
		while (
			!ini.KeyExists("TS", "Exe") ||
			!ini.KeyExists("FinalSun", "FileSearchLikeTS") ||
			!ini.KeyExists("FinalSun", "Language") ||
			!fs::exists(installpath + "ra2.mix")
			)
		{
			ppmfc::CString pMessage = Translations::TranslateOrDefault("LoadMixError.WrongDirectory",
				"The game directory is incorrect. Do you want to reset?");

			int result = IDYES;

			if (!firstRun)
				result = MessageBox(NULL, pMessage, Translations::TranslateOrDefault("FatalError", "Fatal Error"), MB_YESNO | MB_ICONEXCLAMATION);


			if (result == IDYES)
			{
				firstRun = false;
				this->FileSearchLikeTS = TRUE;
				*reinterpret_cast<int*>(0x7EE07C) = TRUE;
				this->GetDialog()->Settings();
				*reinterpret_cast<int*>(0x7EE07C) = FALSE;

			}
			if (result == IDNO)
			{
				exit(EXIT_SUCCESS);
				break;
			}

			ini.ClearAndLoad(path.c_str());
			installpath = ini.GetString("TS", "Exe");
			installpath = installpath.substr(0, installpath.find_last_of("\\") + 1);

		}
	}

	this->InstallPath = newGamePath.empty() ? ini.GetString("TS", "Exe").m_pchData : newGamePath;
	this->FileSearchLikeTS = ini.GetBool("FinalSun", "FileSearchLikeTS");
	if (ini.KeyExists("FinalSun", "LanguageSP"))
	{
		auto lang = ini.GetString("FinalSun", "LanguageSP");
		this->Language = lang;
		FinalAlertConfig::Language = lang;
	}
	else
	{
		auto lang = ini.GetString("FinalSun", "Language");
		this->Language = lang;
		FinalAlertConfig::Language = lang;
	}

	// HACK, Game like pls
	this->FileSearchLikeTS = TRUE;

	// No graphics, no need for them in fact
	this->NoAircraftGraphics = ini.GetBool("Graphics", "NoAircraftGraphics");
	this->NoVehicleGraphics = ini.GetBool("Graphics", "NoVehicleGraphics");
	this->NoBuildingGraphics = ini.GetBool("Graphics", "NoBuildingGraphics");
	this->NoInfantryGraphics = ini.GetBool("Graphics", "NoInfantryGraphics");
	this->NoTreeGraphics = ini.GetBool("Graphics", "NoTreeGraphics");
	this->NoSnowGraphics = ini.GetBool("Graphics", "NoSnowGraphics");
	this->NoTemperateGraphics = ini.GetBool("Graphics", "NoTemperateGraphics");
	this->NoBMPs = ini.GetBool("Graphics", "NoBMPs");
	this->NoOverlayGraphics = ini.GetBool("Graphics", "NoOverlayGraphics");

	// User interface
	this->DisableAutoShore = ini.GetBool("UserInterface", "DisableAutoShore");
	this->DisableAutoLat = ini.GetBool("UserInterface", "DisableAutoLat");
	this->NoSounds = ini.GetBool("UserInterface", "NoSounds");
	this->DisableSlopeCorrection = ini.GetBool("UserInterface", "DisableSlopeCorrection");
	this->ShowBuildingCells = ini.GetBool("UserInterface", "ShowBuildingCells");

	ExtConfigs::TreeViewCameo_Display = ini.GetBool("UserInterface", "ShowTreeViewCameo");
	ExtConfigs::DisableAutoConnectWall = ini.GetBool("UserInterface", "DisableAutoConnectWall");
	
	// recent files
	this->RecentFilesExt.resize(ExtConfigs::RecentFileLimit);
	for (size_t i = 0; i < this->RecentFilesExt.size(); ++i)
		this->RecentFilesExt[i] = ini.GetString("Files", std::format("{0:d}", i).c_str());

	if (this->NoTemperateGraphics && this->NoSnowGraphics)
	{
		ppmfc::CString pMessage = Translations::TranslateOrDefault("NoTemperateSnowGraphics",
			"You have turned off loading of both snow and temperate terrain in 'FinalAlert.ini'. "
			"At least one of these must be loaded. The application will now quit.");

		::MessageBox(
			NULL,
			pMessage,
			Translations::TranslateOrDefault("Error", "Error"),
			MB_OK);
		exit(0xFFFFFC2A);
	}

	// No easy mode
	this->EasyMode = FALSE;

	// Process file path
	FA2sp::Buffer = this->InstallPath;
	FA2sp::Buffer.SetAt(FA2sp::Buffer.ReverseFind('\\') + 1, '\0');
	strcpy_s(CFinalSunApp::FilePath, 260, FA2sp::Buffer);
	
	// Others
	CLoading loading(nullptr);
	this->Loading = &loading;

	bool is_watcher_running = true;
	std::thread watcher([&is_watcher_running]()
		{
			FileWatcher fw(
				CFinalSunApp::MapPath(), 
				std::chrono::milliseconds{1000}, 
				is_watcher_running, 
				SaveMapExt::SaveTime
			);
			fw.start(fw.Callback);
		}
	);

	std::thread([]() {
		CheckUpdate();
	}).detach();

	CFinalSunDlg dlg(nullptr);
	this->m_pMainWnd = &dlg;

	dlg.DoModal();

	is_watcher_running = false; // stop watcher
	watcher.join(); // wait for thread exit

	return FALSE;
}

void CFinalSunAppExt::ParseCommandLine(const char* cmdLine)
{
	FString line(cmdLine);
	auto encoding = STDHelpers::GetFileEncoding((uint8_t*)line.data(), line.size());
	if (encoding == FileEncoding::UTF8 || encoding == FileEncoding::UTF8_BOM)
		line.toANSI();
	std::vector<std::string> args;
	if (!cmdLine || !*cmdLine) return;

	const char* p = line.data();
	std::string current;

	while (*p) {
		while (*p && std::isspace(static_cast<unsigned char>(*p))) ++p;

		if (!*p) break;

		current.clear();

		if (*p == '"' || *p == '\'') {
			char quote = *p;
			++p;
			while (*p && *p != quote) {
				current += *p;
				++p;
			}
			if (*p == quote) ++p;
		}
		else {
			while (*p && !std::isspace(static_cast<unsigned char>(*p))) {
				current += *p;
				++p;
			}
		}

		if (!current.empty()) {
			args.push_back(std::move(current));
		}
	}

	std::string output_file;

	for (size_t i = 0; i < args.size(); ++i) {
		std::string_view arg = args[i];

		if (arg == "-o" || arg == "--open") {
			if (i + 1 < args.size()) output_file = args[++i];
		}
		else if (arg == "-x") {
			if (i + 1 < args.size()) {
				LauncherName = args[++i];
				LauncherName.Replace("\"", "");
			}
		}
	}

	if (output_file.size() < MAX_PATH && std::filesystem::exists(output_file))
	{
		strcpy_s(CFinalSunApp::MapPath(), output_file.c_str());
	}
}

#pragma comment(lib, "wininet.lib")
std::string HttpGetSimple(const std::wstring& url)
{
	std::string result;

	HINTERNET hInternet = InternetOpenW(
		L"Mozilla/5.0",
		INTERNET_OPEN_TYPE_PRECONFIG,
		NULL, NULL, 0);

	if (!hInternet) return result;

	HINTERNET hFile = InternetOpenUrlW(
		hInternet,
		url.c_str(),
		NULL, 0,
		INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE,
		0);

	if (!hFile) {
		InternetCloseHandle(hInternet);
		return result;
	}

	char buffer[4096];
	DWORD bytesRead = 0;

	while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead)
	{
		result.append(buffer, bytesRead);
	}

	InternetCloseHandle(hFile);
	InternetCloseHandle(hInternet);

	return result;
}

Version ParseVersionFromJson(const std::string& json)
{
	Version v{};

	size_t pos = json.find("\"tag_name\"");
	if (pos == std::string::npos)
		return v;

	size_t start = json.find("\"", pos + 10);
	if (start == std::string::npos)
		return v;

	start++;

	size_t end = json.find("\"", start);
	if (end == std::string::npos)
		return v;

	std::string tag = json.substr(start, end - start);

	size_t numStart = tag.find_first_of("0123456789");
	if (numStart == std::string::npos)
		return v;

	std::string versionPart = tag.substr(numStart);

	sscanf_s(versionPart.c_str(), "%d.%d.%d",
		&v.major, &v.minor, &v.revision);

	return v;
}

bool IsNewer(const Version& local, const Version& remote)
{
	if (remote.major != local.major)
		return remote.major > local.major;

	if (remote.minor != local.minor)
		return remote.minor > local.minor;

	return remote.revision > local.revision;
}

void CFinalSunAppExt::CheckUpdate()
{
	std::string json = HttpGetSimple(
		L"https://api.github.com/repos/handama/fa2sp/releases/latest"
	);
	if (json.empty())
		return;

	Version remote = ParseVersionFromJson(json);
	Version local = { HE_PRODUCT_MAJOR, HE_PRODUCT_MINOR, HE_PRODUCT_REVISION };

	if (IsNewer(local, remote))
	{
		HasNewVersion = true;
		NewVersion.Format("%d.%d.%d", remote.major, remote.minor, remote.revision);

		if (!CMapData::Instance->MapWidthPlusHeight)
		{
			ppmfc::CString caption;
			CFinalSunDlg::Instance->GetWindowTextA(caption);
			FString title(caption);
			title += " - ";
			title += Translations::TranslateOrDefault("NewVersionAvailable", "New version available:");
			title += " ";
			title += CFinalSunAppExt::NewVersion;
			CFinalSunDlg::Instance->SetWindowTextA(title);
		}
	}
}