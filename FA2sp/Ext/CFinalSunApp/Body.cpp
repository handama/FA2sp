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
#include <filesystem>
#include "../../Helpers/Translations.h"
#include "../../Miscs/VoxelDrawer.h"
#include "../CLoading/Body.h"
#include "../../Miscs/DialogStyle.h"
namespace fs = std::filesystem;

#pragma warning(disable : 6262)

std::vector<std::string> CFinalSunAppExt::RecentFilesExt;
bool CFinalSunAppExt::HoldingKey = false;
std::array<std::pair<std::string, std::string>, 7> CFinalSunAppExt::ExternalLinks
{
	std::make_pair("https://github.com/secsome/FA2sp", ""),
	std::make_pair("https://github.com/handama/FA2sp", ""),
	std::make_pair("https://phobos.readthedocs.io/zh-cn/latest/", ""),
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
	// Nothing yet huh...

	std::string path;
	path = CFinalSunApp::ExePath;
	path += "\\FAData.ini";
	CINI::FAData->ClearAndLoad(path.c_str());

	if (auto pSection = CINI::FAData().GetSection("Include"))
	{
		for (auto& pair : pSection->GetEntities())
		{
			std::string includePath;
			includePath = CFinalSunApp::ExePath;
			includePath += "\\" + pair.second;
			if (fs::exists(includePath))
				CINI::FAData->ParseINI(includePath.c_str(), 0, 0);
		}
	}

	path = CFinalSunApp::ExePath;
	path += "\\FALanguage.ini";
	CINI::FALanguage->ClearAndLoad(path.c_str());
	if (auto pSection = CINI::FALanguage().GetSection("Include"))
	{
		for (auto& pair : pSection->GetEntities())
		{
			std::string includePath;
			includePath = CFinalSunApp::ExePath;
			includePath += "\\" + pair.second;
			if (fs::exists(includePath))
				CINI::FALanguage->ParseINI(includePath.c_str(), 0, 0);
		}
	}
	// No need to validate falanguage I guess

	FA2sp::ExtConfigsInitialize(); // ExtConfigs
	VoxelDrawer::Initalize();

	if (ExtConfigs::EnableVisualStyle)
	{
		// GetModuleName
		char ModuleNameBuffer[MAX_PATH];
		GetModuleFileName(static_cast<HMODULE>(FA2sp::hInstance), ModuleNameBuffer, MAX_PATH);
		int nLength = strlen(ModuleNameBuffer);
		int i = nLength - 1;
		for (; i >= 0; --i)
		{
			if (ModuleNameBuffer[i] == '\\')
				break;
		}
		++i;
		int nModuleNameLen = nLength - i;
		memcpy(ModuleNameBuffer, ModuleNameBuffer + i, nModuleNameLen);
		ModuleNameBuffer[nModuleNameLen] = '\0';

		// Codes from 
		// https://referencesource.microsoft.com/#System.Windows.Forms/winforms/Managed/System/WinForms/UnsafeNativeMethods.cs,8197
		ACTCTX enableThemingActivationContext;
		enableThemingActivationContext.cbSize = sizeof ACTCTX;
		enableThemingActivationContext.lpSource = ModuleNameBuffer; // "FA2sp.dll"
		enableThemingActivationContext.lpResourceName = (LPCSTR)101;
		enableThemingActivationContext.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
		HANDLE hActCtx = ::CreateActCtx(&enableThemingActivationContext);
		if (hActCtx != INVALID_HANDLE_VALUE)
		{
			if (::ActivateActCtx(hActCtx, &FA2sp::ulCookie))
				Logger::Put("Visual Style Enabled!\n");
		}

		ACTCTX actCtx = { sizeof(ACTCTX) };
		actCtx.dwFlags = ACTCTX_FLAG_SET_PROCESS_DEFAULT;
		actCtx.lpSource = NULL; 
		actCtx.lpResourceName = (LPCSTR)101;

		const char* tempManifest = "<?xml version='1.0' encoding='UTF-8' standalone='yes'?>"
			"<assembly xmlns='urn:schemas-microsoft-com:asm.v1' manifestVersion='1.0'>"
			"<dependency>"
			"<dependentAssembly>"
			"<assemblyIdentity type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*' />"
			"</dependentAssembly>"
			"</dependency>"
			"</assembly>";

		char tempPath[MAX_PATH];
		GetTempPathA(MAX_PATH, tempPath);
		char manifestPath[MAX_PATH];
		wsprintfA(manifestPath, "%s\\fa2_manifest.xml", tempPath);
		HANDLE hFile = CreateFileA(manifestPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			DWORD written;
			WriteFile(hFile, tempManifest, strlen(tempManifest), &written, NULL);
			CloseHandle(hFile);
		}

		actCtx.lpSource = manifestPath;
		HANDLE hActCtxEx = CreateActCtxA(&actCtx);
		if (hActCtxEx != INVALID_HANDLE_VALUE) {
			BOOL activated = ActivateActCtx(hActCtxEx, &FA2sp::ulCookieEx);
			if (!activated) {
				ReleaseActCtx(hActCtxEx);
			}
			else {
				INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES };
				InitCommonControlsEx(&icc);
			}
		}
	}
	
	CINI ini;
	path = CFinalSunApp::ExePath;
	path += "\\FinalAlert.ini";

	bool firstRun = !fs::exists(path);

	ini.ClearAndLoad(path.c_str());
	std::string installpath = std::string(ini.GetString("TS", "Exe"));
	installpath = installpath.substr(0, installpath.find_last_of("\\") + 1);

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

	this->InstallPath = ini.GetString("TS", "Exe");
	this->FileSearchLikeTS = ini.GetBool("FinalSun", "FileSearchLikeTS");
	this->Language = ini.GetString("FinalSun", "Language");

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

	if (ExtConfigs::LoadImageDataFromServer)
	{
		CLoadingExt::PipeName = "\\\\.\\pipe\\ImagePipe_" + std::to_string(GetCurrentProcessId());

		std::thread([]() {
			CLoadingExt::StartImageServerProcess();
			}).detach();
	}


	CFinalSunDlg dlg(nullptr);
	this->m_pMainWnd = &dlg;

	dlg.DoModal();

	is_watcher_running = false; // stop watcher
	watcher.join(); // wait for thread exit
	DarkTheme::CleanupDarkThemeBrushes();
	if (ExtConfigs::LoadImageDataFromServer)
	{
		CLoadingExt::PingServerRunning = false;
		CLoadingExt::SendRequestText("QUIT_PROGRAME");
	}

	return FALSE;
}